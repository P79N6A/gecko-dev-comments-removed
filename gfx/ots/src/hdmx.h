



#ifndef OTS_HDMX_H_
#define OTS_HDMX_H_

#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeHDMXDeviceRecord {
  uint8_t pixel_size;
  uint8_t max_width;
  std::vector<uint8_t> widths;
};

struct OpenTypeHDMX {
  uint16_t version;
  int32_t size_device_record;
  int32_t pad_len;
  std::vector<OpenTypeHDMXDeviceRecord> records;
};

}  

#endif
