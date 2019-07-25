



#include "cff.h"

#include <cstring>
#include <utility>  
#include <vector>

#include "cff_type2_charstring.h"





namespace {

enum DICT_OPERAND_TYPE {
  DICT_OPERAND_INTEGER,
  DICT_OPERAND_REAL,
  DICT_OPERATOR,
};

enum DICT_DATA_TYPE {
  DICT_DATA_TOPLEVEL,
  DICT_DATA_FDARRAY,
};


const size_t kNStdString = 390;

bool ReadOffset(ots::Buffer *table, uint8_t off_size, uint32_t *offset) {
  if (off_size > 4) {
    return OTS_FAILURE();
  }

  uint32_t tmp32 = 0;
  for (unsigned i = 0; i < off_size; ++i) {
    uint8_t tmp8 = 0;
    if (!table->ReadU8(&tmp8)) {
      return OTS_FAILURE();
    }
    tmp32 <<= 8;
    tmp32 += tmp8;
  }
  *offset = tmp32;
  return true;
}

bool ParseIndex(ots::Buffer *table, ots::CFFIndex *index) {
  index->off_size = 0;
  index->offsets.clear();

  if (!table->ReadU16(&(index->count))) {
    return OTS_FAILURE();
  }
  if (index->count == 0) {
    
    index->offset_to_next = table->offset() + sizeof(index->count);
    return true;
  }

  if (!table->ReadU8(&(index->off_size))) {
    return OTS_FAILURE();
  }
  if ((index->off_size == 0) ||
      (index->off_size > 4)) {
    return OTS_FAILURE();
  }

  const size_t array_size = (index->count + 1) * index->off_size;
  
  const size_t object_data_offset = table->offset() + array_size;
  

  if (object_data_offset >= table->length()) {
    return OTS_FAILURE();
  }

  for (unsigned i = 0; i <= index->count; ++i) {  
    uint32_t rel_offset = 0;
    if (!ReadOffset(table, index->off_size, &rel_offset)) {
      return OTS_FAILURE();
    }
    if (rel_offset < 1) {
      return OTS_FAILURE();
    }
    if (i == 0 && rel_offset != 1) {
      return OTS_FAILURE();
    }

    if (rel_offset > table->length()) {
      return OTS_FAILURE();
    }

    
    if (object_data_offset > table->length() - (rel_offset - 1)) {
      return OTS_FAILURE();
    }

    index->offsets.push_back(
        object_data_offset + (rel_offset - 1));  
  }

  for (unsigned i = 1; i < index->offsets.size(); ++i) {
    
    
    if (index->offsets[i] < index->offsets[i - 1]) {
      return OTS_FAILURE();
    }
  }

  index->offset_to_next = index->offsets.back();
  return true;
}

bool ParseNameData(
    ots::Buffer *table, const ots::CFFIndex &index, std::string* out_name) {
  uint8_t name[256] = {0};
  if (index.offsets.size() == 0) {  
    return OTS_FAILURE();
  }
  for (unsigned i = 1; i < index.offsets.size(); ++i) {
    const size_t length = index.offsets[i] - index.offsets[i - 1];
    
    if (length > 127) {
      return OTS_FAILURE();
    }

    table->set_offset(index.offsets[i - 1]);
    if (!table->Read(name, length)) {
      return OTS_FAILURE();
    }

    for (size_t j = 0; j < length; ++j) {
      
      if (j == 0 && name[j] == 0) continue;
      
      if (name[j] < 33 || name[j] > 126) {
        return OTS_FAILURE();
      }
      
      if (std::strchr("[](){}<>/% ", name[j])) {
        return OTS_FAILURE();
      }
    }
  }

  *out_name = reinterpret_cast<char *>(name);
  return true;
}

bool CheckOffset(const std::pair<uint32_t, DICT_OPERAND_TYPE>& operand,
                 size_t table_length) {
  if (operand.second != DICT_OPERAND_INTEGER) {
    return OTS_FAILURE();
  }
  if (operand.first >= table_length) {
    return OTS_FAILURE();
  }
  return true;
}

bool CheckSid(const std::pair<uint32_t, DICT_OPERAND_TYPE>& operand,
              size_t sid_max) {
  if (operand.second != DICT_OPERAND_INTEGER) {
    return OTS_FAILURE();
  }
  if (operand.first > sid_max) {
    return OTS_FAILURE();
  }
  return true;
}

bool ParseDictDataBcd(
    ots::Buffer *table,
    std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > *operands) {
  bool read_decimal_point = false;
  bool read_e = false;

  uint8_t nibble = 0;
  size_t count = 0;
  while (true) {
    if (!table->ReadU8(&nibble)) {
      return OTS_FAILURE();
    }
    if ((nibble & 0xf0) == 0xf0) {
      if ((nibble & 0xf) == 0xf) {
        
        
        operands->push_back(std::make_pair(0, DICT_OPERAND_REAL));
        return true;
      }
      return OTS_FAILURE();
    }
    if ((nibble & 0x0f) == 0x0f) {
      operands->push_back(std::make_pair(0, DICT_OPERAND_REAL));
      return true;
    }

    
    uint8_t nibbles[2];
    nibbles[0] = (nibble & 0xf0) >> 8;
    nibbles[1] = (nibble & 0x0f);
    for (unsigned i = 0; i < 2; ++i) {
      if (nibbles[i] == 0xd) {  
        return OTS_FAILURE();
      }
      if ((nibbles[i] == 0xe) &&  
          ((count > 0) || (i > 0))) {
        return OTS_FAILURE();  
      }
      if (nibbles[i] == 0xa) {  
        if (!read_decimal_point) {
          read_decimal_point = true;
        } else {
          return OTS_FAILURE();  
        }
      }
      if ((nibbles[i] == 0xb) ||  
          (nibbles[i] == 0xc)) {  
        if (!read_e) {
          read_e = true;
        } else {
          return OTS_FAILURE();  
        }
      }
    }
    ++count;
  }
}

bool ParseDictDataEscapedOperator(
    ots::Buffer *table,
    std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > *operands) {
  uint8_t op = 0;
  if (!table->ReadU8(&op)) {
    return OTS_FAILURE();
  }

  if ((op <= 14) ||
      (op >= 17 && op <= 23) ||
      (op >= 30 && op <= 38)) {
    operands->push_back(std::make_pair((12U << 8) + op, DICT_OPERATOR));
    return true;
  }

  
  return OTS_FAILURE();
}

bool ParseDictDataNumber(
    ots::Buffer *table, uint8_t b0,
    std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > *operands) {
  uint8_t b1 = 0;
  uint8_t b2 = 0;
  uint8_t b3 = 0;
  uint8_t b4 = 0;

  switch (b0) {
    case 28:  
      if (!table->ReadU8(&b1) ||
          !table->ReadU8(&b2)) {
        return OTS_FAILURE();
      }
      operands->push_back(std::make_pair((b1 << 8) + b2, DICT_OPERAND_INTEGER));
      return true;

    case 29:  
      if (!table->ReadU8(&b1) ||
          !table->ReadU8(&b2) ||
          !table->ReadU8(&b3) ||
          !table->ReadU8(&b4)) {
        return OTS_FAILURE();
      }
      operands->push_back(std::make_pair(
          (b1 << 24) + (b2 << 16) + (b3 << 8) + b4, DICT_OPERAND_INTEGER));
      return true;

    case 30:  
      return ParseDictDataBcd(table, operands);

    default:
      break;
  }

  uint32_t result;
  if (b0 >=32 && b0 <=246) {
    result = b0 - 139;
  } else if (b0 >=247 && b0 <= 250) {
    if (!table->ReadU8(&b1)) {
      return OTS_FAILURE();
    }
    result = (b0 - 247) * 256 + b1 + 108;
  } else if (b0 >= 251 && b0 <= 254) {
    if (!table->ReadU8(&b1)) {
      return OTS_FAILURE();
    }
    result = -(b0 - 251) * 256 + b1 - 108;
  } else {
    return OTS_FAILURE();
  }

  operands->push_back(std::make_pair(result, DICT_OPERAND_INTEGER));
  return true;
}

bool ParseDictDataReadNext(
    ots::Buffer *table,
    std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > *operands) {
  uint8_t op = 0;
  if (!table->ReadU8(&op)) {
    return OTS_FAILURE();
  }
  if (op <= 21) {
    if (op == 12) {
      return ParseDictDataEscapedOperator(table, operands);
    }
    operands->push_back(std::make_pair(op, DICT_OPERATOR));
    return true;
  } else if (op <= 27 || op == 31 || op == 255) {
    
    return OTS_FAILURE();
  }

  return ParseDictDataNumber(table, op, operands);
}

bool ParsePrivateDictData(
    const uint8_t *data,
    size_t table_length, size_t offset, size_t dict_length,
    DICT_DATA_TYPE type, ots::OpenTypeCFF *out_cff) {
  ots::Buffer table(data + offset, dict_length);
  std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > operands;

  
  
  
  if (type == DICT_DATA_FDARRAY) {
    out_cff->local_subrs_per_font.push_back(new ots::CFFIndex);
  }

  while (table.offset() < dict_length) {
    if (!ParseDictDataReadNext(&table, &operands)) {
      return OTS_FAILURE();
    }
    if (operands.empty()) {
      return OTS_FAILURE();
    }
    if (operands.size() > 48) {
      
      return OTS_FAILURE();
    }
    if (operands.back().second != DICT_OPERATOR) {
      continue;
    }

    
    const uint32_t op = operands.back().first;
    operands.pop_back();

    switch (op) {
      
      case 6:  
      case 7:  
      case 8:  
      case 9:  
      case (12U << 8) + 12:  
      case (12U << 8) + 13:  
        if (operands.empty()) {
          return OTS_FAILURE();
        }
        break;

      
      case 10:  
      case 11:  
      case 20:  
      case 21:  
      case (12U << 8) + 9:   
      case (12U << 8) + 10:  
      case (12U << 8) + 11:  
      case (12U << 8) + 17:  
      case (12U << 8) + 18:  
      case (12U << 8) + 19:  
        if (operands.size() != 1) {
          return OTS_FAILURE();
        }
        break;

      
      case 19: {
        if (operands.size() != 1) {
          return OTS_FAILURE();
        }
        if (operands.back().second != DICT_OPERAND_INTEGER) {
          return OTS_FAILURE();
        }
        if (operands.back().first >= 1024 * 1024 * 1024) {
          return OTS_FAILURE();
        }
        if (operands.back().first + offset >= table_length) {
          return OTS_FAILURE();
        }
        
        ots::Buffer table(data, table_length);
        table.set_offset(operands.back().first + offset);
        ots::CFFIndex *local_subrs_index = NULL;
        if (type == DICT_DATA_FDARRAY) {
          if (out_cff->local_subrs_per_font.empty()) {
            return OTS_FAILURE();  
          }
          local_subrs_index = out_cff->local_subrs_per_font.back();
        } else if (type == DICT_DATA_TOPLEVEL) {
          if (out_cff->local_subrs) {
            return OTS_FAILURE();  
          }
          local_subrs_index = new ots::CFFIndex;
          out_cff->local_subrs = local_subrs_index;
        }
        if (!ParseIndex(&table, local_subrs_index)) {
          return OTS_FAILURE();
        }
        break;
      }

      
      case (12U << 8) + 14:  
        if (operands.size() != 1) {
          return OTS_FAILURE();
        }
        if (operands.back().second != DICT_OPERAND_INTEGER) {
          return OTS_FAILURE();
        }
        if (operands.back().first >= 2) {
          return OTS_FAILURE();
        }
        break;

      default:
        return OTS_FAILURE();
    }
    operands.clear();
  }

  return true;
}

bool ParseDictData(const uint8_t *data, size_t table_length,
                   const ots::CFFIndex &index, size_t sid_max,
                   DICT_DATA_TYPE type, ots::OpenTypeCFF *out_cff) {
  for (unsigned i = 1; i < index.offsets.size(); ++i) {
    if (type == DICT_DATA_TOPLEVEL) {
      out_cff->char_strings_array.push_back(new ots::CFFIndex);
    }
    size_t dict_length = index.offsets[i] - index.offsets[i - 1];
    ots::Buffer table(data + index.offsets[i - 1], dict_length);

    std::vector<std::pair<uint32_t, DICT_OPERAND_TYPE> > operands;

    bool have_ros = false;
    size_t glyphs = 0;
    size_t charset_offset = 0;

    while (table.offset() < dict_length) {
      if (!ParseDictDataReadNext(&table, &operands)) {
        return OTS_FAILURE();
      }
      if (operands.empty()) {
        return OTS_FAILURE();
      }
      if (operands.size() > 48) {
        
        return OTS_FAILURE();
      }
      if (operands.back().second != DICT_OPERATOR) continue;

      
      const uint32_t op = operands.back().first;
      operands.pop_back();

      switch (op) {
        
        case 0:   
        case 1:   
        case 2:   
        case 3:   
        case 4:   
        case (12U << 8) + 0:   
        case (12U << 8) + 21:  
        case (12U << 8) + 22:  
        case (12U << 8) + 38:  
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (!CheckSid(operands.back(), sid_max)) {
            return OTS_FAILURE();
          }
          break;

        
        case 5:   
        case 14:  
        case (12U << 8) + 7:   
        case (12U << 8) + 23:  
          if (operands.empty()) {
            return OTS_FAILURE();
          }
          break;

        
        case 13:  
        case (12U << 8) + 2:   
        case (12U << 8) + 3:   
        case (12U << 8) + 4:   
        case (12U << 8) + 5:   
        case (12U << 8) + 8:   
        case (12U << 8) + 20:  
        case (12U << 8) + 31:  
        case (12U << 8) + 32:  
        case (12U << 8) + 33:  
        case (12U << 8) + 34:  
        case (12U << 8) + 35:  
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          break;
        case (12U << 8) + 6:   
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if(operands.back().second != DICT_OPERAND_INTEGER) {
            return OTS_FAILURE();
          }
          if (operands.back().first != 2) {
            
            
            return OTS_FAILURE();
          }
          break;

        
        case (12U << 8) + 1:   
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (operands.back().second != DICT_OPERAND_INTEGER) {
            return OTS_FAILURE();
          }
          if (operands.back().first >= 2) {
            return OTS_FAILURE();
          }
          break;

        
        case 15:  
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (operands.back().first <= 2) {
            
            break;
          }
          if (!CheckOffset(operands.back(), table_length)) {
            return OTS_FAILURE();
          }
          if (charset_offset) {
            return OTS_FAILURE();  
          }
          charset_offset = operands.back().first;
          break;

        case 16: {  
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (operands.back().first <= 1) {
            break;  
          }
          if (!CheckOffset(operands.back(), table_length)) {
            return OTS_FAILURE();
          }

          
          ots::Buffer table(data, table_length);
          table.set_offset(operands.back().first);
          uint8_t format = 0;
          if (!table.ReadU8(&format)) {
            return OTS_FAILURE();
          }
          if (format & 0x80) {
            
            return OTS_FAILURE();
          }
          
          break;
        }

        case 17: {  
          if (type != DICT_DATA_TOPLEVEL) {
            return OTS_FAILURE();
          }
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (!CheckOffset(operands.back(), table_length)) {
            return OTS_FAILURE();
          }
          
          ots::Buffer table(data, table_length);
          table.set_offset(operands.back().first);
          ots::CFFIndex *charstring_index = out_cff->char_strings_array.back();
          if (!ParseIndex(&table, charstring_index)) {
            return OTS_FAILURE();
          }
          if (charstring_index->count < 2) {
            return OTS_FAILURE();
          }
          if (glyphs) {
            return OTS_FAILURE();  
          }
          glyphs = charstring_index->count;
          break;
        }

        case (12U << 8) + 36: {  
          if (type != DICT_DATA_TOPLEVEL) {
            return OTS_FAILURE();
          }
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (!CheckOffset(operands.back(), table_length)) {
            return OTS_FAILURE();
          }

          
          ots::Buffer table(data, table_length);
          table.set_offset(operands.back().first);
          ots::CFFIndex sub_dict_index;
          if (!ParseIndex(&table, &sub_dict_index)) {
            return OTS_FAILURE();
          }
          if (!ParseDictData(data, table_length,
                             sub_dict_index, sid_max, DICT_DATA_FDARRAY,
                             out_cff)) {
            return OTS_FAILURE();
          }
          if (out_cff->font_dict_length != 0) {
            return OTS_FAILURE();  
          }
          out_cff->font_dict_length = sub_dict_index.count;
          break;
        }

        case (12U << 8) + 37: {  
          if (type != DICT_DATA_TOPLEVEL) {
            return OTS_FAILURE();
          }
          if (operands.size() != 1) {
            return OTS_FAILURE();
          }
          if (!CheckOffset(operands.back(), table_length)) {
            return OTS_FAILURE();
          }

          
          ots::Buffer table(data, table_length);
          table.set_offset(operands.back().first);
          uint8_t format = 0;
          if (!table.ReadU8(&format)) {
            return OTS_FAILURE();
          }
          if (format == 0) {
            for (size_t j = 0; j < glyphs; ++j) {
              uint8_t fd_index = 0;
              if (!table.ReadU8(&fd_index)) {
                return OTS_FAILURE();
              }
              (out_cff->fd_select)[j] = fd_index;
            }
          } else if (format == 3) {
            uint16_t n_ranges = 0;
            if (!table.ReadU16(&n_ranges)) {
              return OTS_FAILURE();
            }
            if (n_ranges == 0) {
              return OTS_FAILURE();
            }

            uint16_t last_gid = 0;
            uint8_t fd_index = 0;
            for (unsigned j = 0; j < n_ranges; ++j) {
              uint16_t first = 0;  
              if (!table.ReadU16(&first)) {
                return OTS_FAILURE();
              }

              
              if ((j == 0) && (first != 0)) {
                return OTS_FAILURE();
              }
              if ((j != 0) && (last_gid >= first)) {
                return OTS_FAILURE();  
              }

              
              if (j != 0) {
                for (uint16_t k = last_gid; k < first; ++k) {
                  if (!out_cff->fd_select.insert(
                          std::make_pair(k, fd_index)).second) {
                    return OTS_FAILURE();
                  }
                }
              }

              if (!table.ReadU8(&fd_index)) {
                return OTS_FAILURE();
              }
              last_gid = first;
              
            }
            uint16_t sentinel = 0;
            if (!table.ReadU16(&sentinel)) {
              return OTS_FAILURE();
            }
            if (last_gid >= sentinel) {
              return OTS_FAILURE();
            }
            for (uint16_t k = last_gid; k < sentinel; ++k) {
              if (!out_cff->fd_select.insert(
                      std::make_pair(k, fd_index)).second) {
                return OTS_FAILURE();
              }
            }
          } else {
            
            return OTS_FAILURE();
          }
          break;
        }

        
        case 18: {
          if (operands.size() != 2) {
            return OTS_FAILURE();
          }
          if (operands.back().second != DICT_OPERAND_INTEGER) {
            return OTS_FAILURE();
          }
          const uint32_t private_offset = operands.back().first;
          operands.pop_back();
          if (operands.back().second != DICT_OPERAND_INTEGER) {
            return OTS_FAILURE();
          }
          const uint32_t private_length = operands.back().first;
          if (private_offset >= table_length) {
            return OTS_FAILURE();
          }
          if (private_length >= table_length) {
            return OTS_FAILURE();
          }
          if (private_length + private_offset > table_length) {
            
            return OTS_FAILURE();
          }
          
          if (!ParsePrivateDictData(data, table_length,
                                    private_offset, private_length,
                                    type, out_cff)) {
            return OTS_FAILURE();
          }
          break;
        }

        
        case (12U << 8) + 30:
          if (type != DICT_DATA_TOPLEVEL) {
            return OTS_FAILURE();
          }
          if (operands.size() != 3) {
            return OTS_FAILURE();
          }
          
          operands.pop_back();  
          if (!CheckSid(operands.back(), sid_max)) {
            return OTS_FAILURE();
          }
          operands.pop_back();
          if (!CheckSid(operands.back(), sid_max)) {
            return OTS_FAILURE();
          }
          if (have_ros) {
            return OTS_FAILURE();  
          }
          have_ros = true;
          break;

        default:
          return OTS_FAILURE();
      }
      operands.clear();
    }

    
    if (charset_offset) {
      ots::Buffer table(data, table_length);
      table.set_offset(charset_offset);
      uint8_t format = 0;
      if (!table.ReadU8(&format)) {
        return OTS_FAILURE();
      }
      switch (format) {
        case 0:
          for (unsigned j = 1 ; j < glyphs; ++j) {
            uint16_t sid = 0;
            if (!table.ReadU16(&sid)) {
              return OTS_FAILURE();
            }
            if (!have_ros && (sid > sid_max)) {
              return OTS_FAILURE();
            }
            
          }
          break;

        case 1:
        case 2: {
          uint32_t total = 1;  
          while (total < glyphs) {
            uint16_t sid = 0;
            if (!table.ReadU16(&sid)) {
              return OTS_FAILURE();
            }
            if (!have_ros && (sid > sid_max)) {
              return OTS_FAILURE();
            }
            

            if (format == 1) {
              uint8_t left = 0;
              if (!table.ReadU8(&left)) {
                return OTS_FAILURE();
              }
              total += (left + 1);
            } else {
              uint16_t left = 0;
              if (!table.ReadU16(&left)) {
                return OTS_FAILURE();
              }
              total += (left + 1);
            }
          }
          break;
        }

        default:
          return OTS_FAILURE();
      }
    }
  }
  return true;
}

}  

namespace ots {

bool ots_cff_parse(OpenTypeFile *file, const uint8_t *data, size_t length) {
  Buffer table(data, length);

  file->cff = new OpenTypeCFF;
  file->cff->data = data;
  file->cff->length = length;
  file->cff->font_dict_length = 0;
  file->cff->local_subrs = NULL;

  
  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t hdr_size = 0;
  uint8_t off_size = 0;
  if (!table.ReadU8(&major)) {
    return OTS_FAILURE();
  }
  if (!table.ReadU8(&minor)) {
    return OTS_FAILURE();
  }
  if (!table.ReadU8(&hdr_size)) {
    return OTS_FAILURE();
  }
  if (!table.ReadU8(&off_size)) {
    return OTS_FAILURE();
  }
  if ((off_size == 0) || (off_size > 4)) {
    return OTS_FAILURE();
  }

  if ((major != 1) ||
      (minor != 0) ||
      (hdr_size != 4)) {
    return OTS_FAILURE();
  }
  if (hdr_size >= length) {
    return OTS_FAILURE();
  }

  
  table.set_offset(hdr_size);
  CFFIndex name_index;
  if (!ParseIndex(&table, &name_index)) {
    return OTS_FAILURE();
  }
  if (!ParseNameData(&table, name_index, &(file->cff->name))) {
    return OTS_FAILURE();
  }

  
  table.set_offset(name_index.offset_to_next);
  CFFIndex top_dict_index;
  if (!ParseIndex(&table, &top_dict_index)) {
    return OTS_FAILURE();
  }
  if (name_index.count != top_dict_index.count) {
    return OTS_FAILURE();
  }

  
  table.set_offset(top_dict_index.offset_to_next);
  CFFIndex string_index;
  if (!ParseIndex(&table, &string_index)) {
    return OTS_FAILURE();
  }
  if (string_index.count >= 65000 - kNStdString) {
    return OTS_FAILURE();
  }

  const size_t sid_max = string_index.count + kNStdString;
  

  
  if (!ParseDictData(data, length, top_dict_index,
                     sid_max, DICT_DATA_TOPLEVEL, file->cff)) {
    return OTS_FAILURE();
  }

  
  table.set_offset(string_index.offset_to_next);
  CFFIndex global_subrs_index;
  if (!ParseIndex(&table, &global_subrs_index)) {
    return OTS_FAILURE();
  }

  
  std::map<uint16_t, uint8_t>::const_iterator iter;
  std::map<uint16_t, uint8_t>::const_iterator end = file->cff->fd_select.end();
  for (iter = file->cff->fd_select.begin(); iter != end; ++iter) {
    if (iter->second >= file->cff->font_dict_length) {
      return OTS_FAILURE();
    }
  }

  
  for (size_t i = 0; i < file->cff->char_strings_array.size(); ++i) {
    if (!ValidateType2CharStringIndex(*(file->cff->char_strings_array.at(i)),
                                      global_subrs_index,
                                      file->cff->fd_select,
                                      file->cff->local_subrs_per_font,
                                      file->cff->local_subrs,
                                      &table)) {
      return OTS_FAILURE();
    }
  }

  return true;
}

bool ots_cff_should_serialise(OpenTypeFile *file) {
  return file->cff != NULL;
}

bool ots_cff_serialise(OTSStream *out, OpenTypeFile *file) {
  
  
  if (!out->Write(file->cff->data, file->cff->length)) {
    return OTS_FAILURE();
  }
  return true;
}

void ots_cff_free(OpenTypeFile *file) {
  if (file->cff) {
    for (size_t i = 0; i < file->cff->char_strings_array.size(); ++i) {
      delete (file->cff->char_strings_array)[i];
    }
    for (size_t i = 0; i < file->cff->local_subrs_per_font.size(); ++i) {
      delete (file->cff->local_subrs_per_font)[i];
    }
    delete file->cff->local_subrs;
    delete file->cff;
  }
}

}  
