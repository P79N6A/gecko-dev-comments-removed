




#ifndef NS_SVGTEXTCONTAINERFRAME_H
#define NS_SVGTEXTCONTAINERFRAME_H

#include "mozilla/Attributes.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsQueryFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsTArray.h"

class nsFrameList;
class nsISVGGlyphFragmentNode;
class nsStyleContext;
class nsSVGGlyphFrame;
class nsSVGTextFrame;

namespace mozilla {
class nsISVGPoint;

namespace dom {
class SVGIRect;
}
}

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

  
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD RemoveFrame(ChildListID aListID, nsIFrame *aOldFrame) MOZ_OVERRIDE;

  NS_IMETHOD GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval);
  NS_IMETHOD GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval);
  NS_IMETHOD GetExtentOfChar(uint32_t charnum, mozilla::dom::SVGIRect **_retval);
  NS_IMETHOD GetRotationOfChar(uint32_t charnum, float *_retval);

  


  virtual uint32_t GetNumberOfChars();

  


  virtual float GetComputedTextLength();

  


  virtual float GetSubStringLength(uint32_t charnum, uint32_t nchars);

  


  virtual int32_t GetCharNumAtPosition(mozilla::nsISVGPoint *point);
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
                        uint32_t aOffset);
  void CopyRotateList(nsTArray<float> *parentList,
                      const SVGNumberList *selfList,
                      nsTArray<float> &dstList,
                      uint32_t aOffset);
  uint32_t BuildPositionList(uint32_t aOffset, uint32_t aDepth);

  void SetWhitespaceCompression();
private:
  


  static nsSVGGlyphFrame *
  GetGlyphFrameAtCharNum(nsISVGGlyphFragmentNode* node,
                         uint32_t charnum,
                         uint32_t *offset);

  



  nsSVGTextFrame * GetTextFrame();
  nsTArray<float> mX;
  nsTArray<float> mY;
  nsTArray<float> mDx;
  nsTArray<float> mDy;
  nsTArray<float> mRotate;
};

#endif
