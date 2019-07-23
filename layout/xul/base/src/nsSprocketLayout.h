




































#ifndef nsSprocketLayout_h___
#define nsSprocketLayout_h___

#include "nsBoxLayout.h"
#include "nsCOMPtr.h"

class nsBoxSize
{
public:

  nsBoxSize();

  nscoord pref;
  nscoord min;
  nscoord max;
  nscoord ascent;
  nscoord flex;
  nscoord left;
  nscoord right;
  PRBool  collapsed;
  PRBool  bogus;

  nsBoxSize* next;

  void* operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t sz);
};

class nsComputedBoxSize
{
public:
  nsComputedBoxSize();

  nscoord size;
  PRBool  valid;
  PRBool  resized;
  nsComputedBoxSize* next;

  void* operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t sz);
};

#define GET_WIDTH(size, isHorizontal) (isHorizontal ? size.width : size.height)
#define GET_HEIGHT(size, isHorizontal) (isHorizontal ? size.height : size.width)
#define GET_X(size, isHorizontal) (isHorizontal ? size.x : size.y)
#define GET_Y(size, isHorizontal) (isHorizontal ? size.y : size.x)
#define GET_COORD(aX, aY, isHorizontal) (isHorizontal ? aX : aY)

#define SET_WIDTH(size, coord, isHorizontal)  if (isHorizontal) { (size).width  = (coord); } else { (size).height = (coord); }
#define SET_HEIGHT(size, coord, isHorizontal) if (isHorizontal) { (size).height = (coord); } else { (size).width  = (coord); }
#define SET_X(size, coord, isHorizontal) if (isHorizontal) { (size).x = (coord); } else { (size).y  = (coord); }
#define SET_Y(size, coord, isHorizontal) if (isHorizontal) { (size).y = (coord); } else { (size).x  = (coord); }

#define SET_COORD(aX, aY, coord, isHorizontal) if (isHorizontal) { aX = (coord); } else { aY  = (coord); }

nsresult NS_NewSprocketLayout(nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout);

class nsSprocketLayout : public nsBoxLayout {

public:

  friend nsresult NS_NewSprocketLayout(nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout);
  static void Shutdown();

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);

  nsSprocketLayout();

  static PRBool IsHorizontal(nsIBox* aBox);

  static void SetLargestSize(nsSize& aSize1, const nsSize& aSize2, PRBool aIsHorizontal);
  static void SetSmallestSize(nsSize& aSize1, const nsSize& aSize2, PRBool aIsHorizontal);

  static void AddLargestSize(nsSize& aSize, const nsSize& aSizeToAdd, PRBool aIsHorizontal);
  static void AddSmallestSize(nsSize& aSize, const nsSize& aSizeToAdd, PRBool aIsHorizontal);
  static void AddCoord(nscoord& aCoord, nscoord aCoordToAdd);

protected:


  void ComputeChildsNextPosition(nsIBox* aBox,
                                 nscoord& aCurX, 
                                 nscoord& aCurY, 
                                 nscoord& aNextX, 
                                 nscoord& aNextY, 
                                 const nsRect& aChildSize, 
                                 const nsRect& aContainingRect,
                                 nscoord childAscent,
                                 nscoord aMaxAscent);

  void ChildResized(nsIBox* aBox,
                    nsBoxLayoutState& aState, 
                    nsIBox* aChild,
                    nsBoxSize* aChildBoxSize, 
                    nsComputedBoxSize* aChildComputedBoxSize, 
                    nsBoxSize* aBoxSizes, 
                    nsComputedBoxSize* aComputedBoxSizes, 
                    const nsRect& aChildLayoutRect, 
                    nsRect& aChildActualRect, 
                    nsRect& aContainingRect, 
                    PRInt32 aFlexes, 
                    PRBool& aFinished);

  virtual void ComputeChildSizes(nsIBox* aBox, 
                         nsBoxLayoutState& aState, 
                         nscoord& aGivenSize, 
                         nsBoxSize* aBoxSizes, 
                         nsComputedBoxSize*& aComputedBoxSizes);


  virtual void PopulateBoxSizes(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsBoxSize*& aBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, PRInt32& aFlexes);

  virtual void InvalidateComputedSizes(nsComputedBoxSize* aComputedBoxSizes);

  virtual PRBool GetDefaultFlex(PRInt32& aFlex);

  virtual void GetFrameState(nsIBox* aBox, nsFrameState& aState);

private:


  
  
  static nsIBoxLayout* gInstance;

};

#endif

