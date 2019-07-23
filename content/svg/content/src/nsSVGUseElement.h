



































#ifndef __NS_SVGUSEELEMENT_H__
#define __NS_SVGUSEELEMENT_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGUseElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsTArray.h"

class nsIContent;
class nsINodeInfo;

#define NS_SVG_USE_ELEMENT_IMPL_CID \
{ 0xa95c13d3, 0xc193, 0x465f, {0x81, 0xf0, 0x02, 0x6d, 0x67, 0x05, 0x54, 0x58 } }

nsresult
NS_NewSVGSVGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);

typedef nsSVGGraphicElement nsSVGUseElementBase;

class nsSVGUseElement : public nsSVGUseElementBase,
                        public nsIDOMSVGURIReference,
                        public nsIDOMSVGUseElement,
                        public nsStubMutationObserver
{
  friend class nsSVGUseFrame;
protected:
  friend nsresult NS_NewSVGUseElement(nsIContent **aResult,
                                      nsINodeInfo *aNodeInfo);
  nsSVGUseElement(nsINodeInfo *aNodeInfo);
  virtual ~nsSVGUseElement();
  
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_USE_ELEMENT_IMPL_CID)

  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSVGUseElement, nsSVGUseElementBase)
  NS_DECL_NSIDOMSVGUSEELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  
  NS_FORWARD_NSIDOMNODE(nsSVGUseElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGUseElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGUseElementBase::)

  
  nsIContent* CreateAnonymousContent();
  void DestroyAnonymousContent();

  
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidChangeString(PRUint8 aAttrEnum, PRBool aDoSetAttr);

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual StringAttributesInfo GetStringInfo();

  void SyncWidthHeight(PRUint8 aAttrEnum);
  nsIContent *LookupHref();
  void TriggerReclone();
  void RemoveListener();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  nsCOMPtr<nsIContent> mOriginal; 
  nsCOMPtr<nsIContent> mClone;    
  nsCOMPtr<nsIContent> mSourceContent;  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGUseElement, NS_SVG_USE_ELEMENT_IMPL_CID)

#endif
