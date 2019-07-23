




































#ifndef NSMARGIN_H
#define NSMARGIN_H

#include "nsCoord.h"
#include "nsPoint.h"
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

  nsPoint TopLeft() const { return nsPoint(left, top); }

  nscoord& side(mozilla::css::Side aSide) {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }

  nscoord side(mozilla::css::Side aSide) const {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }

  
  
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

  PRBool IsZero() { return !left && !top && !right && !bottom; }
};

struct nsIntMargin {
  PRInt32 top, right, bottom, left;

  
  nsIntMargin() {}
  nsIntMargin(const nsIntMargin& aMargin) {*this = aMargin;}
  nsIntMargin(PRInt32 aLeft,  PRInt32 aTop,
              PRInt32 aRight, PRInt32 aBottom) {left = aLeft; top = aTop;
                                                right = aRight; bottom = aBottom;}
  void SizeTo(PRInt32 aLeft,  PRInt32 aTop,
              PRInt32 aRight, PRInt32 aBottom) {left = aLeft; top = aTop;
                                                right = aRight; bottom = aBottom;}

  PRInt32 LeftRight() const { return left + right; }
  PRInt32 TopBottom() const { return top + bottom; }

  nsPoint TopLeft() const { return nsPoint(left, top); }

  PRInt32& side(mozilla::css::Side aSide) {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }

  PRInt32 side(mozilla::css::Side aSide) const {
    NS_PRECONDITION(aSide <= NS_SIDE_LEFT, "Out of range side");
    return *(&top + aSide);
  }

  PRBool operator!=(const nsIntMargin& aMargin) const {
    return (PRBool) ((left != aMargin.left) || (top != aMargin.top) ||
                     (right != aMargin.right) || (bottom != aMargin.bottom));
  }
  nsIntMargin operator+(const nsIntMargin& aMargin) const {
    return nsIntMargin(left + aMargin.left, top + aMargin.top,
                    right + aMargin.right, bottom + aMargin.bottom);
  }

  PRBool IsZero() { return !left && !top && !right && !bottom; }
};

#endif 
