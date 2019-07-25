




































#ifndef NSMARGIN_H
#define NSMARGIN_H

#include "nsCoord.h"
#include "nsPoint.h"
#include "gfxCore.h"
#include "mozilla/BaseMargin.h"

struct nsMargin : public mozilla::BaseMargin<nscoord, nsMargin> {
  typedef mozilla::BaseMargin<nscoord, nsMargin> Super;

  
  nsMargin() : Super() {}
  nsMargin(const nsMargin& aMargin) : Super(aMargin) {}
  nsMargin(nscoord aLeft,  nscoord aTop, nscoord aRight, nscoord aBottom)
    : Super(aLeft, aTop, aRight, aBottom) {}
};

struct nsIntMargin : public mozilla::BaseMargin<PRInt32, nsIntMargin> {
  typedef mozilla::BaseMargin<PRInt32, nsIntMargin> Super;

  
  nsIntMargin() : Super() {}
  nsIntMargin(const nsIntMargin& aMargin) : Super(aMargin) {}
  nsIntMargin(PRInt32 aLeft,  PRInt32 aTop, PRInt32 aRight, PRInt32 aBottom)
    : Super(aLeft, aTop, aRight, aBottom) {}
};

#endif 
