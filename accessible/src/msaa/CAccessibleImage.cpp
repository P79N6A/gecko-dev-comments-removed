







































#include "CAccessibleImage.h"

#include "AccessibleImage_i.c"

#include "nsIAccessible.h"
#include "nsIAccessibleImage.h"

#include "nsCOMPtr.h"
#include "nsString.h"



STDMETHODIMP
CAccessibleImage::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleImage == iid) {
    nsCOMPtr<nsIAccessibleImage> imageAcc(do_QueryInterface(this));
    if (!imageAcc)
      return E_FAIL;

    *ppv = static_cast<IAccessibleImage*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleImage::get_description(BSTR *aDescription)
{
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString description;
  nsresult rv = acc->GetName(description);
  if (NS_FAILED(rv))
    return E_FAIL;

  return ::SysReAllocStringLen(aDescription, description.get(),
                               description.Length());
}

STDMETHODIMP
CAccessibleImage::get_imagePosition(enum IA2CoordinateType aCoordinateType,
                                    long *aX,
                                    long *aY)
{
  *aX = 0;
  *aY = 0;

  
  
  
  if (aCoordinateType != IA2_COORDTYPE_SCREEN_RELATIVE)
    return E_NOTIMPL;

  nsCOMPtr<nsIAccessibleImage> imageAcc(do_QueryInterface(this));
  if (!imageAcc)
    return E_FAIL;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = imageAcc->GetImageBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aX = x;
  *aY = y;

  return S_OK;
}

STDMETHODIMP
CAccessibleImage::get_imageSize(long *aHeight, long *aWidth)
{
  *aHeight = 0;
  *aWidth = 0;

  nsCOMPtr<nsIAccessibleImage> imageAcc(do_QueryInterface(this));
  if (!imageAcc)
    return E_FAIL;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = imageAcc->GetImageBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aHeight = width;
  *aWidth = height;

  return S_OK;
}

