




#include "gfxColorManagement.h"
#include "mozilla/Preferences.h"
#include "mozilla/Mutex.h"
#include "nsCRTGlue.h"

using namespace mozilla;

gfxColorManagement* gfxColorManagement::sInstance = nullptr;


#define kCMSPrefNameForcesRGB       "gfx.color_management.force_srgb"
#define kCMSPrefNameMode            "gfx.color_management.mode"
#define kCMSPrefNameEnablev4        "gfx.color_management.enablev4"
#define kCMSPrefNameRenderingIntent "gfx.color_management.rendering_intent"
#define kCMSPrefNameDisplayProfile  "gfx.color_management.display_profile"
#define kCMSObsoletePrefEnabled     "gfx.color_management.enabled"


static const char* kObservedCMSPrefs[] = {
  kCMSPrefNameForcesRGB,
  kCMSPrefNameMode,
  kCMSPrefNameEnablev4,
  kCMSPrefNameRenderingIntent,
  kCMSPrefNameDisplayProfile,
  nullptr
};



class CMSPrefsObserver MOZ_FINAL : public nsIObserver,
                                   public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS2(CMSPrefsObserver, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
CMSPrefsObserver::Observe(nsISupports*, const char*, const PRUnichar *aData)
{
  return gfxColorManagement::InstanceNC().PreferencesModified(aData);
}

const gfxColorManagement&
gfxColorManagement::Instance()
{
  if (!sInstance) {
    sInstance = new gfxColorManagement;
  }
  return *sInstance;
}

gfxColorManagement&
gfxColorManagement::InstanceNC()
{
  if (!sInstance) {
    sInstance = new gfxColorManagement;
  }
  return *sInstance;
}

void
gfxColorManagement::Destroy()
{
  if (sInstance) {
    delete sInstance;
    sInstance = nullptr;
  }
}

bool
gfxColorManagement::Exists()
{
  return sInstance != nullptr;
}

gfxColorManagement::gfxColorManagement()
  : mPrefsObserver(nullptr),
    mPrefLock(nullptr),
    mPrefEnableV4(false),
    mPrefForcesRGB(false),
    mPrefMode(-1),
    mPrefIntent(-1),
    mPrefDisplayProfile(),
#ifdef DEBUG
    mPrefsInitialized(false),
#endif
    mModeSet(false),
    mMode(eCMSMode_Off),
    mIntent(-2),
    mOutputProfile(nullptr),
    msRGBProfile(nullptr),
    mRGBTransform(nullptr),
    mInverseRGBTransform(nullptr),
    mRGBATransform(nullptr),
    mDefaultPlatformProfile(nullptr)
{
  mPrefLock = new Mutex("gfxColorManagement::mPrefLock");
}

gfxColorManagement::~gfxColorManagement()
{
  
  NS_ASSERTION(mPrefsObserver, "mPrefsObserver is already gone");
  if (mPrefsObserver) {
    Preferences::RemoveObservers(mPrefsObserver, kObservedCMSPrefs);
    mPrefsObserver = nullptr;
  }

  ResetAll();
  delete mPrefLock;
}

NS_IMETHODIMP
gfxColorManagement::PreferencesModified(const PRUnichar* aName)
{
  if (NS_strcmp(aName, NS_LITERAL_STRING(kCMSPrefNameForcesRGB).get())) {
    mPrefForcesRGB = Preferences::GetBool(kCMSPrefNameForcesRGB, false);
    ResetAll();

  } else if (NS_strcmp(aName, NS_LITERAL_STRING(kCMSPrefNameMode).get())) {
    mPrefMode = Preferences::GetInt(kCMSPrefNameMode, -1);

  } else if (NS_strcmp(aName, NS_LITERAL_STRING(kCMSPrefNameEnablev4).get())) {
    mPrefEnableV4 = Preferences::GetBool(kCMSPrefNameEnablev4, false);

  } else if (NS_strcmp(aName, NS_LITERAL_STRING(kCMSPrefNameRenderingIntent).get())) {
    mPrefIntent = Preferences::GetInt(kCMSPrefNameRenderingIntent, -1);

  } else if (NS_strcmp(aName, NS_LITERAL_STRING(kCMSPrefNameDisplayProfile).get())) {
    MutexAutoLock autoLock(*mPrefLock);
    mPrefDisplayProfile = Preferences::GetCString(kCMSPrefNameDisplayProfile);
  }
  return NS_OK;
}

void gfxColorManagement::MigratePreferences()
{
  
  
  if (Preferences::HasUserValue(kCMSObsoletePrefEnabled)) {
    if (Preferences::GetBool(kCMSObsoletePrefEnabled, false)) {
      Preferences::SetInt(kCMSPrefNameMode, static_cast<int32_t>(eCMSMode_All));
    }
    Preferences::ClearUser(kCMSObsoletePrefEnabled);
  }
}

void gfxColorManagement::Initialize(qcms_profile* aDefaultPlatformProfile)
{
  mDefaultPlatformProfile = aDefaultPlatformProfile;

  
  MigratePreferences();

  
  mPrefMode = Preferences::GetInt(kCMSPrefNameMode, -1);
  mPrefEnableV4 = Preferences::GetBool(kCMSPrefNameEnablev4, false);
  mPrefIntent = Preferences::GetInt(kCMSPrefNameRenderingIntent, -1);
  mPrefForcesRGB = Preferences::GetBool(kCMSPrefNameForcesRGB, false);
  {
    
    MutexAutoLock autoLock(*mPrefLock);
    mPrefDisplayProfile = Preferences::GetCString(kCMSPrefNameDisplayProfile);
  }

  
  mPrefsObserver = new CMSPrefsObserver;
  Preferences::AddWeakObservers(mPrefsObserver, kObservedCMSPrefs);

#ifdef DEBUG
  mPrefsInitialized = true;
#endif
}

void gfxColorManagement::ResetAll()
{
  if (mRGBTransform) {
    qcms_transform_release(mRGBTransform);
    mRGBTransform = nullptr;
  }
  if (mInverseRGBTransform) {
    qcms_transform_release(mInverseRGBTransform);
    mInverseRGBTransform = nullptr;
  }
  if (mRGBATransform) {
    qcms_transform_release(mRGBATransform);
    mRGBATransform = nullptr;
  }
  if (mOutputProfile) {
    qcms_profile_release(mOutputProfile);

    
    if (msRGBProfile == mOutputProfile) {
      msRGBProfile = nullptr;
    }
    mOutputProfile = nullptr;
  }
  if (msRGBProfile) {
    qcms_profile_release(msRGBProfile);
    msRGBProfile = nullptr;
  }

  
  mIntent = -2;
  mMode = eCMSMode_Off;
  mModeSet = false;
}

eCMSMode
gfxColorManagement::GetMode() const
{
  NS_ASSERTION(mPrefsInitialized, "Prefs have not been initialized");
  if (mModeSet == false) {
    mModeSet = true;

    if ((mPrefMode >= 0) && (mPrefMode < eCMSMode_AllCount)) {
      mMode = static_cast<eCMSMode>(mPrefMode);
    }

    if (mPrefEnableV4) {
      qcms_enable_iccv4();
    }
  }
  return mMode;
}

int
gfxColorManagement::GetRenderingIntent() const
{
  NS_ASSERTION(mPrefsInitialized, "Prefs have not been initialized");
  if (mIntent == -2) {
    if (mPrefIntent >= 0) {
      
      if ((mPrefIntent >= QCMS_INTENT_MIN) && (mPrefIntent <= QCMS_INTENT_MAX)) {
        mIntent = mPrefIntent;
      }
      
      else {
        mIntent = -1;
      }
    }
    
    else {
      mIntent = QCMS_INTENT_DEFAULT;
    }
  }
  return mIntent;
}

qcms_profile*
gfxColorManagement::GetOutputProfile() const
{
  NS_ASSERTION(mPrefsInitialized, "Prefs have not been initialized");
  if (!mOutputProfile) {
    






    if (mPrefForcesRGB) {
      mOutputProfile = GetsRGBProfile();
    }

    if (!mOutputProfile) {
      MutexAutoLock autoLock(*mPrefLock);
      if (!mPrefDisplayProfile.IsEmpty()) {
        mOutputProfile = qcms_profile_from_path(mPrefDisplayProfile);
      }
    }

    if (!mOutputProfile) {
      mOutputProfile = mDefaultPlatformProfile;
    }

    

    if (mOutputProfile && qcms_profile_is_bogus(mOutputProfile)) {
      NS_ASSERTION(mOutputProfile != GetsRGBProfile(),
                   "Builtin sRGB profile tagged as bogus!!!");
      qcms_profile_release(mOutputProfile);
      mOutputProfile = nullptr;
    }

    if (!mOutputProfile) {
      mOutputProfile = GetsRGBProfile();
    }
    

    qcms_profile_precache_output_transform(mOutputProfile);
  }
  return mOutputProfile;
}

qcms_profile *
gfxColorManagement::GetsRGBProfile() const
{
  if (!msRGBProfile) {
    
    msRGBProfile = qcms_profile_sRGB();
  }
  return msRGBProfile;
}

qcms_transform *
gfxColorManagement::GetRGBTransform() const
{
  if (!mRGBTransform) {
    qcms_profile *inProfile, *outProfile;
    outProfile = GetOutputProfile();
    inProfile = GetsRGBProfile();

    if (!inProfile || !outProfile) {
      return nullptr;
    }

    mRGBTransform = qcms_transform_create(inProfile, QCMS_DATA_RGB_8,
                                          outProfile, QCMS_DATA_RGB_8,
                                          QCMS_INTENT_PERCEPTUAL);
  }
  return mRGBTransform;
}

qcms_transform *
gfxColorManagement::GetInverseRGBTransform() const
{
  NS_ASSERTION(mPrefsInitialized, "Prefs have not been initialized");
  if (!mInverseRGBTransform) {
    qcms_profile *inProfile, *outProfile;
    inProfile = GetOutputProfile();
    outProfile = GetsRGBProfile();

    if (!inProfile || !outProfile) {
      return nullptr;
    }

    mInverseRGBTransform = qcms_transform_create(inProfile, QCMS_DATA_RGB_8,
                                                 outProfile, QCMS_DATA_RGB_8,
                                                 QCMS_INTENT_PERCEPTUAL);
  }

  return mInverseRGBTransform;
}

qcms_transform *
gfxColorManagement::GetRGBATransform() const
{
  if (!mRGBATransform) {
    qcms_profile *inProfile, *outProfile;
    outProfile = GetOutputProfile();
    inProfile = GetsRGBProfile();

    if (!inProfile || !outProfile) {
      return nullptr;
    }

    mRGBATransform = qcms_transform_create(inProfile, QCMS_DATA_RGBA_8,
                                           outProfile, QCMS_DATA_RGBA_8,
                                           QCMS_INTENT_PERCEPTUAL);
  }
  return mRGBATransform;
}

void
gfxColorManagement::TransformPixel(const gfxRGBA& aIn, gfxRGBA& aOut,
                                   qcms_transform* aTransform) const
{
    if (aTransform) {
        
#ifdef IS_LITTLE_ENDIAN
        
        uint32_t packed = aIn.Packed(gfxRGBA::PACKED_ABGR);
        qcms_transform_data(aTransform,
                       (uint8_t *)&packed, (uint8_t *)&packed,
                       1);
        aOut.~gfxRGBA();
        new (&aOut) gfxRGBA(packed, gfxRGBA::PACKED_ABGR);
#else
        
        uint32_t packed = aIn.Packed(gfxRGBA::PACKED_ARGB);
        
        qcms_transform_data(aTransform,
                       (uint8_t *)&packed + 1, (uint8_t *)&packed + 1,
                       1);
        aOut.~gfxRGBA();
        new (&aOut) gfxRGBA(packed, gfxRGBA::PACKED_ARGB);
#endif
    }

    else if (&aOut != &aIn)
        aOut = aIn;
}

gfxColorManagement::gfxColorManagement(const gfxColorManagement&)
{
  NS_ASSERTION(false, "Should not be calling gfxColorManagement copy constructor");
}

gfxColorManagement& gfxColorManagement::operator=(const gfxColorManagement&)
{
  NS_ASSERTION(false, "Should not be calling gfxColorManagement::operator=");
  return *this;
}
