



































#include "nsFontFace.h"

nsFontFace::nsFontFace(gfxFontEntry* aFontEntry)
  : mFontEntry(aFontEntry)
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
  return NS_ERROR_NOT_IMPLEMENTED;
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
