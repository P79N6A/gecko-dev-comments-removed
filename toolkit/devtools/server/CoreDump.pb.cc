


#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "CoreDump.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>


namespace mozilla {
namespace devtools {
namespace protobuf {

namespace {

const ::google::protobuf::Descriptor* Metadata_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Metadata_reflection_ = NULL;
const ::google::protobuf::Descriptor* Node_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Node_reflection_ = NULL;
const ::google::protobuf::Descriptor* Edge_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Edge_reflection_ = NULL;

}  


void protobuf_AssignDesc_CoreDump_2eproto() {
  protobuf_AddDesc_CoreDump_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "CoreDump.proto");
  GOOGLE_CHECK(file != NULL);
  Metadata_descriptor_ = file->message_type(0);
  static const int Metadata_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Metadata, timestamp_),
  };
  Metadata_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Metadata_descriptor_,
      Metadata::default_instance_,
      Metadata_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Metadata, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Metadata, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Metadata));
  Node_descriptor_ = file->message_type(1);
  static const int Node_offsets_[4] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, typename__),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, size_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, edges_),
  };
  Node_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Node_descriptor_,
      Node::default_instance_,
      Node_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Node, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Node));
  Edge_descriptor_ = file->message_type(2);
  static const int Edge_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Edge, referent_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Edge, name_),
  };
  Edge_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Edge_descriptor_,
      Edge::default_instance_,
      Edge_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Edge, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Edge, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Edge));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_CoreDump_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Metadata_descriptor_, &Metadata::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Node_descriptor_, &Node::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Edge_descriptor_, &Edge::default_instance());
}

}  

void protobuf_ShutdownFile_CoreDump_2eproto() {
  delete Metadata::default_instance_;
  delete Metadata_reflection_;
  delete Node::default_instance_;
  delete Node_reflection_;
  delete Edge::default_instance_;
  delete Edge_reflection_;
}

void protobuf_AddDesc_CoreDump_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\016CoreDump.proto\022\031mozilla.devtools.proto"
    "buf\"\035\n\010Metadata\022\021\n\ttimeStamp\030\001 \001(\004\"b\n\004No"
    "de\022\n\n\002id\030\001 \001(\004\022\020\n\010typeName\030\002 \001(\014\022\014\n\004size"
    "\030\003 \001(\004\022.\n\005edges\030\004 \003(\0132\037.mozilla.devtools"
    ".protobuf.Edge\"&\n\004Edge\022\020\n\010referent\030\001 \001(\004"
    "\022\014\n\004name\030\002 \001(\014", 214);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "CoreDump.proto", &protobuf_RegisterTypes);
  Metadata::default_instance_ = new Metadata();
  Node::default_instance_ = new Node();
  Edge::default_instance_ = new Edge();
  Metadata::default_instance_->InitAsDefaultInstance();
  Node::default_instance_->InitAsDefaultInstance();
  Edge::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_CoreDump_2eproto);
}


struct StaticDescriptorInitializer_CoreDump_2eproto {
  StaticDescriptorInitializer_CoreDump_2eproto() {
    protobuf_AddDesc_CoreDump_2eproto();
  }
} static_descriptor_initializer_CoreDump_2eproto_;



#ifndef _MSC_VER
const int Metadata::kTimeStampFieldNumber;
#endif  

Metadata::Metadata()
  : ::google::protobuf::Message() {
  SharedCtor();
  
}

void Metadata::InitAsDefaultInstance() {
}

Metadata::Metadata(const Metadata& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  
}

void Metadata::SharedCtor() {
  _cached_size_ = 0;
  timestamp_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Metadata::~Metadata() {
  
  SharedDtor();
}

void Metadata::SharedDtor() {
  if (this != default_instance_) {
  }
}

void Metadata::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Metadata::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Metadata_descriptor_;
}

const Metadata& Metadata::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_CoreDump_2eproto();
  return *default_instance_;
}

Metadata* Metadata::default_instance_ = NULL;

Metadata* Metadata::New() const {
  return new Metadata;
}

void Metadata::Clear() {
  timestamp_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Metadata::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &timestamp_)));
          set_has_timestamp();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  
  return true;
failure:
  
  return false;
#undef DO_
}

void Metadata::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_timestamp()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->timestamp(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  
}

::google::protobuf::uint8* Metadata::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  
  
  if (has_timestamp()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(1, this->timestamp(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  
  return target;
}

int Metadata::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_timestamp()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->timestamp());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Metadata::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Metadata* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Metadata*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Metadata::MergeFrom(const Metadata& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_timestamp()) {
      set_timestamp(from.timestamp());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Metadata::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Metadata::CopyFrom(const Metadata& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Metadata::IsInitialized() const {

  return true;
}

void Metadata::Swap(Metadata* other) {
  if (other != this) {
    std::swap(timestamp_, other->timestamp_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Metadata::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Metadata_descriptor_;
  metadata.reflection = Metadata_reflection_;
  return metadata;
}




#ifndef _MSC_VER
const int Node::kIdFieldNumber;
const int Node::kTypeNameFieldNumber;
const int Node::kSizeFieldNumber;
const int Node::kEdgesFieldNumber;
#endif  

Node::Node()
  : ::google::protobuf::Message() {
  SharedCtor();
  
}

void Node::InitAsDefaultInstance() {
}

Node::Node(const Node& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  
}

void Node::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  id_ = GOOGLE_ULONGLONG(0);
  typename__ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  size_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Node::~Node() {
  
  SharedDtor();
}

void Node::SharedDtor() {
  if (typename__ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete typename__;
  }
  if (this != default_instance_) {
  }
}

void Node::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Node::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Node_descriptor_;
}

const Node& Node::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_CoreDump_2eproto();
  return *default_instance_;
}

Node* Node::default_instance_ = NULL;

Node* Node::New() const {
  return new Node;
}

void Node::Clear() {
  if (_has_bits_[0 / 32] & 7) {
    id_ = GOOGLE_ULONGLONG(0);
    if (has_typename_()) {
      if (typename__ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        typename__->clear();
      }
    }
    size_ = GOOGLE_ULONGLONG(0);
  }
  edges_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Node::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &id_)));
          set_has_id();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_typeName;
        break;
      }

      
      case 2: {
        if (tag == 18) {
         parse_typeName:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_typename_()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_size;
        break;
      }

      
      case 3: {
        if (tag == 24) {
         parse_size:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &size_)));
          set_has_size();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(34)) goto parse_edges;
        break;
      }

      
      case 4: {
        if (tag == 34) {
         parse_edges:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_edges()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(34)) goto parse_edges;
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  
  return true;
failure:
  
  return false;
#undef DO_
}

void Node::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->id(), output);
  }

  
  if (has_typename_()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      2, this->typename_(), output);
  }

  
  if (has_size()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(3, this->size(), output);
  }

  
  for (int i = 0; i < this->edges_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      4, this->edges(i), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  
}

::google::protobuf::uint8* Node::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  
  
  if (has_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(1, this->id(), target);
  }

  
  if (has_typename_()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        2, this->typename_(), target);
  }

  
  if (has_size()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(3, this->size(), target);
  }

  
  for (int i = 0; i < this->edges_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        4, this->edges(i), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  
  return target;
}

int Node::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->id());
    }

    
    if (has_typename_()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->typename_());
    }

    
    if (has_size()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->size());
    }

  }
  
  total_size += 1 * this->edges_size();
  for (int i = 0; i < this->edges_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->edges(i));
  }

  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Node::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Node* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Node*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Node::MergeFrom(const Node& from) {
  GOOGLE_CHECK_NE(&from, this);
  edges_.MergeFrom(from.edges_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_id()) {
      set_id(from.id());
    }
    if (from.has_typename_()) {
      set_typename_(from.typename_());
    }
    if (from.has_size()) {
      set_size(from.size());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Node::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Node::CopyFrom(const Node& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Node::IsInitialized() const {

  return true;
}

void Node::Swap(Node* other) {
  if (other != this) {
    std::swap(id_, other->id_);
    std::swap(typename__, other->typename__);
    std::swap(size_, other->size_);
    edges_.Swap(&other->edges_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Node::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Node_descriptor_;
  metadata.reflection = Node_reflection_;
  return metadata;
}




#ifndef _MSC_VER
const int Edge::kReferentFieldNumber;
const int Edge::kNameFieldNumber;
#endif  

Edge::Edge()
  : ::google::protobuf::Message() {
  SharedCtor();
  
}

void Edge::InitAsDefaultInstance() {
}

Edge::Edge(const Edge& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  
}

void Edge::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  referent_ = GOOGLE_ULONGLONG(0);
  name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Edge::~Edge() {
  
  SharedDtor();
}

void Edge::SharedDtor() {
  if (name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete name_;
  }
  if (this != default_instance_) {
  }
}

void Edge::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Edge::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Edge_descriptor_;
}

const Edge& Edge::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_CoreDump_2eproto();
  return *default_instance_;
}

Edge* Edge::default_instance_ = NULL;

Edge* Edge::New() const {
  return new Edge;
}

void Edge::Clear() {
  if (_has_bits_[0 / 32] & 3) {
    referent_ = GOOGLE_ULONGLONG(0);
    if (has_name()) {
      if (name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        name_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Edge::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &referent_)));
          set_has_referent();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_name;
        break;
      }

      
      case 2: {
        if (tag == 18) {
         parse_name:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_name()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  
  return true;
failure:
  
  return false;
#undef DO_
}

void Edge::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_referent()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->referent(), output);
  }

  
  if (has_name()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      2, this->name(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  
}

::google::protobuf::uint8* Edge::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  
  
  if (has_referent()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(1, this->referent(), target);
  }

  
  if (has_name()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        2, this->name(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  
  return target;
}

int Edge::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_referent()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->referent());
    }

    
    if (has_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->name());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Edge::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Edge* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Edge*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Edge::MergeFrom(const Edge& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_referent()) {
      set_referent(from.referent());
    }
    if (from.has_name()) {
      set_name(from.name());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Edge::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Edge::CopyFrom(const Edge& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Edge::IsInitialized() const {

  return true;
}

void Edge::Swap(Edge* other) {
  if (other != this) {
    std::swap(referent_, other->referent_);
    std::swap(name_, other->name_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Edge::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Edge_descriptor_;
  metadata.reflection = Edge_reflection_;
  return metadata;
}




}  
}  
}  


