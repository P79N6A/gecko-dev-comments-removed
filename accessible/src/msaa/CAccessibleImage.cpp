







































#include "CAccessibleImage.h"

#include "AccessibleImage_i.c"

#include "nsIAccessible.h"
#include "nsIAccessibleImage.h"
#include "nsIAccessibleTypes.h"
#include "nsAccessNodeWrap.h"

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
__try {
  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsAutoString description;
  nsresult rv = acc->GetName(description);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (!::SysReAllocStringLen(aDescription, description.get(), description.Length()))
    return E_OUTOFMEMORY;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleImage::get_imagePosition(enum IA2CoordinateType aCoordType,
                                    long *aX,
                                    long *aY)
{
__try {
  *aX = 0;
  *aY = 0;

  PRUint32 geckoCoordType = (aCoordType == IA2_COORDTYPE_SCREEN_RELATIVE) ?
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE;

  nsCOMPtr<nsIAccessibleImage> imageAcc(do_QueryInterface(this));
  if (!imageAcc)
    return E_FAIL;

  PRInt32 x = 0, y = 0;
  nsresult rv = imageAcc->GetImagePosition(geckoCoordType, &x, &y);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aX = x;
  *aY = y;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleImage::get_imageSize(long *aHeight, long *aWidth)
{
__try {
  *aHeight = 0;
  *aWidth = 0;

  nsCOMPtr<nsIAccessibleImage> imageAcc(do_QueryInterface(this));
  if (!imageAcc)
    return E_FAIL;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = imageAcc->GetImageSize(&width, &height);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aHeight = width;
  *aWidth = height;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

