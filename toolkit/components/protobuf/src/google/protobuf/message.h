












































































































#ifndef GOOGLE_PROTOBUF_MESSAGE_H__
#define GOOGLE_PROTOBUF_MESSAGE_H__

#include <iosfwd>
#include <string>
#include <vector>

#include <google/protobuf/message_lite.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>


#define GOOGLE_PROTOBUF_HAS_ONEOF

namespace google {
namespace protobuf {


class Message;
class Reflection;
class MessageFactory;


class UnknownFieldSet;         
namespace io {
  class ZeroCopyInputStream;   
  class ZeroCopyOutputStream;  
  class CodedInputStream;      
  class CodedOutputStream;     
}


template<typename T>
class RepeatedField;     

template<typename T>
class RepeatedPtrField;  


struct Metadata {
  const Descriptor* descriptor;
  const Reflection* reflection;
};











class LIBPROTOBUF_EXPORT Message : public MessageLite {
 public:
  inline Message() {}
  virtual ~Message();

  

  
  
  
  virtual Message* New() const = 0;

  
  
  
  virtual void CopyFrom(const Message& from);

  
  
  
  
  
  virtual void MergeFrom(const Message& from);

  
  
  void CheckInitialized() const;

  
  
  
  
  void FindInitializationErrors(vector<string>* errors) const;

  
  
  string InitializationErrorString() const;

  
  
  
  
  
  
  
  
  
  
  virtual void DiscardUnknownFields();

  
  
  
  virtual int SpaceUsed() const;

  

  
  
  string DebugString() const;
  
  string ShortDebugString() const;
  
  string Utf8DebugString() const;
  
  void PrintDebugString() const;

  
  
  

  
  
  bool ParseFromFileDescriptor(int file_descriptor);
  
  
  bool ParsePartialFromFileDescriptor(int file_descriptor);
  
  
  bool ParseFromIstream(istream* input);
  
  
  bool ParsePartialFromIstream(istream* input);

  
  
  bool SerializeToFileDescriptor(int file_descriptor) const;
  
  bool SerializePartialToFileDescriptor(int file_descriptor) const;
  
  
  bool SerializeToOstream(ostream* output) const;
  
  bool SerializePartialToOstream(ostream* output) const;


  
  
  

  virtual string GetTypeName() const;
  virtual void Clear();
  virtual bool IsInitialized() const;
  virtual void CheckTypeAndMergeFrom(const MessageLite& other);
  virtual bool MergePartialFromCodedStream(io::CodedInputStream* input);
  virtual int ByteSize() const;
  virtual void SerializeWithCachedSizes(io::CodedOutputStream* output) const;

 private:
  
  
  
  
  
  
  
  virtual void SetCachedSize(int size) const;

 public:

  

  
  typedef google::protobuf::Reflection Reflection;

  
  
  const Descriptor* GetDescriptor() const { return GetMetadata().descriptor; }

  
  
  
  
  
  
  
  virtual const Reflection* GetReflection() const {
    return GetMetadata().reflection;
  }

 protected:
  
  
  
  virtual Metadata GetMetadata() const  = 0;


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Message);
};










































class LIBPROTOBUF_EXPORT Reflection {
 public:
  inline Reflection() {}
  virtual ~Reflection();

  
  
  
  virtual const UnknownFieldSet& GetUnknownFields(
      const Message& message) const = 0;
  
  
  
  virtual UnknownFieldSet* MutableUnknownFields(Message* message) const = 0;

  
  virtual int SpaceUsed(const Message& message) const = 0;

  
  virtual bool HasField(const Message& message,
                        const FieldDescriptor* field) const = 0;

  
  virtual int FieldSize(const Message& message,
                        const FieldDescriptor* field) const = 0;

  
  
  virtual void ClearField(Message* message,
                          const FieldDescriptor* field) const = 0;

  
  
  
  
  virtual bool HasOneof(const Message& message,
                        const OneofDescriptor* oneof_descriptor) const {
    return false;
  }

  virtual void ClearOneof(Message* message,
                          const OneofDescriptor* oneof_descriptor) const {}

  
  
  virtual const FieldDescriptor* GetOneofFieldDescriptor(
      const Message& message,
      const OneofDescriptor* oneof_descriptor) const {
    return NULL;
  }

  
  
  
  
  
  
  
  virtual void RemoveLast(Message* message,
                          const FieldDescriptor* field) const = 0;
  
  
  virtual Message* ReleaseLast(Message* message,
                               const FieldDescriptor* field) const = 0;

  
  virtual void Swap(Message* message1, Message* message2) const = 0;

  
  virtual void SwapFields(Message* message1,
                          Message* message2,
                          const vector<const FieldDescriptor*>& fields)
      const = 0;

  
  virtual void SwapElements(Message* message,
                            const FieldDescriptor* field,
                            int index1,
                            int index2) const = 0;

  
  
  
  
  
  virtual void ListFields(const Message& message,
                          vector<const FieldDescriptor*>* output) const = 0;

  
  
  

  virtual int32  GetInt32 (const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual int64  GetInt64 (const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual uint32 GetUInt32(const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual uint64 GetUInt64(const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual float  GetFloat (const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual double GetDouble(const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual bool   GetBool  (const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual string GetString(const Message& message,
                           const FieldDescriptor* field) const = 0;
  virtual const EnumValueDescriptor* GetEnum(
      const Message& message, const FieldDescriptor* field) const = 0;
  
  virtual const Message& GetMessage(const Message& message,
                                    const FieldDescriptor* field,
                                    MessageFactory* factory = NULL) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual const string& GetStringReference(const Message& message,
                                           const FieldDescriptor* field,
                                           string* scratch) const = 0;


  
  

  virtual void SetInt32 (Message* message,
                         const FieldDescriptor* field, int32  value) const = 0;
  virtual void SetInt64 (Message* message,
                         const FieldDescriptor* field, int64  value) const = 0;
  virtual void SetUInt32(Message* message,
                         const FieldDescriptor* field, uint32 value) const = 0;
  virtual void SetUInt64(Message* message,
                         const FieldDescriptor* field, uint64 value) const = 0;
  virtual void SetFloat (Message* message,
                         const FieldDescriptor* field, float  value) const = 0;
  virtual void SetDouble(Message* message,
                         const FieldDescriptor* field, double value) const = 0;
  virtual void SetBool  (Message* message,
                         const FieldDescriptor* field, bool   value) const = 0;
  virtual void SetString(Message* message,
                         const FieldDescriptor* field,
                         const string& value) const = 0;
  virtual void SetEnum  (Message* message,
                         const FieldDescriptor* field,
                         const EnumValueDescriptor* value) const = 0;
  
  
  
  
  
  
  
  
  
  
  virtual Message* MutableMessage(Message* message,
                                  const FieldDescriptor* field,
                                  MessageFactory* factory = NULL) const = 0;
  
  
  
  
  virtual void SetAllocatedMessage(Message* message,
                                   Message* sub_message,
                                   const FieldDescriptor* field) const = 0;
  
  
  
  
  
  
  
  virtual Message* ReleaseMessage(Message* message,
                                  const FieldDescriptor* field,
                                  MessageFactory* factory = NULL) const = 0;


  
  

  virtual int32  GetRepeatedInt32 (const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual int64  GetRepeatedInt64 (const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual uint32 GetRepeatedUInt32(const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual uint64 GetRepeatedUInt64(const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual float  GetRepeatedFloat (const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual double GetRepeatedDouble(const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual bool   GetRepeatedBool  (const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual string GetRepeatedString(const Message& message,
                                   const FieldDescriptor* field,
                                   int index) const = 0;
  virtual const EnumValueDescriptor* GetRepeatedEnum(
      const Message& message,
      const FieldDescriptor* field, int index) const = 0;
  virtual const Message& GetRepeatedMessage(
      const Message& message,
      const FieldDescriptor* field, int index) const = 0;

  
  virtual const string& GetRepeatedStringReference(
      const Message& message, const FieldDescriptor* field,
      int index, string* scratch) const = 0;


  
  

  virtual void SetRepeatedInt32 (Message* message,
                                 const FieldDescriptor* field,
                                 int index, int32  value) const = 0;
  virtual void SetRepeatedInt64 (Message* message,
                                 const FieldDescriptor* field,
                                 int index, int64  value) const = 0;
  virtual void SetRepeatedUInt32(Message* message,
                                 const FieldDescriptor* field,
                                 int index, uint32 value) const = 0;
  virtual void SetRepeatedUInt64(Message* message,
                                 const FieldDescriptor* field,
                                 int index, uint64 value) const = 0;
  virtual void SetRepeatedFloat (Message* message,
                                 const FieldDescriptor* field,
                                 int index, float  value) const = 0;
  virtual void SetRepeatedDouble(Message* message,
                                 const FieldDescriptor* field,
                                 int index, double value) const = 0;
  virtual void SetRepeatedBool  (Message* message,
                                 const FieldDescriptor* field,
                                 int index, bool   value) const = 0;
  virtual void SetRepeatedString(Message* message,
                                 const FieldDescriptor* field,
                                 int index, const string& value) const = 0;
  virtual void SetRepeatedEnum(Message* message,
                               const FieldDescriptor* field, int index,
                               const EnumValueDescriptor* value) const = 0;
  
  
  virtual Message* MutableRepeatedMessage(
      Message* message, const FieldDescriptor* field, int index) const = 0;


  
  

  virtual void AddInt32 (Message* message,
                         const FieldDescriptor* field, int32  value) const = 0;
  virtual void AddInt64 (Message* message,
                         const FieldDescriptor* field, int64  value) const = 0;
  virtual void AddUInt32(Message* message,
                         const FieldDescriptor* field, uint32 value) const = 0;
  virtual void AddUInt64(Message* message,
                         const FieldDescriptor* field, uint64 value) const = 0;
  virtual void AddFloat (Message* message,
                         const FieldDescriptor* field, float  value) const = 0;
  virtual void AddDouble(Message* message,
                         const FieldDescriptor* field, double value) const = 0;
  virtual void AddBool  (Message* message,
                         const FieldDescriptor* field, bool   value) const = 0;
  virtual void AddString(Message* message,
                         const FieldDescriptor* field,
                         const string& value) const = 0;
  virtual void AddEnum  (Message* message,
                         const FieldDescriptor* field,
                         const EnumValueDescriptor* value) const = 0;
  
  virtual Message* AddMessage(Message* message,
                              const FieldDescriptor* field,
                              MessageFactory* factory = NULL) const = 0;


  
  
  
  
  
  
  
  
  

  
  template<typename T>
  const RepeatedField<T>& GetRepeatedField(
      const Message&, const FieldDescriptor*) const;

  
  template<typename T>
  RepeatedField<T>* MutableRepeatedField(
      Message*, const FieldDescriptor*) const;

  
  
  template<typename T>
  const RepeatedPtrField<T>& GetRepeatedPtrField(
      const Message&, const FieldDescriptor*) const;

  
  
  template<typename T>
  RepeatedPtrField<T>* MutableRepeatedPtrField(
      Message*, const FieldDescriptor*) const;

  

  
  
  virtual const FieldDescriptor* FindKnownExtensionByName(
      const string& name) const = 0;

  
  
  virtual const FieldDescriptor* FindKnownExtensionByNumber(
      int number) const = 0;

  

 protected:
  
  
  
  
  
  virtual void* MutableRawRepeatedField(
      Message* message, const FieldDescriptor* field, FieldDescriptor::CppType,
      int ctype, const Descriptor* message_type) const = 0;

 private:
  
  
  
  
  void* MutableRawRepeatedString(
      Message* message, const FieldDescriptor* field, bool is_string) const;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Reflection);
};


class LIBPROTOBUF_EXPORT MessageFactory {
 public:
  inline MessageFactory() {}
  virtual ~MessageFactory();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual const Message* GetPrototype(const Descriptor* type) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static MessageFactory* generated_factory();

  
  
  
  
  
  
  
  
  static void InternalRegisterGeneratedFile(
      const char* filename, void (*register_messages)(const string&));

  
  
  
  static void InternalRegisterGeneratedMessage(const Descriptor* descriptor,
                                               const Message* prototype);


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageFactory);
};

#define DECLARE_GET_REPEATED_FIELD(TYPE)                         \
template<>                                                       \
LIBPROTOBUF_EXPORT                                               \
const RepeatedField<TYPE>& Reflection::GetRepeatedField<TYPE>(   \
    const Message& message, const FieldDescriptor* field) const; \
                                                                 \
template<>                                                       \
RepeatedField<TYPE>* Reflection::MutableRepeatedField<TYPE>(     \
    Message* message, const FieldDescriptor* field) const;

DECLARE_GET_REPEATED_FIELD(int32)
DECLARE_GET_REPEATED_FIELD(int64)
DECLARE_GET_REPEATED_FIELD(uint32)
DECLARE_GET_REPEATED_FIELD(uint64)
DECLARE_GET_REPEATED_FIELD(float)
DECLARE_GET_REPEATED_FIELD(double)
DECLARE_GET_REPEATED_FIELD(bool)

#undef DECLARE_GET_REPEATED_FIELD








template<>
inline const RepeatedPtrField<string>& Reflection::GetRepeatedPtrField<string>(
    const Message& message, const FieldDescriptor* field) const {
  return *static_cast<RepeatedPtrField<string>* >(
      MutableRawRepeatedString(const_cast<Message*>(&message), field, true));
}

template<>
inline RepeatedPtrField<string>* Reflection::MutableRepeatedPtrField<string>(
    Message* message, const FieldDescriptor* field) const {
  return static_cast<RepeatedPtrField<string>* >(
      MutableRawRepeatedString(message, field, true));
}




template<>
inline const RepeatedPtrField<Message>& Reflection::GetRepeatedPtrField(
    const Message& message, const FieldDescriptor* field) const {
  return *static_cast<RepeatedPtrField<Message>* >(
      MutableRawRepeatedField(const_cast<Message*>(&message), field,
          FieldDescriptor::CPPTYPE_MESSAGE, -1,
          NULL));
}

template<>
inline RepeatedPtrField<Message>* Reflection::MutableRepeatedPtrField(
    Message* message, const FieldDescriptor* field) const {
  return static_cast<RepeatedPtrField<Message>* >(
      MutableRawRepeatedField(message, field,
          FieldDescriptor::CPPTYPE_MESSAGE, -1,
          NULL));
}

template<typename PB>
inline const RepeatedPtrField<PB>& Reflection::GetRepeatedPtrField(
    const Message& message, const FieldDescriptor* field) const {
  return *static_cast<RepeatedPtrField<PB>* >(
      MutableRawRepeatedField(const_cast<Message*>(&message), field,
          FieldDescriptor::CPPTYPE_MESSAGE, -1,
          PB::default_instance().GetDescriptor()));
}

template<typename PB>
inline RepeatedPtrField<PB>* Reflection::MutableRepeatedPtrField(
    Message* message, const FieldDescriptor* field) const {
  return static_cast<RepeatedPtrField<PB>* >(
      MutableRawRepeatedField(message, field,
          FieldDescriptor::CPPTYPE_MESSAGE, -1,
          PB::default_instance().GetDescriptor()));
}

}  

}  
#endif  
