



#include "cmap.h"

#include <set>
#include <utility>
#include <vector>

#include "maxp.h"
#include "os2.h"




namespace {

struct CMAPSubtableHeader {
  uint16_t platform;
  uint16_t encoding;
  uint32_t offset;
  uint16_t format;
  uint32_t length;
};

struct Subtable314Range {
  uint16_t start_range;
  uint16_t end_range;
  int16_t id_delta;
  uint16_t id_range_offset;
  uint32_t id_range_offset_offset;
};



const unsigned kMaxCMAPGroups = 0xFFFF;


const size_t kFormat0ArraySize = 256;


const uint32_t kUnicodeUpperLimit = 0x10FFFF;


bool Parse3x4(ots::OpenTypeFile *file, int encoding,
              const uint8_t *data, size_t length, uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  
  

  if (!file->os2) {
    return OTS_FAILURE();
  }

  if (!subtable.Skip(4)) {
    return OTS_FAILURE();
  }
  uint16_t language = 0;
  if (!subtable.ReadU16(&language)) {
    return OTS_FAILURE();
  }
  if (language) {
    
    return OTS_FAILURE();
  }

  uint16_t segcountx2, search_range, entry_selector, range_shift;
  segcountx2 = search_range = entry_selector = range_shift = 0;
  if (!subtable.ReadU16(&segcountx2) ||
      !subtable.ReadU16(&search_range) ||
      !subtable.ReadU16(&entry_selector) ||
      !subtable.ReadU16(&range_shift)) {
    return OTS_FAILURE();
  }

  if (segcountx2 & 1 || search_range & 1) {
    return OTS_FAILURE();
  }
  const uint16_t segcount = segcountx2 >> 1;
  
  if (segcount < 1) {
    return OTS_FAILURE();
  }

  
  unsigned log2segcount = 0;
  while (1u << (log2segcount + 1) <= segcount) {
    log2segcount++;
  }

  const uint16_t expected_search_range = 2 * 1u << log2segcount;
  if (expected_search_range != search_range) {
    return OTS_FAILURE();
  }

  if (entry_selector != log2segcount) {
    return OTS_FAILURE();
  }

  const uint16_t expected_range_shift = segcountx2 - search_range;
  if (range_shift != expected_range_shift) {
    return OTS_FAILURE();
  }

  std::vector<Subtable314Range> ranges(segcount);

  for (unsigned i = 0; i < segcount; ++i) {
    if (!subtable.ReadU16(&ranges[i].end_range)) {
      return OTS_FAILURE();
    }
  }

  uint16_t padding;
  if (!subtable.ReadU16(&padding)) {
    return OTS_FAILURE();
  }
  if (padding) {
    return OTS_FAILURE();
  }

  for (unsigned i = 0; i < segcount; ++i) {
    if (!subtable.ReadU16(&ranges[i].start_range)) {
      return OTS_FAILURE();
    }
  }
  for (unsigned i = 0; i < segcount; ++i) {
    if (!subtable.ReadS16(&ranges[i].id_delta)) {
      return OTS_FAILURE();
    }
  }
  for (unsigned i = 0; i < segcount; ++i) {
    ranges[i].id_range_offset_offset = subtable.offset();
    if (!subtable.ReadU16(&ranges[i].id_range_offset)) {
      return OTS_FAILURE();
    }

    if (ranges[i].id_range_offset & 1) {
      
      
      
      if (i == segcount - 1u) {
        OTS_WARNING("bad id_range_offset");
        ranges[i].id_range_offset = 0;
        
        
      } else {
        return OTS_FAILURE();
      }
    }
  }

  
  
  for (unsigned i = 1; i < segcount; ++i) {
    if ((i == segcount - 1u) &&
        (ranges[i - 1].start_range == 0xffff) &&
        (ranges[i - 1].end_range == 0xffff) &&
        (ranges[i].start_range == 0xffff) &&
        (ranges[i].end_range == 0xffff)) {
      
      
      OTS_WARNING("multiple 0xffff terminators found");
      continue;
    }

    
    
    if (ranges[i].end_range <= ranges[i - 1].end_range) {
      return OTS_FAILURE();
    }
    if (ranges[i].start_range <= ranges[i - 1].end_range) {
      return OTS_FAILURE();
    }

    
    
    if (file->os2->first_char_index != 0xFFFF &&
        ranges[i].start_range != 0xFFFF &&
        file->os2->first_char_index > ranges[i].start_range) {
      file->os2->first_char_index = ranges[i].start_range;
    }
    if (file->os2->last_char_index != 0xFFFF &&
        ranges[i].end_range != 0xFFFF &&
        file->os2->last_char_index < ranges[i].end_range) {
      file->os2->last_char_index = ranges[i].end_range;
    }
  }

  
  if (ranges[segcount - 1].end_range != 0xffff) {
    return OTS_FAILURE();
  }

  
  
  
  for (unsigned i = 1; i < segcount; ++i) {
    for (unsigned cp = ranges[i].start_range; cp <= ranges[i].end_range; ++cp) {
      const uint16_t code_point = cp;
      if (ranges[i].id_range_offset == 0) {
        
        const uint16_t glyph = code_point + ranges[i].id_delta;
        if (glyph >= num_glyphs) {
          return OTS_FAILURE();
        }
      } else {
        const uint16_t range_delta = code_point - ranges[i].start_range;
        
        
        const uint32_t glyph_id_offset = ranges[i].id_range_offset_offset +
                                         ranges[i].id_range_offset +
                                         range_delta * 2;
        
        if (glyph_id_offset + 1 >= length) {
          return OTS_FAILURE();
        }
        uint16_t glyph;
        memcpy(&glyph, data + glyph_id_offset, 2);
        glyph = ntohs(glyph);
        if (glyph >= num_glyphs) {
          return OTS_FAILURE();
        }
      }
    }
  }

  
  
  if (encoding == 0) {
    file->cmap->subtable_3_0_4_data = data;
    file->cmap->subtable_3_0_4_length = length;
  } else if (encoding == 1) {
    file->cmap->subtable_3_1_4_data = data;
    file->cmap->subtable_3_1_4_length = length;
  } else {
    return OTS_FAILURE();
  }

  return true;
}

bool Parse31012(ots::OpenTypeFile *file,
                const uint8_t *data, size_t length, uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  

  if (!subtable.Skip(8)) {
    return OTS_FAILURE();
  }
  uint32_t language = 0;
  if (!subtable.ReadU32(&language)) {
    return OTS_FAILURE();
  }
  if (language) {
    return OTS_FAILURE();
  }

  uint32_t num_groups = 0;
  if (!subtable.ReadU32(&num_groups)) {
    return OTS_FAILURE();
  }
  if (num_groups == 0 || num_groups > kMaxCMAPGroups) {
    return OTS_FAILURE();
  }

  std::vector<ots::OpenTypeCMAPSubtableRange> &groups
      = file->cmap->subtable_3_10_12;
  groups.resize(num_groups);

  for (unsigned i = 0; i < num_groups; ++i) {
    if (!subtable.ReadU32(&groups[i].start_range) ||
        !subtable.ReadU32(&groups[i].end_range) ||
        !subtable.ReadU32(&groups[i].start_glyph_id)) {
      return OTS_FAILURE();
    }

    if (groups[i].start_range > kUnicodeUpperLimit ||
        groups[i].end_range > kUnicodeUpperLimit ||
        groups[i].start_glyph_id > 0xFFFF) {
      return OTS_FAILURE();
    }

    
    if (groups[i].start_range >= 0xD800 &&
        groups[i].start_range <= 0xDFFF) {
      return OTS_FAILURE();
    }
    if (groups[i].end_range >= 0xD800 &&
        groups[i].end_range <= 0xDFFF) {
      return OTS_FAILURE();
    }
    if (groups[i].start_range < 0xD800 &&
        groups[i].end_range > 0xDFFF) {
      return OTS_FAILURE();
    }

    
    
    if (groups[i].end_range < groups[i].start_range) {
      return OTS_FAILURE();
    }
    if ((groups[i].end_range - groups[i].start_range) +
        groups[i].start_glyph_id > num_glyphs) {
      return OTS_FAILURE();
    }
  }

  
  for (unsigned i = 1; i < num_groups; ++i) {
    if (groups[i].start_range <= groups[i - 1].start_range) {
      return OTS_FAILURE();
    }
    if (groups[i].start_range <= groups[i - 1].end_range) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool Parse31013(ots::OpenTypeFile *file,
                const uint8_t *data, size_t length, uint16_t num_glyphs) {
  ots::Buffer subtable(data, length);

  
  

  if (!subtable.Skip(8)) {
    return OTS_FAILURE();
  }
  uint16_t language = 0;
  if (!subtable.ReadU16(&language)) {
    return OTS_FAILURE();
  }
  if (language) {
    return OTS_FAILURE();
  }

  uint32_t num_groups = 0;
  if (!subtable.ReadU32(&num_groups)) {
    return OTS_FAILURE();
  }

  
  
  if (num_groups == 0 || num_groups > kMaxCMAPGroups) {
    return OTS_FAILURE();
  }

  std::vector<ots::OpenTypeCMAPSubtableRange> &groups
      = file->cmap->subtable_3_10_13;
  groups.resize(num_groups);

  for (unsigned i = 0; i < num_groups; ++i) {
    if (!subtable.ReadU32(&groups[i].start_range) ||
        !subtable.ReadU32(&groups[i].end_range) ||
        !subtable.ReadU32(&groups[i].start_glyph_id)) {
      return OTS_FAILURE();
    }

    
    
    if (groups[i].start_range > kUnicodeUpperLimit ||
        groups[i].end_range > kUnicodeUpperLimit ||
        groups[i].start_glyph_id > 0xFFFF) {
      return OTS_FAILURE();
    }

    if (groups[i].start_glyph_id >= num_glyphs) {
      return OTS_FAILURE();
    }
  }

  
  for (unsigned i = 1; i < num_groups; ++i) {
    if (groups[i].start_range <= groups[i - 1].start_range) {
      return OTS_FAILURE();
    }
    if (groups[i].start_range <= groups[i - 1].end_range) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool Parse100(ots::OpenTypeFile *file, const uint8_t *data, size_t length) {
  
  ots::Buffer subtable(data, length);

  if (!subtable.Skip(4)) {
    return OTS_FAILURE();
  }
  uint16_t language = 0;
  if (!subtable.ReadU16(&language)) {
    return OTS_FAILURE();
  }
  if (language) {
    
    OTS_WARNING("language id should be zero: %u", language);
  }

  file->cmap->subtable_1_0_0.reserve(kFormat0ArraySize);
  for (size_t i = 0; i < kFormat0ArraySize; ++i) {
    uint8_t glyph_id = 0;
    if (!subtable.ReadU8(&glyph_id)) {
      return OTS_FAILURE();
    }
    file->cmap->subtable_1_0_0.push_back(glyph_id);
  }

  return true;
}

}  

namespace ots {

bool ots_cmap_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  file->cmap = new OpenTypeCMAP;

  uint16_t version = 0;
  uint16_t num_tables = 0;
  if (!table.ReadU16(&version) ||
      !table.ReadU16(&num_tables)) {
    return OTS_FAILURE();
  }

  if (version != 0) {
    return OTS_FAILURE();
  }
  if (!num_tables) {
    return OTS_FAILURE();
  }

  std::vector<CMAPSubtableHeader> subtable_headers;

  
  subtable_headers.reserve(num_tables);
  for (unsigned i = 0; i < num_tables; ++i) {
    CMAPSubtableHeader subt;

    if (!table.ReadU16(&subt.platform) ||
        !table.ReadU16(&subt.encoding) ||
        !table.ReadU32(&subt.offset)) {
      return OTS_FAILURE();
    }

    subtable_headers.push_back(subt);
  }

  const size_t data_offset = table.offset();

  
  uint32_t last_id = 0;
  for (unsigned i = 0; i < num_tables; ++i) {
    if (subtable_headers[i].offset > 1024 * 1024 * 1024) {
      return OTS_FAILURE();
    }
    if (subtable_headers[i].offset < data_offset ||
        subtable_headers[i].offset >= length) {
      return OTS_FAILURE();
    }

    
    uint32_t current_id
        = (subtable_headers[i].platform << 16) + subtable_headers[i].encoding;
    if ((i != 0) && (last_id >= current_id)) {
      return OTS_FAILURE();
    }
    last_id = current_id;
  }

  
  
  for (unsigned i = 0; i < num_tables; ++i) {
    table.set_offset(subtable_headers[i].offset);
    if (!table.ReadU16(&subtable_headers[i].format)) {
      return OTS_FAILURE();
    }

    uint16_t len = 0;
    switch (subtable_headers[i].format) {
      case 0:
      case 4:
        if (!table.ReadU16(&len)) {
          return OTS_FAILURE();
        }
        subtable_headers[i].length = len;
        break;
      case 12:
      case 13:
        if (!table.Skip(2)) {
          return OTS_FAILURE();
        }
        if (!table.ReadU32(&subtable_headers[i].length)) {
          return OTS_FAILURE();
        }
        break;
      default:
        subtable_headers[i].length = 0;
        break;
    }
  }

  
  for (unsigned i = 0; i < num_tables; ++i) {
    if (!subtable_headers[i].length) continue;
    if (subtable_headers[i].length > 1024 * 1024 * 1024) {
      return OTS_FAILURE();
    }
    
    
    const uint32_t end_byte
        = subtable_headers[i].offset + subtable_headers[i].length;
    if (end_byte > length) {
      return OTS_FAILURE();
    }
  }

  
  std::set<std::pair<uint32_t, uint32_t> > uniq_checker;
  std::vector<std::pair<uint32_t, uint8_t> > overlap_checker;
  for (unsigned i = 0; i < num_tables; ++i) {
    const uint32_t end_byte
        = subtable_headers[i].offset + subtable_headers[i].length;

    if (!uniq_checker.insert(std::make_pair(subtable_headers[i].offset,
                                            end_byte)).second) {
      
      
      continue;
    }
    overlap_checker.push_back(
        std::make_pair(subtable_headers[i].offset, 1 ));
    overlap_checker.push_back(
        std::make_pair(end_byte, 0 ));
  }
  std::sort(overlap_checker.begin(), overlap_checker.end());
  int overlap_count = 0;
  for (unsigned i = 0; i < overlap_checker.size(); ++i) {
    overlap_count += (overlap_checker[i].second ? 1 : -1);
    if (overlap_count > 1) {
      return OTS_FAILURE();
    }
  }

  
  
  if (!file->maxp) {
    return OTS_FAILURE();
  }
  const uint16_t num_glyphs = file->maxp->num_glyphs;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  for (unsigned i = 0; i < num_tables; ++i) {
    if (subtable_headers[i].platform == 0) {
      

      if ((subtable_headers[i].encoding == 0) &&
          (subtable_headers[i].format == 4)) {
        
        
        
        
        if (!Parse3x4(file, 1, data + subtable_headers[i].offset,
                      subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      } else if ((subtable_headers[i].encoding == 3) &&
                 (subtable_headers[i].format == 4)) {
        
        if (!Parse3x4(file, 1, data + subtable_headers[i].offset,
                      subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      } else if ((subtable_headers[i].encoding == 3) &&
                 (subtable_headers[i].format == 12)) {
        
        if (!Parse31012(file, data + subtable_headers[i].offset,
                        subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      }

    } else if (subtable_headers[i].platform == 1) {
      

      if ((subtable_headers[i].encoding == 0) &&
          (subtable_headers[i].format == 0)) {
        
        if (!Parse100(file, data + subtable_headers[i].offset,
                      subtable_headers[i].length)) {
          return OTS_FAILURE();
        }
      }

    } else if (subtable_headers[i].platform == 3) {
      

      switch (subtable_headers[i].encoding) {
        case 0:
        case 1:
          if (subtable_headers[i].format == 4) {
            
            if (!Parse3x4(file, subtable_headers[i].encoding,
                          data + subtable_headers[i].offset,
                          subtable_headers[i].length, num_glyphs)) {
              return OTS_FAILURE();
            }
          }
          break;
        case 10:
          if (subtable_headers[i].format == 12) {
            file->cmap->subtable_3_10_12.clear();
            if (!Parse31012(file, data + subtable_headers[i].offset,
                            subtable_headers[i].length, num_glyphs)) {
              return OTS_FAILURE();
            }
          } else if (subtable_headers[i].format == 13) {
            file->cmap->subtable_3_10_13.clear();
            if (!Parse31013(file, data + subtable_headers[i].offset,
                            subtable_headers[i].length, num_glyphs)) {
              return OTS_FAILURE();
            }
          }
          break;
      }
    }
  }

  return true;
}

bool ots_cmap_should_serialise(OpenTypeFile *file) {
  return file->cmap;
}

bool ots_cmap_serialise(OTSStream *out, OpenTypeFile *file) {
  const bool have_100 = file->cmap->subtable_1_0_0.size();
  const bool have_304 = file->cmap->subtable_3_0_4_data;
  
  
  const bool have_314 = (!have_304) && file->cmap->subtable_3_1_4_data;
  const bool have_31012 = file->cmap->subtable_3_10_12.size();
  const bool have_31013 = file->cmap->subtable_3_10_13.size();
  const unsigned num_subtables = static_cast<unsigned>(have_100) +
                                 static_cast<unsigned>(have_304) +
                                 static_cast<unsigned>(have_314) +
                                 static_cast<unsigned>(have_31012) +
                                 static_cast<unsigned>(have_31013);
  const off_t table_start = out->Tell();

  
  
  if (!have_304 && !have_314) {
    return OTS_FAILURE();
  }

  if (!out->WriteU16(0) ||
      !out->WriteU16(num_subtables)) {
    return OTS_FAILURE();
  }

  const off_t record_offset = out->Tell();
  if (!out->Pad(num_subtables * 8)) {
    return OTS_FAILURE();
  }

  const off_t offset_100 = out->Tell();
  if (have_100) {
    if (!out->WriteU16(0) ||  
        !out->WriteU16(6 + kFormat0ArraySize) ||  
        !out->WriteU16(0)) {  
      return OTS_FAILURE();
    }
    if (!out->Write(&(file->cmap->subtable_1_0_0[0]), kFormat0ArraySize)) {
      return OTS_FAILURE();
    }
  }

  const off_t offset_304 = out->Tell();
  if (have_304) {
    if (!out->Write(file->cmap->subtable_3_0_4_data,
                    file->cmap->subtable_3_0_4_length)) {
      return OTS_FAILURE();
    }
  }

  const off_t offset_314 = out->Tell();
  if (have_314) {
    if (!out->Write(file->cmap->subtable_3_1_4_data,
                    file->cmap->subtable_3_1_4_length)) {
      return OTS_FAILURE();
    }
  }

  const off_t offset_31012 = out->Tell();
  if (have_31012) {
    std::vector<OpenTypeCMAPSubtableRange> &groups
        = file->cmap->subtable_3_10_12;
    const unsigned num_groups = groups.size();
    if (!out->WriteU16(12) ||
        !out->WriteU16(0) ||
        !out->WriteU32(num_groups * 12 + 16) ||
        !out->WriteU32(0) ||
        !out->WriteU32(num_groups)) {
      return OTS_FAILURE();
    }

    for (unsigned i = 0; i < num_groups; ++i) {
      if (!out->WriteU32(groups[i].start_range) ||
          !out->WriteU32(groups[i].end_range) ||
          !out->WriteU32(groups[i].start_glyph_id)) {
        return OTS_FAILURE();
      }
    }
  }

  const off_t offset_31013 = out->Tell();
  if (have_31013) {
    std::vector<OpenTypeCMAPSubtableRange> &groups
        = file->cmap->subtable_3_10_13;
    const unsigned num_groups = groups.size();
    if (!out->WriteU16(13) ||
        !out->WriteU16(0) ||
        !out->WriteU32(num_groups * 12 + 14) ||
        !out->WriteU32(0) ||
        !out->WriteU32(num_groups)) {
      return OTS_FAILURE();
    }

    for (unsigned i = 0; i < num_groups; ++i) {
      if (!out->WriteU32(groups[i].start_range) ||
          !out->WriteU32(groups[i].end_range) ||
          !out->WriteU32(groups[i].start_glyph_id)) {
        return OTS_FAILURE();
      }
    }
  }

  const off_t table_end = out->Tell();
  
  
  OTSStream::ChecksumState saved_checksum = out->SaveChecksumState();
  out->ResetChecksum();

  
  if (!out->Seek(record_offset)) {
    return OTS_FAILURE();
  }

  if (have_100) {
    if (!out->WriteU16(1) ||
        !out->WriteU16(0) ||
        !out->WriteU32(offset_100 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (have_304) {
    if (!out->WriteU16(3) ||
        !out->WriteU16(0) ||
        !out->WriteU32(offset_304 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (have_314) {
    if (!out->WriteU16(3) ||
        !out->WriteU16(1) ||
        !out->WriteU32(offset_314 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (have_31012) {
    if (!out->WriteU16(3) ||
        !out->WriteU16(10) ||
        !out->WriteU32(offset_31012 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (have_31013) {
    if (!out->WriteU16(3) ||
        !out->WriteU16(10) ||
        !out->WriteU32(offset_31013 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (!out->Seek(table_end)) {
    return OTS_FAILURE();
  }
  out->RestoreChecksum(saved_checksum);

  return true;
}

void ots_cmap_free(OpenTypeFile *file) {
  delete file->cmap;
}

}  
