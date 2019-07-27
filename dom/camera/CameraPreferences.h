




#ifndef DOM_CAMERA_CAMERAPREFERENCES_H
#define DOM_CAMERA_CAMERAPREFERENCES_H

#include "nsString.h"
#include "nsIObserver.h"
#ifdef MOZ_WIDGET_GONK
#include "mozilla/StaticPtr.h"
#endif

#if defined(MOZ_HAVE_CXX11_STRONG_ENUMS) || defined(MOZ_HAVE_CXX11_ENUM_TYPE)



#define CAMERAPREFERENCES_HAVE_SEPARATE_UINT32_AND_NSRESULT
#endif

namespace mozilla {

template<class T> class StaticAutoPtr;

class CameraPreferences
#ifdef MOZ_WIDGET_GONK
  : public nsIObserver
#endif
{
public:
#ifdef MOZ_WIDGET_GONK
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
#endif

  static bool Initialize();
  static void Shutdown();

  static bool GetPref(const char* aPref, nsACString& aVal);
#ifdef CAMERAPREFERENCES_HAVE_SEPARATE_UINT32_AND_NSRESULT
  static bool GetPref(const char* aPref, nsresult& aVal);
#endif
  static bool GetPref(const char* aPref, uint32_t& aVal);
  static bool GetPref(const char* aPref, bool& aVal);

protected:
  static const uint32_t kPrefNotFound = UINT32_MAX;
  static uint32_t PrefToIndex(const char* aPref);

  static void PreferenceChanged(const char* aPref, void* aClosure);
#ifdef CAMERAPREFERENCES_HAVE_SEPARATE_UINT32_AND_NSRESULT
  static nsresult UpdatePref(const char* aPref, nsresult& aVar);
#endif
  static nsresult UpdatePref(const char* aPref, uint32_t& aVar);
  static nsresult UpdatePref(const char* aPref, nsACString& aVar);
  static nsresult UpdatePref(const char* aPref, bool& aVar);

  enum PrefValueType {
    kPrefValueIsNsResult,
    kPrefValueIsUint32,
    kPrefValueIsCString,
    kPrefValueIsBoolean
  };
  struct Pref {
    const char* const           mPref;
    PrefValueType               mValueType;
    union {
      
      
      
      
      
      void*                     mAsVoid;
      StaticAutoPtr<nsCString>* mAsCString;
      nsresult*                 mAsNsResult;
      uint32_t*                 mAsUint32;
      bool*                     mAsBoolean;
    } mValue;
  };
  static Pref sPrefs[];

  static StaticAutoPtr<nsCString> sPrefTestEnabled;
  static StaticAutoPtr<nsCString> sPrefHardwareTest;
  static StaticAutoPtr<nsCString> sPrefGonkParameters;

  static nsresult sPrefCameraControlMethodErrorOverride;
  static nsresult sPrefCameraControlAsyncErrorOverride;

  static uint32_t sPrefCameraControlLowMemoryThresholdMB;

  static bool sPrefCameraParametersIsLowMemory;

#ifdef MOZ_WIDGET_GONK
  static StaticRefPtr<CameraPreferences> sObserver;

  nsresult PreinitCameraHardware();

protected:
  
  CameraPreferences() { }
  virtual ~CameraPreferences() { }
#else
private:
  
  CameraPreferences();
  ~CameraPreferences();
#endif
};

} 

#endif 
