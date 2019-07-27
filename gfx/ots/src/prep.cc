



#include "prep.h"




#define TABLE_NAME "prep"

namespace ots {

bool ots_prep_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypePREP *prep = new OpenTypePREP;
  file->prep = prep;

  if (length >= 128 * 1024u) {
    return OTS_FAILURE_MSG("table length %ld > 120K", length);  
  }

  if (!table.Skip(length)) {
    return OTS_FAILURE_MSG("Failed to read table of length %ld", length);
  }

  prep->data = data;
  prep->length = length;
  return true;
}

bool ots_prep_should_serialise(OpenTypeFile *file) {
  if (!file->glyf) return false;  
  return file->prep;
}

bool ots_prep_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypePREP *prep = file->prep;

  if (!out->Write(prep->data, prep->length)) {
    return OTS_FAILURE_MSG("Failed to write table length");
  }

  return true;
}

void ots_prep_free(OpenTypeFile *file) {
  delete file->prep;
}

}  

#undef TABLE_NAME
