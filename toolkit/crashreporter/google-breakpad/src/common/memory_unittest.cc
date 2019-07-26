




























#include "breakpad_googletest_includes.h"
#include "common/memory.h"

using namespace google_breakpad;

namespace {
typedef testing::Test PageAllocatorTest;
}

TEST(PageAllocatorTest, Setup) {
  PageAllocator allocator;
}

TEST(PageAllocatorTest, SmallObjects) {
  PageAllocator allocator;

  for (unsigned i = 1; i < 1024; ++i) {
    uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(i));
    ASSERT_FALSE(p == NULL);
    memset(p, 0, i);
  }
}

TEST(PageAllocatorTest, LargeObject) {
  PageAllocator allocator;

  uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(10000));
  ASSERT_FALSE(p == NULL);
  for (unsigned i = 1; i < 10; ++i) {
    uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(i));
    ASSERT_FALSE(p == NULL);
    memset(p, 0, i);
  }
}

namespace {
typedef testing::Test WastefulVectorTest;
}

TEST(WastefulVectorTest, Setup) {
  PageAllocator allocator_;
  wasteful_vector<int> v(&allocator_);
  ASSERT_TRUE(v.empty());
  ASSERT_EQ(v.size(), 0u);
}

TEST(WastefulVectorTest, Simple) {
  PageAllocator allocator_;
  wasteful_vector<unsigned> v(&allocator_);

  for (unsigned i = 0; i < 256; ++i) {
    v.push_back(i);
    ASSERT_EQ(i, v.back());
    ASSERT_EQ(&v.back(), &v[i]);
  }
  ASSERT_FALSE(v.empty());
  ASSERT_EQ(v.size(), 256u);
  for (unsigned i = 0; i < 256; ++i)
    ASSERT_EQ(v[i], i);
}
