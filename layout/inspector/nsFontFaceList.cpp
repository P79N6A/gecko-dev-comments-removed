



#include "nsFontFaceList.h"
#include "nsFontFace.h"
#include "nsFontFaceLoader.h"
#include "nsIFrame.h"
#include "gfxTextRun.h"
#include "mozilla/gfx/2D.h"

nsFontFaceList::nsFontFaceList()
{
}

nsFontFaceList::~nsFontFaceList()
{
}




NS_IMPL_ISUPPORTS(nsFontFaceList, nsIDOMFontFaceList)





struct FindByIndexData {
  uint32_t mTarget;
  uint32_t mCurrent;
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
nsFontFaceList::Item(uint32_t index, nsIDOMFontFace **_retval)
{
  NS_ENSURE_TRUE(index < mFontFaces.Count(), NS_ERROR_INVALID_ARG);
  FindByIndexData userData;
  userData.mTarget = index;
  userData.mCurrent = 0;
  userData.mResult = nullptr;
  mFontFaces.EnumerateRead(FindByIndex, &userData);
  NS_ASSERTION(userData.mResult != nullptr, "null entry in nsFontFaceList?");
  NS_IF_ADDREF(*_retval = userData.mResult);
  return NS_OK;
}


NS_IMETHODIMP
nsFontFaceList::GetLength(uint32_t *aLength)
{
  *aLength = mFontFaces.Count();
  return NS_OK;
}




nsresult
nsFontFaceList::AddFontsFromTextRun(gfxTextRun* aTextRun,
                                    uint32_t aOffset, uint32_t aLength)
{
  gfxTextRun::GlyphRunIterator iter(aTextRun, aOffset, aLength);
  while (iter.NextRun()) {
    gfxFontEntry *fe = iter.GetGlyphRun()->mFont->GetFontEntry();
    
    
    nsFontFace* existingFace =
      static_cast<nsFontFace*>(mFontFaces.GetWeak(fe));
    if (existingFace) {
      existingFace->AddMatchType(iter.GetGlyphRun()->mMatchType);
    } else {
      
      nsRefPtr<nsFontFace> ff =
        new nsFontFace(fe, aTextRun->GetFontGroup(),
                       iter.GetGlyphRun()->mMatchType);
      mFontFaces.Put(fe, ff);
    }
  }

  return NS_OK;
}
