















#ifndef DOM_CAMERA_GONKCAMERAPARAMETERS_H
#define DOM_CAMERA_GONKCAMERAPARAMETERS_H

#include <math.h>
#include "camera/CameraParameters.h"
#include "nsTArray.h"
#include "nsString.h"
#include "AutoRwLock.h"
#include "nsPrintfCString.h"
#include "nsClassHashtable.h"
#include "ICameraControl.h"

namespace mozilla {

class GonkCameraParameters
{
public:
  GonkCameraParameters();
  virtual ~GonkCameraParameters();

  
  
  
  
  
  
  
  template<class T> nsresult
  Set(uint32_t aKey, const T& aValue)
  {
    RwLockAutoEnterWrite lock(mLock);
    nsresult rv = SetTranslated(aKey, aValue);
    mDirty = mDirty || NS_SUCCEEDED(rv);
    return rv;
  }

  template<class T> nsresult
  Get(uint32_t aKey, T& aValue)
  {
    RwLockAutoEnterRead lock(mLock);
    return GetTranslated(aKey, aValue);
  }

  bool
  TestAndClearDirtyFlag()
  {
    bool dirty;

    RwLockAutoEnterWrite lock(mLock);
    dirty = mDirty;
    mDirty = false;
    return dirty;
  }

  android::String8
  Flatten() const
  {
    RwLockAutoEnterRead lock(mLock);
    return mParams.flatten();
  }

  nsresult
  Unflatten(const android::String8& aFlatParameters)
  {
    RwLockAutoEnterWrite lock(mLock);
    mParams.unflatten(aFlatParameters);
    if (mInitialized) {
      return NS_OK;
    }

    
    
    
    return Initialize();
  }

protected:
  PRRWLock* mLock;
  bool mDirty;
  bool mInitialized;

  
  double mExposureCompensationStep;
  int32_t mExposureCompensationMinIndex;
  int32_t mExposureCompensationMaxIndex;
  nsTArray<int> mZoomRatios;
  nsTArray<nsString> mIsoModes;
  nsTArray<nsString> mSceneModes;
  nsClassHashtable<nsStringHashKey, nsCString> mIsoModeMap;

  
  
  class Parameters : public android::CameraParameters
  {
  public:
    using android::CameraParameters::set;
    using android::CameraParameters::get;
    using android::CameraParameters::TRUE;
    using android::CameraParameters::FALSE;

    void set(const char* aKey, float aValue)      { setFloat(aKey, aValue); }
    void set(const char* aKey, double aValue)     { setFloat(aKey, aValue); }
    void set(const char* aKey, bool aValue)       { set(aKey, aValue ? TRUE : FALSE); }
    void get(const char* aKey, float& aRet)       { aRet = getFloat(aKey); }
    void get(const char* aKey, double& aRet)      { aRet = getFloat(aKey); }
    void get(const char* aKey, const char*& aRet) { aRet = get(aKey); }
    void get(const char* aKey, int& aRet)         { aRet = getInt(aKey); }
    void get(const char* aKey, bool& aRet)        { aRet = strcmp(get(aKey), FALSE); }

    void remove(const char* aKey)                 { android::CameraParameters::remove(aKey); }

    static const char* GetTextKey(uint32_t aKey);
  };

  Parameters mParams;

  
  
  
  
  
  
  
  template<typename T> nsresult
  SetImpl(uint32_t aKey, const T& aValue)
  {
    const char* key = Parameters::GetTextKey(aKey);
    NS_ENSURE_TRUE(key, NS_ERROR_NOT_IMPLEMENTED);

    mParams.set(key, aValue);
    return NS_OK;
  }

  template<typename T> nsresult
  GetImpl(uint32_t aKey, T& aValue)
  {
    const char* key = Parameters::GetTextKey(aKey);
    NS_ENSURE_TRUE(key, NS_ERROR_NOT_IMPLEMENTED);

    mParams.get(key, aValue);
    return NS_OK;
  }

  template<class T> nsresult
  SetImpl(const char* aKey, const T& aValue)
  {
    mParams.set(aKey, aValue);
    return NS_OK;
  }

  template<class T> nsresult
  GetImpl(const char* aKey, T& aValue)
  {
    mParams.get(aKey, aValue);
    return NS_OK;
  }

  nsresult
  ClearImpl(const char* aKey)
  {
    mParams.remove(aKey);
    return NS_OK;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult SetTranslated(uint32_t aKey, const nsAString& aValue);
  nsresult GetTranslated(uint32_t aKey, nsAString& aValue);
  nsresult SetTranslated(uint32_t aKey, const ICameraControl::Size& aSize);
  nsresult GetTranslated(uint32_t aKey, ICameraControl::Size& aSize);
  nsresult GetTranslated(uint32_t aKey, nsTArray<ICameraControl::Size>& aSizes);
  nsresult SetTranslated(uint32_t aKey, const nsTArray<ICameraControl::Region>& aRegions);
  nsresult GetTranslated(uint32_t aKey, nsTArray<ICameraControl::Region>& aRegions);
  nsresult SetTranslated(uint32_t aKey, const ICameraControl::Position& aPosition);
  nsresult SetTranslated(uint32_t aKey, const int64_t& aValue);
  nsresult GetTranslated(uint32_t aKey, int64_t& aValue);
  nsresult SetTranslated(uint32_t aKey, const double& aValue);
  nsresult GetTranslated(uint32_t aKey, double& aValue);
  nsresult SetTranslated(uint32_t aKey, const int& aValue);
  nsresult GetTranslated(uint32_t aKey, int& aValue);
  nsresult SetTranslated(uint32_t aKey, const uint32_t& aValue);
  nsresult GetTranslated(uint32_t aKey, uint32_t& aValue);
  nsresult SetTranslated(uint32_t aKey, const bool& aValue);
  nsresult GetTranslated(uint32_t aKey, bool& aValue);
  nsresult GetTranslated(uint32_t aKey, nsTArray<nsString>& aValues);
  nsresult GetTranslated(uint32_t aKey, nsTArray<double>& aValues);

  
  
  
  
  
  
  
  template<class T> nsresult GetListAsArray(uint32_t aKey, nsTArray<T>& aArray);

  
  
  
  
  
  
  
  nsresult MapIsoToGonk(const nsAString& aIso, nsACString& aIsoOut);
  nsresult MapIsoFromGonk(const char* aIso, nsAString& aIsoOut);

  
  
  nsresult Initialize();

  
  
  static bool IsLowMemoryPlatform();
};

} 

#endif 
