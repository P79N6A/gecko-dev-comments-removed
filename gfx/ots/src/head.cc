



#include "head.h"

#include <cstring>




namespace ots {

bool ots_head_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  file->head = new OpenTypeHEAD;

  uint32_t version = 0;
  if (!table.ReadU32(&version) ||
      !table.ReadU32(&file->head->revision)) {
    return OTS_FAILURE();
  }

  if (version >> 16 != 1) {
    return OTS_FAILURE();
  }

  
  if (!table.Skip(4)) {
    return OTS_FAILURE();
  }

  uint32_t magic;
  if (!table.ReadTag(&magic) ||
      std::memcmp(&magic, "\x5F\x0F\x3C\xF5", 4)) {
    return OTS_FAILURE();
  }

  if (!table.ReadU16(&file->head->flags)) {
    return OTS_FAILURE();
  }

  
  file->head->flags &= 0x381f;

  if (!table.ReadU16(&file->head->ppem)) {
    return OTS_FAILURE();
  }

  
  if (file->head->ppem < 16 ||
      file->head->ppem > 16384) {
    return OTS_FAILURE();
  }

  
#if 0
  
  
  if ((file->head->ppem - 1) & file->head->ppem) {
    return OTS_FAILURE();
  }
#endif

  if (!table.ReadR64(&file->head->created) ||
      !table.ReadR64(&file->head->modified)) {
    return OTS_FAILURE();
  }

  if (!table.ReadS16(&file->head->xmin) ||
      !table.ReadS16(&file->head->ymin) ||
      !table.ReadS16(&file->head->xmax) ||
      !table.ReadS16(&file->head->ymax)) {
    return OTS_FAILURE();
  }

  if (file->head->xmin > file->head->xmax) {
    return OTS_FAILURE();
  }
  if (file->head->ymin > file->head->ymax) {
    return OTS_FAILURE();
  }

  if (!table.ReadU16(&file->head->mac_style)) {
    return OTS_FAILURE();
  }

  
  file->head->mac_style &= 0x7f;

  if (!table.ReadU16(&file->head->min_ppem)) {
    return OTS_FAILURE();
  }

  
  if (!table.Skip(2)) {
    return OTS_FAILURE();
  }

  if (!table.ReadS16(&file->head->index_to_loc_format)) {
    return OTS_FAILURE();
  }
  if (file->head->index_to_loc_format < 0 ||
      file->head->index_to_loc_format > 1) {
    return OTS_FAILURE();
  }

  int16_t glyph_data_format;
  if (!table.ReadS16(&glyph_data_format) ||
      glyph_data_format) {
    return OTS_FAILURE();
  }

  return true;
}

bool ots_head_should_serialise(OpenTypeFile *file) {
  return file->head != NULL;
}

bool ots_head_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!out->WriteU32(0x00010000) ||
      !out->WriteU32(file->head->revision) ||
      !out->WriteU32(0) ||  
      !out->WriteU32(0x5F0F3CF5) ||
      !out->WriteU16(file->head->flags) ||
      !out->WriteU16(file->head->ppem) ||
      !out->WriteR64(file->head->created) ||
      !out->WriteR64(file->head->modified) ||
      !out->WriteS16(file->head->xmin) ||
      !out->WriteS16(file->head->ymin) ||
      !out->WriteS16(file->head->xmax) ||
      !out->WriteS16(file->head->ymax) ||
      !out->WriteU16(file->head->mac_style) ||
      !out->WriteU16(file->head->min_ppem) ||
      !out->WriteS16(2) ||
      !out->WriteS16(file->head->index_to_loc_format) ||
      !out->WriteS16(0)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_head_free(OpenTypeFile *file) {
  delete file->head;
}

}  
