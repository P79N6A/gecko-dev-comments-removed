




#include "gfxPrefs.h"

#include "mozilla/Preferences.h"
#include "MainThreadUtils.h"

using namespace mozilla;

gfxPrefs* gfxPrefs::sInstance = nullptr;

void
gfxPrefs::Destroy()
{
  if (sInstance) {
    delete sInstance;
    sInstance = nullptr;
  }
}

bool
gfxPrefs::Exists()
{
  return sInstance != nullptr;
}

gfxPrefs::gfxPrefs()
{
  
  MOZ_ASSERT(NS_IsMainThread(), "must be constructed on the main thread");
}

gfxPrefs::~gfxPrefs()
{
  MOZ_ASSERT(NS_IsMainThread(), "must be destructed on the main thread");
}

void gfxPrefs::PrefAddVarCache(bool* aVariable,
                               const char* aPref,
                               bool aDefault)
{
  Preferences::AddBoolVarCache(aVariable, aPref, aDefault);
}

void gfxPrefs::PrefAddVarCache(int32_t* aVariable,
                               const char* aPref,
                               int32_t aDefault)
{
  Preferences::AddIntVarCache(aVariable, aPref, aDefault);
}

void gfxPrefs::PrefAddVarCache(uint32_t* aVariable,
                               const char* aPref,
                               uint32_t aDefault)
{
  Preferences::AddUintVarCache(aVariable, aPref, aDefault);
}

void gfxPrefs::PrefAddVarCache(float* aVariable,
                               const char* aPref,
                               float aDefault)
{
  Preferences::AddFloatVarCache(aVariable, aPref, aDefault);
}

bool gfxPrefs::PrefGet(const char* aPref, bool aDefault)
{
  return Preferences::GetBool(aPref, aDefault);
}

int32_t gfxPrefs::PrefGet(const char* aPref, int32_t aDefault)
{
  return Preferences::GetInt(aPref, aDefault);
}

uint32_t gfxPrefs::PrefGet(const char* aPref, uint32_t aDefault)
{
  return Preferences::GetUint(aPref, aDefault);
}

float gfxPrefs::PrefGet(const char* aPref, float aDefault)
{
  return Preferences::GetFloat(aPref, aDefault);
}

