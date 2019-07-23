



































#include "nsSVGPathGeometryElement.h"

nsSVGPathGeometryElement::nsSVGPathGeometryElement(nsINodeInfo *aNodeInfo)
  : nsSVGPathGeometryElementBase(aNodeInfo)
{
}

PRBool
nsSVGPathGeometryElement::IsDependentAttribute(nsIAtom *aName)
{
  
  LengthAttributesInfo info = GetLengthInfo();
  for (PRUint32 i = 0; i < info.mLengthCount; i++) {
    if (aName == *info.mLengthInfo[i].mName) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRBool
nsSVGPathGeometryElement::IsMarkable()
{
  return PR_FALSE;
}

void
nsSVGPathGeometryElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
}

nsSVGFlattenedPath *
nsSVGPathGeometryElement::GetFlattenedPath(nsIDOMSVGMatrix *aMatrix)
{
  return nsnull;
}
