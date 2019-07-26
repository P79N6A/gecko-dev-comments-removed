



#include "nsAsyncScrollEventDetail.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMClassInfo.h"
#include "nsIClassInfo.h"
#include "nsDOMClassInfo.h"

NS_IMPL_ADDREF(nsAsyncScrollEventDetail)
NS_IMPL_RELEASE(nsAsyncScrollEventDetail)
NS_INTERFACE_MAP_BEGIN(nsAsyncScrollEventDetail)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIAsyncScrollEventDetail)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(AsyncScrollEventDetail)
NS_INTERFACE_MAP_END

DOMCI_DATA(AsyncScrollEventDetail, nsAsyncScrollEventDetail)


NS_IMETHODIMP nsAsyncScrollEventDetail::GetTop(float *aTop)
{
  *aTop = mTop;
  return NS_OK;
}


NS_IMETHODIMP nsAsyncScrollEventDetail::GetLeft(float *aLeft)
{
  *aLeft = mLeft;
  return NS_OK;
}


NS_IMETHODIMP nsAsyncScrollEventDetail::GetWidth(float *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}


NS_IMETHODIMP nsAsyncScrollEventDetail::GetHeight(float *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}


NS_IMETHODIMP nsAsyncScrollEventDetail::GetScrollWidth(float *aScrollWidth)
{
  *aScrollWidth = mScrollWidth;
  return NS_OK;
}


NS_IMETHODIMP nsAsyncScrollEventDetail::GetScrollHeight(float *aScrollHeight)
{
  *aScrollHeight = mScrollHeight;
  return NS_OK;
}

