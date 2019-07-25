



#ifndef OTS_CMAP_H_
#define OTS_CMAP_H_

#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeCMAPSubtableRange {
  uint32_t start_range;
  uint32_t end_range;
  uint32_t start_glyph_id;
};

struct OpenTypeCMAP {
  OpenTypeCMAP()
      : subtable_3_0_4_data(NULL),
        subtable_3_0_4_length(0),
        subtable_3_1_4_data(NULL),
        subtable_3_1_4_length(0),
        subtable_0_5_14_data(NULL),
        subtable_0_5_14_length(0) {
  }

  
  const uint8_t *subtable_3_0_4_data;
  size_t subtable_3_0_4_length;
  
  const uint8_t *subtable_3_1_4_data;
  size_t subtable_3_1_4_length;
  
  const uint8_t *subtable_0_5_14_data;
  size_t subtable_0_5_14_length;

  
  std::vector<OpenTypeCMAPSubtableRange> subtable_3_10_12;
  
  std::vector<OpenTypeCMAPSubtableRange> subtable_3_10_13;
  
  std::vector<uint8_t> subtable_1_0_0;
};

}  

#endif
