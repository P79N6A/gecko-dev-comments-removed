



#ifndef OTS_HEAD_H_
#define OTS_HEAD_H_

#include "ots.h"

namespace ots {

struct OpenTypeHEAD {
  uint32_t revision;
  uint16_t flags;
  uint16_t ppem;
  uint64_t created;
  uint64_t modified;

  int16_t xmin, xmax;
  int16_t ymin, ymax;

  uint16_t mac_style;
  uint16_t min_ppem;
  int16_t index_to_loc_format;
};

}  

#endif  
