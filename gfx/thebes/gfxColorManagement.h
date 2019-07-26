




#ifndef GFX_COLORMANAGEMENT_H
#define GFX_COLORMANAGEMENT_H

#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "gfxColor.h"
#include "nsString.h"
#include "qcms.h"

class gfxColorManagement;
namespace mozilla {
class Mutex;
}

enum eCMSMode {
    eCMSMode_Off          = 0,     
    eCMSMode_All          = 1,     
    eCMSMode_TaggedOnly   = 2,     
    eCMSMode_AllCount     = 3
};






class gfxColorManagement MOZ_FINAL
{
public:
  


  static const gfxColorManagement& Instance();
  static void Destroy();
  static bool Exists();

  


  void Initialize(qcms_profile* aDefaultPlatformProfile);

  


  void ResetAll();

  



  qcms_profile* GetOutputProfile() const;

  



  eCMSMode GetMode() const;
  int GetRenderingIntent() const;

  qcms_profile* GetsRGBProfile() const;
  qcms_transform* GetRGBTransform() const;
  qcms_transform* GetInverseRGBTransform() const;
  qcms_transform* GetRGBATransform() const;

  




  void TransformPixel(const gfxRGBA& aIn, gfxRGBA& aOut, qcms_transform* aTransform) const;

private:
  nsCOMPtr<nsIObserver> mPrefsObserver;
  mozilla::Mutex* mPrefLock;
  bool mPrefEnableV4;
  bool mPrefForcesRGB;
  int32_t mPrefMode;
  int32_t mPrefIntent;
  nsAdoptingCString mPrefDisplayProfile;
#ifdef DEBUG
  bool mPrefsInitialized;
#endif

  mutable bool mModeSet;
  mutable eCMSMode mMode;
  mutable int mIntent;

  
  mutable qcms_profile *mOutputProfile;
  mutable qcms_profile *msRGBProfile;

  mutable qcms_transform *mRGBTransform;
  mutable qcms_transform *mInverseRGBTransform;
  mutable qcms_transform *mRGBATransform;

  qcms_profile* mDefaultPlatformProfile;

private:
  static gfxColorManagement* sInstance;

  gfxColorManagement();
  ~gfxColorManagement();

  
  
  gfxColorManagement(const gfxColorManagement&);
  gfxColorManagement& operator=(const gfxColorManagement&);

  void MigratePreferences();
  NS_IMETHODIMP PreferencesModified(const PRUnichar* prefName);

  
  
  
  friend class gfxPlatform;
  friend class CMSPrefsObserver;
  static gfxColorManagement& InstanceNC();
};

#endif 
