




#ifndef NSMARGIN_H
#define NSMARGIN_H

#include "nsCoord.h"
#include "nsPoint.h"
#include "gfxCore.h"
#include "mozilla/gfx/BaseMargin.h"

struct nsMargin : public mozilla::gfx::BaseMargin<nscoord, nsMargin> {
  typedef mozilla::gfx::BaseMargin<nscoord, nsMargin> Super;

  
  nsMargin() : Super() {}
  nsMargin(const nsMargin& aMargin) : Super(aMargin) {}
  nsMargin(nscoord aTop, nscoord aRight, nscoord aBottom, nscoord aLeft)
    : Super(aTop, aRight, aBottom, aLeft) {}
};

struct nsIntMargin : public mozilla::gfx::BaseMargin<int32_t, nsIntMargin> {
  typedef mozilla::gfx::BaseMargin<int32_t, nsIntMargin> Super;

  
  nsIntMargin() : Super() {}
  nsIntMargin(const nsIntMargin& aMargin) : Super(aMargin) {}
  nsIntMargin(int32_t aTop, int32_t aRight, int32_t aBottom, int32_t aLeft)
    : Super(aTop, aRight, aBottom, aLeft) {}
};

#endif 
