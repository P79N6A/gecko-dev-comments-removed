



#ifndef OTS_GSUB_H_
#define OTS_GSUB_H_

#include "ots.h"

namespace ots {

struct OpenTypeGSUB {
  OpenTypeGSUB()
      : num_lookups(0),
        data(NULL),
        length(0) {
  }

  
  uint16_t num_lookups;

  const uint8_t *data;
  size_t length;
};

}  

#endif  

