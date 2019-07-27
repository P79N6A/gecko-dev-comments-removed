





#include <stdio.h>
#include "pldhash.h"




namespace TestPLDHash {

static bool test_pldhash_Init_capacity_ok()
{
  PLDHashTable t;

  
  if (t.IsInitialized()) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  PL_DHashTableInit(&t, PL_DHashGetStubOps(), sizeof(PLDHashEntryStub),
                    PL_DHASH_MAX_INITIAL_LENGTH);

  
  if (!t.IsInitialized()) {
    return false;
  }

  
  PL_DHashTableFinish(&t);
  if (t.IsInitialized()) {
    return false;
  }

  return true;
}

static bool test_pldhash_lazy_storage()
{
  PLDHashTable t;
  PL_DHashTableInit(&t, PL_DHashGetStubOps(), sizeof(PLDHashEntryStub));

  
  
  

  if (!t.IsInitialized()) {
    return false;
  }

  if (t.Capacity() != 0) {
    return false;
  }

  if (t.EntrySize() != sizeof(PLDHashEntryStub)) {
    return false;
  }

  if (t.EntryCount() != 0) {
    return false;
  }

  if (t.Generation() != 0) {
    return false;
  }

  if (PL_DHashTableSearch(&t, (const void*)1)) {
    return false;   
  }

  
  PL_DHashTableRemove(&t, (const void*)2);

  
  
  PLDHashEnumerator enumerator = nullptr;
  if (PL_DHashTableEnumerate(&t, enumerator, nullptr) != 0) {
    return false;   
  }

  for (PLDHashTable::Iterator iter = t.Iterate();
       iter.HasMoreEntries();
       iter.NextEntry()) {
    return false; 
  }

  
  
  mozilla::MallocSizeOf mallocSizeOf = nullptr;
  if (PL_DHashTableSizeOfExcludingThis(&t, nullptr, mallocSizeOf) != 0) {
    return false;   
  }

  PL_DHashTableFinish(&t);

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
    PL_DHashTableDestroy(t);
    return false;
  }

  
  size_t numInserted = 0;
  while (true) {
    if (!PL_DHashTableAdd(t, (const void*)numInserted, mozilla::fallible)) {
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
  DECL_TEST(test_pldhash_lazy_storage),

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
