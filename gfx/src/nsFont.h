




#ifndef nsFont_h___
#define nsFont_h___

#include "gfxCore.h"
#include "nsCoord.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "gfxFontConstants.h"
#include "gfxFontFeatures.h"





typedef bool (*nsFontFamilyEnumFunc)(const nsString& aFamily, bool aGeneric, void *aData);




const uint8_t kGenericFont_NONE         = 0x00;

const uint8_t kGenericFont_moz_variable = 0x00; 
const uint8_t kGenericFont_moz_fixed    = 0x01; 

const uint8_t kGenericFont_serif        = 0x02;
const uint8_t kGenericFont_sans_serif   = 0x04;
const uint8_t kGenericFont_monospace    = 0x08;
const uint8_t kGenericFont_cursive      = 0x10;
const uint8_t kGenericFont_fantasy      = 0x20;

struct gfxFontStyle;


struct NS_GFX nsFont {
  
  nsString name;

  
  uint8_t style;

  
  
  uint8_t systemFont;

  
  uint8_t variant;

  
  
  uint8_t decorations;

  
  uint16_t weight;

  
  
  int16_t stretch;

  
  nscoord size;

  
  
  
  
  float sizeAdjust;

  
  nsTArray<gfxFontFeature> fontFeatureSettings;

  
  
  
  nsString languageOverride;

  
  nsFont(const char* aName, uint8_t aStyle, uint8_t aVariant,
         uint16_t aWeight, int16_t aStretch, uint8_t aDecoration,
         nscoord aSize, float aSizeAdjust=0.0f,
         const nsString* aLanguageOverride = nullptr);

  
  nsFont(const nsString& aName, uint8_t aStyle, uint8_t aVariant,
         uint16_t aWeight, int16_t aStretch, uint8_t aDecoration,
         nscoord aSize, float aSizeAdjust=0.0f,
         const nsString* aLanguageOverride = nullptr);

  
  nsFont(const nsFont& aFont);

  nsFont();
  ~nsFont();

  bool operator==(const nsFont& aOther) const {
    return Equals(aOther);
  }

  bool Equals(const nsFont& aOther) const ;
  
  bool BaseEquals(const nsFont& aOther) const;

  nsFont& operator=(const nsFont& aOther);

  
  void AddFontFeaturesToStyle(gfxFontStyle *aStyle) const;

  
  
  
  
  bool EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const;
  void GetFirstFamily(nsString& aFamily) const;

  
  static void GetGenericID(const nsString& aGeneric, uint8_t* aID);
};

#define NS_FONT_VARIANT_NORMAL            0
#define NS_FONT_VARIANT_SMALL_CAPS        1

#define NS_FONT_DECORATION_NONE           0x0
#define NS_FONT_DECORATION_UNDERLINE      0x1
#define NS_FONT_DECORATION_OVERLINE       0x2
#define NS_FONT_DECORATION_LINE_THROUGH   0x4

#endif 
