// RUN: %check_clang_tidy %s performance-container-default-initializer %t -- -- -std=c++11

namespace std {

template <typename T>
class basic_string {
public:
  basic_string() {}
  basic_string(const char*){}
  ~basic_string() {}
  basic_string<T> *operator+=(const basic_string<T> &) {}
  friend basic_string<T> operator+(const basic_string<T> &, const basic_string<T> &) {}
};
typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;

template<class T>
class initializer_list{};

template<class T1, class T2>
struct pair{
  pair(){}
  pair(const T1&, const T2&){}

  template<class _U1, class _U2>
  pair(const pair<_U1, _U2>&){}

  template <class _U1, class _U2>
  pair(_U1&& __u1, _U2&& __u2){}
};

template<class Key, class T>
class map {
  typedef pair<Key,T> value_type;
public:
  map(){}
  map(initializer_list<value_type>){}
  void insert(initializer_list<value_type>){}
  void insert(const value_type&){}
  template<class ...Args>
  void emplace(Args&& ...args){}
};

template<class T>
class vector {
public:
  vector(){}
  template<class ...T2>
  void emplace_back(T2&& ...){}
  template<class T2>
  void push_back(T2&&){}
};

template<class T>
class set {
public:
  set(){}
  template<class ...T2>
  void emplace(T2&& ...){}
  template<class T2>
  void insert(T2&&){}
};

template<class T1, class T2>
static pair<T1,T2> make_pair(const T1&, const T2&){ return pair<T1,T2>(); }

} // end of STD namespace

class MyObj{
public:
  MyObj(int, int){}
  MyObj(const MyObj&){}
};

bool operator<(const MyObj&, const MyObj&) {return true;}

namespace q{
typedef MyObj type;
}

int main() {
  short shortInt;
  unsigned short uShortInt;
  int Int;
  unsigned int uInt;
  long longInt;
  unsigned long uLongInt;
  long long longLongInt;
  unsigned long long uLongLongInt;
  // Tests for std::vector
  {
    std::vector<short> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<short> vec1{shortInt, (short)uShortInt, (short)Int, (short)uInt, (short)longInt, (short)uLongInt, (short)longLongInt, (short)uLongLongInt};{{$}}
    vec1.push_back(shortInt);
    vec1.emplace_back(uShortInt);
    vec1.push_back(Int);
    vec1.emplace_back(uInt);
    vec1.push_back(longInt);
    vec1.push_back(uLongInt);
    vec1.push_back(longLongInt);
    vec1.emplace_back(uLongLongInt);

  }
  {
    std::vector<int> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<int> vec1{(int)shortInt, (int)uShortInt, Int, (int)uInt, (int)longInt, (int)uLongInt, (int)longLongInt, (int)uLongLongInt};{{$}}
    vec1.push_back(shortInt);
    vec1.emplace_back(uShortInt);
    vec1.push_back(Int);
    vec1.emplace_back(uInt);
    vec1.push_back(longInt);
    vec1.push_back(uLongInt);
    vec1.push_back(longLongInt);
    vec1.emplace_back(uLongLongInt);
  }
  {
    std::vector<double> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<double> vec1{(double)shortInt, (double)uShortInt, (double)Int, (double)uInt, (double)longInt, (double)uLongInt, (double)longLongInt, (double)uLongLongInt};{{$}}
    vec1.push_back(shortInt);
    vec1.emplace_back(uShortInt);
    vec1.push_back(Int);
    vec1.emplace_back(uInt);
    vec1.push_back(longInt);
    vec1.push_back(uLongInt);
    vec1.push_back(longLongInt);
    vec1.emplace_back(uLongLongInt);
  }
  {
    std::vector<MyObj> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<MyObj> vec1{MyObj(1,1), MyObj(1, 1), MyObj(1.0, 1.0), MyObj(1.0, 1), MyObj(MyObj(1.0,1))};{{$}}
    vec1.push_back(MyObj(1,1));
    vec1.emplace_back(1,1);
    vec1.emplace_back(1.0,1.0);
    vec1.push_back(MyObj(1.0, 1));
    vec1.emplace_back(MyObj(1.0,1));
  }
  {
    using namespace std;
    set<int> set1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}set<int> set1{(int)1.0, (int)2.0};{{$}}
    set1.insert(1.0);
    set1.emplace(2.0);
  }
  {
    std::string str;
    std::map<std::string, int> map1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::map<std::string, int> map1{{"A", (int)1.0}, {std::make_pair("B", 2.0)}, {str, (int)2.0}};{{$}}
    map1.emplace("A", 1.0);
    map1.emplace(std::make_pair("B", 2.0));
    map1.insert({str, 2.0});
  }
  {
    std::string str1;
    std::string str2;
    std::set<std::string> set1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::set<std::string> set1{str1, std::string(str2)};{{$}}
    set1.insert(str1);
    set1.emplace(str2);
    str2 = "abc";
    set1.insert(str2);
  }
}