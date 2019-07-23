



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsISupports.h"
#include "nsRect.h"

class gfxContext;
class nsISVGChildFrame;
class nsIURI;
class nsIContent;
class nsIFrame;
class nsSVGRenderState;

#define NS_ISVGFILTERFRAME_IID \
{ 0x85c081f4, 0x63d4, 0x4751, { 0x87, 0xae, 0x6a, 0x99, 0x81, 0x2f, 0xa7, 0xa3 } }

class nsISVGFilterFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGFILTERFRAME_IID)

  NS_IMETHOD FilterPaint(nsSVGRenderState *aContext,
                         nsISVGChildFrame *aTarget) = 0;

  NS_IMETHOD_(nsRect) GetInvalidationRegion(nsIFrame *aTarget) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGFilterFrame, NS_ISVGFILTERFRAME_IID)

nsresult
NS_GetSVGFilterFrame(nsISVGFilterFrame **aResult,
                     nsIURI *aURI,
                     nsIContent *aContent);

#endif
