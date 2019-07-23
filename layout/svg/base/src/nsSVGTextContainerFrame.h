



































#ifndef NS_SVGTEXTCONTAINERFRAME_H
#define NS_SVGTEXTCONTAINERFRAME_H

#include "nsSVGContainerFrame.h"
#include "nsIDOMSVGLengthList.h"
#include "nsIDOMSVGNumberList.h"

class nsISVGGlyphFragmentNode;
class nsISVGGlyphFragmentLeaf;

class nsSVGTextFrame;

class nsSVGTextContainerFrame : public nsSVGDisplayContainerFrame
{
public:
  nsSVGTextContainerFrame(nsStyleContext* aContext) :
    nsSVGDisplayContainerFrame(aContext) {}

  void NotifyGlyphMetricsChange();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetX();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetY();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetDx();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetDy();
  virtual already_AddRefed<nsIDOMSVGNumberList> GetRotate();
  
public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGTextContainerFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
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
