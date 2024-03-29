//===--- InefficientSharedPointerReferenceCheck.cpp - clang-tidy-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InefficientSharedPointerReferenceCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

static std::string getBaseTypeAsString(const Type *Type) {
  const TemplateSpecializationType *AsTemplateSpecT;

  if (const auto *ElaboratedT = dyn_cast<ElaboratedType>(Type)) {
    AsTemplateSpecT = dyn_cast<TemplateSpecializationType>(
        ElaboratedT->getLocallyUnqualifiedSingleStepDesugaredType());
  } else {
    AsTemplateSpecT = dyn_cast<TemplateSpecializationType>(Type);
  }

  if (!AsTemplateSpecT)
    return "";

  for (unsigned I = 0, ArgCount = AsTemplateSpecT->getNumArgs(); I < ArgCount;
       ++I) {
    if (const auto *Record = dyn_cast<RecordType>(
            AsTemplateSpecT->getArg(I).getAsType().getTypePtr())) {
      return Record->getDecl()->getName().str();
    }
  }

  return "";
}

static unsigned getCallExprArgNum(const CallExpr *CallExpr, const Expr *Expr) {
  for (unsigned I = 0, ArgCount = CallExpr->getNumArgs(); I < ArgCount; ++I) {
    if (Expr == CallExpr->getArg(I)) {
      return I;
    }
  }
  return 0;
}

static inline auto getQualTypeForTemplate(const std::string &str)
    -> decltype(qualType()) {
  return qualType(
      hasDeclaration(classTemplateSpecializationDecl(matchesName(str))));
}

void InefficientSharedPointerReferenceCheck::registerMatchers(
    MatchFinder *Finder) {
  // This checker only makes sense for C++11 and up.
  if (!getLangOpts().CPlusPlus11)
    return;

  // The *. regex is needed because shared_ptr could be in a namespace inside
  // std.
  const auto SharedPointerType = getQualTypeForTemplate("std::.*shared_ptr");
  const auto UniquePointerType = getQualTypeForTemplate("std::.*unique_ptr");

  const auto InefficientParameter =
      expr(ignoringImplicit(hasDescendant(
               implicitCastExpr(hasCastKind(CK_ConstructorConversion),
                                unless(has(cxxConstructExpr(hasAnyArgument(
                                    expr(hasType(UniquePointerType)))))))
                   .bind("impCast"))),
           hasType(SharedPointerType));

  Finder->addMatcher(
      callExpr(has(InefficientParameter.bind("shared_ptr_parameter")),
               callee(functionDecl().bind("function")))
          .bind("callExpr"),
      this);
}

void InefficientSharedPointerReferenceCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *SharedPtrParameter =
      Result.Nodes.getNodeAs<Expr>("shared_ptr_parameter");
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("function");
  const auto *ImpCast = Result.Nodes.getNodeAs<ImplicitCastExpr>("impCast");
  const auto *CallExpression = Result.Nodes.getNodeAs<CallExpr>("callExpr");

  const auto ArgumentNum =
      getCallExprArgNum(CallExpression, SharedPtrParameter->IgnoreImplicit());

  const auto *ConstructorConversion =
      dyn_cast<CXXConstructExpr>(ImpCast->getSubExpr());

  const auto *ConvertingConstructorTemplateParam =
      dyn_cast<RecordType>(ConstructorConversion->getConstructor()
                               ->getAsFunction()
                               ->getTemplateSpecializationArgs()
                               ->get(0)
                               .getAsType()
                               .getTypePtr());
  const auto DerivedType =
      ConvertingConstructorTemplateParam->getDecl()->getName().str();

  const auto BaseType =
      getBaseTypeAsString(SharedPtrParameter->getType().getTypePtr());

  diag(SharedPtrParameter->getExprLoc(),
       "inefficient polymorphic cast of std::shared_ptr<" + DerivedType +
           "> to std::shared_ptr<" + BaseType + ">");
  diag(Function->getParamDecl(ArgumentNum)->getLocStart(),
       "consider using const reference "
       "or raw pointer instead of "
       "'std::shared_ptr'",
       DiagnosticIDs::Note);
}

} // namespace performance
} // namespace tidy
} // namespace clang
