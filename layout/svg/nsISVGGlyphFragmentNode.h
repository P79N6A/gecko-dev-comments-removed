





#ifndef __NS_ISVGGLYPHFRAGMENTNODE_H__
#define __NS_ISVGGLYPHFRAGMENTNODE_H__

#include "nsQueryFrame.h"

class nsSVGGlyphFrame;

namespace mozilla {
class nsISVGPoint;
}

class nsISVGGlyphFragmentNode : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsISVGGlyphFragmentNode)

  virtual uint32_t GetNumberOfChars()=0;
  virtual float GetComputedTextLength()=0;
  virtual float GetSubStringLength(uint32_t charnum, uint32_t fragmentChars)=0;
  virtual int32_t GetCharNumAtPosition(mozilla::nsISVGPoint *point)=0;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetFirstGlyphFrame()=0;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetNextGlyphFrame()=0;
  NS_IMETHOD_(void) SetWhitespaceCompression(bool aCompressWhitespace)=0;
};

#endif 
