





































#include "IEnumFE.h"
#include "nsAlgorithm.h"

CEnumFormatEtc::CEnumFormatEtc() :
  mRefCnt(0),
  mCurrentIdx(0)
{
}


CEnumFormatEtc::CEnumFormatEtc(nsTArray<FormatEtc>& aArray) :
  mRefCnt(0),
  mCurrentIdx(0)
{
  
  mFormatList.AppendElements(aArray);
}

CEnumFormatEtc::~CEnumFormatEtc()
{
}



STDMETHODIMP
CEnumFormatEtc::QueryInterface(REFIID riid, LPVOID *ppv)
{
  *ppv = NULL;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IEnumFORMATETC))
      *ppv = (LPVOID)this;

  if (*ppv == NULL)
      return E_NOINTERFACE;

  
  ((LPUNKNOWN)*ppv)->AddRef();
  return S_OK;
}

STDMETHODIMP_(ULONG)
CEnumFormatEtc::AddRef()
{
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "CEnumFormatEtc",sizeof(*this));
  return mRefCnt;
}

STDMETHODIMP_(ULONG)
CEnumFormatEtc::Release()
{
  PRUint32 refReturn;

  refReturn = --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "CEnumFormatEtc");

  if (mRefCnt == 0)
      delete this;

  return refReturn;
}



STDMETHODIMP
CEnumFormatEtc::Next(ULONG aMaxToFetch, FORMATETC *aResult, ULONG *aNumFetched)
{
  
  

  if (aNumFetched)
      *aNumFetched = 0;

  
  if (!aNumFetched && aMaxToFetch > 1)
      return S_FALSE;

  if (!aResult)
      return S_FALSE;

  
  if (mCurrentIdx >= mFormatList.Length())
      return S_FALSE;

  PRInt32 left = mFormatList.Length() - mCurrentIdx;

  if (!left || !aMaxToFetch)
      return S_FALSE;

  PRInt32 count = NS_MIN(static_cast<PRInt32>(aMaxToFetch), left);

  PRUint32 idx = 0;
  while (count > 0) {
      
      mFormatList[mCurrentIdx++].CopyOut(&aResult[idx++]);
      count--;
  }

  if (aNumFetched)
      *aNumFetched = idx;

  return S_OK;
}

STDMETHODIMP
CEnumFormatEtc::Skip(ULONG aSkipNum)
{
  
  

  if ((mCurrentIdx + aSkipNum) >= mFormatList.Length())
      return S_FALSE;

  mCurrentIdx += aSkipNum;

  return S_OK;
}

STDMETHODIMP
CEnumFormatEtc::Reset(void)
{
  mCurrentIdx = 0;
  return S_OK;
}

STDMETHODIMP
CEnumFormatEtc::Clone(LPENUMFORMATETC *aResult)
{
  

  if (!aResult)
      return E_INVALIDARG;

  CEnumFormatEtc * pEnumObj = new CEnumFormatEtc(mFormatList);

  if (!pEnumObj)
      return E_OUTOFMEMORY;

  pEnumObj->AddRef();
  pEnumObj->SetIndex(mCurrentIdx);

  *aResult = pEnumObj;

  return S_OK;
}



void
CEnumFormatEtc::AddFormatEtc(LPFORMATETC aFormat)
{
  if (!aFormat)
      return;
  FormatEtc * etc = mFormatList.AppendElement();
  
  if (etc)
      etc->CopyIn(aFormat);
}



void
CEnumFormatEtc::SetIndex(PRUint32 aIdx)
{
  mCurrentIdx = aIdx;
}
