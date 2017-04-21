// RUN: %check_clang_tidy %s performance-container-default-initializer %t -- -- -std=c++11

#include <vector>

int main() {
  short shortInt;
  unsigned short uShortInt;
  int Int;
  unsigned int uInt;
  long longInt;
  unsigned long uLongInt;
  long long longLongInt;
  unsigned long long uLongLongInt;
  //Tests for std::vector
  {
    std::vector<short> vec1;
    vec1.push_back(shortInt);
    vec1.emplace_back(uShortInt);
    vec1.push_back(Int);
    vec1.emplace_back(uInt);
    vec1.push_back(longInt);
    vec1.push_back(uLongInt);
    vec1.push_back(longLongInt);
    vec1.emplace_back(uLongLongInt);
    // CHECK-MESSAGES: :[[@LINE-9]]:5: warning: Initialize containers in place if you can [performance-container-default-initializer]
    // CHECK-FIXES: {{.*}}std::vector<short> vec1{shortInt, (short)uShortInt, (short)Int, (short)uInt, (short)longInt, (short)uLongInt, (short)longLongInt, (short)uLongLongInt};{{$}}

  }
  /*{
      std::vector<int> vec1;
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
      vec1.push_back(shortInt);
      vec1.emplace_back(uShortInt);
      vec1.push_back(Int);
      vec1.emplace_back(uInt);
      vec1.push_back(longInt);
      vec1.push_back(uLongInt);
      vec1.push_back(longLongInt);
      vec1.emplace_back(uLongLongInt);
    }
*/
}

