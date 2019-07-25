







































#ifndef _ACCESSIBLE_IMAGE_H
#define _ACCESSIBLE_IMAGE_H

#include "nsISupports.h"

#include "AccessibleImage.h"

class CAccessibleImage: public IAccessibleImage
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual  HRESULT STDMETHODCALLTYPE get_description(
       BSTR *description);

  virtual  HRESULT STDMETHODCALLTYPE get_imagePosition(
       enum IA2CoordinateType coordinateType,
       long *x,
       long *y);

  virtual  HRESULT STDMETHODCALLTYPE get_imageSize(
       long *height,
       long *width);

  
  NS_IMETHOD QueryInterface(const nsIID& uuid, void** result) = 0;

};

#endif

