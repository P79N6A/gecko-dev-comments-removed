




#ifndef DOM_CAMERA_CAMERAPREFERENCES_H
#define DOM_CAMERA_CAMERAPREFERENCES_H

#include "nsString.h"

namespace mozilla {

template<class T> class StaticAutoPtr;

class CameraPreferences
{
public:
  static bool Initialize();
  static void Shutdown();

  static bool GetPref(const char* aPref, nsACString& aVal);
  static bool GetPref(const char* aPref, nsresult& aVal);

protected:
  static const uint32_t kPrefNotFound = UINT32_MAX;
  static uint32_t PrefToIndex(const char* aPref);

  static void PreferenceChanged(const char* aPref, void* aClosure);
  static nsresult UpdatePref(const char* aPref, nsresult& aVar);
  static nsresult UpdatePref(const char* aPref, nsACString& aVar);

  enum PrefValueType {
    kPrefValueIsNSResult,
    kPrefValueIsCString
  };
  struct Pref {
    const char* const           mPref;
    PrefValueType               mValueType;
    union {
      
      
      
      
      
      void*                     mAsVoid;
      StaticAutoPtr<nsCString>* mAsCString;
      nsresult*                 mAsNsResult;
    } mValue;
  };
  static Pref sPrefs[];

  static StaticAutoPtr<nsCString> sPrefTestEnabled;
  static StaticAutoPtr<nsCString> sPrefHardwareTest;
  static StaticAutoPtr<nsCString> sPrefGonkParameters;

  static nsresult sPrefCameraControlMethodErrorOverride;
  static nsresult sPrefCameraControlAsyncErrorOverride;

private:
  
  CameraPreferences();
  ~CameraPreferences();
};

} 

#endif 
