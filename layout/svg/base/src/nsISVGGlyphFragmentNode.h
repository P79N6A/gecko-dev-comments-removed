






































#ifndef __NS_ISVGGLYPHFRAGMENTNODE_H__
#define __NS_ISVGGLYPHFRAGMENTNODE_H__

#include "nsISupports.h"

class nsISVGGlyphFragmentLeaf;
class nsIDOMSVGPoint;

#define PRESERVE_WHITESPACE       0x00
#define COMPRESS_WHITESPACE       0x01
#define TRIM_LEADING_WHITESPACE   0x02
#define TRIM_TRAILING_WHITESPACE  0x04

#define NS_ISVGGLYPHFRAGMENTNODE_IID \
{ 0x1297716a, 0xd68d, 0x4c9d, { 0x8e, 0xf8, 0x9e, 0x01, 0x1d, 0x78, 0x21, 0xd0 } }

class nsISVGGlyphFragmentNode : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGGLYPHFRAGMENTNODE_IID)

  NS_IMETHOD_(PRUint32) GetNumberOfChars()=0;
  NS_IMETHOD_(float) GetComputedTextLength()=0;
  NS_IMETHOD_(float) GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars)=0;
  NS_IMETHOD_(PRInt32) GetCharNumAtPosition(nsIDOMSVGPoint *point)=0;
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment()=0;
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment()=0;
  NS_IMETHOD_(void) SetWhitespaceHandling(PRUint8 aWhitespaceHandling)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGGlyphFragmentNode,
                              NS_ISVGGLYPHFRAGMENTNODE_IID)

#endif 
