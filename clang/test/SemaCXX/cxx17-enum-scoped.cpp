// RUN: %clang_cc1 -fsyntax-only -pedantic -std=c++17 -verify -triple x86_64-apple-darwin %s

// expected-no-diagnostics

namespace PR54158 {
  enum class A : int;
  enum class B : int;
  B x{A{}};
}
