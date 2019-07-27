




#include "math_.h"

#include <limits>
#include <vector>

#include "layout.h"
#include "maxp.h"







#define TABLE_NAME "MATH"

namespace {






const unsigned kMathHeaderSize = 4 + 3 * 2;






const unsigned kMathGlyphInfoHeaderSize = 4 * 2;




const unsigned kMathValueRecordSize = 2 * 2;







const unsigned kGlyphPartRecordSize = 5 * 2;



bool ParseMathValueRecord(const ots::OpenTypeFile *file,
                          ots::Buffer* subtable, const uint8_t *data,
                          const size_t length) {
  
  if (!subtable->Skip(2)) {
    return OTS_FAILURE();
  }

  
  uint16_t offset = 0;
  if (!subtable->ReadU16(&offset)) {
    return OTS_FAILURE();
  }
  if (offset) {
    if (offset >= length) {
      return OTS_FAILURE();
    }
    if (!ots::ParseDeviceTable(file, data + offset, length - offset)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool ParseMathConstantsTable(const ots::OpenTypeFile *file,
                             const uint8_t *data, size_t length) {
  ots::Buffer subtable(data, length);

  
  
  
  
  
  if (!subtable.Skip(4 * 2)) {
    return OTS_FAILURE();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  for (unsigned i = 0; i < static_cast<unsigned>(51); ++i) {
    if (!ParseMathValueRecord(file, &subtable, data, length)) {
      return OTS_FAILURE();
    }
  }

  
  
  if (!subtable.Skip(2)) {
    return OTS_FAILURE();
  }

  return true;
}

bool ParseMathValueRecordSequenceForGlyphs(const ots::OpenTypeFile *file,
                                           ots::Buffer* subtable,
                                           const uint8_t *data,
                                           const size_t length,
                                           const uint16_t num_glyphs) {
  
  uint16_t offset_coverage = 0;
  uint16_t sequence_count = 0;
  if (!subtable->ReadU16(&offset_coverage) ||
      !subtable->ReadU16(&sequence_count)) {
    return OTS_FAILURE();
  }

  const unsigned sequence_end = static_cast<unsigned>(2 * 2) +
      sequence_count * kMathValueRecordSize;
  if (sequence_end > std::numeric_limits<uint16_t>::max()) {
    return OTS_FAILURE();
  }

  
  if (offset_coverage < sequence_end || offset_coverage >= length) {
    return OTS_FAILURE();
  }
  if (!ots::ParseCoverageTable(file, data + offset_coverage,
                               length - offset_coverage,
                               num_glyphs, sequence_count)) {
    return OTS_FAILURE();
  }

  
  for (unsigned i = 0; i < sequence_count; ++i) {
    if (!ParseMathValueRecord(file, subtable, data, length)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool ParseMathItalicsCorrectionInfoTable(const ots::OpenTypeFile *file,
                                         const uint8_t *data,
                                         size_t length,
                                         const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);
  return ParseMathValueRecordSequenceForGlyphs(file, &subtable, data, length,
                                               num_glyphs);
}

bool ParseMathTopAccentAttachmentTable(const ots::OpenTypeFile *file,
                                       const uint8_t *data,
                                       size_t length,
                                       const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);
  return ParseMathValueRecordSequenceForGlyphs(file, &subtable, data, length,
                                               num_glyphs);
}

bool ParseMathKernTable(const ots::OpenTypeFile *file,
                        const uint8_t *data, size_t length) {
  ots::Buffer subtable(data, length);

  
  uint16_t height_count = 0;
  if (!subtable.ReadU16(&height_count)) {
    return OTS_FAILURE();
  }

  
  for (unsigned i = 0; i < height_count; ++i) {
    if (!ParseMathValueRecord(file, &subtable, data, length)) {
      return OTS_FAILURE();
    }
  }

  
  for (unsigned i = 0; i <= height_count; ++i) {
    if (!ParseMathValueRecord(file, &subtable, data, length)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool ParseMathKernInfoTable(const ots::OpenTypeFile *file,
                            const uint8_t *data, size_t length,
                            const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  uint16_t offset_coverage = 0;
  uint16_t sequence_count = 0;
  if (!subtable.ReadU16(&offset_coverage) ||
      !subtable.ReadU16(&sequence_count)) {
    return OTS_FAILURE();
  }

  const unsigned sequence_end = static_cast<unsigned>(2 * 2) +
    sequence_count * 4 * 2;
  if (sequence_end > std::numeric_limits<uint16_t>::max()) {
    return OTS_FAILURE();
  }

  
  if (offset_coverage < sequence_end || offset_coverage >= length) {
    return OTS_FAILURE();
  }
  if (!ots::ParseCoverageTable(file, data + offset_coverage, length - offset_coverage,
                               num_glyphs, sequence_count)) {
    return OTS_FAILURE();
  }

  
  for (unsigned i = 0; i < sequence_count; ++i) {
    
    for (unsigned j = 0; j < 4; ++j) {
      uint16_t offset_math_kern = 0;
      if (!subtable.ReadU16(&offset_math_kern)) {
        return OTS_FAILURE();
      }
      if (offset_math_kern) {
        if (offset_math_kern < sequence_end || offset_math_kern >= length ||
            !ParseMathKernTable(file, data + offset_math_kern,
                                length - offset_math_kern)) {
          return OTS_FAILURE();
        }
      }
    }
  }

  return true;
}

bool ParseMathGlyphInfoTable(const ots::OpenTypeFile *file,
                             const uint8_t *data, size_t length,
                             const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  uint16_t offset_math_italics_correction_info = 0;
  uint16_t offset_math_top_accent_attachment = 0;
  uint16_t offset_extended_shaped_coverage = 0;
  uint16_t offset_math_kern_info = 0;
  if (!subtable.ReadU16(&offset_math_italics_correction_info) ||
      !subtable.ReadU16(&offset_math_top_accent_attachment) ||
      !subtable.ReadU16(&offset_extended_shaped_coverage) ||
      !subtable.ReadU16(&offset_math_kern_info)) {
    return OTS_FAILURE();
  }

  
  
  
  
  if (offset_math_italics_correction_info) {
    if (offset_math_italics_correction_info >= length ||
        offset_math_italics_correction_info < kMathGlyphInfoHeaderSize ||
        !ParseMathItalicsCorrectionInfoTable(
            file, data + offset_math_italics_correction_info,
            length - offset_math_italics_correction_info,
            num_glyphs)) {
      return OTS_FAILURE();
    }
  }
  if (offset_math_top_accent_attachment) {
    if (offset_math_top_accent_attachment >= length ||
        offset_math_top_accent_attachment < kMathGlyphInfoHeaderSize ||
        !ParseMathTopAccentAttachmentTable(file, data +
                                           offset_math_top_accent_attachment,
                                           length -
                                           offset_math_top_accent_attachment,
                                           num_glyphs)) {
      return OTS_FAILURE();
    }
  }
  if (offset_extended_shaped_coverage) {
    if (offset_extended_shaped_coverage >= length ||
        offset_extended_shaped_coverage < kMathGlyphInfoHeaderSize ||
        !ots::ParseCoverageTable(file, data + offset_extended_shaped_coverage,
                                 length - offset_extended_shaped_coverage,
                                 num_glyphs)) {
      return OTS_FAILURE();
    }
  }
  if (offset_math_kern_info) {
    if (offset_math_kern_info >= length ||
        offset_math_kern_info < kMathGlyphInfoHeaderSize ||
        !ParseMathKernInfoTable(file, data + offset_math_kern_info,
                                length - offset_math_kern_info, num_glyphs)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool ParseGlyphAssemblyTable(const ots::OpenTypeFile *file,
                             const uint8_t *data,
                             size_t length, const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  uint16_t part_count = 0;
  if (!ParseMathValueRecord(file, &subtable, data, length) ||
      !subtable.ReadU16(&part_count)) {
    return OTS_FAILURE();
  }

  const unsigned sequence_end = kMathValueRecordSize +
    static_cast<unsigned>(2) + part_count * kGlyphPartRecordSize;
  if (sequence_end > std::numeric_limits<uint16_t>::max()) {
    return OTS_FAILURE();
  }

  
  for (unsigned i = 0; i < part_count; ++i) {
    uint16_t glyph = 0;
    uint16_t part_flags = 0;
    if (!subtable.ReadU16(&glyph) ||
        !subtable.Skip(2 * 3) ||
        !subtable.ReadU16(&part_flags)) {
      return OTS_FAILURE();
    }
    if (glyph >= num_glyphs) {
      return OTS_FAILURE_MSG("bad glyph ID: %u", glyph);
    }
    if (part_flags & ~0x00000001) {
      return OTS_FAILURE_MSG("unknown part flag: %u", part_flags);
    }
  }

  return true;
}

bool ParseMathGlyphConstructionTable(const ots::OpenTypeFile *file,
                                     const uint8_t *data,
                                     size_t length, const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  uint16_t offset_glyph_assembly = 0;
  uint16_t variant_count = 0;
  if (!subtable.ReadU16(&offset_glyph_assembly) ||
      !subtable.ReadU16(&variant_count)) {
    return OTS_FAILURE();
  }

  const unsigned sequence_end = static_cast<unsigned>(2 * 2) +
    variant_count * 2 * 2;
  if (sequence_end > std::numeric_limits<uint16_t>::max()) {
    return OTS_FAILURE();
  }

  
  if (offset_glyph_assembly) {
    if (offset_glyph_assembly >= length ||
        offset_glyph_assembly < sequence_end) {
      return OTS_FAILURE();
    }
    if (!ParseGlyphAssemblyTable(file, data + offset_glyph_assembly,
                                 length - offset_glyph_assembly, num_glyphs)) {
      return OTS_FAILURE();
    }
  }

  
  for (unsigned i = 0; i < variant_count; ++i) {
    uint16_t glyph = 0;
    if (!subtable.ReadU16(&glyph) ||
        !subtable.Skip(2)) {
      return OTS_FAILURE();
    }
    if (glyph >= num_glyphs) {
      return OTS_FAILURE_MSG("bad glyph ID: %u", glyph);
    }
  }

  return true;
}

bool ParseMathGlyphConstructionSequence(const ots::OpenTypeFile *file,
                                        ots::Buffer* subtable,
                                        const uint8_t *data,
                                        size_t length,
                                        const uint16_t num_glyphs,
                                        uint16_t offset_coverage,
                                        uint16_t glyph_count,
                                        const unsigned sequence_end) {
  
  if (offset_coverage < sequence_end || offset_coverage >= length) {
    return OTS_FAILURE();
  }
  if (!ots::ParseCoverageTable(file, data + offset_coverage,
                               length - offset_coverage,
                               num_glyphs, glyph_count)) {
    return OTS_FAILURE();
  }

  
  for (unsigned i = 0; i < glyph_count; ++i) {
      uint16_t offset_glyph_construction = 0;
      if (!subtable->ReadU16(&offset_glyph_construction)) {
        return OTS_FAILURE();
      }
      if (offset_glyph_construction < sequence_end ||
          offset_glyph_construction >= length ||
          !ParseMathGlyphConstructionTable(file, data + offset_glyph_construction,
                                           length - offset_glyph_construction,
                                           num_glyphs)) {
        return OTS_FAILURE();
      }
  }

  return true;
}

bool ParseMathVariantsTable(const ots::OpenTypeFile *file,
                            const uint8_t *data,
                            size_t length, const uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  uint16_t offset_vert_glyph_coverage = 0;
  uint16_t offset_horiz_glyph_coverage = 0;
  uint16_t vert_glyph_count = 0;
  uint16_t horiz_glyph_count = 0;
  if (!subtable.Skip(2) ||  
      !subtable.ReadU16(&offset_vert_glyph_coverage) ||
      !subtable.ReadU16(&offset_horiz_glyph_coverage) ||
      !subtable.ReadU16(&vert_glyph_count) ||
      !subtable.ReadU16(&horiz_glyph_count)) {
    return OTS_FAILURE();
  }

  const unsigned sequence_end = 5 * 2 + vert_glyph_count * 2 +
    horiz_glyph_count * 2;
  if (sequence_end > std::numeric_limits<uint16_t>::max()) {
    return OTS_FAILURE();
  }

  if (!ParseMathGlyphConstructionSequence(file, &subtable, data, length, num_glyphs,
                                          offset_vert_glyph_coverage,
                                          vert_glyph_count,
                                          sequence_end) ||
      !ParseMathGlyphConstructionSequence(file, &subtable, data, length, num_glyphs,
                                          offset_horiz_glyph_coverage,
                                          horiz_glyph_count,
                                          sequence_end)) {
    return OTS_FAILURE();
  }

  return true;
}

}  

#define DROP_THIS_TABLE(msg_) \
  do { \
    file->math->data = 0; \
    file->math->length = 0; \
    OTS_FAILURE_MSG(msg_ ", table discarded"); \
  } while (0)

namespace ots {

bool ots_math_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  
  
  if (!file->maxp) {
    return OTS_FAILURE();
  }
  const uint16_t num_glyphs = file->maxp->num_glyphs;

  Buffer table(data, length);

  OpenTypeMATH* math = new OpenTypeMATH;
  file->math = math;

  uint32_t version = 0;
  if (!table.ReadU32(&version)) {
    return OTS_FAILURE();
  }
  if (version != 0x00010000) {
    DROP_THIS_TABLE("bad MATH version");
    return true;
  }

  uint16_t offset_math_constants = 0;
  uint16_t offset_math_glyph_info = 0;
  uint16_t offset_math_variants = 0;
  if (!table.ReadU16(&offset_math_constants) ||
      !table.ReadU16(&offset_math_glyph_info) ||
      !table.ReadU16(&offset_math_variants)) {
    return OTS_FAILURE();
  }

  if (offset_math_constants >= length ||
      offset_math_constants < kMathHeaderSize ||
      offset_math_glyph_info >= length ||
      offset_math_glyph_info < kMathHeaderSize ||
      offset_math_variants >= length ||
      offset_math_variants < kMathHeaderSize) {
    DROP_THIS_TABLE("bad offset in MATH header");
    return true;
  }

  if (!ParseMathConstantsTable(file, data + offset_math_constants,
                               length - offset_math_constants)) {
    DROP_THIS_TABLE("failed to parse MathConstants table");
    return true;
  }
  if (!ParseMathGlyphInfoTable(file, data + offset_math_glyph_info,
                               length - offset_math_glyph_info, num_glyphs)) {
    DROP_THIS_TABLE("failed to parse MathGlyphInfo table");
    return true;
  }
  if (!ParseMathVariantsTable(file, data + offset_math_variants,
                              length - offset_math_variants, num_glyphs)) {
    DROP_THIS_TABLE("failed to parse MathVariants table");
    return true;
  }

  math->data = data;
  math->length = length;
  return true;
}

bool ots_math_should_serialise(OpenTypeFile *file) {
  return file->math != NULL && file->math->data != NULL;
}

bool ots_math_serialise(OTSStream *out, OpenTypeFile *file) {
  if (!out->Write(file->math->data, file->math->length)) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_math_free(OpenTypeFile *file) {
  delete file->math;
}

}  

#undef TABLE_NAME
#undef DROP_THIS_TABLE
