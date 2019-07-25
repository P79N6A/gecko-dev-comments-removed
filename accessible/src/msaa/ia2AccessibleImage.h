






#ifndef _ACCESSIBLE_IMAGE_H
#define _ACCESSIBLE_IMAGE_H

#include "AccessibleImage.h"

class ia2AccessibleImage : public IAccessibleImage
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
};

#endif

