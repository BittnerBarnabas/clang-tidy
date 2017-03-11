//===--- InefficientSharedPointerReferenceCheck.h - clang-tidy---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_INEFFICIENT_SHARED_POINTER_REFERENCE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_INEFFICIENT_SHARED_POINTER_REFERENCE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace performance {

/// Implicit casting an std::shared_ptr<Derived> to std::shared_ptr<Base> in a function call is highly inefficient (where Base is a superclass of Derived).
/// Instead of doing so one should consider using const & or raw pointers instead.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/performance-inefficient-shared-pointer-reference.html
class InefficientSharedPointerReferenceCheck : public ClangTidyCheck {
public:
  InefficientSharedPointerReferenceCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace performance
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_INEFFICIENT_SHARED_POINTER_REFERENCE_H
