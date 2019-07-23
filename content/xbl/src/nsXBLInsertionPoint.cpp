





































#include "nsXBLInsertionPoint.h"
#include "nsContentUtils.h"

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

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLInsertionPoint)
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

nsIContent*
nsXBLInsertionPoint::GetInsertionParent()
{
  return mParentElement;
}

nsIContent*
nsXBLInsertionPoint::GetDefaultContent()
{
  return mDefaultContent;
}

nsIContent*
nsXBLInsertionPoint::GetDefaultContentTemplate()
{
  return mDefaultContentTemplate;
}

nsIContent*
nsXBLInsertionPoint::ChildAt(PRUint32 aIndex)
{
  return mElements.ObjectAt(aIndex);
}

PRBool
nsXBLInsertionPoint::Matches(nsIContent* aContent, PRUint32 aIndex)
{
  return (aContent == mParentElement && mIndex != -1 && ((PRInt32)aIndex) == mIndex);
}

void
nsXBLInsertionPoint::UnbindDefaultContent()
{
  if (!mDefaultContent) {
    return;
  }

  
  nsCOMPtr<nsIContent> defContent = mDefaultContent;

  nsAutoScriptBlocker scriptBlocker;

  
  
  
  PRUint32 childCount = mDefaultContent->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    defContent->GetChildAt(i)->UnbindFromTree();
  }

  defContent->UnbindFromTree();
}
