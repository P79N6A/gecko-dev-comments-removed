



#include "ots.h"

#include <sys/types.h>
#include <zlib.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <vector>

#ifndef OTS_DISABLE_WOFF2
#include "woff2.h"
#endif




namespace {

bool g_debug_output = true;
bool g_enable_woff2 = false;


#define OTS_FAILURE_MSG_TAG(msg_,tag_) OTS_FAILURE_MSG_TAG_(header, msg_, tag_)
#define OTS_FAILURE_MSG_HDR(msg_)      OTS_FAILURE_MSG_(header, msg_)


struct OpenTypeTable {
  uint32_t tag;
  uint32_t chksum;
  uint32_t offset;
  uint32_t length;
  uint32_t uncompressed_length;
};

bool CheckTag(uint32_t tag_value) {
  for (unsigned i = 0; i < 4; ++i) {
    const uint32_t check = tag_value & 0xff;
    if (check < 32 || check > 126) {
      return false;  
    }
    tag_value >>= 8;
  }
  return true;
}

uint32_t Tag(const char *tag_str) {	
  uint32_t ret;	
  std::memcpy(&ret, tag_str, 4);	
  return ret;	
}

struct OutputTable {
  uint32_t tag;
  size_t offset;
  size_t length;
  uint32_t chksum;

  static bool SortByTag(const OutputTable& a, const OutputTable& b) {
    const uint32_t atag = ntohl(a.tag);
    const uint32_t btag = ntohl(b.tag);
    return atag < btag;
  }
};

struct Arena {
 public:
  ~Arena() {
    for (std::vector<uint8_t*>::iterator
         i = hunks_.begin(); i != hunks_.end(); ++i) {
      delete[] *i;
    }
  }

  uint8_t* Allocate(size_t length) {
    uint8_t* p = new uint8_t[length];
    hunks_.push_back(p);
    return p;
  }

 private:
  std::vector<uint8_t*> hunks_;
};

const struct {
  const char* tag;
  bool (*parse)(ots::OpenTypeFile *otf, const uint8_t *data, size_t length);
  bool (*serialise)(ots::OTSStream *out, ots::OpenTypeFile *file);
  bool (*should_serialise)(ots::OpenTypeFile *file);
  void (*free)(ots::OpenTypeFile *file);
  bool required;
} table_parsers[] = {
  { "maxp", ots::ots_maxp_parse, ots::ots_maxp_serialise,
    ots::ots_maxp_should_serialise, ots::ots_maxp_free, true },
  { "head", ots::ots_head_parse, ots::ots_head_serialise,
    ots::ots_head_should_serialise, ots::ots_head_free, true },
  { "OS/2", ots::ots_os2_parse, ots::ots_os2_serialise,
    ots::ots_os2_should_serialise, ots::ots_os2_free, true },
  { "cmap", ots::ots_cmap_parse, ots::ots_cmap_serialise,
    ots::ots_cmap_should_serialise, ots::ots_cmap_free, true },
  { "hhea", ots::ots_hhea_parse, ots::ots_hhea_serialise,
    ots::ots_hhea_should_serialise, ots::ots_hhea_free, true },
  { "hmtx", ots::ots_hmtx_parse, ots::ots_hmtx_serialise,
    ots::ots_hmtx_should_serialise, ots::ots_hmtx_free, true },
  { "name", ots::ots_name_parse, ots::ots_name_serialise,
    ots::ots_name_should_serialise, ots::ots_name_free, true },
  { "post", ots::ots_post_parse, ots::ots_post_serialise,
    ots::ots_post_should_serialise, ots::ots_post_free, true },
  { "loca", ots::ots_loca_parse, ots::ots_loca_serialise,
    ots::ots_loca_should_serialise, ots::ots_loca_free, false },
  { "glyf", ots::ots_glyf_parse, ots::ots_glyf_serialise,
    ots::ots_glyf_should_serialise, ots::ots_glyf_free, false },
  { "CFF ", ots::ots_cff_parse, ots::ots_cff_serialise,
    ots::ots_cff_should_serialise, ots::ots_cff_free, false },
  { "VDMX", ots::ots_vdmx_parse, ots::ots_vdmx_serialise,
    ots::ots_vdmx_should_serialise, ots::ots_vdmx_free, false },
  { "hdmx", ots::ots_hdmx_parse, ots::ots_hdmx_serialise,
    ots::ots_hdmx_should_serialise, ots::ots_hdmx_free, false },
  { "gasp", ots::ots_gasp_parse, ots::ots_gasp_serialise,
    ots::ots_gasp_should_serialise, ots::ots_gasp_free, false },
  { "cvt ", ots::ots_cvt_parse, ots::ots_cvt_serialise,
    ots::ots_cvt_should_serialise, ots::ots_cvt_free, false },
  { "fpgm", ots::ots_fpgm_parse, ots::ots_fpgm_serialise,
    ots::ots_fpgm_should_serialise, ots::ots_fpgm_free, false },
  { "prep", ots::ots_prep_parse, ots::ots_prep_serialise,
    ots::ots_prep_should_serialise, ots::ots_prep_free, false },
  { "LTSH", ots::ots_ltsh_parse, ots::ots_ltsh_serialise,
    ots::ots_ltsh_should_serialise, ots::ots_ltsh_free, false },
  { "VORG", ots::ots_vorg_parse, ots::ots_vorg_serialise,
    ots::ots_vorg_should_serialise, ots::ots_vorg_free, false },
  { "kern", ots::ots_kern_parse, ots::ots_kern_serialise,
    ots::ots_kern_should_serialise, ots::ots_kern_free, false },
  
  
  { "GDEF", ots::ots_gdef_parse, ots::ots_gdef_serialise,
    ots::ots_gdef_should_serialise, ots::ots_gdef_free, false },
  { "GPOS", ots::ots_gpos_parse, ots::ots_gpos_serialise,
    ots::ots_gpos_should_serialise, ots::ots_gpos_free, false },
  { "GSUB", ots::ots_gsub_parse, ots::ots_gsub_serialise,
    ots::ots_gsub_should_serialise, ots::ots_gsub_free, false },
  { "vhea", ots::ots_vhea_parse, ots::ots_vhea_serialise,
    ots::ots_vhea_should_serialise, ots::ots_vhea_free, false },
  { "vmtx", ots::ots_vmtx_parse, ots::ots_vmtx_serialise,
    ots::ots_vmtx_should_serialise, ots::ots_vmtx_free, false },
  { "MATH", ots::ots_math_parse, ots::ots_math_serialise,
    ots::ots_math_should_serialise, ots::ots_math_free, false },
  
  { 0, NULL, NULL, NULL, NULL, false },
};

bool ProcessGeneric(ots::OpenTypeFile *header,
                    uint32_t signature,
                    ots::OTSStream *output,
                    const uint8_t *data, size_t length,
                    const std::vector<OpenTypeTable>& tables,
                    ots::Buffer& file);

bool ProcessTTF(ots::OpenTypeFile *header,
                ots::OTSStream *output, const uint8_t *data, size_t length) {
  ots::Buffer file(data, length);

  
  if (length > 1024 * 1024 * 1024) {
    return OTS_FAILURE_MSG_HDR("file exceeds 1GB");
  }

  if (!file.ReadTag(&header->version)) {
    return OTS_FAILURE_MSG_HDR("error reading version tag");
  }
  if (!ots::IsValidVersionTag(header->version)) {
      return OTS_FAILURE_MSG_HDR("invalid version tag");
  }

  if (!file.ReadU16(&header->num_tables) ||
      !file.ReadU16(&header->search_range) ||
      !file.ReadU16(&header->entry_selector) ||
      !file.ReadU16(&header->range_shift)) {
    return OTS_FAILURE_MSG_HDR("error reading table directory search header");
  }

  
  
  if (header->num_tables >= 4096 || header->num_tables < 1) {
    return OTS_FAILURE_MSG_HDR("excessive (or zero) number of tables");
  }

  unsigned max_pow2 = 0;
  while (1u << (max_pow2 + 1) <= header->num_tables) {
    max_pow2++;
  }
  const uint16_t expected_search_range = (1u << max_pow2) << 4;

  
  
  if (header->search_range != expected_search_range) {
    OTS_WARNING("bad search range");
    header->search_range = expected_search_range;  
  }

  
  if (header->entry_selector != max_pow2) {
    return OTS_FAILURE_MSG_HDR("incorrect entrySelector for table directory");
  }

  
  
  
  const uint32_t expected_range_shift
      = 16 * header->num_tables - header->search_range;
  if (header->range_shift != expected_range_shift) {
    OTS_WARNING("bad range shift");
    header->range_shift = expected_range_shift;  
  }

  
  std::vector<OpenTypeTable> tables;

  for (unsigned i = 0; i < header->num_tables; ++i) {
    OpenTypeTable table;
    if (!file.ReadTag(&table.tag) ||
        !file.ReadU32(&table.chksum) ||
        !file.ReadU32(&table.offset) ||
        !file.ReadU32(&table.length)) {
      return OTS_FAILURE_MSG_HDR("error reading table directory");
    }

    table.uncompressed_length = table.length;
    tables.push_back(table);
  }

  return ProcessGeneric(header, header->version, output, data, length,
                        tables, file);
}

bool ProcessWOFF(ots::OpenTypeFile *header,
                 ots::OTSStream *output, const uint8_t *data, size_t length) {
  ots::Buffer file(data, length);

  
  if (length > 1024 * 1024 * 1024) {
    return OTS_FAILURE_MSG_HDR("file exceeds 1GB");
  }

  uint32_t woff_tag;
  if (!file.ReadTag(&woff_tag)) {
    return OTS_FAILURE_MSG_HDR("error reading WOFF marker");
  }

  if (woff_tag != Tag("wOFF")) {
    return OTS_FAILURE_MSG_HDR("invalid WOFF marker");
  }

  if (!file.ReadTag(&header->version)) {
    return OTS_FAILURE_MSG_HDR("error reading version tag");
  }
  if (!ots::IsValidVersionTag(header->version)) {
    return OTS_FAILURE_MSG_HDR("invalid version tag");
  }

  header->search_range = 0;
  header->entry_selector = 0;
  header->range_shift = 0;

  uint32_t reported_length;
  if (!file.ReadU32(&reported_length) || length != reported_length) {
    return OTS_FAILURE_MSG_HDR("incorrect file size in WOFF header");
  }

  if (!file.ReadU16(&header->num_tables) || !header->num_tables) {
    return OTS_FAILURE_MSG_HDR("error reading number of tables");
  }

  uint16_t reserved_value;
  if (!file.ReadU16(&reserved_value) || reserved_value) {
    return OTS_FAILURE_MSG_HDR("error in reserved field of WOFF header");
  }

  uint32_t reported_total_sfnt_size;
  if (!file.ReadU32(&reported_total_sfnt_size)) {
    return OTS_FAILURE_MSG_HDR("error reading total sfnt size");
  }

  
  
  if (!file.Skip(2 * 2)) {
    return OTS_FAILURE_MSG_HDR("error skipping WOFF header fields");
  }

  
  uint32_t meta_offset;
  uint32_t meta_length;
  uint32_t meta_length_orig;
  if (!file.ReadU32(&meta_offset) ||
      !file.ReadU32(&meta_length) ||
      !file.ReadU32(&meta_length_orig)) {
    return OTS_FAILURE_MSG_HDR("error reading WOFF header fields");
  }
  if (meta_offset) {
    if (meta_offset >= length || length - meta_offset < meta_length) {
      return OTS_FAILURE_MSG_HDR("invalid metadata block location/size");
    }
  }

  
  uint32_t priv_offset;
  uint32_t priv_length;
  if (!file.ReadU32(&priv_offset) ||
      !file.ReadU32(&priv_length)) {
    return OTS_FAILURE_MSG_HDR("error reading WOFF header fields");
  }
  if (priv_offset) {
    if (priv_offset >= length || length - priv_offset < priv_length) {
      return OTS_FAILURE_MSG_HDR("invalid private block location/size");
    }
  }

  
  std::vector<OpenTypeTable> tables;

  uint32_t first_index = 0;
  uint32_t last_index = 0;
  
  uint64_t total_sfnt_size = 12 + 16 * header->num_tables;
  for (unsigned i = 0; i < header->num_tables; ++i) {
    OpenTypeTable table;
    if (!file.ReadTag(&table.tag) ||
        !file.ReadU32(&table.offset) ||
        !file.ReadU32(&table.length) ||
        !file.ReadU32(&table.uncompressed_length) ||
        !file.ReadU32(&table.chksum)) {
      return OTS_FAILURE_MSG_HDR("error reading table directory");
    }

    total_sfnt_size += ots::Round4(table.uncompressed_length);
    if (total_sfnt_size > std::numeric_limits<uint32_t>::max()) {
      return OTS_FAILURE_MSG_HDR("sfnt size overflow");
    }
    tables.push_back(table);
    if (i == 0 || tables[first_index].offset > table.offset)
      first_index = i;
    if (i == 0 || tables[last_index].offset < table.offset)
      last_index = i;
  }

  if (reported_total_sfnt_size != total_sfnt_size) {
    return OTS_FAILURE_MSG_HDR("uncompressed sfnt size mismatch");
  }

  
  if (tables[first_index].offset != ots::Round4(file.offset())) {
    return OTS_FAILURE_MSG_HDR("junk before tables in WOFF file");
  }

  if (tables[last_index].offset >= length ||
      length - tables[last_index].offset < tables[last_index].length) {
    return OTS_FAILURE_MSG_HDR("invalid table location/size");
  }
  
  
  uint64_t block_end = ots::Round4(
      static_cast<uint64_t>(tables[last_index].offset) +
      static_cast<uint64_t>(tables[last_index].length));
  if (block_end > std::numeric_limits<uint32_t>::max()) {
    return OTS_FAILURE_MSG_HDR("invalid table location/size");
  }
  if (meta_offset) {
    if (block_end != meta_offset) {
      return OTS_FAILURE_MSG_HDR("invalid metadata block location");
    }
    block_end = ots::Round4(static_cast<uint64_t>(meta_offset) +
                            static_cast<uint64_t>(meta_length));
    if (block_end > std::numeric_limits<uint32_t>::max()) {
      return OTS_FAILURE_MSG_HDR("invalid metadata block size");
    }
  }
  if (priv_offset) {
    if (block_end != priv_offset) {
      return OTS_FAILURE_MSG_HDR("invalid private block location");
    }
    block_end = ots::Round4(static_cast<uint64_t>(priv_offset) +
                            static_cast<uint64_t>(priv_length));
    if (block_end > std::numeric_limits<uint32_t>::max()) {
      return OTS_FAILURE_MSG_HDR("invalid private block size");
    }
  }
  if (block_end != ots::Round4(length)) {
    return OTS_FAILURE_MSG_HDR("file length mismatch (trailing junk?)");
  }

  return ProcessGeneric(header, woff_tag, output, data, length, tables, file);
}

#ifndef OTS_DISABLE_WOFF2
bool ProcessWOFF2(ots::OpenTypeFile *header,
                  ots::OTSStream *output, const uint8_t *data, size_t length) {
  size_t decompressed_size = ots::ComputeWOFF2FinalSize(data, length);
  if (decompressed_size == 0) {
    return OTS_FAILURE();
  }
  
  if (decompressed_size > 30 * 1024 * 1024) {
    return OTS_FAILURE();
  }

  std::vector<uint8_t> decompressed_buffer(decompressed_size);
  if (!ots::ConvertWOFF2ToTTF(&decompressed_buffer[0], decompressed_size,
                              data, length)) {
    return OTS_FAILURE();
  }
  return ProcessTTF(header, output, &decompressed_buffer[0], decompressed_size);
}
#endif

ots::TableAction GetTableAction(ots::OpenTypeFile *header, uint32_t tag) {
  ots::TableAction action = ots::TABLE_ACTION_DEFAULT;

  action = header->context->GetTableAction(htonl(tag));

  if (action == ots::TABLE_ACTION_DEFAULT) {
    action = ots::TABLE_ACTION_DROP;

    for (unsigned i = 0; ; ++i) {
      if (table_parsers[i].parse == NULL) break;

      if (Tag(table_parsers[i].tag) == tag) {
        action = ots::TABLE_ACTION_SANITIZE;
        break;
      }
    }
  }

  assert(action != ots::TABLE_ACTION_DEFAULT); 
  return action;
}

bool GetTableData(const uint8_t *data,
                  const OpenTypeTable table,
                  Arena *arena,
                  size_t *table_length,
                  const uint8_t **table_data) {
  if (table.uncompressed_length != table.length) {
    
    *table_length = table.uncompressed_length;
    *table_data = (*arena).Allocate(*table_length);
    uLongf dest_len = *table_length;
    int r = uncompress((Bytef*) *table_data, &dest_len,
                       data + table.offset, table.length);
    if (r != Z_OK || dest_len != *table_length) {
      return false;
    }
  } else {
    
    *table_data = data + table.offset;
    *table_length = table.length;
  }

  return true;
}

bool ProcessGeneric(ots::OpenTypeFile *header, uint32_t signature,
                    ots::OTSStream *output,
                    const uint8_t *data, size_t length,
                    const std::vector<OpenTypeTable>& tables,
                    ots::Buffer& file) {
  const size_t data_offset = file.offset();

  uint32_t uncompressed_sum = 0;

  for (unsigned i = 0; i < header->num_tables; ++i) {
    
    
    if (i) {
      const uint32_t this_tag = ntohl(tables[i].tag);
      const uint32_t prev_tag = ntohl(tables[i - 1].tag);
      if (this_tag <= prev_tag) {
        return OTS_FAILURE_MSG_HDR("table directory not correctly ordered");
      }
    }

    
    if (!CheckTag(tables[i].tag)) {
      return OTS_FAILURE_MSG_TAG("invalid table tag", &tables[i].tag);
    }

    
    if (tables[i].offset & 3) {
      return OTS_FAILURE_MSG_TAG("misaligned table", &tables[i].tag);
    }

    
    if (tables[i].offset < data_offset || tables[i].offset >= length) {
      return OTS_FAILURE_MSG_TAG("invalid table offset", &tables[i].tag);
    }
    
    if (tables[i].length < 1) {
      
      return OTS_FAILURE_MSG_TAG("zero-length table", &tables[i].tag);
    }
    
    if (tables[i].length > 1024 * 1024 * 1024) {
      return OTS_FAILURE_MSG_TAG("table length exceeds 1GB", &tables[i].tag);
    }
    
    if (tables[i].uncompressed_length < tables[i].length) {
      return OTS_FAILURE_MSG_TAG("invalid compressed table", &tables[i].tag);
    }
    if (tables[i].uncompressed_length > tables[i].length) {
      

      
      if (tables[i].uncompressed_length > 30 * 1024 * 1024) {
        return OTS_FAILURE_MSG_TAG("uncompressed length exceeds 30MB", &tables[i].tag);
      }
      if (uncompressed_sum + tables[i].uncompressed_length < uncompressed_sum) {
        return OTS_FAILURE_MSG_TAG("overflow of uncompressed sum", &tables[i].tag);
      }

      uncompressed_sum += tables[i].uncompressed_length;
    }
    
    
    uint32_t end_byte = tables[i].offset + tables[i].length;
    
    if (signature == Tag("wOFF")) {
        end_byte = ots::Round4(end_byte);
    }
    if (!end_byte || end_byte > length) {
      return OTS_FAILURE_MSG_TAG("table overruns end of file", &tables[i].tag);
    }
  }

  
  if (uncompressed_sum > 30 * 1024 * 1024) {
    return OTS_FAILURE_MSG_HDR("uncompressed sum exceeds 30MB");
  }

  std::map<uint32_t, OpenTypeTable> table_map;
  for (unsigned i = 0; i < header->num_tables; ++i) {
    table_map[tables[i].tag] = tables[i];
  }

  
  std::vector<std::pair<uint32_t, uint8_t> > overlap_checker;
  for (unsigned i = 0; i < header->num_tables; ++i) {
    overlap_checker.push_back(
        std::make_pair(tables[i].offset, static_cast<uint8_t>(1) ));
    overlap_checker.push_back(
        std::make_pair(tables[i].offset + tables[i].length,
                       static_cast<uint8_t>(0) ));
  }
  std::sort(overlap_checker.begin(), overlap_checker.end());
  int overlap_count = 0;
  for (unsigned i = 0; i < overlap_checker.size(); ++i) {
    overlap_count += (overlap_checker[i].second ? 1 : -1);
    if (overlap_count > 1) {
      return OTS_FAILURE_MSG_HDR("overlapping tables");
    }
  }

  Arena arena;

  for (unsigned i = 0; ; ++i) {
    if (table_parsers[i].parse == NULL) break;

    uint32_t tag = Tag(table_parsers[i].tag);
    const std::map<uint32_t, OpenTypeTable>::const_iterator it = table_map.find(tag);

    ots::TableAction action = GetTableAction(header, tag);
    if (it == table_map.end()) {
      if (table_parsers[i].required && action == ots::TABLE_ACTION_SANITIZE) {
        return OTS_FAILURE_MSG_TAG("missing required table", table_parsers[i].tag);
      }
      continue;
    }

    const uint8_t* table_data;
    size_t table_length;

    if (!GetTableData(data, it->second, &arena, &table_length, &table_data)) {
      return OTS_FAILURE_MSG_TAG("uncompress failed", table_parsers[i].tag);
    }

    if (action == ots::TABLE_ACTION_SANITIZE &&
        !table_parsers[i].parse(header, table_data, table_length)) {
      
      
      return OTS_FAILURE_MSG_TAG("failed to parse table", table_parsers[i].tag);
    }
  }

  if (header->cff) {
    
    if (header->version != Tag("OTTO")) {
      return OTS_FAILURE_MSG_HDR("wrong font version for PostScript glyph data");
    }
    if (header->glyf || header->loca) {
      
      return OTS_FAILURE_MSG_HDR("font contains both PS and TT glyphs");
    }
  } else {
    if (!header->glyf || !header->loca) {
      
      
      return OTS_FAILURE_MSG_HDR("neither PS nor TT glyphs present");
    }
  }

  unsigned num_output_tables = 0;
  for (unsigned i = 0; ; ++i) {
    if (table_parsers[i].parse == NULL) {
      break;
    }

    if (table_parsers[i].should_serialise(header)) {
      num_output_tables++;
    }
  }

  for (std::map<uint32_t, OpenTypeTable>::const_iterator it = table_map.begin();
       it != table_map.end(); ++it) {
    ots::TableAction action = GetTableAction(header, it->first);
    if (action == ots::TABLE_ACTION_PASSTHRU) {
      num_output_tables++;
    }
  }

  unsigned max_pow2 = 0;
  while (1u << (max_pow2 + 1) <= num_output_tables) {
    max_pow2++;
  }
  const uint16_t output_search_range = (1u << max_pow2) << 4;

  
  
  output->ResetChecksum();
  if (!output->WriteTag(header->version) ||
      !output->WriteU16(num_output_tables) ||
      !output->WriteU16(output_search_range) ||
      !output->WriteU16(max_pow2) ||
      !output->WriteU16((num_output_tables << 4) - output_search_range)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }
  const uint32_t offset_table_chksum = output->chksum();

  const size_t table_record_offset = output->Tell();
  if (!output->Pad(16 * num_output_tables)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }

  std::vector<OutputTable> out_tables;

  size_t head_table_offset = 0;
  for (unsigned i = 0; ; ++i) {
    if (table_parsers[i].parse == NULL) {
      break;
    }

    if (!table_parsers[i].should_serialise(header)) {
      continue;
    }

    OutputTable out;
    uint32_t tag = Tag(table_parsers[i].tag);
    out.tag = tag;
    out.offset = output->Tell();

    output->ResetChecksum();
    if (tag == Tag("head")) {
      head_table_offset = out.offset;
    }
    if (!table_parsers[i].serialise(output, header)) {
      return OTS_FAILURE_MSG_TAG("failed to serialize table", table_parsers[i].tag);
    }

    const size_t end_offset = output->Tell();
    if (end_offset <= out.offset) {
      
      
      return OTS_FAILURE_MSG_HDR("error writing output");
    }
    out.length = end_offset - out.offset;

    
    if (!output->Pad((4 - (end_offset & 3)) % 4)) {
      return OTS_FAILURE_MSG_HDR("error writing output");
    }
    out.chksum = output->chksum();
    out_tables.push_back(out);
  }

  for (std::map<uint32_t, OpenTypeTable>::const_iterator it = table_map.begin();
       it != table_map.end(); ++it) {
    ots::TableAction action = GetTableAction(header, it->first);
    if (action == ots::TABLE_ACTION_PASSTHRU) {
      OutputTable out;
      out.tag = it->second.tag;
      out.offset = output->Tell();

      output->ResetChecksum();
      if (it->second.tag == Tag("head")) {
        head_table_offset = out.offset;
      }

      const uint8_t* table_data;
      size_t table_length;

      if (!GetTableData(data, it->second, &arena, &table_length, &table_data)) {
        return OTS_FAILURE_MSG_HDR("Failed to uncompress table");
      }

      if (!output->Write(table_data, table_length)) {
        return OTS_FAILURE_MSG_HDR("Failed to serialize table");
      }

      const size_t end_offset = output->Tell();
      if (end_offset <= out.offset) {
        
        
        return OTS_FAILURE_MSG_HDR("error writing output");
      }
      out.length = end_offset - out.offset;

      
      if (!output->Pad((4 - (end_offset & 3)) % 4)) {
        return OTS_FAILURE_MSG_HDR("error writing output");
      }
      out.chksum = output->chksum();
      out_tables.push_back(out);
    }
  }

  const size_t end_of_file = output->Tell();

  
  std::sort(out_tables.begin(), out_tables.end(), OutputTable::SortByTag);
  if (!output->Seek(table_record_offset)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }

  output->ResetChecksum();
  uint32_t tables_chksum = 0;
  for (unsigned i = 0; i < out_tables.size(); ++i) {
    if (!output->WriteTag(out_tables[i].tag) ||
        !output->WriteU32(out_tables[i].chksum) ||
        !output->WriteU32(out_tables[i].offset) ||
        !output->WriteU32(out_tables[i].length)) {
      return OTS_FAILURE_MSG_HDR("error writing output");
    }
    tables_chksum += out_tables[i].chksum;
  }
  const uint32_t table_record_chksum = output->chksum();

  
  const uint32_t file_chksum
      = offset_table_chksum + tables_chksum + table_record_chksum;
  const uint32_t chksum_magic = static_cast<uint32_t>(0xb1b0afba) - file_chksum;

  
  if (!head_table_offset) {
    return OTS_FAILURE_MSG_HDR("internal error!");
  }
  if (!output->Seek(head_table_offset + 8)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }
  if (!output->WriteU32(chksum_magic)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }

  if (!output->Seek(end_of_file)) {
    return OTS_FAILURE_MSG_HDR("error writing output");
  }

  return true;
}

}  

namespace ots {

bool IsValidVersionTag(uint32_t tag) {
  return tag == Tag("\x00\x01\x00\x00") ||
         
         tag == Tag("OTTO") ||
         
         tag == Tag("true") ||
         tag == Tag("typ1");
}

void DisableDebugOutput() {
  g_debug_output = false;
}

void EnableWOFF2() {
  g_enable_woff2 = true;
}

bool OTSContext::Process(OTSStream *output,
                         const uint8_t *data,
                         size_t length) {
  OpenTypeFile header;

  header.context = this;

  if (length < 4) {
    return OTS_FAILURE_MSG_(&header, "file less than 4 bytes");
  }

  bool result;
  if (data[0] == 'w' && data[1] == 'O' && data[2] == 'F' && data[3] == 'F') {
    result = ProcessWOFF(&header, output, data, length);
#ifndef OTS_DISABLE_WOFF2
  } else if (g_enable_woff2 &&
             data[0] == 'w' && data[1] == 'O' && data[2] == 'F' &&
             data[3] == '2') {
    result = ProcessWOFF2(&header, output, data, length);
#endif
  } else {
    result = ProcessTTF(&header, output, data, length);
  }

  for (unsigned i = 0; ; ++i) {
    if (table_parsers[i].parse == NULL) break;
    table_parsers[i].free(&header);
  }
  return result;
}

#if !defined(_MSC_VER) && defined(OTS_DEBUG)
bool Failure(const char *f, int l, const char *fn) {
  if (g_debug_output) {
    std::fprintf(stderr, "ERROR at %s:%d (%s)\n", f, l, fn);
    std::fflush(stderr);
  }
  return false;
}

void Warning(const char *f, int l, const char *format, ...) {
  if (g_debug_output) {
    std::fprintf(stderr, "WARNING at %s:%d: ", f, l);
    std::va_list va;
    va_start(va, format);
    std::vfprintf(stderr, format, va);
    va_end(va);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
  }
}
#endif

}  
