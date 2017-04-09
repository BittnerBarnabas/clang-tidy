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

template<unsigned int N>
static InsertionCall<N> getInsertionArguments(const MatchFinder::MatchResult &Result,
                                                       const CallExpr *InsertCallExpr) {
  SmallVector<StringRef, N> ArgumentsAsString;
  InsertionCall<N> InsertionCall;
  if(const auto* InsertFuncDecl = InsertCallExpr->getDirectCallee()) {
    const auto InsertFuncName = InsertFuncDecl->getName();
    if(InsertFuncName.contains("emplace")) {
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
  if(ArgumentList.CallType == InsertionType::EMPLACE) Stream << '{';
  for (const auto &Tokens : ArgumentList.CallArguments) {
    Stream << Delimiter << Tokens;
    Delimiter = ", ";
  }
  if(ArgumentList.CallType == InsertionType::EMPLACE) Stream << '}';
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
  const auto *MemberCallExpression = Result.Nodes.getNodeAs<CXXMemberCallExpr>("memberCallExpr");
  const auto *CompoundStatement = Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");

  const auto *CompoundStmtIterator = CompoundStatement->body_begin();

  //This finds the matched container declaration.
  while (dyn_cast<DeclStmt>(*CompoundStmtIterator) == nullptr ? true :
         dyn_cast<DeclStmt>(*CompoundStmtIterator)->getSingleDecl() != ContainerDeclaration) {
    CompoundStmtIterator++;
  }

  CompoundStmtIterator++;
  if (auto *ptr = dyn_cast<ExprWithCleanups>(*CompoundStmtIterator)) {
    auto *FirstMemberCallExpr = dyn_cast<CXXMemberCallExpr>(ptr->getSubExpr());
    if (FirstMemberCallExpr
        && FirstMemberCallExpr->getImplicitObjectArgument()->getReferencedDeclOfCallee() == ContainerDeclaration) {
      std::string Brf;
      llvm::raw_string_ostream Tokens{Brf};
      formatArguments(getInsertionArguments<5>(Result, MemberCallExpression), Tokens);

      diag(MemberCallExpression->getExprLoc(), Tokens.str())
          << FixItHint::CreateInsertion(ContainerDeclaration->getInit()->getLocEnd().getLocWithOffset(
              (int) ContainerDeclaration->getName().size()), (Twine{'{'} + Tokens.str() + Twine{'}'}).str());

      ProcessedDeclarations.insert(ContainerDeclaration);
    }
  }
}

} // namespace performance
} // namespace tidy
} // namespace clang
