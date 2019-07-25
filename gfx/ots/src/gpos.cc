



#include "gpos.h"




namespace ots {

bool ots_gpos_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypeGPOS *gpos = new OpenTypeGPOS;
  file->gpos = gpos;

  if (!table.Skip(length)) {
    return OTS_FAILURE();
  }

  gpos->data = data;
  gpos->length = length;
  return true;
}

bool ots_gpos_should_serialise(OpenTypeFile *file) {
  return file->preserve_otl && file->gpos;
}

bool ots_gpos_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeGPOS *gpos = file->gpos;

  if (!out->Write(gpos->data, gpos->length)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_gpos_free(OpenTypeFile *file) {
  delete file->gpos;
}

}  
