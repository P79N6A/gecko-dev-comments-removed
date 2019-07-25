







































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

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer) = 0;

    nsresult ReadCMAP();

    bool RequiresAATLayout() const { return mRequiresAAT; }

    bool IsCFF();

protected:
    MacOSFontEntry(const nsAString& aPostscriptName, PRInt32 aWeight,
                   gfxFontFamily *aFamily, bool aIsStandardFace = false);

    virtual gfxFont* CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold);

    virtual bool HasFontTable(PRUint32 aTableTag) = 0;

    CGFontRef mFontRef; 

    bool mFontRefInitialized;
    bool mRequiresAAT;
    bool mIsCFF;
    bool mIsCFFInitialized;
};


class ATSFontEntry : public MacOSFontEntry
{
public:
    ATSFontEntry(const nsAString& aPostscriptName, PRInt32 aWeight,
                 gfxFontFamily *aFamily, bool aIsStandardFace = false);

    
    ATSFontEntry(const nsAString& aPostscriptName, ATSFontRef aFontRef,
                 PRUint16 aWeight, PRUint16 aStretch, PRUint32 aItalicStyle,
                 gfxUserFontData *aUserFontData, bool aIsLocal);

    ATSFontRef GetATSFontRef();

    virtual CGFontRef GetFontRef();

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer);

protected:
    virtual bool HasFontTable(PRUint32 aTableTag);

    ATSFontRef   mATSFontRef;
    bool mATSFontRefInitialized;
};

class CGFontEntry : public MacOSFontEntry
{
public:
    CGFontEntry(const nsAString& aPostscriptName, PRInt32 aWeight,
                gfxFontFamily *aFamily, bool aIsStandardFace = false);

    
    CGFontEntry(const nsAString& aPostscriptName, CGFontRef aFontRef,
                PRUint16 aWeight, PRUint16 aStretch, PRUint32 aItalicStyle,
                bool aIsUserFont, bool aIsLocal);

    virtual CGFontRef GetFontRef();

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer);

protected:
    virtual bool HasFontTable(PRUint32 aTableTag);
};

class gfxMacPlatformFontList : public gfxPlatformFontList {
public:
    static gfxMacPlatformFontList* PlatformFontList() {
        return static_cast<gfxMacPlatformFontList*>(sPlatformFontList);
    }

    static PRInt32 AppleWeightToCSSWeight(PRInt32 aAppleWeight);

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, bool& aNeedsBold);

    virtual bool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);
    
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData, PRUint32 aLength);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    static bool UseATSFontEntry() {
        return gfxPlatformMac::GetPlatform()->OSXVersion() < MAC_OS_X_VERSION_10_6_HEX;
    }

private:
    friend class gfxPlatformMac;

    gfxMacPlatformFontList();

    
    virtual nsresult InitFontList();

    
    void InitSingleFaceList();

    gfxFontEntry* MakePlatformFontCG(const gfxProxyFontEntry *aProxyEntry,
                                     const PRUint8 *aFontData, PRUint32 aLength);

    gfxFontEntry* MakePlatformFontATS(const gfxProxyFontEntry *aProxyEntry,
                                      const PRUint8 *aFontData, PRUint32 aLength);

    static void ATSNotification(ATSFontNotificationInfoRef aInfo, void* aUserArg);

    
    PRUint32 mATSGeneration;

    enum {
        kATSGenerationInitial = -1
    };
};

#endif 
