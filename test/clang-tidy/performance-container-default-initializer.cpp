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
  vector(initializer_list<T> ){}
  vector(int, int){}
  class iterator {};
  void insert(iterator, iterator, iterator){}
  iterator begin() { return iterator();}
  iterator end() { return iterator(); }
  template<class ...T2>
  void emplace_back(T2&& ...){}
  template<class T2>
  void push_back(T2&&){}
  int size(){return 0;}
  void reserve(int){}
};

template<class T>
class set {
public:
  set(){}
  set(initializer_list<T>){}
  template<class ...T2>
  void emplace(T2&& ...){}
  template<class T2>
  void insert(T2&&){}
};

template<class T1, class T2>
static pair<T1,T2> make_pair(const T1&, const T2&){ return pair<T1,T2>(); }

} // end of STD namespace*/

template<int N>
class TemplateType {
  int a = N;
public:
  int size(){return N;}
};

template<int N>
void f4(TemplateType<N>& t) {
  std::vector<int> vec1;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Initialize containers in place
  // CHECK-FIXES: {{.*}}std::vector<int> vec1{4};{{$}}
  vec1.push_back(4);
  vec1.push_back(t.size());
}
template<int N>
void f5(TemplateType<N>& t) {
  std::vector<int> vec1;
  vec1.push_back(t.size());
  vec1.push_back(4);
  vec1.push_back(t.size());
}

class MyObj{
public:
  MyObj(int, int){}
  MyObj(const MyObj&){}
};

bool operator<(const MyObj&, const MyObj&) {return true;}

namespace q{
typedef MyObj type;
}

int f1(int) { return 0;}
double f2(int) { return 0.0; }
template<class T>
int f3(const std::vector<T>&) { return 0; }

#define semi ;

int main() {
  short shortInt = 1;
  unsigned short uShortInt = 1;
  int Int = 1;
  unsigned int uInt = 1;
  long longInt = 1;
  unsigned long uLongInt = 1;
  long long longLongInt = 1;
  unsigned long long uLongLongInt = 1;
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
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
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
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
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
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
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
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}set<int> set1{(int)1.0, (int)2.0};{{$}}
    set1.insert(1.0);
    set1.emplace(2.0);
  }
  {
    std::string str;
    std::map<std::string, int> map1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::map<std::string, int> map1{{[{][{]}}"A", (int)1.0}, {std::make_pair("B", 2.0)}, {str, 2.0{{[}][}]}};{{$}}
    map1.emplace("A", 1.0);
    map1.emplace(std::make_pair("B", 2.0));
    map1.insert({str, 2.0});
  }
  {
    std::string str1;
    std::string str2;
    std::set<std::string> set1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::set<std::string> set1{str1, std::string(str2)};{{$}}
    set1.insert(str1);
    set1.emplace(str2);
    str2 = "abc";
    set1.insert(str2);
  }
  {
    std::vector<int> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec1{4};{{$}}
    vec1.push_back(4);
    vec1.push_back(vec1.size());
    vec1.emplace_back(5);
    int a = 5;
    std::vector<int> vec2;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec2{4, (int)f2(a), f1(a)};{{$}}
    vec2.emplace_back(4);
    vec2.emplace_back(f2(a));
    vec2.push_back(f1(a));
    a++;
    vec2.push_back(f1(a));
  }
  {
    std::vector<int> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec1{1};{{$}}
    vec1.push_back(1);
    vec1.push_back(f3(vec1));
    vec1.emplace_back(3);
  }
  {
    std::vector<int> vec1    {};
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec1    {1};{{$}}
    vec1.push_back(1) semi
    std::vector<int> vec2

    ;
    // CHECK-MESSAGES: :[[@LINE-3]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec2{2}{{$}}
    vec2.push_back(2) semi
    std::vector<int> vec3{3};
    vec3.push_back(3);
    std::vector<int> vec4 = { 4 };
    vec4.push_back(4);
    std::vector<int> vec5(4,4);
    vec5.push_back(5);
  }
  {
    int a = 4;
    int b = 3;
    std::vector<int> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec1{(a < b ? b : a), (int)(a < b ? f1(a) : f2(b)), (int)(f1(a), f2(a))};{{$}}
    vec1.push_back(a < b ? b : a);
    vec1.emplace_back(a < b ? f1(a) : f2(b));
    vec1.push_back((f1(a), f2(a)));
    a < b ? vec1.push_back(4) : vec1.emplace_back(4);

    std::map<int, int> map1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::map<int, int> map1{{[{][{]}}(int)(a < b ? f1(a) : f2(b)), (int)(a < b ? f1(a) : f2(b))}, {3, 1}, {a < b ? f1(a) : f2(b), 3}, {3.2, 1.2{{[}][}]}};{{$}}
    map1.emplace(a < b ? f1(a) : f2(b), a < b ? f1(a) : f2(b));
    map1.emplace(3,1);
    map1.insert({a < b ? f1(a) : f2(b), 3});
    map1.insert({3.2, 1.2});

    std::map<int, int> map2;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::map<int, int> map2{{[{][{]}}(int)(a < b ? f1(a) : f2(b)), (int)(a < b ? f1(a) : f2(b))}, {3, 1}, {a < b ? f1(a) : f2(b), 3}, {3.2, 1.2{{[}][}]}};{{$}}
    map2.emplace(a < b ? f1(a) : f2(b), a < b ? f1(a) : f2(b));
    map2.emplace(3,1);
    map2.insert({a < b ? f1(a) : f2(b), 3});
    map2.insert({3.2, 1.2});
  }
  {
    std::vector<int> vec1{2,3,4};
    std::vector<int> vec2;
    vec2.insert(vec2.end(), vec1.begin(), vec1.end());
    vec2.reserve(1 + vec1.size());
    vec2.push_back(4);
    std::map<int,int>map1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::map<int,int>map1{{[{][{]}}1,2{{[}][}]}};{{$}}
    map1.insert({1,2});
  }
  {
    std::vector<int> vec1{2,3,4};
    std::vector<int> vec2;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place
    // CHECK-FIXES: {{.*}}std::vector<int> vec2{4, f1(vec1.size())};{{$}}
    vec2.push_back(4);
    vec2.push_back(f1(vec1.size()));
    TemplateType<5> t1{};
    TemplateType<4> t2{};
    f4(t1);
    f4(t2);
    f5(t2);
  }
}
