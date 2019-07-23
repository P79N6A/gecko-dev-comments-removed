






































#ifndef __NS_SVGRECT_H__
#define __NS_SVGRECT_H__

#include "nsIDOMSVGRect.h"
#include "gfxRect.h"

class nsIDOMSVGLength;

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result,
              float x=0.0f, float y=0.0f,
              float width=0.0f, float height=0.0f);

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, const gfxRect& rect);

nsresult
NS_NewSVGReadonlyRect(nsIDOMSVGRect** result,
                      float x=0.0f, float y=0.0f,
                      float width=0.0f, float height=0.0f);

#endif 
