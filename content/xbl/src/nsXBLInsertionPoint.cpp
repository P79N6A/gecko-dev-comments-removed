





































#include "nsXBLInsertionPoint.h"

nsXBLInsertionPoint::nsXBLInsertionPoint(nsIContent* aParentElement,
                                         PRUint32 aIndex,
                                         nsIContent* aDefaultContent)
  : mParentElement(aParentElement),
    mIndex(aIndex),
    mDefaultContentTemplate(aDefaultContent)
{
}

nsXBLInsertionPoint::~nsXBLInsertionPoint()
{
}

nsrefcnt
nsXBLInsertionPoint::Release()
{
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsXBLInsertionPoint");
  if (mRefCnt == 0) {
    mRefCnt = 1;
    delete this;
    return 0;
  }
  return mRefCnt;
}

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(nsXBLInsertionPoint)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsXBLInsertionPoint)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mElements)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDefaultContentTemplate)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDefaultContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXBLInsertionPoint)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mElements)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDefaultContentTemplate)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDefaultContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXBLInsertionPoint, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXBLInsertionPoint, Release)

already_AddRefed<nsIContent>
nsXBLInsertionPoint::GetInsertionParent()
{
  NS_IF_ADDREF(mParentElement);
  return mParentElement;
}

already_AddRefed<nsIContent>
nsXBLInsertionPoint::GetDefaultContent()
{
  nsIContent* defaultContent = mDefaultContent;
  NS_IF_ADDREF(defaultContent);
  return defaultContent;
}

already_AddRefed<nsIContent>
nsXBLInsertionPoint::GetDefaultContentTemplate()
{
  nsIContent* defaultContent = mDefaultContentTemplate;
  NS_IF_ADDREF(defaultContent);
  return defaultContent;
}

already_AddRefed<nsIContent>
nsXBLInsertionPoint::ChildAt(PRUint32 aIndex)
{
  nsIContent* result = mElements.ObjectAt(aIndex);
  NS_IF_ADDREF(result);
  return result;
}

PRBool
nsXBLInsertionPoint::Matches(nsIContent* aContent, PRUint32 aIndex)
{
  return (aContent == mParentElement && mIndex != -1 && ((PRInt32)aIndex) == mIndex);
}
