set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyPerformanceModule
  ContainerDefaultInitializerCheck.cpp
  FasterStringFindCheck.cpp
  ForRangeCopyCheck.cpp
  ImplicitCastInLoopCheck.cpp
  InefficientStreamUseCheck.cpp
  InefficientStringConcatenationCheck.cpp
  PerformanceTidyModule.cpp
  TypePromotionInMathFnCheck.cpp
  UnnecessaryCopyInitialization.cpp
  UnnecessaryValueParamCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
