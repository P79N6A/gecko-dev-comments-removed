




#include "nsSVGPathGeometryElement.h"




NS_IMPL_ADDREF_INHERITED(nsSVGPathGeometryElement, nsSVGPathGeometryElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPathGeometryElement, nsSVGPathGeometryElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPathGeometryElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTests)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPathGeometryElementBase)




nsSVGPathGeometryElement::nsSVGPathGeometryElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPathGeometryElementBase(aNodeInfo)
{
}

bool
nsSVGPathGeometryElement::AttributeDefinesGeometry(const nsIAtom *aName)
{
  
  LengthAttributesInfo info = GetLengthInfo();
  for (PRUint32 i = 0; i < info.mLengthCount; i++) {
    if (aName == *info.mLengthInfo[i].mName) {
      return true;
    }
  }

  return false;
}

bool
nsSVGPathGeometryElement::IsMarkable()
{
  return false;
}

void
nsSVGPathGeometryElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
}

already_AddRefed<gfxFlattenedPath>
nsSVGPathGeometryElement::GetFlattenedPath(const gfxMatrix &aMatrix)
{
  return nullptr;
}
