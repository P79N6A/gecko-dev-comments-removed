




































#ifndef nsBox_h___
#define nsBox_h___

#include "nsIFrame.h"

class nsITheme;

#define NS_STATE_IS_ROOT        NS_FRAME_STATE_BIT(24)
#define NS_STATE_SET_TO_DEBUG   NS_FRAME_STATE_BIT(26)
#define NS_STATE_DEBUG_WAS_SET  NS_FRAME_STATE_BIT(27)

class nsBox : public nsIFrame {

public:

  friend class nsIFrame;

  static void Shutdown();

  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);

  virtual nsSize GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState);

  virtual PRBool IsCollapsed(nsBoxLayoutState& aBoxLayoutState);

  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         PRBool aRemoveOverflowAreas = PR_FALSE);

  NS_IMETHOD GetBorder(nsMargin& aBorderAndPadding);
  NS_IMETHOD GetPadding(nsMargin& aBorderAndPadding);
  NS_IMETHOD GetMargin(nsMargin& aMargin);

  NS_IMETHOD SetLayoutManager(nsIBoxLayout* aLayout);
  NS_IMETHOD GetLayoutManager(nsIBoxLayout** aLayout);

  virtual Valignment GetVAlign() const { return vAlign_Top; }
  virtual Halignment GetHAlign() const { return hAlign_Left; }


  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild);

#ifdef DEBUG_LAYOUT
  NS_IMETHOD GetDebugBoxAt(const nsPoint& aPoint, nsIBox** aBox);
  NS_IMETHOD GetDebug(PRBool& aDebug);
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, PRBool aDebug);

  NS_IMETHOD DumpBox(FILE* out);
  NS_HIDDEN_(void) PropagateDebug(nsBoxLayoutState& aState);
#endif

  nsBox();
  virtual ~nsBox();

  



  virtual PRBool DoesClipChildren();
  virtual PRBool ComputesOwnOverflowArea() = 0;

  NS_HIDDEN_(nsresult) SyncLayout(nsBoxLayoutState& aBoxLayoutState);

  PRBool DoesNeedRecalc(const nsSize& aSize);
  PRBool DoesNeedRecalc(nscoord aCoord);
  void SizeNeedsRecalc(nsSize& aSize);
  void CoordNeedsRecalc(nscoord& aCoord);

  void AddBorderAndPadding(nsSize& aSize);

  static void AddBorderAndPadding(nsIBox* aBox, nsSize& aSize);
  static void AddMargin(nsIBox* aChild, nsSize& aSize);
  static void AddMargin(nsSize& aSize, const nsMargin& aMargin);

  static nsSize BoundsCheckMinMax(const nsSize& aMinSize, const nsSize& aMaxSize);
  static nsSize BoundsCheck(const nsSize& aMinSize, const nsSize& aPrefSize, const nsSize& aMaxSize);
  static nscoord BoundsCheck(nscoord aMinSize, nscoord aPrefSize, nscoord aMaxSize);

protected:

#ifdef DEBUG_LAYOUT
  virtual void AppendAttribute(const nsAutoString& aAttribute, const nsAutoString& aValue, nsAutoString& aResult);

  virtual void ListBox(nsAutoString& aResult);
#endif
  
  virtual void GetLayoutFlags(PRUint32& aFlags);

  NS_HIDDEN_(nsresult) BeginLayout(nsBoxLayoutState& aState);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_HIDDEN_(nsresult) EndLayout(nsBoxLayoutState& aState);

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName);
  NS_HIDDEN_(void) PropagateDebug(nsBoxLayoutState& aState);
#endif

  static PRBool gGotTheme;
  static nsITheme* gTheme;

  enum eMouseThrough {
    unset,
    never,
    always
  };

private:

  
  
};

#ifdef DEBUG_LAYOUT
#define NS_BOX_ASSERTION(box,expr,str) \
  if (!(expr)) { \
       box->DumpBox(stdout); \
       NS_DebugBreak(NSDebugAssertion, str, #expr, __FILE__, __LINE__); \
  }
#else
#define NS_BOX_ASSERTION(box,expr,str) {}
#endif

#endif

