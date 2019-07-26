



#include "post.h"

#include "maxp.h"




#define TABLE_NAME "post"

namespace ots {

bool ots_post_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypePOST *post = new OpenTypePOST;
  file->post = post;

  if (!table.ReadU32(&post->version) ||
      !table.ReadU32(&post->italic_angle) ||
      !table.ReadS16(&post->underline) ||
      !table.ReadS16(&post->underline_thickness) ||
      !table.ReadU32(&post->is_fixed_pitch)) {
    return OTS_FAILURE_MSG("Failed to read post header");
  }

  if (post->underline_thickness < 0) {
    post->underline_thickness = 1;
  }

  if (post->version == 0x00010000) {
    return true;
  } else if (post->version == 0x00030000) {
    return true;
  } else if (post->version != 0x00020000) {
    
    return OTS_FAILURE_MSG("Bad post version %x", post->version);
  }

  

  
  
  if (!table.Skip(16)) {
    return OTS_FAILURE_MSG("Failed to skip memory usage in post table");
  }

  uint16_t num_glyphs = 0;
  if (!table.ReadU16(&num_glyphs)) {
    return OTS_FAILURE_MSG("Failed to read number of glyphs");
  }

  if (!file->maxp) {
    return OTS_FAILURE_MSG("No maxp table required by post table");
  }

  if (num_glyphs == 0) {
    if (file->maxp->num_glyphs > 258) {
      return OTS_FAILURE_MSG("Can't have no glyphs in the post table if there are more than 256 glyphs in the font");
    }
    OTS_WARNING("table version is 1, but no glyf names are found");
    
    
    post->version = 0x00010000;
    return true;
  }

  if (num_glyphs != file->maxp->num_glyphs) {
    
    return OTS_FAILURE_MSG("Bad number of glyphs in post table %d", num_glyphs);
  }

  post->glyph_name_index.resize(num_glyphs);
  for (unsigned i = 0; i < num_glyphs; ++i) {
    if (!table.ReadU16(&post->glyph_name_index[i])) {
      return OTS_FAILURE_MSG("Failed to read post information for glyph %d", i);
    }
    
    
    
  }

  
  
  const size_t strings_offset = table.offset();
  const uint8_t *strings = data + strings_offset;
  const uint8_t *strings_end = data + length;

  for (;;) {
    if (strings == strings_end) break;
    const unsigned string_length = *strings;
    if (strings + 1 + string_length > strings_end) {
      return OTS_FAILURE_MSG("Bad string length %d", string_length);
    }
    if (std::memchr(strings + 1, '\0', string_length)) {
      return OTS_FAILURE_MSG("Bad string of length %d", string_length);
    }
    post->names.push_back(
        std::string(reinterpret_cast<const char*>(strings + 1), string_length));
    strings += 1 + string_length;
  }
  const unsigned num_strings = post->names.size();

  
  for (unsigned i = 0; i < num_glyphs; ++i) {
    unsigned offset = post->glyph_name_index[i];
    if (offset < 258) {
      continue;
    }

    offset -= 258;
    if (offset >= num_strings) {
      return OTS_FAILURE_MSG("Bad string index %d", offset);
    }
  }

  return true;
}

bool ots_post_should_serialise(OpenTypeFile *file) {
  return file->post != NULL;
}

bool ots_post_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypePOST *post = file->post;

  
  if (file->post && file->cff && file->post->version != 0x00030000) {
    return OTS_FAILURE_MSG("Bad post version %x", post->version);
  }

  if (!out->WriteU32(post->version) ||
      !out->WriteU32(post->italic_angle) ||
      !out->WriteS16(post->underline) ||
      !out->WriteS16(post->underline_thickness) ||
      !out->WriteU32(post->is_fixed_pitch) ||
      !out->WriteU32(0) ||
      !out->WriteU32(0) ||
      !out->WriteU32(0) ||
      !out->WriteU32(0)) {
    return OTS_FAILURE_MSG("Failed to write post header");
  }

  if (post->version != 0x00020000) {
    return true;  
  }

  if (!out->WriteU16(post->glyph_name_index.size())) {
    return OTS_FAILURE_MSG("Failed to write number of indices");
  }

  for (unsigned i = 0; i < post->glyph_name_index.size(); ++i) {
    if (!out->WriteU16(post->glyph_name_index[i])) {
      return OTS_FAILURE_MSG("Failed to write name index %d", i);
    }
  }

  
  for (unsigned i = 0; i < post->names.size(); ++i) {
    const std::string& s = post->names[i];
    const uint8_t string_length = s.size();
    if (!out->Write(&string_length, 1)) {
      return OTS_FAILURE_MSG("Failed to write string %d", i);
    }
    
    
    if (string_length > 0 && !out->Write(s.data(), string_length)) {
      return OTS_FAILURE_MSG("Failed to write string length for string %d", i);
    }
  }

  return true;
}

void ots_post_free(OpenTypeFile *file) {
  delete file->post;
}

}  

#undef TABLE_NAME
