




































#ifndef nsFont_h___
#define nsFont_h___

#include "gfxCore.h"
#include "nsCoord.h"
#include "nsStringGlue.h"
#include "gfxFontConstants.h"





typedef bool (*nsFontFamilyEnumFunc)(const nsString& aFamily, bool aGeneric, void *aData);




const PRUint8 kGenericFont_NONE         = 0x00;

const PRUint8 kGenericFont_moz_variable = 0x00; 
const PRUint8 kGenericFont_moz_fixed    = 0x01; 

const PRUint8 kGenericFont_serif        = 0x02;
const PRUint8 kGenericFont_sans_serif   = 0x04;
const PRUint8 kGenericFont_monospace    = 0x08;
const PRUint8 kGenericFont_cursive      = 0x10;
const PRUint8 kGenericFont_fantasy      = 0x20;


struct NS_GFX nsFont {
  
  nsString name;

  
  PRUint8 style;

  
  
  PRUint8 systemFont;

  
  PRUint8 variant;

  
  PRUint16 weight;

  
  
  PRInt16 stretch;

  
  
  PRUint8 decorations;

  
  nscoord size;

  
  
  
  
  float sizeAdjust;

  
  nsString featureSettings;

  
  
  
  nsString languageOverride;

  
  nsFont(const char* aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRInt16 aStretch, PRUint8 aDecoration,
         nscoord aSize, float aSizeAdjust=0.0f,
         const nsString* aFeatureSettings = nsnull,
         const nsString* aLanguageOverride = nsnull);

  
  nsFont(const nsString& aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRInt16 aStretch, PRUint8 aDecoration,
         nscoord aSize, float aSizeAdjust=0.0f,
         const nsString* aFeatureSettings = nsnull,
         const nsString* aLanguageOverride = nsnull);

  
  nsFont(const nsFont& aFont);

  nsFont();
  ~nsFont();

  bool operator==(const nsFont& aOther) const {
    return Equals(aOther);
  }

  bool Equals(const nsFont& aOther) const ;
  
  bool BaseEquals(const nsFont& aOther) const;

  nsFont& operator=(const nsFont& aOther);

  
  
  
  
  bool EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const;
  void GetFirstFamily(nsString& aFamily) const;

  
  static void GetGenericID(const nsString& aGeneric, PRUint8* aID);
};

#define NS_FONT_VARIANT_NORMAL            0
#define NS_FONT_VARIANT_SMALL_CAPS        1

#define NS_FONT_DECORATION_NONE           0x0
#define NS_FONT_DECORATION_UNDERLINE      0x1
#define NS_FONT_DECORATION_OVERLINE       0x2
#define NS_FONT_DECORATION_LINE_THROUGH   0x4

#endif 
