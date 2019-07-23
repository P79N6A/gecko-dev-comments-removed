





































#include "nsSVGPreserveAspectRatio.h"
#include "nsSVGValue.h"
#include "nsCRT.h"
#include "nsContentUtils.h"




class nsSVGPreserveAspectRatio : public nsIDOMSVGPreserveAspectRatio,
                                 public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGPreserveAspectRatio(
                                        nsIDOMSVGPreserveAspectRatio** result,
                                        PRUint16 aAlign,
                                        PRUint16 aMeetOrSlice);

  nsSVGPreserveAspectRatio(PRUint16 aAlign, PRUint16 aMeetOrSlice);
  ~nsSVGPreserveAspectRatio();

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPRESERVEASPECTRATIO

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

protected:
  PRUint16 mAlign, mMeetOrSlice;
};





nsSVGPreserveAspectRatio::nsSVGPreserveAspectRatio(PRUint16 aAlign,
                                                   PRUint16 aMeetOrSlice)
    : mAlign(aAlign), mMeetOrSlice(aMeetOrSlice)
{
}

nsSVGPreserveAspectRatio::~nsSVGPreserveAspectRatio()
{
}




NS_IMPL_ADDREF(nsSVGPreserveAspectRatio)
NS_IMPL_RELEASE(nsSVGPreserveAspectRatio)

NS_INTERFACE_MAP_BEGIN(nsSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPreserveAspectRatio)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGPreserveAspectRatio::SetValueString(const nsAString& aValue)
{
  char* str = ToNewCString(aValue);
  if (!str) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_OK;

  char* rest = str;
  char* token;
  const char* delimiters = "\x20\x9\xD\xA";
  PRUint16 align, meetOrSlice;

  token = nsCRT::strtok(rest, delimiters, &rest);

  if (token && !strcmp(token, "defer"))
    
    token = nsCRT::strtok(rest, delimiters, &rest);

  if (token) {
    if (!strcmp(token, "none"))
      align = SVG_PRESERVEASPECTRATIO_NONE;
    else if (!strcmp(token, "xMinYMin"))
      align = SVG_PRESERVEASPECTRATIO_XMINYMIN;
    else if (!strcmp(token, "xMidYMin"))
      align = SVG_PRESERVEASPECTRATIO_XMIDYMIN;
    else if (!strcmp(token, "xMaxYMin"))
      align = SVG_PRESERVEASPECTRATIO_XMAXYMIN;
    else if (!strcmp(token, "xMinYMid"))
      align = SVG_PRESERVEASPECTRATIO_XMINYMID;
    else if (!strcmp(token, "xMidYMid"))
      align = SVG_PRESERVEASPECTRATIO_XMIDYMID;
    else if (!strcmp(token, "xMaxYMid"))
      align = SVG_PRESERVEASPECTRATIO_XMAXYMID;
    else if (!strcmp(token, "xMinYMax"))
      align = SVG_PRESERVEASPECTRATIO_XMINYMAX;
    else if (!strcmp(token, "xMidYMax"))
      align = SVG_PRESERVEASPECTRATIO_XMIDYMAX;
    else if (!strcmp(token, "xMaxYMax"))
      align = SVG_PRESERVEASPECTRATIO_XMAXYMAX;
    else
      rv = NS_ERROR_FAILURE;

    if (NS_SUCCEEDED(rv)) {
      token = nsCRT::strtok(rest, delimiters, &rest);
      if (token) {
        if (!strcmp(token, "meet"))
          meetOrSlice = SVG_MEETORSLICE_MEET;
        else if (!strcmp(token, "slice"))
          meetOrSlice = SVG_MEETORSLICE_SLICE;
        else
          rv = NS_ERROR_FAILURE;
      }
      else
        meetOrSlice = SVG_MEETORSLICE_MEET;
    }
  }
  else  
    rv = NS_ERROR_FAILURE;

  if (nsCRT::strtok(rest, delimiters, &rest))  
    rv = NS_ERROR_FAILURE;

  if (NS_SUCCEEDED(rv)) {
    WillModify();
    mAlign = align;
    mMeetOrSlice = meetOrSlice;
    DidModify();
  }

  nsMemory::Free(str);

  return rv;
}

NS_IMETHODIMP
nsSVGPreserveAspectRatio::GetValueString(nsAString& aValue)
{
  

  switch (mAlign) {
    case SVG_PRESERVEASPECTRATIO_NONE:
      aValue.AssignLiteral("none");
      break;
    case SVG_PRESERVEASPECTRATIO_XMINYMIN:
      aValue.AssignLiteral("xMinYMin");
      break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMIN:
      aValue.AssignLiteral("xMidYMin");
      break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMIN:
      aValue.AssignLiteral("xMaxYMin");
      break;
    case SVG_PRESERVEASPECTRATIO_XMINYMID:
      aValue.AssignLiteral("xMinYMid");
      break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMID:
      aValue.AssignLiteral("xMidYMid");
      break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMID:
      aValue.AssignLiteral("xMaxYMid");
      break;
    case SVG_PRESERVEASPECTRATIO_XMINYMAX:
      aValue.AssignLiteral("xMinYMax");
      break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMAX:
      aValue.AssignLiteral("xMidYMax");
      break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMAX:
      aValue.AssignLiteral("xMaxYMax");
      break;
    default:
      NS_NOTREACHED("Unknown value for mAlign");
  }

  

  if (mAlign != SVG_PRESERVEASPECTRATIO_NONE) {
    switch (mMeetOrSlice) {
      case SVG_MEETORSLICE_MEET:
        aValue.AppendLiteral(" meet");
        break;
      case SVG_MEETORSLICE_SLICE:
        aValue.AppendLiteral(" slice");
        break;
      default:
        NS_NOTREACHED("Unknown value for mMeetOrSlice");
    }
  }

  return NS_OK;
}





NS_IMETHODIMP nsSVGPreserveAspectRatio::GetAlign(PRUint16 *aAlign)
{
  *aAlign = mAlign;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPreserveAspectRatio::SetAlign(PRUint16 aAlign)
{
  if (aAlign < SVG_PRESERVEASPECTRATIO_NONE ||
      aAlign > SVG_PRESERVEASPECTRATIO_XMAXYMAX)
    return NS_ERROR_FAILURE;

  WillModify();
  mAlign = aAlign;
  DidModify();

  return NS_OK;
}


NS_IMETHODIMP nsSVGPreserveAspectRatio::GetMeetOrSlice(PRUint16 *aMeetOrSlice)
{
  *aMeetOrSlice = mMeetOrSlice;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPreserveAspectRatio::SetMeetOrSlice(PRUint16 aMeetOrSlice)
{
  if (aMeetOrSlice < SVG_MEETORSLICE_MEET ||
      aMeetOrSlice > SVG_MEETORSLICE_SLICE)
    return NS_ERROR_FAILURE;

  WillModify();
  mMeetOrSlice = aMeetOrSlice;
  DidModify();

  return NS_OK;
}





nsresult
NS_NewSVGPreserveAspectRatio(nsIDOMSVGPreserveAspectRatio** result,
                             PRUint16 aAlign, PRUint16 aMeetOrSlice)
{
  *result = (nsIDOMSVGPreserveAspectRatio*) new nsSVGPreserveAspectRatio(aAlign, aMeetOrSlice);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*result);
  return NS_OK;
}
