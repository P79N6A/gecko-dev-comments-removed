





































#ifndef __NS_ISVGGLYPHFRAGMENTLEAF_H__
#define __NS_ISVGGLYPHFRAGMENTLEAF_H__

#include "nsISVGGlyphFragmentNode.h"
#include "nsIDOMSVGLengthList.h"

class nsIDOMSVGPoint;
class nsIDOMSVGRect;
class nsSVGTextPathFrame;

class nsISVGGlyphFragmentLeaf : public nsISVGGlyphFragmentNode
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISVGGlyphFragmentLeaf)

  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)=0;
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval)=0;

  NS_IMETHOD_(float) GetAdvance(PRBool aForceGlobalTransform)=0;

  NS_IMETHOD_(void) SetGlyphPosition(gfxPoint *aPosition, PRBool aForceGlobalTransform)=0;
  NS_IMETHOD_(nsSVGTextPathFrame*) FindTextPathParent()=0;
  NS_IMETHOD_(PRBool) IsStartOfChunk()=0; 

  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetX()=0;
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetY()=0;
  NS_IMETHOD_(PRUint16) GetTextAnchor()=0;
  NS_IMETHOD_(PRBool) IsAbsolutelyPositioned()=0;
};

#endif 
