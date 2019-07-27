

































#include <google/protobuf/stubs/hash.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/io/strtod.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/stubs/map_util.h>
#include <google/protobuf/stubs/stl_util.h>

#undef PACKAGE  // autoheader #defines this.  :(

namespace google {
namespace protobuf {

const FieldDescriptor::CppType
FieldDescriptor::kTypeToCppTypeMap[MAX_TYPE + 1] = {
  static_cast<CppType>(0),  

  CPPTYPE_DOUBLE,   
  CPPTYPE_FLOAT,    
  CPPTYPE_INT64,    
  CPPTYPE_UINT64,   
  CPPTYPE_INT32,    
  CPPTYPE_UINT64,   
  CPPTYPE_UINT32,   
  CPPTYPE_BOOL,     
  CPPTYPE_STRING,   
  CPPTYPE_MESSAGE,  
  CPPTYPE_MESSAGE,  
  CPPTYPE_STRING,   
  CPPTYPE_UINT32,   
  CPPTYPE_ENUM,     
  CPPTYPE_INT32,    
  CPPTYPE_INT64,    
  CPPTYPE_INT32,    
  CPPTYPE_INT64,    
};

const char * const FieldDescriptor::kTypeToName[MAX_TYPE + 1] = {
  "ERROR",     

  "double",    
  "float",     
  "int64",     
  "uint64",    
  "int32",     
  "fixed64",   
  "fixed32",   
  "bool",      
  "string",    
  "group",     
  "message",   
  "bytes",     
  "uint32",    
  "enum",      
  "sfixed32",  
  "sfixed64",  
  "sint32",    
  "sint64",    
};

const char * const FieldDescriptor::kCppTypeToName[MAX_CPPTYPE + 1] = {
  "ERROR",     

  "int32",     
  "int64",     
  "uint32",    
  "uint64",    
  "double",    
  "float",     
  "bool",      
  "enum",      
  "string",    
  "message",   
};

const char * const FieldDescriptor::kLabelToName[MAX_LABEL + 1] = {
  "ERROR",     

  "optional",  
  "required",  
  "repeated",  
};

static const char * const kNonLinkedWeakMessageReplacementName = "google.protobuf.Empty";

#ifndef _MSC_VER  
const int FieldDescriptor::kMaxNumber;
const int FieldDescriptor::kFirstReservedNumber;
const int FieldDescriptor::kLastReservedNumber;
#endif

namespace {

string ToCamelCase(const string& input) {
  bool capitalize_next = false;
  string result;
  result.reserve(input.size());

  for (int i = 0; i < input.size(); i++) {
    if (input[i] == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      
      if ('a' <= input[i] && input[i] <= 'z') {
        result.push_back(input[i] - 'a' + 'A');
      } else {
        result.push_back(input[i]);
      }
      capitalize_next = false;
    } else {
      result.push_back(input[i]);
    }
  }

  
  if (!result.empty() && 'A' <= result[0] && result[0] <= 'Z') {
    result[0] = result[0] - 'A' + 'a';
  }

  return result;
}















typedef pair<const void*, const char*> PointerStringPair;

struct PointerStringPairEqual {
  inline bool operator()(const PointerStringPair& a,
                         const PointerStringPair& b) const {
    return a.first == b.first && strcmp(a.second, b.second) == 0;
  }
};

template<typename PairType>
struct PointerIntegerPairHash {
  size_t operator()(const PairType& p) const {
    
    
    return reinterpret_cast<intptr_t>(p.first) * ((1 << 16) - 1) + p.second;
  }

#ifdef _MSC_VER
  
  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
#endif
  inline bool operator()(const PairType& a, const PairType& b) const {
    return a.first < b.first ||
          (a.first == b.first && a.second < b.second);
  }
};

typedef pair<const Descriptor*, int> DescriptorIntPair;
typedef pair<const EnumDescriptor*, int> EnumIntPair;

struct PointerStringPairHash {
  size_t operator()(const PointerStringPair& p) const {
    
    
    hash<const char*> cstring_hash;
    return reinterpret_cast<intptr_t>(p.first) * ((1 << 16) - 1) +
           cstring_hash(p.second);
  }

#ifdef _MSC_VER
  
  static const size_t bucket_size = 4;
  static const size_t min_buckets = 8;
#endif
  inline bool operator()(const PointerStringPair& a,
                         const PointerStringPair& b) const {
    if (a.first < b.first) return true;
    if (a.first > b.first) return false;
    return strcmp(a.second, b.second) < 0;
  }
};


struct Symbol {
  enum Type {
    NULL_SYMBOL, MESSAGE, FIELD, ONEOF, ENUM, ENUM_VALUE, SERVICE, METHOD,
    PACKAGE
  };
  Type type;
  union {
    const Descriptor* descriptor;
    const FieldDescriptor* field_descriptor;
    const OneofDescriptor* oneof_descriptor;
    const EnumDescriptor* enum_descriptor;
    const EnumValueDescriptor* enum_value_descriptor;
    const ServiceDescriptor* service_descriptor;
    const MethodDescriptor* method_descriptor;
    const FileDescriptor* package_file_descriptor;
  };

  inline Symbol() : type(NULL_SYMBOL) { descriptor = NULL; }
  inline bool IsNull() const { return type == NULL_SYMBOL; }
  inline bool IsType() const {
    return type == MESSAGE || type == ENUM;
  }
  inline bool IsAggregate() const {
    return type == MESSAGE || type == PACKAGE
        || type == ENUM || type == SERVICE;
  }

#define CONSTRUCTOR(TYPE, TYPE_CONSTANT, FIELD)  \
  inline explicit Symbol(const TYPE* value) {    \
    type = TYPE_CONSTANT;                        \
    this->FIELD = value;                         \
  }

  CONSTRUCTOR(Descriptor         , MESSAGE   , descriptor             )
  CONSTRUCTOR(FieldDescriptor    , FIELD     , field_descriptor       )
  CONSTRUCTOR(OneofDescriptor    , ONEOF     , oneof_descriptor       )
  CONSTRUCTOR(EnumDescriptor     , ENUM      , enum_descriptor        )
  CONSTRUCTOR(EnumValueDescriptor, ENUM_VALUE, enum_value_descriptor  )
  CONSTRUCTOR(ServiceDescriptor  , SERVICE   , service_descriptor     )
  CONSTRUCTOR(MethodDescriptor   , METHOD    , method_descriptor      )
  CONSTRUCTOR(FileDescriptor     , PACKAGE   , package_file_descriptor)
#undef CONSTRUCTOR

  const FileDescriptor* GetFile() const {
    switch (type) {
      case NULL_SYMBOL: return NULL;
      case MESSAGE    : return descriptor           ->file();
      case FIELD      : return field_descriptor     ->file();
      case ONEOF      : return oneof_descriptor     ->containing_type()->file();
      case ENUM       : return enum_descriptor      ->file();
      case ENUM_VALUE : return enum_value_descriptor->type()->file();
      case SERVICE    : return service_descriptor   ->file();
      case METHOD     : return method_descriptor    ->service()->file();
      case PACKAGE    : return package_file_descriptor;
    }
    return NULL;
  }
};

const Symbol kNullSymbol;

typedef hash_map<const char*, Symbol,
                 hash<const char*>, streq>
  SymbolsByNameMap;
typedef hash_map<PointerStringPair, Symbol,
                 PointerStringPairHash, PointerStringPairEqual>
  SymbolsByParentMap;
typedef hash_map<const char*, const FileDescriptor*,
                 hash<const char*>, streq>
  FilesByNameMap;
typedef hash_map<PointerStringPair, const FieldDescriptor*,
                 PointerStringPairHash, PointerStringPairEqual>
  FieldsByNameMap;
typedef hash_map<DescriptorIntPair, const FieldDescriptor*,
                 PointerIntegerPairHash<DescriptorIntPair> >
  FieldsByNumberMap;
typedef hash_map<EnumIntPair, const EnumValueDescriptor*,
                 PointerIntegerPairHash<EnumIntPair> >
  EnumValuesByNumberMap;




typedef map<DescriptorIntPair, const FieldDescriptor*>
  ExtensionsGroupedByDescriptorMap;
typedef hash_map<string, const SourceCodeInfo_Location*> LocationsByPathMap;
}  




class DescriptorPool::Tables {
 public:
  Tables();
  ~Tables();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void AddCheckpoint();

  
  
  
  
  
  
  
  void ClearLastCheckpoint();

  
  
  void RollbackToLastCheckpoint();

  
  
  
  vector<string> pending_files_;

  
  
  
  
  
  hash_set<string> known_bad_files_;

  
  
  
  
  hash_set<string> known_bad_symbols_;

  
  
  hash_set<const Descriptor*> extensions_loaded_from_db_;

  
  

  
  
  inline Symbol FindSymbol(const string& key) const;

  
  
  
  
  Symbol FindByNameHelper(
    const DescriptorPool* pool, const string& name);

  
  inline const FileDescriptor* FindFile(const string& key) const;
  inline const FieldDescriptor* FindExtension(const Descriptor* extendee,
                                              int number);
  inline void FindAllExtensions(const Descriptor* extendee,
                                vector<const FieldDescriptor*>* out) const;

  
  

  
  
  
  
  bool AddSymbol(const string& full_name, Symbol symbol);
  bool AddFile(const FileDescriptor* file);
  bool AddExtension(const FieldDescriptor* field);

  
  

  
  
  
  
  template<typename Type> Type* Allocate();

  
  
  template<typename Type> Type* AllocateArray(int count);

  
  
  string* AllocateString(const string& value);

  
  
  
  
  template<typename Type> Type* AllocateMessage(Type* dummy = NULL);

  
  FileDescriptorTables* AllocateFileTables();

 private:
  vector<string*> strings_;    
  vector<Message*> messages_;  
  vector<FileDescriptorTables*> file_tables_;  
  vector<void*> allocations_;  

  SymbolsByNameMap      symbols_by_name_;
  FilesByNameMap        files_by_name_;
  ExtensionsGroupedByDescriptorMap extensions_;

  struct CheckPoint {
    explicit CheckPoint(const Tables* tables)
      : strings_before_checkpoint(tables->strings_.size()),
        messages_before_checkpoint(tables->messages_.size()),
        file_tables_before_checkpoint(tables->file_tables_.size()),
        allocations_before_checkpoint(tables->allocations_.size()),
        pending_symbols_before_checkpoint(
            tables->symbols_after_checkpoint_.size()),
        pending_files_before_checkpoint(
            tables->files_after_checkpoint_.size()),
        pending_extensions_before_checkpoint(
            tables->extensions_after_checkpoint_.size()) {
    }
    int strings_before_checkpoint;
    int messages_before_checkpoint;
    int file_tables_before_checkpoint;
    int allocations_before_checkpoint;
    int pending_symbols_before_checkpoint;
    int pending_files_before_checkpoint;
    int pending_extensions_before_checkpoint;
  };
  vector<CheckPoint> checkpoints_;
  vector<const char*      > symbols_after_checkpoint_;
  vector<const char*      > files_after_checkpoint_;
  vector<DescriptorIntPair> extensions_after_checkpoint_;

  
  
  void* AllocateBytes(int size);
};










class FileDescriptorTables {
 public:
  FileDescriptorTables();
  ~FileDescriptorTables();

  
  static const FileDescriptorTables kEmpty;

  
  

  
  
  inline Symbol FindNestedSymbol(const void* parent,
                                 const string& name) const;
  inline Symbol FindNestedSymbolOfType(const void* parent,
                                       const string& name,
                                       const Symbol::Type type) const;

  
  inline const FieldDescriptor* FindFieldByNumber(
    const Descriptor* parent, int number) const;
  inline const FieldDescriptor* FindFieldByLowercaseName(
    const void* parent, const string& lowercase_name) const;
  inline const FieldDescriptor* FindFieldByCamelcaseName(
    const void* parent, const string& camelcase_name) const;
  inline const EnumValueDescriptor* FindEnumValueByNumber(
    const EnumDescriptor* parent, int number) const;

  
  

  
  
  
  
  bool AddAliasUnderParent(const void* parent, const string& name,
                           Symbol symbol);
  bool AddFieldByNumber(const FieldDescriptor* field);
  bool AddEnumValueByNumber(const EnumValueDescriptor* value);

  
  
  void AddFieldByStylizedNames(const FieldDescriptor* field);

  
  
  static void BuildLocationsByPath(
      pair<const FileDescriptorTables*, const SourceCodeInfo*>* p);

  
  
  
  
  const SourceCodeInfo_Location* GetSourceLocation(
      const vector<int>& path, const SourceCodeInfo* info) const;

 private:
  SymbolsByParentMap    symbols_by_parent_;
  FieldsByNameMap       fields_by_lowercase_name_;
  FieldsByNameMap       fields_by_camelcase_name_;
  FieldsByNumberMap     fields_by_number_;       
  EnumValuesByNumberMap enum_values_by_number_;

  
  mutable GoogleOnceDynamic locations_by_path_once_;
  mutable LocationsByPathMap locations_by_path_;
};

DescriptorPool::Tables::Tables()
    
    : known_bad_files_(3),
      known_bad_symbols_(3),
      extensions_loaded_from_db_(3),
      symbols_by_name_(3),
      files_by_name_(3) {}


DescriptorPool::Tables::~Tables() {
  GOOGLE_DCHECK(checkpoints_.empty());
  
  
  STLDeleteElements(&messages_);
  for (int i = 0; i < allocations_.size(); i++) {
    operator delete(allocations_[i]);
  }
  STLDeleteElements(&strings_);
  STLDeleteElements(&file_tables_);
}

FileDescriptorTables::FileDescriptorTables()
    
    : symbols_by_parent_(3),
      fields_by_lowercase_name_(3),
      fields_by_camelcase_name_(3),
      fields_by_number_(3),
      enum_values_by_number_(3) {
}

FileDescriptorTables::~FileDescriptorTables() {}

const FileDescriptorTables FileDescriptorTables::kEmpty;

void DescriptorPool::Tables::AddCheckpoint() {
  checkpoints_.push_back(CheckPoint(this));
}

void DescriptorPool::Tables::ClearLastCheckpoint() {
  GOOGLE_DCHECK(!checkpoints_.empty());
  checkpoints_.pop_back();
  if (checkpoints_.empty()) {
    
    
    symbols_after_checkpoint_.clear();
    files_after_checkpoint_.clear();
    extensions_after_checkpoint_.clear();
  }
}

void DescriptorPool::Tables::RollbackToLastCheckpoint() {
  GOOGLE_DCHECK(!checkpoints_.empty());
  const CheckPoint& checkpoint = checkpoints_.back();

  for (int i = checkpoint.pending_symbols_before_checkpoint;
       i < symbols_after_checkpoint_.size();
       i++) {
    symbols_by_name_.erase(symbols_after_checkpoint_[i]);
  }
  for (int i = checkpoint.pending_files_before_checkpoint;
       i < files_after_checkpoint_.size();
       i++) {
    files_by_name_.erase(files_after_checkpoint_[i]);
  }
  for (int i = checkpoint.pending_extensions_before_checkpoint;
       i < extensions_after_checkpoint_.size();
       i++) {
    extensions_.erase(extensions_after_checkpoint_[i]);
  }

  symbols_after_checkpoint_.resize(
      checkpoint.pending_symbols_before_checkpoint);
  files_after_checkpoint_.resize(checkpoint.pending_files_before_checkpoint);
  extensions_after_checkpoint_.resize(
      checkpoint.pending_extensions_before_checkpoint);

  STLDeleteContainerPointers(
      strings_.begin() + checkpoint.strings_before_checkpoint, strings_.end());
  STLDeleteContainerPointers(
      messages_.begin() + checkpoint.messages_before_checkpoint,
      messages_.end());
  STLDeleteContainerPointers(
      file_tables_.begin() + checkpoint.file_tables_before_checkpoint,
      file_tables_.end());
  for (int i = checkpoint.allocations_before_checkpoint;
       i < allocations_.size();
       i++) {
    operator delete(allocations_[i]);
  }

  strings_.resize(checkpoint.strings_before_checkpoint);
  messages_.resize(checkpoint.messages_before_checkpoint);
  file_tables_.resize(checkpoint.file_tables_before_checkpoint);
  allocations_.resize(checkpoint.allocations_before_checkpoint);
  checkpoints_.pop_back();
}



inline Symbol DescriptorPool::Tables::FindSymbol(const string& key) const {
  const Symbol* result = FindOrNull(symbols_by_name_, key.c_str());
  if (result == NULL) {
    return kNullSymbol;
  } else {
    return *result;
  }
}

inline Symbol FileDescriptorTables::FindNestedSymbol(
    const void* parent, const string& name) const {
  const Symbol* result =
    FindOrNull(symbols_by_parent_, PointerStringPair(parent, name.c_str()));
  if (result == NULL) {
    return kNullSymbol;
  } else {
    return *result;
  }
}

inline Symbol FileDescriptorTables::FindNestedSymbolOfType(
    const void* parent, const string& name, const Symbol::Type type) const {
  Symbol result = FindNestedSymbol(parent, name);
  if (result.type != type) return kNullSymbol;
  return result;
}

Symbol DescriptorPool::Tables::FindByNameHelper(
    const DescriptorPool* pool, const string& name) {
  MutexLockMaybe lock(pool->mutex_);
  known_bad_symbols_.clear();
  known_bad_files_.clear();
  Symbol result = FindSymbol(name);

  if (result.IsNull() && pool->underlay_ != NULL) {
    
    result =
      pool->underlay_->tables_->FindByNameHelper(pool->underlay_, name);
  }

  if (result.IsNull()) {
    
    if (pool->TryFindSymbolInFallbackDatabase(name)) {
      result = FindSymbol(name);
    }
  }

  return result;
}

inline const FileDescriptor* DescriptorPool::Tables::FindFile(
    const string& key) const {
  return FindPtrOrNull(files_by_name_, key.c_str());
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByNumber(
    const Descriptor* parent, int number) const {
  return FindPtrOrNull(fields_by_number_, make_pair(parent, number));
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByLowercaseName(
    const void* parent, const string& lowercase_name) const {
  return FindPtrOrNull(fields_by_lowercase_name_,
                       PointerStringPair(parent, lowercase_name.c_str()));
}

inline const FieldDescriptor* FileDescriptorTables::FindFieldByCamelcaseName(
    const void* parent, const string& camelcase_name) const {
  return FindPtrOrNull(fields_by_camelcase_name_,
                       PointerStringPair(parent, camelcase_name.c_str()));
}

inline const EnumValueDescriptor* FileDescriptorTables::FindEnumValueByNumber(
    const EnumDescriptor* parent, int number) const {
  return FindPtrOrNull(enum_values_by_number_, make_pair(parent, number));
}

inline const FieldDescriptor* DescriptorPool::Tables::FindExtension(
    const Descriptor* extendee, int number) {
  return FindPtrOrNull(extensions_, make_pair(extendee, number));
}

inline void DescriptorPool::Tables::FindAllExtensions(
    const Descriptor* extendee, vector<const FieldDescriptor*>* out) const {
  ExtensionsGroupedByDescriptorMap::const_iterator it =
      extensions_.lower_bound(make_pair(extendee, 0));
  for (; it != extensions_.end() && it->first.first == extendee; ++it) {
    out->push_back(it->second);
  }
}



bool DescriptorPool::Tables::AddSymbol(
    const string& full_name, Symbol symbol) {
  if (InsertIfNotPresent(&symbols_by_name_, full_name.c_str(), symbol)) {
    symbols_after_checkpoint_.push_back(full_name.c_str());
    return true;
  } else {
    return false;
  }
}

bool FileDescriptorTables::AddAliasUnderParent(
    const void* parent, const string& name, Symbol symbol) {
  PointerStringPair by_parent_key(parent, name.c_str());
  return InsertIfNotPresent(&symbols_by_parent_, by_parent_key, symbol);
}

bool DescriptorPool::Tables::AddFile(const FileDescriptor* file) {
  if (InsertIfNotPresent(&files_by_name_, file->name().c_str(), file)) {
    files_after_checkpoint_.push_back(file->name().c_str());
    return true;
  } else {
    return false;
  }
}

void FileDescriptorTables::AddFieldByStylizedNames(
    const FieldDescriptor* field) {
  const void* parent;
  if (field->is_extension()) {
    if (field->extension_scope() == NULL) {
      parent = field->file();
    } else {
      parent = field->extension_scope();
    }
  } else {
    parent = field->containing_type();
  }

  PointerStringPair lowercase_key(parent, field->lowercase_name().c_str());
  InsertIfNotPresent(&fields_by_lowercase_name_, lowercase_key, field);

  PointerStringPair camelcase_key(parent, field->camelcase_name().c_str());
  InsertIfNotPresent(&fields_by_camelcase_name_, camelcase_key, field);
}

bool FileDescriptorTables::AddFieldByNumber(const FieldDescriptor* field) {
  DescriptorIntPair key(field->containing_type(), field->number());
  return InsertIfNotPresent(&fields_by_number_, key, field);
}

bool FileDescriptorTables::AddEnumValueByNumber(
    const EnumValueDescriptor* value) {
  EnumIntPair key(value->type(), value->number());
  return InsertIfNotPresent(&enum_values_by_number_, key, value);
}

bool DescriptorPool::Tables::AddExtension(const FieldDescriptor* field) {
  DescriptorIntPair key(field->containing_type(), field->number());
  if (InsertIfNotPresent(&extensions_, key, field)) {
    extensions_after_checkpoint_.push_back(key);
    return true;
  } else {
    return false;
  }
}



template<typename Type>
Type* DescriptorPool::Tables::Allocate() {
  return reinterpret_cast<Type*>(AllocateBytes(sizeof(Type)));
}

template<typename Type>
Type* DescriptorPool::Tables::AllocateArray(int count) {
  return reinterpret_cast<Type*>(AllocateBytes(sizeof(Type) * count));
}

string* DescriptorPool::Tables::AllocateString(const string& value) {
  string* result = new string(value);
  strings_.push_back(result);
  return result;
}

template<typename Type>
Type* DescriptorPool::Tables::AllocateMessage(Type* ) {
  Type* result = new Type;
  messages_.push_back(result);
  return result;
}

FileDescriptorTables* DescriptorPool::Tables::AllocateFileTables() {
  FileDescriptorTables* result = new FileDescriptorTables;
  file_tables_.push_back(result);
  return result;
}

void* DescriptorPool::Tables::AllocateBytes(int size) {
  
  
  
  
  if (size == 0) return NULL;

  void* result = operator new(size);
  allocations_.push_back(result);
  return result;
}

void FileDescriptorTables::BuildLocationsByPath(
    pair<const FileDescriptorTables*, const SourceCodeInfo*>* p) {
  for (int i = 0, len = p->second->location_size(); i < len; ++i) {
    const SourceCodeInfo_Location* loc = &p->second->location().Get(i);
    p->first->locations_by_path_[Join(loc->path(), ",")] = loc;
  }
}

const SourceCodeInfo_Location* FileDescriptorTables::GetSourceLocation(
    const vector<int>& path, const SourceCodeInfo* info) const {
  pair<const FileDescriptorTables*, const SourceCodeInfo*> p(
      make_pair(this, info));
  locations_by_path_once_.Init(&FileDescriptorTables::BuildLocationsByPath, &p);
  return FindPtrOrNull(locations_by_path_, Join(path, ","));
}




DescriptorPool::ErrorCollector::~ErrorCollector() {}

DescriptorPool::DescriptorPool()
  : mutex_(NULL),
    fallback_database_(NULL),
    default_error_collector_(NULL),
    underlay_(NULL),
    tables_(new Tables),
    enforce_dependencies_(true),
    allow_unknown_(false),
    enforce_weak_(false) {}

DescriptorPool::DescriptorPool(DescriptorDatabase* fallback_database,
                               ErrorCollector* error_collector)
  : mutex_(new Mutex),
    fallback_database_(fallback_database),
    default_error_collector_(error_collector),
    underlay_(NULL),
    tables_(new Tables),
    enforce_dependencies_(true),
    allow_unknown_(false),
    enforce_weak_(false) {
}

DescriptorPool::DescriptorPool(const DescriptorPool* underlay)
  : mutex_(NULL),
    fallback_database_(NULL),
    default_error_collector_(NULL),
    underlay_(underlay),
    tables_(new Tables),
    enforce_dependencies_(true),
    allow_unknown_(false),
    enforce_weak_(false) {}

DescriptorPool::~DescriptorPool() {
  if (mutex_ != NULL) delete mutex_;
}




void DescriptorPool::InternalDontEnforceDependencies() {
  enforce_dependencies_ = false;
}

void DescriptorPool::AddUnusedImportTrackFile(const string& file_name) {
  unused_import_track_files_.insert(file_name);
}

void DescriptorPool::ClearUnusedImportTrackFiles() {
  unused_import_track_files_.clear();
}

bool DescriptorPool::InternalIsFileLoaded(const string& filename) const {
  MutexLockMaybe lock(mutex_);
  return tables_->FindFile(filename) != NULL;
}



namespace {


EncodedDescriptorDatabase* generated_database_ = NULL;
DescriptorPool* generated_pool_ = NULL;
GOOGLE_PROTOBUF_DECLARE_ONCE(generated_pool_init_);

void DeleteGeneratedPool() {
  delete generated_database_;
  generated_database_ = NULL;
  delete generated_pool_;
  generated_pool_ = NULL;
}

static void InitGeneratedPool() {
  generated_database_ = new EncodedDescriptorDatabase;
  generated_pool_ = new DescriptorPool(generated_database_);

  internal::OnShutdown(&DeleteGeneratedPool);
}

inline void InitGeneratedPoolOnce() {
  ::google::protobuf::GoogleOnceInit(&generated_pool_init_, &InitGeneratedPool);
}

}  

const DescriptorPool* DescriptorPool::generated_pool() {
  InitGeneratedPoolOnce();
  return generated_pool_;
}

DescriptorPool* DescriptorPool::internal_generated_pool() {
  InitGeneratedPoolOnce();
  return generated_pool_;
}

void DescriptorPool::InternalAddGeneratedFile(
    const void* encoded_file_descriptor, int size) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  InitGeneratedPoolOnce();
  GOOGLE_CHECK(generated_database_->Add(encoded_file_descriptor, size));
}








const FileDescriptor* DescriptorPool::FindFileByName(const string& name) const {
  MutexLockMaybe lock(mutex_);
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  const FileDescriptor* result = tables_->FindFile(name);
  if (result != NULL) return result;
  if (underlay_ != NULL) {
    result = underlay_->FindFileByName(name);
    if (result != NULL) return result;
  }
  if (TryFindFileInFallbackDatabase(name)) {
    result = tables_->FindFile(name);
    if (result != NULL) return result;
  }
  return NULL;
}

const FileDescriptor* DescriptorPool::FindFileContainingSymbol(
    const string& symbol_name) const {
  MutexLockMaybe lock(mutex_);
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  Symbol result = tables_->FindSymbol(symbol_name);
  if (!result.IsNull()) return result.GetFile();
  if (underlay_ != NULL) {
    const FileDescriptor* file_result =
      underlay_->FindFileContainingSymbol(symbol_name);
    if (file_result != NULL) return file_result;
  }
  if (TryFindSymbolInFallbackDatabase(symbol_name)) {
    result = tables_->FindSymbol(symbol_name);
    if (!result.IsNull()) return result.GetFile();
  }
  return NULL;
}

const Descriptor* DescriptorPool::FindMessageTypeByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::MESSAGE) ? result.descriptor : NULL;
}

const FieldDescriptor* DescriptorPool::FindFieldByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  if (result.type == Symbol::FIELD &&
      !result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor* DescriptorPool::FindExtensionByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  if (result.type == Symbol::FIELD &&
      result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const OneofDescriptor* DescriptorPool::FindOneofByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::ONEOF) ? result.oneof_descriptor : NULL;
}

const EnumDescriptor* DescriptorPool::FindEnumTypeByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::ENUM) ? result.enum_descriptor : NULL;
}

const EnumValueDescriptor* DescriptorPool::FindEnumValueByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::ENUM_VALUE) ?
    result.enum_value_descriptor : NULL;
}

const ServiceDescriptor* DescriptorPool::FindServiceByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::SERVICE) ? result.service_descriptor : NULL;
}

const MethodDescriptor* DescriptorPool::FindMethodByName(
    const string& name) const {
  Symbol result = tables_->FindByNameHelper(this, name);
  return (result.type == Symbol::METHOD) ? result.method_descriptor : NULL;
}

const FieldDescriptor* DescriptorPool::FindExtensionByNumber(
    const Descriptor* extendee, int number) const {
  MutexLockMaybe lock(mutex_);
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  const FieldDescriptor* result = tables_->FindExtension(extendee, number);
  if (result != NULL) {
    return result;
  }
  if (underlay_ != NULL) {
    result = underlay_->FindExtensionByNumber(extendee, number);
    if (result != NULL) return result;
  }
  if (TryFindExtensionInFallbackDatabase(extendee, number)) {
    result = tables_->FindExtension(extendee, number);
    if (result != NULL) {
      return result;
    }
  }
  return NULL;
}

void DescriptorPool::FindAllExtensions(
    const Descriptor* extendee, vector<const FieldDescriptor*>* out) const {
  MutexLockMaybe lock(mutex_);
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();

  
  
  if (fallback_database_ != NULL &&
      tables_->extensions_loaded_from_db_.count(extendee) == 0) {
    vector<int> numbers;
    if (fallback_database_->FindAllExtensionNumbers(extendee->full_name(),
                                                    &numbers)) {
      for (int i = 0; i < numbers.size(); ++i) {
        int number = numbers[i];
        if (tables_->FindExtension(extendee, number) == NULL) {
          TryFindExtensionInFallbackDatabase(extendee, number);
        }
      }
      tables_->extensions_loaded_from_db_.insert(extendee);
    }
  }

  tables_->FindAllExtensions(extendee, out);
  if (underlay_ != NULL) {
    underlay_->FindAllExtensions(extendee, out);
  }
}




const FieldDescriptor*
Descriptor::FindFieldByNumber(int key) const {
  const FieldDescriptor* result =
    file()->tables_->FindFieldByNumber(this, key);
  if (result == NULL || result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
Descriptor::FindFieldByLowercaseName(const string& key) const {
  const FieldDescriptor* result =
    file()->tables_->FindFieldByLowercaseName(this, key);
  if (result == NULL || result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
Descriptor::FindFieldByCamelcaseName(const string& key) const {
  const FieldDescriptor* result =
    file()->tables_->FindFieldByCamelcaseName(this, key);
  if (result == NULL || result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
Descriptor::FindFieldByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && !result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const OneofDescriptor*
Descriptor::FindOneofByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::ONEOF);
  if (!result.IsNull()) {
    return result.oneof_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
Descriptor::FindExtensionByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
Descriptor::FindExtensionByLowercaseName(const string& key) const {
  const FieldDescriptor* result =
    file()->tables_->FindFieldByLowercaseName(this, key);
  if (result == NULL || !result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
Descriptor::FindExtensionByCamelcaseName(const string& key) const {
  const FieldDescriptor* result =
    file()->tables_->FindFieldByCamelcaseName(this, key);
  if (result == NULL || !result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const Descriptor*
Descriptor::FindNestedTypeByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::MESSAGE);
  if (!result.IsNull()) {
    return result.descriptor;
  } else {
    return NULL;
  }
}

const EnumDescriptor*
Descriptor::FindEnumTypeByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM);
  if (!result.IsNull()) {
    return result.enum_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
Descriptor::FindEnumValueByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
EnumDescriptor::FindValueByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
EnumDescriptor::FindValueByNumber(int key) const {
  return file()->tables_->FindEnumValueByNumber(this, key);
}

const MethodDescriptor*
ServiceDescriptor::FindMethodByName(const string& key) const {
  Symbol result =
    file()->tables_->FindNestedSymbolOfType(this, key, Symbol::METHOD);
  if (!result.IsNull()) {
    return result.method_descriptor;
  } else {
    return NULL;
  }
}

const Descriptor*
FileDescriptor::FindMessageTypeByName(const string& key) const {
  Symbol result = tables_->FindNestedSymbolOfType(this, key, Symbol::MESSAGE);
  if (!result.IsNull()) {
    return result.descriptor;
  } else {
    return NULL;
  }
}

const EnumDescriptor*
FileDescriptor::FindEnumTypeByName(const string& key) const {
  Symbol result = tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM);
  if (!result.IsNull()) {
    return result.enum_descriptor;
  } else {
    return NULL;
  }
}

const EnumValueDescriptor*
FileDescriptor::FindEnumValueByName(const string& key) const {
  Symbol result =
    tables_->FindNestedSymbolOfType(this, key, Symbol::ENUM_VALUE);
  if (!result.IsNull()) {
    return result.enum_value_descriptor;
  } else {
    return NULL;
  }
}

const ServiceDescriptor*
FileDescriptor::FindServiceByName(const string& key) const {
  Symbol result = tables_->FindNestedSymbolOfType(this, key, Symbol::SERVICE);
  if (!result.IsNull()) {
    return result.service_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
FileDescriptor::FindExtensionByName(const string& key) const {
  Symbol result = tables_->FindNestedSymbolOfType(this, key, Symbol::FIELD);
  if (!result.IsNull() && result.field_descriptor->is_extension()) {
    return result.field_descriptor;
  } else {
    return NULL;
  }
}

const FieldDescriptor*
FileDescriptor::FindExtensionByLowercaseName(const string& key) const {
  const FieldDescriptor* result = tables_->FindFieldByLowercaseName(this, key);
  if (result == NULL || !result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const FieldDescriptor*
FileDescriptor::FindExtensionByCamelcaseName(const string& key) const {
  const FieldDescriptor* result = tables_->FindFieldByCamelcaseName(this, key);
  if (result == NULL || !result->is_extension()) {
    return NULL;
  } else {
    return result;
  }
}

const Descriptor::ExtensionRange*
Descriptor::FindExtensionRangeContainingNumber(int number) const {
  
  
  for (int i = 0; i < extension_range_count(); i++) {
    if (number >= extension_range(i)->start &&
        number <  extension_range(i)->end) {
      return extension_range(i);
    }
  }
  return NULL;
}



bool DescriptorPool::TryFindFileInFallbackDatabase(const string& name) const {
  if (fallback_database_ == NULL) return false;

  if (tables_->known_bad_files_.count(name) > 0) return false;

  FileDescriptorProto file_proto;
  if (!fallback_database_->FindFileByName(name, &file_proto) ||
      BuildFileFromDatabase(file_proto) == NULL) {
    tables_->known_bad_files_.insert(name);
    return false;
  }
  return true;
}

bool DescriptorPool::IsSubSymbolOfBuiltType(const string& name) const {
  string prefix = name;
  for (;;) {
    string::size_type dot_pos = prefix.find_last_of('.');
    if (dot_pos == string::npos) {
      break;
    }
    prefix = prefix.substr(0, dot_pos);
    Symbol symbol = tables_->FindSymbol(prefix);
    
    
    if (!symbol.IsNull() && symbol.type != Symbol::PACKAGE) {
      return true;
    }
  }
  if (underlay_ != NULL) {
    
    return underlay_->IsSubSymbolOfBuiltType(name);
  }
  return false;
}

bool DescriptorPool::TryFindSymbolInFallbackDatabase(const string& name) const {
  if (fallback_database_ == NULL) return false;

  if (tables_->known_bad_symbols_.count(name) > 0) return false;

  FileDescriptorProto file_proto;
  if (
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      IsSubSymbolOfBuiltType(name)

      
      || !fallback_database_->FindFileContainingSymbol(name, &file_proto)

      
      
      
      || tables_->FindFile(file_proto.name()) != NULL

      
      || BuildFileFromDatabase(file_proto) == NULL) {
    tables_->known_bad_symbols_.insert(name);
    return false;
  }

  return true;
}

bool DescriptorPool::TryFindExtensionInFallbackDatabase(
    const Descriptor* containing_type, int field_number) const {
  if (fallback_database_ == NULL) return false;

  FileDescriptorProto file_proto;
  if (!fallback_database_->FindFileContainingExtension(
        containing_type->full_name(), field_number, &file_proto)) {
    return false;
  }

  if (tables_->FindFile(file_proto.name()) != NULL) {
    
    
    
    return false;
  }

  if (BuildFileFromDatabase(file_proto) == NULL) {
    return false;
  }

  return true;
}



string FieldDescriptor::DefaultValueAsString(bool quote_string_type) const {
  GOOGLE_CHECK(has_default_value()) << "No default value";
  switch (cpp_type()) {
    case CPPTYPE_INT32:
      return SimpleItoa(default_value_int32());
      break;
    case CPPTYPE_INT64:
      return SimpleItoa(default_value_int64());
      break;
    case CPPTYPE_UINT32:
      return SimpleItoa(default_value_uint32());
      break;
    case CPPTYPE_UINT64:
      return SimpleItoa(default_value_uint64());
      break;
    case CPPTYPE_FLOAT:
      return SimpleFtoa(default_value_float());
      break;
    case CPPTYPE_DOUBLE:
      return SimpleDtoa(default_value_double());
      break;
    case CPPTYPE_BOOL:
      return default_value_bool() ? "true" : "false";
      break;
    case CPPTYPE_STRING:
      if (quote_string_type) {
        return "\"" + CEscape(default_value_string()) + "\"";
      } else {
        if (type() == TYPE_BYTES) {
          return CEscape(default_value_string());
        } else {
          return default_value_string();
        }
      }
      break;
    case CPPTYPE_ENUM:
      return default_value_enum()->name();
      break;
    case CPPTYPE_MESSAGE:
      GOOGLE_LOG(DFATAL) << "Messages can't have default values!";
      break;
  }
  GOOGLE_LOG(FATAL) << "Can't get here: failed to get default value as string";
  return "";
}



void FileDescriptor::CopyTo(FileDescriptorProto* proto) const {
  proto->set_name(name());
  if (!package().empty()) proto->set_package(package());

  for (int i = 0; i < dependency_count(); i++) {
    proto->add_dependency(dependency(i)->name());
  }

  for (int i = 0; i < public_dependency_count(); i++) {
    proto->add_public_dependency(public_dependencies_[i]);
  }

  for (int i = 0; i < weak_dependency_count(); i++) {
    proto->add_weak_dependency(weak_dependencies_[i]);
  }

  for (int i = 0; i < message_type_count(); i++) {
    message_type(i)->CopyTo(proto->add_message_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < service_count(); i++) {
    service(i)->CopyTo(proto->add_service());
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }

  if (&options() != &FileOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void FileDescriptor::CopySourceCodeInfoTo(FileDescriptorProto* proto) const {
  if (source_code_info_ != &SourceCodeInfo::default_instance()) {
    proto->mutable_source_code_info()->CopyFrom(*source_code_info_);
  }
}

void Descriptor::CopyTo(DescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < field_count(); i++) {
    field(i)->CopyTo(proto->add_field());
  }
  for (int i = 0; i < oneof_decl_count(); i++) {
    oneof_decl(i)->CopyTo(proto->add_oneof_decl());
  }
  for (int i = 0; i < nested_type_count(); i++) {
    nested_type(i)->CopyTo(proto->add_nested_type());
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->CopyTo(proto->add_enum_type());
  }
  for (int i = 0; i < extension_range_count(); i++) {
    DescriptorProto::ExtensionRange* range = proto->add_extension_range();
    range->set_start(extension_range(i)->start);
    range->set_end(extension_range(i)->end);
  }
  for (int i = 0; i < extension_count(); i++) {
    extension(i)->CopyTo(proto->add_extension());
  }

  if (&options() != &MessageOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void FieldDescriptor::CopyTo(FieldDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());

  
  
  proto->set_label(static_cast<FieldDescriptorProto::Label>(
                     implicit_cast<int>(label())));
  proto->set_type(static_cast<FieldDescriptorProto::Type>(
                    implicit_cast<int>(type())));

  if (is_extension()) {
    if (!containing_type()->is_unqualified_placeholder_) {
      proto->set_extendee(".");
    }
    proto->mutable_extendee()->append(containing_type()->full_name());
  }

  if (cpp_type() == CPPTYPE_MESSAGE) {
    if (message_type()->is_placeholder_) {
      
      
      proto->clear_type();
    }

    if (!message_type()->is_unqualified_placeholder_) {
      proto->set_type_name(".");
    }
    proto->mutable_type_name()->append(message_type()->full_name());
  } else if (cpp_type() == CPPTYPE_ENUM) {
    if (!enum_type()->is_unqualified_placeholder_) {
      proto->set_type_name(".");
    }
    proto->mutable_type_name()->append(enum_type()->full_name());
  }

  if (has_default_value()) {
    proto->set_default_value(DefaultValueAsString(false));
  }

  if (containing_oneof() != NULL && !is_extension()) {
    proto->set_oneof_index(containing_oneof()->index());
  }

  if (&options() != &FieldOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void OneofDescriptor::CopyTo(OneofDescriptorProto* proto) const {
  proto->set_name(name());
}

void EnumDescriptor::CopyTo(EnumDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < value_count(); i++) {
    value(i)->CopyTo(proto->add_value());
  }

  if (&options() != &EnumOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void EnumValueDescriptor::CopyTo(EnumValueDescriptorProto* proto) const {
  proto->set_name(name());
  proto->set_number(number());

  if (&options() != &EnumValueOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void ServiceDescriptor::CopyTo(ServiceDescriptorProto* proto) const {
  proto->set_name(name());

  for (int i = 0; i < method_count(); i++) {
    method(i)->CopyTo(proto->add_method());
  }

  if (&options() != &ServiceOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}

void MethodDescriptor::CopyTo(MethodDescriptorProto* proto) const {
  proto->set_name(name());

  if (!input_type()->is_unqualified_placeholder_) {
    proto->set_input_type(".");
  }
  proto->mutable_input_type()->append(input_type()->full_name());

  if (!output_type()->is_unqualified_placeholder_) {
    proto->set_output_type(".");
  }
  proto->mutable_output_type()->append(output_type()->full_name());

  if (&options() != &MethodOptions::default_instance()) {
    proto->mutable_options()->CopyFrom(options());
  }
}



namespace {


bool RetrieveOptions(int depth,
                     const Message &options,
                     vector<string> *option_entries) {
  option_entries->clear();
  const Reflection* reflection = options.GetReflection();
  vector<const FieldDescriptor*> fields;
  reflection->ListFields(options, &fields);
  for (int i = 0; i < fields.size(); i++) {
    int count = 1;
    bool repeated = false;
    if (fields[i]->is_repeated()) {
      count = reflection->FieldSize(options, fields[i]);
      repeated = true;
    }
    for (int j = 0; j < count; j++) {
      string fieldval;
      if (fields[i]->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        string tmp;
        TextFormat::Printer printer;
        printer.SetInitialIndentLevel(depth + 1);
        printer.PrintFieldValueToString(options, fields[i],
                                        repeated ? j : -1, &tmp);
        fieldval.append("{\n");
        fieldval.append(tmp);
        fieldval.append(depth * 2, ' ');
        fieldval.append("}");
      } else {
        TextFormat::PrintFieldValueToString(options, fields[i],
                                            repeated ? j : -1, &fieldval);
      }
      string name;
      if (fields[i]->is_extension()) {
        name = "(." + fields[i]->full_name() + ")";
      } else {
        name = fields[i]->name();
      }
      option_entries->push_back(name + " = " + fieldval);
    }
  }
  return !option_entries->empty();
}



bool FormatBracketedOptions(int depth, const Message &options, string *output) {
  vector<string> all_options;
  if (RetrieveOptions(depth, options, &all_options)) {
    output->append(Join(all_options, ", "));
  }
  return !all_options.empty();
}


bool FormatLineOptions(int depth, const Message &options, string *output) {
  string prefix(depth * 2, ' ');
  vector<string> all_options;
  if (RetrieveOptions(depth, options, &all_options)) {
    for (int i = 0; i < all_options.size(); i++) {
      strings::SubstituteAndAppend(output, "$0option $1;\n",
                                   prefix, all_options[i]);
    }
  }
  return !all_options.empty();
}

}  

string FileDescriptor::DebugString() const {
  string contents = "syntax = \"proto2\";\n\n";

  set<int> public_dependencies;
  set<int> weak_dependencies;
  public_dependencies.insert(public_dependencies_,
                             public_dependencies_ + public_dependency_count_);
  weak_dependencies.insert(weak_dependencies_,
                           weak_dependencies_ + weak_dependency_count_);

  for (int i = 0; i < dependency_count(); i++) {
    if (public_dependencies.count(i) > 0) {
      strings::SubstituteAndAppend(&contents, "import public \"$0\";\n",
                                   dependency(i)->name());
    } else if (weak_dependencies.count(i) > 0) {
      strings::SubstituteAndAppend(&contents, "import weak \"$0\";\n",
                                   dependency(i)->name());
    } else {
      strings::SubstituteAndAppend(&contents, "import \"$0\";\n",
                                   dependency(i)->name());
    }
  }

  if (!package().empty()) {
    strings::SubstituteAndAppend(&contents, "package $0;\n\n", package());
  }

  if (FormatLineOptions(0, options(), &contents)) {
    contents.append("\n");  
  }

  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(0, &contents);
    contents.append("\n");
  }

  
  
  set<const Descriptor*> groups;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < message_type_count(); i++) {
    if (groups.count(message_type(i)) == 0) {
      strings::SubstituteAndAppend(&contents, "message $0",
                                   message_type(i)->name());
      message_type(i)->DebugString(0, &contents);
      contents.append("\n");
    }
  }

  for (int i = 0; i < service_count(); i++) {
    service(i)->DebugString(&contents);
    contents.append("\n");
  }

  const Descriptor* containing_type = NULL;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) contents.append("}\n\n");
      containing_type = extension(i)->containing_type();
      strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                                   containing_type->full_name());
    }
    extension(i)->DebugString(1, FieldDescriptor::PRINT_LABEL, &contents);
  }
  if (extension_count() > 0) contents.append("}\n\n");

  return contents;
}

string Descriptor::DebugString() const {
  string contents;
  strings::SubstituteAndAppend(&contents, "message $0", name());
  DebugString(0, &contents);
  return contents;
}

void Descriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  contents->append(" {\n");

  FormatLineOptions(depth, options(), contents);

  
  
  
  set<const Descriptor*> groups;
  for (int i = 0; i < field_count(); i++) {
    if (field(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(field(i)->message_type());
    }
  }
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->type() == FieldDescriptor::TYPE_GROUP) {
      groups.insert(extension(i)->message_type());
    }
  }

  for (int i = 0; i < nested_type_count(); i++) {
    if (groups.count(nested_type(i)) == 0) {
      strings::SubstituteAndAppend(contents, "$0  message $1",
                                   prefix, nested_type(i)->name());
      nested_type(i)->DebugString(depth, contents);
    }
  }
  for (int i = 0; i < enum_type_count(); i++) {
    enum_type(i)->DebugString(depth, contents);
  }
  for (int i = 0; i < field_count(); i++) {
    if (field(i)->containing_oneof() == NULL) {
      field(i)->DebugString(depth, FieldDescriptor::PRINT_LABEL, contents);
    } else if (field(i)->containing_oneof()->field(0) == field(i)) {
      
      field(i)->containing_oneof()->DebugString(depth, contents);
    }
  }

  for (int i = 0; i < extension_range_count(); i++) {
    strings::SubstituteAndAppend(contents, "$0  extensions $1 to $2;\n",
                                 prefix,
                                 extension_range(i)->start,
                                 extension_range(i)->end - 1);
  }

  
  const Descriptor* containing_type = NULL;
  for (int i = 0; i < extension_count(); i++) {
    if (extension(i)->containing_type() != containing_type) {
      if (i > 0) strings::SubstituteAndAppend(contents, "$0  }\n", prefix);
      containing_type = extension(i)->containing_type();
      strings::SubstituteAndAppend(contents, "$0  extend .$1 {\n",
                                   prefix, containing_type->full_name());
    }
    extension(i)->DebugString(
        depth + 1, FieldDescriptor::PRINT_LABEL, contents);
  }
  if (extension_count() > 0)
    strings::SubstituteAndAppend(contents, "$0  }\n", prefix);

  strings::SubstituteAndAppend(contents, "$0}\n", prefix);
}

string FieldDescriptor::DebugString() const {
  string contents;
  int depth = 0;
  if (is_extension()) {
    strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                                 containing_type()->full_name());
    depth = 1;
  }
  DebugString(depth, PRINT_LABEL, &contents);
  if (is_extension()) {
    contents.append("}\n");
  }
  return contents;
}

void FieldDescriptor::DebugString(int depth,
                                  PrintLabelFlag print_label_flag,
                                  string *contents) const {
  string prefix(depth * 2, ' ');
  string field_type;
  switch (type()) {
    case TYPE_MESSAGE:
      field_type = "." + message_type()->full_name();
      break;
    case TYPE_ENUM:
      field_type = "." + enum_type()->full_name();
      break;
    default:
      field_type = kTypeToName[type()];
  }

  string label;
  if (print_label_flag == PRINT_LABEL) {
    label = kLabelToName[this->label()];
    label.push_back(' ');
  }

  strings::SubstituteAndAppend(contents, "$0$1$2 $3 = $4",
                               prefix,
                               label,
                               field_type,
                               type() == TYPE_GROUP ? message_type()->name() :
                                                      name(),
                               number());

  bool bracketed = false;
  if (has_default_value()) {
    bracketed = true;
    strings::SubstituteAndAppend(contents, " [default = $0",
                                 DefaultValueAsString(true));
  }

  string formatted_options;
  if (FormatBracketedOptions(depth, options(), &formatted_options)) {
    contents->append(bracketed ? ", " : " [");
    bracketed = true;
    contents->append(formatted_options);
  }

  if (bracketed) {
    contents->append("]");
  }

  if (type() == TYPE_GROUP) {
    message_type()->DebugString(depth, contents);
  } else {
    contents->append(";\n");
  }
}

string OneofDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void OneofDescriptor::DebugString(int depth, string* contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  strings::SubstituteAndAppend(
      contents, "$0 oneof $1 {\n", prefix, name());
  for (int i = 0; i < field_count(); i++) {
    field(i)->DebugString(depth, FieldDescriptor::OMIT_LABEL, contents);
  }
  strings::SubstituteAndAppend(contents, "$0}\n", prefix);
}

string EnumDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void EnumDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  strings::SubstituteAndAppend(contents, "$0enum $1 {\n",
                               prefix, name());

  FormatLineOptions(depth, options(), contents);

  for (int i = 0; i < value_count(); i++) {
    value(i)->DebugString(depth, contents);
  }
  strings::SubstituteAndAppend(contents, "$0}\n", prefix);
}

string EnumValueDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void EnumValueDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  strings::SubstituteAndAppend(contents, "$0$1 = $2",
                               prefix, name(), number());

  string formatted_options;
  if (FormatBracketedOptions(depth, options(), &formatted_options)) {
    strings::SubstituteAndAppend(contents, " [$0]", formatted_options);
  }
  contents->append(";\n");
}

string ServiceDescriptor::DebugString() const {
  string contents;
  DebugString(&contents);
  return contents;
}

void ServiceDescriptor::DebugString(string *contents) const {
  strings::SubstituteAndAppend(contents, "service $0 {\n", name());

  FormatLineOptions(1, options(), contents);

  for (int i = 0; i < method_count(); i++) {
    method(i)->DebugString(1, contents);
  }

  contents->append("}\n");
}

string MethodDescriptor::DebugString() const {
  string contents;
  DebugString(0, &contents);
  return contents;
}

void MethodDescriptor::DebugString(int depth, string *contents) const {
  string prefix(depth * 2, ' ');
  ++depth;
  strings::SubstituteAndAppend(contents, "$0rpc $1(.$2) returns (.$3)",
                               prefix, name(),
                               input_type()->full_name(),
                               output_type()->full_name());

  string formatted_options;
  if (FormatLineOptions(depth, options(), &formatted_options)) {
    strings::SubstituteAndAppend(contents, " {\n$0$1}\n",
                                 formatted_options, prefix);
  } else {
    contents->append(";\n");
  }
}




bool FileDescriptor::GetSourceLocation(const vector<int>& path,
                                       SourceLocation* out_location) const {
  GOOGLE_CHECK_NOTNULL(out_location);
  if (source_code_info_) {
    if (const SourceCodeInfo_Location* loc =
        tables_->GetSourceLocation(path, source_code_info_)) {
      const RepeatedField<int32>& span = loc->span();
      if (span.size() == 3 || span.size() == 4) {
        out_location->start_line   = span.Get(0);
        out_location->start_column = span.Get(1);
        out_location->end_line     = span.Get(span.size() == 3 ? 0 : 2);
        out_location->end_column   = span.Get(span.size() - 1);

        out_location->leading_comments = loc->leading_comments();
        out_location->trailing_comments = loc->trailing_comments();
        return true;
      }
    }
  }
  return false;
}

bool FieldDescriptor::is_packed() const {
  return is_packable() && (options_ != NULL) && options_->packed();
}

bool Descriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool FieldDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool OneofDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return containing_type()->file()->GetSourceLocation(path, out_location);
}

bool EnumDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool MethodDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return service()->file()->GetSourceLocation(path, out_location);
}

bool ServiceDescriptor::GetSourceLocation(SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return file()->GetSourceLocation(path, out_location);
}

bool EnumValueDescriptor::GetSourceLocation(
    SourceLocation* out_location) const {
  vector<int> path;
  GetLocationPath(&path);
  return type()->file()->GetSourceLocation(path, out_location);
}

void Descriptor::GetLocationPath(vector<int>* output) const {
  if (containing_type()) {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kNestedTypeFieldNumber);
    output->push_back(index());
  } else {
    output->push_back(FileDescriptorProto::kMessageTypeFieldNumber);
    output->push_back(index());
  }
}

void FieldDescriptor::GetLocationPath(vector<int>* output) const {
  if (is_extension()) {
    if (extension_scope() == NULL) {
      output->push_back(FileDescriptorProto::kExtensionFieldNumber);
      output->push_back(index());
    } else {
      extension_scope()->GetLocationPath(output);
      output->push_back(DescriptorProto::kExtensionFieldNumber);
      output->push_back(index());
    }
  } else {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kFieldFieldNumber);
    output->push_back(index());
  }
}

void OneofDescriptor::GetLocationPath(vector<int>* output) const {
  containing_type()->GetLocationPath(output);
  output->push_back(DescriptorProto::kOneofDeclFieldNumber);
  output->push_back(index());
}

void EnumDescriptor::GetLocationPath(vector<int>* output) const {
  if (containing_type()) {
    containing_type()->GetLocationPath(output);
    output->push_back(DescriptorProto::kEnumTypeFieldNumber);
    output->push_back(index());
  } else {
    output->push_back(FileDescriptorProto::kEnumTypeFieldNumber);
    output->push_back(index());
  }
}

void EnumValueDescriptor::GetLocationPath(vector<int>* output) const {
  type()->GetLocationPath(output);
  output->push_back(EnumDescriptorProto::kValueFieldNumber);
  output->push_back(index());
}

void ServiceDescriptor::GetLocationPath(vector<int>* output) const {
  output->push_back(FileDescriptorProto::kServiceFieldNumber);
  output->push_back(index());
}

void MethodDescriptor::GetLocationPath(vector<int>* output) const {
  service()->GetLocationPath(output);
  output->push_back(ServiceDescriptorProto::kMethodFieldNumber);
  output->push_back(index());
}



namespace {






struct OptionsToInterpret {
  OptionsToInterpret(const string& ns,
                     const string& el,
                     const Message* orig_opt,
                     Message* opt)
      : name_scope(ns),
        element_name(el),
        original_options(orig_opt),
        options(opt) {
  }
  string name_scope;
  string element_name;
  const Message* original_options;
  Message* options;
};

}  

class DescriptorBuilder {
 public:
  DescriptorBuilder(const DescriptorPool* pool,
                    DescriptorPool::Tables* tables,
                    DescriptorPool::ErrorCollector* error_collector);
  ~DescriptorBuilder();

  const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

 private:
  friend class OptionInterpreter;

  const DescriptorPool* pool_;
  DescriptorPool::Tables* tables_;  
  DescriptorPool::ErrorCollector* error_collector_;

  
  
  
  vector<OptionsToInterpret> options_to_interpret_;

  bool had_errors_;
  string filename_;
  FileDescriptor* file_;
  FileDescriptorTables* file_tables_;
  set<const FileDescriptor*> dependencies_;

  
  
  set<const FileDescriptor*> unused_dependency_;

  
  
  
  
  
  
  
  const FileDescriptor* possible_undeclared_dependency_;
  string possible_undeclared_dependency_name_;

  
  
  
  string undefine_resolved_name_;

  void AddError(const string& element_name,
                const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                const string& error);
  void AddError(const string& element_name,
                const Message& descriptor,
                DescriptorPool::ErrorCollector::ErrorLocation location,
                const char* error);
  void AddRecursiveImportError(const FileDescriptorProto& proto, int from_here);
  void AddTwiceListedError(const FileDescriptorProto& proto, int index);
  void AddImportError(const FileDescriptorProto& proto, int index);

  
  
  void AddNotDefinedError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& undefined_symbol);

  void AddWarning(const string& element_name, const Message& descriptor,
                  DescriptorPool::ErrorCollector::ErrorLocation location,
                  const string& error);

  
  
  
  bool IsInPackage(const FileDescriptor* file, const string& package_name);

  
  
  void RecordPublicDependencies(const FileDescriptor* file);

  
  
  
  
  Symbol FindSymbol(const string& name);

  
  
  Symbol FindSymbolNotEnforcingDeps(const string& name);

  
  Symbol FindSymbolNotEnforcingDepsHelper(const DescriptorPool* pool,
                                          const string& name);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum PlaceholderType {
    PLACEHOLDER_MESSAGE,
    PLACEHOLDER_ENUM,
    PLACEHOLDER_EXTENDABLE_MESSAGE
  };
  enum ResolveMode {
    LOOKUP_ALL, LOOKUP_TYPES
  };
  Symbol LookupSymbol(const string& name, const string& relative_to,
                      PlaceholderType placeholder_type = PLACEHOLDER_MESSAGE,
                      ResolveMode resolve_mode = LOOKUP_ALL);

  
  
  Symbol LookupSymbolNoPlaceholder(const string& name,
                                   const string& relative_to,
                                   ResolveMode resolve_mode = LOOKUP_ALL);

  
  
  Symbol NewPlaceholder(const string& name, PlaceholderType placeholder_type);

  
  
  const FileDescriptor* NewPlaceholderFile(const string& name);

  
  
  
  bool AddSymbol(const string& full_name,
                 const void* parent, const string& name,
                 const Message& proto, Symbol symbol);

  
  
  
  
  
  void AddPackage(const string& name, const Message& proto,
                  const FileDescriptor* file);

  
  
  void ValidateSymbolName(const string& name, const string& full_name,
                          const Message& proto);

  
  
  bool ValidateQualifiedName(const string& name);

  
  
  template <typename Type>
  inline void AllocateArray(int size, Type** output) {
    *output = tables_->AllocateArray<Type>(size);
  }

  
  
  
  
  template<class DescriptorT> void AllocateOptions(
      const typename DescriptorT::OptionsType& orig_options,
      DescriptorT* descriptor);
  
  void AllocateOptions(const FileOptions& orig_options,
                       FileDescriptor* descriptor);

  
  template<class DescriptorT> void AllocateOptionsImpl(
      const string& name_scope,
      const string& element_name,
      const typename DescriptorT::OptionsType& orig_options,
      DescriptorT* descriptor);

  
  
  void BuildMessage(const DescriptorProto& proto,
                    const Descriptor* parent,
                    Descriptor* result);
  void BuildFieldOrExtension(const FieldDescriptorProto& proto,
                             const Descriptor* parent,
                             FieldDescriptor* result,
                             bool is_extension);
  void BuildField(const FieldDescriptorProto& proto,
                  const Descriptor* parent,
                  FieldDescriptor* result) {
    BuildFieldOrExtension(proto, parent, result, false);
  }
  void BuildExtension(const FieldDescriptorProto& proto,
                      const Descriptor* parent,
                      FieldDescriptor* result) {
    BuildFieldOrExtension(proto, parent, result, true);
  }
  void BuildExtensionRange(const DescriptorProto::ExtensionRange& proto,
                           const Descriptor* parent,
                           Descriptor::ExtensionRange* result);
  void BuildOneof(const OneofDescriptorProto& proto,
                  Descriptor* parent,
                  OneofDescriptor* result);
  void BuildEnum(const EnumDescriptorProto& proto,
                 const Descriptor* parent,
                 EnumDescriptor* result);
  void BuildEnumValue(const EnumValueDescriptorProto& proto,
                      const EnumDescriptor* parent,
                      EnumValueDescriptor* result);
  void BuildService(const ServiceDescriptorProto& proto,
                    const void* dummy,
                    ServiceDescriptor* result);
  void BuildMethod(const MethodDescriptorProto& proto,
                   const ServiceDescriptor* parent,
                   MethodDescriptor* result);

  void LogUnusedDependency(const FileDescriptor* result);

  
  
  
  
  
  void CrossLinkFile(FileDescriptor* file, const FileDescriptorProto& proto);
  void CrossLinkMessage(Descriptor* message, const DescriptorProto& proto);
  void CrossLinkField(FieldDescriptor* field,
                      const FieldDescriptorProto& proto);
  void CrossLinkEnum(EnumDescriptor* enum_type,
                     const EnumDescriptorProto& proto);
  void CrossLinkEnumValue(EnumValueDescriptor* enum_value,
                          const EnumValueDescriptorProto& proto);
  void CrossLinkService(ServiceDescriptor* service,
                        const ServiceDescriptorProto& proto);
  void CrossLinkMethod(MethodDescriptor* method,
                       const MethodDescriptorProto& proto);

  
  void InterpretOptions();

  
  class OptionInterpreter {
   public:
    
    
    
    explicit OptionInterpreter(DescriptorBuilder* builder);

    ~OptionInterpreter();

    
    
    
    bool InterpretOptions(OptionsToInterpret* options_to_interpret);

    class AggregateOptionFinder;

   private:
    
    
    
    bool InterpretSingleOption(Message* options);

    
    
    
    void AddWithoutInterpreting(const UninterpretedOption& uninterpreted_option,
                                Message* options);

    
    
    
    bool ExamineIfOptionIsSet(
        vector<const FieldDescriptor*>::const_iterator intermediate_fields_iter,
        vector<const FieldDescriptor*>::const_iterator intermediate_fields_end,
        const FieldDescriptor* innermost_field, const string& debug_msg_name,
        const UnknownFieldSet& unknown_fields);

    
    
    bool SetOptionValue(const FieldDescriptor* option_field,
                        UnknownFieldSet* unknown_fields);

    
    
    bool SetAggregateOption(const FieldDescriptor* option_field,
                            UnknownFieldSet* unknown_fields);

    
    
    void SetInt32(int number, int32 value, FieldDescriptor::Type type,
                  UnknownFieldSet* unknown_fields);
    void SetInt64(int number, int64 value, FieldDescriptor::Type type,
                  UnknownFieldSet* unknown_fields);
    void SetUInt32(int number, uint32 value, FieldDescriptor::Type type,
                   UnknownFieldSet* unknown_fields);
    void SetUInt64(int number, uint64 value, FieldDescriptor::Type type,
                   UnknownFieldSet* unknown_fields);

    
    
    bool AddOptionError(DescriptorPool::ErrorCollector::ErrorLocation location,
                        const string& msg) {
      builder_->AddError(options_to_interpret_->element_name,
                         *uninterpreted_option_, location, msg);
      return false;
    }

    
    
    bool AddNameError(const string& msg) {
      return AddOptionError(DescriptorPool::ErrorCollector::OPTION_NAME, msg);
    }

    
    
    bool AddValueError(const string& msg) {
      return AddOptionError(DescriptorPool::ErrorCollector::OPTION_VALUE, msg);
    }

    
    
    DescriptorBuilder* builder_;

    
    
    const OptionsToInterpret* options_to_interpret_;

    
    
    
    
    const UninterpretedOption* uninterpreted_option_;

    
    
    DynamicMessageFactory dynamic_factory_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(OptionInterpreter);
  };

  
  
  
  
  
  
  
  friend class OptionInterpreter;
  friend class OptionInterpreter::AggregateOptionFinder;

  static inline bool get_allow_unknown(const DescriptorPool* pool) {
    return pool->allow_unknown_;
  }
  static inline bool get_enforce_weak(const DescriptorPool* pool) {
    return pool->enforce_weak_;
  }
  static inline bool get_is_placeholder(const Descriptor* descriptor) {
    return descriptor->is_placeholder_;
  }
  static inline void assert_mutex_held(const DescriptorPool* pool) {
    if (pool->mutex_ != NULL) {
      pool->mutex_->AssertHeld();
    }
  }

  
  
  
  
  
  
  void ValidateFileOptions(FileDescriptor* file,
                           const FileDescriptorProto& proto);
  void ValidateMessageOptions(Descriptor* message,
                              const DescriptorProto& proto);
  void ValidateFieldOptions(FieldDescriptor* field,
                            const FieldDescriptorProto& proto);
  void ValidateEnumOptions(EnumDescriptor* enm,
                           const EnumDescriptorProto& proto);
  void ValidateEnumValueOptions(EnumValueDescriptor* enum_value,
                                const EnumValueDescriptorProto& proto);
  void ValidateServiceOptions(ServiceDescriptor* service,
                              const ServiceDescriptorProto& proto);
  void ValidateMethodOptions(MethodDescriptor* method,
                             const MethodDescriptorProto& proto);

  void ValidateMapKey(FieldDescriptor* field,
                      const FieldDescriptorProto& proto);

};

const FileDescriptor* DescriptorPool::BuildFile(
    const FileDescriptorProto& proto) {
  GOOGLE_CHECK(fallback_database_ == NULL)
    << "Cannot call BuildFile on a DescriptorPool that uses a "
       "DescriptorDatabase.  You must instead find a way to get your file "
       "into the underlying database.";
  GOOGLE_CHECK(mutex_ == NULL);   
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  return DescriptorBuilder(this, tables_.get(), NULL).BuildFile(proto);
}

const FileDescriptor* DescriptorPool::BuildFileCollectingErrors(
    const FileDescriptorProto& proto,
    ErrorCollector* error_collector) {
  GOOGLE_CHECK(fallback_database_ == NULL)
    << "Cannot call BuildFile on a DescriptorPool that uses a "
       "DescriptorDatabase.  You must instead find a way to get your file "
       "into the underlying database.";
  GOOGLE_CHECK(mutex_ == NULL);   
  tables_->known_bad_symbols_.clear();
  tables_->known_bad_files_.clear();
  return DescriptorBuilder(this, tables_.get(),
                           error_collector).BuildFile(proto);
}

const FileDescriptor* DescriptorPool::BuildFileFromDatabase(
    const FileDescriptorProto& proto) const {
  mutex_->AssertHeld();
  if (tables_->known_bad_files_.count(proto.name()) > 0) {
    return NULL;
  }
  const FileDescriptor* result =
      DescriptorBuilder(this, tables_.get(),
                        default_error_collector_).BuildFile(proto);
  if (result == NULL) {
    tables_->known_bad_files_.insert(proto.name());
  }
  return result;
}

DescriptorBuilder::DescriptorBuilder(
    const DescriptorPool* pool,
    DescriptorPool::Tables* tables,
    DescriptorPool::ErrorCollector* error_collector)
  : pool_(pool),
    tables_(tables),
    error_collector_(error_collector),
    had_errors_(false),
    possible_undeclared_dependency_(NULL),
    undefine_resolved_name_("") {}

DescriptorBuilder::~DescriptorBuilder() {}

void DescriptorBuilder::AddError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& error) {
  if (error_collector_ == NULL) {
    if (!had_errors_) {
      GOOGLE_LOG(ERROR) << "Invalid proto descriptor for file \"" << filename_
                 << "\":";
    }
    GOOGLE_LOG(ERROR) << "  " << element_name << ": " << error;
  } else {
    error_collector_->AddError(filename_, element_name,
                               &descriptor, location, error);
  }
  had_errors_ = true;
}

void DescriptorBuilder::AddError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const char* error) {
  AddError(element_name, descriptor, location, string(error));
}

void DescriptorBuilder::AddNotDefinedError(
    const string& element_name,
    const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& undefined_symbol) {
  if (possible_undeclared_dependency_ == NULL &&
      undefine_resolved_name_.empty()) {
    AddError(element_name, descriptor, location,
             "\"" + undefined_symbol + "\" is not defined.");
  } else {
    if (possible_undeclared_dependency_ != NULL) {
      AddError(element_name, descriptor, location,
               "\"" + possible_undeclared_dependency_name_ +
               "\" seems to be defined in \"" +
               possible_undeclared_dependency_->name() + "\", which is not "
               "imported by \"" + filename_ + "\".  To use it here, please "
               "add the necessary import.");
    }
    if (!undefine_resolved_name_.empty()) {
      AddError(element_name, descriptor, location,
               "\"" + undefined_symbol + "\" is resolved to \"" +
               undefine_resolved_name_ + "\", which is not defined. "
               "The innermost scope is searched first in name resolution. "
               "Consider using a leading '.'(i.e., \"."
               + undefined_symbol +
               "\") to start from the outermost scope.");
    }
  }
}

void DescriptorBuilder::AddWarning(
    const string& element_name, const Message& descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    const string& error) {
  if (error_collector_ == NULL) {
    GOOGLE_LOG(WARNING) << filename_ << " " << element_name << ": " << error;
  } else {
    error_collector_->AddWarning(filename_, element_name, &descriptor, location,
                                 error);
  }
}

bool DescriptorBuilder::IsInPackage(const FileDescriptor* file,
                                    const string& package_name) {
  return HasPrefixString(file->package(), package_name) &&
           (file->package().size() == package_name.size() ||
            file->package()[package_name.size()] == '.');
}

void DescriptorBuilder::RecordPublicDependencies(const FileDescriptor* file) {
  if (file == NULL || !dependencies_.insert(file).second) return;
  for (int i = 0; file != NULL && i < file->public_dependency_count(); i++) {
    RecordPublicDependencies(file->public_dependency(i));
  }
}

Symbol DescriptorBuilder::FindSymbolNotEnforcingDepsHelper(
    const DescriptorPool* pool, const string& name) {
  
  
  MutexLockMaybe lock((pool == pool_) ? NULL : pool->mutex_);

  Symbol result = pool->tables_->FindSymbol(name);
  if (result.IsNull() && pool->underlay_ != NULL) {
    
    result = FindSymbolNotEnforcingDepsHelper(pool->underlay_, name);
  }

  if (result.IsNull()) {
    
    
    
    
    
    if (pool->TryFindSymbolInFallbackDatabase(name)) {
      result = pool->tables_->FindSymbol(name);
    }
  }

  return result;
}

Symbol DescriptorBuilder::FindSymbolNotEnforcingDeps(const string& name) {
  return FindSymbolNotEnforcingDepsHelper(pool_, name);
}

Symbol DescriptorBuilder::FindSymbol(const string& name) {
  Symbol result = FindSymbolNotEnforcingDeps(name);

  if (result.IsNull()) return result;

  if (!pool_->enforce_dependencies_) {
    
    return result;
  }

  
  
  const FileDescriptor* file = result.GetFile();
  if (file == file_ || dependencies_.count(file) > 0) {
    unused_dependency_.erase(file);
    return result;
  }

  if (result.type == Symbol::PACKAGE) {
    
    
    
    
    
    
    
    if (IsInPackage(file_, name)) return result;
    for (set<const FileDescriptor*>::const_iterator it = dependencies_.begin();
         it != dependencies_.end(); ++it) {
      
      if (*it != NULL && IsInPackage(*it, name)) return result;
    }
  }

  possible_undeclared_dependency_ = file;
  possible_undeclared_dependency_name_ = name;
  return kNullSymbol;
}

Symbol DescriptorBuilder::LookupSymbolNoPlaceholder(
    const string& name, const string& relative_to, ResolveMode resolve_mode) {
  possible_undeclared_dependency_ = NULL;
  undefine_resolved_name_.clear();

  if (name.size() > 0 && name[0] == '.') {
    
    return FindSymbol(name.substr(1));
  }

  
  
  
  
  
  
  
  
  
  
  
  string::size_type name_dot_pos = name.find_first_of('.');
  string first_part_of_name;
  if (name_dot_pos == string::npos) {
    first_part_of_name = name;
  } else {
    first_part_of_name = name.substr(0, name_dot_pos);
  }

  string scope_to_try(relative_to);

  while (true) {
    
    string::size_type dot_pos = scope_to_try.find_last_of('.');
    if (dot_pos == string::npos) {
      return FindSymbol(name);
    } else {
      scope_to_try.erase(dot_pos);
    }

    
    string::size_type old_size = scope_to_try.size();
    scope_to_try.append(1, '.');
    scope_to_try.append(first_part_of_name);
    Symbol result = FindSymbol(scope_to_try);
    if (!result.IsNull()) {
      if (first_part_of_name.size() < name.size()) {
        
        
        if (result.IsAggregate()) {
          scope_to_try.append(name, first_part_of_name.size(),
                              name.size() - first_part_of_name.size());
          result = FindSymbol(scope_to_try);
          if (result.IsNull()) {
            undefine_resolved_name_ = scope_to_try;
          }
          return result;
        } else {
          
        }
      } else {
        if (resolve_mode == LOOKUP_TYPES && !result.IsType()) {
          
        } else {
          return result;
        }
      }
    }

    
    scope_to_try.erase(old_size);
  }
}

Symbol DescriptorBuilder::LookupSymbol(
    const string& name, const string& relative_to,
    PlaceholderType placeholder_type, ResolveMode resolve_mode) {
  Symbol result = LookupSymbolNoPlaceholder(
      name, relative_to, resolve_mode);
  if (result.IsNull() && pool_->allow_unknown_) {
    
    
    result = NewPlaceholder(name, placeholder_type);
  }
  return result;
}

Symbol DescriptorBuilder::NewPlaceholder(const string& name,
                                         PlaceholderType placeholder_type) {
  
  const string* placeholder_full_name;
  const string* placeholder_name;
  const string* placeholder_package;

  if (!ValidateQualifiedName(name)) return kNullSymbol;
  if (name[0] == '.') {
    
    placeholder_full_name = tables_->AllocateString(name.substr(1));
  } else {
    placeholder_full_name = tables_->AllocateString(name);
  }

  string::size_type dotpos = placeholder_full_name->find_last_of('.');
  if (dotpos != string::npos) {
    placeholder_package = tables_->AllocateString(
      placeholder_full_name->substr(0, dotpos));
    placeholder_name = tables_->AllocateString(
      placeholder_full_name->substr(dotpos + 1));
  } else {
    placeholder_package = &internal::GetEmptyString();
    placeholder_name = placeholder_full_name;
  }

  
  FileDescriptor* placeholder_file = tables_->Allocate<FileDescriptor>();
  memset(placeholder_file, 0, sizeof(*placeholder_file));

  placeholder_file->source_code_info_ = &SourceCodeInfo::default_instance();

  placeholder_file->name_ =
    tables_->AllocateString(*placeholder_full_name + ".placeholder.proto");
  placeholder_file->package_ = placeholder_package;
  placeholder_file->pool_ = pool_;
  placeholder_file->options_ = &FileOptions::default_instance();
  placeholder_file->tables_ = &FileDescriptorTables::kEmpty;
  placeholder_file->is_placeholder_ = true;
  

  if (placeholder_type == PLACEHOLDER_ENUM) {
    placeholder_file->enum_type_count_ = 1;
    placeholder_file->enum_types_ =
      tables_->AllocateArray<EnumDescriptor>(1);

    EnumDescriptor* placeholder_enum = &placeholder_file->enum_types_[0];
    memset(placeholder_enum, 0, sizeof(*placeholder_enum));

    placeholder_enum->full_name_ = placeholder_full_name;
    placeholder_enum->name_ = placeholder_name;
    placeholder_enum->file_ = placeholder_file;
    placeholder_enum->options_ = &EnumOptions::default_instance();
    placeholder_enum->is_placeholder_ = true;
    placeholder_enum->is_unqualified_placeholder_ = (name[0] != '.');

    
    placeholder_enum->value_count_ = 1;
    placeholder_enum->values_ = tables_->AllocateArray<EnumValueDescriptor>(1);

    EnumValueDescriptor* placeholder_value = &placeholder_enum->values_[0];
    memset(placeholder_value, 0, sizeof(*placeholder_value));

    placeholder_value->name_ = tables_->AllocateString("PLACEHOLDER_VALUE");
    
    placeholder_value->full_name_ =
      placeholder_package->empty() ? placeholder_value->name_ :
        tables_->AllocateString(*placeholder_package + ".PLACEHOLDER_VALUE");

    placeholder_value->number_ = 0;
    placeholder_value->type_ = placeholder_enum;
    placeholder_value->options_ = &EnumValueOptions::default_instance();

    return Symbol(placeholder_enum);
  } else {
    placeholder_file->message_type_count_ = 1;
    placeholder_file->message_types_ =
      tables_->AllocateArray<Descriptor>(1);

    Descriptor* placeholder_message = &placeholder_file->message_types_[0];
    memset(placeholder_message, 0, sizeof(*placeholder_message));

    placeholder_message->full_name_ = placeholder_full_name;
    placeholder_message->name_ = placeholder_name;
    placeholder_message->file_ = placeholder_file;
    placeholder_message->options_ = &MessageOptions::default_instance();
    placeholder_message->is_placeholder_ = true;
    placeholder_message->is_unqualified_placeholder_ = (name[0] != '.');

    if (placeholder_type == PLACEHOLDER_EXTENDABLE_MESSAGE) {
      placeholder_message->extension_range_count_ = 1;
      placeholder_message->extension_ranges_ =
        tables_->AllocateArray<Descriptor::ExtensionRange>(1);
      placeholder_message->extension_ranges_->start = 1;
      
      placeholder_message->extension_ranges_->end =
        FieldDescriptor::kMaxNumber + 1;
    }

    return Symbol(placeholder_message);
  }
}

const FileDescriptor* DescriptorBuilder::NewPlaceholderFile(
    const string& name) {
  FileDescriptor* placeholder = tables_->Allocate<FileDescriptor>();
  memset(placeholder, 0, sizeof(*placeholder));

  placeholder->name_ = tables_->AllocateString(name);
  placeholder->package_ = &internal::GetEmptyString();
  placeholder->pool_ = pool_;
  placeholder->options_ = &FileOptions::default_instance();
  placeholder->tables_ = &FileDescriptorTables::kEmpty;
  placeholder->is_placeholder_ = true;
  

  return placeholder;
}

bool DescriptorBuilder::AddSymbol(
    const string& full_name, const void* parent, const string& name,
    const Message& proto, Symbol symbol) {
  
  
  if (parent == NULL) parent = file_;

  if (tables_->AddSymbol(full_name, symbol)) {
    if (!file_tables_->AddAliasUnderParent(parent, name, symbol)) {
      GOOGLE_LOG(DFATAL) << "\"" << full_name << "\" not previously defined in "
                     "symbols_by_name_, but was defined in symbols_by_parent_; "
                     "this shouldn't be possible.";
      return false;
    }
    return true;
  } else {
    const FileDescriptor* other_file = tables_->FindSymbol(full_name).GetFile();
    if (other_file == file_) {
      string::size_type dot_pos = full_name.find_last_of('.');
      if (dot_pos == string::npos) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + full_name + "\" is already defined.");
      } else {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + full_name.substr(dot_pos + 1) +
                 "\" is already defined in \"" +
                 full_name.substr(0, dot_pos) + "\".");
      }
    } else {
      
      AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
               "\"" + full_name + "\" is already defined in file \"" +
               other_file->name() + "\".");
    }
    return false;
  }
}

void DescriptorBuilder::AddPackage(
    const string& name, const Message& proto, const FileDescriptor* file) {
  if (tables_->AddSymbol(name, Symbol(file))) {
    
    string::size_type dot_pos = name.find_last_of('.');
    if (dot_pos == string::npos) {
      
      ValidateSymbolName(name, name, proto);
    } else {
      
      string* parent_name = tables_->AllocateString(name.substr(0, dot_pos));
      AddPackage(*parent_name, proto, file);
      ValidateSymbolName(name.substr(dot_pos + 1), name, proto);
    }
  } else {
    Symbol existing_symbol = tables_->FindSymbol(name);
    
    if (existing_symbol.type != Symbol::PACKAGE) {
      
      AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
               "\"" + name + "\" is already defined (as something other than "
               "a package) in file \"" + existing_symbol.GetFile()->name() +
               "\".");
    }
  }
}

void DescriptorBuilder::ValidateSymbolName(
    const string& name, const string& full_name, const Message& proto) {
  if (name.empty()) {
    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
             "Missing name.");
  } else {
    for (int i = 0; i < name.size(); i++) {
      
      if ((name[i] < 'a' || 'z' < name[i]) &&
          (name[i] < 'A' || 'Z' < name[i]) &&
          (name[i] < '0' || '9' < name[i]) &&
          (name[i] != '_')) {
        AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                 "\"" + name + "\" is not a valid identifier.");
      }
    }
  }
}

bool DescriptorBuilder::ValidateQualifiedName(const string& name) {
  bool last_was_period = false;

  for (int i = 0; i < name.size(); i++) {
    
    if (('a' <= name[i] && name[i] <= 'z') ||
        ('A' <= name[i] && name[i] <= 'Z') ||
        ('0' <= name[i] && name[i] <= '9') ||
        (name[i] == '_')) {
      last_was_period = false;
    } else if (name[i] == '.') {
      if (last_was_period) return false;
      last_was_period = true;
    } else {
      return false;
    }
  }

  return !name.empty() && !last_was_period;
}





template<class DescriptorT> void DescriptorBuilder::AllocateOptions(
    const typename DescriptorT::OptionsType& orig_options,
    DescriptorT* descriptor) {
  AllocateOptionsImpl(descriptor->full_name(), descriptor->full_name(),
                      orig_options, descriptor);
}


void DescriptorBuilder::AllocateOptions(const FileOptions& orig_options,
                                        FileDescriptor* descriptor) {
  
  AllocateOptionsImpl(descriptor->package() + ".dummy", descriptor->name(),
                      orig_options, descriptor);
}

template<class DescriptorT> void DescriptorBuilder::AllocateOptionsImpl(
    const string& name_scope,
    const string& element_name,
    const typename DescriptorT::OptionsType& orig_options,
    DescriptorT* descriptor) {
  
  
  
  
  typename DescriptorT::OptionsType* const dummy = NULL;
  typename DescriptorT::OptionsType* options = tables_->AllocateMessage(dummy);
  
  
  
  
  options->ParseFromString(orig_options.SerializeAsString());
  descriptor->options_ = options;

  
  
  
  
  
  
  
  if (options->uninterpreted_option_size() > 0) {
    options_to_interpret_.push_back(
        OptionsToInterpret(name_scope, element_name, &orig_options, options));
  }
}




#define BUILD_ARRAY(INPUT, OUTPUT, NAME, METHOD, PARENT)             \
  OUTPUT->NAME##_count_ = INPUT.NAME##_size();                       \
  AllocateArray(INPUT.NAME##_size(), &OUTPUT->NAME##s_);             \
  for (int i = 0; i < INPUT.NAME##_size(); i++) {                    \
    METHOD(INPUT.NAME(i), PARENT, OUTPUT->NAME##s_ + i);             \
  }

void DescriptorBuilder::AddRecursiveImportError(
    const FileDescriptorProto& proto, int from_here) {
  string error_message("File recursively imports itself: ");
  for (int i = from_here; i < tables_->pending_files_.size(); i++) {
    error_message.append(tables_->pending_files_[i]);
    error_message.append(" -> ");
  }
  error_message.append(proto.name());

  AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
           error_message);
}

void DescriptorBuilder::AddTwiceListedError(const FileDescriptorProto& proto,
                                            int index) {
  AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
           "Import \"" + proto.dependency(index) + "\" was listed twice.");
}

void DescriptorBuilder::AddImportError(const FileDescriptorProto& proto,
                                       int index) {
  string message;
  if (pool_->fallback_database_ == NULL) {
    message = "Import \"" + proto.dependency(index) +
              "\" has not been loaded.";
  } else {
    message = "Import \"" + proto.dependency(index) +
              "\" was not found or had errors.";
  }
  AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER, message);
}

static bool ExistingFileMatchesProto(const FileDescriptor* existing_file,
                                     const FileDescriptorProto& proto) {
  FileDescriptorProto existing_proto;
  existing_file->CopyTo(&existing_proto);
  return existing_proto.SerializeAsString() == proto.SerializeAsString();
}

const FileDescriptor* DescriptorBuilder::BuildFile(
    const FileDescriptorProto& proto) {
  filename_ = proto.name();

  
  
  
  
  
  const FileDescriptor* existing_file = tables_->FindFile(filename_);
  if (existing_file != NULL) {
    
    if (ExistingFileMatchesProto(existing_file, proto)) {
      
      return existing_file;
    }

    
  }

  
  
  
  
  
  
  
  
  
  for (int i = 0; i < tables_->pending_files_.size(); i++) {
    if (tables_->pending_files_[i] == proto.name()) {
      AddRecursiveImportError(proto, i);
      return NULL;
    }
  }

  
  
  
  if (pool_->fallback_database_ != NULL) {
    tables_->pending_files_.push_back(proto.name());
    for (int i = 0; i < proto.dependency_size(); i++) {
      if (tables_->FindFile(proto.dependency(i)) == NULL &&
          (pool_->underlay_ == NULL ||
           pool_->underlay_->FindFileByName(proto.dependency(i)) == NULL)) {
        
        pool_->TryFindFileInFallbackDatabase(proto.dependency(i));
      }
    }
    tables_->pending_files_.pop_back();
  }

  
  tables_->AddCheckpoint();

  FileDescriptor* result = tables_->Allocate<FileDescriptor>();
  file_ = result;

  result->is_placeholder_ = false;
  if (proto.has_source_code_info()) {
    SourceCodeInfo *info = tables_->AllocateMessage<SourceCodeInfo>();
    info->CopyFrom(proto.source_code_info());
    result->source_code_info_ = info;
  } else {
    result->source_code_info_ = &SourceCodeInfo::default_instance();
  }

  file_tables_ = tables_->AllocateFileTables();
  file_->tables_ = file_tables_;

  if (!proto.has_name()) {
    AddError("", proto, DescriptorPool::ErrorCollector::OTHER,
             "Missing field: FileDescriptorProto.name.");
  }

  result->name_ = tables_->AllocateString(proto.name());
  if (proto.has_package()) {
    result->package_ = tables_->AllocateString(proto.package());
  } else {
    
    
    
    
    result->package_ = tables_->AllocateString("");
  }
  result->pool_ = pool_;

  
  if (!tables_->AddFile(result)) {
    AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
             "A file with this name is already in the pool.");
    
    
    tables_->RollbackToLastCheckpoint();
    return NULL;
  }
  if (!result->package().empty()) {
    AddPackage(result->package(), proto, result);
  }

  
  set<string> seen_dependencies;
  result->dependency_count_ = proto.dependency_size();
  result->dependencies_ =
    tables_->AllocateArray<const FileDescriptor*>(proto.dependency_size());
  unused_dependency_.clear();
  set<int> weak_deps;
  for (int i = 0; i < proto.weak_dependency_size(); ++i) {
    weak_deps.insert(proto.weak_dependency(i));
  }
  for (int i = 0; i < proto.dependency_size(); i++) {
    if (!seen_dependencies.insert(proto.dependency(i)).second) {
      AddTwiceListedError(proto, i);
    }

    const FileDescriptor* dependency = tables_->FindFile(proto.dependency(i));
    if (dependency == NULL && pool_->underlay_ != NULL) {
      dependency = pool_->underlay_->FindFileByName(proto.dependency(i));
    }

    if (dependency == NULL) {
      if (pool_->allow_unknown_ ||
          (!pool_->enforce_weak_ && weak_deps.find(i) != weak_deps.end())) {
        dependency = NewPlaceholderFile(proto.dependency(i));
      } else {
        AddImportError(proto, i);
      }
    } else {
      
      
      if (pool_->enforce_dependencies_ &&
          (pool_->unused_import_track_files_.find(proto.name()) !=
           pool_->unused_import_track_files_.end()) &&
          (dependency->public_dependency_count() == 0)) {
        unused_dependency_.insert(dependency);
      }
    }

    result->dependencies_[i] = dependency;
  }

  
  int public_dependency_count = 0;
  result->public_dependencies_ = tables_->AllocateArray<int>(
      proto.public_dependency_size());
  for (int i = 0; i < proto.public_dependency_size(); i++) {
    
    int index = proto.public_dependency(i);
    if (index >= 0 && index < proto.dependency_size()) {
      result->public_dependencies_[public_dependency_count++] = index;
      
      unused_dependency_.erase(result->dependency(index));
    } else {
      AddError(proto.name(), proto,
               DescriptorPool::ErrorCollector::OTHER,
               "Invalid public dependency index.");
    }
  }
  result->public_dependency_count_ = public_dependency_count;

  
  dependencies_.clear();
  for (int i = 0; i < result->dependency_count(); i++) {
    RecordPublicDependencies(result->dependency(i));
  }

  
  int weak_dependency_count = 0;
  result->weak_dependencies_ = tables_->AllocateArray<int>(
      proto.weak_dependency_size());
  for (int i = 0; i < proto.weak_dependency_size(); i++) {
    int index = proto.weak_dependency(i);
    if (index >= 0 && index < proto.dependency_size()) {
      result->weak_dependencies_[weak_dependency_count++] = index;
    } else {
      AddError(proto.name(), proto,
               DescriptorPool::ErrorCollector::OTHER,
               "Invalid weak dependency index.");
    }
  }
  result->weak_dependency_count_ = weak_dependency_count;

  
  BUILD_ARRAY(proto, result, message_type, BuildMessage  , NULL);
  BUILD_ARRAY(proto, result, enum_type   , BuildEnum     , NULL);
  BUILD_ARRAY(proto, result, service     , BuildService  , NULL);
  BUILD_ARRAY(proto, result, extension   , BuildExtension, NULL);

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  

  
  CrossLinkFile(result, proto);

  
  
  
  if (!had_errors_) {
    OptionInterpreter option_interpreter(this);
    for (vector<OptionsToInterpret>::iterator iter =
             options_to_interpret_.begin();
         iter != options_to_interpret_.end(); ++iter) {
      option_interpreter.InterpretOptions(&(*iter));
    }
    options_to_interpret_.clear();
  }

  
  if (!had_errors_) {
    ValidateFileOptions(result, proto);
  }


  if (!unused_dependency_.empty()) {
    LogUnusedDependency(result);
  }

  if (had_errors_) {
    tables_->RollbackToLastCheckpoint();
    return NULL;
  } else {
    tables_->ClearLastCheckpoint();
    return result;
  }
}

void DescriptorBuilder::BuildMessage(const DescriptorProto& proto,
                                     const Descriptor* parent,
                                     Descriptor* result) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_            = tables_->AllocateString(proto.name());
  result->full_name_       = full_name;
  result->file_            = file_;
  result->containing_type_ = parent;
  result->is_placeholder_  = false;
  result->is_unqualified_placeholder_ = false;

  
  BUILD_ARRAY(proto, result, oneof_decl     , BuildOneof         , result);
  BUILD_ARRAY(proto, result, field          , BuildField         , result);
  BUILD_ARRAY(proto, result, nested_type    , BuildMessage       , result);
  BUILD_ARRAY(proto, result, enum_type      , BuildEnum          , result);
  BUILD_ARRAY(proto, result, extension_range, BuildExtensionRange, result);
  BUILD_ARRAY(proto, result, extension      , BuildExtension     , result);

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));

  
  for (int i = 0; i < result->field_count(); i++) {
    const FieldDescriptor* field = result->field(i);
    for (int j = 0; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range = result->extension_range(j);
      if (range->start <= field->number() && field->number() < range->end) {
        AddError(field->full_name(), proto.extension_range(j),
                 DescriptorPool::ErrorCollector::NUMBER,
                 strings::Substitute(
                   "Extension range $0 to $1 includes field \"$2\" ($3).",
                   range->start, range->end - 1,
                   field->name(), field->number()));
      }
    }
  }

  
  for (int i = 0; i < result->extension_range_count(); i++) {
    const Descriptor::ExtensionRange* range1 = result->extension_range(i);
    for (int j = i + 1; j < result->extension_range_count(); j++) {
      const Descriptor::ExtensionRange* range2 = result->extension_range(j);
      if (range1->end > range2->start && range2->end > range1->start) {
        AddError(result->full_name(), proto.extension_range(j),
                 DescriptorPool::ErrorCollector::NUMBER,
                 strings::Substitute("Extension range $0 to $1 overlaps with "
                                     "already-defined range $2 to $3.",
                                     range2->start, range2->end - 1,
                                     range1->start, range1->end - 1));
      }
    }
  }
}

void DescriptorBuilder::BuildFieldOrExtension(const FieldDescriptorProto& proto,
                                              const Descriptor* parent,
                                              FieldDescriptor* result,
                                              bool is_extension) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_         = tables_->AllocateString(proto.name());
  result->full_name_    = full_name;
  result->file_         = file_;
  result->number_       = proto.number();
  result->is_extension_ = is_extension;

  
  
  
  string lowercase_name(proto.name());
  LowerString(&lowercase_name);
  if (lowercase_name == proto.name()) {
    result->lowercase_name_ = result->name_;
  } else {
    result->lowercase_name_ = tables_->AllocateString(lowercase_name);
  }

  
  
  
  result->camelcase_name_ = tables_->AllocateString(ToCamelCase(proto.name()));

  
  
  result->type_  = static_cast<FieldDescriptor::Type>(
                     implicit_cast<int>(proto.type()));
  result->label_ = static_cast<FieldDescriptor::Label>(
                     implicit_cast<int>(proto.label()));

  
  if (result->is_extension_ &&
      result->label_ == FieldDescriptor::LABEL_REQUIRED) {
    AddError(result->full_name(), proto,
             
             
             
             
             
             DescriptorPool::ErrorCollector::TYPE,
             "Message extensions cannot have required fields.");
  }

  
  result->containing_type_ = NULL;
  result->extension_scope_ = NULL;
  result->experimental_map_key_ = NULL;
  result->message_type_ = NULL;
  result->enum_type_ = NULL;

  result->has_default_value_ = proto.has_default_value();
  if (proto.has_default_value() && result->is_repeated()) {
    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::DEFAULT_VALUE,
             "Repeated fields can't have default values.");
  }

  if (proto.has_type()) {
    if (proto.has_default_value()) {
      char* end_pos = NULL;
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_ =
            strtol(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          result->default_value_int64_ =
            strto64(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_ =
            strtoul(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          result->default_value_uint64_ =
            strtou64(proto.default_value().c_str(), &end_pos, 0);
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          if (proto.default_value() == "inf") {
            result->default_value_float_ = numeric_limits<float>::infinity();
          } else if (proto.default_value() == "-inf") {
            result->default_value_float_ = -numeric_limits<float>::infinity();
          } else if (proto.default_value() == "nan") {
            result->default_value_float_ = numeric_limits<float>::quiet_NaN();
          } else  {
            result->default_value_float_ =
              io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
          }
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          if (proto.default_value() == "inf") {
            result->default_value_double_ = numeric_limits<double>::infinity();
          } else if (proto.default_value() == "-inf") {
            result->default_value_double_ = -numeric_limits<double>::infinity();
          } else if (proto.default_value() == "nan") {
            result->default_value_double_ = numeric_limits<double>::quiet_NaN();
          } else  {
            result->default_value_double_ =
                io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
          }
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          if (proto.default_value() == "true") {
            result->default_value_bool_ = true;
          } else if (proto.default_value() == "false") {
            result->default_value_bool_ = false;
          } else {
            AddError(result->full_name(), proto,
                     DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                     "Boolean default must be true or false.");
          }
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          
          result->default_value_enum_ = NULL;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          if (result->type() == FieldDescriptor::TYPE_BYTES) {
            result->default_value_string_ = tables_->AllocateString(
              UnescapeCEscapeString(proto.default_value()));
          } else {
            result->default_value_string_ =
                tables_->AllocateString(proto.default_value());
          }
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Messages can't have default values.");
          result->has_default_value_ = false;
          break;
      }

      if (end_pos != NULL) {
        
        
        
        if (proto.default_value().empty() || *end_pos != '\0') {
          AddError(result->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Couldn't parse default value \"" + proto.default_value() +
                   "\".");
        }
      }
    } else {
      
      switch (result->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          result->default_value_int32_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_INT64:
          result->default_value_int64_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT32:
          result->default_value_uint32_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_UINT64:
          result->default_value_uint64_ = 0;
          break;
        case FieldDescriptor::CPPTYPE_FLOAT:
          result->default_value_float_ = 0.0f;
          break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
          result->default_value_double_ = 0.0;
          break;
        case FieldDescriptor::CPPTYPE_BOOL:
          result->default_value_bool_ = false;
          break;
        case FieldDescriptor::CPPTYPE_ENUM:
          
          result->default_value_enum_ = NULL;
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          result->default_value_string_ = &internal::GetEmptyString();
          break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
          break;
      }
    }
  }

  if (result->number() <= 0) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             "Field numbers must be positive integers.");
  } else if (!is_extension && result->number() > FieldDescriptor::kMaxNumber) {
    
    
    
    
    
    
    
    
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             strings::Substitute("Field numbers cannot be greater than $0.",
                                 FieldDescriptor::kMaxNumber));
  } else if (result->number() >= FieldDescriptor::kFirstReservedNumber &&
             result->number() <= FieldDescriptor::kLastReservedNumber) {
    AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
             strings::Substitute(
               "Field numbers $0 through $1 are reserved for the protocol "
               "buffer library implementation.",
               FieldDescriptor::kFirstReservedNumber,
               FieldDescriptor::kLastReservedNumber));
  }

  if (is_extension) {
    if (!proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee not set for extension field.");
    }

    result->extension_scope_ = parent;

    if (proto.has_oneof_index()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::OTHER,
               "FieldDescriptorProto.oneof_index should not be set for "
               "extensions.");
    }

    
    result->containing_oneof_ = NULL;
  } else {
    if (proto.has_extendee()) {
      AddError(result->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "FieldDescriptorProto.extendee set for non-extension field.");
    }

    result->containing_type_ = parent;

    if (proto.has_oneof_index()) {
      if (proto.oneof_index() < 0 ||
          proto.oneof_index() >= parent->oneof_decl_count()) {
        AddError(result->full_name(), proto,
                 DescriptorPool::ErrorCollector::OTHER,
                 strings::Substitute("FieldDescriptorProto.oneof_index $0 is "
                                     "out of range for type \"$1\".",
                                     proto.oneof_index(),
                                     parent->name()));
        result->containing_oneof_ = NULL;
      } else {
        result->containing_oneof_ = parent->oneof_decl(proto.oneof_index());
      }
    } else {
      result->containing_oneof_ = NULL;
    }
  }

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildExtensionRange(
    const DescriptorProto::ExtensionRange& proto,
    const Descriptor* parent,
    Descriptor::ExtensionRange* result) {
  result->start = proto.start();
  result->end = proto.end();
  if (result->start <= 0) {
    AddError(parent->full_name(), proto,
             DescriptorPool::ErrorCollector::NUMBER,
             "Extension numbers must be positive integers.");
  }

  
  
  
  

  if (result->start >= result->end) {
    AddError(parent->full_name(), proto,
             DescriptorPool::ErrorCollector::NUMBER,
             "Extension range end number must be greater than start number.");
  }
}

void DescriptorBuilder::BuildOneof(const OneofDescriptorProto& proto,
                                   Descriptor* parent,
                                   OneofDescriptor* result) {
  string* full_name = tables_->AllocateString(parent->full_name());
  full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_ = tables_->AllocateString(proto.name());
  result->full_name_ = full_name;

  result->containing_type_ = parent;

  
  result->field_count_ = 0;
  result->fields_ = NULL;

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildEnum(const EnumDescriptorProto& proto,
                                  const Descriptor* parent,
                                  EnumDescriptor* result) {
  const string& scope = (parent == NULL) ?
    file_->package() : parent->full_name();
  string* full_name = tables_->AllocateString(scope);
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_            = tables_->AllocateString(proto.name());
  result->full_name_       = full_name;
  result->file_            = file_;
  result->containing_type_ = parent;
  result->is_placeholder_  = false;
  result->is_unqualified_placeholder_ = false;

  if (proto.value_size() == 0) {
    
    
    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::NAME,
             "Enums must contain at least one value.");
  }

  BUILD_ARRAY(proto, result, value, BuildEnumValue, result);

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildEnumValue(const EnumValueDescriptorProto& proto,
                                       const EnumDescriptor* parent,
                                       EnumValueDescriptor* result) {
  result->name_   = tables_->AllocateString(proto.name());
  result->number_ = proto.number();
  result->type_   = parent;

  
  
  string* full_name = tables_->AllocateString(*parent->full_name_);
  full_name->resize(full_name->size() - parent->name_->size());
  full_name->append(*result->name_);
  result->full_name_ = full_name;

  ValidateSymbolName(proto.name(), *full_name, proto);

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  
  
  
  bool added_to_outer_scope =
    AddSymbol(result->full_name(), parent->containing_type(), result->name(),
              proto, Symbol(result));

  
  
  
  
  bool added_to_inner_scope =
    file_tables_->AddAliasUnderParent(parent, result->name(), Symbol(result));

  if (added_to_inner_scope && !added_to_outer_scope) {
    
    
    
    string outer_scope;
    if (parent->containing_type() == NULL) {
      outer_scope = file_->package();
    } else {
      outer_scope = parent->containing_type()->full_name();
    }

    if (outer_scope.empty()) {
      outer_scope = "the global scope";
    } else {
      outer_scope = "\"" + outer_scope + "\"";
    }

    AddError(result->full_name(), proto,
             DescriptorPool::ErrorCollector::NAME,
             "Note that enum values use C++ scoping rules, meaning that "
             "enum values are siblings of their type, not children of it.  "
             "Therefore, \"" + result->name() + "\" must be unique within "
             + outer_scope + ", not just within \"" + parent->name() + "\".");
  }

  
  
  
  file_tables_->AddEnumValueByNumber(result);
}

void DescriptorBuilder::BuildService(const ServiceDescriptorProto& proto,
                                     const void* ,
                                     ServiceDescriptor* result) {
  string* full_name = tables_->AllocateString(file_->package());
  if (!full_name->empty()) full_name->append(1, '.');
  full_name->append(proto.name());

  ValidateSymbolName(proto.name(), *full_name, proto);

  result->name_      = tables_->AllocateString(proto.name());
  result->full_name_ = full_name;
  result->file_      = file_;

  BUILD_ARRAY(proto, result, method, BuildMethod, result);

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  AddSymbol(result->full_name(), NULL, result->name(),
            proto, Symbol(result));
}

void DescriptorBuilder::BuildMethod(const MethodDescriptorProto& proto,
                                    const ServiceDescriptor* parent,
                                    MethodDescriptor* result) {
  result->name_    = tables_->AllocateString(proto.name());
  result->service_ = parent;

  string* full_name = tables_->AllocateString(parent->full_name());
  full_name->append(1, '.');
  full_name->append(*result->name_);
  result->full_name_ = full_name;

  ValidateSymbolName(proto.name(), *full_name, proto);

  
  result->input_type_ = NULL;
  result->output_type_ = NULL;

  
  if (!proto.has_options()) {
    result->options_ = NULL;  
  } else {
    AllocateOptions(proto.options(), result);
  }

  AddSymbol(result->full_name(), parent, result->name(),
            proto, Symbol(result));
}

#undef BUILD_ARRAY



void DescriptorBuilder::CrossLinkFile(
    FileDescriptor* file, const FileDescriptorProto& proto) {
  if (file->options_ == NULL) {
    file->options_ = &FileOptions::default_instance();
  }

  for (int i = 0; i < file->message_type_count(); i++) {
    CrossLinkMessage(&file->message_types_[i], proto.message_type(i));
  }

  for (int i = 0; i < file->extension_count(); i++) {
    CrossLinkField(&file->extensions_[i], proto.extension(i));
  }

  for (int i = 0; i < file->enum_type_count(); i++) {
    CrossLinkEnum(&file->enum_types_[i], proto.enum_type(i));
  }

  for (int i = 0; i < file->service_count(); i++) {
    CrossLinkService(&file->services_[i], proto.service(i));
  }
}

void DescriptorBuilder::CrossLinkMessage(
    Descriptor* message, const DescriptorProto& proto) {
  if (message->options_ == NULL) {
    message->options_ = &MessageOptions::default_instance();
  }

  for (int i = 0; i < message->nested_type_count(); i++) {
    CrossLinkMessage(&message->nested_types_[i], proto.nested_type(i));
  }

  for (int i = 0; i < message->enum_type_count(); i++) {
    CrossLinkEnum(&message->enum_types_[i], proto.enum_type(i));
  }

  for (int i = 0; i < message->field_count(); i++) {
    CrossLinkField(&message->fields_[i], proto.field(i));
  }

  for (int i = 0; i < message->extension_count(); i++) {
    CrossLinkField(&message->extensions_[i], proto.extension(i));
  }

  

  
  for (int i = 0; i < message->field_count(); i++) {
    const OneofDescriptor* oneof_decl = message->field(i)->containing_oneof();
    if (oneof_decl != NULL) {
      
      
      ++message->oneof_decls_[oneof_decl->index()].field_count_;
    }
  }

  
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    OneofDescriptor* oneof_decl = &message->oneof_decls_[i];

    if (oneof_decl->field_count() == 0) {
      AddError(message->full_name() + "." + oneof_decl->name(),
               proto.oneof_decl(i),
               DescriptorPool::ErrorCollector::NAME,
               "Oneof must have at least one field.");
    }

    oneof_decl->fields_ =
      tables_->AllocateArray<const FieldDescriptor*>(oneof_decl->field_count_);
    oneof_decl->field_count_ = 0;
  }

  
  for (int i = 0; i < message->field_count(); i++) {
    const OneofDescriptor* oneof_decl = message->field(i)->containing_oneof();
    if (oneof_decl != NULL) {
      OneofDescriptor* mutable_oneof_decl =
          &message->oneof_decls_[oneof_decl->index()];
      message->fields_[i].index_in_oneof_ = mutable_oneof_decl->field_count_;
      mutable_oneof_decl->fields_[mutable_oneof_decl->field_count_++] =
          message->field(i);
    }
  }
}

void DescriptorBuilder::CrossLinkField(
    FieldDescriptor* field, const FieldDescriptorProto& proto) {
  if (field->options_ == NULL) {
    field->options_ = &FieldOptions::default_instance();
  }

  if (proto.has_extendee()) {
    Symbol extendee = LookupSymbol(proto.extendee(), field->full_name(),
                                   PLACEHOLDER_EXTENDABLE_MESSAGE);
    if (extendee.IsNull()) {
      AddNotDefinedError(field->full_name(), proto,
                         DescriptorPool::ErrorCollector::EXTENDEE,
                         proto.extendee());
      return;
    } else if (extendee.type != Symbol::MESSAGE) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::EXTENDEE,
               "\"" + proto.extendee() + "\" is not a message type.");
      return;
    }
    field->containing_type_ = extendee.descriptor;

    const Descriptor::ExtensionRange* extension_range = field->containing_type()
        ->FindExtensionRangeContainingNumber(field->number());

    if (extension_range == NULL) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("\"$0\" does not declare $1 as an "
                                   "extension number.",
                                   field->containing_type()->full_name(),
                                   field->number()));
    }
  }

  if (field->containing_oneof() != NULL) {
    if (field->label() != FieldDescriptor::LABEL_OPTIONAL) {
      
      
      
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME,
               "Fields of oneofs must themselves have label LABEL_OPTIONAL.");
    }
  }

  if (proto.has_type_name()) {
    
    
    
    bool expecting_enum = (proto.type() == FieldDescriptorProto::TYPE_ENUM) ||
                          proto.has_default_value();

    Symbol type =
      LookupSymbol(proto.type_name(), field->full_name(),
                   expecting_enum ? PLACEHOLDER_ENUM : PLACEHOLDER_MESSAGE,
                   LOOKUP_TYPES);

    
    if (type.IsNull() && !pool_->enforce_weak_ && proto.options().weak()) {
      type = FindSymbol(kNonLinkedWeakMessageReplacementName);
    }

    if (type.IsNull()) {
      AddNotDefinedError(field->full_name(), proto,
                         DescriptorPool::ErrorCollector::TYPE,
                         proto.type_name());
      return;
    }

    if (!proto.has_type()) {
      
      if (type.type == Symbol::MESSAGE) {
        field->type_ = FieldDescriptor::TYPE_MESSAGE;
      } else if (type.type == Symbol::ENUM) {
        field->type_ = FieldDescriptor::TYPE_ENUM;
      } else {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not a type.");
        return;
      }
    }

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (type.type != Symbol::MESSAGE) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not a message type.");
        return;
      }
      field->message_type_ = type.descriptor;

      if (field->has_default_value()) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                 "Messages can't have default values.");
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      if (type.type != Symbol::ENUM) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "\"" + proto.type_name() + "\" is not an enum type.");
        return;
      }
      field->enum_type_ = type.enum_descriptor;

      if (field->enum_type()->is_placeholder_) {
        
        
        field->has_default_value_ = false;
      }

      if (field->has_default_value()) {
        
        
        
        
        
        if (!io::Tokenizer::IsIdentifier(proto.default_value())) {
          AddError(field->full_name(), proto,
                   DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                   "Default value for an enum field must be an identifier.");
        } else {
          
          
          
          Symbol default_value =
            LookupSymbolNoPlaceholder(proto.default_value(),
                                      field->enum_type()->full_name());

          if (default_value.type == Symbol::ENUM_VALUE &&
              default_value.enum_value_descriptor->type() ==
              field->enum_type()) {
            field->default_value_enum_ = default_value.enum_value_descriptor;
          } else {
            AddError(field->full_name(), proto,
                     DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                     "Enum type \"" + field->enum_type()->full_name() +
                     "\" has no value named \"" + proto.default_value() +
                     "\".");
          }
        }
      } else if (field->enum_type()->value_count() > 0) {
        
        
        
        field->default_value_enum_ = field->enum_type()->value(0);
      }
    } else {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with primitive type has type_name.");
    }
  } else {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
        field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
               "Field with message or enum type missing type_name.");
    }
  }

  
  
  
  if (!file_tables_->AddFieldByNumber(field)) {
    const FieldDescriptor* conflicting_field =
      file_tables_->FindFieldByNumber(field->containing_type(),
                                      field->number());
    if (field->is_extension()) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("Extension number $0 has already been used "
                                   "in \"$1\" by extension \"$2\".",
                                   field->number(),
                                   field->containing_type()->full_name(),
                                   conflicting_field->full_name()));
    } else {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NUMBER,
               strings::Substitute("Field number $0 has already been used in "
                                   "\"$1\" by field \"$2\".",
                                   field->number(),
                                   field->containing_type()->full_name(),
                                   conflicting_field->name()));
    }
  } else {
    if (field->is_extension()) {
      if (!tables_->AddExtension(field)) {
        const FieldDescriptor* conflicting_field =
            tables_->FindExtension(field->containing_type(), field->number());
        string error_msg = strings::Substitute(
            "Extension number $0 has already been used in \"$1\" by extension "
            "\"$2\" defined in $3.",
            field->number(),
            field->containing_type()->full_name(),
            conflicting_field->full_name(),
            conflicting_field->file()->name());
        
        
        
        
        AddWarning(field->full_name(), proto,
                   DescriptorPool::ErrorCollector::NUMBER, error_msg);
      }
    }
  }

  
  file_tables_->AddFieldByStylizedNames(field);
}

void DescriptorBuilder::CrossLinkEnum(
    EnumDescriptor* enum_type, const EnumDescriptorProto& proto) {
  if (enum_type->options_ == NULL) {
    enum_type->options_ = &EnumOptions::default_instance();
  }

  for (int i = 0; i < enum_type->value_count(); i++) {
    CrossLinkEnumValue(&enum_type->values_[i], proto.value(i));
  }
}

void DescriptorBuilder::CrossLinkEnumValue(
    EnumValueDescriptor* enum_value,
    const EnumValueDescriptorProto& ) {
  if (enum_value->options_ == NULL) {
    enum_value->options_ = &EnumValueOptions::default_instance();
  }
}

void DescriptorBuilder::CrossLinkService(
    ServiceDescriptor* service, const ServiceDescriptorProto& proto) {
  if (service->options_ == NULL) {
    service->options_ = &ServiceOptions::default_instance();
  }

  for (int i = 0; i < service->method_count(); i++) {
    CrossLinkMethod(&service->methods_[i], proto.method(i));
  }
}

void DescriptorBuilder::CrossLinkMethod(
    MethodDescriptor* method, const MethodDescriptorProto& proto) {
  if (method->options_ == NULL) {
    method->options_ = &MethodOptions::default_instance();
  }

  Symbol input_type = LookupSymbol(proto.input_type(), method->full_name());
  if (input_type.IsNull()) {
    AddNotDefinedError(method->full_name(), proto,
                       DescriptorPool::ErrorCollector::INPUT_TYPE,
                       proto.input_type());
  } else if (input_type.type != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::INPUT_TYPE,
             "\"" + proto.input_type() + "\" is not a message type.");
  } else {
    method->input_type_ = input_type.descriptor;
  }

  Symbol output_type = LookupSymbol(proto.output_type(), method->full_name());
  if (output_type.IsNull()) {
    AddNotDefinedError(method->full_name(), proto,
                       DescriptorPool::ErrorCollector::OUTPUT_TYPE,
                       proto.output_type());
  } else if (output_type.type != Symbol::MESSAGE) {
    AddError(method->full_name(), proto,
             DescriptorPool::ErrorCollector::OUTPUT_TYPE,
             "\"" + proto.output_type() + "\" is not a message type.");
  } else {
    method->output_type_ = output_type.descriptor;
  }
}



#define VALIDATE_OPTIONS_FROM_ARRAY(descriptor, array_name, type)  \
  for (int i = 0; i < descriptor->array_name##_count(); ++i) {     \
    Validate##type##Options(descriptor->array_name##s_ + i,        \
                            proto.array_name(i));                  \
  }



static bool IsLite(const FileDescriptor* file) {
  
  
  return file != NULL &&
         &file->options() != &FileOptions::default_instance() &&
         file->options().optimize_for() == FileOptions::LITE_RUNTIME;
}

void DescriptorBuilder::ValidateFileOptions(FileDescriptor* file,
                                            const FileDescriptorProto& proto) {
  VALIDATE_OPTIONS_FROM_ARRAY(file, message_type, Message);
  VALIDATE_OPTIONS_FROM_ARRAY(file, enum_type, Enum);
  VALIDATE_OPTIONS_FROM_ARRAY(file, service, Service);
  VALIDATE_OPTIONS_FROM_ARRAY(file, extension, Field);

  
  if (!IsLite(file)) {
    for (int i = 0; i < file->dependency_count(); i++) {
      if (IsLite(file->dependency(i))) {
        AddError(
          file->name(), proto,
          DescriptorPool::ErrorCollector::OTHER,
          "Files that do not use optimize_for = LITE_RUNTIME cannot import "
          "files which do use this option.  This file is not lite, but it "
          "imports \"" + file->dependency(i)->name() + "\" which is.");
        break;
      }
    }
  }
}


void DescriptorBuilder::ValidateMessageOptions(Descriptor* message,
                                               const DescriptorProto& proto) {
  VALIDATE_OPTIONS_FROM_ARRAY(message, field, Field);
  VALIDATE_OPTIONS_FROM_ARRAY(message, nested_type, Message);
  VALIDATE_OPTIONS_FROM_ARRAY(message, enum_type, Enum);
  VALIDATE_OPTIONS_FROM_ARRAY(message, extension, Field);

  const int64 max_extension_range =
      static_cast<int64>(message->options().message_set_wire_format() ?
                         kint32max :
                         FieldDescriptor::kMaxNumber);
  for (int i = 0; i < message->extension_range_count(); ++i) {
    if (message->extension_range(i)->end > max_extension_range + 1) {
      AddError(
          message->full_name(), proto.extension_range(i),
          DescriptorPool::ErrorCollector::NUMBER,
          strings::Substitute("Extension numbers cannot be greater than $0.",
                              max_extension_range));
    }
  }
}

void DescriptorBuilder::ValidateFieldOptions(FieldDescriptor* field,
    const FieldDescriptorProto& proto) {
  if (field->options().has_experimental_map_key()) {
    ValidateMapKey(field, proto);
  }

  
  if (field->options().lazy()) {
    if (field->type() != FieldDescriptor::TYPE_MESSAGE) {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::TYPE,
               "[lazy = true] can only be specified for submessage fields.");
    }
  }

  
  if (field->options().packed() && !field->is_packable()) {
    AddError(
      field->full_name(), proto,
      DescriptorPool::ErrorCollector::TYPE,
      "[packed = true] can only be specified for repeated primitive fields.");
  }

  
  
  if (field->containing_type_ != NULL &&
      &field->containing_type()->options() !=
      &MessageOptions::default_instance() &&
      field->containing_type()->options().message_set_wire_format()) {
    if (field->is_extension()) {
      if (!field->is_optional() ||
          field->type() != FieldDescriptor::TYPE_MESSAGE) {
        AddError(field->full_name(), proto,
                 DescriptorPool::ErrorCollector::TYPE,
                 "Extensions of MessageSets must be optional messages.");
      }
    } else {
      AddError(field->full_name(), proto,
               DescriptorPool::ErrorCollector::NAME,
               "MessageSets cannot have fields, only extensions.");
    }
  }

  
  if (IsLite(field->file()) &&
      field->containing_type_ != NULL &&
      !IsLite(field->containing_type()->file())) {
    AddError(field->full_name(), proto,
             DescriptorPool::ErrorCollector::EXTENDEE,
             "Extensions to non-lite types can only be declared in non-lite "
             "files.  Note that you cannot extend a non-lite type to contain "
             "a lite type, but the reverse is allowed.");
  }

}

void DescriptorBuilder::ValidateEnumOptions(EnumDescriptor* enm,
                                            const EnumDescriptorProto& proto) {
  VALIDATE_OPTIONS_FROM_ARRAY(enm, value, EnumValue);
  if (!enm->options().has_allow_alias() || !enm->options().allow_alias()) {
    map<int, string> used_values;
    for (int i = 0; i < enm->value_count(); ++i) {
      const EnumValueDescriptor* enum_value = enm->value(i);
      if (used_values.find(enum_value->number()) != used_values.end()) {
        string error =
            "\"" + enum_value->full_name() +
            "\" uses the same enum value as \"" +
            used_values[enum_value->number()] + "\". If this is intended, set "
            "'option allow_alias = true;' to the enum definition.";
        if (!enm->options().allow_alias()) {
          
          AddError(enm->full_name(), proto,
                   DescriptorPool::ErrorCollector::NUMBER,
                   error);
        } else {
          
          
          GOOGLE_LOG(ERROR) << error;
        }
      } else {
        used_values[enum_value->number()] = enum_value->full_name();
      }
    }
  }
}

void DescriptorBuilder::ValidateEnumValueOptions(
    EnumValueDescriptor* ,
    const EnumValueDescriptorProto& ) {
  
}
void DescriptorBuilder::ValidateServiceOptions(ServiceDescriptor* service,
    const ServiceDescriptorProto& proto) {
  if (IsLite(service->file()) &&
      (service->file()->options().cc_generic_services() ||
       service->file()->options().java_generic_services())) {
    AddError(service->full_name(), proto,
             DescriptorPool::ErrorCollector::NAME,
             "Files with optimize_for = LITE_RUNTIME cannot define services "
             "unless you set both options cc_generic_services and "
             "java_generic_sevices to false.");
  }

  VALIDATE_OPTIONS_FROM_ARRAY(service, method, Method);
}

void DescriptorBuilder::ValidateMethodOptions(MethodDescriptor* ,
    const MethodDescriptorProto& ) {
  
}

void DescriptorBuilder::ValidateMapKey(FieldDescriptor* field,
                                       const FieldDescriptorProto& proto) {
  if (!field->is_repeated()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map type is only allowed for repeated fields.");
    return;
  }

  if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map type is only allowed for fields with a message type.");
    return;
  }

  const Descriptor* item_type = field->message_type();
  if (item_type == NULL) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Could not find field type.");
    return;
  }

  
  const string& key_name = field->options().experimental_map_key();
  const Symbol key_symbol = LookupSymbol(
      key_name,
      
      
      
      item_type->full_name() + "." + key_name);

  if (key_symbol.IsNull() || key_symbol.field_descriptor->is_extension()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "Could not find field named \"" + key_name + "\" in type \"" +
             item_type->full_name() + "\".");
    return;
  }
  const FieldDescriptor* key_field = key_symbol.field_descriptor;

  if (key_field->is_repeated()) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map_key must not name a repeated field.");
    return;
  }

  if (key_field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
             "map key must name a scalar or string field.");
    return;
  }

  field->experimental_map_key_ = key_field;
}


#undef VALIDATE_OPTIONS_FROM_ARRAY



DescriptorBuilder::OptionInterpreter::OptionInterpreter(
    DescriptorBuilder* builder) : builder_(builder) {
  GOOGLE_CHECK(builder_);
}

DescriptorBuilder::OptionInterpreter::~OptionInterpreter() {
}

bool DescriptorBuilder::OptionInterpreter::InterpretOptions(
    OptionsToInterpret* options_to_interpret) {
  
  
  Message* options = options_to_interpret->options;
  const Message* original_options = options_to_interpret->original_options;

  bool failed = false;
  options_to_interpret_ = options_to_interpret;

  
  
  const FieldDescriptor* uninterpreted_options_field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  GOOGLE_CHECK(uninterpreted_options_field != NULL)
      << "No field named \"uninterpreted_option\" in the Options proto.";
  options->GetReflection()->ClearField(options, uninterpreted_options_field);

  
  const FieldDescriptor* original_uninterpreted_options_field =
      original_options->GetDescriptor()->
          FindFieldByName("uninterpreted_option");
  GOOGLE_CHECK(original_uninterpreted_options_field != NULL)
      << "No field named \"uninterpreted_option\" in the Options proto.";

  const int num_uninterpreted_options = original_options->GetReflection()->
      FieldSize(*original_options, original_uninterpreted_options_field);
  for (int i = 0; i < num_uninterpreted_options; ++i) {
    uninterpreted_option_ = down_cast<const UninterpretedOption*>(
        &original_options->GetReflection()->GetRepeatedMessage(
            *original_options, original_uninterpreted_options_field, i));
    if (!InterpretSingleOption(options)) {
      
      failed = true;
      break;
    }
  }
  
  uninterpreted_option_ = NULL;
  options_to_interpret_ = NULL;

  if (!failed) {
    
    
    
    
    
    
    
    
    string buf;
    options->AppendToString(&buf);
    GOOGLE_CHECK(options->ParseFromString(buf))
        << "Protocol message serialized itself in invalid fashion.";
  }

  return !failed;
}

bool DescriptorBuilder::OptionInterpreter::InterpretSingleOption(
    Message* options) {
  
  if (uninterpreted_option_->name_size() == 0) {
    
    
    return AddNameError("Option must have a name.");
  }
  if (uninterpreted_option_->name(0).name_part() == "uninterpreted_option") {
    return AddNameError("Option must not use reserved name "
                        "\"uninterpreted_option\".");
  }

  const Descriptor* options_descriptor = NULL;
  
  
  
  

  
  
  
  
  
  Symbol symbol = builder_->FindSymbolNotEnforcingDeps(
    options->GetDescriptor()->full_name());
  if (!symbol.IsNull() && symbol.type == Symbol::MESSAGE) {
    options_descriptor = symbol.descriptor;
  } else {
    
    
    
    options_descriptor = options->GetDescriptor();
  }
  GOOGLE_CHECK(options_descriptor);

  
  
  
  
  
  
  const Descriptor* descriptor = options_descriptor;
  const FieldDescriptor* field = NULL;
  vector<const FieldDescriptor*> intermediate_fields;
  string debug_msg_name = "";

  for (int i = 0; i < uninterpreted_option_->name_size(); ++i) {
    const string& name_part = uninterpreted_option_->name(i).name_part();
    if (debug_msg_name.size() > 0) {
      debug_msg_name += ".";
    }
    if (uninterpreted_option_->name(i).is_extension()) {
      debug_msg_name += "(" + name_part + ")";
      
      
      
      
      
      symbol = builder_->LookupSymbol(name_part,
                                      options_to_interpret_->name_scope);
      if (!symbol.IsNull() && symbol.type == Symbol::FIELD) {
        field = symbol.field_descriptor;
      }
      
      
      
      
    } else {
      debug_msg_name += name_part;
      
      field = descriptor->FindFieldByName(name_part);
    }

    if (field == NULL) {
      if (get_allow_unknown(builder_->pool_)) {
        
        
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else if (!(builder_->undefine_resolved_name_).empty()) {
        
        return AddNameError(
            "Option \"" + debug_msg_name + "\" is resolved to \"(" +
            builder_->undefine_resolved_name_ +
            ")\", which is not defined. The innermost scope is searched first "
            "in name resolution. Consider using a leading '.'(i.e., \"(." +
            debug_msg_name.substr(1) +
            "\") to start from the outermost scope.");
      } else {
        return AddNameError("Option \"" + debug_msg_name + "\" unknown.");
      }
    } else if (field->containing_type() != descriptor) {
      if (get_is_placeholder(field->containing_type())) {
        
        
        
        
        
        AddWithoutInterpreting(*uninterpreted_option_, options);
        return true;
      } else {
        
        
        
        return AddNameError("Option field \"" + debug_msg_name +
                            "\" is not a field or extension of message \"" +
                            descriptor->name() + "\".");
      }
    } else if (i < uninterpreted_option_->name_size() - 1) {
      if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
        return AddNameError("Option \"" +  debug_msg_name +
                            "\" is an atomic type, not a message.");
      } else if (field->is_repeated()) {
        return AddNameError("Option field \"" + debug_msg_name +
                            "\" is a repeated message. Repeated message "
                            "options must be initialized using an "
                            "aggregate value.");
      } else {
        
        intermediate_fields.push_back(field);
        descriptor = field->message_type();
      }
    }
  }

  
  
  
  
  
  

  
  if (!field->is_repeated() && !ExamineIfOptionIsSet(
          intermediate_fields.begin(),
          intermediate_fields.end(),
          field, debug_msg_name,
          options->GetReflection()->GetUnknownFields(*options))) {
    return false;  
  }


  
  
  scoped_ptr<UnknownFieldSet> unknown_fields(new UnknownFieldSet());
  if (!SetOptionValue(field, unknown_fields.get())) {
    return false;  
  }

  
  
  for (vector<const FieldDescriptor*>::reverse_iterator iter =
           intermediate_fields.rbegin();
       iter != intermediate_fields.rend(); ++iter) {
    scoped_ptr<UnknownFieldSet> parent_unknown_fields(new UnknownFieldSet());
    switch ((*iter)->type()) {
      case FieldDescriptor::TYPE_MESSAGE: {
        io::StringOutputStream outstr(
            parent_unknown_fields->AddLengthDelimited((*iter)->number()));
        io::CodedOutputStream out(&outstr);
        internal::WireFormat::SerializeUnknownFields(*unknown_fields, &out);
        GOOGLE_CHECK(!out.HadError())
            << "Unexpected failure while serializing option submessage "
            << debug_msg_name << "\".";
        break;
      }

      case FieldDescriptor::TYPE_GROUP: {
         parent_unknown_fields->AddGroup((*iter)->number())
                              ->MergeFrom(*unknown_fields);
        break;
      }

      default:
        GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: "
                   << (*iter)->type();
        return false;
    }
    unknown_fields.reset(parent_unknown_fields.release());
  }

  
  
  options->GetReflection()->MutableUnknownFields(options)->MergeFrom(
      *unknown_fields);

  return true;
}

void DescriptorBuilder::OptionInterpreter::AddWithoutInterpreting(
    const UninterpretedOption& uninterpreted_option, Message* options) {
  const FieldDescriptor* field =
    options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  GOOGLE_CHECK(field != NULL);

  options->GetReflection()->AddMessage(options, field)
    ->CopyFrom(uninterpreted_option);
}

bool DescriptorBuilder::OptionInterpreter::ExamineIfOptionIsSet(
    vector<const FieldDescriptor*>::const_iterator intermediate_fields_iter,
    vector<const FieldDescriptor*>::const_iterator intermediate_fields_end,
    const FieldDescriptor* innermost_field, const string& debug_msg_name,
    const UnknownFieldSet& unknown_fields) {
  
  
  

  if (intermediate_fields_iter == intermediate_fields_end) {
    
    for (int i = 0; i < unknown_fields.field_count(); i++) {
      if (unknown_fields.field(i).number() == innermost_field->number()) {
        return AddNameError("Option \"" + debug_msg_name +
                            "\" was already set.");
      }
    }
    return true;
  }

  for (int i = 0; i < unknown_fields.field_count(); i++) {
    if (unknown_fields.field(i).number() ==
        (*intermediate_fields_iter)->number()) {
      const UnknownField* unknown_field = &unknown_fields.field(i);
      FieldDescriptor::Type type = (*intermediate_fields_iter)->type();
      
      switch (type) {
        case FieldDescriptor::TYPE_MESSAGE:
          if (unknown_field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
            UnknownFieldSet intermediate_unknown_fields;
            if (intermediate_unknown_fields.ParseFromString(
                    unknown_field->length_delimited()) &&
                !ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end,
                                      innermost_field, debug_msg_name,
                                      intermediate_unknown_fields)) {
              return false;  
            }
          }
          break;

        case FieldDescriptor::TYPE_GROUP:
          if (unknown_field->type() == UnknownField::TYPE_GROUP) {
            if (!ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                      intermediate_fields_end,
                                      innermost_field, debug_msg_name,
                                      unknown_field->group())) {
              return false;  
            }
          }
          break;

        default:
          GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_MESSAGE: " << type;
          return false;
      }
    }
  }
  return true;
}

bool DescriptorBuilder::OptionInterpreter::SetOptionValue(
    const FieldDescriptor* option_field,
    UnknownFieldSet* unknown_fields) {
  
  switch (option_field->cpp_type()) {

    case FieldDescriptor::CPPTYPE_INT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64>(kint32max)) {
          return AddValueError("Value out of range for int32 option \"" +
                               option_field->full_name() + "\".");
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        if (uninterpreted_option_->negative_int_value() <
            static_cast<int64>(kint32min)) {
          return AddValueError("Value out of range for int32 option \"" +
                               option_field->full_name() + "\".");
        } else {
          SetInt32(option_field->number(),
                   uninterpreted_option_->negative_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError("Value must be integer for int32 option \"" +
                             option_field->full_name() + "\".");
      }
      break;

    case FieldDescriptor::CPPTYPE_INT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() >
            static_cast<uint64>(kint64max)) {
          return AddValueError("Value out of range for int64 option \"" +
                               option_field->full_name() + "\".");
        } else {
          SetInt64(option_field->number(),
                   uninterpreted_option_->positive_int_value(),
                   option_field->type(), unknown_fields);
        }
      } else if (uninterpreted_option_->has_negative_int_value()) {
        SetInt64(option_field->number(),
                 uninterpreted_option_->negative_int_value(),
                 option_field->type(), unknown_fields);
      } else {
        return AddValueError("Value must be integer for int64 option \"" +
                             option_field->full_name() + "\".");
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT32:
      if (uninterpreted_option_->has_positive_int_value()) {
        if (uninterpreted_option_->positive_int_value() > kuint32max) {
          return AddValueError("Value out of range for uint32 option \"" +
                               option_field->name() + "\".");
        } else {
          SetUInt32(option_field->number(),
                    uninterpreted_option_->positive_int_value(),
                    option_field->type(), unknown_fields);
        }
      } else {
        return AddValueError("Value must be non-negative integer for uint32 "
                             "option \"" + option_field->full_name() + "\".");
      }
      break;

    case FieldDescriptor::CPPTYPE_UINT64:
      if (uninterpreted_option_->has_positive_int_value()) {
        SetUInt64(option_field->number(),
                  uninterpreted_option_->positive_int_value(),
                  option_field->type(), unknown_fields);
      } else {
        return AddValueError("Value must be non-negative integer for uint64 "
                             "option \"" + option_field->full_name() + "\".");
      }
      break;

    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else {
        return AddValueError("Value must be number for float option \"" +
                             option_field->full_name() + "\".");
      }
      unknown_fields->AddFixed32(option_field->number(),
          google::protobuf::internal::WireFormatLite::EncodeFloat(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value;
      if (uninterpreted_option_->has_double_value()) {
        value = uninterpreted_option_->double_value();
      } else if (uninterpreted_option_->has_positive_int_value()) {
        value = uninterpreted_option_->positive_int_value();
      } else if (uninterpreted_option_->has_negative_int_value()) {
        value = uninterpreted_option_->negative_int_value();
      } else {
        return AddValueError("Value must be number for double option \"" +
                             option_field->full_name() + "\".");
      }
      unknown_fields->AddFixed64(option_field->number(),
          google::protobuf::internal::WireFormatLite::EncodeDouble(value));
      break;
    }

    case FieldDescriptor::CPPTYPE_BOOL:
      uint64 value;
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError("Value must be identifier for boolean option "
                             "\"" + option_field->full_name() + "\".");
      }
      if (uninterpreted_option_->identifier_value() == "true") {
        value = 1;
      } else if (uninterpreted_option_->identifier_value() == "false") {
        value = 0;
      } else {
        return AddValueError("Value must be \"true\" or \"false\" for boolean "
                             "option \"" + option_field->full_name() + "\".");
      }
      unknown_fields->AddVarint(option_field->number(), value);
      break;

    case FieldDescriptor::CPPTYPE_ENUM: {
      if (!uninterpreted_option_->has_identifier_value()) {
        return AddValueError("Value must be identifier for enum-valued option "
                             "\"" + option_field->full_name() + "\".");
      }
      const EnumDescriptor* enum_type = option_field->enum_type();
      const string& value_name = uninterpreted_option_->identifier_value();
      const EnumValueDescriptor* enum_value = NULL;

      if (enum_type->file()->pool() != DescriptorPool::generated_pool()) {
        
        
        string fully_qualified_name = enum_type->full_name();
        fully_qualified_name.resize(fully_qualified_name.size() -
                                    enum_type->name().size());
        fully_qualified_name += value_name;

        
        
        
        
        Symbol symbol =
          builder_->FindSymbolNotEnforcingDeps(fully_qualified_name);
        if (!symbol.IsNull() && symbol.type == Symbol::ENUM_VALUE) {
          if (symbol.enum_value_descriptor->type() != enum_type) {
            return AddValueError("Enum type \"" + enum_type->full_name() +
                "\" has no value named \"" + value_name + "\" for option \"" +
                option_field->full_name() +
                "\". This appears to be a value from a sibling type.");
          } else {
            enum_value = symbol.enum_value_descriptor;
          }
        }
      } else {
        
        
        enum_value = enum_type->FindValueByName(value_name);
      }

      if (enum_value == NULL) {
        return AddValueError("Enum type \"" +
                             option_field->enum_type()->full_name() +
                             "\" has no value named \"" + value_name + "\" for "
                             "option \"" + option_field->full_name() + "\".");
      } else {
        
        
        unknown_fields->AddVarint(option_field->number(),
          static_cast<uint64>(static_cast<int64>(enum_value->number())));
      }
      break;
    }

    case FieldDescriptor::CPPTYPE_STRING:
      if (!uninterpreted_option_->has_string_value()) {
        return AddValueError("Value must be quoted string for string option "
                             "\"" + option_field->full_name() + "\".");
      }
      
      unknown_fields->AddLengthDelimited(option_field->number(),
          uninterpreted_option_->string_value());
      break;

    case FieldDescriptor::CPPTYPE_MESSAGE:
      if (!SetAggregateOption(option_field, unknown_fields)) {
        return false;
      }
      break;
  }

  return true;
}

class DescriptorBuilder::OptionInterpreter::AggregateOptionFinder
    : public TextFormat::Finder {
 public:
  DescriptorBuilder* builder_;

  virtual const FieldDescriptor* FindExtension(
      Message* message, const string& name) const {
    assert_mutex_held(builder_->pool_);
    const Descriptor* descriptor = message->GetDescriptor();
    Symbol result = builder_->LookupSymbolNoPlaceholder(
        name, descriptor->full_name());
    if (result.type == Symbol::FIELD &&
        result.field_descriptor->is_extension()) {
      return result.field_descriptor;
    } else if (result.type == Symbol::MESSAGE &&
               descriptor->options().message_set_wire_format()) {
      const Descriptor* foreign_type = result.descriptor;
      
      
      
      
      
      for (int i = 0; i < foreign_type->extension_count(); i++) {
        const FieldDescriptor* extension = foreign_type->extension(i);
        if (extension->containing_type() == descriptor &&
            extension->type() == FieldDescriptor::TYPE_MESSAGE &&
            extension->is_optional() &&
            extension->message_type() == foreign_type) {
          
          return extension;
        }
      }
    }
    return NULL;
  }
};


namespace {
class AggregateErrorCollector : public io::ErrorCollector {
 public:
  string error_;

  virtual void AddError(int , int ,
                        const string& message) {
    if (!error_.empty()) {
      error_ += "; ";
    }
    error_ += message;
  }

  virtual void AddWarning(int , int ,
                          const string& ) {
    
  }
};
}




bool DescriptorBuilder::OptionInterpreter::SetAggregateOption(
    const FieldDescriptor* option_field,
    UnknownFieldSet* unknown_fields) {
  if (!uninterpreted_option_->has_aggregate_value()) {
    return AddValueError("Option \"" + option_field->full_name() +
                         "\" is a message. To set the entire message, use "
                         "syntax like \"" + option_field->name() +
                         " = { <proto text format> }\". "
                         "To set fields within it, use "
                         "syntax like \"" + option_field->name() +
                         ".foo = value\".");
  }

  const Descriptor* type = option_field->message_type();
  scoped_ptr<Message> dynamic(dynamic_factory_.GetPrototype(type)->New());
  GOOGLE_CHECK(dynamic.get() != NULL)
      << "Could not create an instance of " << option_field->DebugString();

  AggregateErrorCollector collector;
  AggregateOptionFinder finder;
  finder.builder_ = builder_;
  TextFormat::Parser parser;
  parser.RecordErrorsTo(&collector);
  parser.SetFinder(&finder);
  if (!parser.ParseFromString(uninterpreted_option_->aggregate_value(),
                              dynamic.get())) {
    AddValueError("Error while parsing option value for \"" +
                  option_field->name() + "\": " + collector.error_);
    return false;
  } else {
    string serial;
    dynamic->SerializeToString(&serial);  
    if (option_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      unknown_fields->AddLengthDelimited(option_field->number(), serial);
    } else {
      GOOGLE_CHECK_EQ(option_field->type(),  FieldDescriptor::TYPE_GROUP);
      UnknownFieldSet* group = unknown_fields->AddGroup(option_field->number());
      group->ParseFromString(serial);
    }
    return true;
  }
}

void DescriptorBuilder::OptionInterpreter::SetInt32(int number, int32 value,
    FieldDescriptor::Type type, UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
      unknown_fields->AddVarint(number,
        static_cast<uint64>(static_cast<int64>(value)));
      break;

    case FieldDescriptor::TYPE_SFIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32>(value));
      break;

    case FieldDescriptor::TYPE_SINT32:
      unknown_fields->AddVarint(number,
          google::protobuf::internal::WireFormatLite::ZigZagEncode32(value));
      break;

    default:
      GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT32: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetInt64(int number, int64 value,
    FieldDescriptor::Type type, UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_INT64:
      unknown_fields->AddVarint(number, static_cast<uint64>(value));
      break;

    case FieldDescriptor::TYPE_SFIXED64:
      unknown_fields->AddFixed64(number, static_cast<uint64>(value));
      break;

    case FieldDescriptor::TYPE_SINT64:
      unknown_fields->AddVarint(number,
          google::protobuf::internal::WireFormatLite::ZigZagEncode64(value));
      break;

    default:
      GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_INT64: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetUInt32(int number, uint32 value,
    FieldDescriptor::Type type, UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT32:
      unknown_fields->AddVarint(number, static_cast<uint64>(value));
      break;

    case FieldDescriptor::TYPE_FIXED32:
      unknown_fields->AddFixed32(number, static_cast<uint32>(value));
      break;

    default:
      GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT32: " << type;
      break;
  }
}

void DescriptorBuilder::OptionInterpreter::SetUInt64(int number, uint64 value,
    FieldDescriptor::Type type, UnknownFieldSet* unknown_fields) {
  switch (type) {
    case FieldDescriptor::TYPE_UINT64:
      unknown_fields->AddVarint(number, value);
      break;

    case FieldDescriptor::TYPE_FIXED64:
      unknown_fields->AddFixed64(number, value);
      break;

    default:
      GOOGLE_LOG(FATAL) << "Invalid wire type for CPPTYPE_UINT64: " << type;
      break;
  }
}

void DescriptorBuilder::LogUnusedDependency(const FileDescriptor* result) {

  if (!unused_dependency_.empty()) {
    std::set<string> annotation_extensions;
    annotation_extensions.insert("google.protobuf.MessageOptions");
    annotation_extensions.insert("google.protobuf.FileOptions");
    annotation_extensions.insert("google.protobuf.FieldOptions");
    annotation_extensions.insert("google.protobuf.EnumOptions");
    annotation_extensions.insert("google.protobuf.EnumValueOptions");
    annotation_extensions.insert("google.protobuf.ServiceOptions");
    annotation_extensions.insert("google.protobuf.MethodOptions");
    annotation_extensions.insert("google.protobuf.StreamOptions");
    for (set<const FileDescriptor*>::const_iterator
             it = unused_dependency_.begin();
         it != unused_dependency_.end(); ++it) {
      
      int i;
      for (i = 0 ; i < (*it)->extension_count(); ++i) {
        if (annotation_extensions.find(
                (*it)->extension(i)->containing_type()->full_name())
            != annotation_extensions.end()) {
          break;
        }
      }
      
      if (i == (*it)->extension_count()) {
        GOOGLE_LOG(WARNING) << "Warning: Unused import: \"" << result->name()
                     << "\" imports \"" << (*it)->name()
                     << "\" which is not used.";
      }
    }
  }
}

}  
}  
