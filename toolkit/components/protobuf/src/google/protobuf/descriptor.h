




















































#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_H__
#define GOOGLE_PROTOBUF_DESCRIPTOR_H__

#include <set>
#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>


namespace google {
namespace protobuf {


class Descriptor;
class FieldDescriptor;
class OneofDescriptor;
class EnumDescriptor;
class EnumValueDescriptor;
class ServiceDescriptor;
class MethodDescriptor;
class FileDescriptor;
class DescriptorDatabase;
class DescriptorPool;


class DescriptorProto;
class FieldDescriptorProto;
class OneofDescriptorProto;
class EnumDescriptorProto;
class EnumValueDescriptorProto;
class ServiceDescriptorProto;
class MethodDescriptorProto;
class FileDescriptorProto;
class MessageOptions;
class FieldOptions;
class EnumOptions;
class EnumValueOptions;
class ServiceOptions;
class MethodOptions;
class FileOptions;
class UninterpretedOption;
class SourceCodeInfo;


class Message;


class DescriptorBuilder;
class FileDescriptorTables;


class UnknownField;


struct SourceLocation {
  int start_line;
  int end_line;
  int start_column;
  int end_column;

  
  
  
  string leading_comments;
  string trailing_comments;
};






class LIBPROTOBUF_EXPORT Descriptor {
 public:
  
  const string& name() const;

  
  
  
  
  
  const string& full_name() const;

  
  
  int index() const;

  
  const FileDescriptor* file() const;

  
  
  const Descriptor* containing_type() const;

  
  
  
  
  
  const MessageOptions& options() const;

  
  
  
  void CopyTo(DescriptorProto* proto) const;

  
  
  string DebugString() const;

  
  
  
  bool is_placeholder() const;

  

  
  int field_count() const;
  
  
  const FieldDescriptor* field(int index) const;

  
  
  const FieldDescriptor* FindFieldByNumber(int number) const;
  
  const FieldDescriptor* FindFieldByName(const string& name) const;

  
  
  
  const FieldDescriptor* FindFieldByLowercaseName(
      const string& lowercase_name) const;

  
  
  
  
  const FieldDescriptor* FindFieldByCamelcaseName(
      const string& camelcase_name) const;

  
  int oneof_decl_count() const;
  
  
  const OneofDescriptor* oneof_decl(int index) const;

  
  const OneofDescriptor* FindOneofByName(const string& name) const;

  

  
  int nested_type_count() const;
  
  
  const Descriptor* nested_type(int index) const;

  
  
  const Descriptor* FindNestedTypeByName(const string& name) const;

  

  
  int enum_type_count() const;
  
  
  const EnumDescriptor* enum_type(int index) const;

  
  const EnumDescriptor* FindEnumTypeByName(const string& name) const;

  
  
  const EnumValueDescriptor* FindEnumValueByName(const string& name) const;

  

  
  
  struct ExtensionRange {
    int start;  
    int end;    
  };

  
  int extension_range_count() const;
  
  
  
  const ExtensionRange* extension_range(int index) const;

  
  bool IsExtensionNumber(int number) const;

  
  const ExtensionRange* FindExtensionRangeContainingNumber(int number) const;

  
  
  int extension_count() const;
  
  
  const FieldDescriptor* extension(int index) const;

  
  
  const FieldDescriptor* FindExtensionByName(const string& name) const;

  
  
  const FieldDescriptor* FindExtensionByLowercaseName(const string& name) const;

  
  
  const FieldDescriptor* FindExtensionByCamelcaseName(const string& name) const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef MessageOptions OptionsType;

  
  
  void DebugString(int depth, string *contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const FileDescriptor* file_;
  const Descriptor* containing_type_;
  const MessageOptions* options_;

  
  bool is_placeholder_;
  
  bool is_unqualified_placeholder_;

  int field_count_;
  FieldDescriptor* fields_;
  int oneof_decl_count_;
  OneofDescriptor* oneof_decls_;
  int nested_type_count_;
  Descriptor* nested_types_;
  int enum_type_count_;
  EnumDescriptor* enum_types_;
  int extension_range_count_;
  ExtensionRange* extension_ranges_;
  int extension_count_;
  FieldDescriptor* extensions_;
  
  
  

  
  Descriptor() {}
  friend class DescriptorBuilder;
  friend class EnumDescriptor;
  friend class FieldDescriptor;
  friend class OneofDescriptor;
  friend class MethodDescriptor;
  friend class FileDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Descriptor);
};













class LIBPROTOBUF_EXPORT FieldDescriptor {
 public:
  
  
  enum Type {
    TYPE_DOUBLE         = 1,   
    TYPE_FLOAT          = 2,   
    TYPE_INT64          = 3,   
                               
                               
    TYPE_UINT64         = 4,   
    TYPE_INT32          = 5,   
                               
                               
    TYPE_FIXED64        = 6,   
    TYPE_FIXED32        = 7,   
    TYPE_BOOL           = 8,   
    TYPE_STRING         = 9,   
    TYPE_GROUP          = 10,  
    TYPE_MESSAGE        = 11,  

    TYPE_BYTES          = 12,  
    TYPE_UINT32         = 13,  
    TYPE_ENUM           = 14,  
    TYPE_SFIXED32       = 15,  
    TYPE_SFIXED64       = 16,  
    TYPE_SINT32         = 17,  
    TYPE_SINT64         = 18,  

    MAX_TYPE            = 18,  
                               
  };

  
  
  
  enum CppType {
    CPPTYPE_INT32       = 1,     
    CPPTYPE_INT64       = 2,     
    CPPTYPE_UINT32      = 3,     
    CPPTYPE_UINT64      = 4,     
    CPPTYPE_DOUBLE      = 5,     
    CPPTYPE_FLOAT       = 6,     
    CPPTYPE_BOOL        = 7,     
    CPPTYPE_ENUM        = 8,     
    CPPTYPE_STRING      = 9,     
    CPPTYPE_MESSAGE     = 10,    

    MAX_CPPTYPE         = 10,    
                                 
  };

  
  
  enum Label {
    LABEL_OPTIONAL      = 1,    
    LABEL_REQUIRED      = 2,    
    LABEL_REPEATED      = 3,    

    MAX_LABEL           = 3,    
                                
  };

  
  static const int kMaxNumber = (1 << 29) - 1;

  
  
  static const int kFirstReservedNumber = 19000;
  
  
  static const int kLastReservedNumber  = 19999;

  const string& name() const;        
  const string& full_name() const;   
  const FileDescriptor* file() const;
  bool is_extension() const;         
  int number() const;                

  
  
  
  
  
  
  const string& lowercase_name() const;

  
  
  
  
  
  
  
  
  
  const string& camelcase_name() const;

  Type type() const;                  
  const char* type_name() const;      
  CppType cpp_type() const;           
  const char* cpp_type_name() const;  
  Label label() const;                

  bool is_required() const;      
  bool is_optional() const;      
  bool is_repeated() const;      
  bool is_packable() const;      
                                 
  bool is_packed() const;        
                                 

  
  
  int index() const;

  
  bool has_default_value() const;

  
  
  int32 default_value_int32() const;
  
  
  int64 default_value_int64() const;
  
  
  uint32 default_value_uint32() const;
  
  
  uint64 default_value_uint64() const;
  
  
  float default_value_float() const;
  
  
  double default_value_double() const;
  
  
  bool default_value_bool() const;
  
  
  
  
  const EnumValueDescriptor* default_value_enum() const;
  
  
  const string& default_value_string() const;

  
  
  const Descriptor* containing_type() const;

  
  
  const OneofDescriptor* containing_oneof() const;

  
  int index_in_oneof() const;

  
  
  
  
  
  const Descriptor* extension_scope() const;

  
  
  const Descriptor* message_type() const;
  
  
  const EnumDescriptor* enum_type() const;

  
  
  
  
  const FieldDescriptor* experimental_map_key() const;

  
  
  
  
  
  
  const FieldOptions& options() const;

  
  void CopyTo(FieldDescriptorProto* proto) const;

  
  string DebugString() const;

  
  static CppType TypeToCppType(Type type);

  
  static const char* TypeName(Type type);

  
  static const char* CppTypeName(CppType cpp_type);

  
  static inline bool IsTypePackable(Type field_type);

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef FieldOptions OptionsType;

  
  enum PrintLabelFlag { PRINT_LABEL, OMIT_LABEL };
  void DebugString(int depth, PrintLabelFlag print_label_flag,
                   string* contents) const;

  
  
  
  string DefaultValueAsString(bool quote_string_type) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const string* lowercase_name_;
  const string* camelcase_name_;
  const FileDescriptor* file_;
  int number_;
  Type type_;
  Label label_;
  bool is_extension_;
  int index_in_oneof_;
  const Descriptor* containing_type_;
  const OneofDescriptor* containing_oneof_;
  const Descriptor* extension_scope_;
  const Descriptor* message_type_;
  const EnumDescriptor* enum_type_;
  const FieldDescriptor* experimental_map_key_;
  const FieldOptions* options_;
  
  
  

  bool has_default_value_;
  union {
    int32  default_value_int32_;
    int64  default_value_int64_;
    uint32 default_value_uint32_;
    uint64 default_value_uint64_;
    float  default_value_float_;
    double default_value_double_;
    bool   default_value_bool_;

    const EnumValueDescriptor* default_value_enum_;
    const string* default_value_string_;
  };

  static const CppType kTypeToCppTypeMap[MAX_TYPE + 1];

  static const char * const kTypeToName[MAX_TYPE + 1];

  static const char * const kCppTypeToName[MAX_CPPTYPE + 1];

  static const char * const kLabelToName[MAX_LABEL + 1];

  
  FieldDescriptor() {}
  friend class DescriptorBuilder;
  friend class FileDescriptor;
  friend class Descriptor;
  friend class OneofDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldDescriptor);
};


class LIBPROTOBUF_EXPORT OneofDescriptor {
 public:
  const string& name() const;       
  const string& full_name() const;  

  
  int index() const;

  
  const Descriptor* containing_type() const;

  
  int field_count() const;
  
  
  const FieldDescriptor* field(int index) const;

  
  void CopyTo(OneofDescriptorProto* proto) const;

  
  string DebugString() const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  
  void DebugString(int depth, string* contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const Descriptor* containing_type_;
  bool is_extendable_;
  int field_count_;
  const FieldDescriptor** fields_;
  
  
  

  
  OneofDescriptor() {}
  friend class DescriptorBuilder;
  friend class Descriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(OneofDescriptor);
};




class LIBPROTOBUF_EXPORT EnumDescriptor {
 public:
  
  const string& name() const;

  
  const string& full_name() const;

  
  int index() const;

  
  const FileDescriptor* file() const;

  
  
  int value_count() const;
  
  
  const EnumValueDescriptor* value(int index) const;

  
  const EnumValueDescriptor* FindValueByName(const string& name) const;
  
  
  const EnumValueDescriptor* FindValueByNumber(int number) const;

  
  
  const Descriptor* containing_type() const;

  
  
  
  
  const EnumOptions& options() const;

  
  void CopyTo(EnumDescriptorProto* proto) const;

  
  string DebugString() const;

  
  
  
  bool is_placeholder() const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef EnumOptions OptionsType;

  
  void DebugString(int depth, string *contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const FileDescriptor* file_;
  const Descriptor* containing_type_;
  const EnumOptions* options_;

  
  bool is_placeholder_;
  
  bool is_unqualified_placeholder_;

  int value_count_;
  EnumValueDescriptor* values_;
  
  
  

  
  EnumDescriptor() {}
  friend class DescriptorBuilder;
  friend class Descriptor;
  friend class FieldDescriptor;
  friend class EnumValueDescriptor;
  friend class FileDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumDescriptor);
};






class LIBPROTOBUF_EXPORT EnumValueDescriptor {
 public:
  const string& name() const;  
  int index() const;           
  int number() const;          

  
  
  
  
  
  const string& full_name() const;

  
  const EnumDescriptor* type() const;

  
  
  
  
  
  const EnumValueOptions& options() const;

  
  void CopyTo(EnumValueDescriptorProto* proto) const;

  
  string DebugString() const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef EnumValueOptions OptionsType;

  
  void DebugString(int depth, string *contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  int number_;
  const EnumDescriptor* type_;
  const EnumValueOptions* options_;
  
  
  

  
  EnumValueDescriptor() {}
  friend class DescriptorBuilder;
  friend class EnumDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumValueDescriptor);
};





class LIBPROTOBUF_EXPORT ServiceDescriptor {
 public:
  
  const string& name() const;
  
  const string& full_name() const;
  
  int index() const;

  
  const FileDescriptor* file() const;

  
  
  
  
  
  const ServiceOptions& options() const;

  
  int method_count() const;
  
  
  const MethodDescriptor* method(int index) const;

  
  const MethodDescriptor* FindMethodByName(const string& name) const;
  
  void CopyTo(ServiceDescriptorProto* proto) const;

  
  string DebugString() const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef ServiceOptions OptionsType;

  
  void DebugString(string *contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const FileDescriptor* file_;
  const ServiceOptions* options_;
  int method_count_;
  MethodDescriptor* methods_;
  
  
  

  
  ServiceDescriptor() {}
  friend class DescriptorBuilder;
  friend class FileDescriptor;
  friend class MethodDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceDescriptor);
};





class LIBPROTOBUF_EXPORT MethodDescriptor {
 public:
  
  const string& name() const;
  
  const string& full_name() const;
  
  int index() const;

  
  const ServiceDescriptor* service() const;

  
  const Descriptor* input_type() const;
  
  const Descriptor* output_type() const;

  
  
  
  
  
  const MethodOptions& options() const;

  
  void CopyTo(MethodDescriptorProto* proto) const;

  
  string DebugString() const;

  

  
  
  
  bool GetSourceLocation(SourceLocation* out_location) const;

 private:
  typedef MethodOptions OptionsType;

  
  void DebugString(int depth, string *contents) const;

  
  
  void GetLocationPath(vector<int>* output) const;

  const string* name_;
  const string* full_name_;
  const ServiceDescriptor* service_;
  const Descriptor* input_type_;
  const Descriptor* output_type_;
  const MethodOptions* options_;
  
  
  

  
  MethodDescriptor() {}
  friend class DescriptorBuilder;
  friend class ServiceDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MethodDescriptor);
};





class LIBPROTOBUF_EXPORT FileDescriptor {
 public:
  
  
  const string& name() const;

  
  const string& package() const;

  
  
  const DescriptorPool* pool() const;

  
  int dependency_count() const;
  
  
  const FileDescriptor* dependency(int index) const;

  
  
  int public_dependency_count() const;
  
  
  
  const FileDescriptor* public_dependency(int index) const;

  
  
  int weak_dependency_count() const;
  
  
  
  const FileDescriptor* weak_dependency(int index) const;

  
  
  int message_type_count() const;
  
  
  const Descriptor* message_type(int index) const;

  
  
  int enum_type_count() const;
  
  
  const EnumDescriptor* enum_type(int index) const;

  
  int service_count() const;
  
  
  const ServiceDescriptor* service(int index) const;

  
  
  int extension_count() const;
  
  
  const FieldDescriptor* extension(int index) const;

  
  
  
  
  
  const FileOptions& options() const;

  
  const Descriptor* FindMessageTypeByName(const string& name) const;
  
  const EnumDescriptor* FindEnumTypeByName(const string& name) const;
  
  
  const EnumValueDescriptor* FindEnumValueByName(const string& name) const;
  
  const ServiceDescriptor* FindServiceByName(const string& name) const;
  
  const FieldDescriptor* FindExtensionByName(const string& name) const;
  
  
  const FieldDescriptor* FindExtensionByLowercaseName(const string& name) const;
  
  
  const FieldDescriptor* FindExtensionByCamelcaseName(const string& name) const;

  
  
  
  
  void CopyTo(FileDescriptorProto* proto) const;
  
  
  void CopySourceCodeInfoTo(FileDescriptorProto* proto) const;

  
  string DebugString() const;

  
  
  
  bool is_placeholder() const;

 private:
  

  
  
  
  
  
  bool GetSourceLocation(const vector<int>& path,
                         SourceLocation* out_location) const;

  typedef FileOptions OptionsType;

  const string* name_;
  const string* package_;
  const DescriptorPool* pool_;
  int dependency_count_;
  const FileDescriptor** dependencies_;
  int public_dependency_count_;
  int* public_dependencies_;
  int weak_dependency_count_;
  int* weak_dependencies_;
  int message_type_count_;
  Descriptor* message_types_;
  int enum_type_count_;
  EnumDescriptor* enum_types_;
  int service_count_;
  ServiceDescriptor* services_;
  int extension_count_;
  bool is_placeholder_;
  FieldDescriptor* extensions_;
  const FileOptions* options_;

  const FileDescriptorTables* tables_;
  const SourceCodeInfo* source_code_info_;
  
  
  

  FileDescriptor() {}
  friend class DescriptorBuilder;
  friend class Descriptor;
  friend class FieldDescriptor;
  friend class OneofDescriptor;
  friend class EnumDescriptor;
  friend class EnumValueDescriptor;
  friend class MethodDescriptor;
  friend class ServiceDescriptor;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileDescriptor);
};



























class LIBPROTOBUF_EXPORT DescriptorPool {
 public:
  
  DescriptorPool();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  class ErrorCollector;
  explicit DescriptorPool(DescriptorDatabase* fallback_database,
                          ErrorCollector* error_collector = NULL);

  ~DescriptorPool();

  
  
  
  static const DescriptorPool* generated_pool();

  
  
  const FileDescriptor* FindFileByName(const string& name) const;

  
  
  
  
  const FileDescriptor* FindFileContainingSymbol(
      const string& symbol_name) const;

  
  
  
  

  const Descriptor* FindMessageTypeByName(const string& name) const;
  const FieldDescriptor* FindFieldByName(const string& name) const;
  const FieldDescriptor* FindExtensionByName(const string& name) const;
  const OneofDescriptor* FindOneofByName(const string& name) const;
  const EnumDescriptor* FindEnumTypeByName(const string& name) const;
  const EnumValueDescriptor* FindEnumValueByName(const string& name) const;
  const ServiceDescriptor* FindServiceByName(const string& name) const;
  const MethodDescriptor* FindMethodByName(const string& name) const;

  
  
  const FieldDescriptor* FindExtensionByNumber(const Descriptor* extendee,
                                               int number) const;

  
  
  
  
  
  void FindAllExtensions(const Descriptor* extendee,
                         vector<const FieldDescriptor*>* out) const;

  

  
  
  
  class LIBPROTOBUF_EXPORT ErrorCollector {
   public:
    inline ErrorCollector() {}
    virtual ~ErrorCollector();

    
    
    
    enum ErrorLocation {
      NAME,              
      NUMBER,            
      TYPE,              
      EXTENDEE,          
      DEFAULT_VALUE,     
      INPUT_TYPE,        
      OUTPUT_TYPE,       
      OPTION_NAME,       
      OPTION_VALUE,      
      OTHER              
    };

    
    
    virtual void AddError(
      const string& filename,      
      const string& element_name,  
      const Message* descriptor,   
      ErrorLocation location,      
      const string& message        
      ) = 0;

    
    
    virtual void AddWarning(
      const string& filename,      
      const string& element_name,  
      const Message* descriptor,   
      ErrorLocation location,      
      const string& message        
      ) {}

   private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ErrorCollector);
  };

  
  
  
  
  
  const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

  
  const FileDescriptor* BuildFileCollectingErrors(
    const FileDescriptorProto& proto,
    ErrorCollector* error_collector);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void AllowUnknownDependencies() { allow_unknown_ = true; }

  
  
  
  
  void EnforceWeakDependencies(bool enforce) { enforce_weak_ = enforce; }

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  explicit DescriptorPool(const DescriptorPool* underlay);

  
  
  
  static void InternalAddGeneratedFile(
      const void* encoded_file_descriptor, int size);


  
  
  
  
  static DescriptorPool* internal_generated_pool();

  
  
  
  void InternalDontEnforceDependencies();

  
  void internal_set_underlay(const DescriptorPool* underlay) {
    underlay_ = underlay;
  }

  
  
  
  bool InternalIsFileLoaded(const string& filename) const;


  
  
  void AddUnusedImportTrackFile(const string& file_name);
  void ClearUnusedImportTrackFiles();

 private:
  friend class Descriptor;
  friend class FieldDescriptor;
  friend class EnumDescriptor;
  friend class ServiceDescriptor;
  friend class FileDescriptor;
  friend class DescriptorBuilder;

  
  
  
  bool IsSubSymbolOfBuiltType(const string& name) const;

  
  
  
  
  bool TryFindFileInFallbackDatabase(const string& name) const;
  bool TryFindSymbolInFallbackDatabase(const string& name) const;
  bool TryFindExtensionInFallbackDatabase(const Descriptor* containing_type,
                                          int field_number) const;

  
  
  
  const FileDescriptor* BuildFileFromDatabase(
    const FileDescriptorProto& proto) const;

  
  
  Mutex* mutex_;

  
  DescriptorDatabase* fallback_database_;
  ErrorCollector* default_error_collector_;
  const DescriptorPool* underlay_;

  
  
  class Tables;
  scoped_ptr<Tables> tables_;

  bool enforce_dependencies_;
  bool allow_unknown_;
  bool enforce_weak_;
  std::set<string> unused_import_track_files_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DescriptorPool);
};




#define PROTOBUF_DEFINE_ACCESSOR(CLASS, FIELD, TYPE) \
  inline TYPE CLASS::FIELD() const { return FIELD##_; }


#define PROTOBUF_DEFINE_STRING_ACCESSOR(CLASS, FIELD) \
  inline const string& CLASS::FIELD() const { return *FIELD##_; }


#define PROTOBUF_DEFINE_ARRAY_ACCESSOR(CLASS, FIELD, TYPE) \
  inline TYPE CLASS::FIELD(int index) const { return FIELD##s_ + index; }

#define PROTOBUF_DEFINE_OPTIONS_ACCESSOR(CLASS, TYPE) \
  inline const TYPE& CLASS::options() const { return *options_; }

PROTOBUF_DEFINE_STRING_ACCESSOR(Descriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(Descriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, file, const FileDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, containing_type, const Descriptor*)

PROTOBUF_DEFINE_ACCESSOR(Descriptor, field_count, int)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, oneof_decl_count, int)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, nested_type_count, int)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, enum_type_count, int)

PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, field, const FieldDescriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, oneof_decl, const OneofDescriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, nested_type, const Descriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, enum_type, const EnumDescriptor*)

PROTOBUF_DEFINE_ACCESSOR(Descriptor, extension_range_count, int)
PROTOBUF_DEFINE_ACCESSOR(Descriptor, extension_count, int)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, extension_range,
                               const Descriptor::ExtensionRange*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(Descriptor, extension,
                               const FieldDescriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(Descriptor, MessageOptions);
PROTOBUF_DEFINE_ACCESSOR(Descriptor, is_placeholder, bool)

PROTOBUF_DEFINE_STRING_ACCESSOR(FieldDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(FieldDescriptor, full_name)
PROTOBUF_DEFINE_STRING_ACCESSOR(FieldDescriptor, lowercase_name)
PROTOBUF_DEFINE_STRING_ACCESSOR(FieldDescriptor, camelcase_name)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, file, const FileDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, number, int)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, is_extension, bool)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, type, FieldDescriptor::Type)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, label, FieldDescriptor::Label)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, containing_type, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, containing_oneof,
                         const OneofDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, index_in_oneof, int)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, extension_scope, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, message_type, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, enum_type, const EnumDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, experimental_map_key,
                         const FieldDescriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(FieldDescriptor, FieldOptions)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, has_default_value, bool)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_int32 , int32 )
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_int64 , int64 )
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_uint32, uint32)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_uint64, uint64)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_float , float )
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_double, double)
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_bool  , bool  )
PROTOBUF_DEFINE_ACCESSOR(FieldDescriptor, default_value_enum,
                         const EnumValueDescriptor*)
PROTOBUF_DEFINE_STRING_ACCESSOR(FieldDescriptor, default_value_string)

PROTOBUF_DEFINE_STRING_ACCESSOR(OneofDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(OneofDescriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(OneofDescriptor, containing_type, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(OneofDescriptor, field_count, int)

PROTOBUF_DEFINE_STRING_ACCESSOR(EnumDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(EnumDescriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(EnumDescriptor, file, const FileDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(EnumDescriptor, containing_type, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(EnumDescriptor, value_count, int)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(EnumDescriptor, value,
                               const EnumValueDescriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(EnumDescriptor, EnumOptions);
PROTOBUF_DEFINE_ACCESSOR(EnumDescriptor, is_placeholder, bool)

PROTOBUF_DEFINE_STRING_ACCESSOR(EnumValueDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(EnumValueDescriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(EnumValueDescriptor, number, int)
PROTOBUF_DEFINE_ACCESSOR(EnumValueDescriptor, type, const EnumDescriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(EnumValueDescriptor, EnumValueOptions)

PROTOBUF_DEFINE_STRING_ACCESSOR(ServiceDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(ServiceDescriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(ServiceDescriptor, file, const FileDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(ServiceDescriptor, method_count, int)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(ServiceDescriptor, method,
                               const MethodDescriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(ServiceDescriptor, ServiceOptions);

PROTOBUF_DEFINE_STRING_ACCESSOR(MethodDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(MethodDescriptor, full_name)
PROTOBUF_DEFINE_ACCESSOR(MethodDescriptor, service, const ServiceDescriptor*)
PROTOBUF_DEFINE_ACCESSOR(MethodDescriptor, input_type, const Descriptor*)
PROTOBUF_DEFINE_ACCESSOR(MethodDescriptor, output_type, const Descriptor*)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(MethodDescriptor, MethodOptions);
PROTOBUF_DEFINE_STRING_ACCESSOR(FileDescriptor, name)
PROTOBUF_DEFINE_STRING_ACCESSOR(FileDescriptor, package)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, pool, const DescriptorPool*)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, dependency_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, public_dependency_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, weak_dependency_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, message_type_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, enum_type_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, service_count, int)
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, extension_count, int)
PROTOBUF_DEFINE_OPTIONS_ACCESSOR(FileDescriptor, FileOptions);
PROTOBUF_DEFINE_ACCESSOR(FileDescriptor, is_placeholder, bool)

PROTOBUF_DEFINE_ARRAY_ACCESSOR(FileDescriptor, message_type, const Descriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(FileDescriptor, enum_type, const EnumDescriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(FileDescriptor, service,
                               const ServiceDescriptor*)
PROTOBUF_DEFINE_ARRAY_ACCESSOR(FileDescriptor, extension,
                               const FieldDescriptor*)

#undef PROTOBUF_DEFINE_ACCESSOR
#undef PROTOBUF_DEFINE_STRING_ACCESSOR
#undef PROTOBUF_DEFINE_ARRAY_ACCESSOR



inline bool Descriptor::IsExtensionNumber(int number) const {
  return FindExtensionRangeContainingNumber(number) != NULL;
}

inline bool FieldDescriptor::is_required() const {
  return label() == LABEL_REQUIRED;
}

inline bool FieldDescriptor::is_optional() const {
  return label() == LABEL_OPTIONAL;
}

inline bool FieldDescriptor::is_repeated() const {
  return label() == LABEL_REPEATED;
}

inline bool FieldDescriptor::is_packable() const {
  return is_repeated() && IsTypePackable(type());
}



inline int FieldDescriptor::index() const {
  if (!is_extension_) {
    return static_cast<int>(this - containing_type_->fields_);
  } else if (extension_scope_ != NULL) {
    return static_cast<int>(this - extension_scope_->extensions_);
  } else {
    return static_cast<int>(this - file_->extensions_);
  }
}

inline int Descriptor::index() const {
  if (containing_type_ == NULL) {
    return static_cast<int>(this - file_->message_types_);
  } else {
    return static_cast<int>(this - containing_type_->nested_types_);
  }
}

inline int OneofDescriptor::index() const {
  return static_cast<int>(this - containing_type_->oneof_decls_);
}

inline int EnumDescriptor::index() const {
  if (containing_type_ == NULL) {
    return static_cast<int>(this - file_->enum_types_);
  } else {
    return static_cast<int>(this - containing_type_->enum_types_);
  }
}

inline int EnumValueDescriptor::index() const {
  return static_cast<int>(this - type_->values_);
}

inline int ServiceDescriptor::index() const {
  return static_cast<int>(this - file_->services_);
}

inline int MethodDescriptor::index() const {
  return static_cast<int>(this - service_->methods_);
}

inline const char* FieldDescriptor::type_name() const {
  return kTypeToName[type_];
}

inline FieldDescriptor::CppType FieldDescriptor::cpp_type() const {
  return kTypeToCppTypeMap[type_];
}

inline const char* FieldDescriptor::cpp_type_name() const {
  return kCppTypeToName[kTypeToCppTypeMap[type_]];
}

inline FieldDescriptor::CppType FieldDescriptor::TypeToCppType(Type type) {
  return kTypeToCppTypeMap[type];
}

inline const char* FieldDescriptor::TypeName(Type type) {
  return kTypeToName[type];
}

inline const char* FieldDescriptor::CppTypeName(CppType cpp_type) {
  return kCppTypeToName[cpp_type];
}

inline bool FieldDescriptor::IsTypePackable(Type field_type) {
  return (field_type != FieldDescriptor::TYPE_STRING &&
          field_type != FieldDescriptor::TYPE_GROUP &&
          field_type != FieldDescriptor::TYPE_MESSAGE &&
          field_type != FieldDescriptor::TYPE_BYTES);
}

inline const FileDescriptor* FileDescriptor::dependency(int index) const {
  return dependencies_[index];
}

inline const FileDescriptor* FileDescriptor::public_dependency(
    int index) const {
  return dependencies_[public_dependencies_[index]];
}

inline const FileDescriptor* FileDescriptor::weak_dependency(
    int index) const {
  return dependencies_[weak_dependencies_[index]];
}



inline const FieldDescriptor* OneofDescriptor::field(int index) const {
  return fields_[index];
}

}  

}  
#endif  
