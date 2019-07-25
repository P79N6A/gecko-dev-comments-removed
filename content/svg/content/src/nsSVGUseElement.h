



































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
#include "nsReferencedElement.h"
#include "mozilla/dom/FromParser.h"

class nsIContent;
class nsINodeInfo;

#define NS_SVG_USE_ELEMENT_IMPL_CID \
{ 0x55fb86fe, 0xd81f, 0x4ae4, \
  { 0x80, 0x3f, 0xeb, 0x90, 0xfe, 0xe0, 0x7a, 0xe9 } }

nsresult
NS_NewSVGSVGElement(nsIContent **aResult,
                    already_AddRefed<nsINodeInfo> aNodeInfo,
                    mozilla::dom::FromParser aFromParser);

typedef nsSVGGraphicElement nsSVGUseElementBase;

class nsSVGUseElement : public nsSVGUseElementBase,
                        public nsIDOMSVGURIReference,
                        public nsIDOMSVGUseElement,
                        public nsStubMutationObserver
{
  friend class nsSVGUseFrame;
protected:
  friend nsresult NS_NewSVGUseElement(nsIContent **aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGUseElement(already_AddRefed<nsINodeInfo> aNodeInfo);
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
  nsIContent* GetAnonymousContent() const { return mClone; }
  void DestroyAnonymousContent();

  
  virtual gfxMatrix PrependLocalTransformTo(const gfxMatrix &aMatrix) const;
  virtual void DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr);
  virtual void DidAnimateLength(PRUint8 aAttrEnum);
  virtual void DidChangeString(PRUint8 aAttrEnum);
  virtual void DidAnimateString(PRUint8 aAttrEnum);

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  class SourceReference : public nsReferencedElement {
  public:
    SourceReference(nsSVGUseElement* aContainer) : mContainer(aContainer) {}
  protected:
    virtual void ElementChanged(Element* aFrom, Element* aTo) {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      if (aFrom) {
        aFrom->RemoveMutationObserver(mContainer);
      }
      mContainer->TriggerReclone();
    }
  private:
    nsSVGUseElement* mContainer;
  };

  virtual LengthAttributesInfo GetLengthInfo();
  virtual StringAttributesInfo GetStringInfo();

  PRBool HasValidDimensions();
  void SyncWidthHeight(PRUint8 aAttrEnum);
  void LookupHref();
  void TriggerReclone();
  void UnlinkSource();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  nsCOMPtr<nsIContent> mOriginal; 
  nsCOMPtr<nsIContent> mClone;    
  SourceReference      mSource;   
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGUseElement, NS_SVG_USE_ELEMENT_IMPL_CID)

#endif
