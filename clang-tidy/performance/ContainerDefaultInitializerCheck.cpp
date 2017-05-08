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
    "std::.*vector|std::.*map|std::.*deque|std::.*set";

std::set<const Decl *> ProcessedDeclarations{};

enum class InsertionType { EMPLACE, STANDARD };
enum class ContainerType { MAP, OTHER };

template <unsigned int N> class InsertionCall {
  SmallVector<std::string, N> CallArguments;
  InsertionType InsertionCallType;
  QualType CallQualType;
  ContainerType ContainerType;

public:
  InsertionCall(const SmallVector<std::string, N> &CallArguments,
                InsertionType CallType, const QualType &CallQualType,
                const enum ContainerType &ContainerType)
      : CallArguments(CallArguments), InsertionCallType(CallType),
        CallQualType(CallQualType), ContainerType(ContainerType) {}
  const SmallVector<std::string, N> &getCallArguments() const {
    return CallArguments;
  }
  InsertionType getCallType() const { return InsertionCallType; }
  const QualType &getCallQualType() const { return CallQualType; }
  const enum ContainerType &getContainerType() const { return ContainerType; }
};

static SourceRange
getRangeWithFollowingToken(const MatchFinder::MatchResult &Result,
                           const Expr *Expression) {
  SourceRange Range{Expression->getSourceRange()};
  Token Tok;
  Lexer::getRawToken(Expression->getLocEnd().getLocWithOffset(1), Tok,
                     *Result.SourceManager,
                     Result.Context->getLangOpts(), /*ignore WS*/
                     true);
  Range.setEnd(Tok.getEndLoc());
  return Range;
}

static std::string getNarrowingCastStr(const QualType &CastSource,
                                       const QualType &CastDestination) {
  if (CastSource.getAsString() == CastDestination.getAsString())
    return "";
  if (CastDestination.getTypePtr()->isBuiltinType()) {
    return "(" + CastDestination.getAsString() + ")";
  }
  return "";
}

static bool isTernaryOperator(const Expr *Expression) {
  return isa<ConditionalOperator>(Expression->IgnoreImplicit());
}

static void getExprAsStr(const Expr *Expr, std::string &ArgAsString,
                         const std::string &SrcTextForExpr) {
  if (isTernaryOperator(Expr)) {
    ArgAsString.append("(").append(SrcTextForExpr).append(")");
  } else {
    ArgAsString.append(SrcTextForExpr);
  }
}
template <unsigned int N>
static InsertionCall<N>
getInsertionArguments(const MatchFinder::MatchResult &Result,
                      const CallExpr *InsertCallExpr,
                      const TemplateSpecializationType *TemplateArguments,
                      ContainerType ContainerType) {

  const auto getSourceTextForExpr = [&](const Expr *Expression) {
    return Lexer::getSourceText(
        CharSourceRange::getTokenRange(Expression->getLocStart(),
                                       Expression->getLocEnd()),
        *Result.SourceManager, Result.Context->getLangOpts());
  };

  SmallVector<std::string, N> ArgumentsAsString;
  InsertionType InsertionCallType = InsertionType::STANDARD;
  if (const auto *InsertFuncDecl = InsertCallExpr->getDirectCallee()) {
    if (InsertFuncDecl->getName().contains("emplace")) {
      InsertionCallType = InsertionType::EMPLACE;
    }
  }

  for (size_t I = 0, ArgCount = InsertCallExpr->getNumArgs(); I < ArgCount;
       ++I) {
    const auto *Expr = InsertCallExpr->getArg((unsigned int)I);
    std::string ArgAsString;
    switch (ContainerType) {
    case ContainerType::MAP:
      if (InsertionCallType == InsertionType::EMPLACE) {
        const auto ArgCastStr =
            getNarrowingCastStr(Expr->IgnoreImplicit()->getType(),
                                TemplateArguments->getArg((int)I).getAsType());
        ArgAsString = ArgCastStr;
        getExprAsStr(Expr, ArgAsString, getSourceTextForExpr(Expr));
      } else {
        ArgAsString = getSourceTextForExpr(Expr);
      }
      break;
    case ContainerType::OTHER:
      if (InsertionCallType == InsertionType::EMPLACE &&
          !isa<BuiltinType>(
              TemplateArguments->getArg(0).getAsType().getTypePtr())) {
        ArgAsString = getSourceTextForExpr(Expr).str();
      } else {
        const auto ArgCast = getNarrowingCastStr(
            Expr->IgnoreImplicit()->getType().getCanonicalType(),
            TemplateArguments->getArg(0).getAsType());
        ArgAsString = ArgCast;
        const auto SrcTextForExpr = getSourceTextForExpr(Expr).str();
        getExprAsStr(Expr, ArgAsString, SrcTextForExpr);
      }
      break;
    }
    ArgumentsAsString.push_back(ArgAsString);
  }

  return InsertionCall<N>(ArgumentsAsString, InsertionCallType,
                          TemplateArguments->getArg(0).getAsType(),
                          ContainerType);
}

static FixItHint
getRefactoredContainerDecl(const VarDecl *ContainerDecl,
                           llvm::raw_string_ostream &TokenStream) {
  if (ContainerDecl->getInitStyle() == VarDecl::InitializationStyle::CallInit) {
    return FixItHint::CreateInsertion(
        ContainerDecl->getLocEnd().getLocWithOffset(
            (int)ContainerDecl->getName().size()),
        (Twine{'{'} + TokenStream.str() + Twine{'}'}).str());
  }
  return FixItHint::CreateInsertion(ContainerDecl->getLocEnd(),
                                    TokenStream.str());
}

template <unsigned N>
static void formatArguments(const InsertionCall<N> ArgumentList,
                            llvm::raw_ostream &Stream) {
  StringRef Delimiter = "";
  const auto IsMapType = ArgumentList.getContainerType() == ContainerType::MAP;
  const auto IsEmplaceCall =
      ArgumentList.getCallType() == InsertionType::EMPLACE;
  const auto IsArgumentBuiltInType =
      isa<BuiltinType>(ArgumentList.getCallQualType().getTypePtr());
  if (IsMapType && IsEmplaceCall) {
    Stream << "{";
  } else if (IsEmplaceCall && !IsArgumentBuiltInType) {
    const auto &ArgumentType = ArgumentList.getCallQualType();
    if (const auto *RecordTypeCall =
            dyn_cast<RecordType>(ArgumentType.getTypePtr())) {
      Stream << RecordTypeCall->getDecl()->getName();
    } else {
      Stream << ArgumentType.getAsString();
    }
    Stream << '(';
  }
  for (const auto &Tokens : ArgumentList.getCallArguments()) {
    Stream << Delimiter << Tokens;
    Delimiter = ", ";
  }
  if (IsMapType && IsEmplaceCall) {
    Stream << "}";
  } else if (IsEmplaceCall && !IsArgumentBuiltInType) {
    Stream << ')';
  }
}

static bool isInsertionCall(const CallExpr *CallExpr) {
  if (CallExpr == nullptr)
    return false;
  const auto *InsertionMethodDecl =
      dyn_cast<CXXMethodDecl>(CallExpr->getCalleeDecl());
  if (!InsertionMethodDecl)
    return false;
  llvm::Regex Reg(OperationsToMatchRegex);
  return Reg.match(InsertionMethodDecl->getName());
}

const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          UnresolvedMemberExpr>
    unresolvedMemberExpr;

AST_MATCHER_P(UnresolvedMemberExpr, onBase,
              ast_matchers::internal::Matcher<Expr>, InnerMatcher) {
  const Expr *ExprNode = Node.getBase()->IgnoreParenImpCasts();
  return (ExprNode != nullptr &&
          InnerMatcher.matches(*ExprNode, Finder, Builder));
}

void ContainerDefaultInitializerCheck::registerMatchers(MatchFinder *Finder) {
  // This checker only makes sense for C++11 and up.
  if (!getLangOpts().CPlusPlus11)
    return;

  const auto HasOperationsNamedDecl =
      hasDeclaration(namedDecl(matchesName(OperationsToMatchRegex)));
  const auto ContainterType =
      classTemplateSpecializationDecl(matchesName(ContainersToMatchRegex));

  const auto DefaultConstructor = cxxConstructExpr(
      hasDeclaration(cxxConstructorDecl(isDefaultConstructor())));

  const auto DeclRefExprToContainer = declRefExpr(
      hasDeclaration(varDecl(hasType(ContainterType), has(DefaultConstructor))
                         .bind("containerDecl")));

  const auto MemberCallExpr =
      cxxMemberCallExpr(HasOperationsNamedDecl, on(DeclRefExprToContainer))
          .bind("memberCallExpr");

  const auto MemberCallExprWithRefToContainer =
      cxxMemberCallExpr(
          HasOperationsNamedDecl,
          hasAnyArgument(expr(hasDescendant(DeclRefExprToContainer))),
          on(DeclRefExprToContainer))
          .bind("dirtyMemberCallExpr");

  const auto UnresolvedMemberExpr =
      callExpr(has(unresolvedMemberExpr(onBase(DeclRefExprToContainer))))
          .bind("unresolvedCallExpr");

  const auto HasInstantiatedDeclAncestor = hasAncestor(decl(isInstantiated()));

  Finder->addMatcher(
      compoundStmt(unless(HasInstantiatedDeclAncestor),
                   forEach(expr(ignoringImplicit(UnresolvedMemberExpr))))
          .bind("compoundStmt"),
      this);

  Finder->addMatcher(compoundStmt(unless(HasInstantiatedDeclAncestor),
                                  forEach(expr(ignoringImplicit(
                                      MemberCallExprWithRefToContainer))))
                         .bind("compoundStmt"),
                     this);

  Finder->addMatcher(
      compoundStmt(unless(HasInstantiatedDeclAncestor),
                   forEach(expr(ignoringImplicit(MemberCallExpr))))
          .bind("compoundStmt"),
      this);
}

void ContainerDefaultInitializerCheck::check(
    const MatchFinder::MatchResult &Result) {
  std::vector<int> vec1(1, 2);
  const auto *ContainerDeclaration =
      Result.Nodes.getNodeAs<VarDecl>("containerDecl");
  if (ProcessedDeclarations.find(ContainerDeclaration) !=
      ProcessedDeclarations.end())
    return;

  const auto *CompoundStatement =
      Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");
  const auto *DirtyMemberCall =
      Result.Nodes.getNodeAs<CXXMemberCallExpr>("dirtyMemberCallExpr");
  const auto *CompoundStmtIterator = CompoundStatement->body_begin();
  const auto *UnresolvedCallExpr =
      Result.Nodes.getNodeAs<CallExpr>("unresolvedCallExpr");

  const TemplateSpecializationType *ContainerTemplateParameters;
  const auto *ContainerType = ContainerDeclaration->getType().getTypePtr();
  if (isa<TemplateSpecializationType>(ContainerType)) {
    ContainerTemplateParameters =
        dyn_cast<TemplateSpecializationType>(ContainerType);
  } else {
    ContainerTemplateParameters = dyn_cast<TemplateSpecializationType>(
        ContainerType->getLocallyUnqualifiedSingleStepDesugaredType()
            .getTypePtr());
  }

  const auto pointsToOriginalContainerDecl = [&](const Stmt *const Iter) {
    if (const auto *DeclStatement = dyn_cast<DeclStmt>(Iter)) {
      if (DeclStatement->isSingleDecl()) {
        return DeclStatement->getSingleDecl() == ContainerDeclaration;
      }
    }
    return false;
  };

  while (!pointsToOriginalContainerDecl(*CompoundStmtIterator)) {
    CompoundStmtIterator++;
  }

  CompoundStmtIterator++;

  std::string Brf;
  llvm::raw_string_ostream Tokens{Brf};
  auto HasInsertionCall = false;
  StringRef Delimiter = "";
  SmallVector<FixItHint, 5> FixitHints{};

  while (CompoundStmtIterator != CompoundStatement->body_end()) {
    const CXXMemberCallExpr *ActualMemberCallExpr;
    if (!(ActualMemberCallExpr = dyn_cast<CXXMemberCallExpr>(
              (*CompoundStmtIterator)->IgnoreImplicit())) ||
        !isInsertionCall(ActualMemberCallExpr) ||
        ActualMemberCallExpr == UnresolvedCallExpr ||
        ActualMemberCallExpr == DirtyMemberCall ||
        ActualMemberCallExpr->getImplicitObjectArgument()
                ->getReferencedDeclOfCallee() != ContainerDeclaration) {
      break;
    }

    Tokens << Delimiter;
    Delimiter = ", ";

    const auto BaseContainerType =
        StringRef(ContainerDeclaration->getType().getAsString()).contains("map")
            ? ContainerType::MAP
            : ContainerType::OTHER;

    formatArguments(getInsertionArguments<5>(Result, ActualMemberCallExpr,
                                             ContainerTemplateParameters,
                                             BaseContainerType),
                    Tokens);
    HasInsertionCall = true;
    FixitHints.push_back(FixItHint::CreateRemoval(
        /* get the range with tailing semicolon */
        getRangeWithFollowingToken(Result, ActualMemberCallExpr)));
    CompoundStmtIterator++;
  }

  if (HasInsertionCall) {
    auto DiagnosticBuilder =
        diag(ContainerDeclaration->getLocStart(),
             "Initialize containers in place if you can")
        << getRefactoredContainerDecl(ContainerDeclaration, Tokens);
    for (const auto &Fixit : FixitHints) {
      DiagnosticBuilder << Fixit;
    }
  }

  ProcessedDeclarations.insert(ContainerDeclaration);
}

} // namespace performance
} // namespace tidy
} // namespace clang
