



#include "hhea.h"

#include "head.h"
#include "maxp.h"




namespace ots {

bool ots_hhea_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  OpenTypeHHEA *hhea = new OpenTypeHHEA;
  file->hhea = hhea;

  if (!table.ReadU32(&hhea->header.version)) {
    return OTS_FAILURE();
  }
  if (hhea->header.version >> 16 != 1) {
    return OTS_FAILURE();
  }

  if (!ParseMetricsHeader(file, &table, &hhea->header)) {
    return OTS_FAILURE();
  }

  return true;
}

bool ots_hhea_should_serialise(OpenTypeFile *file) {
  return file->hhea != NULL;
}

bool ots_hhea_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!SerialiseMetricsHeader(out, &file->hhea->header)) {
    return OTS_FAILURE();
  }
  return true;
}

void ots_hhea_free(OpenTypeFile *file) {
  delete file->hhea;
}

}  
