

































#include "breakpad_googletest_includes.h"
#include "common/scoped_ptr.h"
#include "processor/contained_range_map-inl.h"
#include "processor/static_contained_range_map-inl.h"
#include "processor/simple_serializer-inl.h"
#include "processor/map_serializers-inl.h"
#include "processor/logging.h"

namespace {

typedef google_breakpad::ContainedRangeMap<unsigned int, int> CRMMap;
typedef google_breakpad::StaticContainedRangeMap<unsigned int, int> TestMap;



const int test_data[] = {
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  9,   
  7,   
  1,   
  5,   
  6,   
  6,   
  6,   
  6,   
  6,   
  6,   
  6,   
  5,   
  7,   
  8,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  10,  
  10,  
  10,  
  11,  
  11,  
  11,  
  0,   
  0,   
  0,   
  0,   
  14,  
  14,  
  14,  
  14,  
  15,  
  15,  
  15,  
  15,  
  0,   
  0,   
  19,  
  18,  
  18,  
  18,  
  18,  
  18,  
  18,  
  18,  
  18,  
  20,  
  21,  
  25,  
  26,  
  26,  
  26,  
  26,  
  26,  
  26,  
  24,  
  22,  
  30,  
  30,  
  30,  
  30,  
  31,  
  31,  
  30,  
  32,  
  32,  
  30,  
  34,  
  35,  
  36,  
  39,  
  38,  
  37,  
  43,  
  44,  
  41,  
  45,  
  42,  
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0,   
  0    
};

}  

namespace google_breakpad {

class TestStaticCRMMap : public ::testing::Test {
 protected:
  void SetUp();

  
  google_breakpad::ContainedRangeMap<unsigned int, int> crm_map_;

  
  
  
  google_breakpad::StaticContainedRangeMap<unsigned int, int> test_map_;

  google_breakpad::ContainedRangeMapSerializer<unsigned int, int> serializer_;

  scoped_array<char> serialized_data_;
};

void TestStaticCRMMap::SetUp() {
  
  
  
  ASSERT_TRUE (crm_map_.StoreRange(10, 10,  1));
  ASSERT_FALSE(crm_map_.StoreRange(10, 10,  2));  
  ASSERT_FALSE(crm_map_.StoreRange(11, 10,  3));  
  ASSERT_FALSE(crm_map_.StoreRange( 9, 10,  4));  
  ASSERT_TRUE (crm_map_.StoreRange(11,  9,  5));  
  ASSERT_TRUE (crm_map_.StoreRange(12,  7,  6));
  ASSERT_TRUE (crm_map_.StoreRange( 9, 12,  7));  
  ASSERT_TRUE (crm_map_.StoreRange( 9, 13,  8));
  ASSERT_TRUE (crm_map_.StoreRange( 8, 14,  9));
  ASSERT_TRUE (crm_map_.StoreRange(30,  3, 10));
  ASSERT_TRUE (crm_map_.StoreRange(33,  3, 11));
  ASSERT_TRUE (crm_map_.StoreRange(30,  6, 12));  
  ASSERT_TRUE (crm_map_.StoreRange(40,  8, 13));  
  ASSERT_TRUE (crm_map_.StoreRange(40,  4, 14));
  ASSERT_TRUE (crm_map_.StoreRange(44,  4, 15));
  ASSERT_FALSE(crm_map_.StoreRange(32, 10, 16));  
  ASSERT_FALSE(crm_map_.StoreRange(50,  0, 17));  
  ASSERT_TRUE (crm_map_.StoreRange(50, 10, 18));
  ASSERT_TRUE (crm_map_.StoreRange(50,  1, 19));
  ASSERT_TRUE (crm_map_.StoreRange(59,  1, 20));
  ASSERT_TRUE (crm_map_.StoreRange(60,  1, 21));
  ASSERT_TRUE (crm_map_.StoreRange(69,  1, 22));
  ASSERT_TRUE (crm_map_.StoreRange(60, 10, 23));
  ASSERT_TRUE (crm_map_.StoreRange(68,  1, 24));
  ASSERT_TRUE (crm_map_.StoreRange(61,  1, 25));
  ASSERT_TRUE (crm_map_.StoreRange(61,  8, 26));
  ASSERT_FALSE(crm_map_.StoreRange(59,  9, 27));
  ASSERT_FALSE(crm_map_.StoreRange(59, 10, 28));
  ASSERT_FALSE(crm_map_.StoreRange(59, 11, 29));
  ASSERT_TRUE (crm_map_.StoreRange(70, 10, 30));
  ASSERT_TRUE (crm_map_.StoreRange(74,  2, 31));
  ASSERT_TRUE (crm_map_.StoreRange(77,  2, 32));
  ASSERT_FALSE(crm_map_.StoreRange(72,  6, 33));
  ASSERT_TRUE (crm_map_.StoreRange(80,  3, 34));
  ASSERT_TRUE (crm_map_.StoreRange(81,  1, 35));
  ASSERT_TRUE (crm_map_.StoreRange(82,  1, 36));
  ASSERT_TRUE (crm_map_.StoreRange(83,  3, 37));
  ASSERT_TRUE (crm_map_.StoreRange(84,  1, 38));
  ASSERT_TRUE (crm_map_.StoreRange(83,  1, 39));
  ASSERT_TRUE (crm_map_.StoreRange(86,  5, 40));
  ASSERT_TRUE (crm_map_.StoreRange(88,  1, 41));
  ASSERT_TRUE (crm_map_.StoreRange(90,  1, 42));
  ASSERT_TRUE (crm_map_.StoreRange(86,  1, 43));
  ASSERT_TRUE (crm_map_.StoreRange(87,  1, 44));
  ASSERT_TRUE (crm_map_.StoreRange(89,  1, 45));
  ASSERT_TRUE (crm_map_.StoreRange(87,  4, 46));
  ASSERT_TRUE (crm_map_.StoreRange(87,  3, 47));
  ASSERT_FALSE(crm_map_.StoreRange(86,  2, 48));

  
  unsigned int size;
  serialized_data_.reset(serializer_.Serialize(&crm_map_, &size));
  BPLOG(INFO) << "Serialized data size: " << size << " Bytes.";

  
  test_map_ = TestMap(serialized_data_.get());
}

TEST_F(TestStaticCRMMap, TestEmptyMap) {
  CRMMap empty_crm_map;

  unsigned int size;
  scoped_array<char> serialized_data;
  serialized_data.reset(serializer_.Serialize(&empty_crm_map, &size));
  scoped_ptr<TestMap> test_map(new TestMap(serialized_data.get()));

  const unsigned int kCorrectSizeForEmptyMap = 16;
  ASSERT_EQ(kCorrectSizeForEmptyMap, size);

  const int *entry_test;
  ASSERT_FALSE(test_map->RetrieveRange(-1, entry_test));
  ASSERT_FALSE(test_map->RetrieveRange(0, entry_test));
  ASSERT_FALSE(test_map->RetrieveRange(10, entry_test));
}

TEST_F(TestStaticCRMMap, TestSingleElementMap) {
  CRMMap crm_map;
  
  int entry = 1;
  crm_map.StoreRange(10, 10,  entry);

  unsigned int size;
  scoped_array<char> serialized_data;
  serialized_data.reset(serializer_.Serialize(&crm_map, &size));
  scoped_ptr<TestMap> test_map(new TestMap(serialized_data.get()));

  const unsigned int kCorrectSizeForSingleElementMap = 40;
  ASSERT_EQ(kCorrectSizeForSingleElementMap, size);

  const int *entry_test;
  ASSERT_FALSE(test_map->RetrieveRange(-1, entry_test));
  ASSERT_FALSE(test_map->RetrieveRange(0, entry_test));
  ASSERT_TRUE(test_map->RetrieveRange(10, entry_test));
  ASSERT_EQ(*entry_test, entry);
  ASSERT_TRUE(test_map->RetrieveRange(13, entry_test));
  ASSERT_EQ(*entry_test, entry);
}

TEST_F(TestStaticCRMMap, RunTestData) {
  unsigned int test_high = sizeof(test_data) / sizeof(test_data[0]);

  
  
  
  
  
  
#ifdef GENERATE_TEST_DATA
  printf("  const int test_data[] = {\n");
#endif  

  for (unsigned int address = 0; address < test_high; ++address) {
    const int *entryptr;
    int value = 0;
    if (test_map_.RetrieveRange(address, entryptr))
      value = *entryptr;

#ifndef GENERATE_TEST_DATA
    
    
    
    EXPECT_EQ(value, test_data[address]) << "FAIL: retrieve address "
                                         << address;
#else  
    printf("    %d%c%s  // %d\n", value,
                                  address == test_high - 1 ? ' ' : ',',
                                  value < 10 ? " " : "",
                                  address);
#endif  
  }

#ifdef GENERATE_TEST_DATA
  printf("  };\n");
#endif  
}

}  

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
