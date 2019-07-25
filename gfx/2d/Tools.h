




































#ifndef MOZILLA_GFX_TOOLS_H_
#define MOZILLA_GFX_TOOLS_H_

#include "Types.h"

namespace mozilla {
namespace gfx {

bool
IsOperatorBoundByMask(CompositionOp aOp) {
  switch (aOp) {
  case OP_IN:
  case OP_OUT:
  case OP_DEST_IN:
  case OP_DEST_ATOP:
  case OP_SOURCE:
    return false;
  default:
    return true;
  }
}

}
}

#endif 
