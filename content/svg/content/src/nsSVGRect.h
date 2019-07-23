






































#ifndef __NS_SVGRECT_H__
#define __NS_SVGRECT_H__

#include "nsIDOMSVGRect.h"
#include "nsSVGValue.h"
#include "gfxRect.h"

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




class nsSVGRect : public nsIDOMSVGRect,
                  public nsSVGValue
{
public:
  nsSVGRect(float x=0.0f, float y=0.0f, float w=0.0f, float h=0.0f);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGRECT

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  void Clear();

protected:
  float mX, mY, mWidth, mHeight;
};

#endif 
