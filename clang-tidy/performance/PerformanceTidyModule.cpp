//===--- PeformanceTidyModule.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "ContainerDefaultInitializerCheck.h"
#include "FasterStringFindCheck.h"
#include "ForRangeCopyCheck.h"
#include "ImplicitCastInLoopCheck.h"
#include "InefficientSharedPointerReferenceCheck.h"
#include "InefficientStreamUseCheck.h"
#include "InefficientStringConcatenationCheck.h"
#include "TypePromotionInMathFnCheck.h"
#include "UnnecessaryCopyInitialization.h"
#include "UnnecessaryValueParamCheck.h"

namespace clang {
namespace tidy {
namespace performance {

class PerformanceModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ContainerDefaultInitializerCheck>(
        "performance-container-default-initializer");
    CheckFactories.registerCheck<FasterStringFindCheck>(
        "performance-faster-string-find");
    CheckFactories.registerCheck<ForRangeCopyCheck>(
        "performance-for-range-copy");
    CheckFactories.registerCheck<ImplicitCastInLoopCheck>(
        "performance-implicit-cast-in-loop");
    CheckFactories.registerCheck<InefficientSharedPointerReferenceCheck>(
        "performance-inefficient-shared-pointer-reference");
    CheckFactories.registerCheck<InefficientStreamUseCheck>(
        "performance-inefficient-stream-use");
    CheckFactories.registerCheck<InefficientStringConcatenationCheck>(
        "performance-inefficient-string-concatenation");
    CheckFactories.registerCheck<TypePromotionInMathFnCheck>(
        "performance-type-promotion-in-math-fn");
    CheckFactories.registerCheck<UnnecessaryCopyInitialization>(
        "performance-unnecessary-copy-initialization");
    CheckFactories.registerCheck<UnnecessaryValueParamCheck>(
        "performance-unnecessary-value-param");
  }
};

// Register the PerformanceModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<PerformanceModule>
    X("performance-module", "Adds performance checks.");

} // namespace performance

// This anchor is used to force the linker to link in the generated object file
// and thus register the PerformanceModule.
volatile int PerformanceModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
