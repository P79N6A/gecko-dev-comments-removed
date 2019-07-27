




































#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_REFLECTION_H__

#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>


#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/unknown_field_set.h>


namespace google {
namespace upb {
namespace google_opensource {
class GMR_Handlers;
}  
}  

namespace protobuf {
  class DescriptorPool;
}

namespace protobuf {
namespace internal {
class DefaultEmptyOneof;


class GeneratedMessageReflection;


class ExtensionSet;             

























class LIBPROTOBUF_EXPORT GeneratedMessageReflection : public Reflection {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  GeneratedMessageReflection(const Descriptor* descriptor,
                             const Message* default_instance,
                             const int offsets[],
                             int has_bits_offset,
                             int unknown_fields_offset,
                             int extensions_offset,
                             const DescriptorPool* pool,
                             MessageFactory* factory,
                             int object_size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  GeneratedMessageReflection(const Descriptor* descriptor,
                             const Message* default_instance,
                             const int offsets[],
                             int has_bits_offset,
                             int unknown_fields_offset,
                             int extensions_offset,
                             const void* default_oneof_instance,
                             int oneof_case_offset,
                             const DescriptorPool* pool,
                             MessageFactory* factory,
                             int object_size);
  ~GeneratedMessageReflection();

  

  const UnknownFieldSet& GetUnknownFields(const Message& message) const;
  UnknownFieldSet* MutableUnknownFields(Message* message) const;

  int SpaceUsed(const Message& message) const;

  bool HasField(const Message& message, const FieldDescriptor* field) const;
  int FieldSize(const Message& message, const FieldDescriptor* field) const;
  void ClearField(Message* message, const FieldDescriptor* field) const;
  bool HasOneof(const Message& message,
                const OneofDescriptor* oneof_descriptor) const;
  void ClearOneof(Message* message, const OneofDescriptor* field) const;
  void RemoveLast(Message* message, const FieldDescriptor* field) const;
  Message* ReleaseLast(Message* message, const FieldDescriptor* field) const;
  void Swap(Message* message1, Message* message2) const;
  void SwapFields(Message* message1, Message* message2,
                  const vector<const FieldDescriptor*>& fields) const;
  void SwapElements(Message* message, const FieldDescriptor* field,
                    int index1, int index2) const;
  void ListFields(const Message& message,
                  vector<const FieldDescriptor*>* output) const;

  int32  GetInt32 (const Message& message,
                   const FieldDescriptor* field) const;
  int64  GetInt64 (const Message& message,
                   const FieldDescriptor* field) const;
  uint32 GetUInt32(const Message& message,
                   const FieldDescriptor* field) const;
  uint64 GetUInt64(const Message& message,
                   const FieldDescriptor* field) const;
  float  GetFloat (const Message& message,
                   const FieldDescriptor* field) const;
  double GetDouble(const Message& message,
                   const FieldDescriptor* field) const;
  bool   GetBool  (const Message& message,
                   const FieldDescriptor* field) const;
  string GetString(const Message& message,
                   const FieldDescriptor* field) const;
  const string& GetStringReference(const Message& message,
                                   const FieldDescriptor* field,
                                   string* scratch) const;
  const EnumValueDescriptor* GetEnum(const Message& message,
                                     const FieldDescriptor* field) const;
  const Message& GetMessage(const Message& message,
                            const FieldDescriptor* field,
                            MessageFactory* factory = NULL) const;

  const FieldDescriptor* GetOneofFieldDescriptor(
      const Message& message,
      const OneofDescriptor* oneof_descriptor) const;

 public:
  void SetInt32 (Message* message,
                 const FieldDescriptor* field, int32  value) const;
  void SetInt64 (Message* message,
                 const FieldDescriptor* field, int64  value) const;
  void SetUInt32(Message* message,
                 const FieldDescriptor* field, uint32 value) const;
  void SetUInt64(Message* message,
                 const FieldDescriptor* field, uint64 value) const;
  void SetFloat (Message* message,
                 const FieldDescriptor* field, float  value) const;
  void SetDouble(Message* message,
                 const FieldDescriptor* field, double value) const;
  void SetBool  (Message* message,
                 const FieldDescriptor* field, bool   value) const;
  void SetString(Message* message,
                 const FieldDescriptor* field,
                 const string& value) const;
  void SetEnum  (Message* message, const FieldDescriptor* field,
                 const EnumValueDescriptor* value) const;
  Message* MutableMessage(Message* message, const FieldDescriptor* field,
                          MessageFactory* factory = NULL) const;
  void SetAllocatedMessage(Message* message,
                           Message* sub_message,
                           const FieldDescriptor* field) const;
  Message* ReleaseMessage(Message* message, const FieldDescriptor* field,
                          MessageFactory* factory = NULL) const;

  int32  GetRepeatedInt32 (const Message& message,
                           const FieldDescriptor* field, int index) const;
  int64  GetRepeatedInt64 (const Message& message,
                           const FieldDescriptor* field, int index) const;
  uint32 GetRepeatedUInt32(const Message& message,
                           const FieldDescriptor* field, int index) const;
  uint64 GetRepeatedUInt64(const Message& message,
                           const FieldDescriptor* field, int index) const;
  float  GetRepeatedFloat (const Message& message,
                           const FieldDescriptor* field, int index) const;
  double GetRepeatedDouble(const Message& message,
                           const FieldDescriptor* field, int index) const;
  bool   GetRepeatedBool  (const Message& message,
                           const FieldDescriptor* field, int index) const;
  string GetRepeatedString(const Message& message,
                           const FieldDescriptor* field, int index) const;
  const string& GetRepeatedStringReference(const Message& message,
                                           const FieldDescriptor* field,
                                           int index, string* scratch) const;
  const EnumValueDescriptor* GetRepeatedEnum(const Message& message,
                                             const FieldDescriptor* field,
                                             int index) const;
  const Message& GetRepeatedMessage(const Message& message,
                                    const FieldDescriptor* field,
                                    int index) const;

  
  void SetRepeatedInt32 (Message* message,
                         const FieldDescriptor* field, int index, int32  value) const;
  void SetRepeatedInt64 (Message* message,
                         const FieldDescriptor* field, int index, int64  value) const;
  void SetRepeatedUInt32(Message* message,
                         const FieldDescriptor* field, int index, uint32 value) const;
  void SetRepeatedUInt64(Message* message,
                         const FieldDescriptor* field, int index, uint64 value) const;
  void SetRepeatedFloat (Message* message,
                         const FieldDescriptor* field, int index, float  value) const;
  void SetRepeatedDouble(Message* message,
                         const FieldDescriptor* field, int index, double value) const;
  void SetRepeatedBool  (Message* message,
                         const FieldDescriptor* field, int index, bool   value) const;
  void SetRepeatedString(Message* message,
                         const FieldDescriptor* field, int index,
                         const string& value) const;
  void SetRepeatedEnum(Message* message, const FieldDescriptor* field,
                       int index, const EnumValueDescriptor* value) const;
  
  Message* MutableRepeatedMessage(Message* message,
                                  const FieldDescriptor* field,
                                  int index) const;

  void AddInt32 (Message* message,
                 const FieldDescriptor* field, int32  value) const;
  void AddInt64 (Message* message,
                 const FieldDescriptor* field, int64  value) const;
  void AddUInt32(Message* message,
                 const FieldDescriptor* field, uint32 value) const;
  void AddUInt64(Message* message,
                 const FieldDescriptor* field, uint64 value) const;
  void AddFloat (Message* message,
                 const FieldDescriptor* field, float  value) const;
  void AddDouble(Message* message,
                 const FieldDescriptor* field, double value) const;
  void AddBool  (Message* message,
                 const FieldDescriptor* field, bool   value) const;
  void AddString(Message* message,
                 const FieldDescriptor* field, const string& value) const;
  void AddEnum(Message* message,
               const FieldDescriptor* field,
               const EnumValueDescriptor* value) const;
  Message* AddMessage(Message* message, const FieldDescriptor* field,
                      MessageFactory* factory = NULL) const;

  const FieldDescriptor* FindKnownExtensionByName(const string& name) const;
  const FieldDescriptor* FindKnownExtensionByNumber(int number) const;

 protected:
  virtual void* MutableRawRepeatedField(
      Message* message, const FieldDescriptor* field, FieldDescriptor::CppType,
      int ctype, const Descriptor* desc) const;

 private:
  friend class GeneratedMessage;

  
  
  friend class LIBPROTOBUF_EXPORT upb::google_opensource::GMR_Handlers;

  const Descriptor* descriptor_;
  const Message* default_instance_;
  const void* default_oneof_instance_;
  const int* offsets_;

  int has_bits_offset_;
  int oneof_case_offset_;
  int unknown_fields_offset_;
  int extensions_offset_;
  int object_size_;

  const DescriptorPool* descriptor_pool_;
  MessageFactory* message_factory_;

  template <typename Type>
  inline const Type& GetRaw(const Message& message,
                            const FieldDescriptor* field) const;
  template <typename Type>
  inline Type* MutableRaw(Message* message,
                          const FieldDescriptor* field) const;
  template <typename Type>
  inline const Type& DefaultRaw(const FieldDescriptor* field) const;
  template <typename Type>
  inline const Type& DefaultOneofRaw(const FieldDescriptor* field) const;

  inline const uint32* GetHasBits(const Message& message) const;
  inline uint32* MutableHasBits(Message* message) const;
  inline uint32 GetOneofCase(
      const Message& message,
      const OneofDescriptor* oneof_descriptor) const;
  inline uint32* MutableOneofCase(
      Message* message,
      const OneofDescriptor* oneof_descriptor) const;
  inline const ExtensionSet& GetExtensionSet(const Message& message) const;
  inline ExtensionSet* MutableExtensionSet(Message* message) const;

  inline bool HasBit(const Message& message,
                     const FieldDescriptor* field) const;
  inline void SetBit(Message* message,
                     const FieldDescriptor* field) const;
  inline void ClearBit(Message* message,
                       const FieldDescriptor* field) const;
  inline void SwapBit(Message* message1,
                      Message* message2,
                      const FieldDescriptor* field) const;

  
  
  void SwapField(Message* message1,
                 Message* message2,
                 const FieldDescriptor* field) const;

  void SwapOneofField(Message* message1,
                      Message* message2,
                      const OneofDescriptor* oneof_descriptor) const;

  inline bool HasOneofField(const Message& message,
                            const FieldDescriptor* field) const;
  inline void SetOneofCase(Message* message,
                           const FieldDescriptor* field) const;
  inline void ClearOneofField(Message* message,
                              const FieldDescriptor* field) const;

  template <typename Type>
  inline const Type& GetField(const Message& message,
                              const FieldDescriptor* field) const;
  template <typename Type>
  inline void SetField(Message* message,
                       const FieldDescriptor* field, const Type& value) const;
  template <typename Type>
  inline Type* MutableField(Message* message,
                            const FieldDescriptor* field) const;
  template <typename Type>
  inline const Type& GetRepeatedField(const Message& message,
                                      const FieldDescriptor* field,
                                      int index) const;
  template <typename Type>
  inline const Type& GetRepeatedPtrField(const Message& message,
                                         const FieldDescriptor* field,
                                         int index) const;
  template <typename Type>
  inline void SetRepeatedField(Message* message,
                               const FieldDescriptor* field, int index,
                               Type value) const;
  template <typename Type>
  inline Type* MutableRepeatedField(Message* message,
                                    const FieldDescriptor* field,
                                    int index) const;
  template <typename Type>
  inline void AddField(Message* message,
                       const FieldDescriptor* field, const Type& value) const;
  template <typename Type>
  inline Type* AddField(Message* message,
                        const FieldDescriptor* field) const;

  int GetExtensionNumberOrDie(const Descriptor* type) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(GeneratedMessageReflection);
};














#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(TYPE, FIELD)    \
  static_cast<int>(                                           \
      reinterpret_cast<const char*>(                          \
          &reinterpret_cast<const TYPE*>(16)->FIELD) -        \
      reinterpret_cast<const char*>(16))

#define PROTO2_GENERATED_DEFAULT_ONEOF_FIELD_OFFSET(ONEOF, FIELD)     \
  static_cast<int>(                                                   \
      reinterpret_cast<const char*>(&(ONEOF->FIELD))                  \
      - reinterpret_cast<const char*>(ONEOF))
























template<typename To, typename From>
inline To dynamic_cast_if_available(From from) {
#if defined(GOOGLE_PROTOBUF_NO_RTTI) || (defined(_MSC_VER)&&!defined(_CPPRTTI))
  return NULL;
#else
  return dynamic_cast<To>(from);
#endif
}

}  
}  

}  
#endif  
