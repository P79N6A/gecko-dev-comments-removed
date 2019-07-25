



#include <cstring>

#include "cff.h"
#include "ots.h"




namespace ots {

bool ots_name_parse(OpenTypeFile *, const uint8_t *, size_t) {
  return true;
}

bool ots_name_should_serialise(OpenTypeFile *) {
  return true;
}

bool ots_name_serialise(OTSStream *out, OpenTypeFile *file) {
  
  

  const char* kStrings[] = {
      "Derived font data",  
      "OTS derived font",  
      "Unspecified",  
      "UniqueID",  
      "OTS derivied font",  
      "1.000",  
      "False",  
      NULL,  
      "OTS",  
      "OTS",  
  };
  static const size_t kStringsLen = sizeof(kStrings) / sizeof(kStrings[0]);

  
  
  
  if (file->cff && !file->cff->name.empty()) {
    kStrings[6] = file->cff->name.c_str();
  }

  unsigned num_strings = 0;
  for (unsigned i = 0; i < kStringsLen; ++i) {
    if (kStrings[i]) num_strings++;
  }

  if (!out->WriteU16(0) ||  
      
      
      
      
      !out->WriteU16(num_strings * 2) ||  
      !out->WriteU16(6 + num_strings * 2 * 12)) {  
    return OTS_FAILURE();
  }

  unsigned current_offset = 0;
  for (unsigned i = 0; i < kStringsLen; ++i) {
    if (!kStrings[i]) continue;

    
    size_t len = std::strlen(kStrings[i]);

    if (!out->WriteU16(1) ||  
        !out->WriteU16(0) ||  
        !out->WriteU16(0) ||  
        !out->WriteU16(i) ||
        !out->WriteU16(len) ||
        !out->WriteU16(current_offset)) {
      return OTS_FAILURE();
    }

    current_offset += len;
  }

  for (unsigned i = 0; i < kStringsLen; ++i) {
    if (!kStrings[i]) continue;

    
    size_t len = std::strlen(kStrings[i]) * 2;

    if (!out->WriteU16(3) ||  
        !out->WriteU16(1) ||  
        !out->WriteU16(0x0409) ||  
        !out->WriteU16(i) ||
        !out->WriteU16(len) ||
        !out->WriteU16(current_offset)) {
      return OTS_FAILURE();
    }

    current_offset += len;
  }

  
  
  for (unsigned i = 0; i < kStringsLen; ++i) {
    if (!kStrings[i]) continue;

    const size_t len = std::strlen(kStrings[i]);
    if (!out->Write(kStrings[i], len)) {
      return OTS_FAILURE();
    }
  }

  
  
  for (unsigned i = 0; i < kStringsLen; ++i) {
    if (!kStrings[i]) continue;

    const size_t len = std::strlen(kStrings[i]);
    for (size_t j = 0; j < len; ++j) {
      uint16_t v = kStrings[i][j];
      if (!out->WriteU16(v)) {
        return OTS_FAILURE();
      }
    }
  }

  return true;
}

void ots_name_free(OpenTypeFile *) {
}

}  
