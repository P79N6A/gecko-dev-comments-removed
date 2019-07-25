



#ifndef OTS_CFF_H_
#define OTS_CFF_H_

#include "ots.h"

#include <map>
#include <string>
#include <vector>

namespace ots {

struct CFFIndex {
  CFFIndex()
      : count(0), off_size(0), offset_to_next(0) {}
  uint16_t count;
  uint8_t off_size;
  std::vector<uint32_t> offsets;
  uint32_t offset_to_next;
};

struct OpenTypeCFF {
  const uint8_t *data;
  size_t length;
  
  std::string name;

  
  size_t font_dict_length;
  
  std::map<uint16_t, uint8_t> fd_select;

  
  std::vector<CFFIndex *> char_strings_array;
  
  std::vector<CFFIndex *> local_subrs_per_font;
  
  CFFIndex *local_subrs;
};

}  

#endif  
