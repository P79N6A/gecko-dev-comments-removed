





#include <stdio.h>
#include "pldhash.h"




namespace TestPLDHash {

static bool test_pldhash_Init_capacity_ok()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  PLDHashTable t(PL_DHashGetStubOps(), sizeof(PLDHashEntryStub),
                 PL_DHASH_MAX_INITIAL_LENGTH);

  return true;
}

static bool test_pldhash_lazy_storage()
{
  PLDHashTable t(PL_DHashGetStubOps(), sizeof(PLDHashEntryStub));

  
  
  

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

  for (auto iter = t.Iter(); !iter.Done(); iter.Get()) {
    return false; 
  }

  
  
  mozilla::MallocSizeOf mallocSizeOf = nullptr;
  if (PL_DHashTableSizeOfExcludingThis(&t, nullptr, mallocSizeOf) != 0) {
    return false;   
  }

  return true;
}




static PLDHashNumber
TrivialHash(PLDHashTable *table, const void *key)
{
  return (PLDHashNumber)(size_t)key;
}

static void
TrivialInitEntry(PLDHashEntryHdr* aEntry, const void* aKey)
{
  auto entry = static_cast<PLDHashEntryStub*>(aEntry);
  entry->key = aKey;
}

static const PLDHashTableOps trivialOps = {
  TrivialHash,
  PL_DHashMatchEntryStub,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  TrivialInitEntry
};

static bool test_pldhash_move_semantics()
{
  PLDHashTable t1(&trivialOps, sizeof(PLDHashEntryStub));
  PL_DHashTableAdd(&t1, (const void*)88);
  PLDHashTable t2(&trivialOps, sizeof(PLDHashEntryStub));
  PL_DHashTableAdd(&t2, (const void*)99);

  t1 = mozilla::Move(t1);   

  t1 = mozilla::Move(t2);   

  PLDHashTable t3(&trivialOps, sizeof(PLDHashEntryStub));
  PLDHashTable t4(&trivialOps, sizeof(PLDHashEntryStub));
  PL_DHashTableAdd(&t3, (const void*)88);

  t3 = mozilla::Move(t4);   

  PLDHashTable t5(&trivialOps, sizeof(PLDHashEntryStub));
  PLDHashTable t6(&trivialOps, sizeof(PLDHashEntryStub));
  PL_DHashTableAdd(&t6, (const void*)88);

  t5 = mozilla::Move(t6);   

  PLDHashTable t7(&trivialOps, sizeof(PLDHashEntryStub));
  PLDHashTable t8(mozilla::Move(t7));  

  PLDHashTable t9(&trivialOps, sizeof(PLDHashEntryStub));
  PL_DHashTableAdd(&t9, (const void*)88);
  PLDHashTable t10(mozilla::Move(t9));  

  return true;
}

static bool test_pldhash_Clear()
{
  PLDHashTable t1(&trivialOps, sizeof(PLDHashEntryStub));

  t1.Clear();
  if (t1.EntryCount() != 0) {
    return false;
  }

  t1.ClearAndPrepareForLength(100);
  if (t1.EntryCount() != 0) {
    return false;
  }

  PL_DHashTableAdd(&t1, (const void*)77);
  PL_DHashTableAdd(&t1, (const void*)88);
  PL_DHashTableAdd(&t1, (const void*)99);
  if (t1.EntryCount() != 3) {
    return false;
  }

  t1.Clear();
  if (t1.EntryCount() != 0) {
    return false;
  }

  PL_DHashTableAdd(&t1, (const void*)55);
  PL_DHashTableAdd(&t1, (const void*)66);
  PL_DHashTableAdd(&t1, (const void*)77);
  PL_DHashTableAdd(&t1, (const void*)88);
  PL_DHashTableAdd(&t1, (const void*)99);
  if (t1.EntryCount() != 5) {
    return false;
  }

  t1.ClearAndPrepareForLength(8192);
  if (t1.EntryCount() != 0) {
    return false;
  }

  return true;
}

static bool test_pldhash_Iterator()
{
  PLDHashTable t(&trivialOps, sizeof(PLDHashEntryStub));

  
  
  
  {
    PLDHashTable::Iterator iter1(&t);
    PLDHashTable::Iterator iter2(mozilla::Move(iter1));
  }

  
  for (PLDHashTable::Iterator iter(&t); !iter.Done(); iter.Next()) {
    (void) iter.Get();
    return false;   
  }

  
  PL_DHashTableAdd(&t, (const void*)77);
  PL_DHashTableAdd(&t, (const void*)88);
  PL_DHashTableAdd(&t, (const void*)99);

  
  bool saw77 = false, saw88 = false, saw99 = false;
  int n = 0;
  for (auto iter(t.Iter()); !iter.Done(); iter.Next()) {
    auto entry = static_cast<PLDHashEntryStub*>(iter.Get());
    if (entry->key == (const void*)77) {
      saw77 = true;
    }
    if (entry->key == (const void*)88) {
      saw88 = true;
    }
    if (entry->key == (const void*)99) {
      saw99 = true;
    }
    n++;
  }
  if (!saw77 || !saw88 || !saw99 || n != 3) {
    return false;
  }

  return true;
}


#ifndef MOZ_WIDGET_ANDROID
static bool test_pldhash_grow_to_max_capacity()
{
  
  PLDHashTable* t =
    new PLDHashTable(&trivialOps, sizeof(PLDHashEntryStub), 128);

  
  size_t numInserted = 0;
  while (true) {
    if (!PL_DHashTableAdd(t, (const void*)numInserted, mozilla::fallible)) {
      break;
    }
    numInserted++;
  }

  
  
  if (numInserted != PL_DHASH_MAX_CAPACITY - (PL_DHASH_MAX_CAPACITY >> 5)) {
    delete t;
    return false;
  }

  delete t;
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
  DECL_TEST(test_pldhash_move_semantics),
  DECL_TEST(test_pldhash_Clear),
  DECL_TEST(test_pldhash_Iterator),


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
    printf("%35s : %s\n", t->name, test_result ? "SUCCESS" : "FAILURE");
    if (!test_result)
      success = false;
  }
  return success ? 0 : -1;
}
