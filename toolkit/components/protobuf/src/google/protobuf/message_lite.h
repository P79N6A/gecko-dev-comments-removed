





































#ifndef GOOGLE_PROTOBUF_MESSAGE_LITE_H__
#define GOOGLE_PROTOBUF_MESSAGE_LITE_H__

#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

namespace io {
  class CodedInputStream;
  class CodedOutputStream;
  class ZeroCopyInputStream;
  class ZeroCopyOutputStream;
}
























class LIBPROTOBUF_EXPORT MessageLite {
 public:
  inline MessageLite() {}
  virtual ~MessageLite();

  

  
  virtual string GetTypeName() const = 0;

  
  
  virtual MessageLite* New() const = 0;

  
  
  
  
  
  virtual void Clear() = 0;

  
  virtual bool IsInitialized() const = 0;

  
  
  
  virtual string InitializationErrorString() const;

  
  
  virtual void CheckTypeAndMergeFrom(const MessageLite& other) = 0;

  
  
  
  

  
  
  
  bool ParseFromCodedStream(io::CodedInputStream* input);
  
  
  bool ParsePartialFromCodedStream(io::CodedInputStream* input);
  
  
  bool ParseFromZeroCopyStream(io::ZeroCopyInputStream* input);
  
  
  bool ParsePartialFromZeroCopyStream(io::ZeroCopyInputStream* input);
  
  
  
  bool ParseFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input, int size);
  
  
  bool ParsePartialFromBoundedZeroCopyStream(io::ZeroCopyInputStream* input,
                                             int size);
  
  bool ParseFromString(const string& data);
  
  
  bool ParsePartialFromString(const string& data);
  
  bool ParseFromArray(const void* data, int size);
  
  
  bool ParsePartialFromArray(const void* data, int size);


  
  
  
  
  
  
  
  
  
  
  
  bool MergeFromCodedStream(io::CodedInputStream* input);

  
  
  
  
  
  virtual bool MergePartialFromCodedStream(io::CodedInputStream* input) = 0;


  
  
  

  
  
  
  bool SerializeToCodedStream(io::CodedOutputStream* output) const;
  
  bool SerializePartialToCodedStream(io::CodedOutputStream* output) const;
  
  
  bool SerializeToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  
  bool SerializePartialToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  
  
  bool SerializeToString(string* output) const;
  
  bool SerializePartialToString(string* output) const;
  
  
  bool SerializeToArray(void* data, int size) const;
  
  bool SerializePartialToArray(void* data, int size) const;

  
  
  
  
  
  
  string SerializeAsString() const;
  
  string SerializePartialAsString() const;

  
  
  bool AppendToString(string* output) const;
  
  bool AppendPartialToString(string* output) const;

  
  
  
  virtual int ByteSize() const = 0;

  
  
  
  virtual void SerializeWithCachedSizes(
      io::CodedOutputStream* output) const = 0;

  
  
  
  virtual uint8* SerializeWithCachedSizesToArray(uint8* target) const;

  
  
  
  
  
  
  
  
  
  
  
  virtual int GetCachedSize() const = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageLite);
};

}  

}  
#endif  
