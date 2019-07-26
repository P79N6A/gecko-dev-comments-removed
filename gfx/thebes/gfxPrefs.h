




#ifndef GFX_PREFS_H
#define GFX_PREFS_H

#include <stdint.h>
#include "mozilla/Assertions.h"
#include "mozilla/TypedEnum.h"










































#define DECL_GFX_PREFS(Update, Pref, Name, Type, Default)                     \
public:                                                                       \
static Type Name() { MOZ_ASSERT(Exists()); return One().mPref##Name.mValue; } \
private:                                                                      \
static const char* Get##Name##PrefName() { return Pref; }                     \
PrefTemplate<UpdatePolicy::Update, Type, Default, Get##Name##PrefName> mPref##Name

class gfxPrefs;
class gfxPrefs MOZ_FINAL
{
private:
  
  MOZ_BEGIN_NESTED_ENUM_CLASS(UpdatePolicy)
    Skip, 
    Once, 
    Live  
  MOZ_END_NESTED_ENUM_CLASS(UpdatePolicy)

  
    template <MOZ_ENUM_CLASS_ENUM_TYPE(UpdatePolicy) Update, class T, T Default, const char* Pref(void)>
  class PrefTemplate
  {
  public:
    PrefTemplate()
    : mValue(Default)
    {
      Register(Update, Pref());
    }
    void Register(UpdatePolicy aUpdate, const char* aPreference)
    {
      switch(aUpdate) {
        case UpdatePolicy::Skip:
          break;
        case UpdatePolicy::Once:
          mValue = PrefGet(aPreference, mValue);
          break;
        case UpdatePolicy::Live:
          PrefAddVarCache(&mValue,aPreference, mValue);
          break;
        default:
          MOZ_CRASH();
          break;
      }
    }
    T mValue;
  };

public:
  
  
  

public:
  
  static gfxPrefs& One()
  { 
    if (!sInstance) {
      sInstance = new gfxPrefs;
    }
    return *sInstance;
  }
  static void Destroy();
  static bool Exists();

private:
  static gfxPrefs* sInstance;

private:
  
  static void PrefAddVarCache(bool*, const char*, bool);
  static void PrefAddVarCache(int32_t*, const char*, int32_t);
  static void PrefAddVarCache(uint32_t*, const char*, uint32_t);
  static bool PrefGet(const char*, bool);
  static int32_t PrefGet(const char*, int32_t);
  static uint32_t PrefGet(const char*, uint32_t);

  gfxPrefs();
  ~gfxPrefs();
  gfxPrefs(const gfxPrefs&) MOZ_DELETE;
  gfxPrefs& operator=(const gfxPrefs&) MOZ_DELETE;
};

#undef DECL_GFX_PREFS /* Don't need it outside of this file */

#endif 
