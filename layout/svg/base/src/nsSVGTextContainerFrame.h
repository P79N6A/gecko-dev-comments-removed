



































#ifndef NS_SVGTEXTCONTAINERFRAME_H
#define NS_SVGTEXTCONTAINERFRAME_H

#include "nsSVGContainerFrame.h"
#include "nsIDOMSVGLengthList.h"

class nsISVGGlyphFragmentNode;
class nsISVGGlyphFragmentLeaf;

class nsSVGTextFrame;

class nsSVGTextContainerFrame : public nsSVGDisplayContainerFrame
{
public:
  nsSVGTextContainerFrame(nsStyleContext* aContext) :
    nsSVGDisplayContainerFrame(aContext) {}

  void NotifyGlyphMetricsChange();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetX();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetY();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDx();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDy();
  
public:
  NS_DECL_QUERYFRAME
  NS_DECLARE_FRAME_ACCESSOR(nsSVGTextContainerFrame)

  
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom *aListName, nsIFrame *aOldFrame);

  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval);

  


  virtual PRUint32 GetNumberOfChars();

  


  virtual float GetComputedTextLength();

  


  virtual float GetSubStringLength(PRUint32 charnum, PRUint32 nchars);

  


  virtual PRInt32 GetCharNumAtPosition(nsIDOMSVGPoint *point);

protected:
  


  nsISVGGlyphFragmentNode *
  GetFirstGlyphFragmentChildNode();

  


  nsISVGGlyphFragmentNode *
  GetNextGlyphFragmentChildNode(nsISVGGlyphFragmentNode *node);

  


  void SetWhitespaceHandling();

private:
  


  static nsISVGGlyphFragmentLeaf *
  GetGlyphFragmentAtCharNum(nsISVGGlyphFragmentNode* node,
                            PRUint32 charnum,
                            PRUint32 *offset);

  



  nsSVGTextFrame * GetTextFrame();
};

#endif
