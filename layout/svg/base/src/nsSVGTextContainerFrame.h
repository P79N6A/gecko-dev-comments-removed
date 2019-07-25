



































#ifndef NS_SVGTEXTCONTAINERFRAME_H
#define NS_SVGTEXTCONTAINERFRAME_H

#include "nsSVGContainerFrame.h"
#include "nsIDOMSVGNumberList.h"

class nsISVGGlyphFragmentNode;
class nsISVGGlyphFragmentLeaf;
class nsSVGTextFrame;
namespace mozilla {
class SVGUserUnitList;
}

class nsSVGTextContainerFrame : public nsSVGDisplayContainerFrame
{
public:
  nsSVGTextContainerFrame(nsStyleContext* aContext) :
    nsSVGDisplayContainerFrame(aContext) {}

  void NotifyGlyphMetricsChange();
  virtual void GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY);
  virtual void GetDxDy(mozilla::SVGUserUnitList *aDx, mozilla::SVGUserUnitList *aDy);
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
  void GetEffectiveXY(nsTArray<float> &aX, nsTArray<float> &aY);
  void GetEffectiveDxDy(nsTArray<float> &aDx, nsTArray<float> &aDy);
  void GetEffectiveRotate(nsTArray<float> &aRotate);

protected:
  


  nsISVGGlyphFragmentNode *
  GetFirstGlyphFragmentChildNode();

  


  nsISVGGlyphFragmentNode *
  GetNextGlyphFragmentChildNode(nsISVGGlyphFragmentNode *node);

  


  void SetWhitespaceHandling();
  void CopyPositionList(nsTArray<float> *parentList,
                        mozilla::SVGUserUnitList *selfList,
                        nsTArray<float> &dstList,
                        PRUint32 aOffset);
  void CopyRotateList(nsTArray<float> *parentList,
                      nsCOMPtr<nsIDOMSVGNumberList> selfList,
                      nsTArray<float> &dstList,
                      PRUint32 aOffset);
  PRUint32 BuildPositionList(PRUint32 aOffset, PRUint32 aDepth);

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
