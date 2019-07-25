



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


const uint32_t kMaxCMAPSelectorRecords = 259;




const uint32_t kMongolianVSStart = 0x180B;
const uint32_t kMongolianVSEnd = 0x180D;
const uint32_t kVSStart = 0xFE00;
const uint32_t kVSEnd = 0xFE0F;
const uint32_t kIVSStart = 0xE0100;
const uint32_t kIVSEnd = 0xE01EF;
const uint32_t kUVSUpperLimit = 0xFFFFFF;


bool ParseFormat4(ots::OpenTypeFile *file, int platform, int encoding,
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

  
  
  if (platform == 3 && encoding == 0) {
    file->cmap->subtable_3_0_4_data = data;
    file->cmap->subtable_3_0_4_length = length;
  } else if (platform == 3 && encoding == 1) {
    file->cmap->subtable_3_1_4_data = data;
    file->cmap->subtable_3_1_4_length = length;
  } else if (platform == 0 && encoding == 3) {
    file->cmap->subtable_0_3_4_data = data;
    file->cmap->subtable_0_3_4_length = length;
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

bool Parse0514(ots::OpenTypeFile *file,
               const uint8_t *data, size_t length, uint16_t num_glyphs) {
  
  ots::Buffer subtable(data, length);

  
  

  
  if (!subtable.Skip(6)) {
    return OTS_FAILURE();
  }

  uint32_t num_records = 0;
  if (!subtable.ReadU32(&num_records)) {
    return OTS_FAILURE();
  }
  if (num_records == 0 || num_records > kMaxCMAPSelectorRecords) {
    return OTS_FAILURE();
  }

  std::vector<ots::OpenTypeCMAPSubtableVSRecord>& records
      = file->cmap->subtable_0_5_14;
  records.resize(num_records);

  for (unsigned i = 0; i < num_records; ++i) {
    if (!subtable.ReadU24(&records[i].var_selector) ||
        !subtable.ReadU32(&records[i].default_offset) ||
        !subtable.ReadU32(&records[i].non_default_offset)) {
      return OTS_FAILURE();
    }
    
    if (!((records[i].var_selector >= kMongolianVSStart &&
           records[i].var_selector <= kMongolianVSEnd) ||
          (records[i].var_selector >= kVSStart &&
           records[i].var_selector <= kVSEnd) ||
          (records[i].var_selector >= kIVSStart &&
           records[i].var_selector <= kIVSEnd))) {
      return OTS_FAILURE();
    }
    if (i > 0 &&
        records[i-1].var_selector >= records[i].var_selector) {
      return OTS_FAILURE();
    }

    
    if (!records[i].default_offset && !records[i].non_default_offset) {
      return OTS_FAILURE();
    }
    if (records[i].default_offset &&
        records[i].default_offset >= length) {
      return OTS_FAILURE();
    }
    if (records[i].non_default_offset &&
        records[i].non_default_offset >= length) {
      return OTS_FAILURE();
    }
  }

  for (unsigned i = 0; i < num_records; ++i) {
    
    if (records[i].default_offset) {
      subtable.set_offset(records[i].default_offset);
      uint32_t num_ranges = 0;
      if (!subtable.ReadU32(&num_ranges)) {
        return OTS_FAILURE();
      }
      if (!num_ranges || num_ranges > kMaxCMAPGroups) {
        return OTS_FAILURE();
      }

      uint32_t last_unicode_value = 0;
      std::vector<ots::OpenTypeCMAPSubtableVSRange>& ranges
          = records[i].ranges;
      ranges.resize(num_ranges);

      for (unsigned j = 0; j < num_ranges; ++j) {
        if (!subtable.ReadU24(&ranges[j].unicode_value) ||
            !subtable.ReadU8(&ranges[j].additional_count)) {
          return OTS_FAILURE();
        }
        const uint32_t check_value =
            ranges[j].unicode_value + ranges[i].additional_count;
        if (ranges[j].unicode_value == 0 ||
            ranges[j].unicode_value > kUnicodeUpperLimit ||
            check_value > kUVSUpperLimit ||
            (last_unicode_value &&
             ranges[j].unicode_value <= last_unicode_value)) {
          return OTS_FAILURE();
        }
        last_unicode_value = check_value;
      }
    }

    
    if (records[i].non_default_offset) {
      subtable.set_offset(records[i].non_default_offset);
      uint32_t num_mappings = 0;
      if (!subtable.ReadU32(&num_mappings)) {
        return OTS_FAILURE();
      }
      if (!num_mappings || num_mappings > kMaxCMAPGroups) {
        return OTS_FAILURE();
      }

      uint32_t last_unicode_value = 0;
      std::vector<ots::OpenTypeCMAPSubtableVSMapping>& mappings
          = records[i].mappings;
      mappings.resize(num_mappings);

      for (unsigned j = 0; j < num_mappings; ++j) {
        if (!subtable.ReadU24(&mappings[j].unicode_value) ||
            !subtable.ReadU16(&mappings[j].glyph_id)) {
          return OTS_FAILURE();
        }
        if (mappings[j].glyph_id == 0 ||
            mappings[j].unicode_value == 0 ||
            mappings[j].unicode_value > kUnicodeUpperLimit ||
            (last_unicode_value &&
             mappings[j].unicode_value <= last_unicode_value)) {
          return OTS_FAILURE();
        }
        last_unicode_value = mappings[j].unicode_value;
      }
    }
  }

  if (subtable.offset() != length) {
    return OTS_FAILURE();
  }
  file->cmap->subtable_0_5_14_length = subtable.offset();
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
      case 14:
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
        
        
        
        
        if (!ParseFormat4(file, 3, 1, data + subtable_headers[i].offset,
                      subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      } else if ((subtable_headers[i].encoding == 3) &&
                 (subtable_headers[i].format == 4)) {
        
        if (!ParseFormat4(file, 0, 3, data + subtable_headers[i].offset,
                      subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      } else if ((subtable_headers[i].encoding == 3) &&
                 (subtable_headers[i].format == 12)) {
        
        if (!Parse31012(file, data + subtable_headers[i].offset,
                        subtable_headers[i].length, num_glyphs)) {
          return OTS_FAILURE();
        }
      } else if ((subtable_headers[i].encoding == 5) &&
                 (subtable_headers[i].format == 14)) {
        if (!Parse0514(file, data + subtable_headers[i].offset,
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
            
            if (!ParseFormat4(file, subtable_headers[i].platform,
                          subtable_headers[i].encoding,
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
  return file->cmap != NULL;
}

bool ots_cmap_serialise(OTSStream *out, OpenTypeFile *file) {
  const bool have_034 = file->cmap->subtable_0_3_4_data != NULL;
  const bool have_0514 = file->cmap->subtable_0_5_14.size() != 0;
  const bool have_100 = file->cmap->subtable_1_0_0.size() != 0;
  const bool have_304 = file->cmap->subtable_3_0_4_data != NULL;
  
  
  const bool have_314 = (!have_304) && file->cmap->subtable_3_1_4_data;
  const bool have_31012 = file->cmap->subtable_3_10_12.size() != 0;
  const bool have_31013 = file->cmap->subtable_3_10_13.size() != 0;
  const unsigned num_subtables = static_cast<unsigned>(have_034) +
                                 static_cast<unsigned>(have_0514) +
                                 static_cast<unsigned>(have_100) +
                                 static_cast<unsigned>(have_304) +
                                 static_cast<unsigned>(have_314) +
                                 static_cast<unsigned>(have_31012) +
                                 static_cast<unsigned>(have_31013);
  const off_t table_start = out->Tell();

  
  
  if (!have_304 && !have_314 && !have_034) {
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

  const off_t offset_034 = out->Tell();
  if (have_034) {
    if (!out->Write(file->cmap->subtable_0_3_4_data,
                    file->cmap->subtable_0_3_4_length)) {
      return OTS_FAILURE();
    }
  }

  const off_t offset_0514 = out->Tell();
  if (have_0514) {
    const std::vector<ots::OpenTypeCMAPSubtableVSRecord> &records
        = file->cmap->subtable_0_5_14;
    const unsigned num_records = records.size();
    if (!out->WriteU16(14) ||
        !out->WriteU32(file->cmap->subtable_0_5_14_length) ||
        !out->WriteU32(num_records)) {
      return OTS_FAILURE();
    }
    for (unsigned i = 0; i < num_records; ++i) {
      if (!out->WriteU24(records[i].var_selector) ||
          !out->WriteU32(records[i].default_offset) ||
          !out->WriteU32(records[i].non_default_offset)) {
        return OTS_FAILURE();
      }
    }
    for (unsigned i = 0; i < num_records; ++i) {
      if (records[i].default_offset) {
        const std::vector<ots::OpenTypeCMAPSubtableVSRange> &ranges
            = records[i].ranges;
        const unsigned num_ranges = ranges.size();
        if (!out->Seek(records[i].default_offset + offset_0514) ||
            !out->WriteU32(num_ranges)) {
          return OTS_FAILURE();
        }
        for (unsigned j = 0; j < num_ranges; ++j) {
          if (!out->WriteU24(ranges[j].unicode_value) ||
              !out->WriteU8(ranges[j].additional_count)) {
            return OTS_FAILURE();
          }
        }
      }
      if (records[i].non_default_offset) {
        const std::vector<ots::OpenTypeCMAPSubtableVSMapping> &mappings
            = records[i].mappings;
        const unsigned num_mappings = mappings.size();
        if (!out->Seek(records[i].non_default_offset + offset_0514) ||
            !out->WriteU32(num_mappings)) {
          return OTS_FAILURE();
        }
        for (unsigned j = 0; j < num_mappings; ++j) {
          if (!out->WriteU24(mappings[j].unicode_value) ||
              !out->WriteU16(mappings[j].glyph_id)) {
            return OTS_FAILURE();
          }
        }
      }
    }
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

  if (have_034) {
    if (!out->WriteU16(0) ||
        !out->WriteU16(3) ||
        !out->WriteU32(offset_034 - table_start)) {
      return OTS_FAILURE();
    }
  }

  if (have_0514) {
    if (!out->WriteU16(0) ||
        !out->WriteU16(5) ||
        !out->WriteU32(offset_0514 - table_start)) {
      return OTS_FAILURE();
    }
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
