




#ifndef mozilla_dom_SVGPathElement_h
#define mozilla_dom_SVGPathElement_h

#include "nsIDOMSVGAnimatedPathData.h"
#include "nsIDOMSVGPathElement.h"
#include "nsSVGNumber2.h"
#include "nsSVGPathGeometryElement.h"
#include "SVGAnimatedPathSegList.h"

nsresult NS_NewSVGPathElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

class gfxContext;

typedef nsSVGPathGeometryElement SVGPathElementBase;

namespace mozilla {
namespace dom {

class SVGPathElement MOZ_FINAL : public SVGPathElementBase,
                                 public nsIDOMSVGPathElement,
                                 public nsIDOMSVGAnimatedPathData
{
friend class nsSVGPathFrame;

protected:
  friend nsresult (::NS_NewSVGPathElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGPathElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  typedef SVGAnimatedPathSegList SVGAnimatedPathSegList;
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPATHELEMENT
  NS_DECL_NSIDOMSVGANIMATEDPATHDATA

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGPathElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual bool HasValidDimensions() const;

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(const gfxMatrix &aMatrix);

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual SVGAnimatedPathSegList* GetAnimPathSegList() {
    return &mD;
  }

  virtual nsIAtom* GetPathDataAttrName() const {
    return nsGkAtoms::d;
  }

  enum PathLengthScaleForType {
    eForTextPath,
    eForStroking
  };

  




  gfxFloat GetPathLengthScale(PathLengthScaleForType aFor);

protected:

  
  virtual NumberAttributesInfo GetNumberInfo();

  SVGAnimatedPathSegList mD;
  nsSVGNumber2 mPathLength;
  static NumberInfo sNumberInfo;
};

} 
} 

#endif 
