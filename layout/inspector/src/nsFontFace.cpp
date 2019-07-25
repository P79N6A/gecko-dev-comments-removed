



































#define _IMPL_NS_LAYOUT

#include "nsFontFace.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsCSSRules.h"

nsFontFace::nsFontFace(gfxFontEntry* aFontEntry,
                       nsCSSFontFaceRule* aRule)
  : mFontEntry(aFontEntry),
    mRule(aRule)
{
}

nsFontFace::~nsFontFace()
{
}




NS_IMPL_ISUPPORTS1(nsFontFace, nsIDOMFontFace)





NS_IMETHODIMP
nsFontFace::GetFromFontGroup(PRBool * aFromFontGroup)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetFromLanguagePrefs(PRBool * aFromLanguagePrefs)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetFromSystemFallback(PRBool * aFromSystemFallback)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetName(nsAString & aName)
{
  aName = mFontEntry->Name();
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetCSSFamilyName(nsAString & aCSSFamilyName)
{
  aCSSFamilyName = mFontEntry->FamilyName();
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetRule(nsIDOMCSSFontFaceRule **aRule)
{
  NS_IF_ADDREF(*aRule = mRule.get());
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetSrcIndex(PRInt32 * aSrcIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetURI(nsAString & aURI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetLocalName(nsAString & aLocalName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetFormat(nsAString & aFormat)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsFontFace::GetMetadata(nsAString & aMetadata)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
