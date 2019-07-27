




































#ifndef GOOGLE_PROTOBUF_TEXT_FORMAT_H__
#define GOOGLE_PROTOBUF_TEXT_FORMAT_H__

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace google {
namespace protobuf {

namespace io {
  class ErrorCollector;      
}






class LIBPROTOBUF_EXPORT TextFormat {
 public:
  
  
  static bool Print(const Message& message, io::ZeroCopyOutputStream* output);

  
  
  
  static bool PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                                 io::ZeroCopyOutputStream* output);

  
  static bool PrintToString(const Message& message, string* output);

  
  static bool PrintUnknownFieldsToString(const UnknownFieldSet& unknown_fields,
                                         string* output);

  
  
  
  
  static void PrintFieldValueToString(const Message& message,
                                      const FieldDescriptor* field,
                                      int index,
                                      string* output);

  
  
  
  
  
  class LIBPROTOBUF_EXPORT FieldValuePrinter {
   public:
    FieldValuePrinter();
    virtual ~FieldValuePrinter();
    virtual string PrintBool(bool val) const;
    virtual string PrintInt32(int32 val) const;
    virtual string PrintUInt32(uint32 val) const;
    virtual string PrintInt64(int64 val) const;
    virtual string PrintUInt64(uint64 val) const;
    virtual string PrintFloat(float val) const;
    virtual string PrintDouble(double val) const;
    virtual string PrintString(const string& val) const;
    virtual string PrintBytes(const string& val) const;
    virtual string PrintEnum(int32 val, const string& name) const;
    virtual string PrintFieldName(const Message& message,
                                  const Reflection* reflection,
                                  const FieldDescriptor* field) const;
    virtual string PrintMessageStart(const Message& message,
                                     int field_index,
                                     int field_count,
                                     bool single_line_mode) const;
    virtual string PrintMessageEnd(const Message& message,
                                   int field_index,
                                   int field_count,
                                   bool single_line_mode) const;

   private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldValuePrinter);
  };

  
  
  class LIBPROTOBUF_EXPORT Printer {
   public:
    Printer();
    ~Printer();

    
    bool Print(const Message& message, io::ZeroCopyOutputStream* output) const;
    
    bool PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                            io::ZeroCopyOutputStream* output) const;
    
    bool PrintToString(const Message& message, string* output) const;
    
    bool PrintUnknownFieldsToString(const UnknownFieldSet& unknown_fields,
                                    string* output) const;
    
    void PrintFieldValueToString(const Message& message,
                                 const FieldDescriptor* field,
                                 int index,
                                 string* output) const;

    
    
    void SetInitialIndentLevel(int indent_level) {
      initial_indent_level_ = indent_level;
    }

    
    
    void SetSingleLineMode(bool single_line_mode) {
      single_line_mode_ = single_line_mode;
    }

    bool IsInSingleLineMode() {
      return single_line_mode_;
    }

    
    void SetUseFieldNumber(bool use_field_number) {
      use_field_number_ = use_field_number;
    }

    
    
    
    
    
    void SetUseShortRepeatedPrimitives(bool use_short_repeated_primitives) {
      use_short_repeated_primitives_ = use_short_repeated_primitives;
    }

    
    
    
    
    void SetUseUtf8StringEscaping(bool as_utf8);

    
    
    
    void SetDefaultFieldValuePrinter(const FieldValuePrinter* printer);

    
    
    
    
    
    
    void SetHideUnknownFields(bool hide) {
      hide_unknown_fields_ = hide;
    }

    
    
    
    void SetPrintMessageFieldsInIndexOrder(
        bool print_message_fields_in_index_order) {
      print_message_fields_in_index_order_ =
          print_message_fields_in_index_order;
    }

    
    
    
    
    
    bool RegisterFieldValuePrinter(const FieldDescriptor* field,
                                   const FieldValuePrinter* printer);

   private:
    
    
    class TextGenerator;

    
    
    void Print(const Message& message,
               TextGenerator& generator) const;

    
    void PrintField(const Message& message,
                    const Reflection* reflection,
                    const FieldDescriptor* field,
                    TextGenerator& generator) const;

    
    void PrintShortRepeatedField(const Message& message,
                                 const Reflection* reflection,
                                 const FieldDescriptor* field,
                                 TextGenerator& generator) const;

    
    
    void PrintFieldName(const Message& message,
                        const Reflection* reflection,
                        const FieldDescriptor* field,
                        TextGenerator& generator) const;

    
    
    void PrintFieldValue(const Message& message,
                         const Reflection* reflection,
                         const FieldDescriptor* field,
                         int index,
                         TextGenerator& generator) const;

    
    
    
    void PrintUnknownFields(const UnknownFieldSet& unknown_fields,
                            TextGenerator& generator) const;

    int initial_indent_level_;

    bool single_line_mode_;

    bool use_field_number_;

    bool use_short_repeated_primitives_;

    bool hide_unknown_fields_;

    bool print_message_fields_in_index_order_;

    scoped_ptr<const FieldValuePrinter> default_field_value_printer_;
    typedef map<const FieldDescriptor*,
                const FieldValuePrinter*> CustomPrinterMap;
    CustomPrinterMap custom_printers_;
  };

  
  
  
  static bool Parse(io::ZeroCopyInputStream* input, Message* output);
  
  static bool ParseFromString(const string& input, Message* output);

  
  
  static bool Merge(io::ZeroCopyInputStream* input, Message* output);
  
  static bool MergeFromString(const string& input, Message* output);

  
  
  
  static bool ParseFieldValueFromString(const string& input,
                                        const FieldDescriptor* field,
                                        Message* message);

  
  
  
  class LIBPROTOBUF_EXPORT Finder {
   public:
    virtual ~Finder();

    
    
    virtual const FieldDescriptor* FindExtension(
        Message* message,
        const string& name) const = 0;
  };

  
  struct ParseLocation {
    int line;
    int column;

    ParseLocation() : line(-1), column(-1) {}
    ParseLocation(int line_param, int column_param)
        : line(line_param), column(column_param) {}
  };

  
  
  class LIBPROTOBUF_EXPORT ParseInfoTree {
   public:
    ParseInfoTree();
    ~ParseInfoTree();

    
    
    
    ParseLocation GetLocation(const FieldDescriptor* field, int index) const;

    
    
    
    ParseInfoTree* GetTreeForNested(const FieldDescriptor* field,
                                    int index) const;

   private:
    
    friend class TextFormat;

    
    void RecordLocation(const FieldDescriptor* field, ParseLocation location);

    
    ParseInfoTree* CreateNested(const FieldDescriptor* field);

    
    typedef map<const FieldDescriptor*, vector<ParseLocation> > LocationMap;

    
    
    typedef map<const FieldDescriptor*, vector<ParseInfoTree*> > NestedMap;

    LocationMap locations_;
    NestedMap nested_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ParseInfoTree);
  };

  
  class LIBPROTOBUF_EXPORT Parser {
   public:
    Parser();
    ~Parser();

    
    bool Parse(io::ZeroCopyInputStream* input, Message* output);
    
    bool ParseFromString(const string& input, Message* output);
    
    bool Merge(io::ZeroCopyInputStream* input, Message* output);
    
    bool MergeFromString(const string& input, Message* output);

    
    
    void RecordErrorsTo(io::ErrorCollector* error_collector) {
      error_collector_ = error_collector;
    }

    
    
    
    void SetFinder(Finder* finder) {
      finder_ = finder;
    }

    
    
    void WriteLocationsTo(ParseInfoTree* tree) {
      parse_info_tree_ = tree;
    }

    
    
    void AllowPartialMessage(bool allow) {
      allow_partial_ = allow;
    }

    
    
    
    
    void AllowCaseInsensitiveField(bool allow) {
      allow_case_insensitive_field_ = allow;
    }

    
    bool ParseFieldValueFromString(const string& input,
                                   const FieldDescriptor* field,
                                   Message* output);


    void AllowFieldNumber(bool allow) {
      allow_field_number_ = allow;
    }

   private:
    
    
    class ParserImpl;

    
    
    bool MergeUsingImpl(io::ZeroCopyInputStream* input,
                        Message* output,
                        ParserImpl* parser_impl);

    io::ErrorCollector* error_collector_;
    Finder* finder_;
    ParseInfoTree* parse_info_tree_;
    bool allow_partial_;
    bool allow_case_insensitive_field_;
    bool allow_unknown_field_;
    bool allow_unknown_enum_;
    bool allow_field_number_;
    bool allow_relaxed_whitespace_;
    bool allow_singular_overwrites_;
  };


 private:
  
  
  
  
  static inline void RecordLocation(ParseInfoTree* info_tree,
                                    const FieldDescriptor* field,
                                    ParseLocation location);
  static inline ParseInfoTree* CreateNested(ParseInfoTree* info_tree,
                                            const FieldDescriptor* field);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TextFormat);
};

inline void TextFormat::RecordLocation(ParseInfoTree* info_tree,
                                       const FieldDescriptor* field,
                                       ParseLocation location) {
  info_tree->RecordLocation(field, location);
}


inline TextFormat::ParseInfoTree* TextFormat::CreateNested(
    ParseInfoTree* info_tree, const FieldDescriptor* field) {
  return info_tree->CreateNested(field);
}

}  

}  
#endif  
