





#ifndef mozilla_a11y_EnumVariant_h__
#define mozilla_a11y_EnumVariant_h__

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {




class ChildrenEnumVariant MOZ_FINAL : public IEnumVARIANT
{
public:
  ChildrenEnumVariant(AccessibleWrap* aAnchor) : mAnchorAcc(aAnchor),
    mCurAcc(mAnchorAcc->GetChildAt(0)), mCurIndex(0), mRefCnt(0) { }

  
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(
     REFIID aRefIID,
     void** aObject);

  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  
  virtual  HRESULT STDMETHODCALLTYPE Next(
     ULONG aCount,
     VARIANT* aItems,
     ULONG* aCountFetched);

  virtual HRESULT STDMETHODCALLTYPE Skip(
     ULONG aCount);

  virtual HRESULT STDMETHODCALLTYPE Reset();

  virtual HRESULT STDMETHODCALLTYPE Clone(
     IEnumVARIANT** aEnumVaraint);

private:
  ChildrenEnumVariant() MOZ_DELETE;
  ChildrenEnumVariant& operator =(const ChildrenEnumVariant&) MOZ_DELETE;

  ChildrenEnumVariant(const ChildrenEnumVariant& aEnumVariant) :
    mAnchorAcc(aEnumVariant.mAnchorAcc), mCurAcc(aEnumVariant.mCurAcc),
    mCurIndex(aEnumVariant.mCurIndex), mRefCnt(0) { }
  virtual ~ChildrenEnumVariant() { }

protected:
  nsRefPtr<AccessibleWrap> mAnchorAcc;
  Accessible* mCurAcc;
  PRUint32 mCurIndex;

private:
  ULONG mRefCnt;
};

} 
} 

#endif
