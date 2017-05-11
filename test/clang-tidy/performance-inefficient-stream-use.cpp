// RUN: %check_clang_tidy %s performance-inefficient-stream-use %t

namespace std {
template<class T>
class basic_ostream {};

template<class T>
class basic_stringstream : public virtual basic_ostream<T> {};

typedef basic_ostream<char> ostream;
typedef basic_stringstream<char> stringstream;
extern ostream cout;

template<class T>
basic_ostream<T>& endl(basic_ostream<T> &os){ return os; }

ostream& operator<<(ostream& os, const char*){return os;}
ostream& operator<<(ostream& os, char){ return os;}
ostream& operator<<(ostream& os, ostream& (*__pf)(ostream&)){return os;}


}

int main() {
  std::stringstream ss;
  ss << "A" << "B";
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: Inefficient cast from const char[2] to const char *, consider using a character literal [performance-inefficient-stream-use]
  // CHECK-MESSAGES: :[[@LINE-2]]:16: warning: Inefficient cast
  // CHECK-FIXES: {{.*}}ss << 'A' << 'B';{{$}}

  std::cout << 'C';
  std::cout << "A";
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Inefficient cast
  // CHECK-FIXES: {{.*}}std::cout << 'A';{{$}}
  std::cout << 'C' << "B" << "A";
  // CHECK-MESSAGES: :[[@LINE-1]]:23: warning: Inefficient cast
  // CHECK-MESSAGES: :[[@LINE-2]]:30: warning: Inefficient cast
  // CHECK-FIXES: {{.*}}std::cout << 'C' << 'B' << 'A';{{$}}
  std::cout << "A" << "B" << 'C';
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Inefficient cast
  // CHECK-MESSAGES: :[[@LINE-2]]:23: warning: Inefficient cast
  // CHECK-FIXES: {{.*}}std::cout << 'A' << 'B' << 'C';{{$}}
  std::cout << std::endl << std::endl << std::endl;
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Multiple std::endl use is not efficient consider using '\n' instead. [performance-inefficient-stream-use]
  // CHECK-MESSAGES: :[[@LINE-2]]:29: warning: Multiple std::endl
  // CHECK-FIXES: {{.*}}std::cout << '\n' << '\n' << std::endl;{{$}}
  std::cout << std::endl;
  std::cout << std::endl << std::endl;
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Multiple std::endl
  // CHECK-FIXES: {{.*}}std::cout << '\n' << std::endl;{{$}}
  std::cout << "A" << std::endl;
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Inefficient cast
  // CHECK-FIXES: {{.*}}std::cout << 'A' << std::endl;{{$}}
  std::cout << "A" << std::endl << std::endl;
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Inefficient cast
  // CHECK-MESSAGES: :[[@LINE-2]]:23: warning: Multiple std::endl
  // CHECK-FIXES: {{.*}}std::cout << 'A' << '\n' << std::endl;{{$}}
  std::cout << "A" << 'B' << std::endl << std::endl;
  // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: Inefficient cast
  // CHECK-MESSAGES: :[[@LINE-2]]:30: warning: Multiple std::endl
  // CHECK-FIXES: {{.*}}std::cout << 'A' << 'B' << '\n' << std::endl;{{$}}
}
