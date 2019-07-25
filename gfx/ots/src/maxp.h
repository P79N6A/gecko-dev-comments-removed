



#ifndef OTS_MAXP_H_
#define OTS_MAXP_H_

#include "ots.h"

namespace ots {

struct OpenTypeMAXP {
  uint16_t num_glyphs;
  bool version_1;

  uint16_t max_points;
  uint16_t max_contours;
  uint16_t max_c_points;
  uint16_t max_c_contours;

  uint16_t max_zones;
  uint16_t max_t_points;
  uint16_t max_storage;
  uint16_t max_fdefs;
  uint16_t max_idefs;
  uint16_t max_stack;
  uint16_t max_size_glyf_instructions;

  uint16_t max_c_components;
  uint16_t max_c_depth;
};

}  

#endif  
