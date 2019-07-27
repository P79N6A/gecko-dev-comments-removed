




#ifndef nsFont_h___
#define nsFont_h___

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxCore.h"                    
#include "gfxFontFamilyList.h"
#include "gfxFontFeatures.h"
#include "nsAutoPtr.h"                  
#include "nsCoord.h"                    
#include "nsStringFwd.h"                
#include "nsString.h"               
#include "nsTArray.h"                   

struct gfxFontStyle;





typedef bool (*nsFontFamilyEnumFunc)(const nsString& aFamily, bool aGeneric, void *aData);




const uint8_t kGenericFont_NONE         = 0x00;

const uint8_t kGenericFont_moz_variable = 0x00; 
const uint8_t kGenericFont_moz_fixed    = 0x01; 

const uint8_t kGenericFont_serif        = 0x02;
const uint8_t kGenericFont_sans_serif   = 0x04;
const uint8_t kGenericFont_monospace    = 0x08;
const uint8_t kGenericFont_cursive      = 0x10;
const uint8_t kGenericFont_fantasy      = 0x20;


struct NS_GFX nsFont {

  
  mozilla::FontFamilyList fontlist;

  
  uint8_t style;

  
  
  bool systemFont;

  
  uint8_t variantCaps;
  uint8_t variantNumeric;
  uint8_t variantPosition;

  uint16_t variantLigatures;
  uint16_t variantEastAsian;

  
  
  

  
  uint16_t variantAlternates;

  
  
  uint8_t decorations;

  
  uint8_t smoothing;

  
  uint16_t weight;

  
  
  int16_t stretch;

  
  uint8_t kerning;

  
  uint8_t synthesis;

  
  nscoord size;

  
  
  
  
  float sizeAdjust;

  
  nsTArray<gfxAlternateValue> alternateValues;

  
  nsRefPtr<gfxFontFeatureValueSet> featureValueLookup;

  
  nsTArray<gfxFontFeature> fontFeatureSettings;

  
  
  
  nsString languageOverride;

  
  nsFont(const mozilla::FontFamilyList& aFontlist, uint8_t aStyle,
         uint16_t aWeight, int16_t aStretch,
         uint8_t aDecoration, nscoord aSize);

  
  nsFont(mozilla::FontFamilyType aGenericType, uint8_t aStyle,
         uint16_t aWeight, int16_t aStretch, uint8_t aDecoration,
         nscoord aSize);

  
  nsFont(const nsFont& aFont);

  nsFont();
  ~nsFont();

  bool operator==(const nsFont& aOther) const {
    return Equals(aOther);
  }

  bool Equals(const nsFont& aOther) const ;
  
  bool BaseEquals(const nsFont& aOther) const;

  nsFont& operator=(const nsFont& aOther);

  void CopyAlternates(const nsFont& aOther);

  
  void AddFontFeaturesToStyle(gfxFontStyle *aStyle) const;

protected:
  void Init(); 
};

#define NS_FONT_VARIANT_NORMAL            0
#define NS_FONT_VARIANT_SMALL_CAPS        1

#define NS_FONT_DECORATION_NONE           0x0
#define NS_FONT_DECORATION_UNDERLINE      0x1
#define NS_FONT_DECORATION_OVERLINE       0x2
#define NS_FONT_DECORATION_LINE_THROUGH   0x4

#endif 
