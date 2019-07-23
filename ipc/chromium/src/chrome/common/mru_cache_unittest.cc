



#include "base/basictypes.h"
#include "chrome/common/mru_cache.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

int cached_item_live_count = 0;

struct CachedItem {
  CachedItem() : value(0) {
    cached_item_live_count++;
  }

  CachedItem(int new_value) : value(new_value) {
    cached_item_live_count++;
  }

  CachedItem(const CachedItem& other) : value(other.value) {
    cached_item_live_count++;
  }

  ~CachedItem() {
    cached_item_live_count--;
  }

  int value;
};

}  

TEST(MRUCacheTest, Basic) {
  typedef MRUCache<int, CachedItem> Cache;
  Cache cache(Cache::NO_AUTO_EVICT);

  
  {
    CachedItem test_item;
    EXPECT_TRUE(cache.Get(0) == cache.end());
    EXPECT_TRUE(cache.Peek(0) == cache.end());
  }

  static const int kItem1Key = 5;
  CachedItem item1(10);
  Cache::iterator inserted_item = cache.Put(kItem1Key, item1);
  EXPECT_EQ(1U, cache.size());

  
  {
    Cache::iterator found = cache.Get(kItem1Key);
    EXPECT_TRUE(inserted_item == cache.begin());
    EXPECT_TRUE(found != cache.end());

    found = cache.Peek(kItem1Key);
    EXPECT_TRUE(found != cache.end());

    EXPECT_EQ(kItem1Key, found->first);
    EXPECT_EQ(item1.value, found->second.value);
  }

  static const int kItem2Key = 7;
  CachedItem item2(12);
  cache.Put(kItem2Key, item2);
  EXPECT_EQ(2U, cache.size());

  
  {
    Cache::reverse_iterator oldest = cache.rbegin();
    ASSERT_TRUE(oldest != cache.rend());
    EXPECT_EQ(kItem1Key, oldest->first);
    EXPECT_EQ(item1.value, oldest->second.value);
  }

  
  {
    Cache::iterator test_item = cache.Get(kItem1Key);
    ASSERT_TRUE(test_item != cache.end());
    EXPECT_EQ(kItem1Key, test_item->first);
    EXPECT_EQ(item1.value, test_item->second.value);
  }

  
  {
    Cache::reverse_iterator oldest = cache.rbegin();
    ASSERT_TRUE(oldest != cache.rend());
    EXPECT_EQ(kItem2Key, oldest->first);
    EXPECT_EQ(item2.value, oldest->second.value);
  }

  
  {
    Cache::reverse_iterator next = cache.Erase(cache.rbegin());

    EXPECT_EQ(1U, cache.size());

    EXPECT_TRUE(next == cache.rbegin());
    EXPECT_EQ(kItem1Key, next->first);
    EXPECT_EQ(item1.value, next->second.value);

    cache.Erase(cache.begin());
    EXPECT_EQ(0U, cache.size());
  }
}

TEST(MRUCacheTest, GetVsPeek) {
  typedef MRUCache<int, CachedItem> Cache;
  Cache cache(Cache::NO_AUTO_EVICT);

  static const int kItem1Key = 1;
  CachedItem item1(10);
  cache.Put(kItem1Key, item1);

  static const int kItem2Key = 2;
  CachedItem item2(20);
  cache.Put(kItem2Key, item2);

  
  cache.ShrinkToSize(100);

  
  {
    Cache::reverse_iterator iter = cache.rbegin();
    ASSERT_TRUE(iter != cache.rend());
    EXPECT_EQ(kItem1Key, iter->first);
    EXPECT_EQ(item1.value, iter->second.value);
  }

  
  {
    Cache::iterator peekiter = cache.Peek(kItem1Key);
    ASSERT_TRUE(peekiter != cache.end());

    Cache::reverse_iterator iter = cache.rbegin();
    ASSERT_TRUE(iter != cache.rend());
    EXPECT_EQ(kItem1Key, iter->first);
    EXPECT_EQ(item1.value, iter->second.value);
  }
}

TEST(MRUCacheTest, KeyReplacement) {
  typedef MRUCache<int, CachedItem> Cache;
  Cache cache(Cache::NO_AUTO_EVICT);

  static const int kItem1Key = 1;
  CachedItem item1(10);
  cache.Put(kItem1Key, item1);

  static const int kItem2Key = 2;
  CachedItem item2(20);
  cache.Put(kItem2Key, item2);

  static const int kItem3Key = 3;
  CachedItem item3(30);
  cache.Put(kItem3Key, item3);

  static const int kItem4Key = 4;
  CachedItem item4(40);
  cache.Put(kItem4Key, item4);

  CachedItem item5(50);
  cache.Put(kItem3Key, item5);

  EXPECT_EQ(4U, cache.size());
  for (int i = 0; i < 3; ++i) {
    Cache::reverse_iterator iter = cache.rbegin();
    ASSERT_TRUE(iter != cache.rend());
  }

  
  cache.ShrinkToSize(1);

  Cache::iterator iter = cache.begin();
  EXPECT_EQ(kItem3Key, iter->first);
  EXPECT_EQ(item5.value, iter->second.value);
}


TEST(MRUCacheTest, Owning) {
  typedef OwningMRUCache<int, CachedItem*> Cache;
  Cache cache(Cache::NO_AUTO_EVICT);

  int initial_count = cached_item_live_count;

  
  static const int kItem1Key = 1;
  cache.Put(kItem1Key, new CachedItem(20));
  cache.Put(kItem1Key, new CachedItem(22));

  
  Cache::iterator iter = cache.Get(kItem1Key);
  EXPECT_EQ(1U, cache.size());
  EXPECT_TRUE(iter != cache.end());
  EXPECT_EQ(initial_count + 1, cached_item_live_count);

  
  cache.Erase(cache.begin());
  EXPECT_EQ(initial_count, cached_item_live_count);

  
  
  {
    Cache cache2(Cache::NO_AUTO_EVICT);
    cache2.Put(1, new CachedItem(20));
    cache2.Put(2, new CachedItem(20));
  }

  
  EXPECT_EQ(initial_count, cached_item_live_count);
}

TEST(MRUCacheTest, AutoEvict) {
  typedef OwningMRUCache<int, CachedItem*> Cache;
  static const Cache::size_type kMaxSize = 3;

  int initial_count = cached_item_live_count;

  {
    Cache cache(kMaxSize);

    static const int kItem1Key = 1, kItem2Key = 2, kItem3Key = 3, kItem4Key = 4;
    cache.Put(kItem1Key, new CachedItem(20));
    cache.Put(kItem2Key, new CachedItem(21));
    cache.Put(kItem3Key, new CachedItem(22));
    cache.Put(kItem4Key, new CachedItem(23));

    
    
    EXPECT_EQ(kMaxSize, cache.size());
  }

  
  EXPECT_EQ(initial_count, cached_item_live_count);
}
