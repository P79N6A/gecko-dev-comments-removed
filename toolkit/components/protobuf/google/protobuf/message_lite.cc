


































#include <google/protobuf/message_lite.h>
#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/stubs/stl_util-inl.h>

namespace google {
namespace protobuf {

MessageLite::~MessageLite() {}

string MessageLite::InitializationErrorString() const {
  return "(cannot determine missing fields for lite message)";
}

namespace {







void ByteSizeConsistencyError(int byte_size_before_serialization,
                              int byte_size_after_serialization,
                              int bytes_produced_by_serialization) {
  GOOGLE_CHECK_EQ(byte_size_before_serialization, byte_size_after_serialization)
      << "Protocol message was modified concurrently during serialization.";
  GOOGLE_CHECK_EQ(bytes_produced_by_serialization, byte_size_before_serialization)
      << "Byte size calculation and serialization were inconsistent.  This "
         "may indicate a bug in protocol buffers or it may be caused by "
         "concurrent modification of the message.";
  GOOGLE_LOG(FATAL) << "This shouldn't be called if all the sizes are equal.";
}

string InitializationErrorMessage(const char* action,
                                  const MessageLite& message) {
  
  
  
  
  
  
  
  

  string result;
  result += "Can't ";
  result += action;
  result += " message of type \"";
  result += message.GetTypeName();
  result += "\" because it is missing required fields: ";
  result += message.InitializationErrorString();
  return result;
}











inline bool InlineMergeFromCodedStream(io::CodedInputStream* input,
                                       MessageLite* message)
                                       GOOGLE_ATTRIBUTE_ALWAYS_INLINE;
inline bool InlineParseFromCodedStream(io::CodedInputStream* input,
                                       MessageLite* message)
                                       GOOGLE_ATTRIBUTE_ALWAYS_INLINE;
inline bool InlineParsePartialFromCodedStream(io::CodedInputStream* input,
                                              MessageLite* message)
                                              GOOGLE_ATTRIBUTE_ALWAYS_INLINE;
inline bool InlineParseFromArray(const void* data, int size,
                                 MessageLite* message)
                                 GOOGLE_ATTRIBUTE_ALWAYS_INLINE;
inline bool InlineParsePartialFromArray(const void* data, int size,
                                        MessageLite* message)
                                        GOOGLE_ATTRIBUTE_ALWAYS_INLINE;

bool InlineMergeFromCodedStream(io::CodedInputStream* input,
                                MessageLite* message) {
  if (!message->MergePartialFromCodedStream(input)) return false;
  if (!message->IsInitialized()) {
    GOOGLE_LOG(ERROR) << InitializationErrorMessage("parse", *message);
    return false;
  }
  return true;
}

bool InlineParseFromCodedStream(io::CodedInputStream* input,
                                MessageLite* message) {
  message->Clear();
  return InlineMergeFromCodedStream(input, message);
}

bool InlineParsePartialFromCodedStream(io::CodedInputStream* input,
                                       MessageLite* message) {
  message->Clear();
  return message->MergePartialFromCodedStream(input);
}

bool InlineParseFromArray(const void* data, int size, MessageLite* message) {
  io::CodedInputStream input(reinterpret_cast<const uint8*>(data), size);
  return InlineParseFromCodedStream(&input, message) &&
         input.ConsumedEntireMessage();
}

bool InlineParsePartialFromArray(const void* data, int size,
                                 MessageLite* message) {
  io::CodedInputStream input(reinterpret_cast<const uint8*>(data), size);
  return InlineParsePartialFromCodedStream(&input, message) &&
         input.ConsumedEntireMessage();
}

}  

bool MessageLite::MergeFromCodedStream(io::CodedInputStream* input) {
  return InlineMergeFromCodedStream(input, this);
}

bool MessageLite::ParseFromCodedStream(io::CodedInputStream* input) {
  return InlineParseFromCodedStream(input, this);
}

bool MessageLite::ParsePartialFromCodedStream(io::CodedInputStream* input) {
  return InlineParsePartialFromCodedStream(input, this);
}

bool MessageLite::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParseFromCodedStream(&decoder) && decoder.ConsumedEntireMessage();
}

bool MessageLite::ParsePartialFromZeroCopyStream(
    io::ZeroCopyInputStream* input) {
  io::CodedInputStream decoder(input);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage();
}

bool MessageLite::ParseFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParseFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool MessageLite::ParsePartialFromBoundedZeroCopyStream(
    io::ZeroCopyInputStream* input, int size) {
  io::CodedInputStream decoder(input);
  decoder.PushLimit(size);
  return ParsePartialFromCodedStream(&decoder) &&
         decoder.ConsumedEntireMessage() &&
         decoder.BytesUntilLimit() == 0;
}

bool MessageLite::ParseFromString(const string& data) {
  return InlineParseFromArray(data.data(), data.size(), this);
}

bool MessageLite::ParsePartialFromString(const string& data) {
  return InlineParsePartialFromArray(data.data(), data.size(), this);
}

bool MessageLite::ParseFromArray(const void* data, int size) {
  return InlineParseFromArray(data, size, this);
}

bool MessageLite::ParsePartialFromArray(const void* data, int size) {
  return InlineParsePartialFromArray(data, size, this);
}




uint8* MessageLite::SerializeWithCachedSizesToArray(uint8* target) const {
  
  
  int size = GetCachedSize();
  io::ArrayOutputStream out(target, size);
  io::CodedOutputStream coded_out(&out);
  SerializeWithCachedSizes(&coded_out);
  GOOGLE_CHECK(!coded_out.HadError());
  return target + size;
}

bool MessageLite::SerializeToCodedStream(io::CodedOutputStream* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return SerializePartialToCodedStream(output);
}

bool MessageLite::SerializePartialToCodedStream(
    io::CodedOutputStream* output) const {
  const int size = ByteSize();  
  uint8* buffer = output->GetDirectBufferForNBytesAndAdvance(size);
  if (buffer != NULL) {
    uint8* end = SerializeWithCachedSizesToArray(buffer);
    if (end - buffer != size) {
      ByteSizeConsistencyError(size, ByteSize(), end - buffer);
    }
    return true;
  } else {
    int original_byte_count = output->ByteCount();
    SerializeWithCachedSizes(output);
    if (output->HadError()) {
      return false;
    }
    int final_byte_count = output->ByteCount();

    if (final_byte_count - original_byte_count != size) {
      ByteSizeConsistencyError(size, ByteSize(),
                               final_byte_count - original_byte_count);
    }

    return true;
  }
}

bool MessageLite::SerializeToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializeToCodedStream(&encoder);
}

bool MessageLite::SerializePartialToZeroCopyStream(
    io::ZeroCopyOutputStream* output) const {
  io::CodedOutputStream encoder(output);
  return SerializePartialToCodedStream(&encoder);
}

bool MessageLite::AppendToString(string* output) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return AppendPartialToString(output);
}

bool MessageLite::AppendPartialToString(string* output) const {
  int old_size = output->size();
  int byte_size = ByteSize();
  STLStringResizeUninitialized(output, old_size + byte_size);
  uint8* start = reinterpret_cast<uint8*>(string_as_array(output) + old_size);
  uint8* end = SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
    ByteSizeConsistencyError(byte_size, ByteSize(), end - start);
  }
  return true;
}

bool MessageLite::SerializeToString(string* output) const {
  output->clear();
  return AppendToString(output);
}

bool MessageLite::SerializePartialToString(string* output) const {
  output->clear();
  return AppendPartialToString(output);
}

bool MessageLite::SerializeToArray(void* data, int size) const {
  GOOGLE_DCHECK(IsInitialized()) << InitializationErrorMessage("serialize", *this);
  return SerializePartialToArray(data, size);
}

bool MessageLite::SerializePartialToArray(void* data, int size) const {
  int byte_size = ByteSize();
  if (size < byte_size) return false;
  uint8* start = reinterpret_cast<uint8*>(data);
  uint8* end = SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
    ByteSizeConsistencyError(byte_size, ByteSize(), end - start);
  }
  return true;
}

string MessageLite::SerializeAsString() const {
  
  
  
  
  string output;
  if (!AppendToString(&output))
    output.clear();
  return output;
}

string MessageLite::SerializePartialAsString() const {
  string output;
  if (!AppendPartialToString(&output))
    output.clear();
  return output;
}

}  
}  
