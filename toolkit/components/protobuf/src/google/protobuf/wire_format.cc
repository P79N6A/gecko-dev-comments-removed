

































#include <stack>
#include <string>
#include <vector>

#include <google/protobuf/wire_format.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/unknown_field_set.h>



namespace google {
namespace protobuf {
namespace internal {

namespace {


inline int GetEnumNumber(const EnumValueDescriptor* descriptor) {
  return descriptor->number();
}

}  



bool UnknownFieldSetFieldSkipper::SkipField(
    io::CodedInputStream* input, uint32 tag) {
  return WireFormat::SkipField(input, tag, unknown_fields_);
}

bool UnknownFieldSetFieldSkipper::SkipMessage(io::CodedInputStream* input) {
  return WireFormat::SkipMessage(input, unknown_fields_);
}

void UnknownFieldSetFieldSkipper::SkipUnknownEnum(
    int field_number, int value) {
  unknown_fields_->AddVarint(field_number, value);
}

bool WireFormat::SkipField(io::CodedInputStream* input, uint32 tag,
                           UnknownFieldSet* unknown_fields) {
  int number = WireFormatLite::GetTagFieldNumber(tag);

  switch (WireFormatLite::GetTagWireType(tag)) {
    case WireFormatLite::WIRETYPE_VARINT: {
      uint64 value;
      if (!input->ReadVarint64(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddVarint(number, value);
      return true;
    }
    case WireFormatLite::WIRETYPE_FIXED64: {
      uint64 value;
      if (!input->ReadLittleEndian64(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddFixed64(number, value);
      return true;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      uint32 length;
      if (!input->ReadVarint32(&length)) return false;
      if (unknown_fields == NULL) {
        if (!input->Skip(length)) return false;
      } else {
        if (!input->ReadString(unknown_fields->AddLengthDelimited(number),
                               length)) {
          return false;
        }
      }
      return true;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      if (!input->IncrementRecursionDepth()) return false;
      if (!SkipMessage(input, (unknown_fields == NULL) ?
                              NULL : unknown_fields->AddGroup(number))) {
        return false;
      }
      input->DecrementRecursionDepth();
      
      if (!input->LastTagWas(WireFormatLite::MakeTag(
          WireFormatLite::GetTagFieldNumber(tag),
          WireFormatLite::WIRETYPE_END_GROUP))) {
        return false;
      }
      return true;
    }
    case WireFormatLite::WIRETYPE_END_GROUP: {
      return false;
    }
    case WireFormatLite::WIRETYPE_FIXED32: {
      uint32 value;
      if (!input->ReadLittleEndian32(&value)) return false;
      if (unknown_fields != NULL) unknown_fields->AddFixed32(number, value);
      return true;
    }
    default: {
      return false;
    }
  }
}

bool WireFormat::SkipMessage(io::CodedInputStream* input,
                             UnknownFieldSet* unknown_fields) {
  while(true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) {
      
      return true;
    }

    WireFormatLite::WireType wire_type = WireFormatLite::GetTagWireType(tag);

    if (wire_type == WireFormatLite::WIRETYPE_END_GROUP) {
      
      return true;
    }

    if (!SkipField(input, tag, unknown_fields)) return false;
  }
}

void WireFormat::SerializeUnknownFields(const UnknownFieldSet& unknown_fields,
                                        io::CodedOutputStream* output) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_VARINT));
        output->WriteVarint64(field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_FIXED32));
        output->WriteLittleEndian32(field.fixed32());
        break;
      case UnknownField::TYPE_FIXED64:
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_FIXED64));
        output->WriteLittleEndian64(field.fixed64());
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
        output->WriteVarint32(field.length_delimited().size());
        output->WriteRawMaybeAliased(field.length_delimited().data(),
                                     field.length_delimited().size());
        break;
      case UnknownField::TYPE_GROUP:
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_START_GROUP));
        SerializeUnknownFields(field.group(), output);
        output->WriteVarint32(WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_END_GROUP));
        break;
    }
  }
}

uint8* WireFormat::SerializeUnknownFieldsToArray(
    const UnknownFieldSet& unknown_fields,
    uint8* target) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        target = WireFormatLite::WriteInt64ToArray(
            field.number(), field.varint(), target);
        break;
      case UnknownField::TYPE_FIXED32:
        target = WireFormatLite::WriteFixed32ToArray(
            field.number(), field.fixed32(), target);
        break;
      case UnknownField::TYPE_FIXED64:
        target = WireFormatLite::WriteFixed64ToArray(
            field.number(), field.fixed64(), target);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        target = WireFormatLite::WriteBytesToArray(
            field.number(), field.length_delimited(), target);
        break;
      case UnknownField::TYPE_GROUP:
        target = WireFormatLite::WriteTagToArray(
            field.number(), WireFormatLite::WIRETYPE_START_GROUP, target);
        target = SerializeUnknownFieldsToArray(field.group(), target);
        target = WireFormatLite::WriteTagToArray(
            field.number(), WireFormatLite::WIRETYPE_END_GROUP, target);
        break;
    }
  }
  return target;
}

void WireFormat::SerializeUnknownMessageSetItems(
    const UnknownFieldSet& unknown_fields,
    io::CodedOutputStream* output) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);
    
    
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      
      output->WriteVarint32(WireFormatLite::kMessageSetItemStartTag);

      
      output->WriteVarint32(WireFormatLite::kMessageSetTypeIdTag);
      output->WriteVarint32(field.number());

      
      output->WriteVarint32(WireFormatLite::kMessageSetMessageTag);
      field.SerializeLengthDelimitedNoTag(output);

      
      output->WriteVarint32(WireFormatLite::kMessageSetItemEndTag);
    }
  }
}

uint8* WireFormat::SerializeUnknownMessageSetItemsToArray(
    const UnknownFieldSet& unknown_fields,
    uint8* target) {
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    
    
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetItemStartTag, target);

      
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetTypeIdTag, target);
      target = io::CodedOutputStream::WriteVarint32ToArray(
          field.number(), target);

      
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetMessageTag, target);
      target = field.SerializeLengthDelimitedNoTagToArray(target);

      
      target = io::CodedOutputStream::WriteTagToArray(
          WireFormatLite::kMessageSetItemEndTag, target);
    }
  }

  return target;
}

int WireFormat::ComputeUnknownFieldsSize(
    const UnknownFieldSet& unknown_fields) {
  int size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_VARINT));
        size += io::CodedOutputStream::VarintSize64(field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_FIXED32));
        size += sizeof(int32);
        break;
      case UnknownField::TYPE_FIXED64:
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_FIXED64));
        size += sizeof(int64);
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
        size += io::CodedOutputStream::VarintSize32(
            field.length_delimited().size());
        size += field.length_delimited().size();
        break;
      case UnknownField::TYPE_GROUP:
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_START_GROUP));
        size += ComputeUnknownFieldsSize(field.group());
        size += io::CodedOutputStream::VarintSize32(
            WireFormatLite::MakeTag(field.number(),
            WireFormatLite::WIRETYPE_END_GROUP));
        break;
    }
  }

  return size;
}

int WireFormat::ComputeUnknownMessageSetItemsSize(
    const UnknownFieldSet& unknown_fields) {
  int size = 0;
  for (int i = 0; i < unknown_fields.field_count(); i++) {
    const UnknownField& field = unknown_fields.field(i);

    
    
    if (field.type() == UnknownField::TYPE_LENGTH_DELIMITED) {
      size += WireFormatLite::kMessageSetItemTagsSize;
      size += io::CodedOutputStream::VarintSize32(field.number());

      int field_size = field.GetLengthDelimitedSize();
      size += io::CodedOutputStream::VarintSize32(field_size);
      size += field_size;
    }
  }

  return size;
}



bool WireFormat::ParseAndMergePartial(io::CodedInputStream* input,
                                      Message* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* message_reflection = message->GetReflection();

  while(true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) {
      
      return true;
    }

    if (WireFormatLite::GetTagWireType(tag) ==
        WireFormatLite::WIRETYPE_END_GROUP) {
      
      return true;
    }

    const FieldDescriptor* field = NULL;

    if (descriptor != NULL) {
      int field_number = WireFormatLite::GetTagFieldNumber(tag);
      field = descriptor->FindFieldByNumber(field_number);

      
      if (field == NULL && descriptor->IsExtensionNumber(field_number)) {
        if (input->GetExtensionPool() == NULL) {
          field = message_reflection->FindKnownExtensionByNumber(field_number);
        } else {
          field = input->GetExtensionPool()
                       ->FindExtensionByNumber(descriptor, field_number);
        }
      }

      
      
      if (field == NULL &&
          descriptor->options().message_set_wire_format() &&
          tag == WireFormatLite::kMessageSetItemStartTag) {
        if (!ParseAndMergeMessageSetItem(input, message)) {
          return false;
        }
        continue;  
      }
    }

    if (!ParseAndMergeField(tag, field, message, input)) {
      return false;
    }
  }
}

bool WireFormat::SkipMessageSetField(io::CodedInputStream* input,
                                     uint32 field_number,
                                     UnknownFieldSet* unknown_fields) {
  uint32 length;
  if (!input->ReadVarint32(&length)) return false;
  return input->ReadString(
      unknown_fields->AddLengthDelimited(field_number), length);
}

bool WireFormat::ParseAndMergeMessageSetField(uint32 field_number,
                                              const FieldDescriptor* field,
                                              Message* message,
                                              io::CodedInputStream* input) {
  const Reflection* message_reflection = message->GetReflection();
  if (field == NULL) {
    
    return SkipMessageSetField(
        input, field_number, message_reflection->MutableUnknownFields(message));
  } else if (field->is_repeated() ||
             field->type() != FieldDescriptor::TYPE_MESSAGE) {
    
    
    GOOGLE_LOG(ERROR) << "Extensions of MessageSets must be optional messages.";
    return false;
  } else {
    Message* sub_message = message_reflection->MutableMessage(
        message, field, input->GetExtensionFactory());
    return WireFormatLite::ReadMessage(input, sub_message);
  }
}

bool WireFormat::ParseAndMergeField(
    uint32 tag,
    const FieldDescriptor* field,        
    Message* message,
    io::CodedInputStream* input) {
  const Reflection* message_reflection = message->GetReflection();

  enum { UNKNOWN, NORMAL_FORMAT, PACKED_FORMAT } value_format;

  if (field == NULL) {
    value_format = UNKNOWN;
  } else if (WireFormatLite::GetTagWireType(tag) ==
             WireTypeForFieldType(field->type())) {
    value_format = NORMAL_FORMAT;
  } else if (field->is_packable() &&
             WireFormatLite::GetTagWireType(tag) ==
             WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    value_format = PACKED_FORMAT;
  } else {
    
    
    value_format = UNKNOWN;
  }

  if (value_format == UNKNOWN) {
    return SkipField(input, tag,
                     message_reflection->MutableUnknownFields(message));
  } else if (value_format == PACKED_FORMAT) {
    uint32 length;
    if (!input->ReadVarint32(&length)) return false;
    io::CodedInputStream::Limit limit = input->PushLimit(length);

    switch (field->type()) {
#define HANDLE_PACKED_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                      \
      case FieldDescriptor::TYPE_##TYPE: {                                     \
        while (input->BytesUntilLimit() > 0) {                                 \
          CPPTYPE value;                                                       \
          if (!WireFormatLite::ReadPrimitive<                                  \
                CPPTYPE, WireFormatLite::TYPE_##TYPE>(input, &value))          \
            return false;                                                      \
          message_reflection->Add##CPPTYPE_METHOD(message, field, value);      \
        }                                                                      \
        break;                                                                 \
      }

      HANDLE_PACKED_TYPE( INT32,  int32,  Int32)
      HANDLE_PACKED_TYPE( INT64,  int64,  Int64)
      HANDLE_PACKED_TYPE(SINT32,  int32,  Int32)
      HANDLE_PACKED_TYPE(SINT64,  int64,  Int64)
      HANDLE_PACKED_TYPE(UINT32, uint32, UInt32)
      HANDLE_PACKED_TYPE(UINT64, uint64, UInt64)

      HANDLE_PACKED_TYPE( FIXED32, uint32, UInt32)
      HANDLE_PACKED_TYPE( FIXED64, uint64, UInt64)
      HANDLE_PACKED_TYPE(SFIXED32,  int32,  Int32)
      HANDLE_PACKED_TYPE(SFIXED64,  int64,  Int64)

      HANDLE_PACKED_TYPE(FLOAT , float , Float )
      HANDLE_PACKED_TYPE(DOUBLE, double, Double)

      HANDLE_PACKED_TYPE(BOOL, bool, Bool)
#undef HANDLE_PACKED_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        while (input->BytesUntilLimit() > 0) {
          int value;
          if (!WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(
                  input, &value)) return false;
          const EnumValueDescriptor* enum_value =
              field->enum_type()->FindValueByNumber(value);
          if (enum_value != NULL) {
            message_reflection->AddEnum(message, field, enum_value);
          }
        }

        break;
      }

      case FieldDescriptor::TYPE_STRING:
      case FieldDescriptor::TYPE_GROUP:
      case FieldDescriptor::TYPE_MESSAGE:
      case FieldDescriptor::TYPE_BYTES:
        
        
        return false;
        break;
    }

    input->PopLimit(limit);
  } else {
    
    switch (field->type()) {
#define HANDLE_TYPE(TYPE, CPPTYPE, CPPTYPE_METHOD)                            \
      case FieldDescriptor::TYPE_##TYPE: {                                    \
        CPPTYPE value;                                                        \
        if (!WireFormatLite::ReadPrimitive<                                   \
                CPPTYPE, WireFormatLite::TYPE_##TYPE>(input, &value))         \
          return false;                                                       \
        if (field->is_repeated()) {                                           \
          message_reflection->Add##CPPTYPE_METHOD(message, field, value);     \
        } else {                                                              \
          message_reflection->Set##CPPTYPE_METHOD(message, field, value);     \
        }                                                                     \
        break;                                                                \
      }

      HANDLE_TYPE( INT32,  int32,  Int32)
      HANDLE_TYPE( INT64,  int64,  Int64)
      HANDLE_TYPE(SINT32,  int32,  Int32)
      HANDLE_TYPE(SINT64,  int64,  Int64)
      HANDLE_TYPE(UINT32, uint32, UInt32)
      HANDLE_TYPE(UINT64, uint64, UInt64)

      HANDLE_TYPE( FIXED32, uint32, UInt32)
      HANDLE_TYPE( FIXED64, uint64, UInt64)
      HANDLE_TYPE(SFIXED32,  int32,  Int32)
      HANDLE_TYPE(SFIXED64,  int64,  Int64)

      HANDLE_TYPE(FLOAT , float , Float )
      HANDLE_TYPE(DOUBLE, double, Double)

      HANDLE_TYPE(BOOL, bool, Bool)
#undef HANDLE_TYPE

      case FieldDescriptor::TYPE_ENUM: {
        int value;
        if (!WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(
                input, &value)) return false;
        const EnumValueDescriptor* enum_value =
          field->enum_type()->FindValueByNumber(value);
        if (enum_value != NULL) {
          if (field->is_repeated()) {
            message_reflection->AddEnum(message, field, enum_value);
          } else {
            message_reflection->SetEnum(message, field, enum_value);
          }
        } else {
          
          
          int64 sign_extended_value = static_cast<int64>(value);
          message_reflection->MutableUnknownFields(message)
                            ->AddVarint(WireFormatLite::GetTagFieldNumber(tag),
                                        sign_extended_value);
        }
        break;
      }

      
      case FieldDescriptor::TYPE_STRING: {
        string value;
        if (!WireFormatLite::ReadString(input, &value)) return false;
        VerifyUTF8StringNamedField(value.data(), value.length(), PARSE,
                                   field->name().c_str());
        if (field->is_repeated()) {
          message_reflection->AddString(message, field, value);
        } else {
          message_reflection->SetString(message, field, value);
        }
        break;
      }

      case FieldDescriptor::TYPE_BYTES: {
        string value;
        if (!WireFormatLite::ReadBytes(input, &value)) return false;
        if (field->is_repeated()) {
          message_reflection->AddString(message, field, value);
        } else {
          message_reflection->SetString(message, field, value);
        }
        break;
      }

      case FieldDescriptor::TYPE_GROUP: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(
              message, field, input->GetExtensionFactory());
        } else {
          sub_message = message_reflection->MutableMessage(
              message, field, input->GetExtensionFactory());
        }

        if (!WireFormatLite::ReadGroup(WireFormatLite::GetTagFieldNumber(tag),
                                       input, sub_message))
          return false;
        break;
      }

      case FieldDescriptor::TYPE_MESSAGE: {
        Message* sub_message;
        if (field->is_repeated()) {
          sub_message = message_reflection->AddMessage(
              message, field, input->GetExtensionFactory());
        } else {
          sub_message = message_reflection->MutableMessage(
              message, field, input->GetExtensionFactory());
        }

        if (!WireFormatLite::ReadMessage(input, sub_message)) return false;
        break;
      }
    }
  }

  return true;
}

bool WireFormat::ParseAndMergeMessageSetItem(
    io::CodedInputStream* input,
    Message* message) {
  const Reflection* message_reflection = message->GetReflection();

  
  
  

  uint32 last_type_id = 0;

  
  
  const FieldDescriptor* field = NULL;

  
  
  string message_data;

  while (true) {
    uint32 tag = input->ReadTag();
    if (tag == 0) return false;

    switch (tag) {
      case WireFormatLite::kMessageSetTypeIdTag: {
        uint32 type_id;
        if (!input->ReadVarint32(&type_id)) return false;
        last_type_id = type_id;
        field = message_reflection->FindKnownExtensionByNumber(type_id);

        if (!message_data.empty()) {
          
          
          io::ArrayInputStream raw_input(message_data.data(),
                                         message_data.size());
          io::CodedInputStream sub_input(&raw_input);
          if (!ParseAndMergeMessageSetField(last_type_id, field, message,
                                            &sub_input)) {
            return false;
          }
          message_data.clear();
        }

        break;
      }

      case WireFormatLite::kMessageSetMessageTag: {
        if (last_type_id == 0) {
          
          string temp;
          uint32 length;
          if (!input->ReadVarint32(&length)) return false;
          if (!input->ReadString(&temp, length)) return false;
          io::StringOutputStream output_stream(&message_data);
          io::CodedOutputStream coded_output(&output_stream);
          coded_output.WriteVarint32(length);
          coded_output.WriteString(temp);
        } else {
          
          if (!ParseAndMergeMessageSetField(last_type_id, field, message,
                                            input)) {
            return false;
          }
        }

        break;
      }

      case WireFormatLite::kMessageSetItemEndTag: {
        return true;
      }

      default: {
        if (!SkipField(input, tag, NULL)) return false;
      }
    }
  }
}



void WireFormat::SerializeWithCachedSizes(
    const Message& message,
    int size, io::CodedOutputStream* output) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();
  int expected_endpoint = output->ByteCount() + size;

  vector<const FieldDescriptor*> fields;
  message_reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    SerializeFieldWithCachedSizes(fields[i], message, output);
  }

  if (descriptor->options().message_set_wire_format()) {
    SerializeUnknownMessageSetItems(
        message_reflection->GetUnknownFields(message), output);
  } else {
    SerializeUnknownFields(
        message_reflection->GetUnknownFields(message), output);
  }

  GOOGLE_CHECK_EQ(output->ByteCount(), expected_endpoint)
    << ": Protocol message serialized to a size different from what was "
       "originally expected.  Perhaps it was modified by another thread "
       "during serialization?";
}

void WireFormat::SerializeFieldWithCachedSizes(
    const FieldDescriptor* field,
    const Message& message,
    io::CodedOutputStream* output) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    SerializeMessageSetItemWithCachedSizes(field, message, output);
    return;
  }

  int count = 0;

  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  const bool is_packed = field->options().packed();
  if (is_packed && count > 0) {
    WireFormatLite::WriteTag(field->number(),
        WireFormatLite::WIRETYPE_LENGTH_DELIMITED, output);
    const int data_size = FieldDataOnlyByteSize(field, message);
    output->WriteVarint32(data_size);
  }

  for (int j = 0; j < count; j++) {
    switch (field->type()) {
#define HANDLE_PRIMITIVE_TYPE(TYPE, CPPTYPE, TYPE_METHOD, CPPTYPE_METHOD)      \
      case FieldDescriptor::TYPE_##TYPE: {                                     \
        const CPPTYPE value = field->is_repeated() ?                           \
                              message_reflection->GetRepeated##CPPTYPE_METHOD( \
                                message, field, j) :                           \
                              message_reflection->Get##CPPTYPE_METHOD(         \
                                message, field);                               \
        if (is_packed) {                                                       \
          WireFormatLite::Write##TYPE_METHOD##NoTag(value, output);            \
        } else {                                                               \
          WireFormatLite::Write##TYPE_METHOD(field->number(), value, output);  \
        }                                                                      \
        break;                                                                 \
      }

      HANDLE_PRIMITIVE_TYPE( INT32,  int32,  Int32,  Int32)
      HANDLE_PRIMITIVE_TYPE( INT64,  int64,  Int64,  Int64)
      HANDLE_PRIMITIVE_TYPE(SINT32,  int32, SInt32,  Int32)
      HANDLE_PRIMITIVE_TYPE(SINT64,  int64, SInt64,  Int64)
      HANDLE_PRIMITIVE_TYPE(UINT32, uint32, UInt32, UInt32)
      HANDLE_PRIMITIVE_TYPE(UINT64, uint64, UInt64, UInt64)

      HANDLE_PRIMITIVE_TYPE( FIXED32, uint32,  Fixed32, UInt32)
      HANDLE_PRIMITIVE_TYPE( FIXED64, uint64,  Fixed64, UInt64)
      HANDLE_PRIMITIVE_TYPE(SFIXED32,  int32, SFixed32,  Int32)
      HANDLE_PRIMITIVE_TYPE(SFIXED64,  int64, SFixed64,  Int64)

      HANDLE_PRIMITIVE_TYPE(FLOAT , float , Float , Float )
      HANDLE_PRIMITIVE_TYPE(DOUBLE, double, Double, Double)

      HANDLE_PRIMITIVE_TYPE(BOOL, bool, Bool, Bool)
#undef HANDLE_PRIMITIVE_TYPE

      case FieldDescriptor::TYPE_GROUP:
        WireFormatLite::WriteGroup(
              field->number(),
              field->is_repeated() ?
                message_reflection->GetRepeatedMessage(
                  message, field, j) :
                message_reflection->GetMessage(message, field),
              output);
        break;

      case FieldDescriptor::TYPE_MESSAGE:
        WireFormatLite::WriteMessage(
              field->number(),
              field->is_repeated() ?
                message_reflection->GetRepeatedMessage(
                  message, field, j) :
                message_reflection->GetMessage(message, field),
              output);
        break;

      case FieldDescriptor::TYPE_ENUM: {
        const EnumValueDescriptor* value = field->is_repeated() ?
          message_reflection->GetRepeatedEnum(message, field, j) :
          message_reflection->GetEnum(message, field);
        if (is_packed) {
          WireFormatLite::WriteEnumNoTag(value->number(), output);
        } else {
          WireFormatLite::WriteEnum(field->number(), value->number(), output);
        }
        break;
      }

      
      
      case FieldDescriptor::TYPE_STRING: {
        string scratch;
        const string& value = field->is_repeated() ?
          message_reflection->GetRepeatedStringReference(
            message, field, j, &scratch) :
          message_reflection->GetStringReference(message, field, &scratch);
        VerifyUTF8StringNamedField(value.data(), value.length(), SERIALIZE,
                                   field->name().c_str());
        WireFormatLite::WriteString(field->number(), value, output);
        break;
      }

      case FieldDescriptor::TYPE_BYTES: {
        string scratch;
        const string& value = field->is_repeated() ?
          message_reflection->GetRepeatedStringReference(
            message, field, j, &scratch) :
          message_reflection->GetStringReference(message, field, &scratch);
        WireFormatLite::WriteBytes(field->number(), value, output);
        break;
      }
    }
  }
}

void WireFormat::SerializeMessageSetItemWithCachedSizes(
    const FieldDescriptor* field,
    const Message& message,
    io::CodedOutputStream* output) {
  const Reflection* message_reflection = message.GetReflection();

  
  output->WriteVarint32(WireFormatLite::kMessageSetItemStartTag);

  
  output->WriteVarint32(WireFormatLite::kMessageSetTypeIdTag);
  output->WriteVarint32(field->number());

  
  output->WriteVarint32(WireFormatLite::kMessageSetMessageTag);

  const Message& sub_message = message_reflection->GetMessage(message, field);
  output->WriteVarint32(sub_message.GetCachedSize());
  sub_message.SerializeWithCachedSizes(output);

  
  output->WriteVarint32(WireFormatLite::kMessageSetItemEndTag);
}



int WireFormat::ByteSize(const Message& message) {
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* message_reflection = message.GetReflection();

  int our_size = 0;

  vector<const FieldDescriptor*> fields;
  message_reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); i++) {
    our_size += FieldByteSize(fields[i], message);
  }

  if (descriptor->options().message_set_wire_format()) {
    our_size += ComputeUnknownMessageSetItemsSize(
      message_reflection->GetUnknownFields(message));
  } else {
    our_size += ComputeUnknownFieldsSize(
      message_reflection->GetUnknownFields(message));
  }

  return our_size;
}

int WireFormat::FieldByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  if (field->is_extension() &&
      field->containing_type()->options().message_set_wire_format() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      !field->is_repeated()) {
    return MessageSetItemByteSize(field, message);
  }

  int count = 0;
  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  const int data_size = FieldDataOnlyByteSize(field, message);
  int our_size = data_size;
  if (field->options().packed()) {
    if (data_size > 0) {
      
      
      
      our_size += TagSize(field->number(), FieldDescriptor::TYPE_STRING);
      our_size += io::CodedOutputStream::VarintSize32(data_size);
    }
  } else {
    our_size += count * TagSize(field->number(), field->type());
  }
  return our_size;
}

int WireFormat::FieldDataOnlyByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  int count = 0;
  if (field->is_repeated()) {
    count = message_reflection->FieldSize(message, field);
  } else if (message_reflection->HasField(message, field)) {
    count = 1;
  }

  int data_size = 0;
  switch (field->type()) {
#define HANDLE_TYPE(TYPE, TYPE_METHOD, CPPTYPE_METHOD)                     \
    case FieldDescriptor::TYPE_##TYPE:                                     \
      if (field->is_repeated()) {                                          \
        for (int j = 0; j < count; j++) {                                  \
          data_size += WireFormatLite::TYPE_METHOD##Size(                  \
            message_reflection->GetRepeated##CPPTYPE_METHOD(               \
              message, field, j));                                         \
        }                                                                  \
      } else {                                                             \
        data_size += WireFormatLite::TYPE_METHOD##Size(                    \
          message_reflection->Get##CPPTYPE_METHOD(message, field));        \
      }                                                                    \
      break;

#define HANDLE_FIXED_TYPE(TYPE, TYPE_METHOD)                               \
    case FieldDescriptor::TYPE_##TYPE:                                     \
      data_size += count * WireFormatLite::k##TYPE_METHOD##Size;           \
      break;

    HANDLE_TYPE( INT32,  Int32,  Int32)
    HANDLE_TYPE( INT64,  Int64,  Int64)
    HANDLE_TYPE(SINT32, SInt32,  Int32)
    HANDLE_TYPE(SINT64, SInt64,  Int64)
    HANDLE_TYPE(UINT32, UInt32, UInt32)
    HANDLE_TYPE(UINT64, UInt64, UInt64)

    HANDLE_FIXED_TYPE( FIXED32,  Fixed32)
    HANDLE_FIXED_TYPE( FIXED64,  Fixed64)
    HANDLE_FIXED_TYPE(SFIXED32, SFixed32)
    HANDLE_FIXED_TYPE(SFIXED64, SFixed64)

    HANDLE_FIXED_TYPE(FLOAT , Float )
    HANDLE_FIXED_TYPE(DOUBLE, Double)

    HANDLE_FIXED_TYPE(BOOL, Bool)

    HANDLE_TYPE(GROUP  , Group  , Message)
    HANDLE_TYPE(MESSAGE, Message, Message)
#undef HANDLE_TYPE
#undef HANDLE_FIXED_TYPE

    case FieldDescriptor::TYPE_ENUM: {
      if (field->is_repeated()) {
        for (int j = 0; j < count; j++) {
          data_size += WireFormatLite::EnumSize(
            message_reflection->GetRepeatedEnum(message, field, j)->number());
        }
      } else {
        data_size += WireFormatLite::EnumSize(
          message_reflection->GetEnum(message, field)->number());
      }
      break;
    }

    
    
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
      for (int j = 0; j < count; j++) {
        string scratch;
        const string& value = field->is_repeated() ?
          message_reflection->GetRepeatedStringReference(
            message, field, j, &scratch) :
          message_reflection->GetStringReference(message, field, &scratch);
        data_size += WireFormatLite::StringSize(value);
      }
      break;
    }
  }
  return data_size;
}

int WireFormat::MessageSetItemByteSize(
    const FieldDescriptor* field,
    const Message& message) {
  const Reflection* message_reflection = message.GetReflection();

  int our_size = WireFormatLite::kMessageSetItemTagsSize;

  
  our_size += io::CodedOutputStream::VarintSize32(field->number());

  
  const Message& sub_message = message_reflection->GetMessage(message, field);
  int message_size = sub_message.ByteSize();

  our_size += io::CodedOutputStream::VarintSize32(message_size);
  our_size += message_size;

  return our_size;
}

void WireFormat::VerifyUTF8StringFallback(const char* data,
                                          int size,
                                          Operation op,
                                          const char* field_name) {
  if (!IsStructurallyValidUTF8(data, size)) {
    const char* operation_str = NULL;
    switch (op) {
      case PARSE:
        operation_str = "parsing";
        break;
      case SERIALIZE:
        operation_str = "serializing";
        break;
      
    }
    string quoted_field_name = "";
    if (field_name != NULL) {
      quoted_field_name = StringPrintf(" '%s'", field_name);
    }
    
    GOOGLE_LOG(ERROR) << "String field" << quoted_field_name << " contains invalid "
               << "UTF-8 data when " << operation_str << " a protocol "
               << "buffer. Use the 'bytes' type if you intend to send raw "
               << "bytes. ";
  }
}


}  
}  
}  
