




#include "gtest/gtest.h"

#include "ProfileEntry.h"


TEST(ThreadProfile, Initialization) {
  PseudoStack stack;
  Thread::tid_t tid = 1000;
  ThreadProfile tp("testThread", 10, &stack, tid, nullptr, true, nullptr);
}


TEST(ThreadProfile, InsertOneTag) {
  PseudoStack stack;
  Thread::tid_t tid = 1000;
  ThreadProfile tp("testThread", 10, &stack, tid, nullptr, true, nullptr);
  tp.addTag(ProfileEntry('t', 123.1f));
  ASSERT_TRUE(tp.mEntries != nullptr);
  ASSERT_TRUE(tp.mEntries[tp.mReadPos].mTagName == 't');
  ASSERT_TRUE(tp.mEntries[tp.mReadPos].mTagFloat == 123.1f);
}


TEST(ThreadProfile, InsertTagsNoWrap) {
  PseudoStack stack;
  Thread::tid_t tid = 1000;
  ThreadProfile tp("testThread", 100, &stack, tid, nullptr, true, nullptr);
  int test_size = 50;
  for (int i = 0; i < test_size; i++) {
    tp.addTag(ProfileEntry('t', i));
  }
  ASSERT_TRUE(tp.mEntries != nullptr);
  int readPos = tp.mReadPos;
  while (readPos != tp.mWritePos) {
    ASSERT_TRUE(tp.mEntries[readPos].mTagName == 't');
    ASSERT_TRUE(tp.mEntries[readPos].mTagInt == readPos);
    readPos = (readPos + 1) % tp.mEntrySize;
  }
}


TEST(ThreadProfile, InsertTagsWrap) {
  PseudoStack stack;
  Thread::tid_t tid = 1000;
  
  int tags = 24;
  int buffer_size = tags + 1;
  ThreadProfile tp("testThread", buffer_size, &stack, tid, nullptr, true, nullptr);
  int test_size = 43;
  for (int i = 0; i < test_size; i++) {
    tp.addTag(ProfileEntry('t', i));
  }
  ASSERT_TRUE(tp.mEntries != nullptr);
  int readPos = tp.mReadPos;
  int ctr = 0;
  while (readPos != tp.mWritePos) {
    ASSERT_TRUE(tp.mEntries[readPos].mTagName == 't');
    
    ASSERT_TRUE(tp.mEntries[readPos].mTagInt == ctr + (test_size - tags));
    ctr++;
    readPos = (readPos + 1) % tp.mEntrySize;
  }
}

