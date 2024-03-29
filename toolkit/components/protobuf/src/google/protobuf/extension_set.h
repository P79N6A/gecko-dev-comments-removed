




































#ifndef GOOGLE_PROTOBUF_EXTENSION_SET_H__
#define GOOGLE_PROTOBUF_EXTENSION_SET_H__

#include <vector>
#include <map>
#include <utility>
#include <string>


#include <google/protobuf/stubs/common.h>

#include <google/protobuf/repeated_field.h>

namespace google {

namespace protobuf {
  class Descriptor;                                    
  class FieldDescriptor;                               
  class DescriptorPool;                                
  class MessageLite;                                   
  class Message;                                       
  class MessageFactory;                                
  class UnknownFieldSet;                               
  namespace io {
    class CodedInputStream;                              
    class CodedOutputStream;                             
  }
  namespace internal {
    class FieldSkipper;                                  
  }
}

namespace protobuf {
namespace internal {





typedef uint8 FieldType;




typedef bool EnumValidityFunc(int number);



typedef bool EnumValidityFuncWithArg(const void* arg, int number);


struct ExtensionInfo {
  inline ExtensionInfo() {}
  inline ExtensionInfo(FieldType type_param, bool isrepeated, bool ispacked)
      : type(type_param), is_repeated(isrepeated), is_packed(ispacked),
        descriptor(NULL) {}

  FieldType type;
  bool is_repeated;
  bool is_packed;

  struct EnumValidityCheck {
    EnumValidityFuncWithArg* func;
    const void* arg;
  };

  union {
    EnumValidityCheck enum_validity_check;
    const MessageLite* message_prototype;
  };

  
  
  
  const FieldDescriptor* descriptor;
};



class LIBPROTOBUF_EXPORT ExtensionFinder {
 public:
  virtual ~ExtensionFinder();

  
  virtual bool Find(int number, ExtensionInfo* output) = 0;
};



class LIBPROTOBUF_EXPORT GeneratedExtensionFinder : public ExtensionFinder {
 public:
  GeneratedExtensionFinder(const MessageLite* containing_type)
      : containing_type_(containing_type) {}
  virtual ~GeneratedExtensionFinder() {}

  
  virtual bool Find(int number, ExtensionInfo* output);

 private:
  const MessageLite* containing_type_;
};


class MessageSetFieldSkipper;















class LIBPROTOBUF_EXPORT ExtensionSet {
 public:
  ExtensionSet();
  ~ExtensionSet();

  
  
  
  
  
  static void RegisterExtension(const MessageLite* containing_type,
                                int number, FieldType type,
                                bool is_repeated, bool is_packed);
  static void RegisterEnumExtension(const MessageLite* containing_type,
                                    int number, FieldType type,
                                    bool is_repeated, bool is_packed,
                                    EnumValidityFunc* is_valid);
  static void RegisterMessageExtension(const MessageLite* containing_type,
                                       int number, FieldType type,
                                       bool is_repeated, bool is_packed,
                                       const MessageLite* prototype);

  

  
  
  void AppendToList(const Descriptor* containing_type,
                    const DescriptorPool* pool,
                    vector<const FieldDescriptor*>* output) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  bool Has(int number) const;
  int ExtensionSize(int number) const;   
  int NumExtensions() const;  
  FieldType ExtensionType(int number) const;
  void ClearExtension(int number);

  

  int32  GetInt32 (int number, int32  default_value) const;
  int64  GetInt64 (int number, int64  default_value) const;
  uint32 GetUInt32(int number, uint32 default_value) const;
  uint64 GetUInt64(int number, uint64 default_value) const;
  float  GetFloat (int number, float  default_value) const;
  double GetDouble(int number, double default_value) const;
  bool   GetBool  (int number, bool   default_value) const;
  int    GetEnum  (int number, int    default_value) const;
  const string & GetString (int number, const string&  default_value) const;
  const MessageLite& GetMessage(int number,
                                const MessageLite& default_value) const;
  const MessageLite& GetMessage(int number, const Descriptor* message_type,
                                MessageFactory* factory) const;

  
  
  
#define desc const FieldDescriptor* descriptor  // avoid line wrapping
  void SetInt32 (int number, FieldType type, int32  value, desc);
  void SetInt64 (int number, FieldType type, int64  value, desc);
  void SetUInt32(int number, FieldType type, uint32 value, desc);
  void SetUInt64(int number, FieldType type, uint64 value, desc);
  void SetFloat (int number, FieldType type, float  value, desc);
  void SetDouble(int number, FieldType type, double value, desc);
  void SetBool  (int number, FieldType type, bool   value, desc);
  void SetEnum  (int number, FieldType type, int    value, desc);
  void SetString(int number, FieldType type, const string& value, desc);
  string * MutableString (int number, FieldType type, desc);
  MessageLite* MutableMessage(int number, FieldType type,
                              const MessageLite& prototype, desc);
  MessageLite* MutableMessage(const FieldDescriptor* decsriptor,
                              MessageFactory* factory);
  
  
  
  void SetAllocatedMessage(int number, FieldType type,
                           const FieldDescriptor* descriptor,
                           MessageLite* message);
  MessageLite* ReleaseMessage(int number, const MessageLite& prototype);
  MessageLite* ReleaseMessage(const FieldDescriptor* descriptor,
                              MessageFactory* factory);
#undef desc

  

  
  
  
  const void* GetRawRepeatedField(int number, const void* default_value) const;
  
  
  
  void* MutableRawRepeatedField(int number, FieldType field_type,
                                bool packed, const FieldDescriptor* desc);

  
  
  
  
  void* MutableRawRepeatedField(int number);

  int32  GetRepeatedInt32 (int number, int index) const;
  int64  GetRepeatedInt64 (int number, int index) const;
  uint32 GetRepeatedUInt32(int number, int index) const;
  uint64 GetRepeatedUInt64(int number, int index) const;
  float  GetRepeatedFloat (int number, int index) const;
  double GetRepeatedDouble(int number, int index) const;
  bool   GetRepeatedBool  (int number, int index) const;
  int    GetRepeatedEnum  (int number, int index) const;
  const string & GetRepeatedString (int number, int index) const;
  const MessageLite& GetRepeatedMessage(int number, int index) const;

  void SetRepeatedInt32 (int number, int index, int32  value);
  void SetRepeatedInt64 (int number, int index, int64  value);
  void SetRepeatedUInt32(int number, int index, uint32 value);
  void SetRepeatedUInt64(int number, int index, uint64 value);
  void SetRepeatedFloat (int number, int index, float  value);
  void SetRepeatedDouble(int number, int index, double value);
  void SetRepeatedBool  (int number, int index, bool   value);
  void SetRepeatedEnum  (int number, int index, int    value);
  void SetRepeatedString(int number, int index, const string& value);
  string * MutableRepeatedString (int number, int index);
  MessageLite* MutableRepeatedMessage(int number, int index);

#define desc const FieldDescriptor* descriptor  // avoid line wrapping
  void AddInt32 (int number, FieldType type, bool packed, int32  value, desc);
  void AddInt64 (int number, FieldType type, bool packed, int64  value, desc);
  void AddUInt32(int number, FieldType type, bool packed, uint32 value, desc);
  void AddUInt64(int number, FieldType type, bool packed, uint64 value, desc);
  void AddFloat (int number, FieldType type, bool packed, float  value, desc);
  void AddDouble(int number, FieldType type, bool packed, double value, desc);
  void AddBool  (int number, FieldType type, bool packed, bool   value, desc);
  void AddEnum  (int number, FieldType type, bool packed, int    value, desc);
  void AddString(int number, FieldType type, const string& value, desc);
  string * AddString (int number, FieldType type, desc);
  MessageLite* AddMessage(int number, FieldType type,
                          const MessageLite& prototype, desc);
  MessageLite* AddMessage(const FieldDescriptor* descriptor,
                          MessageFactory* factory);
#undef desc

  void RemoveLast(int number);
  MessageLite* ReleaseLast(int number);
  void SwapElements(int number, int index1, int index2);

  
  

  
  
  
  
  

  void Clear();
  void MergeFrom(const ExtensionSet& other);
  void Swap(ExtensionSet* other);
  void SwapExtension(ExtensionSet* other, int number);
  bool IsInitialized() const;

  
  
  bool ParseField(uint32 tag, io::CodedInputStream* input,
                  ExtensionFinder* extension_finder,
                  FieldSkipper* field_skipper);

  
  
  
  
  
  
  bool ParseField(uint32 tag, io::CodedInputStream* input,
                  const MessageLite* containing_type);
  bool ParseField(uint32 tag, io::CodedInputStream* input,
                  const Message* containing_type,
                  UnknownFieldSet* unknown_fields);
  bool ParseField(uint32 tag, io::CodedInputStream* input,
                  const MessageLite* containing_type,
                  io::CodedOutputStream* unknown_fields);

  
  
  bool ParseMessageSet(io::CodedInputStream* input,
                       ExtensionFinder* extension_finder,
                       MessageSetFieldSkipper* field_skipper);

  
  
  bool ParseMessageSet(io::CodedInputStream* input,
                       const MessageLite* containing_type);
  bool ParseMessageSet(io::CodedInputStream* input,
                       const Message* containing_type,
                       UnknownFieldSet* unknown_fields);

  
  
  
  
  void SerializeWithCachedSizes(int start_field_number,
                                int end_field_number,
                                io::CodedOutputStream* output) const;

  
  
  
  
  
  uint8* SerializeWithCachedSizesToArray(int start_field_number,
                                         int end_field_number,
                                         uint8* target) const;

  
  void SerializeMessageSetWithCachedSizes(io::CodedOutputStream* output) const;
  uint8* SerializeMessageSetWithCachedSizesToArray(uint8* target) const;

  
  int ByteSize() const;

  
  int MessageSetByteSize() const;

  
  
  
  
  
  
  
  int SpaceUsedExcludingSelf() const;

 private:

  
  class LIBPROTOBUF_EXPORT LazyMessageExtension {
   public:
    LazyMessageExtension() {}
    virtual ~LazyMessageExtension() {}

    virtual LazyMessageExtension* New() const = 0;
    virtual const MessageLite& GetMessage(
        const MessageLite& prototype) const = 0;
    virtual MessageLite* MutableMessage(const MessageLite& prototype) = 0;
    virtual void SetAllocatedMessage(MessageLite *message) = 0;
    virtual MessageLite* ReleaseMessage(const MessageLite& prototype) = 0;

    virtual bool IsInitialized() const = 0;
    virtual int ByteSize() const = 0;
    virtual int SpaceUsed() const = 0;

    virtual void MergeFrom(const LazyMessageExtension& other) = 0;
    virtual void Clear() = 0;

    virtual bool ReadMessage(const MessageLite& prototype,
                             io::CodedInputStream* input) = 0;
    virtual void WriteMessage(int number,
                              io::CodedOutputStream* output) const = 0;
    virtual uint8* WriteMessageToArray(int number, uint8* target) const = 0;
   private:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LazyMessageExtension);
  };
  struct Extension {
    
    
    union {
      int32                 int32_value;
      int64                 int64_value;
      uint32                uint32_value;
      uint64                uint64_value;
      float                 float_value;
      double                double_value;
      bool                  bool_value;
      int                   enum_value;
      string*               string_value;
      MessageLite*          message_value;
      LazyMessageExtension* lazymessage_value;

      RepeatedField   <int32      >* repeated_int32_value;
      RepeatedField   <int64      >* repeated_int64_value;
      RepeatedField   <uint32     >* repeated_uint32_value;
      RepeatedField   <uint64     >* repeated_uint64_value;
      RepeatedField   <float      >* repeated_float_value;
      RepeatedField   <double     >* repeated_double_value;
      RepeatedField   <bool       >* repeated_bool_value;
      RepeatedField   <int        >* repeated_enum_value;
      RepeatedPtrField<string     >* repeated_string_value;
      RepeatedPtrField<MessageLite>* repeated_message_value;
    };

    FieldType type;
    bool is_repeated;

    
    
    
    
    
    
    bool is_cleared : 4;

    
    
    
    
    
    bool is_lazy : 4;

    
    bool is_packed;

    
    
    
    mutable int cached_size;

    
    
    
    const FieldDescriptor* descriptor;

    
    void SerializeFieldWithCachedSizes(
        int number,
        io::CodedOutputStream* output) const;
    uint8* SerializeFieldWithCachedSizesToArray(
        int number,
        uint8* target) const;
    void SerializeMessageSetItemWithCachedSizes(
        int number,
        io::CodedOutputStream* output) const;
    uint8* SerializeMessageSetItemWithCachedSizesToArray(
        int number,
        uint8* target) const;
    int ByteSize(int number) const;
    int MessageSetItemByteSize(int number) const;
    void Clear();
    int GetSize() const;
    void Free();
    int SpaceUsedExcludingSelf() const;
  };


  
  
  
  
  bool FindExtensionInfoFromTag(uint32 tag, ExtensionFinder* extension_finder,
                                int* field_number, ExtensionInfo* extension,
                                bool* was_packed_on_wire);

  
  
  
  
  bool FindExtensionInfoFromFieldNumber(int wire_type, int field_number,
                                        ExtensionFinder* extension_finder,
                                        ExtensionInfo* extension,
                                        bool* was_packed_on_wire);

  
  
  
  
  bool ParseFieldWithExtensionInfo(int field_number,
                                   bool was_packed_on_wire,
                                   const ExtensionInfo& extension,
                                   io::CodedInputStream* input,
                                   FieldSkipper* field_skipper);

  
  
  bool ParseFieldMaybeLazily(int wire_type, int field_number,
                             io::CodedInputStream* input,
                             ExtensionFinder* extension_finder,
                             MessageSetFieldSkipper* field_skipper);

  
  
  bool MaybeNewExtension(int number, const FieldDescriptor* descriptor,
                         Extension** result);

  
  
  bool ParseMessageSetItem(io::CodedInputStream* input,
                           ExtensionFinder* extension_finder,
                           MessageSetFieldSkipper* field_skipper);


  
  
  
  
  

  
  static inline int RepeatedMessage_SpaceUsedExcludingSelf(
      RepeatedPtrFieldBase* field);

  
  
  
  
  
  
  std::map<int, Extension> extensions_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ExtensionSet);
};


inline void ExtensionSet::SetString(int number, FieldType type,
                                    const string& value,
                                    const FieldDescriptor* descriptor) {
  MutableString(number, type, descriptor)->assign(value);
}
inline void ExtensionSet::SetRepeatedString(int number, int index,
                                            const string& value) {
  MutableRepeatedString(number, index)->assign(value);
}
inline void ExtensionSet::AddString(int number, FieldType type,
                                    const string& value,
                                    const FieldDescriptor* descriptor) {
  AddString(number, type, descriptor)->assign(value);
}




























































template <typename Type>
class PrimitiveTypeTraits {
 public:
  typedef Type ConstType;
  typedef Type MutableType;
  typedef PrimitiveTypeTraits<Type> Singular;

  static inline ConstType Get(int number, const ExtensionSet& set,
                              ConstType default_value);
  static inline void Set(int number, FieldType field_type,
                         ConstType value, ExtensionSet* set);
};

template <typename Type>
class RepeatedPrimitiveTypeTraits {
 public:
  typedef Type ConstType;
  typedef Type MutableType;
  typedef RepeatedPrimitiveTypeTraits<Type> Repeated;

  typedef RepeatedField<Type> RepeatedFieldType;

  static inline Type Get(int number, const ExtensionSet& set, int index);
  static inline void Set(int number, int index, Type value, ExtensionSet* set);
  static inline void Add(int number, FieldType field_type,
                         bool is_packed, Type value, ExtensionSet* set);

  static inline const RepeatedField<ConstType>&
      GetRepeated(int number, const ExtensionSet& set);
  static inline RepeatedField<Type>*
      MutableRepeated(int number, FieldType field_type,
                      bool is_packed, ExtensionSet* set);

  static const RepeatedFieldType* GetDefaultRepeatedField();
};


void InitializeDefaultRepeatedFields();
void DestroyDefaultRepeatedFields();

class LIBPROTOBUF_EXPORT RepeatedPrimitiveGenericTypeTraits {
 private:
  template<typename Type> friend class RepeatedPrimitiveTypeTraits;
  friend void InitializeDefaultRepeatedFields();
  friend void DestroyDefaultRepeatedFields();
  static const RepeatedField<int32>* default_repeated_field_int32_;
  static const RepeatedField<int64>* default_repeated_field_int64_;
  static const RepeatedField<uint32>* default_repeated_field_uint32_;
  static const RepeatedField<uint64>* default_repeated_field_uint64_;
  static const RepeatedField<double>* default_repeated_field_double_;
  static const RepeatedField<float>* default_repeated_field_float_;
  static const RepeatedField<bool>* default_repeated_field_bool_;
};

#define PROTOBUF_DEFINE_PRIMITIVE_TYPE(TYPE, METHOD)                       \
template<> inline TYPE PrimitiveTypeTraits<TYPE>::Get(                     \
    int number, const ExtensionSet& set, TYPE default_value) {             \
  return set.Get##METHOD(number, default_value);                           \
}                                                                          \
template<> inline void PrimitiveTypeTraits<TYPE>::Set(                     \
    int number, FieldType field_type, TYPE value, ExtensionSet* set) {     \
  set->Set##METHOD(number, field_type, value, NULL);                       \
}                                                                          \
                                                                           \
template<> inline TYPE RepeatedPrimitiveTypeTraits<TYPE>::Get(             \
    int number, const ExtensionSet& set, int index) {                      \
  return set.GetRepeated##METHOD(number, index);                           \
}                                                                          \
template<> inline void RepeatedPrimitiveTypeTraits<TYPE>::Set(             \
    int number, int index, TYPE value, ExtensionSet* set) {                \
  set->SetRepeated##METHOD(number, index, value);                          \
}                                                                          \
template<> inline void RepeatedPrimitiveTypeTraits<TYPE>::Add(             \
    int number, FieldType field_type, bool is_packed,                      \
    TYPE value, ExtensionSet* set) {                                       \
  set->Add##METHOD(number, field_type, is_packed, value, NULL);            \
}                                                                          \
template<> inline const RepeatedField<TYPE>*                               \
    RepeatedPrimitiveTypeTraits<TYPE>::GetDefaultRepeatedField() {         \
  return RepeatedPrimitiveGenericTypeTraits::                              \
      default_repeated_field_##TYPE##_;                                    \
}                                                                          \
template<> inline const RepeatedField<TYPE>&                               \
    RepeatedPrimitiveTypeTraits<TYPE>::GetRepeated(int number,             \
                                               const ExtensionSet& set) {  \
  return *reinterpret_cast<const RepeatedField<TYPE>*>(                    \
                            set.GetRawRepeatedField(                       \
                                number, GetDefaultRepeatedField()));       \
}                                                                          \
template<> inline RepeatedField<TYPE>*                                     \
    RepeatedPrimitiveTypeTraits<TYPE>::MutableRepeated(int number,         \
                                                   FieldType field_type,   \
                                                   bool is_packed,         \
                                                   ExtensionSet* set) {    \
  return reinterpret_cast<RepeatedField<TYPE>*>(                           \
      set->MutableRawRepeatedField(number, field_type, is_packed, NULL));  \
}

PROTOBUF_DEFINE_PRIMITIVE_TYPE( int32,  Int32)
PROTOBUF_DEFINE_PRIMITIVE_TYPE( int64,  Int64)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(uint32, UInt32)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(uint64, UInt64)
PROTOBUF_DEFINE_PRIMITIVE_TYPE( float,  Float)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(double, Double)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(  bool,   Bool)

#undef PROTOBUF_DEFINE_PRIMITIVE_TYPE





class LIBPROTOBUF_EXPORT StringTypeTraits {
 public:
  typedef const string& ConstType;
  typedef string* MutableType;
  typedef StringTypeTraits Singular;

  static inline const string& Get(int number, const ExtensionSet& set,
                                  ConstType default_value) {
    return set.GetString(number, default_value);
  }
  static inline void Set(int number, FieldType field_type,
                         const string& value, ExtensionSet* set) {
    set->SetString(number, field_type, value, NULL);
  }
  static inline string* Mutable(int number, FieldType field_type,
                                ExtensionSet* set) {
    return set->MutableString(number, field_type, NULL);
  }
};

class LIBPROTOBUF_EXPORT RepeatedStringTypeTraits {
 public:
  typedef const string& ConstType;
  typedef string* MutableType;
  typedef RepeatedStringTypeTraits Repeated;

  typedef RepeatedPtrField<string> RepeatedFieldType;

  static inline const string& Get(int number, const ExtensionSet& set,
                                  int index) {
    return set.GetRepeatedString(number, index);
  }
  static inline void Set(int number, int index,
                         const string& value, ExtensionSet* set) {
    set->SetRepeatedString(number, index, value);
  }
  static inline string* Mutable(int number, int index, ExtensionSet* set) {
    return set->MutableRepeatedString(number, index);
  }
  static inline void Add(int number, FieldType field_type,
                         bool , const string& value,
                         ExtensionSet* set) {
    set->AddString(number, field_type, value, NULL);
  }
  static inline string* Add(int number, FieldType field_type,
                            ExtensionSet* set) {
    return set->AddString(number, field_type, NULL);
  }
  static inline const RepeatedPtrField<string>&
      GetRepeated(int number, const ExtensionSet& set) {
    return *reinterpret_cast<const RepeatedPtrField<string>*>(
        set.GetRawRepeatedField(number, GetDefaultRepeatedField()));
  }

  static inline RepeatedPtrField<string>*
      MutableRepeated(int number, FieldType field_type,
                      bool is_packed, ExtensionSet* set) {
    return reinterpret_cast<RepeatedPtrField<string>*>(
        set->MutableRawRepeatedField(number, field_type,
                                     is_packed, NULL));
  }

  static const RepeatedFieldType* GetDefaultRepeatedField() {
    return default_repeated_field_;
  }

 private:
  friend void InitializeDefaultRepeatedFields();
  friend void DestroyDefaultRepeatedFields();
  static const RepeatedFieldType *default_repeated_field_;
};






template <typename Type, bool IsValid(int)>
class EnumTypeTraits {
 public:
  typedef Type ConstType;
  typedef Type MutableType;
  typedef EnumTypeTraits<Type, IsValid> Singular;

  static inline ConstType Get(int number, const ExtensionSet& set,
                              ConstType default_value) {
    return static_cast<Type>(set.GetEnum(number, default_value));
  }
  static inline void Set(int number, FieldType field_type,
                         ConstType value, ExtensionSet* set) {
    GOOGLE_DCHECK(IsValid(value));
    set->SetEnum(number, field_type, value, NULL);
  }
};

template <typename Type, bool IsValid(int)>
class RepeatedEnumTypeTraits {
 public:
  typedef Type ConstType;
  typedef Type MutableType;
  typedef RepeatedEnumTypeTraits<Type, IsValid> Repeated;

  typedef RepeatedField<Type> RepeatedFieldType;

  static inline ConstType Get(int number, const ExtensionSet& set, int index) {
    return static_cast<Type>(set.GetRepeatedEnum(number, index));
  }
  static inline void Set(int number, int index,
                         ConstType value, ExtensionSet* set) {
    GOOGLE_DCHECK(IsValid(value));
    set->SetRepeatedEnum(number, index, value);
  }
  static inline void Add(int number, FieldType field_type,
                         bool is_packed, ConstType value, ExtensionSet* set) {
    GOOGLE_DCHECK(IsValid(value));
    set->AddEnum(number, field_type, is_packed, value, NULL);
  }
  static inline const RepeatedField<Type>& GetRepeated(int number,
                                                       const ExtensionSet&
                                                       set) {
    
    
    
    
    return *reinterpret_cast<const RepeatedField<Type>*>(
        set.GetRawRepeatedField(number, GetDefaultRepeatedField()));
  }

  static inline RepeatedField<Type>* MutableRepeated(int number,
                                                     FieldType field_type,
                                                     bool is_packed,
                                                     ExtensionSet* set) {
    return reinterpret_cast<RepeatedField<Type>*>(
        set->MutableRawRepeatedField(number, field_type, is_packed, NULL));
  }

  static const RepeatedFieldType* GetDefaultRepeatedField() {
    
    
    
    
    
    
    return reinterpret_cast<const RepeatedField<Type>*>(
        RepeatedPrimitiveTypeTraits<int32>::GetDefaultRepeatedField());
  }
};







template <typename Type>
class MessageTypeTraits {
 public:
  typedef const Type& ConstType;
  typedef Type* MutableType;
  typedef MessageTypeTraits<Type> Singular;

  static inline ConstType Get(int number, const ExtensionSet& set,
                              ConstType default_value) {
    return static_cast<const Type&>(
        set.GetMessage(number, default_value));
  }
  static inline MutableType Mutable(int number, FieldType field_type,
                                    ExtensionSet* set) {
    return static_cast<Type*>(
      set->MutableMessage(number, field_type, Type::default_instance(), NULL));
  }
  static inline void SetAllocated(int number, FieldType field_type,
                                  MutableType message, ExtensionSet* set) {
    set->SetAllocatedMessage(number, field_type, NULL, message);
  }
  static inline MutableType Release(int number, FieldType ,
                                    ExtensionSet* set) {
    return static_cast<Type*>(set->ReleaseMessage(
        number, Type::default_instance()));
  }
};


class RepeatedMessageGenericTypeTraits;

template <typename Type>
class RepeatedMessageTypeTraits {
 public:
  typedef const Type& ConstType;
  typedef Type* MutableType;
  typedef RepeatedMessageTypeTraits<Type> Repeated;

  typedef RepeatedPtrField<Type> RepeatedFieldType;

  static inline ConstType Get(int number, const ExtensionSet& set, int index) {
    return static_cast<const Type&>(set.GetRepeatedMessage(number, index));
  }
  static inline MutableType Mutable(int number, int index, ExtensionSet* set) {
    return static_cast<Type*>(set->MutableRepeatedMessage(number, index));
  }
  static inline MutableType Add(int number, FieldType field_type,
                                ExtensionSet* set) {
    return static_cast<Type*>(
        set->AddMessage(number, field_type, Type::default_instance(), NULL));
  }
  static inline const RepeatedPtrField<Type>& GetRepeated(int number,
                                                          const ExtensionSet&
                                                          set) {
    
    
    
    
    
    return *reinterpret_cast<const RepeatedPtrField<Type>*>(
        set.GetRawRepeatedField(number, GetDefaultRepeatedField()));
  }
  static inline RepeatedPtrField<Type>* MutableRepeated(int number,
                                                        FieldType field_type,
                                                        bool is_packed,
                                                        ExtensionSet* set) {
    return reinterpret_cast<RepeatedPtrField<Type>*>(
        set->MutableRawRepeatedField(number, field_type, is_packed, NULL));
  }

  static const RepeatedFieldType* GetDefaultRepeatedField();
};



class LIBPROTOBUF_EXPORT RepeatedMessageGenericTypeTraits {
 public:
  typedef RepeatedPtrField< ::google::protobuf::MessageLite*> RepeatedFieldType;
 private:
  template<typename Type> friend class RepeatedMessageTypeTraits;
  friend void InitializeDefaultRepeatedFields();
  friend void DestroyDefaultRepeatedFields();
  static const RepeatedFieldType* default_repeated_field_;
};

template<typename Type> inline
    const typename RepeatedMessageTypeTraits<Type>::RepeatedFieldType*
    RepeatedMessageTypeTraits<Type>::GetDefaultRepeatedField() {
  return reinterpret_cast<const RepeatedFieldType*>(
      RepeatedMessageGenericTypeTraits::default_repeated_field_);
}


















template <typename ExtendeeType, typename TypeTraitsType,
          FieldType field_type, bool is_packed>
class ExtensionIdentifier {
 public:
  typedef TypeTraitsType TypeTraits;
  typedef ExtendeeType Extendee;

  ExtensionIdentifier(int number, typename TypeTraits::ConstType default_value)
      : number_(number), default_value_(default_value) {}
  inline int number() const { return number_; }
  typename TypeTraits::ConstType default_value() const {
    return default_value_;
  }

 private:
  const int number_;
  typename TypeTraits::ConstType default_value_;
};














#define GOOGLE_PROTOBUF_EXTENSION_ACCESSORS(CLASSNAME)                        \
  /* Has, Size, Clear */                                                      \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline bool HasExtension(                                                   \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) const {   \
    return _extensions_.Has(id.number());                                     \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline void ClearExtension(                                                 \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) {         \
    _extensions_.ClearExtension(id.number());                                 \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline int ExtensionSize(                                                   \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) const {   \
    return _extensions_.ExtensionSize(id.number());                           \
  }                                                                           \
                                                                              \
  /* Singular accessors */                                                    \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Singular::ConstType GetExtension(        \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) const {   \
    return _proto_TypeTraits::Get(id.number(), _extensions_,                  \
                                  id.default_value());                        \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Singular::MutableType MutableExtension(  \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) {         \
    return _proto_TypeTraits::Mutable(id.number(), _field_type,               \
                                      &_extensions_);                         \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline void SetExtension(                                                   \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      typename _proto_TypeTraits::Singular::ConstType value) {                \
    _proto_TypeTraits::Set(id.number(), _field_type, value, &_extensions_);   \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline void SetAllocatedExtension(                                          \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      typename _proto_TypeTraits::Singular::MutableType value) {              \
    _proto_TypeTraits::SetAllocated(id.number(), _field_type,                 \
                                    value, &_extensions_);                    \
  }                                                                           \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Singular::MutableType ReleaseExtension(  \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) {         \
    return _proto_TypeTraits::Release(id.number(), _field_type,               \
                                      &_extensions_);                         \
  }                                                                           \
                                                                              \
  /* Repeated accessors */                                                    \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Repeated::ConstType GetExtension(        \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      int index) const {                                                      \
    return _proto_TypeTraits::Get(id.number(), _extensions_, index);          \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Repeated::MutableType MutableExtension(  \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      int index) {                                                            \
    return _proto_TypeTraits::Mutable(id.number(), index, &_extensions_);     \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline void SetExtension(                                                   \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      int index, typename _proto_TypeTraits::Repeated::ConstType value) {     \
    _proto_TypeTraits::Set(id.number(), index, value, &_extensions_);         \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Repeated::MutableType AddExtension(      \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id) {         \
    return _proto_TypeTraits::Add(id.number(), _field_type, &_extensions_);   \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline void AddExtension(                                                   \
      const ::google::protobuf::internal::ExtensionIdentifier<                          \
        CLASSNAME, _proto_TypeTraits, _field_type, _is_packed>& id,           \
      typename _proto_TypeTraits::Repeated::ConstType value) {                \
    _proto_TypeTraits::Add(id.number(), _field_type, _is_packed,              \
                           value, &_extensions_);                             \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline const typename _proto_TypeTraits::Repeated::RepeatedFieldType&       \
      GetRepeatedExtension(                                                   \
          const ::google::protobuf::internal::ExtensionIdentifier<                      \
            CLASSNAME, _proto_TypeTraits, _field_type,                        \
            _is_packed>& id) const {                                          \
    return _proto_TypeTraits::GetRepeated(id.number(), _extensions_);         \
  }                                                                           \
                                                                              \
  template <typename _proto_TypeTraits,                                       \
            ::google::protobuf::internal::FieldType _field_type,                        \
            bool _is_packed>                                                  \
  inline typename _proto_TypeTraits::Repeated::RepeatedFieldType*             \
      MutableRepeatedExtension(                                               \
          const ::google::protobuf::internal::ExtensionIdentifier<                      \
              CLASSNAME, _proto_TypeTraits, _field_type,                      \
              _is_packed>& id) {                                              \
    return _proto_TypeTraits::MutableRepeated(id.number(), _field_type,       \
                                              _is_packed, &_extensions_);     \
  }

}  
}  

}  
#endif  
