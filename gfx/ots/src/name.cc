



#include "name.h"

#include <algorithm>
#include <cstring>

#include "cff.h"




namespace { 

inline bool
valid_in_ps_name(char c) {
  return (c > 0x20 && c < 0x7f && !std::strchr("[](){}<>/%", c));
}

inline bool
check_ps_name_ascii(std::string& name) {
  for (unsigned int i = 0; i < name.size(); ++i) {
    if (!valid_in_ps_name(name[i])) {
      return false;
    }
  }
  return true;
}

inline bool
check_ps_name_utf16_be(std::string& name) {
  for (unsigned int i = 0; i < name.size(); i += 2) {
    if (name[i] != 0) {
      return false;
    }
    if (!valid_in_ps_name(name[i+1])) {
      return false;
    }
  }
  return true;
}

void
assign_to_utf16_be_from_ascii(std::string& target, const std::string& source) {
  target.resize(source.size() * 2);
  for (unsigned int i = 0, j = 0; i < source.size(); i++) {
    target[j++] = '\0';
    target[j++] = source[i];
  }
}

} 

namespace ots {

bool ots_name_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  OpenTypeNAME *name = new OpenTypeNAME;
  file->name = name;

  uint16_t format;
  if (!table.ReadU16(&format) || format > 1) {
    return OTS_FAILURE();
  }

  uint16_t count;
  if (!table.ReadU16(&count)) {
    return OTS_FAILURE();
  }

  uint16_t stringOffset;
  if (!table.ReadU16(&stringOffset) || stringOffset > length) {
    return OTS_FAILURE();
  }
  const char* stringBase = (const char*)data + stringOffset;

  NameRecord prevRec;
  bool sortRequired = false;

  
  
  
  
  for (unsigned int i = 0; i < count; ++i) {
    NameRecord rec;
    uint16_t nameLength, nameOffset;
    if (!table.ReadU16(&rec.platformID) ||
        !table.ReadU16(&rec.encodingID) ||
        !table.ReadU16(&rec.languageID) ||
        !table.ReadU16(&rec.nameID) ||
        !table.ReadU16(&nameLength) ||
        !table.ReadU16(&nameOffset)) {
      return OTS_FAILURE();
    }
    
    switch (rec.platformID) {
    case 0: 
      if (rec.encodingID > 6) {
        continue;
      }
      break;
    case 1: 
      if (rec.encodingID > 32) {
        continue;
      }
      break;
    case 2: 
      if (rec.encodingID > 2) {
        continue;
      }
      break;
    case 3: 
      if (rec.encodingID > 6 && rec.encodingID != 10) {
        continue;
      }
      break;
    case 4: 
      if (rec.encodingID > 255) {
        continue;
      }
      break;
    default: 
      continue;
    }

    if (size_t(stringOffset) + nameOffset + nameLength > length) {
      continue;
    }
    rec.text.resize(nameLength);
    rec.text.assign(stringBase + nameOffset, nameLength);

    if (rec.nameID == 6) {
      
      if (rec.platformID == 1) {
        if (file->cff && !file->cff->name.empty()) {
          rec.text = file->cff->name;
        } else if (!check_ps_name_ascii(rec.text)) {
          continue;
        }
      } else if (rec.platformID == 0 || rec.platformID == 3) {
        if (file->cff && !file->cff->name.empty()) {
          assign_to_utf16_be_from_ascii(rec.text, file->cff->name);
        } else if (!check_ps_name_utf16_be(rec.text)) {
          continue;
        }
      }
    }

    if (i > 0) {
      if (!(prevRec < rec)) {
        sortRequired = true;
      }
    }

    name->names.push_back(rec);
    prevRec = rec;
  }

  if (format == 1) {
    
    uint16_t langTagCount;
    if (!table.ReadU16(&langTagCount)) {
      return OTS_FAILURE();
    }
    for (unsigned int i = 0; i < langTagCount; ++i) {
      uint16_t tagLength, tagOffset;
      if (!table.ReadU16(&tagLength) || !table.ReadU16(&tagOffset)) {
        return OTS_FAILURE();
      }
      if (size_t(stringOffset) + tagOffset + tagLength > length) {
        return OTS_FAILURE();
      }
      std::string tag(stringBase + tagOffset, tagLength);
      name->langTags.push_back(tag);
    }
  }

  if (table.offset() > stringOffset) {
    
    
    return OTS_FAILURE();
  }

  
  
  
  
  
  
  
  
  const unsigned int kStdNameCount = 7;
  const char* kStdNames[kStdNameCount] = {
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

  
  
  bool macName[kStdNameCount] = { 0 };
  bool winName[kStdNameCount] = { 0 };
  for (std::vector<NameRecord>::iterator nameIter = name->names.begin();
       nameIter != name->names.end(); nameIter++) {
    uint16_t id = nameIter->nameID;
    if (id >= kStdNameCount || kStdNames[id] == NULL) {
      continue;
    }
    if (nameIter->platformID == 1) {
      macName[id] = true;
      continue;
    }
    if (nameIter->platformID == 3) {
      winName[id] = true;
      continue;
    }
  }

  for (unsigned int i = 0; i < kStdNameCount; ++i) {
    if (kStdNames[i] == NULL) {
      continue;
    }
    if (!macName[i]) {
      NameRecord rec(1, 0, 0, i);
      rec.text.assign(kStdNames[i]);
      name->names.push_back(rec);
      sortRequired = true;
    }
    if (!winName[i]) {
      NameRecord rec(3, 1, 1033, i);
      assign_to_utf16_be_from_ascii(rec.text, std::string(kStdNames[i]));
      name->names.push_back(rec);
      sortRequired = true;
    }
  }

  if (sortRequired) {
    std::sort(name->names.begin(), name->names.end());
  }

  return true;
}

bool ots_name_should_serialise(OpenTypeFile *file) {
  return file->name != NULL;
}

bool ots_name_serialise(OTSStream *out, OpenTypeFile *file) {
  const OpenTypeNAME *name = file->name;

  uint16_t nameCount = name->names.size();
  uint16_t langTagCount = name->langTags.size();
  uint16_t format = 0;
  size_t stringOffset = 6 + nameCount * 12;

  if (name->langTags.size() > 0) {
    
    format = 1;
    stringOffset = 8 + nameCount * 12 + langTagCount * 4;
  }
  if (stringOffset > 0xffff) {
    return OTS_FAILURE();
  }
  if (!out->WriteU16(format) ||
      !out->WriteU16(nameCount) ||
      !out->WriteU16(stringOffset)) {
    return OTS_FAILURE();
  }

  std::string stringData;
  for (std::vector<NameRecord>::const_iterator nameIter = name->names.begin();
       nameIter != name->names.end(); nameIter++) {
    const NameRecord& rec = *nameIter;
    if (!out->WriteU16(rec.platformID) ||
        !out->WriteU16(rec.encodingID) ||
        !out->WriteU16(rec.languageID) ||
        !out->WriteU16(rec.nameID) ||
        !out->WriteU16(rec.text.size()) ||
        !out->WriteU16(stringData.size()) ) {
      return OTS_FAILURE();
    }
    stringData.append(rec.text);
  }

  if (format == 1) {
    if (!out->WriteU16(langTagCount)) {
      return OTS_FAILURE();
    }
    for (std::vector<std::string>::const_iterator tagIter = name->langTags.begin();
         tagIter != name->langTags.end(); tagIter++) {
      if (!out->WriteU16(tagIter->size()) ||
          !out->WriteU16(stringData.size())) {
        return OTS_FAILURE();
      }
      stringData.append(*tagIter);
    }
  }

  if (!out->Write(stringData.data(), stringData.size())) {
    return OTS_FAILURE();
  }

  return true;
}

void ots_name_free(OpenTypeFile *file) {
  delete file->name;
}

}  
