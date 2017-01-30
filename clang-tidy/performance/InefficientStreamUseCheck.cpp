//===--- InefficientStreamUseCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InefficientStreamUseCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace performance {

void InefficientStreamUseCheck::registerMatchers(MatchFinder *Finder) {
  const auto ImplicitCastDestination =
      hasImplicitDestinationType(asString("const char *"));
  const auto ImplicitCastSource = hasSourceExpression(
      stringLiteral(hasSize(1)).bind("sourceStringLiteral"));
  const auto CharArrayToCharImplicitCast =
      implicitCastExpr(ImplicitCastSource, ImplicitCastDestination,
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

  if (ToCharCastedString != nullptr) {
    std::string buf("\'");
    buf.append(getEscapedString(ToCharCastedString->getString().data()))
        .append("\'");
    diag(ToCharCastedString->getExprLoc(),
         "Inefficient cast from const "
         "char[2] to const char *, consider "
         "using ' '")
        << FixItHint::CreateReplacement(ToCharCastedString->getExprLoc(), buf);
  } else {
    diag(
        EndlineRef->getExprLoc(),
        "Multiple std::endl use is not efficient consider using '\\n' instead.")
        << FixItHint::CreateReplacement(EndlineRef->getSourceRange(), "'\\n'");
  }
}

std::string
InefficientStreamUseCheck::getEscapedString(const StringRef &strRef) {
  std::string tmp;
  switch (strRef.str()[0]) {
  case '\n':
    tmp = "\\n";
    break;
  case '\t':
    tmp = "\\t";
    break;
  case '\a':
    tmp = "\\a";
    break;
  case '\b':
    tmp = "\\b";
    break;
  case '\f':
    tmp = "\\f";
    break;
  case '\r':
    tmp = "\\r";
    break;
  case '\v':
    tmp = "\\v";
    break;
  case '\'':
    tmp = "\\'";
    break;
  default:
    tmp = strRef.str()[0];
  }
  return tmp;
}

} // namespace performance
} // namespace tidy
} // namespace clang
