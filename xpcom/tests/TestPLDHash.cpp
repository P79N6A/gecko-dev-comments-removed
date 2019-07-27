





#include <stdio.h>
#include "pldhash.h"






namespace TestPLDHash {

static bool test_pldhash_Init_capacity_ok()
{
  PLDHashTable t;

  
  if (t.IsInitialized()) {
    return false;
  }

  
  
  
  if (!PL_DHashTableInit(&t, PL_DHashGetStubOps(), sizeof(PLDHashEntryStub),
                         mozilla::fallible, PL_DHASH_MAX_INITIAL_LENGTH)) {
    return false;
  }

  
  if (!t.IsInitialized()) {
    return false;
  }

  
  PL_DHashTableFinish(&t);
  if (t.IsInitialized()) {
    return false;
  }

  return true;
}

static bool test_pldhash_Init_capacity_too_large()
{
  PLDHashTable t;

  
  if (t.IsInitialized()) {
    return false;
  }

  
  if (PL_DHashTableInit(&t, PL_DHashGetStubOps(),
                        sizeof(PLDHashEntryStub),
                        mozilla::fallible,
                        PL_DHASH_MAX_INITIAL_LENGTH + 1)) {
    return false;   
  }
  

  
  if (t.IsInitialized()) {
    return false;
  }

  return true;
}

static bool test_pldhash_Init_overflow()
{
  PLDHashTable t;

  
  if (t.IsInitialized()) {
    return false;
  }

  
  
  
  
  
  

  struct OneKBEntry {
      PLDHashEntryHdr hdr;
      char buf[1024 - sizeof(PLDHashEntryHdr)];
  };

  if (PL_DHashTableInit(&t, PL_DHashGetStubOps(), sizeof(OneKBEntry),
                        mozilla::fallible, PL_DHASH_MAX_INITIAL_LENGTH)) {
    return false;   
  }
  

  
  if (t.IsInitialized()) {
    return false;
  }

  return true;
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
    hash,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    nullptr
  };

  
  PLDHashTable* t = PL_NewDHashTable(&ops, sizeof(PLDHashEntryStub), 128);

  
  if (!t->IsInitialized()) {
    return false;
  }

  
  size_t numInserted = 0;
  while (true) {
    if (!PL_DHashTableAdd(t, (const void*)numInserted)) {
      break;
    }
    numInserted++;
  }

  
  
  if (numInserted != PL_DHASH_MAX_CAPACITY - (PL_DHASH_MAX_CAPACITY >> 5)) {
    return false;
  }

  PL_DHashTableDestroy(t);

  return true;
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
