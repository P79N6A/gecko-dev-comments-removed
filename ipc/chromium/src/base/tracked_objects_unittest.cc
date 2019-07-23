





#include "base/tracked_objects.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracked_objects {

class TrackedObjectsTest : public testing::Test {
 public:
  MessageLoop message_loop_;
};

TEST_F(TrackedObjectsTest, MinimalStartupShutdown) {
  
  if (!ThreadData::StartTracking(true))
    return;

  EXPECT_FALSE(ThreadData::first());  
  ThreadData* data = ThreadData::current();
  EXPECT_TRUE(ThreadData::first());  
  EXPECT_TRUE(data);
  EXPECT_TRUE(!data->next());
  EXPECT_EQ(data, ThreadData::current());
  ThreadData::BirthMap birth_map;
  data->SnapshotBirthMap(&birth_map);
  EXPECT_EQ(0u, birth_map.size());
  ThreadData::DeathMap death_map;
  data->SnapshotDeathMap(&death_map);
  EXPECT_EQ(0u, death_map.size());
  ThreadData::ShutdownSingleThreadedCleanup();

  
  ThreadData::StartTracking(true);
  EXPECT_FALSE(ThreadData::first());  
  data = ThreadData::current();
  EXPECT_TRUE(ThreadData::first());  
  EXPECT_TRUE(data);
  EXPECT_TRUE(!data->next());
  EXPECT_EQ(data, ThreadData::current());
  birth_map.clear();
  data->SnapshotBirthMap(&birth_map);
  EXPECT_EQ(0u, birth_map.size());
  death_map.clear();
  data->SnapshotDeathMap(&death_map);
  EXPECT_EQ(0u, death_map.size());
  ThreadData::ShutdownSingleThreadedCleanup();
}

class NoopTracked : public tracked_objects::Tracked {
};

TEST_F(TrackedObjectsTest, TinyStartupShutdown) {
  if (!ThreadData::StartTracking(true))
    return;

  
  NoopTracked tracked;

  const ThreadData* data = ThreadData::first();
  EXPECT_TRUE(data);
  EXPECT_TRUE(!data->next());
  EXPECT_EQ(data, ThreadData::current());
  ThreadData::BirthMap birth_map;
  data->SnapshotBirthMap(&birth_map);
  EXPECT_EQ(1u, birth_map.size());                         
  EXPECT_EQ(1, birth_map.begin()->second->birth_count());  
  ThreadData::DeathMap death_map;
  data->SnapshotDeathMap(&death_map);
  EXPECT_EQ(0u, death_map.size());                         


  
  delete new NoopTracked;

  birth_map.clear();
  data->SnapshotBirthMap(&birth_map);
  EXPECT_EQ(1u, birth_map.size());                         
  EXPECT_EQ(2, birth_map.begin()->second->birth_count());  
  death_map.clear();
  data->SnapshotDeathMap(&death_map);
  EXPECT_EQ(1u, death_map.size());                         
  EXPECT_EQ(1, death_map.begin()->second.count());         

  
  EXPECT_EQ(birth_map.begin()->second, death_map.begin()->first);

  ThreadData::ShutdownSingleThreadedCleanup();
}

}  
