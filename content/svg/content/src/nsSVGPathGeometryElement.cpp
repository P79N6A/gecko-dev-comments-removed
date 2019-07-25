



































#include "nsSVGPathGeometryElement.h"

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
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

bool
nsSVGPathGeometryElement::IsMarkable()
{
  return PR_FALSE;
}

void
nsSVGPathGeometryElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
}

already_AddRefed<gfxFlattenedPath>
nsSVGPathGeometryElement::GetFlattenedPath(const gfxMatrix &aMatrix)
{
  return nsnull;
}
