



#ifndef OTS_GRAPHITE_H_
#define OTS_GRAPHITE_H_

#include "ots.h"

namespace ots {

struct OpenTypeSILF {
  const uint8_t *data;
  uint32_t length;
};

struct OpenTypeSILL {
  const uint8_t *data;
  uint32_t length;
};

struct OpenTypeGLOC {
  const uint8_t *data;
  uint32_t length;
};

struct OpenTypeGLAT {
  const uint8_t *data;
  uint32_t length;
};

struct OpenTypeFEAT {
  const uint8_t *data;
  uint32_t length;
};

}  

#endif  
