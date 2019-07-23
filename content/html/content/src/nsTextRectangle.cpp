





































#include "nsTextRectangle.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"

NS_INTERFACE_MAP_BEGIN(nsTextRectangle)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextRectangle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(TextRectangle)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsTextRectangle)
NS_IMPL_RELEASE(nsTextRectangle)

nsTextRectangle::nsTextRectangle()
  : mX(0.0), mY(0.0), mWidth(0.0), mHeight(0.0)
{
}

NS_IMETHODIMP
nsTextRectangle::GetLeft(float* aResult)
{
  *aResult = mX;
  return NS_OK;
}

NS_IMETHODIMP
nsTextRectangle::GetTop(float* aResult)
{
  *aResult = mY;
  return NS_OK;
}

NS_IMETHODIMP
nsTextRectangle::GetRight(float* aResult)
{
  *aResult = mX + mWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsTextRectangle::GetBottom(float* aResult)
{
  *aResult = mY + mHeight;
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN(nsTextRectangleList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextRectangleList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(TextRectangleList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsTextRectangleList)
NS_IMPL_RELEASE(nsTextRectangleList)


NS_IMETHODIMP    
nsTextRectangleList::GetLength(PRUint32* aLength)
{
  *aLength = mArray.Count();
  return NS_OK;
}

NS_IMETHODIMP    
nsTextRectangleList::Item(PRUint32 aIndex, nsIDOMTextRectangle** aReturn)
{
  if (aIndex >= PRUint32(mArray.Count())) {
    *aReturn = nsnull;
    return NS_OK;
  } 
  
  NS_IF_ADDREF(*aReturn = mArray.ObjectAt(aIndex));
  return NS_OK;
}
