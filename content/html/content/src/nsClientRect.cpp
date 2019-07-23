





































#include "nsClientRect.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"

NS_INTERFACE_TABLE_HEAD(nsClientRect)
  NS_INTERFACE_TABLE1(nsClientRect, nsIDOMClientRect)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ClientRect)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsClientRect)
NS_IMPL_RELEASE(nsClientRect)

nsClientRect::nsClientRect()
  : mX(0.0), mY(0.0), mWidth(0.0), mHeight(0.0)
{
}

NS_IMETHODIMP
nsClientRect::GetLeft(float* aResult)
{
  *aResult = mX;
  return NS_OK;
}

NS_IMETHODIMP
nsClientRect::GetTop(float* aResult)
{
  *aResult = mY;
  return NS_OK;
}

NS_IMETHODIMP
nsClientRect::GetRight(float* aResult)
{
  *aResult = mX + mWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsClientRect::GetBottom(float* aResult)
{
  *aResult = mY + mHeight;
  return NS_OK;
}

NS_INTERFACE_TABLE_HEAD(nsClientRectList)
  NS_INTERFACE_TABLE1(nsClientRectList, nsIDOMClientRectList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ClientRectList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsClientRectList)
NS_IMPL_RELEASE(nsClientRectList)


NS_IMETHODIMP    
nsClientRectList::GetLength(PRUint32* aLength)
{
  *aLength = mArray.Count();
  return NS_OK;
}

NS_IMETHODIMP    
nsClientRectList::Item(PRUint32 aIndex, nsIDOMClientRect** aReturn)
{
  if (aIndex >= PRUint32(mArray.Count())) {
    *aReturn = nsnull;
    return NS_OK;
  } 
  
  NS_IF_ADDREF(*aReturn = mArray.ObjectAt(aIndex));
  return NS_OK;
}
