




































#include "IENUMFE.H"
#include <stdio.h>
#include "nsISupports.h"










CEnumFormatEtc::CEnumFormatEtc(ULONG aNumFEs, LPFORMATETC aFEList)
{
  UINT        i;

  mRefCnt     = 0;

  mCurrentInx = 0;
  mNumFEs     = aNumFEs;
  mMaxNumFEs  = aNumFEs;
  mFEList     = new FORMATETC[(UINT)aNumFEs];

  if (NULL!=mFEList) {
    for (i=0; i < aNumFEs; i++) {
      mFEList[i] = aFEList[i];
    }
  }

}










CEnumFormatEtc::CEnumFormatEtc(ULONG aMaxFE)
{
  mRefCnt     = 0;

  mCurrentInx = 0;
  mNumFEs     = 0;
  mMaxNumFEs  = aMaxFE;
  mFEList     = new FORMATETC[(UINT)aMaxFE];
}



CEnumFormatEtc::~CEnumFormatEtc(void)
{
  if (NULL != mFEList) {
    delete [] mFEList;
  }

}


void CEnumFormatEtc::AddFE(LPFORMATETC aFE)
{
  mFEList[mNumFEs++] = *aFE;
}


bool CEnumFormatEtc::InsertFEAt(LPFORMATETC aFE, ULONG aIndex)
{
  if (NULL==aFE || aIndex >= mNumFEs) return FALSE;

  UINT i;
  if (mNumFEs == mMaxNumFEs) {
    
    
    LPFORMATETC newFEList = (LPFORMATETC) new FORMATETC[(UINT) mNumFEs + 1];
    for (i = 0; i < mNumFEs - 1; ++i) {
      newFEList[i] = mFEList[i];
    }
    delete [] mFEList;
    mFEList = newFEList;
    ++mMaxNumFEs;
  }  
  
  
  
  
  for (i = mNumFEs; i > aIndex; --i) {
    mFEList[i] = mFEList[i - 1];
  }
  mFEList[aIndex] = *aFE;
  ++mNumFEs;

  return TRUE;
}

















STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, LPVOID *ppv)
{
  *ppv=NULL;

  




  if (IsEqualIID(riid, IID_IUnknown)
      || IsEqualIID(riid, IID_IEnumFORMATETC))
      *ppv=(LPVOID)this;

  
  if (NULL!=*ppv)
      {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
      }

  return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
{
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "CEnumFormatEtc",sizeof(*this));
#ifdef DEBUG
  
#endif
  return mRefCnt;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
{
  ULONG cRefT;

  cRefT = --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "CEnumFormatEtc");

  if (0L == mRefCnt)
      delete this;

#ifdef DEBUG
  
#endif
  return cRefT;
}



















STDMETHODIMP CEnumFormatEtc::Next(ULONG cFE, LPFORMATETC pFE, ULONG * pulFE)
{
  ULONG               cReturn=0L;

  if (NULL==mFEList)
      return ResultFromScode(S_FALSE);

  if (NULL==pulFE)
      {
      if (1L!=cFE)
          return ResultFromScode(E_POINTER);
      }
  else
      *pulFE=0L;

  if (NULL==pFE || mCurrentInx >= mNumFEs)
      return ResultFromScode(S_FALSE);

  while (mCurrentInx < mNumFEs && cFE > 0)
      {
      *pFE++=mFEList[mCurrentInx++];
      cReturn++;
      cFE--;
      }

  if (NULL!=pulFE)
      *pulFE=cReturn;

  return NOERROR;
}















STDMETHODIMP CEnumFormatEtc::Skip(ULONG cSkip)
{
  if (((mCurrentInx+cSkip) >= mNumFEs) || NULL==mFEList)
      return ResultFromScode(S_FALSE);

  mCurrentInx+=cSkip;
  return NOERROR;
}














STDMETHODIMP CEnumFormatEtc::Reset(void)
{
  mCurrentInx=0;
  return NOERROR;
}















STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC *ppEnum)
{
  LPCEnumFormatEtc    pNew;

  *ppEnum=NULL;

  
  pNew = new CEnumFormatEtc(mNumFEs, mFEList);

  if (NULL==pNew)
      return ResultFromScode(E_OUTOFMEMORY);

  pNew->AddRef();
  pNew->mCurrentInx=mCurrentInx;

  *ppEnum=pNew;
  return NOERROR;
}
