





#ifndef mozilla_a11y_EnumVariant_h__
#define mozilla_a11y_EnumVariant_h__

#include "AccessibleWrap.h"
#include "IUnknownImpl.h"

namespace mozilla {
namespace a11y {




class ChildrenEnumVariant MOZ_FINAL : public IEnumVARIANT
{
public:
  ChildrenEnumVariant(AccessibleWrap* aAnchor) : mAnchorAcc(aAnchor),
    mCurAcc(mAnchorAcc->GetChildAt(0)), mCurIndex(0) { }

  
  DECL_IUNKNOWN

  
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
  ChildrenEnumVariant() = delete;
  ChildrenEnumVariant& operator =(const ChildrenEnumVariant&) = delete;

  ChildrenEnumVariant(const ChildrenEnumVariant& aEnumVariant) :
    mAnchorAcc(aEnumVariant.mAnchorAcc), mCurAcc(aEnumVariant.mCurAcc),
    mCurIndex(aEnumVariant.mCurIndex) { }
  virtual ~ChildrenEnumVariant() { }

protected:
  nsRefPtr<AccessibleWrap> mAnchorAcc;
  Accessible* mCurAcc;
  uint32_t mCurIndex;
};

} 
} 

#endif
