





#include "EnumVariant.h"

using namespace mozilla;
using namespace mozilla::a11y;





STDMETHODIMP
ChildrenEnumVariant::QueryInterface(REFIID aIID, void** aObject)
{
__try {
  if (!aObject)
    return E_INVALIDARG;

  if (aIID == IID_IEnumVARIANT) {
    *aObject = static_cast<IEnumVARIANT*>(this);
    AddRef();
    return S_OK;
  }

  if (aIID == IID_IUnknown) {
    *aObject = static_cast<IUnknown*>(this);
    AddRef();
    return S_OK;
  }

  
  if (!mAnchorAcc->IsDefunct())
    return mAnchorAcc->QueryInterface(aIID, aObject);

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),
                                                  GetExceptionInformation())) { }

  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
ChildrenEnumVariant::AddRef()
{
  return ++mRefCnt;
}

ULONG STDMETHODCALLTYPE
ChildrenEnumVariant::Release()
{
  mRefCnt--;
  ULONG r = mRefCnt;
  if (r == 0)
    delete this;

  return r;
}

STDMETHODIMP
ChildrenEnumVariant::Next(ULONG aCount, VARIANT FAR* aItems,
                          ULONG FAR* aCountFetched)
{
__try {
  if (!aItems || !aCountFetched)
    return E_INVALIDARG;

  *aCountFetched = 0;

  if (mAnchorAcc->IsDefunct() || mAnchorAcc->GetChildAt(mCurIndex) != mCurAcc)
    return CO_E_OBJNOTCONNECTED;

  ULONG countFetched = 0;
  for (; mCurAcc && countFetched < aCount; countFetched++) {
    VariantInit(aItems + countFetched);
    aItems[countFetched].pdispVal = nsAccessibleWrap::NativeAccessible(mCurAcc);
    aItems[countFetched].vt = VT_DISPATCH;

    mCurIndex++;
    mCurAcc = mAnchorAcc->GetChildAt(mCurIndex);
  }

  (*aCountFetched) = countFetched;

  return countFetched < aCount ? S_FALSE : S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),
                                                  GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
ChildrenEnumVariant::Skip(ULONG aCount)
{
__try {
  if (mAnchorAcc->IsDefunct() || mAnchorAcc->GetChildAt(mCurIndex) != mCurAcc)
    return CO_E_OBJNOTCONNECTED;

  mCurIndex += aCount;
  mCurAcc = mAnchorAcc->GetChildAt(mCurIndex);

  return mCurAcc ? S_OK : S_FALSE;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),
                                                  GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
ChildrenEnumVariant::Reset()
{
__try {
  if (mAnchorAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  mCurIndex = 0;
  mCurAcc = mAnchorAcc->GetChildAt(0);

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),
                                                  GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
ChildrenEnumVariant::Clone(IEnumVARIANT** aEnumVariant)
{
__try {
  if (!aEnumVariant)
    return E_INVALIDARG;

  *aEnumVariant = new ChildrenEnumVariant(*this);
  (*aEnumVariant)->AddRef();

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),
                                                  GetExceptionInformation())) { }

  return E_FAIL;
}
