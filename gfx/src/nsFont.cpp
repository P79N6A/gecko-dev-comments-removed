




































#include "nsFont.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"

nsFont::nsFont(const char* aName, PRUint8 aStyle, PRUint8 aVariant,
               PRUint16 aWeight, PRInt16 aStretch, PRUint8 aDecoration,
               nscoord aSize, float aSizeAdjust,
               const nsString* aFeatureSettings,
               const nsString* aLanguageOverride)
{
  NS_ASSERTION(aName && IsASCII(nsDependentCString(aName)),
               "Must only pass ASCII names here");
  name.AssignASCII(aName);
  style = aStyle;
  systemFont = false;
  variant = aVariant;
  weight = aWeight;
  stretch = aStretch;
  decorations = aDecoration;
  size = aSize;
  sizeAdjust = aSizeAdjust;
  if (aFeatureSettings) {
    featureSettings = *aFeatureSettings;
  }
  if (aLanguageOverride) {
    languageOverride = *aLanguageOverride;
  }
}

nsFont::nsFont(const nsString& aName, PRUint8 aStyle, PRUint8 aVariant,
               PRUint16 aWeight, PRInt16 aStretch, PRUint8 aDecoration,
               nscoord aSize, float aSizeAdjust,
               const nsString* aFeatureSettings,
               const nsString* aLanguageOverride)
  : name(aName)
{
  style = aStyle;
  systemFont = false;
  variant = aVariant;
  weight = aWeight;
  stretch = aStretch;
  decorations = aDecoration;
  size = aSize;
  sizeAdjust = aSizeAdjust;
  if (aFeatureSettings) {
    featureSettings = *aFeatureSettings;
  }
  if (aLanguageOverride) {
    languageOverride = *aLanguageOverride;
  }
}

nsFont::nsFont(const nsFont& aOther)
  : name(aOther.name)
{
  style = aOther.style;
  systemFont = aOther.systemFont;
  variant = aOther.variant;
  weight = aOther.weight;
  stretch = aOther.stretch;
  decorations = aOther.decorations;
  size = aOther.size;
  sizeAdjust = aOther.sizeAdjust;
  featureSettings = aOther.featureSettings;
  languageOverride = aOther.languageOverride;
}

nsFont::nsFont()
{
}

nsFont::~nsFont()
{
}

bool nsFont::BaseEquals(const nsFont& aOther) const
{
  if ((style == aOther.style) &&
      (systemFont == aOther.systemFont) &&
      (weight == aOther.weight) &&
      (stretch == aOther.stretch) &&
      (size == aOther.size) &&
      (sizeAdjust == aOther.sizeAdjust) &&
      name.Equals(aOther.name, nsCaseInsensitiveStringComparator()) &&
      (featureSettings == aOther.featureSettings) &&
      (languageOverride == aOther.languageOverride)) {
    return true;
  }
  return false;
}

bool nsFont::Equals(const nsFont& aOther) const
{
  if (BaseEquals(aOther) &&
      (variant == aOther.variant) &&
      (decorations == aOther.decorations)) {
    return true;
  }
  return false;
}

nsFont& nsFont::operator=(const nsFont& aOther)
{
  name = aOther.name;
  style = aOther.style;
  systemFont = aOther.systemFont;
  variant = aOther.variant;
  weight = aOther.weight;
  stretch = aOther.stretch;
  decorations = aOther.decorations;
  size = aOther.size;
  sizeAdjust = aOther.sizeAdjust;
  featureSettings = aOther.featureSettings;
  languageOverride = aOther.languageOverride;
  return *this;
}

static bool IsGenericFontFamily(const nsString& aFamily)
{
  PRUint8 generic;
  nsFont::GetGenericID(aFamily, &generic);
  return generic != kGenericFont_NONE;
}

const PRUnichar kNullCh       = PRUnichar('\0');
const PRUnichar kSingleQuote  = PRUnichar('\'');
const PRUnichar kDoubleQuote  = PRUnichar('\"');
const PRUnichar kComma        = PRUnichar(',');

bool nsFont::EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const
{
  const PRUnichar *p, *p_end;
  name.BeginReading(p);
  name.EndReading(p_end);
  nsAutoString family;

  while (p < p_end) {
    while (nsCRT::IsAsciiSpace(*p))
      if (++p == p_end)
        return true;

    bool generic;
    if (*p == kSingleQuote || *p == kDoubleQuote) {
      
      PRUnichar quoteMark = *p;
      if (++p == p_end)
        return true;
      const PRUnichar *nameStart = p;

      
      while (*p != quoteMark)
        if (++p == p_end)
          return true;

      family = Substring(nameStart, p);
      generic = false;

      while (++p != p_end && *p != kComma)
         ;

    } else {
      
      const PRUnichar *nameStart = p;
      while (++p != p_end && *p != kComma)
         ;

      family = Substring(nameStart, p);
      family.CompressWhitespace(false, true);
      generic = IsGenericFontFamily(family);
    }

    if (!family.IsEmpty() && !(*aFunc)(family, generic, aData))
      return false;

    ++p; 
  }

  return true;
}

static bool FontEnumCallback(const nsString& aFamily, bool aGeneric, void *aData)
{
  *((nsString*)aData) = aFamily;
  return false;
}

void nsFont::GetFirstFamily(nsString& aFamily) const
{
  EnumerateFamilies(FontEnumCallback, &aFamily);
}


void nsFont::GetGenericID(const nsString& aGeneric, PRUint8* aID)
{
  *aID = kGenericFont_NONE;
  if (aGeneric.LowerCaseEqualsLiteral("-moz-fixed"))      *aID = kGenericFont_moz_fixed;
  else if (aGeneric.LowerCaseEqualsLiteral("serif"))      *aID = kGenericFont_serif;
  else if (aGeneric.LowerCaseEqualsLiteral("sans-serif")) *aID = kGenericFont_sans_serif;
  else if (aGeneric.LowerCaseEqualsLiteral("cursive"))    *aID = kGenericFont_cursive;
  else if (aGeneric.LowerCaseEqualsLiteral("fantasy"))    *aID = kGenericFont_fantasy;
  else if (aGeneric.LowerCaseEqualsLiteral("monospace"))  *aID = kGenericFont_monospace;
}
