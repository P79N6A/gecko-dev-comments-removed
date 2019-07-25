




































#ifndef NSSIZE_H
#define NSSIZE_H

#include "nsCoord.h"


#define NS_MAXSIZE nscoord_MAX

struct nsSize {
  nscoord width, height;

  
  nsSize() {}
  nsSize(const nsSize& aSize) {width = aSize.width; height = aSize.height;}
  nsSize(nscoord aWidth, nscoord aHeight) {width = aWidth; height = aHeight;}

  void SizeTo(nscoord aWidth, nscoord aHeight) {width = aWidth; height = aHeight;}
  void SizeBy(nscoord aDeltaWidth, nscoord aDeltaHeight) {width += aDeltaWidth;
                                                          height += aDeltaHeight;}

  
  
  PRBool  operator==(const nsSize& aSize) const {
    return (PRBool) ((width == aSize.width) && (height == aSize.height));
  }
  PRBool  operator!=(const nsSize& aSize) const {
    return (PRBool) ((width != aSize.width) || (height != aSize.height));
  }
  nsSize operator+(const nsSize& aSize) const {
    return nsSize(width + aSize.width, height + aSize.height);
  }
  nsSize& operator+=(const nsSize& aSize) {width += aSize.width;
                                           height += aSize.height;
                                           return *this;}

  
  inline nsSize ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const;
};

struct nsIntSize {
  PRInt32 width, height;

  nsIntSize() {}
  nsIntSize(const nsIntSize& aSize) {width = aSize.width; height = aSize.height;}
  nsIntSize(PRInt32 aWidth, PRInt32 aHeight) {width = aWidth; height = aHeight;}

  
  
  PRBool  operator==(const nsIntSize& aSize) const {
    return (PRBool) ((width == aSize.width) && (height == aSize.height));
  }
  PRBool  operator!=(const nsIntSize& aSize) const {
    return (PRBool) ((width != aSize.width) || (height != aSize.height));
  }

  void SizeTo(PRInt32 aWidth, PRInt32 aHeight) {width = aWidth; height = aHeight;}
};

inline nsSize
nsSize::ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const {
  if (aFromAPP != aToAPP) {
    nsSize size;
    size.width = NSToCoordRound(NSCoordScale(width, aFromAPP, aToAPP));
    size.height = NSToCoordRound(NSCoordScale(height, aFromAPP, aToAPP));
    return size;
  }
  return *this;
}

#endif 
