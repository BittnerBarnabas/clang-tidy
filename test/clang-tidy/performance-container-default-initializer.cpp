// RUN: %check_clang_tidy %s performance-container-default-initializer %t -- -- -std=c++11

#include <vector>
#include <map>
#include <set>

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
    
    std::vector<MyObj> vec1;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<MyObj> vec1{MyObj(1,1), MyObj(1, 1), MyObj(1.0, 1.0), MyObj(1.0, 1), MyObj(MyObj(1.0,1))};{{$}}
    vec1.push_back(MyObj(1,1));
    vec1.emplace_back(1,1);
    vec1.emplace_back(1.0,1.0);
    vec1.push_back(MyObj(1.0, 1));
    vec1.emplace_back(MyObj(1.0,1));
  }
}
