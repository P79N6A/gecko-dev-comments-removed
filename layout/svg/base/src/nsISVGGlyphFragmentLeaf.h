





































#ifndef __NS_ISVGGLYPHFRAGMENTLEAF_H__
#define __NS_ISVGGLYPHFRAGMENTLEAF_H__

#include "nsISVGGlyphFragmentNode.h"
#include "nsIDOMSVGLengthList.h"

class nsIDOMSVGPoint;
class nsIDOMSVGRect;
class nsSVGTextPathFrame;


#define NS_ISVGGLYPHFRAGMENTLEAF_IID \
{ 0x2c466aed, 0xcf7b, 0x4479, { 0xa8, 0x7, 0x98, 0x15, 0x12, 0x15, 0xa6, 0x45 } }

class nsISVGGlyphFragmentLeaf : public nsISVGGlyphFragmentNode
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGGLYPHFRAGMENTLEAF_IID)

  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)=0;
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)=0;
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval)=0;

  enum { BASELINE_ALPHABETIC = 0U };
  enum { BASELINE_HANGING = 1U };
  enum { BASELINE_IDEOGRAPHC = 2U };
  enum { BASELINE_MATHEMATICAL = 3U };
  enum { BASELINE_CENTRAL = 4U };
  enum { BASELINE_MIDDLE = 5U };
  enum { BASELINE_TEXT_BEFORE_EDGE = 6U };
  enum { BASELINE_TEXT_AFTER_EDGE = 7U };

  NS_IMETHOD_(float) GetBaselineOffset(PRUint16 baselineIdentifier)=0;
  NS_IMETHOD_(float) GetAdvance()=0;

  NS_IMETHOD_(void) SetGlyphPosition(float x, float y)=0;
  NS_IMETHOD_(nsSVGTextPathFrame*) FindTextPathParent()=0;
  NS_IMETHOD_(PRBool) IsStartOfChunk()=0; 
  NS_IMETHOD_(void) GetAdjustedPosition( float &x,  float &y)=0;

  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetX()=0;
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetY()=0;
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDx()=0;
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDy()=0;
  NS_IMETHOD_(PRUint16) GetTextAnchor()=0;
  NS_IMETHOD_(PRBool) IsAbsolutelyPositioned()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGGlyphFragmentLeaf,
                              NS_ISVGGLYPHFRAGMENTLEAF_IID)

#endif 
