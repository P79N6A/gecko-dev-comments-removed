





































#ifndef __NS_SVGPATHELEMENT_H__
#define __NS_SVGPATHELEMENT_H__

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGPathElement.h"
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsSVGNumber2.h"
#include "SVGAnimatedPathSegList.h"
#include "gfxPath.h"

class gfxContext;

typedef nsSVGPathGeometryElement nsSVGPathElementBase;

class nsSVGPathElement : public nsSVGPathElementBase,
                         public nsIDOMSVGPathElement,
                         public nsIDOMSVGAnimatedPathData
{
friend class nsSVGPathFrame;

protected:
  friend nsresult NS_NewSVGPathElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGPathElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  typedef mozilla::SVGAnimatedPathSegList SVGAnimatedPathSegList;
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPATHELEMENT
  NS_DECL_NSIDOMSVGANIMATEDPATHDATA

  
  NS_FORWARD_NSIDOMNODE(nsSVGPathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGPathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPathElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual PRBool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual PRBool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(const gfxMatrix &aMatrix);

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual SVGAnimatedPathSegList* GetAnimPathSegList() {
    return &mD;
  }

  virtual nsIAtom* GetPathDataAttrName() const {
    return nsGkAtoms::d;
  }

  gfxFloat GetScale();

protected:

  
  virtual NumberAttributesInfo GetNumberInfo();

  SVGAnimatedPathSegList mD;
  nsSVGNumber2 mPathLength;
  static NumberInfo sNumberInfo;
};

#endif
