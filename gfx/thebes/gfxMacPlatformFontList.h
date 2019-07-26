




#ifndef gfxMacPlatformFontList_H_
#define gfxMacPlatformFontList_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"

#include "gfxPlatformFontList.h"
#include "gfxPlatform.h"
#include "gfxPlatformMac.h"

#include <Carbon/Carbon.h>

#include "nsUnicharUtils.h"
#include "nsTArray.h"

class gfxMacPlatformFontList;


class MacOSFontEntry : public gfxFontEntry
{
public:
    friend class gfxMacPlatformFontList;

    virtual ~MacOSFontEntry() {
        ::CGFontRelease(mFontRef);
    }

    virtual CGFontRef GetFontRef() = 0;

    virtual nsresult GetFontTable(uint32_t aTableTag,
                                  FallibleTArray<uint8_t>& aBuffer) = 0;

    nsresult ReadCMAP();

    bool RequiresAATLayout() const { return mRequiresAAT; }

    bool IsCFF();

protected:
    MacOSFontEntry(const nsAString& aPostscriptName, int32_t aWeight,
                   gfxFontFamily *aFamily, bool aIsStandardFace = false);

    virtual gfxFont* CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold);

    virtual bool HasFontTable(uint32_t aTableTag) = 0;

    CGFontRef mFontRef; 

    bool mFontRefInitialized;
    bool mRequiresAAT;
    bool mIsCFF;
    bool mIsCFFInitialized;
};


class ATSFontEntry : public MacOSFontEntry
{
public:
    ATSFontEntry(const nsAString& aPostscriptName, int32_t aWeight,
                 gfxFontFamily *aFamily, bool aIsStandardFace = false);

    
    ATSFontEntry(const nsAString& aPostscriptName, ATSFontRef aFontRef,
                 uint16_t aWeight, uint16_t aStretch, uint32_t aItalicStyle,
                 gfxUserFontData *aUserFontData, bool aIsLocal);

    ATSFontRef GetATSFontRef();

    virtual CGFontRef GetFontRef();

    virtual nsresult GetFontTable(uint32_t aTableTag,
                                  FallibleTArray<uint8_t>& aBuffer);

    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

protected:
    virtual bool HasFontTable(uint32_t aTableTag);

    ATSFontRef   mATSFontRef;
    bool mATSFontRefInitialized;
};

class CGFontEntry : public MacOSFontEntry
{
public:
    CGFontEntry(const nsAString& aPostscriptName, int32_t aWeight,
                gfxFontFamily *aFamily, bool aIsStandardFace = false);

    
    CGFontEntry(const nsAString& aPostscriptName, CGFontRef aFontRef,
                uint16_t aWeight, uint16_t aStretch, uint32_t aItalicStyle,
                bool aIsUserFont, bool aIsLocal);

    virtual CGFontRef GetFontRef();

    virtual nsresult GetFontTable(uint32_t aTableTag,
                                  FallibleTArray<uint8_t>& aBuffer);

    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

protected:
    virtual bool HasFontTable(uint32_t aTableTag);
};

class gfxMacPlatformFontList : public gfxPlatformFontList {
public:
    static gfxMacPlatformFontList* PlatformFontList() {
        return static_cast<gfxMacPlatformFontList*>(sPlatformFontList);
    }

    static int32_t AppleWeightToCSSWeight(int32_t aAppleWeight);

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, bool& aNeedsBold);

    virtual bool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);
    
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData, uint32_t aLength);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    static bool UseATSFontEntry() {
        return gfxPlatformMac::GetPlatform()->OSXVersion() < MAC_OS_X_VERSION_10_6_HEX;
    }

private:
    friend class gfxPlatformMac;

    gfxMacPlatformFontList();
    virtual ~gfxMacPlatformFontList();

    
    virtual nsresult InitFontList();

    
    void InitSingleFaceList();

    gfxFontEntry* MakePlatformFontCG(const gfxProxyFontEntry *aProxyEntry,
                                     const uint8_t *aFontData, uint32_t aLength);

    gfxFontEntry* MakePlatformFontATS(const gfxProxyFontEntry *aProxyEntry,
                                      const uint8_t *aFontData, uint32_t aLength);

    static void ATSNotification(ATSFontNotificationInfoRef aInfo, void* aUserArg);

    
    virtual gfxFontEntry* GlobalFontFallback(const uint32_t aCh,
                                             int32_t aRunScript,
                                             const gfxFontStyle* aMatchStyle,
                                             uint32_t& aCmapCount);

    virtual bool UsesSystemFallback() { return true; }

    
    uint32_t mATSGeneration;

    enum {
        kATSGenerationInitial = -1
    };

    
    CTFontRef mDefaultFont;
};

#endif 
