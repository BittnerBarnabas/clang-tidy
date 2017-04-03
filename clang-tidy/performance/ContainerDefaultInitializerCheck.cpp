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

std::map<const Decl *, StringRef> DeclarationMap{};

static SmallVector<StringRef, 5> getInsertionArguments(const MatchFinder::MatchResult &Result,
                                                       const CallExpr *InsertFunctionCall) {

  SmallVector<StringRef, 5> ArgumentsAsString;
  for (size_t I = 0, ArgCount = InsertFunctionCall->getNumArgs(); I < ArgCount; ++I) {
    const Expr *Expr = InsertFunctionCall->getArg(I);
    ArgumentsAsString.push_back(Lexer::getSourceText(
        CharSourceRange::getTokenRange(Expr->getLocStart(), Expr->getLocEnd()),
        *Result.SourceManager, Result.Context->getLangOpts()));

  }
  return ArgumentsAsString;
}

template<unsigned N>
static void formatArguments(const SmallVector<StringRef, N> ArgumentList, llvm::raw_ostream &Stream) {
  StringRef Delimiter = "";
  for (const auto &Tokens : ArgumentList) {
    Stream << Delimiter << Tokens;
    Delimiter = ", ";
  }
}

void ContainerDefaultInitializerCheck::registerMatchers(MatchFinder *Finder) {
  const auto HasOperationsNamedDecl = hasDeclaration(namedDecl(matchesName(OperationsToMatchRegex)));
  const auto ContainterType = qualType(hasDeclaration(namedDecl(matchesName(ContainersToMatchRegex))));

  const auto
      DeclRefExprToContainer = declRefExpr(hasDeclaration(varDecl(hasType(ContainterType)).bind("containerDecl")));

  const auto MemberCallExpr = cxxMemberCallExpr(HasOperationsNamedDecl,
                                                on(DeclRefExprToContainer)).bind("memberCallExpr");

  Finder->addMatcher(compoundStmt(forEach(exprWithCleanups(has(MemberCallExpr)))).bind("compoundStmt"),
                     this);
}

void ContainerDefaultInitializerCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *ContainerDeclaration = Result.Nodes.getNodeAs<Decl>("containerDecl");
  const auto *MemberCallExpression = Result.Nodes.getNodeAs<CXXMemberCallExpr>("memberCallExpr");
  const auto *CompoundStatement = Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");

  const auto *CompoundStmtIterator = CompoundStatement->body_begin();

  while (dyn_cast<DeclStmt>(*CompoundStmtIterator) == nullptr ? true :
         dyn_cast<DeclStmt>(*CompoundStmtIterator)->getSingleDecl() != ContainerDeclaration) {
    CompoundStmtIterator++;
  }

  CompoundStmtIterator++;
  if (auto *ptr = dyn_cast<ExprWithCleanups>(*CompoundStmtIterator)) {
    if (isa<CXXMemberCallExpr>(ptr->getSubExpr())) {
      std::string Brf;
      llvm::raw_string_ostream Tokens{Brf};
      formatArguments(getInsertionArguments(Result, MemberCallExpression), Tokens);

      diag(MemberCallExpression->getExprLoc(), Tokens.str());
    }
  }
}

} // namespace performance
} // namespace tidy
} // namespace clang
