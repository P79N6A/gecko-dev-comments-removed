





#include "nsScriptableRegion.h"
#include <stdint.h>                     
#include <sys/types.h>                  
#include "js/RootingAPI.h"              
#include "js/Value.h"                   
#include "jsapi.h"                      
#include "mozilla/Assertions.h"         
#include "nsError.h"                    
#include "nsID.h"
#include "nsRect.h"                     
#include "nscore.h"                     

class JSObject;
struct JSContext;

nsScriptableRegion::nsScriptableRegion()
{
}

NS_IMPL_ISUPPORTS(nsScriptableRegion, nsIScriptableRegion)

NS_IMETHODIMP nsScriptableRegion::Init()
{
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SetToRegion(nsIScriptableRegion *aRegion)
{
  aRegion->GetRegion(&mRegion);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SetToRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight)
{
  mRegion = nsIntRect(aX, aY, aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IntersectRegion(nsIScriptableRegion *aRegion)
{
  nsIntRegion region;
  aRegion->GetRegion(&region);
  mRegion.And(mRegion, region);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IntersectRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight)
{
  mRegion.And(mRegion, nsIntRect(aX, aY, aWidth, aHeight));
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::UnionRegion(nsIScriptableRegion *aRegion)
{
  nsIntRegion region;
  aRegion->GetRegion(&region);
  mRegion.Or(mRegion, region);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::UnionRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight)
{
  mRegion.Or(mRegion, nsIntRect(aX, aY, aWidth, aHeight));
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SubtractRegion(nsIScriptableRegion *aRegion)
{
  nsIntRegion region;
  aRegion->GetRegion(&region);
  mRegion.Sub(mRegion, region);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::SubtractRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight)
{
  mRegion.Sub(mRegion, nsIntRect(aX, aY, aWidth, aHeight));
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IsEmpty(bool *isEmpty)
{
  *isEmpty = mRegion.IsEmpty();
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::IsEqualRegion(nsIScriptableRegion *aRegion, bool *isEqual)
{
  nsIntRegion region;
  aRegion->GetRegion(&region);
  *isEqual = mRegion.IsEqual(region);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::GetBoundingBox(int32_t *aX, int32_t *aY, int32_t *aWidth, int32_t *aHeight)
{
  nsIntRect boundRect = mRegion.GetBounds();
  *aX = boundRect.x;
  *aY = boundRect.y;
  *aWidth = boundRect.width;
  *aHeight = boundRect.height;
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::Offset(int32_t aXOffset, int32_t aYOffset)
{
  mRegion.MoveBy(aXOffset, aYOffset);
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::ContainsRect(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight, bool *containsRect)
{
  *containsRect = mRegion.Contains(nsIntRect(aX, aY, aWidth, aHeight));
  return NS_OK;
}


NS_IMETHODIMP nsScriptableRegion::GetRegion(nsIntRegion* outRgn)
{
  *outRgn = mRegion;
  return NS_OK;
}

NS_IMETHODIMP nsScriptableRegion::GetRects(JSContext* aCx, JS::MutableHandle<JS::Value> aRects)
{
  uint32_t numRects = mRegion.GetNumRects();

  if (!numRects) {
    aRects.setNull();
    return NS_OK;
  }

  JS::Rooted<JSObject*> destArray(aCx, JS_NewArrayObject(aCx, numRects * 4));
  if (!destArray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aRects.setObject(*destArray);

  uint32_t n = 0;
  nsIntRegionRectIterator iter(mRegion);
  const nsIntRect *rect;

  while ((rect = iter.Next())) {
    if (!JS_DefineElement(aCx, destArray, n, rect->x, JSPROP_ENUMERATE) ||
        !JS_DefineElement(aCx, destArray, n + 1, rect->y, JSPROP_ENUMERATE) ||
        !JS_DefineElement(aCx, destArray, n + 2, rect->width, JSPROP_ENUMERATE) ||
        !JS_DefineElement(aCx, destArray, n + 3, rect->height, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }
    n += 4;
  }

  return NS_OK;
}
