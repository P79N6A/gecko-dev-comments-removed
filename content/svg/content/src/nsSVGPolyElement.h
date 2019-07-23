



































#include "nsSVGPathGeometryElement.h"
#include "nsCOMPtr.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGPointList.h"
#include "nsIDOMSVGAnimatedPoints.h"
#include "nsSVGUtils.h"

typedef nsSVGPathGeometryElement nsSVGPolyElementBase;

class gfxContext;

class nsSVGPolyElement : public nsSVGPolyElementBase,
                         public nsIDOMSVGAnimatedPoints
{
protected:
  nsSVGPolyElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATEDPOINTS

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* name) const;
  
  
  virtual PRBool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual PRBool IsMarkable() { return PR_TRUE; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

protected:
  nsCOMPtr<nsIDOMSVGPointList> mPoints;

};

