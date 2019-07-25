





































#ifndef __NS_ISVGGLYPHFRAGMENTLEAF_H__
#define __NS_ISVGGLYPHFRAGMENTLEAF_H__

#include "nsISVGGlyphFragmentNode.h"

class nsIDOMSVGPoint;
class nsIDOMSVGRect;
class nsSVGTextPathFrame;
namespace mozilla {
class SVGUserUnitList;
}

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

  NS_IMETHOD_(void) GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY)=0;
  NS_IMETHOD_(void) SetStartIndex(PRUint32 aStartIndex)=0;
  



  NS_IMETHOD_(void) GetEffectiveXY(PRInt32 strLength,
                                   nsTArray<float> &aX, nsTArray<float> &aY)=0;
  



  NS_IMETHOD_(void) GetEffectiveDxDy(PRInt32 strLength, 
                                     nsTArray<float> &aDx,
                                     nsTArray<float> &aDy)=0;
  



  NS_IMETHOD_(void) GetEffectiveRotate(PRInt32 strLength,
                                       nsTArray<float> &aRotate)=0;
  NS_IMETHOD_(PRUint16) GetTextAnchor()=0;
  NS_IMETHOD_(PRBool) IsAbsolutelyPositioned()=0;
  NS_IMETHOD_(PRBool) IsTextEmpty() const=0;
  NS_IMETHOD_(void) SetTrimLeadingWhitespace(PRBool aTrimLeadingWhitespace)=0;
  NS_IMETHOD_(void) SetTrimTrailingWhitespace(PRBool aTrimTrailingWhitespace)=0;
  NS_IMETHOD_(PRBool) EndsWithWhitespace() const=0;
  NS_IMETHOD_(PRBool) IsAllWhitespace() const=0;
};

#endif 
