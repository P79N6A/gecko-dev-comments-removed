



#include "name.h"

#include <algorithm>
#include <cstring>

#include "cff.h"




namespace {

bool ValidInPsName(char c) {
  return (c > 0x20 && c < 0x7f && !std::strchr("[](){}<>/%", c));
}

bool CheckPsNameAscii(const std::string& name) {
  for (unsigned i = 0; i < name.size(); ++i) {
    if (!ValidInPsName(name[i])) {
      return false;
    }
  }
  return true;
}

bool CheckPsNameUtf16Be(const std::string& name) {
  if ((name.size() & 1) != 0)
    return false;

  for (unsigned i = 0; i < name.size(); i += 2) {
    if (name[i] != 0) {
      return false;
    }
    if (!ValidInPsName(name[i+1])) {
      return false;
    }
  }
  return true;
}

void AssignToUtf16BeFromAscii(std::string* target,
                              const std::string& source) {
  target->resize(source.size() * 2);
  for (unsigned i = 0, j = 0; i < source.size(); i++) {
    (*target)[j++] = '\0';
    (*target)[j++] = source[i];
  }
}

}  


namespace ots {

bool ots_name_parse(OpenTypeFile* file, const uint8_t* data, size_t length) {
  Buffer table(data, length);

  OpenTypeNAME* name = new OpenTypeNAME;
  file->name = name;

  uint16_t format = 0;
  if (!table.ReadU16(&format) || format > 1) {
    return OTS_FAILURE();
  }

  uint16_t count = 0;
  if (!table.ReadU16(&count)) {
    return OTS_FAILURE();
  }

  uint16_t string_offset = 0;
  if (!table.ReadU16(&string_offset) || string_offset > length) {
    return OTS_FAILURE();
  }
  const char* string_base = reinterpret_cast<const char*>(data) +
      string_offset;

  NameRecord prev_record;
  bool sort_required = false;

  
  
  
  
  for (unsigned i = 0; i < count; ++i) {
    NameRecord rec;
    uint16_t name_length, name_offset;
    if (!table.ReadU16(&rec.platform_id) ||
        !table.ReadU16(&rec.encoding_id) ||
        !table.ReadU16(&rec.language_id) ||
        !table.ReadU16(&rec.name_id) ||
        !table.ReadU16(&name_length) ||
        !table.ReadU16(&name_offset)) {
      return OTS_FAILURE();
    }
    
    switch (rec.platform_id) {
      case 0:  
        if (rec.encoding_id > 6) {
          continue;
        }
        break;
      case 1:  
        if (rec.encoding_id > 32) {
          continue;
        }
        break;
      case 2:  
        if (rec.encoding_id > 2) {
          continue;
        }
        break;
      case 3:  
        if (rec.encoding_id > 6 && rec.encoding_id != 10) {
          continue;
        }
        break;
      case 4:  
        if (rec.encoding_id > 255) {
          continue;
        }
        break;
      default:  
        continue;
    }

    const unsigned name_end = static_cast<unsigned>(string_offset) +
        name_offset + name_length;
    if (name_end > length) {
      continue;
    }
    rec.text.resize(name_length);
    rec.text.assign(string_base + name_offset, name_length);

    if (rec.name_id == 6) {
      
      if (rec.platform_id == 1) {
        if (file->cff && !file->cff->name.empty()) {
          rec.text = file->cff->name;
        } else if (!CheckPsNameAscii(rec.text)) {
          continue;
        }
      } else if (rec.platform_id == 0 || rec.platform_id == 3) {
        if (file->cff && !file->cff->name.empty()) {
          AssignToUtf16BeFromAscii(&rec.text, file->cff->name);
        } else if (!CheckPsNameUtf16Be(rec.text)) {
          continue;
        }
      }
    }

    if ((i > 0) && !(prev_record < rec)) {
      OTS_WARNING("name records are not sorted.");
      sort_required = true;
    }

    name->names.push_back(rec);
    prev_record = rec;
  }

  if (format == 1) {
    
    uint16_t lang_tag_count;
    if (!table.ReadU16(&lang_tag_count)) {
      return OTS_FAILURE();
    }
    for (unsigned i = 0; i < lang_tag_count; ++i) {
      uint16_t tag_length = 0;
      uint16_t tag_offset = 0;
      if (!table.ReadU16(&tag_length) || !table.ReadU16(&tag_offset)) {
        return OTS_FAILURE();
      }
      const unsigned tag_end = static_cast<unsigned>(string_offset) +
          tag_offset + tag_length;
      if (tag_end > length) {
        return OTS_FAILURE();
      }
      std::string tag(string_base + tag_offset, tag_length);
      name->lang_tags.push_back(tag);
    }
  }

  if (table.offset() > string_offset) {
    
    
    return OTS_FAILURE();
  }

  
  
  
  
  
  
  
  
  static const unsigned kStdNameCount = 7;
  static const char* kStdNames[kStdNameCount] = {
    NULL,
    "OTS derived font",
    "Unspecified",
    NULL,
    "OTS derived font",
    "1.000",
    "OTS-derived-font"
  };
  
  
  
  if (file->cff && !file->cff->name.empty()) {
    kStdNames[6] = file->cff->name.c_str();
  }

  
  
  bool mac_name[kStdNameCount] = { 0 };
  bool win_name[kStdNameCount] = { 0 };
  for (std::vector<NameRecord>::iterator name_iter = name->names.begin();
       name_iter != name->names.end(); name_iter++) {
    const uint16_t id = name_iter->name_id;
    if (id >= kStdNameCount || kStdNames[id] == NULL) {
      continue;
    }
    if (name_iter->platform_id == 1) {
      mac_name[id] = true;
      continue;
    }
    if (name_iter->platform_id == 3) {
      win_name[id] = true;
      continue;
    }
  }

  for (unsigned i = 0; i < kStdNameCount; ++i) {
    if (kStdNames[i] == NULL) {
      continue;
    }
    if (!mac_name[i]) {
      NameRecord rec(1 , 0 ,
                     0  , i );
      rec.text.assign(kStdNames[i]);
      name->names.push_back(rec);
      sort_required = true;
    }
    if (!win_name[i]) {
      NameRecord rec(3 , 1 ,
                     1033  , i );
      AssignToUtf16BeFromAscii(&rec.text, std::string(kStdNames[i]));
      name->names.push_back(rec);
      sort_required = true;
    }
  }

  if (sort_required) {
    std::sort(name->names.begin(), name->names.end());
  }

  return true;
}

bool ots_name_should_serialise(OpenTypeFile* file) {
  return file->name != NULL;
}

bool ots_name_serialise(OTSStream* out, OpenTypeFile* file) {
  const OpenTypeNAME* name = file->name;

  uint16_t name_count = name->names.size();
  uint16_t lang_tag_count = name->lang_tags.size();
  uint16_t format = 0;
  size_t string_offset = 6 + name_count * 12;

  if (name->lang_tags.size() > 0) {
    
    format = 1;
    string_offset += 2 + lang_tag_count * 4;
  }
  if (string_offset > 0xffff) {
    return OTS_FAILURE();
  }
  if (!out->WriteU16(format) ||
      !out->WriteU16(name_count) ||
      !out->WriteU16(string_offset)) {
    return OTS_FAILURE();
  }

  std::string string_data;
  for (std::vector<NameRecord>::const_iterator name_iter = name->names.begin();
       name_iter != name->names.end(); name_iter++) {
    const NameRecord& rec = *name_iter;
    if (!out->WriteU16(rec.platform_id) ||
        !out->WriteU16(rec.encoding_id) ||
        !out->WriteU16(rec.language_id) ||
        !out->WriteU16(rec.name_id) ||
        !out->WriteU16(rec.text.size()) ||
        !out->WriteU16(string_data.size()) ) {
      return OTS_FAILURE();
    }
    string_data.append(rec.text);
  }

  if (format == 1) {
    if (!out->WriteU16(lang_tag_count)) {
      return OTS_FAILURE();
    }
    for (std::vector<std::string>::const_iterator tag_iter =
             name->lang_tags.begin();
         tag_iter != name->lang_tags.end(); tag_iter++) {
      if (!out->WriteU16(tag_iter->size()) ||
          !out->WriteU16(string_data.size())) {
        return OTS_FAILURE();
      }
      string_data.append(*tag_iter);
    }
  }

  if (!out->Write(string_data.data(), string_data.size())) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_name_free(OpenTypeFile* file) {
  delete file->name;
}

}  
