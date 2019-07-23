






































#ifndef nsHTMLContainerFrame_h___
#define nsHTMLContainerFrame_h___

#include "nsContainerFrame.h"
#include "gfxPoint.h"
class nsString;
class nsAbsoluteFrame;
class nsPlaceholderFrame;
struct nsStyleDisplay;
struct nsStylePosition;
struct nsHTMLReflowMetrics;
struct nsHTMLReflowState;
class nsLineBox;



#ifdef DEBUG
#define CRAZY_W 500000


#define CRAZY_H 22500000

#define CRAZY_WIDTH(_x) (((_x) < -CRAZY_W) || ((_x) > CRAZY_W))
#define CRAZY_HEIGHT(_y) (((_y) < -CRAZY_H) || ((_y) > CRAZY_H))
#endif

class nsDisplayTextDecoration;



class nsHTMLContainerFrame : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  



  static nsresult CreateViewForFrame(nsIFrame* aFrame,
                                     PRBool aForce);

  static nsresult ReparentFrameView(nsPresContext* aPresContext,
                                    nsIFrame*       aChildFrame,
                                    nsIFrame*       aOldParentFrame,
                                    nsIFrame*       aNewParentFrame);

  static nsresult ReparentFrameViewList(nsPresContext*     aPresContext,
                                        const nsFrameList& aChildFrameList,
                                        nsIFrame*          aOldParentFrame,
                                        nsIFrame*          aNewParentFrame);

  












  nsresult CreateNextInFlow(nsPresContext* aPresContext,
                            nsIFrame*       aFrame,
                            nsIFrame*&      aNextInFlowResult);

  




  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
                              
  nsresult DisplayTextDecorations(nsDisplayListBuilder* aBuilder,
                                  nsDisplayList* aBelowTextDecorations,
                                  nsDisplayList* aAboveTextDecorations,
                                  nsLineBox* aLine);

protected:
  nsHTMLContainerFrame(nsStyleContext *aContext) : nsContainerFrame(aContext) {}

  





  nsresult DisplayTextDecorationsAndChildren(nsDisplayListBuilder* aBuilder, 
                                             const nsRect& aDirtyRect,
                                             const nsDisplayListSet& aLists);

  















  void GetTextDecorations(nsPresContext* aPresContext, 
                          PRBool aIsBlock,
                          PRUint8& aDecorations, 
                          nscolor& aUnderColor, 
                          nscolor& aOverColor, 
                          nscolor& aStrikeColor);

  

















  virtual void PaintTextDecorationLine(gfxContext* aCtx,
                                       const nsPoint& aPt,
                                       nsLineBox* aLine,
                                       nscolor aColor,
                                       gfxFloat aOffset,
                                       gfxFloat aAscent,
                                       gfxFloat aSize,
                                       const PRUint8 aDecoration);

  virtual void AdjustForTextIndent(const nsLineBox* aLine,
                                   nscoord& start,
                                   nscoord& width);

  friend class nsDisplayTextDecoration;
  friend class nsDisplayTextShadow;
};

#endif 
