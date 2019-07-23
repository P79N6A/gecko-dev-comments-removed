



































#include "nsSVGPathGeometryElement.h"
#include "nsCOMPtr.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGPointList.h"
#include "nsIDOMSVGAnimatedPoints.h"
#include "nsSVGUtils.h"

typedef nsSVGPathGeometryElement nsSVGPolyElementBase;

class nsSVGPolyElement : public nsSVGPolyElementBase,
                         public nsIDOMSVGAnimatedPoints
{
protected:
  nsSVGPolyElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATEDPOINTS

  
  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;
  
  
  virtual PRBool IsDependentAttribute(nsIAtom *aName);
  virtual PRBool IsMarkable() { return PR_TRUE; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(cairo_t *aCtx);

protected:
  nsCOMPtr<nsIDOMSVGPointList> mPoints;

};

