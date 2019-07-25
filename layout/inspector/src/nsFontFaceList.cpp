



































#define _IMPL_NS_LAYOUT

#include "nsFontFaceList.h"
#include "nsFontFace.h"
#include "nsFontFaceLoader.h"
#include "nsIFrame.h"
#include "gfxFont.h"

nsFontFaceList::nsFontFaceList()
{
  mFontFaces.Init();
}

nsFontFaceList::~nsFontFaceList()
{
}




NS_IMPL_ISUPPORTS1(nsFontFaceList, nsIDOMFontFaceList)





struct FindByIndexData {
  PRUint32 mTarget;
  PRUint32 mCurrent;
  nsIDOMFontFace* mResult;
};

static PLDHashOperator
FindByIndex(gfxFontEntry* aKey, nsIDOMFontFace* aData, void* aUserData)
{
  FindByIndexData* data = static_cast<FindByIndexData*>(aUserData);
  if (data->mCurrent == data->mTarget) {
    data->mResult = aData;
    return PL_DHASH_STOP;
  }
  data->mCurrent++;
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsFontFaceList::Item(PRUint32 index, nsIDOMFontFace **_retval NS_OUTPARAM)
{
  NS_ENSURE_TRUE(index < mFontFaces.Count(), NS_ERROR_INVALID_ARG);
  FindByIndexData userData;
  userData.mTarget = index;
  userData.mCurrent = 0;
  userData.mResult = nsnull;
  mFontFaces.EnumerateRead(FindByIndex, &userData);
  NS_ASSERTION(userData.mResult != nsnull, "null entry in nsFontFaceList?");
  NS_IF_ADDREF(*_retval = userData.mResult);
  return NS_OK;
}


NS_IMETHODIMP
nsFontFaceList::GetLength(PRUint32 *aLength)
{
  *aLength = mFontFaces.Count();
  return NS_OK;
}




nsresult
nsFontFaceList::AddFontsFromTextRun(gfxTextRun* aTextRun,
                                    PRUint32 aOffset, PRUint32 aLength,
                                    nsIFrame* aFrame)
{
  gfxTextRun::GlyphRunIterator iter(aTextRun, aOffset, aLength);
  while (iter.NextRun()) {
    gfxFontEntry *fe = iter.GetGlyphRun()->mFont->GetFontEntry();
    if (!mFontFaces.GetWeak(fe)) {
      
      nsRefPtr<nsCSSFontFaceRule> rule;
      nsUserFontSet* fontSet =
        static_cast<nsUserFontSet*>(aFrame->PresContext()->GetUserFontSet());
      if (fontSet) {
        rule = fontSet->FindRuleForEntry(fe);
      }
      nsCOMPtr<nsFontFace> ff = new nsFontFace(fe, rule);
      if (!mFontFaces.Put(fe, ff)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  return NS_OK;
}
