




































#include "nsFont.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"

nsFont::nsFont(const char* aName, PRUint8 aStyle, PRUint8 aVariant,
               PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize,
               float aSizeAdjust)
{
  NS_ASSERTION(aName && IsASCII(nsDependentCString(aName)),
               "Must only pass ASCII names here");
  name.AssignASCII(aName);
  style = aStyle;
  systemFont = PR_FALSE;
  variant = aVariant;
  familyNameQuirks = PR_FALSE;
  weight = aWeight;
  decorations = aDecoration;
  size = aSize;
  sizeAdjust = aSizeAdjust;
}

nsFont::nsFont(const nsString& aName, PRUint8 aStyle, PRUint8 aVariant,
               PRUint16 aWeight, PRUint8 aDecoration, nscoord aSize,
               float aSizeAdjust)
  : name(aName)
{
  style = aStyle;
  systemFont = PR_FALSE;
  variant = aVariant;
  familyNameQuirks = PR_FALSE;
  weight = aWeight;
  decorations = aDecoration;
  size = aSize;
  sizeAdjust = aSizeAdjust;
}

nsFont::nsFont(const nsFont& aOther)
  : name(aOther.name)
{
  style = aOther.style;
  systemFont = aOther.systemFont;
  variant = aOther.variant;
  familyNameQuirks = aOther.familyNameQuirks;
  weight = aOther.weight;
  decorations = aOther.decorations;
  size = aOther.size;
  sizeAdjust = aOther.sizeAdjust;
}

nsFont::nsFont()
{
}

nsFont::~nsFont()
{
}

PRBool nsFont::BaseEquals(const nsFont& aOther) const
{
  if ((style == aOther.style) &&
      (systemFont == aOther.systemFont) &&
      (familyNameQuirks == aOther.familyNameQuirks) &&
      (weight == aOther.weight) &&
      (size == aOther.size) &&
      (sizeAdjust == aOther.sizeAdjust) &&
      name.Equals(aOther.name, nsCaseInsensitiveStringComparator())) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool nsFont::Equals(const nsFont& aOther) const
{
  if (BaseEquals(aOther) &&
      (variant == aOther.variant) &&
      (decorations == aOther.decorations)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsFont& nsFont::operator=(const nsFont& aOther)
{
  name = aOther.name;
  style = aOther.style;
  systemFont = aOther.systemFont;
  variant = aOther.variant;
  familyNameQuirks = aOther.familyNameQuirks;
  weight = aOther.weight;
  decorations = aOther.decorations;
  size = aOther.size;
  sizeAdjust = aOther.sizeAdjust;
  return *this;
}

static PRBool IsGenericFontFamily(const nsString& aFamily)
{
  PRUint8 generic;
  nsFont::GetGenericID(aFamily, &generic);
  return generic != kGenericFont_NONE;
}

const PRUnichar kNullCh       = PRUnichar('\0');
const PRUnichar kSingleQuote  = PRUnichar('\'');
const PRUnichar kDoubleQuote  = PRUnichar('\"');
const PRUnichar kComma        = PRUnichar(',');

PRBool nsFont::EnumerateFamilies(nsFontFamilyEnumFunc aFunc, void* aData) const
{
  const PRUnichar *p, *p_end;
  name.BeginReading(p);
  name.EndReading(p_end);
  nsAutoString family;

  while (p < p_end) {
    while (nsCRT::IsAsciiSpace(*p))
      if (++p == p_end)
        return PR_TRUE;

    PRBool generic;
    if (*p == kSingleQuote || *p == kDoubleQuote) {
      
      PRUnichar quoteMark = *p;
      if (++p == p_end)
        return PR_TRUE;
      const PRUnichar *nameStart = p;

      
      while (*p != quoteMark)
        if (++p == p_end)
          return PR_TRUE;

      family = Substring(nameStart, p);
      generic = PR_FALSE;

      while (++p != p_end && *p != kComma)
         ;

    } else {
      
      const PRUnichar *nameStart = p;
      while (++p != p_end && *p != kComma)
         ;

      family = Substring(nameStart, p);
      family.CompressWhitespace(PR_FALSE, PR_TRUE);
      generic = IsGenericFontFamily(family);
    }

    if (!family.IsEmpty() && !(*aFunc)(family, generic, aData))
      return PR_FALSE;

    ++p; 
  }

  return PR_TRUE;
}

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  *((nsString*)aData) = aFamily;
  return PR_FALSE;
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
