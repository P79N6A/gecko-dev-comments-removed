






































#ifndef __NS_ISVGGLYPHFRAGMENTNODE_H__
#define __NS_ISVGGLYPHFRAGMENTNODE_H__

#include "nsQueryFrame.h"

class nsISVGGlyphFragmentLeaf;
class nsIDOMSVGPoint;

#define PRESERVE_WHITESPACE       0x00
#define COMPRESS_WHITESPACE       0x01
#define TRIM_LEADING_WHITESPACE   0x02
#define TRIM_TRAILING_WHITESPACE  0x04

class nsISVGGlyphFragmentNode : public nsQueryFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsISVGGlyphFragmentNode)

  virtual PRUint32 GetNumberOfChars()=0;
  virtual float GetComputedTextLength()=0;
  virtual float GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars)=0;
  virtual PRInt32 GetCharNumAtPosition(nsIDOMSVGPoint *point)=0;
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment()=0;
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment()=0;
  NS_IMETHOD_(void) SetWhitespaceHandling(PRUint8 aWhitespaceHandling)=0;
};

#endif 
