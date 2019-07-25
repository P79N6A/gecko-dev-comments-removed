



































#ifndef NS_SVGTEXTCONTAINERFRAME_H
#define NS_SVGTEXTCONTAINERFRAME_H

#include "nsSVGContainerFrame.h"

class nsISVGGlyphFragmentNode;
class nsISVGGlyphFragmentLeaf;
class nsSVGTextFrame;

class nsSVGTextContainerFrame : public nsSVGDisplayContainerFrame
{
protected:
  nsSVGTextContainerFrame(nsStyleContext* aContext) :
    nsSVGDisplayContainerFrame(aContext) {}

public:
  void NotifyGlyphMetricsChange();
  virtual void GetXY(SVGUserUnitList *aX, SVGUserUnitList *aY);
  virtual void GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy);
  virtual const SVGNumberList *GetRotate();
  
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
  void GetEffectiveXY(nsTArray<float> &aX, nsTArray<float> &aY);
  void GetEffectiveDxDy(nsTArray<float> &aDx, nsTArray<float> &aDy);
  void GetEffectiveRotate(nsTArray<float> &aRotate);

protected:
  


  nsISVGGlyphFragmentNode *
  GetFirstGlyphFragmentChildNode();

  


  nsISVGGlyphFragmentNode *
  GetNextGlyphFragmentChildNode(nsISVGGlyphFragmentNode *node);

  void CopyPositionList(nsTArray<float> *parentList,
                        SVGUserUnitList *selfList,
                        nsTArray<float> &dstList,
                        PRUint32 aOffset);
  void CopyRotateList(nsTArray<float> *parentList,
                      const SVGNumberList *selfList,
                      nsTArray<float> &dstList,
                      PRUint32 aOffset);
  PRUint32 BuildPositionList(PRUint32 aOffset, PRUint32 aDepth);

  void SetWhitespaceCompression();
private:
  


  static nsISVGGlyphFragmentLeaf *
  GetGlyphFragmentAtCharNum(nsISVGGlyphFragmentNode* node,
                            PRUint32 charnum,
                            PRUint32 *offset);

  



  nsSVGTextFrame * GetTextFrame();
  nsTArray<float> mX;
  nsTArray<float> mY;
  nsTArray<float> mDx;
  nsTArray<float> mDy;
  nsTArray<float> mRotate;
};

#endif
