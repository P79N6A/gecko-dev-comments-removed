






































#ifndef nsHTMLFrameset_h___
#define nsHTMLFrameset_h___

#include "nsGkAtoms.h"
#include "nsHTMLContainerFrame.h"
#include "nsColor.h"
#include "nsIObserver.h"
#include "nsWeakPtr.h"
#include "nsIFrameSetElement.h"

class  nsIContent;
class  nsIFrame;
class  nsPresContext;
class  nsIRenderingContext;
struct nsRect;
struct nsHTMLReflowState;
struct nsSize;
class  nsIAtom;
class  nsHTMLFramesetBorderFrame;
class  nsGUIEvent;
class  nsHTMLFramesetFrame;

#define NS_IFRAMESETFRAME_IID \
{ 0xf47deac0, 0x4200, 0x11d2, \
  { 0x80, 0x3c, 0x0, 0x60, 0x8, 0x15, 0xa7, 0x91 } }

#define NO_COLOR 0xFFFFFFFA

struct nsBorderColor 
{
  nscolor mLeft;
  nscolor mRight;
  nscolor mTop;
  nscolor mBottom;

  nsBorderColor() { Set(NO_COLOR); }
  ~nsBorderColor() {}
  void Set(nscolor aColor) { mLeft = mRight = mTop = mBottom = aColor; }
};

enum nsFrameborder {
  eFrameborder_Yes = 0,
  eFrameborder_No,
  eFrameborder_Notset
};

struct nsFramesetDrag {
  PRBool               mVertical;  
  PRInt32              mIndex;     
  PRInt32              mChange;    
  nsHTMLFramesetFrame* mSource;    
  PRBool               mActive;

  nsFramesetDrag();
  nsFramesetDrag(PRBool               aVertical, 
                 PRInt32              aIndex, 
                 PRInt32              aChange, 
                 nsHTMLFramesetFrame* aSource); 
  void Reset(PRBool               aVertical, 
             PRInt32              aIndex, 
             PRInt32              aChange, 
             nsHTMLFramesetFrame* aSource); 
  void UnSet();
};




class nsHTMLFramesetFrame : public nsHTMLContainerFrame
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMESETFRAME_IID)

  nsHTMLFramesetFrame(nsStyleContext* aContext);

  virtual ~nsHTMLFramesetFrame();

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  static PRBool  gDragInProgress;

  void GetSizeOfChild(nsIFrame* aChild, nsSize& aSize);

  void GetSizeOfChildAt(PRInt32  aIndexInParent, 
                        nsSize&  aSize, 
                        nsPoint& aCellIndex);

  static nsHTMLFramesetFrame* GetFramesetParent(nsIFrame* aChild);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD GetCursor(const nsPoint&    aPoint,
                       nsIFrame::Cursor& aCursor);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsIAtom* GetType() const;
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual PRBool IsLeaf() const;
  
  NS_IMETHOD  VerifyTree() const;

  void StartMouseDrag(nsPresContext*            aPresContext, 
                      nsHTMLFramesetBorderFrame* aBorder, 
                      nsGUIEvent*                aEvent);

  void MouseDrag(nsPresContext* aPresContext, 
                 nsGUIEvent*     aEvent);

  void EndMouseDrag(nsPresContext* aPresContext);

  nsFrameborder GetParentFrameborder() { return mParentFrameborder; }

  void SetParentFrameborder(nsFrameborder aValue) { mParentFrameborder = aValue; }

  nsFramesetDrag& GetDrag() { return mDrag; }

  void RecalculateBorderResize();

protected:
  void Scale(nscoord  aDesired, 
             PRInt32  aNumIndicies, 
             PRInt32* aIndicies, 
             PRInt32  aNumItems,
             PRInt32* aItems);

  void CalculateRowCol(nsPresContext*       aPresContext, 
                       nscoord               aSize, 
                       PRInt32               aNumSpecs, 
                       const nsFramesetSpec* aSpecs, 
                       nscoord*              aValues);

  void GenerateRowCol(nsPresContext*       aPresContext,
                      nscoord               aSize,
                      PRInt32               aNumSpecs,
                      const nsFramesetSpec* aSpecs,
                      nscoord*              aValues,
                      nsString&             aNewAttr);

  virtual void GetDesiredSize(nsPresContext*          aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics&     aDesiredSize);

  PRInt32 GetBorderWidth(nsPresContext* aPresContext,
                         PRBool aTakeForcingIntoAccount);

  PRInt32 GetParentBorderWidth() { return mParentBorderWidth; }

  void SetParentBorderWidth(PRInt32 aWidth) { mParentBorderWidth = aWidth; }

  nscolor GetParentBorderColor() { return mParentBorderColor; }

  void SetParentBorderColor(nscolor aColor) { mParentBorderColor = aColor; }

  nsFrameborder GetFrameBorder();

  nsFrameborder GetFrameBorder(nsIContent* aContent);

  nscolor GetBorderColor();

  nscolor GetBorderColor(nsIContent* aFrameContent);

  PRBool GetNoResize(nsIFrame* aChildFrame); 
  
  virtual PRIntn GetSkipSides() const;

  void ReflowPlaceChild(nsIFrame*                aChild,
                        nsPresContext*          aPresContext,
                        const nsHTMLReflowState& aReflowState,
                        nsPoint&                 aOffset,
                        nsSize&                  aSize,
                        nsPoint*                 aCellIndex = 0);
  
  PRBool CanResize(PRBool aVertical, 
                   PRBool aLeft); 

  PRBool CanChildResize(PRBool  aVertical, 
                        PRBool  aLeft, 
                        PRInt32 aChildX,
                        PRBool  aFrameset);
  
  void SetBorderResize(PRInt32*                   aChildTypes, 
                       nsHTMLFramesetBorderFrame* aBorderFrame);

  PRBool ChildIsFrameset(nsIFrame* aChild); 

  static int FrameResizePrefCallback(const char* aPref, void* aClosure);

  PRInt32          mNumRows;
  nscoord*         mRowSizes;  
  PRInt32          mNumCols;
  nscoord*         mColSizes;  
  PRInt32          mNonBorderChildCount; 
  PRInt32          mNonBlankChildCount; 
  PRInt32          mEdgeVisibility;
  nsBorderColor    mEdgeColors;
  nsFrameborder    mParentFrameborder;
  nscolor          mParentBorderColor;
  PRInt32          mParentBorderWidth;

  nsHTMLFramesetBorderFrame* mDragger;
  nsFramesetDrag   mDrag;
  nsPoint          mFirstDragPoint;
  PRInt32          mPrevNeighborOrigSize; 
  PRInt32          mNextNeighborOrigSize;
  PRInt32          mMinDrag;
  PRInt32          mChildCount;
  nsHTMLFramesetFrame* mTopLevelFrameset;
  nsHTMLFramesetBorderFrame** mVerBorders;  
  nsHTMLFramesetBorderFrame** mHorBorders;  

  PRInt32*         mChildTypes; 
  nsFrameborder*   mChildFrameborder; 
  nsBorderColor*   mChildBorderColors;
  
  PRBool mForceFrameResizability;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLFramesetFrame, NS_IFRAMESETFRAME_IID)

#endif
