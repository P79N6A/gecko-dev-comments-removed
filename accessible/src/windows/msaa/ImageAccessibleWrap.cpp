






#include "ImageAccessibleWrap.h"

using namespace mozilla;
using namespace mozilla::a11y;

NS_IMPL_ISUPPORTS_INHERITED0(ImageAccessibleWrap,
                             ImageAccessible)

IMPL_IUNKNOWN_INHERITED1(ImageAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleImage)

