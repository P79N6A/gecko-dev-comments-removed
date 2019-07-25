



#include "hmtx.h"

#include "hhea.h"
#include "maxp.h"




namespace ots {

bool ots_hmtx_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  OpenTypeHMTX *hmtx = new OpenTypeHMTX;
  file->hmtx = hmtx;

  if (!file->hhea || !file->maxp) {
    return OTS_FAILURE();
  }

  if (!ParseMetricsTable(&table, file->maxp->num_glyphs,
                         &file->hhea->header, &hmtx->metrics)) {
    return OTS_FAILURE();
  }

  return true;
}

bool ots_hmtx_should_serialise(OpenTypeFile *file) {
  return file->hmtx != NULL;
}

bool ots_hmtx_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!SerialiseMetricsTable(out, &file->hmtx->metrics)) {
    return OTS_FAILURE();
  }
  return true;
}

void ots_hmtx_free(OpenTypeFile *file) {
  delete file->hmtx;
}

}  
