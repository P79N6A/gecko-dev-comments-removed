







































#ifndef gfxMacPlatformFontList_H_
#define gfxMacPlatformFontList_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"

#include "gfxPlatformFontList.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

#include "nsUnicharUtils.h"
#include "nsTArray.h"

class gfxMacPlatformFontList;


class MacOSFontEntry : public gfxFontEntry
{
public:
    friend class gfxMacPlatformFontList;

    MacOSFontEntry(const nsAString& aPostscriptName, PRInt32 aWeight,
                   gfxFontFamily *aFamily, PRBool aIsStandardFace = PR_FALSE);

    ATSFontRef GetFontRef();
    nsresult ReadCMAP();

    virtual nsresult GetFontTable(PRUint32 aTableTag, nsTArray<PRUint8>& aBuffer);

protected:
    
    MacOSFontEntry(const nsAString& aPostscriptName, ATSFontRef aFontRef,
                   PRUint16 aWeight, PRUint16 aStretch, PRUint32 aItalicStyle,
                   gfxUserFontData *aUserFontData);

    virtual gfxFont* CreateFontInstance(const gfxFontStyle *aFontStyle, PRBool aNeedsBold);

    ATSFontRef mATSFontRef;
    PRPackedBool mATSFontRefInitialized;
};

class gfxMacPlatformFontList : public gfxPlatformFontList {
public:
    static gfxMacPlatformFontList* PlatformFontList() {
        return static_cast<gfxMacPlatformFontList*>(sPlatformFontList);
    }

    static PRInt32 AppleWeightToCSSWeight(PRInt32 aAppleWeight);

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold);

    virtual PRBool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);
    
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData, PRUint32 aLength);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

private:
    friend class gfxPlatformMac;

    gfxMacPlatformFontList();

    
    virtual void InitFontList();

    
    void InitSingleFaceList();

    
    void EliminateDuplicateFaces(const nsAString& aFamilyName);

    static void ATSNotification(ATSFontNotificationInfoRef aInfo, void* aUserArg);

    
    PRUint32 mATSGeneration;

    enum {
        kATSGenerationInitial = -1
    };
};

#endif 
