




































#include "AccCollector.h"

#include "nsAccessible.h"





AccCollector::
  AccCollector(nsAccessible* aRoot, filters::FilterFuncPtr aFilterFunc) :
  mFilterFunc(aFilterFunc), mRoot(aRoot), mRootChildIdx(0)
{
}

AccCollector::~AccCollector()
{
}

PRUint32
AccCollector::Count()
{
  EnsureNGetIndex(nsnull);
  return mObjects.Length();
}

nsAccessible*
AccCollector::GetAccessibleAt(PRUint32 aIndex)
{
  nsAccessible *accessible = mObjects.SafeElementAt(aIndex, nsnull);
  if (accessible)
    return accessible;

  return EnsureNGetObject(aIndex);
}

PRInt32
AccCollector::GetIndexAt(nsAccessible *aAccessible)
{
  PRInt32 index = mObjects.IndexOf(aAccessible);
  if (index != -1)
    return index;

  return EnsureNGetIndex(aAccessible);
}




nsAccessible*
AccCollector::EnsureNGetObject(PRUint32 aIndex)
{
  PRInt32 childCount = mRoot->GetChildCount();
  while (mRootChildIdx < childCount) {
    nsAccessible* child = mRoot->GetChildAt(mRootChildIdx++);
    if (!mFilterFunc(child))
      continue;

    AppendObject(child);
    if (mObjects.Length() - 1 == aIndex)
      return mObjects[aIndex];
  }

  return nsnull;
}

PRInt32
AccCollector::EnsureNGetIndex(nsAccessible* aAccessible)
{
  PRInt32 childCount = mRoot->GetChildCount();
  while (mRootChildIdx < childCount) {
    nsAccessible* child = mRoot->GetChildAt(mRootChildIdx++);
    if (!mFilterFunc(child))
      continue;

    AppendObject(child);
    if (child == aAccessible)
      return mObjects.Length() - 1;
  }

  return -1;
}

void
AccCollector::AppendObject(nsAccessible* aAccessible)
{
  mObjects.AppendElement(aAccessible);
}





PRInt32
EmbeddedObjCollector::GetIndexAt(nsAccessible *aAccessible)
{
  if (aAccessible->mParent != mRoot)
    return -1;

  if (aAccessible->mIndexOfEmbeddedChild != -1)
    return aAccessible->mIndexOfEmbeddedChild;

  return mFilterFunc(aAccessible) ? EnsureNGetIndex(aAccessible) : -1;
}

void
EmbeddedObjCollector::AppendObject(nsAccessible* aAccessible)
{
  aAccessible->mIndexOfEmbeddedChild = mObjects.Length();
  mObjects.AppendElement(aAccessible);
}
