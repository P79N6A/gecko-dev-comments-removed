




#include "gtest/gtest.h"

#include "gfxPrefs.h"
#ifdef GFX_DECL_PREF
#error "This is not supposed to be defined outside of gfxPrefs.h"
#endif






TEST(GfxPrefs, Singleton) {
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());
}

TEST(GfxPrefs, LiveValues) {
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());

  
  ASSERT_FALSE(gfxPrefs::CanvasAzureAccelerated());

  
  ASSERT_TRUE(gfxPrefs::LayerScopePort() == 23456);

  
  ASSERT_TRUE(gfxPrefs::MSAALevel() == 2);
}

TEST(GfxPrefs, OnceValues) {
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());

  
  ASSERT_TRUE(gfxPrefs::WorkAroundDriverBugs());

  
  ASSERT_FALSE(gfxPrefs::LayersDump());

  
  ASSERT_TRUE(gfxPrefs::CanvasSkiaGLCacheSize() == 96);

  
  ASSERT_TRUE(gfxPrefs::APZMaxVelocityQueueSize() == 5);

  
  ASSERT_TRUE(gfxPrefs::APZMaxVelocity() == -1.0f);
}

TEST(GfxPrefs, Set) {
  gfxPrefs::GetSingleton();
  ASSERT_TRUE(gfxPrefs::SingletonExists());

  
  ASSERT_FALSE(gfxPrefs::LayersDump());
  gfxPrefs::SetLayersDump(true);
  ASSERT_TRUE(gfxPrefs::LayersDump());
  gfxPrefs::SetLayersDump(false);
  ASSERT_FALSE(gfxPrefs::LayersDump());

  
  ASSERT_FALSE(gfxPrefs::CanvasAzureAccelerated());
  gfxPrefs::SetCanvasAzureAccelerated(true);
  ASSERT_TRUE(gfxPrefs::CanvasAzureAccelerated());
  gfxPrefs::SetCanvasAzureAccelerated(false);
  ASSERT_FALSE(gfxPrefs::CanvasAzureAccelerated());

  
  ASSERT_TRUE(gfxPrefs::APZMaxVelocity() == -1.0f);
  gfxPrefs::SetAPZMaxVelocity(1.75f);
  ASSERT_TRUE(gfxPrefs::APZMaxVelocity() == 1.75f);
  gfxPrefs::SetAPZMaxVelocity(-1.0f);
  ASSERT_TRUE(gfxPrefs::APZMaxVelocity() == -1.0f);
}
