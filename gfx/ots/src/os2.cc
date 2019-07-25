



#include <cstddef>

#include "os2.h"

#include "head.h"




namespace ots {

bool ots_os2_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypeOS2 *os2 = new OpenTypeOS2;
  file->os2 = os2;

  if (!table.ReadU16(&os2->version) ||
      !table.ReadS16(&os2->avg_char_width) ||
      !table.ReadU16(&os2->weight_class) ||
      !table.ReadU16(&os2->width_class) ||
      !table.ReadU16(&os2->type) ||
      !table.ReadS16(&os2->subscript_x_size) ||
      !table.ReadS16(&os2->subscript_y_size) ||
      !table.ReadS16(&os2->subscript_x_offset) ||
      !table.ReadS16(&os2->subscript_y_offset) ||
      !table.ReadS16(&os2->superscript_x_size) ||
      !table.ReadS16(&os2->superscript_y_size) ||
      !table.ReadS16(&os2->superscript_x_offset) ||
      !table.ReadS16(&os2->superscript_y_offset) ||
      !table.ReadS16(&os2->strikeout_size) ||
      !table.ReadS16(&os2->strikeout_position) ||
      !table.ReadS16(&os2->family_class)) {
    return OTS_FAILURE();
  }

  if (os2->version > 4) {
    return OTS_FAILURE();
  }

  
  
  if (os2->weight_class < 100 ||
      os2->weight_class > 900 ||
      os2->weight_class % 100) {
    OTS_WARNING("bad weight: %u", os2->weight_class);
    os2->weight_class = 400;  
  }
  if (os2->width_class < 1) {
    OTS_WARNING("bad width: %u", os2->width_class);
    os2->width_class = 1;
  } else if (os2->width_class > 9) {
    OTS_WARNING("bad width: %u", os2->width_class);
    os2->width_class = 9;
  }

  
  if (os2->type & 0x2) {
    
    os2->type &= 0xfff3u;
  } else if (os2->type & 0x4) {
    
    os2->type &= 0xfff4u;
  } else if (os2->type & 0x8) {
    
    os2->type &= 0xfff9u;
  }

  
  os2->type &= 0x30f;

  if (os2->subscript_x_size < 0) {
    OTS_WARNING("bad subscript_x_size: %d", os2->subscript_x_size);
    os2->subscript_x_size = 0;
  }
  if (os2->subscript_y_size < 0) {
    OTS_WARNING("bad subscript_y_size: %d", os2->subscript_y_size);
    os2->subscript_y_size = 0;
  }
  if (os2->superscript_x_size < 0) {
    OTS_WARNING("bad superscript_x_size: %d", os2->superscript_x_size);
    os2->superscript_x_size = 0;
  }
  if (os2->superscript_y_size < 0) {
    OTS_WARNING("bad superscript_y_size: %d", os2->superscript_y_size);
    os2->superscript_y_size = 0;
  }
  if (os2->strikeout_size < 0) {
    OTS_WARNING("bad strikeout_size: %d", os2->strikeout_size);
    os2->strikeout_size = 0;
  }

  for (unsigned i = 0; i < 10; ++i) {
    if (!table.ReadU8(&os2->panose[i])) {
      return OTS_FAILURE();
    }
  }

  if (!table.ReadU32(&os2->unicode_range_1) ||
      !table.ReadU32(&os2->unicode_range_2) ||
      !table.ReadU32(&os2->unicode_range_3) ||
      !table.ReadU32(&os2->unicode_range_4) ||
      !table.ReadU32(&os2->vendor_id) ||
      !table.ReadU16(&os2->selection) ||
      !table.ReadU16(&os2->first_char_index) ||
      !table.ReadU16(&os2->last_char_index) ||
      !table.ReadS16(&os2->typo_ascender) ||
      !table.ReadS16(&os2->typo_descender) ||
      !table.ReadS16(&os2->typo_linegap) ||
      !table.ReadU16(&os2->win_ascent) ||
      !table.ReadU16(&os2->win_descent)) {
    return OTS_FAILURE();
  }

  
  if (os2->selection & 0x40) {
    os2->selection &= 0xffdeu;
  }

  
  
  if (!file->head) {
    return OTS_FAILURE();
  }
  if ((os2->selection & 0x1) &&
      !(file->head->mac_style & 0x2)) {
    OTS_WARNING("adjusting Mac style (italic)");
    file->head->mac_style |= 0x2;
  }
  if ((os2->selection & 0x2) &&
      !(file->head->mac_style & 0x4)) {
    OTS_WARNING("adjusting Mac style (underscore)");
    file->head->mac_style |= 0x4;
  }

  
  
  if ((os2->selection & 0x40) &&
      (file->head->mac_style & 0x3)) {
    OTS_WARNING("adjusting Mac style (regular)");
    file->head->mac_style &= 0xfffcu;
  }

  if ((os2->version < 4) &&
      (os2->selection & 0x300)) {
    
    return OTS_FAILURE();
  }

  
  os2->selection &= 0x3ff;

  if (os2->first_char_index > os2->last_char_index) {
    return OTS_FAILURE();
  }
  if (os2->typo_linegap < 0) {
    OTS_WARNING("bad linegap: %d", os2->typo_linegap);
    os2->typo_linegap = 0;
  }

  if (os2->version < 1) {
    
    return true;
  }

  if (length < offsetof(OpenTypeOS2, code_page_range_2)) {
    OTS_WARNING("bad version number: %u", os2->version);
    
    
    os2->version = 0;
    return true;
  }

  if (!table.ReadU32(&os2->code_page_range_1) ||
      !table.ReadU32(&os2->code_page_range_2)) {
    return OTS_FAILURE();
  }

  if (os2->version < 2) {
    
    return true;
  }

  if (length < offsetof(OpenTypeOS2, max_context)) {
    OTS_WARNING("bad version number: %u", os2->version);
    
    
    os2->version = 1;
    return true;
  }

  if (!table.ReadS16(&os2->x_height) ||
      !table.ReadS16(&os2->cap_height) ||
      !table.ReadU16(&os2->default_char) ||
      !table.ReadU16(&os2->break_char) ||
      !table.ReadU16(&os2->max_context)) {
    return OTS_FAILURE();
  }

  if (os2->x_height < 0) {
    OTS_WARNING("bad x_height: %d", os2->x_height);
    os2->x_height = 0;
  }
  if (os2->cap_height < 0) {
    OTS_WARNING("bad cap_height: %d", os2->cap_height);
    os2->cap_height = 0;
  }

  return true;
}

bool ots_os2_should_serialise(OpenTypeFile *file) {
  return file->os2 != NULL;
}

bool ots_os2_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeOS2 *os2 = file->os2;

  if (!out->WriteU16(os2->version) ||
      !out->WriteS16(os2->avg_char_width) ||
      !out->WriteU16(os2->weight_class) ||
      !out->WriteU16(os2->width_class) ||
      !out->WriteU16(os2->type) ||
      !out->WriteS16(os2->subscript_x_size) ||
      !out->WriteS16(os2->subscript_y_size) ||
      !out->WriteS16(os2->subscript_x_offset) ||
      !out->WriteS16(os2->subscript_y_offset) ||
      !out->WriteS16(os2->superscript_x_size) ||
      !out->WriteS16(os2->superscript_y_size) ||
      !out->WriteS16(os2->superscript_x_offset) ||
      !out->WriteS16(os2->superscript_y_offset) ||
      !out->WriteS16(os2->strikeout_size) ||
      !out->WriteS16(os2->strikeout_position) ||
      !out->WriteS16(os2->family_class)) {
    return OTS_FAILURE();
  }

  for (unsigned i = 0; i < 10; ++i) {
    if (!out->Write(&os2->panose[i], 1)) {
      return OTS_FAILURE();
    }
  }

  if (!out->WriteU32(os2->unicode_range_1) ||
      !out->WriteU32(os2->unicode_range_2) ||
      !out->WriteU32(os2->unicode_range_3) ||
      !out->WriteU32(os2->unicode_range_4) ||
      !out->WriteU32(os2->vendor_id) ||
      !out->WriteU16(os2->selection) ||
      !out->WriteU16(os2->first_char_index) ||
      !out->WriteU16(os2->last_char_index) ||
      !out->WriteS16(os2->typo_ascender) ||
      !out->WriteS16(os2->typo_descender) ||
      !out->WriteS16(os2->typo_linegap) ||
      !out->WriteU16(os2->win_ascent) ||
      !out->WriteU16(os2->win_descent)) {
    return OTS_FAILURE();
  }

  if (os2->version < 1) {
    return true;
  }

  if (!out->WriteU32(os2->code_page_range_1) ||
      !out->WriteU32(os2->code_page_range_2)) {
    return OTS_FAILURE();
  }

  if (os2->version < 2) {
    return true;
  }

  if (!out->WriteS16(os2->x_height) ||
      !out->WriteS16(os2->cap_height) ||
      !out->WriteU16(os2->default_char) ||
      !out->WriteU16(os2->break_char) ||
      !out->WriteU16(os2->max_context)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_os2_free(OpenTypeFile *file) {
  delete file->os2;
}

}  
