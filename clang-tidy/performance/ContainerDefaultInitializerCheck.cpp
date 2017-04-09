//===--- ContainerDefaultInitializerCheck.cpp - clang-tidy-----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <clang/Lex/Lexer.h>
#include "ContainerDefaultInitializerCheck.h"
using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

static const auto OperationsToMatchRegex = "push_back|emplace|emplace_back";

static const auto ContainersToMatchRegex = "std::.*vector|std::.*map|std::.*deque";

std::set<const Decl *> ProcessedDeclarations{};

enum class InsertionType { EMPLACE, STANDARD };

template<unsigned int N>
struct InsertionCall {
  SmallVector<StringRef, N> CallArguments;
  InsertionType CallType;
};

static SourceRange getRangeWithSemicolon(const MatchFinder::MatchResult &Result, const Expr *Expression) {
  SourceRange Range{Expression->getSourceRange()};
  int Offset = 1;
  while (true) {
    SourceLocation OffsetLocationEnd{
        Expression->getLocEnd().getLocWithOffset(Offset)};
    SourceRange RangeForString{OffsetLocationEnd};

    CharSourceRange CSR = Lexer::makeFileCharRange(
        CharSourceRange::getTokenRange(RangeForString),
        *Result.SourceManager,
        Result.Context->getLangOpts());

    const auto SourceSnippet =
        Lexer::getSourceText(CSR, *Result.SourceManager,
                             Result.Context->getLangOpts());

    if (SourceSnippet == ";") {
      Range.setEnd(OffsetLocationEnd);
      return Range;
    } else {
      Offset++;
    }
  }
}

template<unsigned int N>
static InsertionCall<N> getInsertionArguments(const MatchFinder::MatchResult &Result,
                                              const CallExpr *InsertCallExpr) {
  SmallVector<StringRef, N> ArgumentsAsString;
  InsertionCall<N> InsertionCall;
  if (const auto *InsertFuncDecl = InsertCallExpr->getDirectCallee()) {
    const auto InsertFuncName = InsertFuncDecl->getName();
    if (InsertFuncName.contains("emplace")) {
      InsertionCall.CallType = InsertionType::EMPLACE;
    } else {
      InsertionCall.CallType = InsertionType::STANDARD;
    }
  }
  for (size_t I = 0, ArgCount = InsertCallExpr->getNumArgs(); I < ArgCount; ++I) {
    const Expr *Expr = InsertCallExpr->getArg((unsigned int) I);
    ArgumentsAsString.push_back(Lexer::getSourceText(
        CharSourceRange::getTokenRange(Expr->getLocStart(), Expr->getLocEnd()),
        *Result.SourceManager, Result.Context->getLangOpts()));

  }
  InsertionCall.CallArguments = ArgumentsAsString;
  return InsertionCall;
}

template<unsigned N>
static void formatArguments(const InsertionCall<N> ArgumentList, llvm::raw_ostream &Stream) {
  StringRef Delimiter = "";
  if (ArgumentList.CallType == InsertionType::EMPLACE)
    Stream << '{';
  for (const auto &Tokens : ArgumentList.CallArguments) {
    Stream << Delimiter << Tokens;
    Delimiter = ", ";
  }
  if (ArgumentList.CallType == InsertionType::EMPLACE)
    Stream << '}';
}

void ContainerDefaultInitializerCheck::registerMatchers(MatchFinder *Finder) {
  const auto HasOperationsNamedDecl = hasDeclaration(namedDecl(matchesName(OperationsToMatchRegex)));
  const auto ContainterType = qualType(hasDeclaration(namedDecl(matchesName(ContainersToMatchRegex))));

  const auto
      DeclRefExprToContainer = declRefExpr(hasDeclaration(varDecl(hasType(ContainterType),
                                                                  has(cxxConstructExpr(hasDeclaration(
                                                                      cxxConstructorDecl(
                                                                          isDefaultConstructor()))))).bind(
      "containerDecl")));

  const auto MemberCallExpr = cxxMemberCallExpr(HasOperationsNamedDecl,
                                                on(DeclRefExprToContainer)).bind("memberCallExpr");

  Finder->addMatcher(compoundStmt(forEach(exprWithCleanups(has(MemberCallExpr)))).bind("compoundStmt"),
                     this);
}

void ContainerDefaultInitializerCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *ContainerDeclaration = Result.Nodes.getNodeAs<VarDecl>("containerDecl");
  if (ProcessedDeclarations.find(ContainerDeclaration) != ProcessedDeclarations.end())
    return;
  const auto *CompoundStatement = Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");

  const auto *CompoundStmtIterator = CompoundStatement->body_begin();

  //This finds the matched container declaration.
  while (dyn_cast<DeclStmt>(*CompoundStmtIterator) == nullptr ? true :
         dyn_cast<DeclStmt>(*CompoundStmtIterator)->getSingleDecl() != ContainerDeclaration) {
    CompoundStmtIterator++;
  }

  CompoundStmtIterator++;

  std::string Brf;
  llvm::raw_string_ostream Tokens{Brf};
  auto HasInsertionCall = false;
  StringRef Delimiter = "";

  SmallVector<FixItHint, 5> FixitHints{};

  while (auto *ptr = dyn_cast<ExprWithCleanups>(*CompoundStmtIterator)) {
    auto *FirstMemberCallExpr = dyn_cast<CXXMemberCallExpr>(ptr->getSubExpr());
    if (FirstMemberCallExpr
        && FirstMemberCallExpr->getImplicitObjectArgument()->getReferencedDeclOfCallee() == ContainerDeclaration) {
      Tokens << Delimiter;
      Delimiter = ", ";
      formatArguments(getInsertionArguments<5>(Result, FirstMemberCallExpr), Tokens);
      HasInsertionCall = true;
      FixitHints.push_back(FixItHint::CreateRemoval(getRangeWithSemicolon(Result, FirstMemberCallExpr)));
    } else {
      break;
    }
    CompoundStmtIterator++;
  }

  if (HasInsertionCall) {
    auto DiagnosticBuilder = diag(ContainerDeclaration->getLocStart(), "Initialize containers in place you bastard")
        << FixItHint::CreateInsertion(ContainerDeclaration->getInit()->getLocEnd().getLocWithOffset(
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
