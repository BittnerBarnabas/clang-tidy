//===--- ContainerDefaultInitializerCheck.h - clang-tidy---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_CONTAINER_DEFAULT_INITIALIZER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_CONTAINER_DEFAULT_INITIALIZER_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace performance {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/performance-container-default-initializer.html
class ContainerDefaultInitializerCheck : public ClangTidyCheck {
public:
  ContainerDefaultInitializerCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace performance
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PERFORMANCE_CONTAINER_DEFAULT_INITIALIZER_H
