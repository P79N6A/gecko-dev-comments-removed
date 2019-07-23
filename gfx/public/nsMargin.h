




































#ifndef NSMARGIN_H
#define NSMARGIN_H

#include "nsCoord.h"
#include "gfxCore.h"

struct nsMargin {
  nscoord top, right, bottom, left;

  
  nsMargin() {}
  nsMargin(const nsMargin& aMargin) {*this = aMargin;}
  nsMargin(nscoord aLeft,  nscoord aTop,
           nscoord aRight, nscoord aBottom) {left = aLeft; top = aTop;
                                             right = aRight; bottom = aBottom;}

  void SizeTo(nscoord aLeft,  nscoord aTop,
              nscoord aRight, nscoord aBottom) {left = aLeft; top = aTop;
                                                right = aRight; bottom = aBottom;}
  void SizeBy(nscoord aLeft, nscoord  aTop,
              nscoord aRight, nscoord aBottom) {left += aLeft; top += aTop;
                                                right += aRight; bottom += aBottom;}

  nscoord LeftRight() const { return left + right; }
  nscoord TopBottom() const { return top + bottom; }

#if (NS_SIDE_TOP == 0) && (NS_SIDE_RIGHT == 1) && (NS_SIDE_BOTTOM == 2) && (NS_SIDE_LEFT == 3)
  nscoord& side(PRUint8 aSide) {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }    

  nscoord side(PRUint8 aSide) const {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }    
#else
#error "Somebody changed the side constants."
#endif

  
  
  PRBool operator==(const nsMargin& aMargin) const {
    return (PRBool) ((left == aMargin.left) && (top == aMargin.top) &&
                     (right == aMargin.right) && (bottom == aMargin.bottom));
  }
  PRBool operator!=(const nsMargin& aMargin) const {
    return (PRBool) ((left != aMargin.left) || (top != aMargin.top) ||
                     (right != aMargin.right) || (bottom != aMargin.bottom));
  }
  nsMargin operator+(const nsMargin& aMargin) const {
    return nsMargin(left + aMargin.left, top + aMargin.top,
                    right + aMargin.right, bottom + aMargin.bottom);
  }
  nsMargin operator-(const nsMargin& aMargin) const {
    return nsMargin(left - aMargin.left, top - aMargin.top,
                    right - aMargin.right, bottom - aMargin.bottom);
  }
  nsMargin& operator+=(const nsMargin& aMargin) {left += aMargin.left;
                                                 top += aMargin.top;
                                                 right += aMargin.right;
                                                 bottom += aMargin.bottom;
                                                 return *this;}
  nsMargin& operator-=(const nsMargin& aMargin) {left -= aMargin.left;
                                                 top -= aMargin.top;
                                                 right -= aMargin.right;
                                                 bottom -= aMargin.bottom;
                                                 return *this;}
};

#ifdef NS_COORD_IS_FLOAT
struct nsIntMargin {
  PRInt32 top, right, bottom, left;

  
  nsIntMargin() {}
  nsIntMargin(const nsIntMargin& aMargin) {*this = aMargin;}
  nsIntMargin(PRInt32 aLeft,  PRInt32 aTop,
              PRInt32 aRight, PRInt32 aBottom) {left = aLeft; top = aTop;
                                                right = aRight; bottom = aBottom;}
};
#else
typedef nsMargin nsIntMargin;
#endif

#endif 
