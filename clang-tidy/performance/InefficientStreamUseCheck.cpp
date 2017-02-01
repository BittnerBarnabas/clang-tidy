//===--- InefficientStreamUseCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InefficientStreamUseCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

static inline std::string
getEscapedString(StringRef StrRef) {
  std::string Tmp;
  switch (StrRef.str()[0]) {
  case '\n':Tmp = "\\n";
    break;
  case '\t':Tmp = "\\t";
    break;
  case '\a':Tmp = "\\a";
    break;
  case '\b':Tmp = "\\b";
    break;
  case '\f':Tmp = "\\f";
    break;
  case '\r':Tmp = "\\r";
    break;
  case '\v':Tmp = "\\v";
    break;
  case '\'':Tmp = "\\'";
    break;
  default:Tmp = StrRef.str()[0];
  }
  return Tmp;
}
void InefficientStreamUseCheck::registerMatchers(MatchFinder *Finder) {
  const auto ImplicitCastFromConstCharPtr =
      hasImplicitDestinationType(asString("const char *"));
  const auto ImplicitCastToStringLiteral = hasSourceExpression(
      stringLiteral(hasSize(1)).bind("sourceStringLiteral"));
  const auto CharArrayToCharImplicitCast = implicitCastExpr(
      ImplicitCastFromConstCharPtr, ImplicitCastToStringLiteral,
      hasAncestor(cxxOperatorCallExpr()));

  const auto StdEndlineFunctionReference = ignoringImpCasts(
      declRefExpr(hasDeclaration(functionDecl(hasName("std::endl"))))
          .bind("endline"));
  const auto MultipleEndlineMatcher =
      cxxOperatorCallExpr(hasOverloadedOperatorName("<<"),
                          hasAnyArgument(StdEndlineFunctionReference),
                          hasDescendant(cxxOperatorCallExpr(
                              hasOverloadedOperatorName("<<"),
                              hasAnyArgument(StdEndlineFunctionReference))));

  Finder->addMatcher(MultipleEndlineMatcher, this);
  Finder->addMatcher(CharArrayToCharImplicitCast, this);
}

void InefficientStreamUseCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *ToCharCastedString =
      Result.Nodes.getNodeAs<StringLiteral>("sourceStringLiteral");
  const auto *EndlineRef = Result.Nodes.getNodeAs<DeclRefExpr>("endline");

  if (ToCharCastedString) {
    const std::string ReplacementSuggestion =
        (Twine{"\'"} + getEscapedString(ToCharCastedString->getString()) + "\'")
            .str();
    diag(ToCharCastedString->getExprLoc(),
         "Inefficient cast from const "
             "char[2] to const char *, consider "
             "using a character literal")
        << FixItHint::CreateReplacement(ToCharCastedString->getExprLoc(),
                                        ReplacementSuggestion);
  } else {
    diag(
        EndlineRef->getExprLoc(),
        "Multiple std::endl use is not efficient consider using '\\n' instead.")
        << FixItHint::CreateReplacement(EndlineRef->getSourceRange(), "'\\n'");
  }
}

} // namespace performance
} // namespace tidy
} // namespace clang
