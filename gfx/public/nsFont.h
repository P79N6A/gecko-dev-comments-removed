




































#ifndef nsFont_h___
#define nsFont_h___

#include "gfxCore.h"
#include "nsCoord.h"
#include "nsStringGlue.h"





typedef PRBool (*nsFontFamilyEnumFunc)(const nsString& aFamily, PRBool aGeneric, void *aData);




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

  
  unsigned int style : 7;

  
  
  unsigned int systemFont : 1;

  
  PRUint8 variant : 7;

  
  
  PRPackedBool familyNameQuirks : 1;

  
  PRUint16 weight;

  
  
  PRUint8 decorations;

  
  nscoord size;

  
  
  
  
  float sizeAdjust;

  
  nsFont(const char* aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize,
         float aSizeAdjust=0.0f);

  
  nsFont(const nsString& aName, PRUint8 aStyle, PRUint8 aVariant,
         PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize,
         float aSizeAdjust=0.0f);

  
  nsFont(const nsFont& aFont);

  nsFont();
  ~nsFont();

  PRBool operator==(const nsFont& aOther) const {
    return Equals(aOther);
  }

  PRBool Equals(const nsFont& aOther) const ;
  
  PRBool BaseEquals(const nsFont& aOther) const;

  nsFont& operator=(const nsFont& aOther);

  
  
  
  
  PRBool EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const;
  void GetFirstFamily(nsString& aFamily) const;

  
  static void GetGenericID(const nsString& aGeneric, PRUint8* aID);
};

#define NS_FONT_STYLE_NORMAL              0
#define NS_FONT_STYLE_ITALIC              1
#define NS_FONT_STYLE_OBLIQUE             2

#define NS_FONT_VARIANT_NORMAL            0
#define NS_FONT_VARIANT_SMALL_CAPS        1

#define NS_FONT_DECORATION_NONE           0x0
#define NS_FONT_DECORATION_UNDERLINE      0x1
#define NS_FONT_DECORATION_OVERLINE       0x2
#define NS_FONT_DECORATION_LINE_THROUGH   0x4

#define NS_FONT_WEIGHT_NORMAL             400
#define NS_FONT_WEIGHT_BOLD               700

#endif 
