




#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"




nsSVGPathGeometryElement::nsSVGPathGeometryElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPathGeometryElementBase(aNodeInfo)
{
}

bool
nsSVGPathGeometryElement::AttributeDefinesGeometry(const nsIAtom *aName)
{
  
  LengthAttributesInfo info = GetLengthInfo();
  for (uint32_t i = 0; i < info.mLengthCount; i++) {
    if (aName == *info.mLengthInfo[i].mName) {
      return true;
    }
  }

  return false;
}

bool
nsSVGPathGeometryElement::GeometryDependsOnCoordCtx()
{
  
  LengthAttributesInfo info = const_cast<nsSVGPathGeometryElement*>(this)->GetLengthInfo();
  for (uint32_t i = 0; i < info.mLengthCount; i++) {
    if (info.mLengths[i].GetSpecifiedUnitType() == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
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

already_AddRefed<gfxPath>
nsSVGPathGeometryElement::GetPath(const gfxMatrix &aMatrix)
{
  return nullptr;
}
