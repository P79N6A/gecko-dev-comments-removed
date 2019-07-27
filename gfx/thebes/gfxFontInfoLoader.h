




#ifndef GFX_FONT_INFO_LOADER_H
#define GFX_FONT_INFO_LOADER_H

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIThread.h"
#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "gfxFont.h"
#include "nsIRunnable.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"



struct FontFaceData {
    FontFaceData() : mUVSOffset(0), mSymbolFont(false) {}

    FontFaceData(const FontFaceData& aFontFaceData) {
        mFullName = aFontFaceData.mFullName;
        mPostscriptName = aFontFaceData.mPostscriptName;
        mCharacterMap = aFontFaceData.mCharacterMap;
        mUVSOffset = aFontFaceData.mUVSOffset;
        mSymbolFont = aFontFaceData.mSymbolFont;
    }

    nsString mFullName;
    nsString mPostscriptName;
    nsRefPtr<gfxCharacterMap> mCharacterMap;
    uint32_t mUVSOffset;
    bool mSymbolFont;
};








class FontInfoData {
public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FontInfoData)

    FontInfoData(bool aLoadOtherNames,
                 bool aLoadFaceNames,
                 bool aLoadCmaps) :
        mLoadOtherNames(aLoadOtherNames),
        mLoadFaceNames(aLoadFaceNames),
        mLoadCmaps(aLoadCmaps)
    {
        MOZ_COUNT_CTOR(FontInfoData);
    }

protected:
    
    virtual ~FontInfoData() {
        MOZ_COUNT_DTOR(FontInfoData);
    }

public:
    virtual void Load();

    
    
    virtual void LoadFontFamilyData(const nsAString& aFamilyName) = 0;

    

    
    virtual already_AddRefed<gfxCharacterMap>
    GetCMAP(const nsAString& aFontName,
            uint32_t& aUVSOffset,
            bool& aSymbolFont)
    {
        FontFaceData faceData;
        if (!mFontFaceData.Get(aFontName, &faceData) ||
            !faceData.mCharacterMap) {
            return nullptr;
        }

        aUVSOffset = faceData.mUVSOffset;
        aSymbolFont = faceData.mSymbolFont;
        nsRefPtr<gfxCharacterMap> cmap = faceData.mCharacterMap;
        return cmap.forget();
    }

    
    virtual void GetFaceNames(const nsAString& aFontName,
                              nsAString& aFullName,
                              nsAString& aPostscriptName)
    {
        FontFaceData faceData;
        if (!mFontFaceData.Get(aFontName, &faceData)) {
            return;
        }

        aFullName = faceData.mFullName;
        aPostscriptName = faceData.mPostscriptName;
    }

    
    virtual bool GetOtherFamilyNames(const nsAString& aFamilyName,
                                     nsTArray<nsString>& aOtherFamilyNames)
    {
        return mOtherFamilyNames.Get(aFamilyName, &aOtherFamilyNames); 
    }

    nsTArray<nsString> mFontFamiliesToLoad;

    
    mozilla::TimeDuration mLoadTime;

    struct FontCounts {
        uint32_t families;
        uint32_t fonts;
        uint32_t cmaps;
        uint32_t facenames;
        uint32_t othernames;
    };

    FontCounts mLoadStats;

    bool mLoadOtherNames;
    bool mLoadFaceNames;
    bool mLoadCmaps;

    
    nsDataHashtable<nsStringHashKey, FontFaceData> mFontFaceData;

    
    nsDataHashtable<nsStringHashKey, nsTArray<nsString> > mOtherFamilyNames;
};









class gfxFontInfoLoader {
public:

    
    
    
    
    
    
    
    
    typedef enum {
        stateInitial,
        stateTimerOnDelay,
        stateAsyncLoad,
        stateTimerOnInterval,
        stateTimerOff
    } TimerState;

    gfxFontInfoLoader() :
        mInterval(0), mState(stateInitial)
    {
        MOZ_COUNT_CTOR(gfxFontInfoLoader);
    }

    virtual ~gfxFontInfoLoader();

    
    void StartLoader(uint32_t aDelay, uint32_t aInterval);

    
    virtual void FinalizeLoader(FontInfoData *aFontInfo);

    
    void CancelLoader();

    uint32_t GetInterval() { return mInterval; }

protected:
    class ShutdownObserver : public nsIObserver
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER

        explicit ShutdownObserver(gfxFontInfoLoader *aLoader)
            : mLoader(aLoader)
        { }

    protected:
        virtual ~ShutdownObserver()
        { }

        gfxFontInfoLoader *mLoader;
    };

    
    
    virtual already_AddRefed<FontInfoData> CreateFontInfoData() {
        return nullptr;
    }

    
    virtual void InitLoader() = 0;

    
    
    virtual bool LoadFontInfo() = 0;

    
    virtual void CleanupLoader() {
        mFontInfo = nullptr;
    }

    
    static void LoadFontInfoCallback(nsITimer *aTimer, void *aThis) {
        gfxFontInfoLoader *loader = static_cast<gfxFontInfoLoader*>(aThis);
        loader->LoadFontInfoTimerFire();
    }

    static void DelayedStartCallback(nsITimer *aTimer, void *aThis) {
        gfxFontInfoLoader *loader = static_cast<gfxFontInfoLoader*>(aThis);
        loader->StartLoader(0, loader->GetInterval());
    }

    void LoadFontInfoTimerFire();

    void AddShutdownObserver();
    void RemoveShutdownObserver();

    nsCOMPtr<nsITimer> mTimer;
    nsCOMPtr<nsIObserver> mObserver;
    nsCOMPtr<nsIThread> mFontLoaderThread;
    uint32_t mInterval;
    TimerState mState;

    
    nsRefPtr<FontInfoData> mFontInfo;

    
    mozilla::TimeDuration mLoadTime;
};

#endif 
