



#include "nsFontFace.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsCSSRules.h"
#include "gfxUserFontSet.h"
#include "nsFontFaceLoader.h"
#include "zlib.h"

nsFontFace::nsFontFace(gfxFontEntry*      aFontEntry,
                       gfxFontGroup*      aFontGroup,
                       uint8_t            aMatchType)
  : mFontEntry(aFontEntry),
    mFontGroup(aFontGroup),
    mMatchType(aMatchType)
{
}

nsFontFace::~nsFontFace()
{
}




NS_IMPL_ISUPPORTS1(nsFontFace, nsIDOMFontFace)





NS_IMETHODIMP
nsFontFace::GetFromFontGroup(bool * aFromFontGroup)
{
  *aFromFontGroup =
    (mMatchType & gfxTextRange::kFontGroup) != 0;
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetFromLanguagePrefs(bool * aFromLanguagePrefs)
{
  *aFromLanguagePrefs =
    (mMatchType & gfxTextRange::kPrefsFallback) != 0;
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetFromSystemFallback(bool * aFromSystemFallback)
{
  *aFromSystemFallback =
    (mMatchType & gfxTextRange::kSystemFallback) != 0;
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetName(nsAString & aName)
{
  if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    aName = mFontEntry->mUserFontData->mRealName;
  } else {
    aName = mFontEntry->RealFaceName();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetCSSFamilyName(nsAString & aCSSFamilyName)
{
  if (mFontEntry->IsUserFont()) {
    
    nsUserFontSet* fontSet =
      static_cast<nsUserFontSet*>(mFontGroup->GetUserFontSet());
    if (fontSet) {
      nsCSSFontFaceRule* rule = fontSet->FindRuleForEntry(mFontEntry);
      if (rule) {
        nsCOMPtr<nsIDOMCSSStyleDeclaration> style;
        nsresult rv = rule->GetStyle(getter_AddRefs(style));
        if (NS_SUCCEEDED(rv)) {
          nsString familyName;
          rv = style->GetPropertyValue(NS_LITERAL_STRING("font-family"),
                                       aCSSFamilyName);
          if (NS_SUCCEEDED(rv)) {
            
            
            
            if (aCSSFamilyName[0] == '"' &&
                aCSSFamilyName[aCSSFamilyName.Length() - 1] == '"') {
              aCSSFamilyName.Truncate(aCSSFamilyName.Length() - 1);
              aCSSFamilyName.Cut(0, 1);
            }
            return NS_OK;
          }
        }
      }
    }
  }

  
  uint32_t count = mFontGroup->FontListLength();
  for (uint32_t i = 0; i < count; ++i) {
    if (mFontGroup->GetFontAt(i)->GetFontEntry() == mFontEntry) {
      aCSSFamilyName = mFontGroup->GetFamilyNameAt(i);
      return NS_OK;
    }
  }

  
  aCSSFamilyName = mFontEntry->FamilyName();
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetRule(nsIDOMCSSFontFaceRule **aRule)
{
  
  
  nsCSSFontFaceRule* rule = nullptr;
  if (mFontEntry->IsUserFont()) {
    nsUserFontSet* fontSet =
      static_cast<nsUserFontSet*>(mFontGroup->GetUserFontSet());
    if (fontSet) {
      rule = fontSet->FindRuleForEntry(mFontEntry);
    }
  }

  NS_IF_ADDREF(*aRule = rule);
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetSrcIndex(int32_t * aSrcIndex)
{
  if (mFontEntry->IsUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    *aSrcIndex = mFontEntry->mUserFontData->mSrcIndex;
  } else {
    *aSrcIndex = -1;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetURI(nsAString & aURI)
{
  aURI.Truncate();
  if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    if (mFontEntry->mUserFontData->mURI) {
      nsAutoCString spec;
      mFontEntry->mUserFontData->mURI->GetSpec(spec);
      AppendUTF8toUTF16(spec, aURI);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetLocalName(nsAString & aLocalName)
{
  if (mFontEntry->IsLocalUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    aLocalName = mFontEntry->mUserFontData->mLocalName;
  } else {
    aLocalName.Truncate();
  }
  return NS_OK;
}


static void
AppendToFormat(nsAString & aResult, const char* aFormat)
{
  if (!aResult.IsEmpty()) {
    aResult.AppendASCII(",");
  }
  aResult.AppendASCII(aFormat);
}

NS_IMETHODIMP
nsFontFace::GetFormat(nsAString & aFormat)
{
  aFormat.Truncate();
  if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    uint32_t formatFlags = mFontEntry->mUserFontData->mFormat;
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_OPENTYPE) {
      AppendToFormat(aFormat, "opentype");
    }
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_TRUETYPE) {
      AppendToFormat(aFormat, "truetype");
    }
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_TRUETYPE_AAT) {
      AppendToFormat(aFormat, "truetype-aat");
    }
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_EOT) {
      AppendToFormat(aFormat, "embedded-opentype");
    }
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_SVG) {
      AppendToFormat(aFormat, "svg");
    }
    if (formatFlags & gfxUserFontSet::FLAG_FORMAT_WOFF) {
      AppendToFormat(aFormat, "woff");
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFontFace::GetMetadata(nsAString & aMetadata)
{
  aMetadata.Truncate();
  if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
    NS_ASSERTION(mFontEntry->mUserFontData, "missing userFontData");
    const gfxUserFontData* userFontData = mFontEntry->mUserFontData;
    if (userFontData->mMetadata.Length() && userFontData->mMetaOrigLen) {
      nsAutoCString str;
      str.SetLength(userFontData->mMetaOrigLen);
      if (str.Length() == userFontData->mMetaOrigLen) {
        uLongf destLen = userFontData->mMetaOrigLen;
        if (uncompress((Bytef *)(str.BeginWriting()), &destLen,
                       (const Bytef *)(userFontData->mMetadata.Elements()),
                       userFontData->mMetadata.Length()) == Z_OK &&
            destLen == userFontData->mMetaOrigLen)
        {
          AppendUTF8toUTF16(str, aMetadata);
        }
      }
    }
  }
  return NS_OK;
}
