



#include "hmtx.h"

#include "hhea.h"
#include "maxp.h"




#define TABLE_NAME "hmtx"

namespace ots {

bool ots_hmtx_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  OpenTypeHMTX *hmtx = new OpenTypeHMTX;
  file->hmtx = hmtx;

  if (!file->hhea || !file->maxp) {
    return OTS_FAILURE_MSG("Missing hhea or maxp tables in font, needed by hmtx");
  }

  if (!ParseMetricsTable(file, &table, file->maxp->num_glyphs,
                         &file->hhea->header, &hmtx->metrics)) {
    return OTS_FAILURE_MSG("Failed to parse hmtx metrics");
  }

  return true;
}

bool ots_hmtx_should_serialise(OpenTypeFile *file) {
  return file->hmtx != NULL;
}

bool ots_hmtx_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!SerialiseMetricsTable(file, out, &file->hmtx->metrics)) {
    return OTS_FAILURE_MSG("Failed to serialise htmx metrics");
  }
  return true;
}

void ots_hmtx_free(OpenTypeFile *file) {
  delete file->hmtx;
}

}  

#undef TABLE_NAME
