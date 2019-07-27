

































#include <algorithm>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stack>
#include <limits>
#include <vector>

#include <google/protobuf/text_format.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/map_util.h>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {

namespace {

inline bool IsHexNumber(const string& str) {
  return (str.length() >= 2 && str[0] == '0' &&
          (str[1] == 'x' || str[1] == 'X'));
}

inline bool IsOctNumber(const string& str) {
  return (str.length() >= 2 && str[0] == '0' &&
          (str[1] >= '0' && str[1] < '8'));
}

}  

string Message::DebugString() const {
  string debug_string;

  TextFormat::PrintToString(*this, &debug_string);

  return debug_string;
}

string Message::ShortDebugString() const {
  string debug_string;

  TextFormat::Printer printer;
  printer.SetSingleLineMode(true);

  printer.PrintToString(*this, &debug_string);
  
  if (debug_string.size() > 0 &&
      debug_string[debug_string.size() - 1] == ' ') {
    debug_string.resize(debug_string.size() - 1);
  }

  return debug_string;
}

string Message::Utf8DebugString() const {
  string debug_string;

  TextFormat::Printer printer;
  printer.SetUseUtf8StringEscaping(true);

  printer.PrintToString(*this, &debug_string);

  return debug_string;
}

void Message::PrintDebugString() const {
  printf("%s", DebugString().c_str());
}




TextFormat::ParseInfoTree::ParseInfoTree() { }

TextFormat::ParseInfoTree::~ParseInfoTree() {
  
  for (NestedMap::iterator it = nested_.begin(); it != nested_.end(); ++it) {
    STLDeleteElements(&(it->second));
  }
}

void TextFormat::ParseInfoTree::RecordLocation(
    const FieldDescriptor* field,
    TextFormat::ParseLocation location) {
  locations_[field].push_back(location);
}

TextFormat::ParseInfoTree* TextFormat::ParseInfoTree::CreateNested(
    const FieldDescriptor* field) {
  
  TextFormat::ParseInfoTree* instance = new TextFormat::ParseInfoTree();
  vector<TextFormat::ParseInfoTree*>* trees = &nested_[field];
  GOOGLE_CHECK(trees);
  trees->push_back(instance);
  return instance;
}

void CheckFieldIndex(const FieldDescriptor* field, int index) {
  if (field == NULL) { return; }

  if (field->is_repeated() && index == -1) {
    GOOGLE_LOG(DFATAL) << "Index must be in range of repeated field values. "
                << "Field: " << field->name();
  } else if (!field->is_repeated() && index != -1) {
    GOOGLE_LOG(DFATAL) << "Index must be -1 for singular fields."
                << "Field: " << field->name();
  }
}

TextFormat::ParseLocation TextFormat::ParseInfoTree::GetLocation(
    const FieldDescriptor* field, int index) const {
  CheckFieldIndex(field, index);
  if (index == -1) { index = 0; }

  const vector<TextFormat::ParseLocation>* locations =
      FindOrNull(locations_, field);
  if (locations == NULL || index >= locations->size()) {
    return TextFormat::ParseLocation();
  }

  return (*locations)[index];
}

TextFormat::ParseInfoTree* TextFormat::ParseInfoTree::GetTreeForNested(
    const FieldDescriptor* field, int index) const {
  CheckFieldIndex(field, index);
  if (index == -1) { index = 0; }

  const vector<TextFormat::ParseInfoTree*>* trees = FindOrNull(nested_, field);
  if (trees == NULL || index >= trees->size()) {
    return NULL;
  }

  return (*trees)[index];
}












#define DO(STATEMENT) if (STATEMENT) {} else return false

class TextFormat::Parser::ParserImpl {
 public:

  
  
  
  
  enum SingularOverwritePolicy {
    ALLOW_SINGULAR_OVERWRITES = 0,   
    FORBID_SINGULAR_OVERWRITES = 1,  
  };

  ParserImpl(const Descriptor* root_message_type,
             io::ZeroCopyInputStream* input_stream,
             io::ErrorCollector* error_collector,
             TextFormat::Finder* finder,
             ParseInfoTree* parse_info_tree,
             SingularOverwritePolicy singular_overwrite_policy,
             bool allow_case_insensitive_field,
             bool allow_unknown_field,
             bool allow_unknown_enum,
             bool allow_field_number,
             bool allow_relaxed_whitespace)
    : error_collector_(error_collector),
      finder_(finder),
      parse_info_tree_(parse_info_tree),
      tokenizer_error_collector_(this),
      tokenizer_(input_stream, &tokenizer_error_collector_),
      root_message_type_(root_message_type),
      singular_overwrite_policy_(singular_overwrite_policy),
      allow_case_insensitive_field_(allow_case_insensitive_field),
      allow_unknown_field_(allow_unknown_field),
      allow_unknown_enum_(allow_unknown_enum),
      allow_field_number_(allow_field_number),
      had_errors_(false) {
    
    
    tokenizer_.set_allow_f_after_float(true);

    
    tokenizer_.set_comment_style(io::Tokenizer::SH_COMMENT_STYLE);

    if (allow_relaxed_whitespace) {
      tokenizer_.set_require_space_after_number(false);
      tokenizer_.set_allow_multiline_strings(true);
    }

    
    tokenizer_.Next();
  }
  ~ParserImpl() { }

  
  
  
  
  bool Parse(Message* output) {
    
    while (true) {
      if (LookingAtType(io::Tokenizer::TYPE_END)) {
        return !had_errors_;
      }

      DO(ConsumeField(output));
    }
  }

  bool ParseField(const FieldDescriptor* field, Message* output) {
    bool suc;
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      suc = ConsumeFieldMessage(output, output->GetReflection(), field);
    } else {
      suc = ConsumeFieldValue(output, output->GetReflection(), field);
    }
    return suc && LookingAtType(io::Tokenizer::TYPE_END);
  }

  void ReportError(int line, int col, const string& message) {
    had_errors_ = true;
    if (error_collector_ == NULL) {
      if (line >= 0) {
        GOOGLE_LOG(ERROR) << "Error parsing text-format "
                   << root_message_type_->full_name()
                   << ": " << (line + 1) << ":"
                   << (col + 1) << ": " << message;
      } else {
        GOOGLE_LOG(ERROR) << "Error parsing text-format "
                   << root_message_type_->full_name()
                   << ": " << message;
      }
    } else {
      error_collector_->AddError(line, col, message);
    }
  }

  void ReportWarning(int line, int col, const string& message) {
    if (error_collector_ == NULL) {
      if (line >= 0) {
        GOOGLE_LOG(WARNING) << "Warning parsing text-format "
                     << root_message_type_->full_name()
                     << ": " << (line + 1) << ":"
                     << (col + 1) << ": " << message;
      } else {
        GOOGLE_LOG(WARNING) << "Warning parsing text-format "
                     << root_message_type_->full_name()
                     << ": " << message;
      }
    } else {
      error_collector_->AddWarning(line, col, message);
    }
  }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ParserImpl);

  
  
  void ReportError(const string& message) {
    ReportError(tokenizer_.current().line, tokenizer_.current().column,
                message);
  }

  
  
  void ReportWarning(const string& message) {
    ReportWarning(tokenizer_.current().line, tokenizer_.current().column,
                  message);
  }

  
  
  
  bool ConsumeMessage(Message* message, const string delimeter) {
    while (!LookingAt(">") &&  !LookingAt("}")) {
      DO(ConsumeField(message));
    }

    
    DO(Consume(delimeter));

    return true;
  }


  
  
  bool ConsumeField(Message* message) {
    const Reflection* reflection = message->GetReflection();
    const Descriptor* descriptor = message->GetDescriptor();

    string field_name;

    const FieldDescriptor* field = NULL;
    int start_line = tokenizer_.current().line;
    int start_column = tokenizer_.current().column;

    if (TryConsume("[")) {
      
      DO(ConsumeIdentifier(&field_name));
      while (TryConsume(".")) {
        string part;
        DO(ConsumeIdentifier(&part));
        field_name += ".";
        field_name += part;
      }
      DO(Consume("]"));

      field = (finder_ != NULL
               ? finder_->FindExtension(message, field_name)
               : reflection->FindKnownExtensionByName(field_name));

      if (field == NULL) {
        if (!allow_unknown_field_) {
          ReportError("Extension \"" + field_name + "\" is not defined or "
                      "is not an extension of \"" +
                      descriptor->full_name() + "\".");
          return false;
        } else {
          ReportWarning("Extension \"" + field_name + "\" is not defined or "
                        "is not an extension of \"" +
                        descriptor->full_name() + "\".");
        }
      }
    } else {
      DO(ConsumeIdentifier(&field_name));

      int32 field_number;
      if (allow_field_number_ && safe_strto32(field_name, &field_number)) {
        if (descriptor->IsExtensionNumber(field_number)) {
          field = reflection->FindKnownExtensionByNumber(field_number);
        } else {
          field = descriptor->FindFieldByNumber(field_number);
        }
      } else {
        field = descriptor->FindFieldByName(field_name);
        
        
        
        if (field == NULL) {
          string lower_field_name = field_name;
          LowerString(&lower_field_name);
          field = descriptor->FindFieldByName(lower_field_name);
          
          if (field != NULL && field->type() != FieldDescriptor::TYPE_GROUP) {
            field = NULL;
          }
        }
        
        if (field != NULL && field->type() == FieldDescriptor::TYPE_GROUP
            && field->message_type()->name() != field_name) {
          field = NULL;
        }

        if (field == NULL && allow_case_insensitive_field_) {
          string lower_field_name = field_name;
          LowerString(&lower_field_name);
          field = descriptor->FindFieldByLowercaseName(lower_field_name);
        }
      }

      if (field == NULL) {
        if (!allow_unknown_field_) {
          ReportError("Message type \"" + descriptor->full_name() +
                      "\" has no field named \"" + field_name + "\".");
          return false;
        } else {
          ReportWarning("Message type \"" + descriptor->full_name() +
                        "\" has no field named \"" + field_name + "\".");
        }
      }
    }

    
    if (field == NULL) {
      GOOGLE_CHECK(allow_unknown_field_);
      
      
      
      
      
      
      if (TryConsume(":") && !LookingAt("{") && !LookingAt("<")) {
        return SkipFieldValue();
      } else {
        return SkipFieldMessage();
      }
    }

    if (singular_overwrite_policy_ == FORBID_SINGULAR_OVERWRITES) {
      
      if (!field->is_repeated() && reflection->HasField(*message, field)) {
        ReportError("Non-repeated field \"" + field_name +
                    "\" is specified multiple times.");
        return false;
      }
      
      
      const OneofDescriptor* oneof = field->containing_oneof();
      if (oneof != NULL && reflection->HasOneof(*message, oneof)) {
        const FieldDescriptor* other_field =
            reflection->GetOneofFieldDescriptor(*message, oneof);
        ReportError("Field \"" + field_name + "\" is specified along with "
                    "field \"" + other_field->name() + "\", another member "
                    "of oneof \"" + oneof->name() + "\".");
        return false;
      }
    }

    
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      
      TryConsume(":");
    } else {
      
      DO(Consume(":"));
    }

    if (field->is_repeated() && TryConsume("[")) {
      
      while (true) {
        if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
          
          DO(ConsumeFieldMessage(message, reflection, field));
        } else {
          DO(ConsumeFieldValue(message, reflection, field));
        }
        if (TryConsume("]")) {
          break;
        }
        DO(Consume(","));
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      DO(ConsumeFieldMessage(message, reflection, field));
    } else {
      DO(ConsumeFieldValue(message, reflection, field));
    }

    
    
    TryConsume(";") || TryConsume(",");

    if (field->options().deprecated()) {
      ReportWarning("text format contains deprecated field \""
                    + field_name + "\"");
    }

    
    
    if (parse_info_tree_ != NULL) {
      RecordLocation(parse_info_tree_, field,
                     ParseLocation(start_line, start_column));
    }

    return true;
  }

  
  bool SkipField() {
    string field_name;
    if (TryConsume("[")) {
      
      DO(ConsumeIdentifier(&field_name));
      while (TryConsume(".")) {
        string part;
        DO(ConsumeIdentifier(&part));
        field_name += ".";
        field_name += part;
      }
      DO(Consume("]"));
    } else {
      DO(ConsumeIdentifier(&field_name));
    }

    
    
    
    
    
    
    if (TryConsume(":") && !LookingAt("{") && !LookingAt("<")) {
      DO(SkipFieldValue());
    } else {
      DO(SkipFieldMessage());
    }
    
    
    TryConsume(";") || TryConsume(",");
    return true;
  }

  bool ConsumeFieldMessage(Message* message,
                           const Reflection* reflection,
                           const FieldDescriptor* field) {

    
    
    ParseInfoTree* parent = parse_info_tree_;
    if (parent != NULL) {
      parse_info_tree_ = CreateNested(parent, field);
    }

    string delimeter;
    if (TryConsume("<")) {
      delimeter = ">";
    } else {
      DO(Consume("{"));
      delimeter = "}";
    }

    if (field->is_repeated()) {
      DO(ConsumeMessage(reflection->AddMessage(message, field), delimeter));
    } else {
      DO(ConsumeMessage(reflection->MutableMessage(message, field),
                        delimeter));
    }

    
    parse_info_tree_ = parent;
    return true;
  }

  
  
  bool SkipFieldMessage() {
    string delimeter;
    if (TryConsume("<")) {
      delimeter = ">";
    } else {
      DO(Consume("{"));
      delimeter = "}";
    }
    while (!LookingAt(">") &&  !LookingAt("}")) {
      DO(SkipField());
    }
    DO(Consume(delimeter));
    return true;
  }

  bool ConsumeFieldValue(Message* message,
                         const Reflection* reflection,
                         const FieldDescriptor* field) {




#define SET_FIELD(CPPTYPE, VALUE)                                  \
        if (field->is_repeated()) {                                \
          reflection->Add##CPPTYPE(message, field, VALUE);         \
        } else {                                                   \
          reflection->Set##CPPTYPE(message, field, VALUE);         \
        }                                                          \

    switch(field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32: {
        int64 value;
        DO(ConsumeSignedInteger(&value, kint32max));
        SET_FIELD(Int32, static_cast<int32>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_UINT32: {
        uint64 value;
        DO(ConsumeUnsignedInteger(&value, kuint32max));
        SET_FIELD(UInt32, static_cast<uint32>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_INT64: {
        int64 value;
        DO(ConsumeSignedInteger(&value, kint64max));
        SET_FIELD(Int64, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_UINT64: {
        uint64 value;
        DO(ConsumeUnsignedInteger(&value, kuint64max));
        SET_FIELD(UInt64, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_FLOAT: {
        double value;
        DO(ConsumeDouble(&value));
        SET_FIELD(Float, static_cast<float>(value));
        break;
      }

      case FieldDescriptor::CPPTYPE_DOUBLE: {
        double value;
        DO(ConsumeDouble(&value));
        SET_FIELD(Double, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_STRING: {
        string value;
        DO(ConsumeString(&value));
        SET_FIELD(String, value);
        break;
      }

      case FieldDescriptor::CPPTYPE_BOOL: {
        if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
          uint64 value;
          DO(ConsumeUnsignedInteger(&value, 1));
          SET_FIELD(Bool, value);
        } else {
          string value;
          DO(ConsumeIdentifier(&value));
          if (value == "true" || value == "True" || value == "t") {
            SET_FIELD(Bool, true);
          } else if (value == "false" || value == "False" || value == "f") {
            SET_FIELD(Bool, false);
          } else {
            ReportError("Invalid value for boolean field \"" + field->name()
                        + "\". Value: \"" + value  + "\".");
            return false;
          }
        }
        break;
      }

      case FieldDescriptor::CPPTYPE_ENUM: {
        string value;
        const EnumDescriptor* enum_type = field->enum_type();
        const EnumValueDescriptor* enum_value = NULL;

        if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
          DO(ConsumeIdentifier(&value));
          
          enum_value = enum_type->FindValueByName(value);

        } else if (LookingAt("-") ||
                   LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
          int64 int_value;
          DO(ConsumeSignedInteger(&int_value, kint32max));
          value = SimpleItoa(int_value);        
          enum_value = enum_type->FindValueByNumber(int_value);
        } else {
          ReportError("Expected integer or identifier.");
          return false;
        }

        if (enum_value == NULL) {
          if (!allow_unknown_enum_) {
            ReportError("Unknown enumeration value of \"" + value  + "\" for "
                        "field \"" + field->name() + "\".");
            return false;
          } else {
            ReportWarning("Unknown enumeration value of \"" + value  + "\" for "
                          "field \"" + field->name() + "\".");
            return true;
          }
        }

        SET_FIELD(Enum, enum_value);
        break;
      }

      case FieldDescriptor::CPPTYPE_MESSAGE: {
        
        
        GOOGLE_LOG(FATAL) << "Reached an unintended state: CPPTYPE_MESSAGE";
        break;
      }
    }
#undef SET_FIELD
    return true;
  }

  bool SkipFieldValue() {
    if (LookingAtType(io::Tokenizer::TYPE_STRING)) {
      while (LookingAtType(io::Tokenizer::TYPE_STRING)) {
        tokenizer_.Next();
      }
      return true;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool has_minus = TryConsume("-");
    if (!LookingAtType(io::Tokenizer::TYPE_INTEGER) &&
        !LookingAtType(io::Tokenizer::TYPE_FLOAT) &&
        !LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      return false;
    }
    
    
    
    
    
    
    if (has_minus && LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      string text = tokenizer_.current().text;
      LowerString(&text);
      if (text != "inf" &&
          text != "infinity" &&
          text != "nan") {
        ReportError("Invalid float number: " + text);
        return false;
      }
    }
    tokenizer_.Next();
    return true;
  }

  
  bool LookingAt(const string& text) {
    return tokenizer_.current().text == text;
  }

  
  bool LookingAtType(io::Tokenizer::TokenType token_type) {
    return tokenizer_.current().type == token_type;
  }

  
  
  bool ConsumeIdentifier(string* identifier) {
    if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      *identifier = tokenizer_.current().text;
      tokenizer_.Next();
      return true;
    }

    
    
    if ((allow_field_number_ || allow_unknown_field_)
        && LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      *identifier = tokenizer_.current().text;
      tokenizer_.Next();
      return true;
    }

    ReportError("Expected identifier.");
    return false;
  }

  
  
  bool ConsumeString(string* text) {
    if (!LookingAtType(io::Tokenizer::TYPE_STRING)) {
      ReportError("Expected string.");
      return false;
    }

    text->clear();
    while (LookingAtType(io::Tokenizer::TYPE_STRING)) {
      io::Tokenizer::ParseStringAppend(tokenizer_.current().text, text);

      tokenizer_.Next();
    }

    return true;
  }

  
  
  bool ConsumeUnsignedInteger(uint64* value, uint64 max_value) {
    if (!LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      ReportError("Expected integer.");
      return false;
    }

    if (!io::Tokenizer::ParseInteger(tokenizer_.current().text,
                                     max_value, value)) {
      ReportError("Integer out of range.");
      return false;
    }

    tokenizer_.Next();
    return true;
  }

  
  
  
  
  
  bool ConsumeSignedInteger(int64* value, uint64 max_value) {
    bool negative = false;

    if (TryConsume("-")) {
      negative = true;
      
      
      ++max_value;
    }

    uint64 unsigned_value;

    DO(ConsumeUnsignedInteger(&unsigned_value, max_value));

    *value = static_cast<int64>(unsigned_value);

    if (negative) {
      *value = -*value;
    }

    return true;
  }

  
  
  bool ConsumeUnsignedDecimalInteger(uint64* value, uint64 max_value) {
    if (!LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      ReportError("Expected integer.");
      return false;
    }

    const string& text = tokenizer_.current().text;
    if (IsHexNumber(text) || IsOctNumber(text)) {
      ReportError("Expect a decimal number.");
      return false;
    }

    if (!io::Tokenizer::ParseInteger(text, max_value, value)) {
      ReportError("Integer out of range.");
      return false;
    }

    tokenizer_.Next();
    return true;
  }

  
  
  
  
  
  bool ConsumeDouble(double* value) {
    bool negative = false;

    if (TryConsume("-")) {
      negative = true;
    }

    
    
    if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
      
      uint64 integer_value;
      DO(ConsumeUnsignedDecimalInteger(&integer_value, kuint64max));

      *value = static_cast<double>(integer_value);
    } else if (LookingAtType(io::Tokenizer::TYPE_FLOAT)) {
      
      *value = io::Tokenizer::ParseFloat(tokenizer_.current().text);

      
      tokenizer_.Next();
    } else if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
      string text = tokenizer_.current().text;
      LowerString(&text);
      if (text == "inf" ||
          text == "infinity") {
        *value = std::numeric_limits<double>::infinity();
        tokenizer_.Next();
      } else if (text == "nan") {
        *value = std::numeric_limits<double>::quiet_NaN();
        tokenizer_.Next();
      } else {
        ReportError("Expected double.");
        return false;
      }
    } else {
      ReportError("Expected double.");
      return false;
    }

    if (negative) {
      *value = -*value;
    }

    return true;
  }

  
  
  
  bool Consume(const string& value) {
    const string& current_value = tokenizer_.current().text;

    if (current_value != value) {
      ReportError("Expected \"" + value + "\", found \"" + current_value
                  + "\".");
      return false;
    }

    tokenizer_.Next();

    return true;
  }

  
  
  bool TryConsume(const string& value) {
    if (tokenizer_.current().text == value) {
      tokenizer_.Next();
      return true;
    } else {
      return false;
    }
  }

  
  
  class ParserErrorCollector : public io::ErrorCollector {
   public:
    explicit ParserErrorCollector(TextFormat::Parser::ParserImpl* parser) :
        parser_(parser) { }

    virtual ~ParserErrorCollector() { }

    virtual void AddError(int line, int column, const string& message) {
      parser_->ReportError(line, column, message);
    }

    virtual void AddWarning(int line, int column, const string& message) {
      parser_->ReportWarning(line, column, message);
    }

   private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ParserErrorCollector);
    TextFormat::Parser::ParserImpl* parser_;
  };

  io::ErrorCollector* error_collector_;
  TextFormat::Finder* finder_;
  ParseInfoTree* parse_info_tree_;
  ParserErrorCollector tokenizer_error_collector_;
  io::Tokenizer tokenizer_;
  const Descriptor* root_message_type_;
  SingularOverwritePolicy singular_overwrite_policy_;
  const bool allow_case_insensitive_field_;
  const bool allow_unknown_field_;
  const bool allow_unknown_enum_;
  const bool allow_field_number_;
  bool had_errors_;
};

#undef DO




class TextFormat::Printer::TextGenerator {
 public:
  explicit TextGenerator(io::ZeroCopyOutputStream* output,
                         int initial_indent_level)
    : output_(output),
      buffer_(NULL),
      buffer_size_(0),
      at_start_of_line_(true),
      failed_(false),
      indent_(""),
      initial_indent_level_(initial_indent_level) {
    indent_.resize(initial_indent_level_ * 2, ' ');
  }

  ~TextGenerator() {
    
    
    if (!failed_ && buffer_size_ > 0) {
      output_->BackUp(buffer_size_);
    }
  }

  
  
  
  void Indent() {
    indent_ += "  ";
  }

  
  
  void Outdent() {
    if (indent_.empty() ||
        indent_.size() < initial_indent_level_ * 2) {
      GOOGLE_LOG(DFATAL) << " Outdent() without matching Indent().";
      return;
    }

    indent_.resize(indent_.size() - 2);
  }

  
  void Print(const string& str) {
    Print(str.data(), str.size());
  }

  
  void Print(const char* text) {
    Print(text, strlen(text));
  }

  
  void Print(const char* text, int size) {
    int pos = 0;  

    for (int i = 0; i < size; i++) {
      if (text[i] == '\n') {
        
        
        Write(text + pos, i - pos + 1);
        pos = i + 1;

        
        
        at_start_of_line_ = true;
      }
    }

    
    Write(text + pos, size - pos);
  }

  
  
  
  bool failed() const { return failed_; }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TextGenerator);

  void Write(const char* data, int size) {
    if (failed_) return;
    if (size == 0) return;

    if (at_start_of_line_) {
      
      at_start_of_line_ = false;
      Write(indent_.data(), indent_.size());
      if (failed_) return;
    }

    while (size > buffer_size_) {
      
      
      memcpy(buffer_, data, buffer_size_);
      data += buffer_size_;
      size -= buffer_size_;
      void* void_buffer;
      failed_ = !output_->Next(&void_buffer, &buffer_size_);
      if (failed_) return;
      buffer_ = reinterpret_cast<char*>(void_buffer);
    }

    
    memcpy(buffer_, data, size);
    buffer_ += size;
    buffer_size_ -= size;
  }

  io::ZeroCopyOutputStream* const output_;
  char* buffer_;
  int buffer_size_;
  bool at_start_of_line_;
  bool failed_;

  string indent_;
  int initial_indent_level_;
};



TextFormat::Finder::~Finder() {
}

TextFormat::Parser::Parser()
  : error_collector_(NULL),
    finder_(NULL),
    parse_info_tree_(NULL),
    allow_partial_(false),
    allow_case_insensitive_field_(false),
    allow_unknown_field_(false),
    allow_unknown_enum_(false),
    allow_field_number_(false),
    allow_relaxed_whitespace_(false),
    allow_singular_overwrites_(false) {
}

TextFormat::Parser::~Parser() {}

bool TextFormat::Parser::Parse(io::ZeroCopyInputStream* input,
                               Message* output) {
  output->Clear();

  ParserImpl::SingularOverwritePolicy overwrites_policy =
      allow_singular_overwrites_
      ? ParserImpl::ALLOW_SINGULAR_OVERWRITES
      : ParserImpl::FORBID_SINGULAR_OVERWRITES;

  ParserImpl parser(output->GetDescriptor(), input, error_collector_,
                    finder_, parse_info_tree_,
                    overwrites_policy,
                    allow_case_insensitive_field_, allow_unknown_field_,
                    allow_unknown_enum_, allow_field_number_,
                    allow_relaxed_whitespace_);
  return MergeUsingImpl(input, output, &parser);
}

bool TextFormat::Parser::ParseFromString(const string& input,
                                         Message* output) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  return Parse(&input_stream, output);
}

bool TextFormat::Parser::Merge(io::ZeroCopyInputStream* input,
                               Message* output) {
  ParserImpl parser(output->GetDescriptor(), input, error_collector_,
                    finder_, parse_info_tree_,
                    ParserImpl::ALLOW_SINGULAR_OVERWRITES,
                    allow_case_insensitive_field_, allow_unknown_field_,
                    allow_unknown_enum_, allow_field_number_,
                    allow_relaxed_whitespace_);
  return MergeUsingImpl(input, output, &parser);
}

bool TextFormat::Parser::MergeFromString(const string& input,
                                         Message* output) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  return Merge(&input_stream, output);
}

bool TextFormat::Parser::MergeUsingImpl(io::ZeroCopyInputStream* ,
                                        Message* output,
                                        ParserImpl* parser_impl) {
  if (!parser_impl->Parse(output)) return false;
  if (!allow_partial_ && !output->IsInitialized()) {
    vector<string> missing_fields;
    output->FindInitializationErrors(&missing_fields);
    parser_impl->ReportError(-1, 0, "Message missing required fields: " +
                                        Join(missing_fields, ", "));
    return false;
  }
  return true;
}

bool TextFormat::Parser::ParseFieldValueFromString(
    const string& input,
    const FieldDescriptor* field,
    Message* output) {
  io::ArrayInputStream input_stream(input.data(), input.size());
  ParserImpl parser(output->GetDescriptor(), &input_stream, error_collector_,
                    finder_, parse_info_tree_,
                    ParserImpl::ALLOW_SINGULAR_OVERWRITES,
                    allow_case_insensitive_field_, allow_unknown_field_,
                    allow_unknown_enum_, allow_field_number_,
                    allow_relaxed_whitespace_);
  return parser.ParseField(field, output);
}

 bool TextFormat::Parse(io::ZeroCopyInputStream* input,
                                    Message* output) {
  return Parser().Parse(input, output);
}

 bool TextFormat::Merge(io::ZeroCopyInputStream* input,
                                    Message* output) {
  return Parser().Merge(input, output);
}

 bool TextFormat::ParseFromString(const string& input,
                                              Message* output) {
  return Parser().ParseFromString(input, output);
}

 bool TextFormat::MergeFromString(const string& input,
                                              Message* output) {
  return Parser().MergeFromString(input, output);
}






TextFormat::FieldValuePrinter::FieldValuePrinter() {}
TextFormat::FieldValuePrinter::~FieldValuePrinter() {}
string TextFormat::FieldValuePrinter::PrintBool(bool val) const {
  return val ? "true" : "false";
}
string TextFormat::FieldValuePrinter::PrintInt32(int32 val) const {
  return SimpleItoa(val);
}
string TextFormat::FieldValuePrinter::PrintUInt32(uint32 val) const {
  return SimpleItoa(val);
}
string TextFormat::FieldValuePrinter::PrintInt64(int64 val) const {
  return SimpleItoa(val);
}
string TextFormat::FieldValuePrinter::PrintUInt64(uint64 val) const {
  return SimpleItoa(val);
}
string TextFormat::FieldValuePrinter::PrintFloat(float val) const {
  return SimpleFtoa(val);
}
string TextFormat::FieldValuePrinter::PrintDouble(double val) const {
  return SimpleDtoa(val);
}
string TextFormat::FieldValuePrinter::PrintString(const string& val) const {
  return StrCat("\"", CEscape(val), "\"");
}
string TextFormat::FieldValuePrinter::PrintBytes(const string& val) const {
  return PrintString(val);
}
string TextFormat::FieldValuePrinter::PrintEnum(int32 val,
                                                const string& name) const {
  return name;
}
string TextFormat::FieldValuePrinter::PrintFieldName(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field) const {
  if (field->is_extension()) {
    
    if (field->containing_type()->options().message_set_wire_format()
        && field->type() == FieldDescriptor::TYPE_MESSAGE
        && field->is_optional()
        && field->extension_scope() == field->message_type()) {
      return StrCat("[", field->message_type()->full_name(), "]");
    } else {
      return StrCat("[", field->full_name(), "]");
    }
  } else if (field->type() == FieldDescriptor::TYPE_GROUP) {
    
    return field->message_type()->name();
  } else {
    return field->name();
  }
}
string TextFormat::FieldValuePrinter::PrintMessageStart(
    const Message& message,
    int field_index,
    int field_count,
    bool single_line_mode) const {
  return single_line_mode ? " { " : " {\n";
}
string TextFormat::FieldValuePrinter::PrintMessageEnd(
    const Message& message,
    int field_index,
    int field_count,
    bool single_line_mode) const {
  return single_line_mode ? "} " : "}\n";
}

namespace {

class FieldValuePrinterUtf8Escaping : public TextFormat::FieldValuePrinter {
 public:
  virtual string PrintString(const string& val) const {
    return StrCat("\"", strings::Utf8SafeCEscape(val), "\"");
  }
  virtual string PrintBytes(const string& val) const {
    return TextFormat::FieldValuePrinter::PrintString(val);
  }
};

}  

TextFormat::Printer::Printer()
  : initial_indent_level_(0),
    single_line_mode_(false),
    use_field_number_(false),
    use_short_repeated_primitives_(false),
    hide_unknown_fields_(false),
    print_message_fields_in_index_order_(false) {
  SetUseUtf8StringEscaping(false);
}

TextFormat::Printer::~Printer() {
  STLDeleteValues(&custom_printers_);
}

void TextFormat::Printer::SetUseUtf8StringEscaping(bool as_utf8) {
  SetDefaultFieldValuePrinter(as_utf8
                              ? new FieldValuePrinterUtf8Escaping()
                              : new FieldValuePrinter());
}

void TextFormat::Printer::SetDefaultFieldValuePrinter(
    const FieldValuePrinter* printer) {
  default_field_value_printer_.reset(printer);
}

bool TextFormat::Printer::RegisterFieldValuePrinter(
    const FieldDescriptor* field,
    const FieldValuePrinter* printer) {
  return field != NULL
      && printer != NULL
      && custom_printers_.insert(make_pair(field, printer)).second;
}

bool TextFormat::Printer::PrintToString(const Message& message,
                                        string* output) const {
  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);

  return Print(message, &output_stream);
}

bool TextFormat::Printer::PrintUnknownFieldsToString(
    const UnknownFieldSet& unknown_fields,
    string* output) const {
  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);
  return PrintUnknownFields(unknown_fields, &output_stream);
}

bool TextFormat::Printer::Print(const Message& message,
                                io::ZeroCopyOutputStream* output) const {
  TextGenerator generator(output, initial_indent_level_);

  Print(message, generator);

  
  return !generator.failed();
}

bool TextFormat::Printer::PrintUnknownFields(
    const UnknownFieldSet& unknown_fields,
    io::ZeroCopyOutputStream* output) const {
  TextGenerator generator(output, initial_indent_level_);

  PrintUnknownFields(unknown_fields, generator);

  
  return !generator.failed();
}

namespace {

struct FieldIndexSorter {
  bool operator()(const FieldDescriptor* left,
                  const FieldDescriptor* right) const {
    return left->index() < right->index();
  }
};
}  

void TextFormat::Printer::Print(const Message& message,
                                TextGenerator& generator) const {
  const Reflection* reflection = message.GetReflection();
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  if (print_message_fields_in_index_order_) {
    sort(fields.begin(), fields.end(), FieldIndexSorter());
  }
  for (int i = 0; i < fields.size(); i++) {
    PrintField(message, reflection, fields[i], generator);
  }
  if (!hide_unknown_fields_) {
    PrintUnknownFields(reflection->GetUnknownFields(message), generator);
  }
}

void TextFormat::Printer::PrintFieldValueToString(
    const Message& message,
    const FieldDescriptor* field,
    int index,
    string* output) const {

  GOOGLE_DCHECK(output) << "output specified is NULL";

  output->clear();
  io::StringOutputStream output_stream(output);
  TextGenerator generator(&output_stream, initial_indent_level_);

  PrintFieldValue(message, message.GetReflection(), field, index, generator);
}

void TextFormat::Printer::PrintField(const Message& message,
                                     const Reflection* reflection,
                                     const FieldDescriptor* field,
                                     TextGenerator& generator) const {
  if (use_short_repeated_primitives_ &&
      field->is_repeated() &&
      field->cpp_type() != FieldDescriptor::CPPTYPE_STRING &&
      field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    PrintShortRepeatedField(message, reflection, field, generator);
    return;
  }

  int count = 0;

  if (field->is_repeated()) {
    count = reflection->FieldSize(message, field);
  } else if (reflection->HasField(message, field)) {
    count = 1;
  }

  for (int j = 0; j < count; ++j) {
    const int field_index = field->is_repeated() ? j : -1;

    PrintFieldName(message, reflection, field, generator);

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      const FieldValuePrinter* printer = FindWithDefault(
          custom_printers_, field, default_field_value_printer_.get());
      const Message& sub_message =
              field->is_repeated()
              ? reflection->GetRepeatedMessage(message, field, j)
              : reflection->GetMessage(message, field);
      generator.Print(
          printer->PrintMessageStart(
              sub_message, field_index, count, single_line_mode_));
      generator.Indent();
      Print(sub_message, generator);
      generator.Outdent();
      generator.Print(
          printer->PrintMessageEnd(
              sub_message, field_index, count, single_line_mode_));
    } else {
      generator.Print(": ");
      
      PrintFieldValue(message, reflection, field, field_index, generator);
      if (single_line_mode_) {
        generator.Print(" ");
      } else {
        generator.Print("\n");
      }
    }
  }
}

void TextFormat::Printer::PrintShortRepeatedField(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field,
    TextGenerator& generator) const {
  
  PrintFieldName(message, reflection, field, generator);

  int size = reflection->FieldSize(message, field);
  generator.Print(": [");
  for (int i = 0; i < size; i++) {
    if (i > 0) generator.Print(", ");
    PrintFieldValue(message, reflection, field, i, generator);
  }
  if (single_line_mode_) {
    generator.Print("] ");
  } else {
    generator.Print("]\n");
  }
}

void TextFormat::Printer::PrintFieldName(const Message& message,
                                         const Reflection* reflection,
                                         const FieldDescriptor* field,
                                         TextGenerator& generator) const {
  
  
  if (use_field_number_) {
    generator.Print(SimpleItoa(field->number()));
    return;
  }

  const FieldValuePrinter* printer = FindWithDefault(
      custom_printers_, field, default_field_value_printer_.get());
  generator.Print(printer->PrintFieldName(message, reflection, field));
}

void TextFormat::Printer::PrintFieldValue(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field,
    int index,
    TextGenerator& generator) const {
  GOOGLE_DCHECK(field->is_repeated() || (index == -1))
      << "Index must be -1 for non-repeated fields";

  const FieldValuePrinter* printer
      = FindWithDefault(custom_printers_, field,
                        default_field_value_printer_.get());

  switch (field->cpp_type()) {
#define OUTPUT_FIELD(CPPTYPE, METHOD)                                   \
    case FieldDescriptor::CPPTYPE_##CPPTYPE:                            \
      generator.Print(printer->Print##METHOD(field->is_repeated()       \
               ? reflection->GetRepeated##METHOD(message, field, index) \
               : reflection->Get##METHOD(message, field)));             \
        break

    OUTPUT_FIELD( INT32,  Int32);
    OUTPUT_FIELD( INT64,  Int64);
    OUTPUT_FIELD(UINT32, UInt32);
    OUTPUT_FIELD(UINT64, UInt64);
    OUTPUT_FIELD( FLOAT,  Float);
    OUTPUT_FIELD(DOUBLE, Double);
    OUTPUT_FIELD(  BOOL,   Bool);
#undef OUTPUT_FIELD

    case FieldDescriptor::CPPTYPE_STRING: {
      string scratch;
      const string& value = field->is_repeated()
          ? reflection->GetRepeatedStringReference(
              message, field, index, &scratch)
          : reflection->GetStringReference(message, field, &scratch);
      if (field->type() == FieldDescriptor::TYPE_STRING) {
        generator.Print(printer->PrintString(value));
      } else {
        GOOGLE_DCHECK_EQ(field->type(), FieldDescriptor::TYPE_BYTES);
        generator.Print(printer->PrintBytes(value));
      }
      break;
    }

    case FieldDescriptor::CPPTYPE_ENUM: {
      const EnumValueDescriptor *enum_val = field->is_repeated()
          ? reflection->GetRepeatedEnum(message, field, index)
          : reflection->GetEnum(message, field);
      generator.Print(printer->PrintEnum(enum_val->number(), enum_val->name()));
      break;
    }

    case FieldDescriptor::CPPTYPE_MESSAGE:
      Print(field->is_repeated()
            ? reflection->GetRepeatedMessage(message, field, index)
            : reflection->GetMessage(message, field),
            generator);
      break;
  }
}

 bool TextFormat::Print(const Message& message,
                                    io::ZeroCopyOutputStream* output) {
  return Printer().Print(message, output);
}

 bool TextFormat::PrintUnknownFields(
    const UnknownFieldSet& unknown_fields,
    io::ZeroCopyOutputStream* output) {
  return Printer().PrintUnknownFields(unknown_fields, output);
}

 bool TextFormat::PrintToString(
    const Message& message, string* output) {
  return Printer().PrintToString(message, output);
}

 bool TextFormat::PrintUnknownFieldsToString(
    const UnknownFieldSet& unknown_fields, string* output) {
  return Printer().PrintUnknownFieldsToString(unknown_fields, output);
}

 void TextFormat::PrintFieldValueToString(
    const Message& message,
    const FieldDescriptor* field,
    int index,
    string* output) {
  return Printer().PrintFieldValueToString(message, field, index, output);
}

 bool TextFormat::ParseFieldValueFromString(
    const string& input,
    const FieldDescriptor* field,
    Message* message) {
  return Parser().ParseFieldValueFromString(input, field, message);
}



template<typename IntType>
static string PaddedHex(IntType value) {
  string result;
  result.reserve(sizeof(value) * 2);
  for (int i = sizeof(value) * 2 - 1; i >= 0; i--) {
    result.push_back(int_to_hex_digit(value >> (i*4) & 0x0F));
  }
  return result;
}

void TextFormat::Printer::PrintUnknownFields(
    const UnknownFieldSet& unknown_fields, TextGenerator& generator) const {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    string field_number = SimpleItoa(field.number());

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        generator.Print(field_number);
        generator.Print(": ");
        generator.Print(SimpleItoa(field.varint()));
        if (single_line_mode_) {
          generator.Print(" ");
        } else {
          generator.Print("\n");
        }
        break;
      case UnknownField::TYPE_FIXED32: {
        generator.Print(field_number);
        generator.Print(": 0x");
        char buffer[kFastToBufferSize];
        generator.Print(FastHex32ToBuffer(field.fixed32(), buffer));
        if (single_line_mode_) {
          generator.Print(" ");
        } else {
          generator.Print("\n");
        }
        break;
      }
      case UnknownField::TYPE_FIXED64: {
        generator.Print(field_number);
        generator.Print(": 0x");
        char buffer[kFastToBufferSize];
        generator.Print(FastHex64ToBuffer(field.fixed64(), buffer));
        if (single_line_mode_) {
          generator.Print(" ");
        } else {
          generator.Print("\n");
        }
        break;
      }
      case UnknownField::TYPE_LENGTH_DELIMITED: {
        generator.Print(field_number);
        const string& value = field.length_delimited();
        UnknownFieldSet embedded_unknown_fields;
        if (!value.empty() && embedded_unknown_fields.ParseFromString(value)) {
          
          
          if (single_line_mode_) {
            generator.Print(" { ");
          } else {
            generator.Print(" {\n");
            generator.Indent();
          }
          PrintUnknownFields(embedded_unknown_fields, generator);
          if (single_line_mode_) {
            generator.Print("} ");
          } else {
            generator.Outdent();
            generator.Print("}\n");
          }
        } else {
          
          
          generator.Print(": \"");
          generator.Print(CEscape(value));
          generator.Print("\"");
          if (single_line_mode_) {
            generator.Print(" ");
          } else {
            generator.Print("\n");
          }
        }
        break;
      }
      case UnknownField::TYPE_GROUP:
        generator.Print(field_number);
        if (single_line_mode_) {
          generator.Print(" { ");
        } else {
          generator.Print(" {\n");
          generator.Indent();
        }
        PrintUnknownFields(field.group(), generator);
        if (single_line_mode_) {
          generator.Print("} ");
        } else {
          generator.Outdent();
          generator.Print("}\n");
        }
        break;
    }
  }
}

}  
}  
