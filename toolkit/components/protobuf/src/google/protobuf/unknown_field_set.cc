

































#include <google/protobuf/unknown_field_set.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {

UnknownFieldSet::UnknownFieldSet()
    : fields_(NULL) {}

UnknownFieldSet::~UnknownFieldSet() {
  Clear();
  delete fields_;
}

void UnknownFieldSet::ClearFallback() {
  GOOGLE_DCHECK(fields_ != NULL);
  for (int i = 0; i < fields_->size(); i++) {
    (*fields_)[i].Delete();
  }
  fields_->clear();
}

void UnknownFieldSet::ClearAndFreeMemory() {
  if (fields_ != NULL) {
    Clear();
    delete fields_;
    fields_ = NULL;
  }
}

void UnknownFieldSet::MergeFrom(const UnknownFieldSet& other) {
  for (int i = 0; i < other.field_count(); i++) {
    AddField(other.field(i));
  }
}

int UnknownFieldSet::SpaceUsedExcludingSelf() const {
  if (fields_ == NULL) return 0;

  int total_size = sizeof(*fields_) + sizeof(UnknownField) * fields_->size();
  for (int i = 0; i < fields_->size(); i++) {
    const UnknownField& field = (*fields_)[i];
    switch (field.type()) {
      case UnknownField::TYPE_LENGTH_DELIMITED:
        total_size += sizeof(*field.length_delimited_.string_value_) +
                      internal::StringSpaceUsedExcludingSelf(
                          *field.length_delimited_.string_value_);
        break;
      case UnknownField::TYPE_GROUP:
        total_size += field.group_->SpaceUsed();
        break;
      default:
        break;
    }
  }
  return total_size;
}

int UnknownFieldSet::SpaceUsed() const {
  return sizeof(*this) + SpaceUsedExcludingSelf();
}

void UnknownFieldSet::AddVarint(int number, uint64 value) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  UnknownField field;
  field.number_ = number;
  field.SetType(UnknownField::TYPE_VARINT);
  field.varint_ = value;
  fields_->push_back(field);
}

void UnknownFieldSet::AddFixed32(int number, uint32 value) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  UnknownField field;
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED32);
  field.fixed32_ = value;
  fields_->push_back(field);
}

void UnknownFieldSet::AddFixed64(int number, uint64 value) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  UnknownField field;
  field.number_ = number;
  field.SetType(UnknownField::TYPE_FIXED64);
  field.fixed64_ = value;
  fields_->push_back(field);
}

string* UnknownFieldSet::AddLengthDelimited(int number) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  UnknownField field;
  field.number_ = number;
  field.SetType(UnknownField::TYPE_LENGTH_DELIMITED);
  field.length_delimited_.string_value_ = new string;
  fields_->push_back(field);
  return field.length_delimited_.string_value_;
}


UnknownFieldSet* UnknownFieldSet::AddGroup(int number) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  UnknownField field;
  field.number_ = number;
  field.SetType(UnknownField::TYPE_GROUP);
  field.group_ = new UnknownFieldSet;
  fields_->push_back(field);
  return field.group_;
}

void UnknownFieldSet::AddField(const UnknownField& field) {
  if (fields_ == NULL) fields_ = new vector<UnknownField>;
  fields_->push_back(field);
  fields_->back().DeepCopy();
}

void UnknownFieldSet::DeleteSubrange(int start, int num) {
  GOOGLE_DCHECK(fields_ != NULL);
  
  for (int i = 0; i < num; ++i) {
    (*fields_)[i + start].Delete();
  }
  
  for (int i = start + num; i < fields_->size(); ++i) {
    (*fields_)[i - num] = (*fields_)[i];
  }
  
  for (int i = 0; i < num; ++i) {
    fields_->pop_back();
  }
}

void UnknownFieldSet::DeleteByNumber(int number) {
  if (fields_ == NULL) return;
  int left = 0;  
  for (int i = 0; i < fields_->size(); ++i) {
    UnknownField* field = &(*fields_)[i];
    if (field->number() == number) {
      field->Delete();
    } else {
      if (i != left) {
        (*fields_)[left] = (*fields_)[i];
      }
      ++left;
    }
  }
  fields_->resize(left);
}

bool UnknownFieldSet::MergeFromCodedStream(io::CodedInputStream* input) {
  UnknownFieldSet other;
  if (internal::WireFormat::SkipMessage(input, &other) &&
      input->ConsumedEntireMessage()) {
    MergeFrom(other);
    return true;
  } else {
    return false;
  }
}

bool UnknownFieldSet::ParseFromCodedStream(io::CodedInputStream* input) {
  Clear();
  return MergeFromCodedStream(input);
}

bool UnknownFieldSet::ParseFromZeroCopyStream(io::ZeroCopyInputStream* input) {
  io::CodedInputStream coded_input(input);
  return (ParseFromCodedStream(&coded_input) &&
          coded_input.ConsumedEntireMessage());
}

bool UnknownFieldSet::ParseFromArray(const void* data, int size) {
  io::ArrayInputStream input(data, size);
  return ParseFromZeroCopyStream(&input);
}

void UnknownField::Delete() {
  switch (type()) {
    case UnknownField::TYPE_LENGTH_DELIMITED:
      delete length_delimited_.string_value_;
      break;
    case UnknownField::TYPE_GROUP:
      delete group_;
      break;
    default:
      break;
  }
}

void UnknownField::DeepCopy() {
  switch (type()) {
    case UnknownField::TYPE_LENGTH_DELIMITED:
      length_delimited_.string_value_ = new string(
          *length_delimited_.string_value_);
      break;
    case UnknownField::TYPE_GROUP: {
      UnknownFieldSet* group = new UnknownFieldSet;
      group->MergeFrom(*group_);
      group_ = group;
      break;
    }
    default:
      break;
  }
}


void UnknownField::SerializeLengthDelimitedNoTag(
    io::CodedOutputStream* output) const {
  GOOGLE_DCHECK_EQ(TYPE_LENGTH_DELIMITED, type());
  const string& data = *length_delimited_.string_value_;
  output->WriteVarint32(data.size());
  output->WriteRawMaybeAliased(data.data(), data.size());
}

uint8* UnknownField::SerializeLengthDelimitedNoTagToArray(uint8* target) const {
  GOOGLE_DCHECK_EQ(TYPE_LENGTH_DELIMITED, type());
  const string& data = *length_delimited_.string_value_;
  target = io::CodedOutputStream::WriteVarint32ToArray(data.size(), target);
  target = io::CodedOutputStream::WriteStringToArray(data, target);
  return target;
}

}  
}  
