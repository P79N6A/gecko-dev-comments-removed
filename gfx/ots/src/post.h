



#ifndef OTS_POST_H_
#define OTS_POST_H_

#include "ots.h"

#include <map>
#include <string>
#include <vector>

namespace ots {

struct OpenTypePOST {
  uint32_t version;
  uint32_t italic_angle;
  int16_t underline;
  int16_t underline_thickness;
  uint32_t is_fixed_pitch;

  std::vector<uint16_t> glyph_name_index;
  std::vector<std::string> names;
};

}  

#endif  
