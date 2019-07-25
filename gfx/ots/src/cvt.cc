



#include "cvt.h"




namespace ots {

bool ots_cvt_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypeCVT *cvt = new OpenTypeCVT;
  file->cvt = cvt;

  if (length >= 128 * 1024u) {
    return OTS_FAILURE();  
  }

  if (length % 2 != 0) {
    return OTS_FAILURE();
  }

  if (!table.Skip(length)) {
    return OTS_FAILURE();
  }

  cvt->data = data;
  cvt->length = length;
  return true;
}

bool ots_cvt_should_serialise(OpenTypeFile *file) {
  if (!file->glyf) {
    return false;  
  }
  return g_transcode_hints && file->cvt;
}

bool ots_cvt_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeCVT *cvt = file->cvt;

  if (!out->Write(cvt->data, cvt->length)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_cvt_free(OpenTypeFile *file) {
  delete file->cvt;
}

}  
