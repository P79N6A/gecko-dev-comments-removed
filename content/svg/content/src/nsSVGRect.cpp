






































#include "nsSVGRect.h"
#include "prdtoa.h"
#include "nsSVGValue.h"
#include "nsISVGValueUtils.h"
#include "nsTextFormatter.h"
#include "nsCRT.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGLength.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"




class nsSVGRect : public nsIDOMSVGRect,
                  public nsSVGValue
{
public:
  nsSVGRect(float x=0.0f, float y=0.0f, float w=0.0f, float h=0.0f);
  
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGRECT

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);


protected:
  float mX, mY, mWidth, mHeight;
};




nsSVGRect::nsSVGRect(float x, float y, float w, float h)
    : mX(x), mY(y), mWidth(w), mHeight(h)
{
}




NS_IMPL_ADDREF(nsSVGRect)
NS_IMPL_RELEASE(nsSVGRect)

NS_INTERFACE_MAP_BEGIN(nsSVGRect)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGRect)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGRect::SetValueString(const nsAString& aValue)
{
  nsresult rv = NS_OK;

  char* str = ToNewCString(aValue);

  char* rest = str;
  char* token;
  const char* delimiters = ",\x20\x9\xD\xA";

  double vals[4];
  int i;
  for (i=0;i<4;++i) {
    if (!(token = nsCRT::strtok(rest, delimiters, &rest))) break; 

    char *end;
    vals[i] = PR_strtod(token, &end);
    if (*end != '\0') break; 
  }
  if (i!=4 || (nsCRT::strtok(rest, delimiters, &rest)!=0)) {
    
    rv = NS_ERROR_FAILURE;
  }
  else {
    WillModify();
    mX      = float(vals[0]);
    mY      = float(vals[1]);
    mWidth  = float(vals[2]);
    mHeight = float(vals[3]);
    DidModify();
  }

  nsMemory::Free(str);

  return rv;
}

NS_IMETHODIMP
nsSVGRect::GetValueString(nsAString& aValue)
{
  PRUnichar buf[200];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g %g %g %g").get(),
                            (double)mX, (double)mY,
                            (double)mWidth, (double)mHeight);
  aValue.Assign(buf);

  return NS_OK;
}





NS_IMETHODIMP nsSVGRect::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetX(float aX)
{
  WillModify();
  mX = aX;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetY(float aY)
{
  WillModify();
  mY = aY;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetWidth(float *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetWidth(float aWidth)
{
  WillModify();
  mWidth = aWidth;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetHeight(float *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetHeight(float aHeight)
{
  WillModify();
  mHeight = aHeight;
  DidModify();
  return NS_OK;
}









class nsSVGReadonlyRect : public nsSVGRect
{
public:
  nsSVGReadonlyRect(float x, float y, float width, float height)
    : nsSVGRect(x, y, width, height)
  {
  }

  
  NS_IMETHODIMP SetX(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetY(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetWidth(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetHeight(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetValueString(const nsAString&) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
};




nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, float x, float y,
              float width, float height)
{
  *result = new nsSVGRect(x, y, width, height);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, const gfxRect& rect)
{
  return NS_NewSVGRect(result,
                       rect.X(), rect.Y(),
                       rect.Width(), rect.Height());
}

nsresult
NS_NewSVGReadonlyRect(nsIDOMSVGRect** result, float x, float y,
                      float width, float height)
{
  *result = new nsSVGReadonlyRect(x, y, width, height);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

