

































#include <climits>
#include <cstdio>

#include "processor/range_map-inl.h"

#include "processor/linked_ptr.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"


namespace {


using google_breakpad::linked_ptr;
using google_breakpad::scoped_ptr;
using google_breakpad::RangeMap;




class CountedObject {
 public:
  explicit CountedObject(int id) : id_(id) { ++count_; }
  ~CountedObject() { --count_; }

  static int count() { return count_; }
  int id() const { return id_; }

 private:
  static int count_;
  int id_;
};

int CountedObject::count_;


typedef int AddressType;
typedef RangeMap< AddressType, linked_ptr<CountedObject> > TestMap;




struct RangeTest {
  
  AddressType address;

  
  AddressType size;

  
  int id;

  
  bool expect_storable;
};




struct RangeTestSet {
  
  const RangeTest *range_tests;

  
  unsigned int range_test_count;
};





static bool StoreTest(TestMap *range_map, const RangeTest *range_test) {
  linked_ptr<CountedObject> object(new CountedObject(range_test->id));
  bool stored = range_map->StoreRange(range_test->address,
                                      range_test->size,
                                      object);

  if (stored != range_test->expect_storable) {
    fprintf(stderr, "FAILED: "
            "StoreRange id %d, expected %s, observed %s\n",
            range_test->id,
            range_test->expect_storable ? "storable" : "not storable",
            stored ? "stored" : "not stored");
    return false;
  }

  return true;
}







static bool RetrieveTest(TestMap *range_map, const RangeTest *range_test) {
  for (unsigned int side = 0; side <= 1; ++side) {
    
    

    
    

    
    
    
    
    AddressType low_offset = -1;
    AddressType high_offset = 1;
    if (range_test->size == 1) {
      if (!side)          
        high_offset = 0;  
      else                
        low_offset = 0;   
    }

    for (AddressType offset = low_offset; offset <= high_offset; ++offset) {
      AddressType address =
          offset +
          (!side ? range_test->address :
                   range_test->address + range_test->size - 1);

      bool expected_result = false;  
      if (range_test->expect_storable) {
        if (offset == 0)             
          expected_result = true;    
        else if (offset == -1)       
          expected_result = side;    
        else                         
          expected_result = !side;   
      }

      linked_ptr<CountedObject> object;
      AddressType retrieved_base;
      AddressType retrieved_size;
      bool retrieved = range_map->RetrieveRange(address, &object,
                                                &retrieved_base,
                                                &retrieved_size);

      bool observed_result = retrieved && object->id() == range_test->id;

      if (observed_result != expected_result) {
        fprintf(stderr, "FAILED: "
                        "RetrieveRange id %d, side %d, offset %d, "
                        "expected %s, observed %s\n",
                        range_test->id,
                        side,
                        offset,
                        expected_result ? "true" : "false",
                        observed_result ? "true" : "false");
        return false;
      }

      
      
      if (observed_result == true &&
          (retrieved_base != range_test->address ||
           retrieved_size != range_test->size)) {
        fprintf(stderr, "FAILED: "
                        "RetrieveRange id %d, side %d, offset %d, "
                        "expected base/size %d/%d, observed %d/%d\n",
                        range_test->id,
                        side,
                        offset,
                        range_test->address, range_test->size,
                        retrieved_base, retrieved_size);
        return false;
      }

      
      
      
      bool expected_nearest = range_test->expect_storable;
      if (!side && offset < 0)
        expected_nearest = false;

      linked_ptr<CountedObject> nearest_object;
      AddressType nearest_base;
      bool retrieved_nearest = range_map->RetrieveNearestRange(address,
                                                               &nearest_object,
                                                               &nearest_base,
                                                               NULL);

      
      
      
      
      if (side && offset > 0 && nearest_base == address) {
        expected_nearest = false;
      }

      bool observed_nearest = retrieved_nearest &&
                              nearest_object->id() == range_test->id;

      if (observed_nearest != expected_nearest) {
        fprintf(stderr, "FAILED: "
                        "RetrieveNearestRange id %d, side %d, offset %d, "
                        "expected %s, observed %s\n",
                        range_test->id,
                        side,
                        offset,
                        expected_nearest ? "true" : "false",
                        observed_nearest ? "true" : "false");
        return false;
      }
    }
  }

  return true;
}








static bool RetrieveIndexTest(TestMap *range_map, int set) {
  linked_ptr<CountedObject> object;
  CountedObject *last_object = NULL;
  AddressType last_base = 0;

  int object_count = range_map->GetCount();
  for (int object_index = 0; object_index < object_count; ++object_index) {
    AddressType base;
    if (!range_map->RetrieveRangeAtIndex(object_index, &object, &base, NULL)) {
      fprintf(stderr, "FAILED: RetrieveRangeAtIndex set %d index %d, "
              "expected success, observed failure\n",
              set, object_index);
      return false;
    }

    if (!object.get()) {
      fprintf(stderr, "FAILED: RetrieveRangeAtIndex set %d index %d, "
              "expected object, observed NULL\n",
              set, object_index);
      return false;
    }

    
    
    if (last_object) {
      
      if (object->id() == last_object->id()) {
        fprintf(stderr, "FAILED: RetrieveRangeAtIndex set %d index %d, "
                "expected different objects, observed same objects (%d)\n",
                set, object_index, object->id());
        return false;
      }

      
      if (base <= last_base) {
        fprintf(stderr, "FAILED: RetrieveRangeAtIndex set %d index %d, "
                "expected different bases, observed same bases (%d)\n",
                set, object_index, base);
        return false;
      }
    }

    last_object = object.get();
    last_base = base;
  }

  
  
  if (range_map->RetrieveRangeAtIndex(object_count, &object, NULL, NULL)) {
    fprintf(stderr, "FAILED: RetrieveRangeAtIndex set %d index %d (too large), "
            "expected failure, observed success\n",
            set, object_count);
    return false;
  }

  return true;
}



static bool RunTests() {
  
  
  const RangeTest range_tests_0[] = {
    { INT_MIN,     16,      1,  true },   
    { -2,          5,       2,  true },   
    { INT_MAX - 9, 11,      3,  false },  
    { INT_MAX - 9, 10,      4,  true },   
    { 5,           0,       5,  false },  
    { 5,           1,       6,  true },   
    { -20,         15,      7,  true },   

    { 10,          10,      10, true },   
    { 9,           10,      11, false },  
    { 9,           11,      12, false },  
    { 9,           12,      13, false },  
    { 10,          9,       14, false },  
    { 10,          10,      15, false },  
    { 10,          11,      16, false },  
    { 11,          8,       17, false },  
    { 11,          9,       18, false },  
    { 11,          10,      19, false },  
    { 9,           2,       20, false },  
    { 10,          1,       21, false },  
    { 19,          1,       22, false },  
    { 19,          2,       23, false },  

    { 9,           1,       24, true },   
    { 20,          1,       25, true },   

    { 6,           3,       26, true },   
    { 7,           3,       27, false },  
    { 7,           5,       28, false },  
    { 4,           20,      29, false },  

    { 30,          50,      30, true },
    { 90,          25,      31, true },
    { 35,          65,      32, false },  
    { 120,         10000,   33, true },   
    { 20000,       20000,   34, true },   
    { 0x10001,     0x10001, 35, true },   

    { 27,          -1,      36, false }   
  };

  
  
  
  
  
  
  const RangeTest range_tests_1[] = {
    { INT_MIN, INT_MAX, 50, true },   
    { -1,      2,       51, true },   
    { 1,       INT_MAX, 52, true },   
    { INT_MIN, INT_MAX, 53, false },  
    { -1,      2,       54, false },
    { 1,       INT_MAX, 55, false },
    { -3,      6,       56, false },  
  };

  
  
  
  
  const RangeTest range_tests_2[] = {
    { INT_MIN, 0, 100, false },  
    { -1,      3, 101, true },
    { INT_MAX, 0, 102, false },  
  };

  
  
  const RangeTest range_tests_3[] = {
    { INT_MIN + 1, 1, 110, true },
    { INT_MAX - 1, 1, 111, true },
    { INT_MIN,     0, 112, false },  
    { INT_MAX,     0, 113, false }   
  };

  
  const RangeTestSet range_test_sets[] = {
    { range_tests_0, sizeof(range_tests_0) / sizeof(RangeTest) },
    { range_tests_1, sizeof(range_tests_1) / sizeof(RangeTest) },
    { range_tests_2, sizeof(range_tests_2) / sizeof(RangeTest) },
    { range_tests_3, sizeof(range_tests_3) / sizeof(RangeTest) },
    { range_tests_0, sizeof(range_tests_0) / sizeof(RangeTest) }   
  };

  
  
  scoped_ptr<TestMap> range_map(new TestMap());

  
  unsigned int range_test_set_count = sizeof(range_test_sets) /
                                      sizeof(RangeTestSet);
  for (unsigned int range_test_set_index = 0;
       range_test_set_index < range_test_set_count;
       ++range_test_set_index) {
    const RangeTest *range_tests =
        range_test_sets[range_test_set_index].range_tests;
    unsigned int range_test_count =
        range_test_sets[range_test_set_index].range_test_count;

    
    
    int stored_count = 0;  
    for (unsigned int range_test_index = 0;
         range_test_index < range_test_count;
         ++range_test_index) {
      const RangeTest *range_test = &range_tests[range_test_index];
      if (!StoreTest(range_map.get(), range_test))
        return false;

      if (range_test->expect_storable)
        ++stored_count;
    }

    
    
    if (CountedObject::count() != stored_count) {
      fprintf(stderr, "FAILED: "
              "stored object counts don't match, expected %d, observed %d\n",
              stored_count,
              CountedObject::count());

      return false;
    }

    
    if (range_map->GetCount() != stored_count) {
      fprintf(stderr, "FAILED: stored object count doesn't match GetCount, "
              "expected %d, observed %d\n",
              stored_count, range_map->GetCount());

      return false;
    }

    
    for (unsigned int range_test_index = 0;
         range_test_index < range_test_count;
         ++range_test_index) {
      const RangeTest *range_test = &range_tests[range_test_index];
      if (!RetrieveTest(range_map.get(), range_test))
        return false;
    }

    if (!RetrieveIndexTest(range_map.get(), range_test_set_index))
      return false;

    
    
    if (range_test_set_index < range_test_set_count - 1)
      range_map->Clear();
    else
      range_map.reset();

    
    
    if (CountedObject::count() != 0) {
      fprintf(stderr, "FAILED: "
              "did not free all objects after %s, %d still allocated\n",
              range_test_set_index < range_test_set_count - 1 ? "clear"
                                                              : "delete",
              CountedObject::count());

      return false;
    }
  }

  return true;
}


}  


int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  return RunTests() ? 0 : 1;
}
