




#ifndef ScrollbarStyles_h
#define ScrollbarStyles_h

#include <stdint.h>

namespace mozilla {

struct ScrollbarStyles
{
  
  
  uint8_t mHorizontal;
  uint8_t mVertical;
  ScrollbarStyles(uint8_t h, uint8_t v) : mHorizontal(h), mVertical(v) {}
  ScrollbarStyles() {}
  bool operator==(const ScrollbarStyles& aStyles) const {
    return aStyles.mHorizontal == mHorizontal && aStyles.mVertical == mVertical;
  }
  bool operator!=(const ScrollbarStyles& aStyles) const {
    return aStyles.mHorizontal != mHorizontal || aStyles.mVertical != mVertical;
  }
};

}

#endif
