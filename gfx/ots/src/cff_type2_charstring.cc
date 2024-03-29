






#include "cff_type2_charstring.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <stack>
#include <string>
#include <utility>

#define TABLE_NAME "CFF"

namespace {



const int32_t kMaxSubrsCount = 65536;
const size_t kMaxCharStringLength = 65535;
const size_t kMaxArgumentStack = 48;
const size_t kMaxNumberOfStemHints = 96;
const size_t kMaxSubrNesting = 10;



const int32_t dummy_result = INT_MAX;

bool ExecuteType2CharString(ots::OpenTypeFile *file,
                            size_t call_depth,
                            const ots::CFFIndex& global_subrs_index,
                            const ots::CFFIndex& local_subrs_index,
                            ots::Buffer *cff_table,
                            ots::Buffer *char_string,
                            std::stack<int32_t> *argument_stack,
                            bool *out_found_endchar,
                            bool *out_found_width,
                            size_t *in_out_num_stems);

#ifdef DUMP_T2CHARSTRING

const char *Type2CharStringOperatorToString(ots::Type2CharStringOperator op) {
  switch (op) {
  case ots::kHStem:
    return "HStem";
  case ots::kVStem:
    return "VStem";
  case ots::kVMoveTo:
    return "VMoveTo";
  case ots::kRLineTo:
    return "RLineTo";
  case ots::kHLineTo:
    return "HLineTo";
  case ots::kVLineTo:
    return "VLineTo";
  case ots::kRRCurveTo:
    return "RRCurveTo";
  case ots::kCallSubr:
    return "CallSubr";
  case ots::kReturn:
    return "Return";
  case ots::kEndChar:
    return "EndChar";
  case ots::kHStemHm:
    return "HStemHm";
  case ots::kHintMask:
    return "HintMask";
  case ots::kCntrMask:
    return "CntrMask";
  case ots::kRMoveTo:
    return "RMoveTo";
  case ots::kHMoveTo:
    return "HMoveTo";
  case ots::kVStemHm:
    return "VStemHm";
  case ots::kRCurveLine:
    return "RCurveLine";
  case ots::kRLineCurve:
    return "RLineCurve";
  case ots::kVVCurveTo:
    return "VVCurveTo";
  case ots::kHHCurveTo:
    return "HHCurveTo";
  case ots::kCallGSubr:
    return "CallGSubr";
  case ots::kVHCurveTo:
    return "VHCurveTo";
  case ots::kHVCurveTo:
    return "HVCurveTo";
  case ots::kDotSection:
    return "DotSection";
  case ots::kAnd:
    return "And";
  case ots::kOr:
    return "Or";
  case ots::kNot:
    return "Not";
  case ots::kAbs:
    return "Abs";
  case ots::kAdd:
    return "Add";
  case ots::kSub:
    return "Sub";
  case ots::kDiv:
    return "Div";
  case ots::kNeg:
    return "Neg";
  case ots::kEq:
    return "Eq";
  case ots::kDrop:
    return "Drop";
  case ots::kPut:
    return "Put";
  case ots::kGet:
    return "Get";
  case ots::kIfElse:
    return "IfElse";
  case ots::kRandom:
    return "Random";
  case ots::kMul:
    return "Mul";
  case ots::kSqrt:
    return "Sqrt";
  case ots::kDup:
    return "Dup";
  case ots::kExch:
    return "Exch";
  case ots::kIndex:
    return "Index";
  case ots::kRoll:
    return "Roll";
  case ots::kHFlex:
    return "HFlex";
  case ots::kFlex:
    return "Flex";
  case ots::kHFlex1:
    return "HFlex1";
  case ots::kFlex1:
    return "Flex1";
  }

  return "UNKNOWN";
}
#endif




bool ReadNextNumberFromType2CharString(ots::Buffer *char_string,
                                       int32_t *out_number,
                                       bool *out_is_operator) {
  uint8_t v = 0;
  if (!char_string->ReadU8(&v)) {
    return OTS_FAILURE();
  }
  *out_is_operator = false;

  
  
  if (v <= 11) {
    *out_number = v;
    *out_is_operator = true;
  } else if (v == 12) {
    uint16_t result = (v << 8);
    if (!char_string->ReadU8(&v)) {
      return OTS_FAILURE();
    }
    result += v;
    *out_number = result;
    *out_is_operator = true;
  } else if (v <= 27) {
    
    
    *out_number = v;
    *out_is_operator = true;
  } else if (v == 28) {
    if (!char_string->ReadU8(&v)) {
      return OTS_FAILURE();
    }
    uint16_t result = (v << 8);
    if (!char_string->ReadU8(&v)) {
      return OTS_FAILURE();
    }
    result += v;
    *out_number = result;
  } else if (v <= 31) {
    *out_number = v;
    *out_is_operator = true;
  } else if (v <= 246) {
    *out_number = static_cast<int32_t>(v) - 139;
  } else if (v <= 250) {
    uint8_t w = 0;
    if (!char_string->ReadU8(&w)) {
      return OTS_FAILURE();
    }
    *out_number = ((static_cast<int32_t>(v) - 247) * 256) +
        static_cast<int32_t>(w) + 108;
  } else if (v <= 254) {
    uint8_t w = 0;
    if (!char_string->ReadU8(&w)) {
      return OTS_FAILURE();
    }
    *out_number = -((static_cast<int32_t>(v) - 251) * 256) -
        static_cast<int32_t>(w) - 108;
  } else if (v == 255) {
    
    
    
    if (!char_string->Skip(4)) {
      return OTS_FAILURE();
    }
    *out_number = dummy_result;
  } else {
    return OTS_FAILURE();
  }

  return true;
}





bool ExecuteType2CharStringOperator(ots::OpenTypeFile *file,
                                    int32_t op,
                                    size_t call_depth,
                                    const ots::CFFIndex& global_subrs_index,
                                    const ots::CFFIndex& local_subrs_index,
                                    ots::Buffer *cff_table,
                                    ots::Buffer *char_string,
                                    std::stack<int32_t> *argument_stack,
                                    bool *out_found_endchar,
                                    bool *in_out_found_width,
                                    size_t *in_out_num_stems) {
  const size_t stack_size = argument_stack->size();

  switch (op) {
  case ots::kCallSubr:
  case ots::kCallGSubr: {
    const ots::CFFIndex& subrs_index =
        (op == ots::kCallSubr ? local_subrs_index : global_subrs_index);

    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    int32_t subr_number = argument_stack->top();
    argument_stack->pop();
    if (subr_number == dummy_result) {
      
      
      
      
      return OTS_FAILURE();
    }

    
    int32_t bias = 32768;
    if (subrs_index.count < 1240) {
      bias = 107;
    } else if (subrs_index.count < 33900) {
      bias = 1131;
    }
    subr_number += bias;

    
    if (subr_number < 0) {
      return OTS_FAILURE();
    }
    if (subr_number >= kMaxSubrsCount) {
      return OTS_FAILURE();
    }
    if (subrs_index.offsets.size() <= static_cast<size_t>(subr_number + 1)) {
      return OTS_FAILURE();  
    }

    
    const size_t length =
      subrs_index.offsets[subr_number + 1] - subrs_index.offsets[subr_number];
    if (length > kMaxCharStringLength) {
      return OTS_FAILURE();
    }
    const size_t offset = subrs_index.offsets[subr_number];
    cff_table->set_offset(offset);
    if (!cff_table->Skip(length)) {
      return OTS_FAILURE();
    }
    ots::Buffer char_string_to_jump(cff_table->buffer() + offset, length);

    return ExecuteType2CharString(file,
                                  call_depth + 1,
                                  global_subrs_index,
                                  local_subrs_index,
                                  cff_table,
                                  &char_string_to_jump,
                                  argument_stack,
                                  out_found_endchar,
                                  in_out_found_width,
                                  in_out_num_stems);
  }

  case ots::kReturn:
    return true;

  case ots::kEndChar:
    *out_found_endchar = true;
    *in_out_found_width = true;  
    return true;

  case ots::kHStem:
  case ots::kVStem:
  case ots::kHStemHm:
  case ots::kVStemHm: {
    bool successful = false;
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    if ((stack_size % 2) == 0) {
      successful = true;
    } else if ((!(*in_out_found_width)) && (((stack_size - 1) % 2) == 0)) {
      
      
      successful = true;
    }
    (*in_out_num_stems) += (stack_size / 2);
    if ((*in_out_num_stems) > kMaxNumberOfStemHints) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    *in_out_found_width = true;  
    return successful ? true : OTS_FAILURE();
  }

  case ots::kRMoveTo: {
    bool successful = false;
    if (stack_size == 2) {
      successful = true;
    } else if ((!(*in_out_found_width)) && (stack_size - 1 == 2)) {
      successful = true;
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    *in_out_found_width = true;
    return successful ? true : OTS_FAILURE();
  }

  case ots::kVMoveTo:
  case ots::kHMoveTo: {
    bool successful = false;
    if (stack_size == 1) {
      successful = true;
    } else if ((!(*in_out_found_width)) && (stack_size - 1 == 1)) {
      successful = true;
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    *in_out_found_width = true;
    return successful ? true : OTS_FAILURE();
  }

  case ots::kHintMask:
  case ots::kCntrMask: {
    bool successful = false;
    if (stack_size == 0) {
      successful = true;
    } else if ((!(*in_out_found_width)) && (stack_size == 1)) {
      
      successful = true;
    } else if ((!(*in_out_found_width)) ||  
               ((stack_size % 2) == 0)) {
      
      
      (*in_out_num_stems) += (stack_size / 2);
      if ((*in_out_num_stems) > kMaxNumberOfStemHints) {
        return OTS_FAILURE();
      }
      successful = true;
    }
    if (!successful) {
       return OTS_FAILURE();
    }

    if ((*in_out_num_stems) == 0) {
      return OTS_FAILURE();
    }
    const size_t mask_bytes = (*in_out_num_stems + 7) / 8;
    if (!char_string->Skip(mask_bytes)) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    *in_out_found_width = true;
    return true;
  }

  case ots::kRLineTo:
    if (!(*in_out_found_width)) {
      
      
      
      return OTS_FAILURE();
    }
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    if ((stack_size % 2) != 0) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kHLineTo:
  case ots::kVLineTo:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kRRCurveTo:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 6) {
      return OTS_FAILURE();
    }
    if ((stack_size % 6) != 0) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kRCurveLine:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 8) {
      return OTS_FAILURE();
    }
    if (((stack_size - 2) % 6) != 0) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kRLineCurve:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 8) {
      return OTS_FAILURE();
    }
    if (((stack_size - 6) % 2) != 0) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kVVCurveTo:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 4) {
      return OTS_FAILURE();
    }
    if (((stack_size % 4) != 0) &&
        (((stack_size - 1) % 4) != 0)) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kHHCurveTo: {
    bool successful = false;
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 4) {
      return OTS_FAILURE();
    }
    if ((stack_size % 4) == 0) {
      
      successful = true;
    } else if (((stack_size - 1) % 4) == 0) {
      
      successful = true;
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return successful ? true : OTS_FAILURE();
  }

  case ots::kVHCurveTo:
  case ots::kHVCurveTo: {
    bool successful = false;
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size < 4) {
      return OTS_FAILURE();
    }
    if (((stack_size - 4) % 8) == 0) {
      
      successful = true;
    } else if ((stack_size >= 5) &&
               ((stack_size - 5) % 8) == 0) {
      
      successful = true;
    } else if ((stack_size >= 8) &&
               ((stack_size - 8) % 8) == 0) {
      
      successful = true;
    } else if ((stack_size >= 9) &&
               ((stack_size - 9) % 8) == 0) {
      
      successful = true;
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return successful ? true : OTS_FAILURE();
  }

  case ots::kDotSection:
    
    if (stack_size != 0) {
      return OTS_FAILURE();
    }
    return true;

  case ots::kAnd:
  case ots::kOr:
  case ots::kEq:
  case ots::kAdd:
  case ots::kSub:
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kNot:
  case ots::kAbs:
  case ots::kNeg:
    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kDiv:
    
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kDrop:
    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    return true;

  case ots::kPut:
  case ots::kGet:
  case ots::kIndex:
    
    
    
    
    return OTS_FAILURE();

  case ots::kRoll:
    
    
    
    return OTS_FAILURE();

  case ots::kRandom:
    
    
    return OTS_FAILURE();

  case ots::kIfElse:
    if (stack_size < 4) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kMul:
    
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kSqrt:
    
    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kDup:
    if (stack_size < 1) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->push(dummy_result);
    argument_stack->push(dummy_result);
    if (argument_stack->size() > kMaxArgumentStack) {
      return OTS_FAILURE();
    }
    
    
    return true;

  case ots::kExch:
    if (stack_size < 2) {
      return OTS_FAILURE();
    }
    argument_stack->pop();
    argument_stack->pop();
    argument_stack->push(dummy_result);
    argument_stack->push(dummy_result);
    
    
    return true;

  case ots::kHFlex:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size != 7) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kFlex:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size != 13) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kHFlex1:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size != 9) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;

  case ots::kFlex1:
    if (!(*in_out_found_width)) {
      return OTS_FAILURE();
    }
    if (stack_size != 11) {
      return OTS_FAILURE();
    }
    while (!argument_stack->empty())
      argument_stack->pop();
    return true;
  }

  return OTS_FAILURE_MSG("Undefined operator: %d (0x%x)", op, op);
}














bool ExecuteType2CharString(ots::OpenTypeFile *file,
                            size_t call_depth,
                            const ots::CFFIndex& global_subrs_index,
                            const ots::CFFIndex& local_subrs_index,
                            ots::Buffer *cff_table,
                            ots::Buffer *char_string,
                            std::stack<int32_t> *argument_stack,
                            bool *out_found_endchar,
                            bool *in_out_found_width,
                            size_t *in_out_num_stems) {
  if (call_depth > kMaxSubrNesting) {
    return OTS_FAILURE();
  }
  *out_found_endchar = false;

  const size_t length = char_string->length();
  while (char_string->offset() < length) {
    int32_t operator_or_operand = 0;
    bool is_operator = false;
    if (!ReadNextNumberFromType2CharString(char_string,
                                           &operator_or_operand,
                                           &is_operator)) {
      return OTS_FAILURE();
    }

#ifdef DUMP_T2CHARSTRING
    




      if (!is_operator) {
        std::fprintf(stderr, "#%d# ", operator_or_operand);
      } else {
        std::fprintf(stderr, "#%s#\n",
           Type2CharStringOperatorToString(
               ots::Type2CharStringOperator(operator_or_operand))
           );
      }
#endif

    if (!is_operator) {
      argument_stack->push(operator_or_operand);
      if (argument_stack->size() > kMaxArgumentStack) {
        return OTS_FAILURE();
      }
      continue;
    }

    
    if (!ExecuteType2CharStringOperator(file,
                                        operator_or_operand,
                                        call_depth,
                                        global_subrs_index,
                                        local_subrs_index,
                                        cff_table,
                                        char_string,
                                        argument_stack,
                                        out_found_endchar,
                                        in_out_found_width,
                                        in_out_num_stems)) {
      return OTS_FAILURE();
    }
    if (*out_found_endchar) {
      return true;
    }
    if (operator_or_operand == ots::kReturn) {
      return true;
    }
  }

  
  return OTS_FAILURE();
}



bool SelectLocalSubr(const std::map<uint16_t, uint8_t> &fd_select,
                     const std::vector<ots::CFFIndex *> &local_subrs_per_font,
                     const ots::CFFIndex *local_subrs,
                     uint16_t glyph_index,  
                     const ots::CFFIndex **out_local_subrs_to_use) {
  *out_local_subrs_to_use = NULL;

  
  if ((fd_select.size() > 0) &&
      (!local_subrs_per_font.empty())) {
    
    std::map<uint16_t, uint8_t>::const_iterator iter =
        fd_select.find(glyph_index);
    if (iter == fd_select.end()) {
      return OTS_FAILURE();
    }
    const uint8_t fd_index = iter->second;
    if (fd_index >= local_subrs_per_font.size()) {
      return OTS_FAILURE();
    }
    *out_local_subrs_to_use = local_subrs_per_font.at(fd_index);
  } else if (local_subrs) {
    
    
    
    *out_local_subrs_to_use = local_subrs;
  } else {
    
    *out_local_subrs_to_use = NULL;
  }

  return true;
}

}  

namespace ots {

bool ValidateType2CharStringIndex(
    ots::OpenTypeFile *file,
    const CFFIndex& char_strings_index,
    const CFFIndex& global_subrs_index,
    const std::map<uint16_t, uint8_t> &fd_select,
    const std::vector<CFFIndex *> &local_subrs_per_font,
    const CFFIndex *local_subrs,
    Buffer* cff_table) {
  const uint16_t num_offsets =
      static_cast<uint16_t>(char_strings_index.offsets.size());
  if (num_offsets != char_strings_index.offsets.size() || num_offsets == 0) {
    return OTS_FAILURE();  
  }

  
  for (uint16_t i = 1; i < num_offsets; ++i) {
    
    
    const size_t length =
      char_strings_index.offsets[i] - char_strings_index.offsets[i - 1];
    if (length > kMaxCharStringLength) {
      return OTS_FAILURE();
    }
    const size_t offset = char_strings_index.offsets[i - 1];
    cff_table->set_offset(offset);
    if (!cff_table->Skip(length)) {
      return OTS_FAILURE();
    }
    Buffer char_string(cff_table->buffer() + offset, length);

    
    const uint16_t glyph_index = i - 1;  
    const CFFIndex *local_subrs_to_use = NULL;
    if (!SelectLocalSubr(fd_select,
                         local_subrs_per_font,
                         local_subrs,
                         glyph_index,
                         &local_subrs_to_use)) {
      return OTS_FAILURE();
    }
    
    CFFIndex default_empty_subrs;
    if (!local_subrs_to_use){
      local_subrs_to_use = &default_empty_subrs;
    }

    
    std::stack<int32_t> argument_stack;
    bool found_endchar = false;
    bool found_width = false;
    size_t num_stems = 0;
    if (!ExecuteType2CharString(file,
                                0 ,
                                global_subrs_index, *local_subrs_to_use,
                                cff_table, &char_string, &argument_stack,
                                &found_endchar, &found_width, &num_stems)) {
      return OTS_FAILURE();
    }
    if (!found_endchar) {
      return OTS_FAILURE();
    }
  }
  return true;
}

}  

#undef TABLE_NAME
