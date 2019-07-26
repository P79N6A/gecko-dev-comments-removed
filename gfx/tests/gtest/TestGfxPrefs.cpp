




#include "gtest/gtest.h"

#include "gfxPrefs.h"
#ifdef GFX_DECL_PREF
#error "This is not supposed to be defined outside of gfxPrefs.h"
#endif






TEST(GfxPrefs, Singleton) {
  ASSERT_FALSE(gfxPrefs::SingletonExists());
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());
  gfxPrefs::DestroySingleton();
  ASSERT_FALSE(gfxPrefs::SingletonExists());
}

TEST(GfxPrefs, LiveValues) {
  ASSERT_FALSE(gfxPrefs::SingletonExists());
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());

  
  ASSERT_FALSE(gfxPrefs::CanvasAzureAccelerated());

  
  ASSERT_TRUE(gfxPrefs::LayerScopePort() == 23456);

  
  ASSERT_TRUE(gfxPrefs::MSAALevel() == 2);

  gfxPrefs::DestroySingleton();
  ASSERT_FALSE(gfxPrefs::SingletonExists());
}

TEST(GfxPrefs, OnceValues) {
  ASSERT_FALSE(gfxPrefs::SingletonExists());
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());

  
  ASSERT_TRUE(gfxPrefs::WorkAroundDriverBugs());

  
  ASSERT_FALSE(gfxPrefs::LayersDump());

  
  ASSERT_TRUE(gfxPrefs::CanvasSkiaGLCacheSize() == 96);

  
  ASSERT_TRUE(gfxPrefs::APZMaxVelocityQueueSize() == 5);

  
  ASSERT_TRUE(gfxPrefs::APZMaxVelocity() == -1.0f);

  gfxPrefs::DestroySingleton();
  ASSERT_FALSE(gfxPrefs::SingletonExists());
}

