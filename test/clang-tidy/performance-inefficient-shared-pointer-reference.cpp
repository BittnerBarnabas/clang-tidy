// RUN: %check_clang_tidy %s performance-inefficient-shared-pointer-reference %t -- -- -std=c++11

namespace std {

template <class T, T val> struct integral_constant {
  static const T value = val;
  typedef T value_type;
  typedef integral_constant type;
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <class T> struct add_rvalue_reference { typedef T &&type; };

template <class T> typename add_rvalue_reference<T>::type declval();

template <class _Tp> void __test_convert(_Tp);

template <class _From, class _To, class = void>
struct is_convertible : public false_type {};

template <class _From, class _To>
struct is_convertible<_From, _To,
                      decltype(__test_convert<_To>(declval<_From>()))>
    : public true_type {};

template<bool, class T = void> struct enable_if{};
template<class T> struct enable_if<true, T> { typedef T value; };

template<class T>
class shared_ptr {
  struct __nat { int bool_val; };
public:
  shared_ptr(){}
  template<class _Yp>
  shared_ptr(const shared_ptr<_Yp>& __r, typename enable_if<is_convertible<_Yp*, T*>::value,__nat>::value = __nat());
  T* get(){ return new T(); };
};

template<class T>
shared_ptr<T> make_shared() {return shared_ptr<T>();}
}

struct Base {};
struct Derived : Base {};

typedef std::shared_ptr<Derived> shrPtrTypedef;
typedef std::shared_ptr<Base> shrBasePtrTypedef;
typedef Derived derivedType;
typedef Base baseType;

template<class T>
using usingCica = std::shared_ptr<T>;

template<class T>
struct templatedShrtPtr {
  typedef std::shared_ptr<T> value;
};

void f1(std::shared_ptr<Base> ptr) {}
void f2(shrBasePtrTypedef ptr1){}
void f3(std::shared_ptr<Base>, std::shared_ptr<Derived>){}
void f4(std::shared_ptr<Derived>, std::shared_ptr<Base>){}
void f5(const Base&){}

int main() {
  auto ptr1 = std::make_shared<Derived>();
  auto ptr2 = shrPtrTypedef();
  auto ptr3 = usingCica<Derived>();
  auto ptr4 = std::make_shared<Base>();
  auto ptr5 = std::make_shared<derivedType>();

  f5(*ptr1.get());
  f1(std::make_shared<Derived>());
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: inefficient polymorphic cast of std::shared_ptr<Derived> to std::shared_ptr<Base> [performance-inefficient-shared-pointer-reference]
  f1(std::shared_ptr<Derived>());
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: inefficient polymorphic cast of std::shared_ptr
  f3(ptr1, ptr1);
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: inefficient polymorphic cast of std::shared_ptr
  f3(ptr4, ptr1);
  f4(ptr1, ptr1);
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: inefficient polymorphic cast of std::shared_ptr
  f4(ptr1, ptr4);
  f1(shrPtrTypedef());
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: inefficient polymorphic cast of std::shared_ptr
  f1(ptr5);
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: inefficient polymorphic cast of std::shared_ptr
}
