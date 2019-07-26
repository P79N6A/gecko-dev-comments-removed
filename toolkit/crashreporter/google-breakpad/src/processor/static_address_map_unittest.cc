
































#include <climits>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>
#include <sstream>

#include "breakpad_googletest_includes.h"
#include "common/using_std_string.h"
#include "processor/address_map-inl.h"
#include "processor/static_address_map-inl.h"
#include "processor/simple_serializer-inl.h"
#include "map_serializers-inl.h"

typedef google_breakpad::StaticAddressMap<int, char> TestMap;
typedef google_breakpad::AddressMap<int, string> AddrMap;

class TestStaticAddressMap : public ::testing::Test {
 protected:
  void SetUp() {
    for (int testcase = 0; testcase < kNumberTestCases; ++testcase) {
      testdata[testcase] = new int[testsize[testcase]];
    }

    

    
    testdata[1][0] = 10;

    
    const int tempdata[] = {5, 10, 14, 15, 16, 20};
    for (int i = 0; i < testsize[2]; ++i)
      testdata[2][i] = tempdata[i];

    
    srand(time(NULL));
    for (int i = 0; i < testsize[3]; ++i)
      testdata[3][i] = rand();

    
    std::stringstream sstream;
    for (int testcase = 0; testcase < kNumberTestCases; ++testcase) {
      for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
        sstream.clear();
        sstream << "test " << testdata[testcase][data_item];
        addr_map[testcase].Store(testdata[testcase][data_item], sstream.str());
      }
      map_data[testcase] = serializer.Serialize(addr_map[testcase], NULL);
      test_map[testcase] = TestMap(map_data[testcase]);
    }
  }

  void TearDown() {
    for (int i = 0; i < kNumberTestCases; ++i) {
      delete [] map_data[i];
      delete [] testdata[i];
    }
  }

  void CompareRetrieveResult(int testcase, int target) {
    int address;
    int address_test;
    string entry;
    string entry_test;
    const char *entry_cstring = NULL;
    bool found;
    bool found_test;

    found = addr_map[testcase].Retrieve(target, &entry, &address);
    found_test =
        test_map[testcase].Retrieve(target, entry_cstring, &address_test);

    ASSERT_EQ(found, found_test);

    if (found && found_test) {
      ASSERT_EQ(address, address_test);
      entry_test = entry_cstring;
      ASSERT_EQ(entry, entry_test);
    }
  }

  void RetrieveTester(int testcase) {
    int target;
    target = INT_MIN;
    CompareRetrieveResult(testcase, target);
    target = INT_MAX;
    CompareRetrieveResult(testcase, target);

    srand(time(0));
    for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
      
      

      
      
      target = testdata[testcase][data_item];
      CompareRetrieveResult(testcase, target);
      
      
      target -= 1;
      CompareRetrieveResult(testcase, target);
      target += 3;
      CompareRetrieveResult(testcase, target);
      
      target = rand();
      CompareRetrieveResult(testcase, target);
    }
  }

  
  static const int kNumberTestCases = 4;
  static const int testsize[];
  int *testdata[kNumberTestCases];

  AddrMap addr_map[kNumberTestCases];
  TestMap test_map[kNumberTestCases];
  char *map_data[kNumberTestCases];
  google_breakpad::AddressMapSerializer<int, string> serializer;
};

const int TestStaticAddressMap::testsize[] = {0, 1, 6, 1000};

TEST_F(TestStaticAddressMap, TestEmptyMap) {
  int testcase = 0;
  int target;
  target = INT_MIN;
  CompareRetrieveResult(testcase, target);
  target = INT_MAX;
  CompareRetrieveResult(testcase, target);
  for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
    target = testdata[testcase][data_item];
    CompareRetrieveResult(testcase, target);
    target -= 1;
    CompareRetrieveResult(testcase, target);
    target += 3;
    CompareRetrieveResult(testcase, target);
    target = rand();
    CompareRetrieveResult(testcase, target);
  }
}

TEST_F(TestStaticAddressMap, TestOneElementMap) {
  int testcase = 1;
  int target;
  target = INT_MIN;
  CompareRetrieveResult(testcase, target);
  target = INT_MAX;
  CompareRetrieveResult(testcase, target);
  for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
    target = testdata[testcase][data_item];
    CompareRetrieveResult(testcase, target);
    target -= 1;
    CompareRetrieveResult(testcase, target);
    target += 3;
    CompareRetrieveResult(testcase, target);
    target = rand();
    CompareRetrieveResult(testcase, target);
  }
}

TEST_F(TestStaticAddressMap, TestSixElementsMap) {
  int testcase = 2;
  int target;
  target = INT_MIN;
  CompareRetrieveResult(testcase, target);
  target = INT_MAX;
  CompareRetrieveResult(testcase, target);
  for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
    target = testdata[testcase][data_item];
    CompareRetrieveResult(testcase, target);
    target -= 1;
    CompareRetrieveResult(testcase, target);
    target += 3;
    CompareRetrieveResult(testcase, target);
    target = rand();
    CompareRetrieveResult(testcase, target);
  }
}

TEST_F(TestStaticAddressMap, Test1000RandomElementsMap) {
  int testcase = 3;
  int target;
  target = INT_MIN;
  CompareRetrieveResult(testcase, target);
  target = INT_MAX;
  CompareRetrieveResult(testcase, target);
  for (int data_item = 0; data_item < testsize[testcase]; ++data_item) {
    target = testdata[testcase][data_item];
    CompareRetrieveResult(testcase, target);
    target -= 1;
    CompareRetrieveResult(testcase, target);
    target += 3;
    CompareRetrieveResult(testcase, target);
    target = rand();
    CompareRetrieveResult(testcase, target);
  }
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
