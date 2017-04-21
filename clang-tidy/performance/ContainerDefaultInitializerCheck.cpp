//===--- ContainerDefaultInitializerCheck.cpp - clang-tidy-----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ContainerDefaultInitializerCheck.h"
#include <clang/Lex/Lexer.h>
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

static const auto OperationsToMatchRegex =
    "push_back|emplace|emplace_back|insert";

static const auto ContainersToMatchRegex =
    "std::.*vector|std::.*map|std::.*deque";

static const std::set <StringRef> IntegralTypes =
    {"bool", "char", "short", "unsigned short", "int", "unsigned int", "long", "unsigned long", "long long",
     "unsigned long long"};

static const std::set <StringRef> FloatingTypes = {"float", "double", "long double"};

std::set<const Decl *> ProcessedDeclarations{};
enum class InsertionType { EMPLACE, STANDARD };
enum class ContainerType { MAP, OTHER };

template<unsigned int N> struct InsertionCall {
  SmallVector <std::string, N> CallArguments;
  InsertionType CallType;
  QualType CallQualType;
  ContainerType ContainerType;
  std::string TypeAsString;
};

static SourceRange getRangeWithSemicolon(const MatchFinder::MatchResult &Result,
                                         const Expr *Expression) {
  SourceRange Range{Expression->getSourceRange()};
  int Offset = 1;
  while (true) {
    SourceLocation OffsetLocationEnd{
        Expression->getLocEnd().getLocWithOffset(Offset)};
    SourceRange RangeForString{OffsetLocationEnd};

    CharSourceRange CSR = Lexer::makeFileCharRange(
        CharSourceRange::getTokenRange(RangeForString), *Result.SourceManager,
        Result.Context->getLangOpts());

    const auto SourceSnippet = Lexer::getSourceText(
        CSR, *Result.SourceManager, Result.Context->getLangOpts());

    if (SourceSnippet == ";") {
      Range.setEnd(OffsetLocationEnd);
      return Range;
    } else {
      Offset++;
    }
  }
}

static std::string getNarrowingCastStr(const QualType &CastSource, const QualType &CastDestination) {
  if (CastSource.getAsString() == CastDestination.getAsString())
    return "";
  const auto DestinationStr = CastDestination.getAsString();
  if (IntegralTypes.find(DestinationStr) != IntegralTypes.end()
      || FloatingTypes.find(DestinationStr) != FloatingTypes.end()) {
    return "(" + DestinationStr + ")";
  }
  return "";
}

template<unsigned int N>
static InsertionCall<N>
getInsertionArguments(const MatchFinder::MatchResult &Result,
                      const CallExpr *InsertCallExpr,
                      const TemplateArgumentList &TemplateArguments,
                      ContainerType ContainerType) {

  const auto getSourceTextForExpr = [&](const Expr *Expression) {
    return Lexer::getSourceText(
        CharSourceRange::getTokenRange(Expression->getLocStart(), Expression->getLocEnd()),
        *Result.SourceManager, Result.Context->getLangOpts());
  };

  SmallVector <std::string, N> ArgumentsAsString;
  InsertionCall<N> InsertionCall;
  InsertionCall.ContainerType = ContainerType;
  if (const auto *InsertFuncDecl = InsertCallExpr->getDirectCallee()) {
    const auto InsertFuncName = InsertFuncDecl->getName();
    if (InsertFuncName.contains("emplace")) {
      InsertionCall.CallType = InsertionType::EMPLACE;
      InsertionCall.CallQualType = TemplateArguments[0].getAsType();
    } else {
      InsertionCall.CallType = InsertionType::STANDARD;
    }
  }

  for (size_t I = 0, ArgCount = InsertCallExpr->getNumArgs(); I < ArgCount;
       ++I) {
    const auto *Expr = InsertCallExpr->getArg((unsigned int) I);

    std::string ArgCastStrRef1;
    std::string ArgCastStrRef2;
    std::string ArgAsString;
    switch (ContainerType) {
    case ContainerType::MAP:
      if (const auto *PairConstructorExpr = dyn_cast<CXXConstructExpr>(Expr->IgnoreImplicit())) {
        const auto *KeyExpr = PairConstructorExpr->getArg(0);
        const auto *ValueExpr = PairConstructorExpr->getArg(1);

        ArgCastStrRef1 = getNarrowingCastStr(KeyExpr->getType().getCanonicalType(), TemplateArguments[0].getAsType());
        ArgCastStrRef2 =
            getNarrowingCastStr(ValueExpr->getType().getCanonicalType(), TemplateArguments[1].getAsType());

        const auto Arg1Str = getSourceTextForExpr(KeyExpr);
        const auto Arg2Str = getSourceTextForExpr(ValueExpr);

        ArgAsString = "{" + ArgCastStrRef1 + Arg1Str.str() + ", " + ArgCastStrRef2 + Arg2Str.str() + "}";
      }
      break;
    case ContainerType::OTHER:
      if (InsertionCall.CallType == InsertionType::EMPLACE) {
        ArgAsString = getSourceTextForExpr(Expr).str();
      } else {
        ArgCastStrRef1 = getNarrowingCastStr(Expr->IgnoreImplicit()->getType().getCanonicalType(),
                                             TemplateArguments[0].getAsType());
        ArgAsString = ArgCastStrRef1 + getSourceTextForExpr(Expr).str();
      }
      break;
    }
    ArgumentsAsString.push_back(ArgAsString);
  }

  InsertionCall.CallArguments = ArgumentsAsString;
  return InsertionCall;
}

template<unsigned N>
static void formatArguments(const InsertionCall<N> ArgumentList,
                            llvm::raw_ostream &Stream) {
  StringRef Delimiter = "";
  const auto IsMapType = ArgumentList.ContainerType == ContainerType::MAP;
  const auto IsEmplaceCall = ArgumentList.CallType == InsertionType::EMPLACE;
  if (IsMapType && IsEmplaceCall) {
    Stream << "{";
  } else if (IsEmplaceCall) {
    if (const auto *RecordT = dyn_cast<RecordType>(ArgumentList.CallQualType.getTypePtr())) {
      Stream << RecordT->getDecl()->getName().str();
    }
    Stream << '(';
  }
  for (const auto &Tokens : ArgumentList.CallArguments) {
    Stream << Delimiter << Tokens;
    Delimiter = ", ";
  }
  if (IsMapType && IsEmplaceCall) {
    Stream << "}";
  } else if (IsEmplaceCall) {
    Stream << ')';
  }
}

void ContainerDefaultInitializerCheck::registerMatchers(MatchFinder *Finder) {
  const auto HasOperationsNamedDecl =
      hasDeclaration(namedDecl(matchesName(OperationsToMatchRegex)));
  const auto ContainterType =
      classTemplateSpecializationDecl(matchesName(ContainersToMatchRegex))
          .bind("containerTemplateSpecialization");

  const auto DefaultConstructor = cxxConstructExpr(
      hasDeclaration(cxxConstructorDecl(isDefaultConstructor())));

  const auto DeclRefExprToContainer = declRefExpr(
      hasDeclaration(varDecl(hasType(ContainterType), has(DefaultConstructor))
                         .bind("containerDecl")));

  const auto MemberCallExpr =
      cxxMemberCallExpr(HasOperationsNamedDecl, on(DeclRefExprToContainer))
          .bind("memberCallExpr");

  const auto MemberCallExpr2 =
      cxxMemberCallExpr(
          HasOperationsNamedDecl,
          hasAnyArgument(expr(hasDescendant(DeclRefExprToContainer))),
          on(DeclRefExprToContainer))
          .bind("dirtyMemberCallExpr");

  Finder->addMatcher(
      compoundStmt(eachOf(forEach(MemberCallExpr2), forEach(exprWithCleanups(has(MemberCallExpr2)))))
          .bind("compoundStmt"),
      this);
  Finder->addMatcher(
      compoundStmt(eachOf(forEach(MemberCallExpr), forEach(exprWithCleanups(has(MemberCallExpr)))))
          .bind("compoundStmt"),
      this);
}

void ContainerDefaultInitializerCheck::check(
    const MatchFinder::MatchResult &Result) {

  const auto *ContainerDeclaration =
      Result.Nodes.getNodeAs<VarDecl>("containerDecl");
  if (ProcessedDeclarations.find(ContainerDeclaration) !=
      ProcessedDeclarations.end())
    return;

  const auto *CompoundStatement =
      Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");
  const auto *ContainerTemplateSpec =
      Result.Nodes.getNodeAs<ClassTemplateSpecializationDecl>(
          "containerTemplateSpecialization");
  const auto *DirtyMemberCall =
      Result.Nodes.getNodeAs<CXXMemberCallExpr>("dirtyMemberCallExpr");
  const auto *CompoundStmtIterator = CompoundStatement->body_begin();

  // This finds the matched container declaration.
  while (!dyn_cast<DeclStmt>(*CompoundStmtIterator)
      || dyn_cast<DeclStmt>(*CompoundStmtIterator)->getSingleDecl() != ContainerDeclaration) {
    CompoundStmtIterator++;
  }

  CompoundStmtIterator++;

  std::string Brf;
  llvm::raw_string_ostream Tokens{Brf};
  auto HasInsertionCall = false;
  StringRef Delimiter = "";

  SmallVector<FixItHint, 5> FixitHints{};

  const auto &ContainerTemplateType =
      ContainerTemplateSpec->getTemplateArgs();

  while (CompoundStmtIterator != CompoundStatement->body_end()) {
    const CXXMemberCallExpr *ActualMemberCallExpr;
    if (const auto *ptr = dyn_cast<ExprWithCleanups>(*CompoundStmtIterator)) {
      ActualMemberCallExpr = dyn_cast<CXXMemberCallExpr>(ptr->getSubExpr());
    } else if (!(ActualMemberCallExpr = dyn_cast<CXXMemberCallExpr>(*CompoundStmtIterator))) {
      break;
    }

    if (ActualMemberCallExpr && ActualMemberCallExpr != DirtyMemberCall &&
        ActualMemberCallExpr->getImplicitObjectArgument()
            ->getReferencedDeclOfCallee() == ContainerDeclaration) {
      Tokens << Delimiter;
      Delimiter = ", ";
      formatArguments(getInsertionArguments<5>(Result,
                                               ActualMemberCallExpr,
                                               ContainerTemplateType,
                                               StringRef(ContainerDeclaration->getType().getAsString()).contains("map")
                                               ? ContainerType::MAP : ContainerType::OTHER),
                      Tokens);
      HasInsertionCall = true;
      FixitHints.push_back(FixItHint::CreateRemoval(
          getRangeWithSemicolon(Result, ActualMemberCallExpr)));
    } else {
      break;
    }
    CompoundStmtIterator++;
  }

  if (HasInsertionCall) {
    auto DiagnosticBuilder = diag(ContainerDeclaration->getLocStart(), "Initialize containers in place if you can")
        << FixItHint::CreateInsertion(ContainerDeclaration->getLocEnd().getLocWithOffset(
            (int) ContainerDeclaration->getName().size()), (Twine{'{'} + Tokens.str() + Twine{'}'}).str());
    for (const auto &Fixit : FixitHints) {
      DiagnosticBuilder << Fixit;
    }
  }

  ProcessedDeclarations.insert(ContainerDeclaration);
}

} // namespace performance
} // namespace tidy
} // namespace clang
