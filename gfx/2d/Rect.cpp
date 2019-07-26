




#include "Rect.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {

void Rect::NudgeToIntegers()
{
  NudgeToInteger(&x);
  NudgeToInteger(&y);
  NudgeToInteger(&width);
  NudgeToInteger(&height);
}

}
}
