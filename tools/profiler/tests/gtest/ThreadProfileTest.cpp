




#include "gtest/gtest.h"

#include "ProfileEntry.h"
#include "ThreadProfile.h"


TEST(ThreadProfile, Initialization) {
  PseudoStack* stack = PseudoStack::create();
  Thread::tid_t tid = 1000;
  ThreadInfo info("testThread", tid, true, stack, nullptr);
  mozilla::RefPtr<ProfileBuffer> pb = new ProfileBuffer(10);
  ThreadProfile tp(&info, pb);
}


TEST(ThreadProfile, InsertOneTag) {
  PseudoStack* stack = PseudoStack::create();
  Thread::tid_t tid = 1000;
  ThreadInfo info("testThread", tid, true, stack, nullptr);
  mozilla::RefPtr<ProfileBuffer> pb = new ProfileBuffer(10);
  pb->addTag(ProfileEntry('t', 123.1));
  ASSERT_TRUE(pb->mEntries != nullptr);
  ASSERT_TRUE(pb->mEntries[pb->mReadPos].mTagName == 't');
  ASSERT_TRUE(pb->mEntries[pb->mReadPos].mTagDouble == 123.1);
}


TEST(ThreadProfile, InsertTagsNoWrap) {
  PseudoStack* stack = PseudoStack::create();
  Thread::tid_t tid = 1000;
  ThreadInfo info("testThread", tid, true, stack, nullptr);
  mozilla::RefPtr<ProfileBuffer> pb = new ProfileBuffer(100);
  int test_size = 50;
  for (int i = 0; i < test_size; i++) {
    pb->addTag(ProfileEntry('t', i));
  }
  ASSERT_TRUE(pb->mEntries != nullptr);
  int readPos = pb->mReadPos;
  while (readPos != pb->mWritePos) {
    ASSERT_TRUE(pb->mEntries[readPos].mTagName == 't');
    ASSERT_TRUE(pb->mEntries[readPos].mTagInt == readPos);
    readPos = (readPos + 1) % pb->mEntrySize;
  }
}


TEST(ThreadProfile, InsertTagsWrap) {
  PseudoStack* stack = PseudoStack::create();
  Thread::tid_t tid = 1000;
  
  int tags = 24;
  int buffer_size = tags + 1;
  ThreadInfo info("testThread", tid, true, stack, nullptr);
  mozilla::RefPtr<ProfileBuffer> pb = new ProfileBuffer(buffer_size);
  int test_size = 43;
  for (int i = 0; i < test_size; i++) {
    pb->addTag(ProfileEntry('t', i));
  }
  ASSERT_TRUE(pb->mEntries != nullptr);
  int readPos = pb->mReadPos;
  int ctr = 0;
  while (readPos != pb->mWritePos) {
    ASSERT_TRUE(pb->mEntries[readPos].mTagName == 't');
    
    ASSERT_TRUE(pb->mEntries[readPos].mTagInt == ctr + (test_size - tags));
    ctr++;
    readPos = (readPos + 1) % pb->mEntrySize;
  }
}

