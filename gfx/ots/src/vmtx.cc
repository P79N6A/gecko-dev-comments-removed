



#include "vmtx.h"

#include "gsub.h"
#include "maxp.h"
#include "vhea.h"




namespace ots {

bool ots_vmtx_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  OpenTypeVMTX *vmtx = new OpenTypeVMTX;
  file->vmtx = vmtx;

  if (!file->vhea || !file->maxp) {
    return OTS_FAILURE();
  }

  if (!ParseMetricsTable(&table, file->maxp->num_glyphs,
                         &file->vhea->header, &vmtx->metrics)) {
    return OTS_FAILURE();
  }

  return true;
}

bool ots_vmtx_should_serialise(OpenTypeFile *file) {
  
  
  return file->vmtx != NULL && file->vhea != NULL &&
      ots_gsub_should_serialise(file);
}

bool ots_vmtx_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!SerialiseMetricsTable(out, &file->vmtx->metrics)) {
    return OTS_FAILURE();
  }
  return true;
}

void ots_vmtx_free(OpenTypeFile *file) {
  delete file->vmtx;
}

}  

