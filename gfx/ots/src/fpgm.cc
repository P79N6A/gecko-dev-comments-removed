



#include "fpgm.h"




namespace ots {

bool ots_fpgm_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypeFPGM *fpgm = new OpenTypeFPGM;
  file->fpgm = fpgm;

  if (length >= 128 * 1024u) {
    return OTS_FAILURE();  
  }

  if (!table.Skip(length)) {
    return OTS_FAILURE();
  }

  fpgm->data = data;
  fpgm->length = length;
  return true;
}

bool ots_fpgm_should_serialise(OpenTypeFile *file) {
  if (!file->glyf) return false;  
  return g_transcode_hints && file->fpgm;
}

bool ots_fpgm_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeFPGM *fpgm = file->fpgm;

  if (!out->Write(fpgm->data, fpgm->length)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_fpgm_free(OpenTypeFile *file) {
  delete file->fpgm;
}

}  
