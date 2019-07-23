





































#ifndef __NS_SVGPATTERNELEMENT_H__
#define __NS_SVGPATTERNELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGPatternElement.h"
#include "nsSVGLength2.h"
#include "nsStubMutationObserver.h"



typedef nsSVGStylableElement nsSVGPatternElementBase;

class nsSVGPatternElement : public nsSVGPatternElementBase,
                            public nsIDOMSVGURIReference,
                            public nsIDOMSVGFitToViewBox,
                            public nsIDOMSVGPatternElement,
                            public nsStubMutationObserver
{
  friend class nsSVGPatternFrame;

protected:
  friend nsresult NS_NewSVGPatternElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGPatternElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGPATTERNELEMENT

  
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_DECL_NSIDOMSVGFITTOVIEWBOX

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  void PushUpdate();

  virtual LengthAttributesInfo GetLengthInfo();
  
  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mPatternUnits;
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mPatternContentUnits;
  nsCOMPtr<nsIDOMSVGAnimatedTransformList> mPatternTransform;

  
  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;

  
  nsCOMPtr<nsIDOMSVGAnimatedRect> mViewBox;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;
};

#endif
