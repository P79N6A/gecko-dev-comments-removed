





#include "pldhash.h"
#include "gtest/gtest.h"




#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


extern unsigned int _gdb_sleep_duration;
#endif









void
TestCrashyOperation(void (*aCrashyOperation)())
{
#if defined(XP_UNIX) && defined(DEBUG) && !defined(MOZ_ASAN)
  
  
  unsigned int old_gdb_sleep_duration = _gdb_sleep_duration;
  _gdb_sleep_duration = 0;

  int pid = fork();
  ASSERT_NE(pid, -1);

  if (pid == 0) {
    
    aCrashyOperation();
    fprintf(stderr, "TestCrashyOperation: didn't crash?!\n");
    ASSERT_TRUE(false);   
  }

  
  int status;
  ASSERT_NE(waitpid(pid, &status, 0), -1);

  
  ASSERT_TRUE(WIFEXITED(status) || WTERMSIG(status));
  if (WIFEXITED(status)) {
    
    
    int signum = WEXITSTATUS(status);
    if (signum != SIGSEGV && signum != SIGBUS) {
      fprintf(stderr, "TestCrashyOperation 'exited' failure: %d\n", signum);
      ASSERT_TRUE(false);
    }
  } else if (WIFSIGNALED(status)) {
    
    
    int signum = WTERMSIG(status);
    if (signum != SIGSEGV && signum != SIGBUS) {
      fprintf(stderr, "TestCrashyOperation 'signaled' failure: %d\n", signum);
      ASSERT_TRUE(false);
    }
  }

  _gdb_sleep_duration = old_gdb_sleep_duration;
#endif
}

void
InitCapacityOk_InitialLengthTooBig()
{
  PLDHashTable t(PL_DHashGetStubOps(), sizeof(PLDHashEntryStub),
                 PLDHashTable::kMaxInitialLength + 1);
}

void
InitCapacityOk_InitialEntryStoreTooBig()
{
  
  
  
  PLDHashTable t(PL_DHashGetStubOps(), (uint32_t)1 << 23, (uint32_t)1 << 8);
}

TEST(PLDHashTableTest, InitCapacityOk)
{
  
  
  
  PLDHashTable t1(PL_DHashGetStubOps(), sizeof(PLDHashEntryStub),
                  PLDHashTable::kMaxInitialLength);

  
  
  PLDHashTable t2(PL_DHashGetStubOps(), (uint32_t)1 << 23, (uint32_t)1 << 7);

  
  TestCrashyOperation(InitCapacityOk_InitialLengthTooBig);

  
  
  TestCrashyOperation(InitCapacityOk_InitialEntryStoreTooBig);

  
  
  
  
}

TEST(PLDHashTableTest, LazyStorage)
{
  PLDHashTable t(PL_DHashGetStubOps(), sizeof(PLDHashEntryStub));

  
  
  

  ASSERT_EQ(t.Capacity(), 0u);
  ASSERT_EQ(t.EntrySize(), sizeof(PLDHashEntryStub));
  ASSERT_EQ(t.EntryCount(), 0u);
  ASSERT_EQ(t.Generation(), 0u);

  ASSERT_TRUE(!PL_DHashTableSearch(&t, (const void*)1));

  
  PL_DHashTableRemove(&t, (const void*)2);

  for (auto iter = t.Iter(); !iter.Done(); iter.Next()) {
    ASSERT_TRUE(false); 
  }

  
  
  mozilla::MallocSizeOf mallocSizeOf = nullptr;
  ASSERT_EQ(PL_DHashTableSizeOfExcludingThis(&t, nullptr, mallocSizeOf), 0u);
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

TEST(PLDHashTableTest, MoveSemantics)
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
}

TEST(PLDHashTableTest, Clear)
{
  PLDHashTable t1(&trivialOps, sizeof(PLDHashEntryStub));

  t1.Clear();
  ASSERT_EQ(t1.EntryCount(), 0u);

  t1.ClearAndPrepareForLength(100);
  ASSERT_EQ(t1.EntryCount(), 0u);

  PL_DHashTableAdd(&t1, (const void*)77);
  PL_DHashTableAdd(&t1, (const void*)88);
  PL_DHashTableAdd(&t1, (const void*)99);
  ASSERT_EQ(t1.EntryCount(), 3u);

  t1.Clear();
  ASSERT_EQ(t1.EntryCount(), 0u);

  PL_DHashTableAdd(&t1, (const void*)55);
  PL_DHashTableAdd(&t1, (const void*)66);
  PL_DHashTableAdd(&t1, (const void*)77);
  PL_DHashTableAdd(&t1, (const void*)88);
  PL_DHashTableAdd(&t1, (const void*)99);
  ASSERT_EQ(t1.EntryCount(), 5u);

  t1.ClearAndPrepareForLength(8192);
  ASSERT_EQ(t1.EntryCount(), 0u);
}

TEST(PLDHashTableTest, Iterator)
{
  PLDHashTable t(&trivialOps, sizeof(PLDHashEntryStub));

  
  
  
  {
    PLDHashTable::Iterator iter1(&t);
    PLDHashTable::Iterator iter2(mozilla::Move(iter1));
  }

  
  for (PLDHashTable::Iterator iter(&t); !iter.Done(); iter.Next()) {
    (void) iter.Get();
    ASSERT_TRUE(false); 
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
  ASSERT_TRUE(saw77 && saw88 && saw99 && n == 3);

  t.Clear();

  
  
  for (intptr_t i = 0; i < 64; i++) {
    PL_DHashTableAdd(&t, (const void*)i);
  }
  ASSERT_EQ(t.EntryCount(), 64u);
  ASSERT_EQ(t.Capacity(), 128u);

  
  
  for (PLDHashTable::Iterator iter(&t); !iter.Done(); iter.Next()) {
    (void) iter.Get();
  }
  ASSERT_EQ(t.EntryCount(), 64u);
  ASSERT_EQ(t.Capacity(), 128u);

  
  
  for (auto iter = t.Iter(); !iter.Done(); iter.Next()) {
    auto entry = static_cast<PLDHashEntryStub*>(iter.Get());
    if ((intptr_t)(entry->key) % 4 == 0) {
      iter.Remove();
    }
  }
  ASSERT_EQ(t.EntryCount(), 48u);
  ASSERT_EQ(t.Capacity(), 128u);

  
  
  for (auto iter = t.Iter(); !iter.Done(); iter.Next()) {
    auto entry = static_cast<PLDHashEntryStub*>(iter.Get());
    if ((intptr_t)(entry->key) % 2 == 0) {
      iter.Remove();
    }
  }
  ASSERT_EQ(t.EntryCount(), 32u);
  ASSERT_EQ(t.Capacity(), 64u);

  
  
  for (auto iter = t.Iter(); !iter.Done(); iter.Next()) {
    iter.Remove();
  }
  ASSERT_EQ(t.EntryCount(), 0u);
  ASSERT_EQ(t.Capacity(), unsigned(PLDHashTable::kMinCapacity));
}



#ifndef MOZ_WIDGET_ANDROID
TEST(PLDHashTableTest, GrowToMaxCapacity)
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

  
  
  if (numInserted !=
      PLDHashTable::kMaxCapacity - (PLDHashTable::kMaxCapacity >> 5)) {
    delete t;
    ASSERT_TRUE(false);
  }

  delete t;
}
#endif

