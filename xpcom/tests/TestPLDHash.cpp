





#include <stdio.h>
#include "pldhash.h"






namespace TestPLDHash {

static bool test_pldhash_Init_capacity_ok()
{
  
  
  
  PLDHashTable t;
  bool ok = PL_DHashTableInit(&t, PL_DHashGetStubOps(), nullptr,
                              sizeof(PLDHashEntryStub),
                              mozilla::fallible_t(),
                              PL_DHASH_MAX_INITIAL_LENGTH);
  if (ok)
    PL_DHashTableFinish(&t);

  return ok;
}

static bool test_pldhash_Init_capacity_too_large()
{
  
  PLDHashTable t;
  bool ok = PL_DHashTableInit(&t, PL_DHashGetStubOps(), nullptr,
                              sizeof(PLDHashEntryStub),
                              mozilla::fallible_t(),
                              PL_DHASH_MAX_INITIAL_LENGTH + 1);
  

  return !ok;   
}

static bool test_pldhash_Init_overflow()
{
  
  
  
  
  
  

  struct OneKBEntry {
      PLDHashEntryHdr hdr;
      char buf[1024 - sizeof(PLDHashEntryHdr)];
  };

  
  PLDHashTable t;
  bool ok = PL_DHashTableInit(&t, nullptr, nullptr,
                              sizeof(OneKBEntry), mozilla::fallible_t(),
                              PL_DHASH_MAX_INITIAL_LENGTH);

  return !ok;   
}


#ifndef MOZ_WIDGET_ANDROID



static PLDHashNumber
hash(PLDHashTable *table, const void *key)
{
  return (PLDHashNumber)(size_t)key;
}

static bool test_pldhash_grow_to_max_capacity()
{
  static const PLDHashTableOps ops = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    hash,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nullptr
  };

  PLDHashTable t;
  bool ok = PL_DHashTableInit(&t, &ops, nullptr, sizeof(PLDHashEntryStub),
                              mozilla::fallible_t(), 128);
  if (!ok)
    return false;

  
  size_t numInserted = 0;
  while (true) {
    if (!PL_DHashTableOperate(&t, (const void*)numInserted, PL_DHASH_ADD)) {
      break;
    }
    numInserted++;
  }

  
  
  return numInserted == PL_DHASH_MAX_CAPACITY - (PL_DHASH_MAX_CAPACITY >> 5);
}
#endif



typedef bool (*TestFunc)();
#define DECL_TEST(name) { #name, name }

static const struct Test {
  const char* name;
  TestFunc    func;
} tests[] = {
  DECL_TEST(test_pldhash_Init_capacity_ok),
  DECL_TEST(test_pldhash_Init_capacity_too_large),
  DECL_TEST(test_pldhash_Init_overflow),

#ifndef MOZ_WIDGET_ANDROID
  DECL_TEST(test_pldhash_grow_to_max_capacity),
#endif
  { nullptr, nullptr }
};

} 

using namespace TestPLDHash;

int main(int argc, char *argv[])
{
  bool success = true;
  for (const Test* t = tests; t->name != nullptr; ++t) {
    bool test_result = t->func();
    printf("%25s : %s\n", t->name, test_result ? "SUCCESS" : "FAILURE");
    if (!test_result)
      success = false;
  }
  return success ? 0 : -1;
}
