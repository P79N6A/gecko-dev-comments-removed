






































#include "nsScriptableRegion.h"
#include "nsCOMPtr.h"

nsScriptableRegion::nsScriptableRegion(nsIRegion* region) : mRegion(nsnull), mRectSet(nsnull)
{
	mRegion = region;
	NS_IF_ADDREF(mRegion);
}

nsScriptableRegion::~nsScriptableRegion()
{
  if (mRegion) {
    mRegion->FreeRects(mRectSet);
    NS_RELEASE(mRegion);
  }
}

NS_IMPL_ISUPPORTS1(nsScriptableRegion, nsIScriptableRegion)

NS_IMETHODIMP nsScriptableRegion::Init()
{
	return mRegion->Init();
}

NS_IMETHODIMP nsScriptableRegion::SetToRegion(nsIScriptableRegion *aRegion)
{
	nsCOMPtr<nsIRegion> region(do_QueryInterface(aRegion));
	mRegion->SetTo(*region);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SetToRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion->SetTo(aX, aY, aWidth, aHeight);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IntersectRegion(nsIScriptableRegion *aRegion)
{
	nsCOMPtr<nsIRegion> region(do_QueryInterface(aRegion));
	mRegion->Intersect(*region);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IntersectRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion->Intersect(aX, aY, aWidth, aHeight);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::UnionRegion(nsIScriptableRegion *aRegion)
{
	nsCOMPtr<nsIRegion> region(do_QueryInterface(aRegion));
	mRegion->Union(*region);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::UnionRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion->Union(aX, aY, aWidth, aHeight);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SubtractRegion(nsIScriptableRegion *aRegion)
{
	nsCOMPtr<nsIRegion> region(do_QueryInterface(aRegion));
	mRegion->Subtract(*region);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SubtractRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	mRegion->Subtract(aX, aY, aWidth, aHeight);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IsEmpty(PRBool *isEmpty)
{
	*isEmpty = mRegion->IsEmpty();
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IsEqualRegion(nsIScriptableRegion *aRegion, PRBool *isEqual)
{
	nsCOMPtr<nsIRegion> region(do_QueryInterface(aRegion));
	*isEqual = mRegion->IsEqual(*region);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
	mRegion->GetBoundingBox(aX, aY, aWidth, aHeight);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
	mRegion->Offset(aXOffset, aYOffset);
	return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool *containsRect)
{
	*containsRect = mRegion->ContainsRect(aX, aY, aWidth, aHeight);
	return NS_OK;
}


NS_IMETHODIMP nsScriptableRegion::GetRegion(nsIRegion** outRgn)
{
  *outRgn = mRegion;
  NS_IF_ADDREF(*outRgn);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::GetRect(PRUint32 aIndex, PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight) {
  
  nsresult rv = mRegion->GetRects(&mRectSet);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aIndex < mRectSet->mNumRects, NS_ERROR_INVALID_ARG);
  nsRegionRect &rect = mRectSet->mRects[aIndex];
  *aX = rect.x;
  *aY = rect.y;
  *aWidth = rect.width;
  *aHeight = rect.height;
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::GetNumRects(PRUint32 *aLength) {
  return mRegion->GetNumRects(aLength);
}
