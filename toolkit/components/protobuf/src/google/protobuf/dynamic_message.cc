































































#include <algorithm>
#include <google/protobuf/stubs/hash.h>

#include <google/protobuf/stubs/common.h>

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format.h>

namespace google {
namespace protobuf {

using internal::WireFormat;
using internal::ExtensionSet;
using internal::GeneratedMessageReflection;





namespace {


int FieldSpaceUsed(const FieldDescriptor* field) {
  typedef FieldDescriptor FD;  
  if (field->label() == FD::LABEL_REPEATED) {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32  : return sizeof(RepeatedField<int32   >);
      case FD::CPPTYPE_INT64  : return sizeof(RepeatedField<int64   >);
      case FD::CPPTYPE_UINT32 : return sizeof(RepeatedField<uint32  >);
      case FD::CPPTYPE_UINT64 : return sizeof(RepeatedField<uint64  >);
      case FD::CPPTYPE_DOUBLE : return sizeof(RepeatedField<double  >);
      case FD::CPPTYPE_FLOAT  : return sizeof(RepeatedField<float   >);
      case FD::CPPTYPE_BOOL   : return sizeof(RepeatedField<bool    >);
      case FD::CPPTYPE_ENUM   : return sizeof(RepeatedField<int     >);
      case FD::CPPTYPE_MESSAGE: return sizeof(RepeatedPtrField<Message>);

      case FD::CPPTYPE_STRING:
        switch (field->options().ctype()) {
          default:  
          case FieldOptions::STRING:
            return sizeof(RepeatedPtrField<string>);
        }
        break;
    }
  } else {
    switch (field->cpp_type()) {
      case FD::CPPTYPE_INT32  : return sizeof(int32   );
      case FD::CPPTYPE_INT64  : return sizeof(int64   );
      case FD::CPPTYPE_UINT32 : return sizeof(uint32  );
      case FD::CPPTYPE_UINT64 : return sizeof(uint64  );
      case FD::CPPTYPE_DOUBLE : return sizeof(double  );
      case FD::CPPTYPE_FLOAT  : return sizeof(float   );
      case FD::CPPTYPE_BOOL   : return sizeof(bool    );
      case FD::CPPTYPE_ENUM   : return sizeof(int     );

      case FD::CPPTYPE_MESSAGE:
        return sizeof(Message*);

      case FD::CPPTYPE_STRING:
        switch (field->options().ctype()) {
          default:  
          case FieldOptions::STRING:
            return sizeof(string*);
        }
        break;
    }
  }

  GOOGLE_LOG(DFATAL) << "Can't get here.";
  return 0;
}



int OneofFieldSpaceUsed(const FieldDescriptor* field) {
  typedef FieldDescriptor FD;  
  switch (field->cpp_type()) {
    case FD::CPPTYPE_INT32  : return sizeof(int32   );
    case FD::CPPTYPE_INT64  : return sizeof(int64   );
    case FD::CPPTYPE_UINT32 : return sizeof(uint32  );
    case FD::CPPTYPE_UINT64 : return sizeof(uint64  );
    case FD::CPPTYPE_DOUBLE : return sizeof(double  );
    case FD::CPPTYPE_FLOAT  : return sizeof(float   );
    case FD::CPPTYPE_BOOL   : return sizeof(bool    );
    case FD::CPPTYPE_ENUM   : return sizeof(int     );

    case FD::CPPTYPE_MESSAGE:
      return sizeof(Message*);

    case FD::CPPTYPE_STRING:
      switch (field->options().ctype()) {
        default:
        case FieldOptions::STRING:
          return sizeof(string*);
      }
      break;
  }

  GOOGLE_LOG(DFATAL) << "Can't get here.";
  return 0;
}

inline int DivideRoundingUp(int i, int j) {
  return (i + (j - 1)) / j;
}

static const int kSafeAlignment = sizeof(uint64);
static const int kMaxOneofUnionSize = sizeof(uint64);

inline int AlignTo(int offset, int alignment) {
  return DivideRoundingUp(offset, alignment) * alignment;
}



inline int AlignOffset(int offset) {
  return AlignTo(offset, kSafeAlignment);
}

#define bitsizeof(T) (sizeof(T) * 8)

}  



class DynamicMessage : public Message {
 public:
  struct TypeInfo {
    int size;
    int has_bits_offset;
    int oneof_case_offset;
    int unknown_fields_offset;
    int extensions_offset;

    
    DynamicMessageFactory* factory;  
    const DescriptorPool* pool;      
    const Descriptor* type;          

    
    
    scoped_array<int> offsets;
    scoped_ptr<const GeneratedMessageReflection> reflection;
    
    
    
    
    const DynamicMessage* prototype;
    void* default_oneof_instance;

    TypeInfo() : prototype(NULL), default_oneof_instance(NULL) {}

    ~TypeInfo() {
      delete prototype;
      operator delete(default_oneof_instance);
    }
  };

  DynamicMessage(const TypeInfo* type_info);
  ~DynamicMessage();

  
  void CrossLinkPrototypes();

  

  Message* New() const;

  int GetCachedSize() const;
  void SetCachedSize(int size) const;

  Metadata GetMetadata() const;


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DynamicMessage);

  inline bool is_prototype() const {
    return type_info_->prototype == this ||
           
           
           type_info_->prototype == NULL;
  }

  inline void* OffsetToPointer(int offset) {
    return reinterpret_cast<uint8*>(this) + offset;
  }
  inline const void* OffsetToPointer(int offset) const {
    return reinterpret_cast<const uint8*>(this) + offset;
  }

  const TypeInfo* type_info_;

  
  mutable int cached_byte_size_;
};

DynamicMessage::DynamicMessage(const TypeInfo* type_info)
  : type_info_(type_info),
    cached_byte_size_(0) {
  
  
  
  
  
  
  
  

  const Descriptor* descriptor = type_info_->type;

  
  for (int i = 0 ; i < descriptor->oneof_decl_count(); ++i) {
    new(OffsetToPointer(type_info_->oneof_case_offset + sizeof(uint32) * i))
        uint32(0);
  }

  new(OffsetToPointer(type_info_->unknown_fields_offset)) UnknownFieldSet;

  if (type_info_->extensions_offset != -1) {
    new(OffsetToPointer(type_info_->extensions_offset)) ExtensionSet;
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);
    if (field->containing_oneof()) {
      continue;
    }
    switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)                                           \
      case FieldDescriptor::CPPTYPE_##CPPTYPE:                               \
        if (!field->is_repeated()) {                                         \
          new(field_ptr) TYPE(field->default_value_##TYPE());                \
        } else {                                                             \
          new(field_ptr) RepeatedField<TYPE>();                              \
        }                                                                    \
        break;

      HANDLE_TYPE(INT32 , int32 );
      HANDLE_TYPE(INT64 , int64 );
      HANDLE_TYPE(UINT32, uint32);
      HANDLE_TYPE(UINT64, uint64);
      HANDLE_TYPE(DOUBLE, double);
      HANDLE_TYPE(FLOAT , float );
      HANDLE_TYPE(BOOL  , bool  );
#undef HANDLE_TYPE

      case FieldDescriptor::CPPTYPE_ENUM:
        if (!field->is_repeated()) {
          new(field_ptr) int(field->default_value_enum()->number());
        } else {
          new(field_ptr) RepeatedField<int>();
        }
        break;

      case FieldDescriptor::CPPTYPE_STRING:
        switch (field->options().ctype()) {
          default:  
          case FieldOptions::STRING:
            if (!field->is_repeated()) {
              if (is_prototype()) {
                new(field_ptr) const string*(&field->default_value_string());
              } else {
                string* default_value =
                  *reinterpret_cast<string* const*>(
                    type_info_->prototype->OffsetToPointer(
                      type_info_->offsets[i]));
                new(field_ptr) string*(default_value);
              }
            } else {
              new(field_ptr) RepeatedPtrField<string>();
            }
            break;
        }
        break;

      case FieldDescriptor::CPPTYPE_MESSAGE: {
        if (!field->is_repeated()) {
          new(field_ptr) Message*(NULL);
        } else {
          new(field_ptr) RepeatedPtrField<Message>();
        }
        break;
      }
    }
  }
}

DynamicMessage::~DynamicMessage() {
  const Descriptor* descriptor = type_info_->type;

  reinterpret_cast<UnknownFieldSet*>(
    OffsetToPointer(type_info_->unknown_fields_offset))->~UnknownFieldSet();

  if (type_info_->extensions_offset != -1) {
    reinterpret_cast<ExtensionSet*>(
      OffsetToPointer(type_info_->extensions_offset))->~ExtensionSet();
  }

  
  
  
  
  
  
  
  
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    if (field->containing_oneof()) {
      void* field_ptr = OffsetToPointer(
          type_info_->oneof_case_offset
          + sizeof(uint32) * field->containing_oneof()->index());
      if (*(reinterpret_cast<const uint32*>(field_ptr)) ==
          field->number()) {
        field_ptr = OffsetToPointer(type_info_->offsets[
            descriptor->field_count() + field->containing_oneof()->index()]);
        if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
          switch (field->options().ctype()) {
            default:
            case FieldOptions::STRING:
              delete *reinterpret_cast<string**>(field_ptr);
              break;
          }
        } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            delete *reinterpret_cast<Message**>(field_ptr);
        }
      }
      continue;
    }
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);

    if (field->is_repeated()) {
      switch (field->cpp_type()) {
#define HANDLE_TYPE(UPPERCASE, LOWERCASE)                                     \
        case FieldDescriptor::CPPTYPE_##UPPERCASE :                           \
          reinterpret_cast<RepeatedField<LOWERCASE>*>(field_ptr)              \
              ->~RepeatedField<LOWERCASE>();                                  \
          break

        HANDLE_TYPE( INT32,  int32);
        HANDLE_TYPE( INT64,  int64);
        HANDLE_TYPE(UINT32, uint32);
        HANDLE_TYPE(UINT64, uint64);
        HANDLE_TYPE(DOUBLE, double);
        HANDLE_TYPE( FLOAT,  float);
        HANDLE_TYPE(  BOOL,   bool);
        HANDLE_TYPE(  ENUM,    int);
#undef HANDLE_TYPE

        case FieldDescriptor::CPPTYPE_STRING:
          switch (field->options().ctype()) {
            default:  
            case FieldOptions::STRING:
              reinterpret_cast<RepeatedPtrField<string>*>(field_ptr)
                  ->~RepeatedPtrField<string>();
              break;
          }
          break;

        case FieldDescriptor::CPPTYPE_MESSAGE:
          reinterpret_cast<RepeatedPtrField<Message>*>(field_ptr)
              ->~RepeatedPtrField<Message>();
          break;
      }

    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      switch (field->options().ctype()) {
        default:  
        case FieldOptions::STRING: {
          string* ptr = *reinterpret_cast<string**>(field_ptr);
          if (ptr != &field->default_value_string()) {
            delete ptr;
          }
          break;
        }
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (!is_prototype()) {
        Message* message = *reinterpret_cast<Message**>(field_ptr);
        if (message != NULL) {
          delete message;
        }
      }
    }
  }
}

void DynamicMessage::CrossLinkPrototypes() {
  
  GOOGLE_CHECK(is_prototype());

  DynamicMessageFactory* factory = type_info_->factory;
  const Descriptor* descriptor = type_info_->type;

  
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    void* field_ptr = OffsetToPointer(type_info_->offsets[i]);
    if (field->containing_oneof()) {
      field_ptr = reinterpret_cast<uint8*>(
          type_info_->default_oneof_instance) + type_info_->offsets[i];
    }

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !field->is_repeated()) {
      
      
      
      
      *reinterpret_cast<const Message**>(field_ptr) =
        factory->GetPrototypeNoLock(field->message_type());
    }
  }
}

Message* DynamicMessage::New() const {
  void* new_base = operator new(type_info_->size);
  memset(new_base, 0, type_info_->size);
  return new(new_base) DynamicMessage(type_info_);
}

int DynamicMessage::GetCachedSize() const {
  return cached_byte_size_;
}

void DynamicMessage::SetCachedSize(int size) const {
  
  
  
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  cached_byte_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}

Metadata DynamicMessage::GetMetadata() const {
  Metadata metadata;
  metadata.descriptor = type_info_->type;
  metadata.reflection = type_info_->reflection.get();
  return metadata;
}



struct DynamicMessageFactory::PrototypeMap {
  typedef hash_map<const Descriptor*, const DynamicMessage::TypeInfo*> Map;
  Map map_;
};

DynamicMessageFactory::DynamicMessageFactory()
  : pool_(NULL), delegate_to_generated_factory_(false),
    prototypes_(new PrototypeMap) {
}

DynamicMessageFactory::DynamicMessageFactory(const DescriptorPool* pool)
  : pool_(pool), delegate_to_generated_factory_(false),
    prototypes_(new PrototypeMap) {
}

DynamicMessageFactory::~DynamicMessageFactory() {
  for (PrototypeMap::Map::iterator iter = prototypes_->map_.begin();
       iter != prototypes_->map_.end(); ++iter) {
    DeleteDefaultOneofInstance(iter->second->type,
                               iter->second->offsets.get(),
                               iter->second->default_oneof_instance);
    delete iter->second;
  }
}

const Message* DynamicMessageFactory::GetPrototype(const Descriptor* type) {
  MutexLock lock(&prototypes_mutex_);
  return GetPrototypeNoLock(type);
}

const Message* DynamicMessageFactory::GetPrototypeNoLock(
    const Descriptor* type) {
  if (delegate_to_generated_factory_ &&
      type->file()->pool() == DescriptorPool::generated_pool()) {
    return MessageFactory::generated_factory()->GetPrototype(type);
  }

  const DynamicMessage::TypeInfo** target = &prototypes_->map_[type];
  if (*target != NULL) {
    
    return (*target)->prototype;
  }

  DynamicMessage::TypeInfo* type_info = new DynamicMessage::TypeInfo;
  *target = type_info;

  type_info->type = type;
  type_info->pool = (pool_ == NULL) ? type->file()->pool() : pool_;
  type_info->factory = this;

  
  
  
  
  
  
  

  
  int* offsets = new int[type->field_count() + type->oneof_decl_count()];
  type_info->offsets.reset(offsets);

  
  
  
  int size = sizeof(DynamicMessage);
  size = AlignOffset(size);

  
  type_info->has_bits_offset = size;
  int has_bits_array_size =
    DivideRoundingUp(type->field_count(), bitsizeof(uint32));
  size += has_bits_array_size * sizeof(uint32);
  size = AlignOffset(size);

  
  if (type->oneof_decl_count() > 0) {
    type_info->oneof_case_offset = size;
    size += type->oneof_decl_count() * sizeof(uint32);
    size = AlignOffset(size);
  }

  
  if (type->extension_range_count() > 0) {
    type_info->extensions_offset = size;
    size += sizeof(ExtensionSet);
    size = AlignOffset(size);
  } else {
    
    type_info->extensions_offset = -1;
  }

  
  for (int i = 0; i < type->field_count(); i++) {
    
    
    if (!type->field(i)->containing_oneof()) {
      int field_size = FieldSpaceUsed(type->field(i));
      size = AlignTo(size, min(kSafeAlignment, field_size));
      offsets[i] = size;
      size += field_size;
    }
  }

  
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    size = AlignTo(size, kSafeAlignment);
    offsets[type->field_count() + i] = size;
    size += kMaxOneofUnionSize;
  }

  
  size = AlignOffset(size);
  type_info->unknown_fields_offset = size;
  size += sizeof(UnknownFieldSet);

  
  
  size = AlignOffset(size);
  type_info->size = size;

  
  void* base = operator new(size);
  memset(base, 0, size);
  DynamicMessage* prototype = new(base) DynamicMessage(type_info);
  type_info->prototype = prototype;

  
  if (type->oneof_decl_count() > 0) {
    
    
    int oneof_size = 0;
    for (int i = 0; i < type->oneof_decl_count(); i++) {
      for (int j = 0; j < type->oneof_decl(i)->field_count(); j++) {
        const FieldDescriptor* field = type->oneof_decl(i)->field(j);
        int field_size = OneofFieldSpaceUsed(field);
        oneof_size = AlignTo(oneof_size, min(kSafeAlignment, field_size));
        offsets[field->index()] = oneof_size;
        oneof_size += field_size;
      }
    }
    
    type_info->default_oneof_instance = ::operator new(oneof_size);
    ConstructDefaultOneofInstance(type_info->type,
                                  type_info->offsets.get(),
                                  type_info->default_oneof_instance);
    type_info->reflection.reset(
        new GeneratedMessageReflection(
            type_info->type,
            type_info->prototype,
            type_info->offsets.get(),
            type_info->has_bits_offset,
            type_info->unknown_fields_offset,
            type_info->extensions_offset,
            type_info->default_oneof_instance,
            type_info->oneof_case_offset,
            type_info->pool,
            this,
            type_info->size));
  } else {
    type_info->reflection.reset(
        new GeneratedMessageReflection(
            type_info->type,
            type_info->prototype,
            type_info->offsets.get(),
            type_info->has_bits_offset,
            type_info->unknown_fields_offset,
            type_info->extensions_offset,
            type_info->pool,
            this,
            type_info->size));
  }
  
  prototype->CrossLinkPrototypes();

  return prototype;
}

void DynamicMessageFactory::ConstructDefaultOneofInstance(
    const Descriptor* type,
    const int offsets[],
    void* default_oneof_instance) {
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    for (int j = 0; j < type->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = type->oneof_decl(i)->field(j);
      void* field_ptr = reinterpret_cast<uint8*>(
          default_oneof_instance) + offsets[field->index()];
      switch (field->cpp_type()) {
#define HANDLE_TYPE(CPPTYPE, TYPE)                                      \
        case FieldDescriptor::CPPTYPE_##CPPTYPE:                        \
          new(field_ptr) TYPE(field->default_value_##TYPE());           \
          break;

        HANDLE_TYPE(INT32 , int32 );
        HANDLE_TYPE(INT64 , int64 );
        HANDLE_TYPE(UINT32, uint32);
        HANDLE_TYPE(UINT64, uint64);
        HANDLE_TYPE(DOUBLE, double);
        HANDLE_TYPE(FLOAT , float );
        HANDLE_TYPE(BOOL  , bool  );
#undef HANDLE_TYPE

        case FieldDescriptor::CPPTYPE_ENUM:
          new(field_ptr) int(field->default_value_enum()->number());
          break;
        case FieldDescriptor::CPPTYPE_STRING:
          switch (field->options().ctype()) {
            default:
            case FieldOptions::STRING:
              if (field->has_default_value()) {
                new(field_ptr) const string*(&field->default_value_string());
              } else {
                new(field_ptr) string*(
                    const_cast<string*>(&internal::GetEmptyString()));
              }
              break;
          }
          break;

        case FieldDescriptor::CPPTYPE_MESSAGE: {
          new(field_ptr) Message*(NULL);
          break;
        }
      }
    }
  }
}

void DynamicMessageFactory::DeleteDefaultOneofInstance(
    const Descriptor* type,
    const int offsets[],
    void* default_oneof_instance) {
  for (int i = 0; i < type->oneof_decl_count(); i++) {
    for (int j = 0; j < type->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = type->oneof_decl(i)->field(j);
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
        switch (field->options().ctype()) {
          default:
          case FieldOptions::STRING:
            break;
        }
      }
    }
  }
}

}  
}  
