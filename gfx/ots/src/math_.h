



#ifndef OTS_MATH_H_
#define OTS_MATH_H_

#include "ots.h"

namespace ots {

struct OpenTypeMATH {
  OpenTypeMATH()
      : data(NULL),
        length(0) {
  }

  const uint8_t *data;
  size_t length;
};

}  

#endif

