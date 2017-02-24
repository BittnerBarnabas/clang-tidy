//===--- InefficientSharedPointerReferenceCheck.cpp - clang-tidy-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InefficientSharedPointerReferenceCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

void InefficientSharedPointerReferenceCheck::registerMatchers(MatchFinder *Finder) {
  const auto SharedPointerType =
      qualType(hasDeclaration(classTemplateSpecializationDecl(matchesName("std::__1::shared_ptr"))));
  const auto InefficientParameter =
      cxxBindTemporaryExpr(has(implicitCastExpr(hasCastKind(CK_ConstructorConversion))), hasType(SharedPointerType));
  Finder->addMatcher(callExpr(hasDescendant(InefficientParameter.bind("shared_ptr_parameter")),
                              callee(functionDecl().bind("function"))), this);
}

void InefficientSharedPointerReferenceCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *SharedPtrParameter = Result.Nodes.getNodeAs<CXXBindTemporaryExpr>("shared_ptr_parameter");
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("function");

  if (SharedPtrParameter && Function) {
    diag(SharedPtrParameter->getExprLoc(), "inefficient shared_ptr cast");
    diag(Function->getSourceRange().getBegin(), "consider using const reference or raw pointer instead of shared_ptr");
  }
}

} // namespace performance
} // namespace tidy
} // namespace clang
