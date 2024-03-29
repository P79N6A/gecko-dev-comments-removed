


#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "LayerScopePacket.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>


namespace mozilla {
namespace layers {
namespace layerscope {

void protobuf_ShutdownFile_LayerScopePacket_2eproto() {
  delete FramePacket::default_instance_;
  delete ColorPacket::default_instance_;
  delete TexturePacket::default_instance_;
  delete LayersPacket::default_instance_;
  delete LayersPacket_Layer::default_instance_;
  delete LayersPacket_Layer_Size::default_instance_;
  delete LayersPacket_Layer_Rect::default_instance_;
  delete LayersPacket_Layer_Region::default_instance_;
  delete LayersPacket_Layer_Matrix::default_instance_;
  delete LayersPacket_Layer_Shadow::default_instance_;
  delete MetaPacket::default_instance_;
  delete DrawPacket::default_instance_;
  delete DrawPacket_Rect::default_instance_;
  delete Packet::default_instance_;
  delete CommandPacket::default_instance_;
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
void protobuf_AddDesc_LayerScopePacket_2eproto_impl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#else
void protobuf_AddDesc_LayerScopePacket_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#endif
  FramePacket::default_instance_ = new FramePacket();
  ColorPacket::default_instance_ = new ColorPacket();
  TexturePacket::default_instance_ = new TexturePacket();
  LayersPacket::default_instance_ = new LayersPacket();
  LayersPacket_Layer::default_instance_ = new LayersPacket_Layer();
  LayersPacket_Layer_Size::default_instance_ = new LayersPacket_Layer_Size();
  LayersPacket_Layer_Rect::default_instance_ = new LayersPacket_Layer_Rect();
  LayersPacket_Layer_Region::default_instance_ = new LayersPacket_Layer_Region();
  LayersPacket_Layer_Matrix::default_instance_ = new LayersPacket_Layer_Matrix();
  LayersPacket_Layer_Shadow::default_instance_ = new LayersPacket_Layer_Shadow();
  MetaPacket::default_instance_ = new MetaPacket();
  DrawPacket::default_instance_ = new DrawPacket();
  DrawPacket_Rect::default_instance_ = new DrawPacket_Rect();
  Packet::default_instance_ = new Packet();
  CommandPacket::default_instance_ = new CommandPacket();
  FramePacket::default_instance_->InitAsDefaultInstance();
  ColorPacket::default_instance_->InitAsDefaultInstance();
  TexturePacket::default_instance_->InitAsDefaultInstance();
  LayersPacket::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer_Size::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer_Rect::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer_Region::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer_Matrix::default_instance_->InitAsDefaultInstance();
  LayersPacket_Layer_Shadow::default_instance_->InitAsDefaultInstance();
  MetaPacket::default_instance_->InitAsDefaultInstance();
  DrawPacket::default_instance_->InitAsDefaultInstance();
  DrawPacket_Rect::default_instance_->InitAsDefaultInstance();
  Packet::default_instance_->InitAsDefaultInstance();
  CommandPacket::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_LayerScopePacket_2eproto);
}

#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AddDesc_LayerScopePacket_2eproto_once_);
void protobuf_AddDesc_LayerScopePacket_2eproto() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AddDesc_LayerScopePacket_2eproto_once_,
                 &protobuf_AddDesc_LayerScopePacket_2eproto_impl);
}
#else

struct StaticDescriptorInitializer_LayerScopePacket_2eproto {
  StaticDescriptorInitializer_LayerScopePacket_2eproto() {
    protobuf_AddDesc_LayerScopePacket_2eproto();
  }
} static_descriptor_initializer_LayerScopePacket_2eproto_;
#endif



#ifndef _MSC_VER
const int FramePacket::kValueFieldNumber;
#endif  

FramePacket::FramePacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void FramePacket::InitAsDefaultInstance() {
}

FramePacket::FramePacket(const FramePacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void FramePacket::SharedCtor() {
  _cached_size_ = 0;
  value_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

FramePacket::~FramePacket() {
  
  SharedDtor();
}

void FramePacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void FramePacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const FramePacket& FramePacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

FramePacket* FramePacket::default_instance_ = NULL;

FramePacket* FramePacket::New() const {
  return new FramePacket;
}

void FramePacket::Clear() {
  value_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool FramePacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &value_)));
          set_has_value();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void FramePacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_value()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->value(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int FramePacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_value()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->value());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void FramePacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const FramePacket*>(&from));
}

void FramePacket::MergeFrom(const FramePacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_value()) {
      set_value(from.value());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void FramePacket::CopyFrom(const FramePacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool FramePacket::IsInitialized() const {

  return true;
}

void FramePacket::Swap(FramePacket* other) {
  if (other != this) {
    std::swap(value_, other->value_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string FramePacket::GetTypeName() const {
  return "mozilla.layers.layerscope.FramePacket";
}




#ifndef _MSC_VER
const int ColorPacket::kLayerrefFieldNumber;
const int ColorPacket::kWidthFieldNumber;
const int ColorPacket::kHeightFieldNumber;
const int ColorPacket::kColorFieldNumber;
#endif  

ColorPacket::ColorPacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void ColorPacket::InitAsDefaultInstance() {
}

ColorPacket::ColorPacket(const ColorPacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void ColorPacket::SharedCtor() {
  _cached_size_ = 0;
  layerref_ = GOOGLE_ULONGLONG(0);
  width_ = 0u;
  height_ = 0u;
  color_ = 0u;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

ColorPacket::~ColorPacket() {
  
  SharedDtor();
}

void ColorPacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void ColorPacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ColorPacket& ColorPacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

ColorPacket* ColorPacket::default_instance_ = NULL;

ColorPacket* ColorPacket::New() const {
  return new ColorPacket;
}

void ColorPacket::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<ColorPacket*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(layerref_, color_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool ColorPacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &layerref_)));
          set_has_layerref();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_width;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_width:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &width_)));
          set_has_width();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_height;
        break;
      }

      
      case 3: {
        if (tag == 24) {
         parse_height:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &height_)));
          set_has_height();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_color;
        break;
      }

      
      case 4: {
        if (tag == 32) {
         parse_color:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &color_)));
          set_has_color();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void ColorPacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_layerref()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->layerref(), output);
  }

  
  if (has_width()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(2, this->width(), output);
  }

  
  if (has_height()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(3, this->height(), output);
  }

  
  if (has_color()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(4, this->color(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int ColorPacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_layerref()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->layerref());
    }

    
    if (has_width()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->width());
    }

    
    if (has_height()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->height());
    }

    
    if (has_color()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->color());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void ColorPacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const ColorPacket*>(&from));
}

void ColorPacket::MergeFrom(const ColorPacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_layerref()) {
      set_layerref(from.layerref());
    }
    if (from.has_width()) {
      set_width(from.width());
    }
    if (from.has_height()) {
      set_height(from.height());
    }
    if (from.has_color()) {
      set_color(from.color());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void ColorPacket::CopyFrom(const ColorPacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ColorPacket::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void ColorPacket::Swap(ColorPacket* other) {
  if (other != this) {
    std::swap(layerref_, other->layerref_);
    std::swap(width_, other->width_);
    std::swap(height_, other->height_);
    std::swap(color_, other->color_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string ColorPacket::GetTypeName() const {
  return "mozilla.layers.layerscope.ColorPacket";
}




#ifndef _MSC_VER
const int TexturePacket::kLayerrefFieldNumber;
const int TexturePacket::kWidthFieldNumber;
const int TexturePacket::kHeightFieldNumber;
const int TexturePacket::kStrideFieldNumber;
const int TexturePacket::kNameFieldNumber;
const int TexturePacket::kTargetFieldNumber;
const int TexturePacket::kDataformatFieldNumber;
const int TexturePacket::kGlcontextFieldNumber;
const int TexturePacket::kDataFieldNumber;
#endif  

TexturePacket::TexturePacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void TexturePacket::InitAsDefaultInstance() {
}

TexturePacket::TexturePacket(const TexturePacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void TexturePacket::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  layerref_ = GOOGLE_ULONGLONG(0);
  width_ = 0u;
  height_ = 0u;
  stride_ = 0u;
  name_ = 0u;
  target_ = 0u;
  dataformat_ = 0u;
  glcontext_ = GOOGLE_ULONGLONG(0);
  data_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

TexturePacket::~TexturePacket() {
  
  SharedDtor();
}

void TexturePacket::SharedDtor() {
  if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete data_;
  }
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void TexturePacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const TexturePacket& TexturePacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

TexturePacket* TexturePacket::default_instance_ = NULL;

TexturePacket* TexturePacket::New() const {
  return new TexturePacket;
}

void TexturePacket::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<TexturePacket*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  if (_has_bits_[0 / 32] & 255) {
    ZR_(layerref_, glcontext_);
  }
  if (has_data()) {
    if (data_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
      data_->clear();
    }
  }

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool TexturePacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &layerref_)));
          set_has_layerref();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_width;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_width:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &width_)));
          set_has_width();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_height;
        break;
      }

      
      case 3: {
        if (tag == 24) {
         parse_height:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &height_)));
          set_has_height();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_stride;
        break;
      }

      
      case 4: {
        if (tag == 32) {
         parse_stride:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &stride_)));
          set_has_stride();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(40)) goto parse_name;
        break;
      }

      
      case 5: {
        if (tag == 40) {
         parse_name:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &name_)));
          set_has_name();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(48)) goto parse_target;
        break;
      }

      
      case 6: {
        if (tag == 48) {
         parse_target:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &target_)));
          set_has_target();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(56)) goto parse_dataformat;
        break;
      }

      
      case 7: {
        if (tag == 56) {
         parse_dataformat:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &dataformat_)));
          set_has_dataformat();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(64)) goto parse_glcontext;
        break;
      }

      
      case 8: {
        if (tag == 64) {
         parse_glcontext:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &glcontext_)));
          set_has_glcontext();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(74)) goto parse_data;
        break;
      }

      
      case 9: {
        if (tag == 74) {
         parse_data:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_data()));
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void TexturePacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_layerref()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(1, this->layerref(), output);
  }

  
  if (has_width()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(2, this->width(), output);
  }

  
  if (has_height()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(3, this->height(), output);
  }

  
  if (has_stride()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(4, this->stride(), output);
  }

  
  if (has_name()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(5, this->name(), output);
  }

  
  if (has_target()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(6, this->target(), output);
  }

  
  if (has_dataformat()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(7, this->dataformat(), output);
  }

  
  if (has_glcontext()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(8, this->glcontext(), output);
  }

  
  if (has_data()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytesMaybeAliased(
      9, this->data(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int TexturePacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_layerref()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->layerref());
    }

    
    if (has_width()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->width());
    }

    
    if (has_height()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->height());
    }

    
    if (has_stride()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->stride());
    }

    
    if (has_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->name());
    }

    
    if (has_target()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->target());
    }

    
    if (has_dataformat()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->dataformat());
    }

    
    if (has_glcontext()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->glcontext());
    }

  }
  if (_has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    
    if (has_data()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->data());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void TexturePacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const TexturePacket*>(&from));
}

void TexturePacket::MergeFrom(const TexturePacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_layerref()) {
      set_layerref(from.layerref());
    }
    if (from.has_width()) {
      set_width(from.width());
    }
    if (from.has_height()) {
      set_height(from.height());
    }
    if (from.has_stride()) {
      set_stride(from.stride());
    }
    if (from.has_name()) {
      set_name(from.name());
    }
    if (from.has_target()) {
      set_target(from.target());
    }
    if (from.has_dataformat()) {
      set_dataformat(from.dataformat());
    }
    if (from.has_glcontext()) {
      set_glcontext(from.glcontext());
    }
  }
  if (from._has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    if (from.has_data()) {
      set_data(from.data());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void TexturePacket::CopyFrom(const TexturePacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool TexturePacket::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void TexturePacket::Swap(TexturePacket* other) {
  if (other != this) {
    std::swap(layerref_, other->layerref_);
    std::swap(width_, other->width_);
    std::swap(height_, other->height_);
    std::swap(stride_, other->stride_);
    std::swap(name_, other->name_);
    std::swap(target_, other->target_);
    std::swap(dataformat_, other->dataformat_);
    std::swap(glcontext_, other->glcontext_);
    std::swap(data_, other->data_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string TexturePacket::GetTypeName() const {
  return "mozilla.layers.layerscope.TexturePacket";
}




bool LayersPacket_Layer_LayerType_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const LayersPacket_Layer_LayerType LayersPacket_Layer::UnknownLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::LayerManager;
const LayersPacket_Layer_LayerType LayersPacket_Layer::ContainerLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::PaintedLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::CanvasLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::ImageLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::ColorLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::RefLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::ReadbackLayer;
const LayersPacket_Layer_LayerType LayersPacket_Layer::LayerType_MIN;
const LayersPacket_Layer_LayerType LayersPacket_Layer::LayerType_MAX;
const int LayersPacket_Layer::LayerType_ARRAYSIZE;
#endif  
bool LayersPacket_Layer_ScrollingDirect_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const LayersPacket_Layer_ScrollingDirect LayersPacket_Layer::VERTICAL;
const LayersPacket_Layer_ScrollingDirect LayersPacket_Layer::HORIZONTAL;
const LayersPacket_Layer_ScrollingDirect LayersPacket_Layer::ScrollingDirect_MIN;
const LayersPacket_Layer_ScrollingDirect LayersPacket_Layer::ScrollingDirect_MAX;
const int LayersPacket_Layer::ScrollingDirect_ARRAYSIZE;
#endif  
bool LayersPacket_Layer_Filter_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_FAST;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_GOOD;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_BEST;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_NEAREST;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_BILINEAR;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_GAUSSIAN;
const LayersPacket_Layer_Filter LayersPacket_Layer::FILTER_SENTINEL;
const LayersPacket_Layer_Filter LayersPacket_Layer::Filter_MIN;
const LayersPacket_Layer_Filter LayersPacket_Layer::Filter_MAX;
const int LayersPacket_Layer::Filter_ARRAYSIZE;
#endif  
#ifndef _MSC_VER
const int LayersPacket_Layer_Size::kWFieldNumber;
const int LayersPacket_Layer_Size::kHFieldNumber;
#endif  

LayersPacket_Layer_Size::LayersPacket_Layer_Size()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer_Size::InitAsDefaultInstance() {
}

LayersPacket_Layer_Size::LayersPacket_Layer_Size(const LayersPacket_Layer_Size& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer_Size::SharedCtor() {
  _cached_size_ = 0;
  w_ = 0;
  h_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer_Size::~LayersPacket_Layer_Size() {
  
  SharedDtor();
}

void LayersPacket_Layer_Size::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void LayersPacket_Layer_Size::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer_Size& LayersPacket_Layer_Size::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer_Size* LayersPacket_Layer_Size::default_instance_ = NULL;

LayersPacket_Layer_Size* LayersPacket_Layer_Size::New() const {
  return new LayersPacket_Layer_Size;
}

void LayersPacket_Layer_Size::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<LayersPacket_Layer_Size*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(w_, h_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer_Size::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &w_)));
          set_has_w();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_h;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_h:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &h_)));
          set_has_h();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer_Size::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_w()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->w(), output);
  }

  
  if (has_h()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->h(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer_Size::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_w()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->w());
    }

    
    if (has_h()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->h());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer_Size::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer_Size*>(&from));
}

void LayersPacket_Layer_Size::MergeFrom(const LayersPacket_Layer_Size& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_w()) {
      set_w(from.w());
    }
    if (from.has_h()) {
      set_h(from.h());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer_Size::CopyFrom(const LayersPacket_Layer_Size& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer_Size::IsInitialized() const {

  return true;
}

void LayersPacket_Layer_Size::Swap(LayersPacket_Layer_Size* other) {
  if (other != this) {
    std::swap(w_, other->w_);
    std::swap(h_, other->h_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer_Size::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer.Size";
}




#ifndef _MSC_VER
const int LayersPacket_Layer_Rect::kXFieldNumber;
const int LayersPacket_Layer_Rect::kYFieldNumber;
const int LayersPacket_Layer_Rect::kWFieldNumber;
const int LayersPacket_Layer_Rect::kHFieldNumber;
#endif  

LayersPacket_Layer_Rect::LayersPacket_Layer_Rect()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer_Rect::InitAsDefaultInstance() {
}

LayersPacket_Layer_Rect::LayersPacket_Layer_Rect(const LayersPacket_Layer_Rect& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer_Rect::SharedCtor() {
  _cached_size_ = 0;
  x_ = 0;
  y_ = 0;
  w_ = 0;
  h_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer_Rect::~LayersPacket_Layer_Rect() {
  
  SharedDtor();
}

void LayersPacket_Layer_Rect::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void LayersPacket_Layer_Rect::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer_Rect& LayersPacket_Layer_Rect::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer_Rect* LayersPacket_Layer_Rect::default_instance_ = NULL;

LayersPacket_Layer_Rect* LayersPacket_Layer_Rect::New() const {
  return new LayersPacket_Layer_Rect;
}

void LayersPacket_Layer_Rect::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<LayersPacket_Layer_Rect*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(x_, h_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer_Rect::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &x_)));
          set_has_x();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_y;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_y:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &y_)));
          set_has_y();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_w;
        break;
      }

      
      case 3: {
        if (tag == 24) {
         parse_w:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &w_)));
          set_has_w();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_h;
        break;
      }

      
      case 4: {
        if (tag == 32) {
         parse_h:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &h_)));
          set_has_h();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer_Rect::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_x()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->x(), output);
  }

  
  if (has_y()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->y(), output);
  }

  
  if (has_w()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->w(), output);
  }

  
  if (has_h()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(4, this->h(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer_Rect::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_x()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->x());
    }

    
    if (has_y()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->y());
    }

    
    if (has_w()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->w());
    }

    
    if (has_h()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->h());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer_Rect::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer_Rect*>(&from));
}

void LayersPacket_Layer_Rect::MergeFrom(const LayersPacket_Layer_Rect& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_x()) {
      set_x(from.x());
    }
    if (from.has_y()) {
      set_y(from.y());
    }
    if (from.has_w()) {
      set_w(from.w());
    }
    if (from.has_h()) {
      set_h(from.h());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer_Rect::CopyFrom(const LayersPacket_Layer_Rect& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer_Rect::IsInitialized() const {

  return true;
}

void LayersPacket_Layer_Rect::Swap(LayersPacket_Layer_Rect* other) {
  if (other != this) {
    std::swap(x_, other->x_);
    std::swap(y_, other->y_);
    std::swap(w_, other->w_);
    std::swap(h_, other->h_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer_Rect::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer.Rect";
}




#ifndef _MSC_VER
const int LayersPacket_Layer_Region::kRFieldNumber;
#endif  

LayersPacket_Layer_Region::LayersPacket_Layer_Region()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer_Region::InitAsDefaultInstance() {
}

LayersPacket_Layer_Region::LayersPacket_Layer_Region(const LayersPacket_Layer_Region& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer_Region::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer_Region::~LayersPacket_Layer_Region() {
  
  SharedDtor();
}

void LayersPacket_Layer_Region::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void LayersPacket_Layer_Region::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer_Region& LayersPacket_Layer_Region::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer_Region* LayersPacket_Layer_Region::default_instance_ = NULL;

LayersPacket_Layer_Region* LayersPacket_Layer_Region::New() const {
  return new LayersPacket_Layer_Region;
}

void LayersPacket_Layer_Region::Clear() {
  r_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer_Region::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 10) {
         parse_r:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_r()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(10)) goto parse_r;
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer_Region::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  for (int i = 0; i < this->r_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      1, this->r(i), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer_Region::ByteSize() const {
  int total_size = 0;

  
  total_size += 1 * this->r_size();
  for (int i = 0; i < this->r_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->r(i));
  }

  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer_Region::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer_Region*>(&from));
}

void LayersPacket_Layer_Region::MergeFrom(const LayersPacket_Layer_Region& from) {
  GOOGLE_CHECK_NE(&from, this);
  r_.MergeFrom(from.r_);
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer_Region::CopyFrom(const LayersPacket_Layer_Region& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer_Region::IsInitialized() const {

  return true;
}

void LayersPacket_Layer_Region::Swap(LayersPacket_Layer_Region* other) {
  if (other != this) {
    r_.Swap(&other->r_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer_Region::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer.Region";
}




#ifndef _MSC_VER
const int LayersPacket_Layer_Matrix::kIs2DFieldNumber;
const int LayersPacket_Layer_Matrix::kIsIdFieldNumber;
const int LayersPacket_Layer_Matrix::kMFieldNumber;
#endif  

LayersPacket_Layer_Matrix::LayersPacket_Layer_Matrix()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer_Matrix::InitAsDefaultInstance() {
}

LayersPacket_Layer_Matrix::LayersPacket_Layer_Matrix(const LayersPacket_Layer_Matrix& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer_Matrix::SharedCtor() {
  _cached_size_ = 0;
  is2d_ = false;
  isid_ = false;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer_Matrix::~LayersPacket_Layer_Matrix() {
  
  SharedDtor();
}

void LayersPacket_Layer_Matrix::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void LayersPacket_Layer_Matrix::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer_Matrix& LayersPacket_Layer_Matrix::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer_Matrix* LayersPacket_Layer_Matrix::default_instance_ = NULL;

LayersPacket_Layer_Matrix* LayersPacket_Layer_Matrix::New() const {
  return new LayersPacket_Layer_Matrix;
}

void LayersPacket_Layer_Matrix::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<LayersPacket_Layer_Matrix*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(is2d_, isid_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  m_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer_Matrix::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &is2d_)));
          set_has_is2d();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_isId;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_isId:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &isid_)));
          set_has_isid();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(29)) goto parse_m;
        break;
      }

      
      case 3: {
        if (tag == 29) {
         parse_m:
          DO_((::google::protobuf::internal::WireFormatLite::ReadRepeatedPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 1, 29, input, this->mutable_m())));
        } else if (tag == 26) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPackedPrimitiveNoInline<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, this->mutable_m())));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(29)) goto parse_m;
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer_Matrix::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_is2d()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(1, this->is2d(), output);
  }

  
  if (has_isid()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(2, this->isid(), output);
  }

  
  for (int i = 0; i < this->m_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(
      3, this->m(i), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer_Matrix::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_is2d()) {
      total_size += 1 + 1;
    }

    
    if (has_isid()) {
      total_size += 1 + 1;
    }

  }
  
  {
    int data_size = 0;
    data_size = 4 * this->m_size();
    total_size += 1 * this->m_size() + data_size;
  }

  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer_Matrix::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer_Matrix*>(&from));
}

void LayersPacket_Layer_Matrix::MergeFrom(const LayersPacket_Layer_Matrix& from) {
  GOOGLE_CHECK_NE(&from, this);
  m_.MergeFrom(from.m_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_is2d()) {
      set_is2d(from.is2d());
    }
    if (from.has_isid()) {
      set_isid(from.isid());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer_Matrix::CopyFrom(const LayersPacket_Layer_Matrix& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer_Matrix::IsInitialized() const {

  return true;
}

void LayersPacket_Layer_Matrix::Swap(LayersPacket_Layer_Matrix* other) {
  if (other != this) {
    std::swap(is2d_, other->is2d_);
    std::swap(isid_, other->isid_);
    m_.Swap(&other->m_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer_Matrix::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer.Matrix";
}




#ifndef _MSC_VER
const int LayersPacket_Layer_Shadow::kClipFieldNumber;
const int LayersPacket_Layer_Shadow::kTransformFieldNumber;
const int LayersPacket_Layer_Shadow::kVRegionFieldNumber;
#endif  

LayersPacket_Layer_Shadow::LayersPacket_Layer_Shadow()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer_Shadow::InitAsDefaultInstance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  clip_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Rect*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Rect::internal_default_instance());
#else
  clip_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Rect*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Rect::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  transform_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::internal_default_instance());
#else
  transform_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  vregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  vregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
}

LayersPacket_Layer_Shadow::LayersPacket_Layer_Shadow(const LayersPacket_Layer_Shadow& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer_Shadow::SharedCtor() {
  _cached_size_ = 0;
  clip_ = NULL;
  transform_ = NULL;
  vregion_ = NULL;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer_Shadow::~LayersPacket_Layer_Shadow() {
  
  SharedDtor();
}

void LayersPacket_Layer_Shadow::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
    delete clip_;
    delete transform_;
    delete vregion_;
  }
}

void LayersPacket_Layer_Shadow::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer_Shadow& LayersPacket_Layer_Shadow::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer_Shadow* LayersPacket_Layer_Shadow::default_instance_ = NULL;

LayersPacket_Layer_Shadow* LayersPacket_Layer_Shadow::New() const {
  return new LayersPacket_Layer_Shadow;
}

void LayersPacket_Layer_Shadow::Clear() {
  if (_has_bits_[0 / 32] & 7) {
    if (has_clip()) {
      if (clip_ != NULL) clip_->::mozilla::layers::layerscope::LayersPacket_Layer_Rect::Clear();
    }
    if (has_transform()) {
      if (transform_ != NULL) transform_->::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::Clear();
    }
    if (has_vregion()) {
      if (vregion_ != NULL) vregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer_Shadow::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_clip()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_transform;
        break;
      }

      
      case 2: {
        if (tag == 18) {
         parse_transform:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_transform()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(26)) goto parse_vRegion;
        break;
      }

      
      case 3: {
        if (tag == 26) {
         parse_vRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_vregion()));
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer_Shadow::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_clip()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      1, this->clip(), output);
  }

  
  if (has_transform()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      2, this->transform(), output);
  }

  
  if (has_vregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      3, this->vregion(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer_Shadow::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_clip()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->clip());
    }

    
    if (has_transform()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->transform());
    }

    
    if (has_vregion()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->vregion());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer_Shadow::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer_Shadow*>(&from));
}

void LayersPacket_Layer_Shadow::MergeFrom(const LayersPacket_Layer_Shadow& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_clip()) {
      mutable_clip()->::mozilla::layers::layerscope::LayersPacket_Layer_Rect::MergeFrom(from.clip());
    }
    if (from.has_transform()) {
      mutable_transform()->::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::MergeFrom(from.transform());
    }
    if (from.has_vregion()) {
      mutable_vregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.vregion());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer_Shadow::CopyFrom(const LayersPacket_Layer_Shadow& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer_Shadow::IsInitialized() const {

  return true;
}

void LayersPacket_Layer_Shadow::Swap(LayersPacket_Layer_Shadow* other) {
  if (other != this) {
    std::swap(clip_, other->clip_);
    std::swap(transform_, other->transform_);
    std::swap(vregion_, other->vregion_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer_Shadow::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer.Shadow";
}




#ifndef _MSC_VER
const int LayersPacket_Layer::kTypeFieldNumber;
const int LayersPacket_Layer::kPtrFieldNumber;
const int LayersPacket_Layer::kParentPtrFieldNumber;
const int LayersPacket_Layer::kClipFieldNumber;
const int LayersPacket_Layer::kTransformFieldNumber;
const int LayersPacket_Layer::kVRegionFieldNumber;
const int LayersPacket_Layer::kShadowFieldNumber;
const int LayersPacket_Layer::kOpacityFieldNumber;
const int LayersPacket_Layer::kCOpaqueFieldNumber;
const int LayersPacket_Layer::kCAlphaFieldNumber;
const int LayersPacket_Layer::kDirectFieldNumber;
const int LayersPacket_Layer::kBarIDFieldNumber;
const int LayersPacket_Layer::kMaskFieldNumber;
const int LayersPacket_Layer::kHitRegionFieldNumber;
const int LayersPacket_Layer::kDispatchRegionFieldNumber;
const int LayersPacket_Layer::kNoActionRegionFieldNumber;
const int LayersPacket_Layer::kHPanRegionFieldNumber;
const int LayersPacket_Layer::kVPanRegionFieldNumber;
const int LayersPacket_Layer::kValidFieldNumber;
const int LayersPacket_Layer::kColorFieldNumber;
const int LayersPacket_Layer::kFilterFieldNumber;
const int LayersPacket_Layer::kRefIDFieldNumber;
const int LayersPacket_Layer::kSizeFieldNumber;
#endif  

LayersPacket_Layer::LayersPacket_Layer()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket_Layer::InitAsDefaultInstance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  clip_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Rect*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Rect::internal_default_instance());
#else
  clip_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Rect*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Rect::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  transform_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::internal_default_instance());
#else
  transform_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Matrix*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  vregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  vregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  shadow_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Shadow*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Shadow::internal_default_instance());
#else
  shadow_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Shadow*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Shadow::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  hitregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  hitregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  dispatchregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  dispatchregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  noactionregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  noactionregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  hpanregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  hpanregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  vpanregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  vpanregion_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  valid_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Region::internal_default_instance());
#else
  valid_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Region*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Region::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  size_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Size*>(
      ::mozilla::layers::layerscope::LayersPacket_Layer_Size::internal_default_instance());
#else
  size_ = const_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Size*>(&::mozilla::layers::layerscope::LayersPacket_Layer_Size::default_instance());
#endif
}

LayersPacket_Layer::LayersPacket_Layer(const LayersPacket_Layer& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket_Layer::SharedCtor() {
  _cached_size_ = 0;
  type_ = 0;
  ptr_ = GOOGLE_ULONGLONG(0);
  parentptr_ = GOOGLE_ULONGLONG(0);
  clip_ = NULL;
  transform_ = NULL;
  vregion_ = NULL;
  shadow_ = NULL;
  opacity_ = 0;
  copaque_ = false;
  calpha_ = false;
  direct_ = 1;
  barid_ = GOOGLE_ULONGLONG(0);
  mask_ = GOOGLE_ULONGLONG(0);
  hitregion_ = NULL;
  dispatchregion_ = NULL;
  noactionregion_ = NULL;
  hpanregion_ = NULL;
  vpanregion_ = NULL;
  valid_ = NULL;
  color_ = 0u;
  filter_ = 0;
  refid_ = GOOGLE_ULONGLONG(0);
  size_ = NULL;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket_Layer::~LayersPacket_Layer() {
  
  SharedDtor();
}

void LayersPacket_Layer::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
    delete clip_;
    delete transform_;
    delete vregion_;
    delete shadow_;
    delete hitregion_;
    delete dispatchregion_;
    delete noactionregion_;
    delete hpanregion_;
    delete vpanregion_;
    delete valid_;
    delete size_;
  }
}

void LayersPacket_Layer::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket_Layer& LayersPacket_Layer::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket_Layer* LayersPacket_Layer::default_instance_ = NULL;

LayersPacket_Layer* LayersPacket_Layer::New() const {
  return new LayersPacket_Layer;
}

void LayersPacket_Layer::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<LayersPacket_Layer*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  if (_has_bits_[0 / 32] & 255) {
    ZR_(ptr_, parentptr_);
    ZR_(type_, opacity_);
    if (has_clip()) {
      if (clip_ != NULL) clip_->::mozilla::layers::layerscope::LayersPacket_Layer_Rect::Clear();
    }
    if (has_transform()) {
      if (transform_ != NULL) transform_->::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::Clear();
    }
    if (has_vregion()) {
      if (vregion_ != NULL) vregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_shadow()) {
      if (shadow_ != NULL) shadow_->::mozilla::layers::layerscope::LayersPacket_Layer_Shadow::Clear();
    }
  }
  if (_has_bits_[8 / 32] & 65280) {
    ZR_(copaque_, calpha_);
    ZR_(barid_, mask_);
    direct_ = 1;
    if (has_hitregion()) {
      if (hitregion_ != NULL) hitregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_dispatchregion()) {
      if (dispatchregion_ != NULL) dispatchregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_noactionregion()) {
      if (noactionregion_ != NULL) noactionregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
  }
  if (_has_bits_[16 / 32] & 8323072) {
    ZR_(color_, refid_);
    if (has_hpanregion()) {
      if (hpanregion_ != NULL) hpanregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_vpanregion()) {
      if (vpanregion_ != NULL) vpanregion_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_valid()) {
      if (valid_ != NULL) valid_->::mozilla::layers::layerscope::LayersPacket_Layer_Region::Clear();
    }
    if (has_size()) {
      if (size_ != NULL) size_->::mozilla::layers::layerscope::LayersPacket_Layer_Size::Clear();
    }
  }

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket_Layer::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(16383);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::mozilla::layers::layerscope::LayersPacket_Layer_LayerType_IsValid(value)) {
            set_type(static_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_LayerType >(value));
          } else {
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_ptr;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_ptr:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &ptr_)));
          set_has_ptr();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(24)) goto parse_parentPtr;
        break;
      }

      
      case 3: {
        if (tag == 24) {
         parse_parentPtr:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &parentptr_)));
          set_has_parentptr();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(82)) goto parse_clip;
        break;
      }

      
      case 10: {
        if (tag == 82) {
         parse_clip:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_clip()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(90)) goto parse_transform;
        break;
      }

      
      case 11: {
        if (tag == 90) {
         parse_transform:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_transform()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(98)) goto parse_vRegion;
        break;
      }

      
      case 12: {
        if (tag == 98) {
         parse_vRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_vregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(106)) goto parse_shadow;
        break;
      }

      
      case 13: {
        if (tag == 106) {
         parse_shadow:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_shadow()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(117)) goto parse_opacity;
        break;
      }

      
      case 14: {
        if (tag == 117) {
         parse_opacity:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &opacity_)));
          set_has_opacity();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(120)) goto parse_cOpaque;
        break;
      }

      
      case 15: {
        if (tag == 120) {
         parse_cOpaque:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &copaque_)));
          set_has_copaque();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(128)) goto parse_cAlpha;
        break;
      }

      
      case 16: {
        if (tag == 128) {
         parse_cAlpha:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &calpha_)));
          set_has_calpha();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(136)) goto parse_direct;
        break;
      }

      
      case 17: {
        if (tag == 136) {
         parse_direct:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::mozilla::layers::layerscope::LayersPacket_Layer_ScrollingDirect_IsValid(value)) {
            set_direct(static_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_ScrollingDirect >(value));
          } else {
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(144)) goto parse_barID;
        break;
      }

      
      case 18: {
        if (tag == 144) {
         parse_barID:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &barid_)));
          set_has_barid();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(152)) goto parse_mask;
        break;
      }

      
      case 19: {
        if (tag == 152) {
         parse_mask:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &mask_)));
          set_has_mask();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(162)) goto parse_hitRegion;
        break;
      }

      
      case 20: {
        if (tag == 162) {
         parse_hitRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_hitregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(170)) goto parse_dispatchRegion;
        break;
      }

      
      case 21: {
        if (tag == 170) {
         parse_dispatchRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_dispatchregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(178)) goto parse_noActionRegion;
        break;
      }

      
      case 22: {
        if (tag == 178) {
         parse_noActionRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_noactionregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(186)) goto parse_hPanRegion;
        break;
      }

      
      case 23: {
        if (tag == 186) {
         parse_hPanRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_hpanregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(194)) goto parse_vPanRegion;
        break;
      }

      
      case 24: {
        if (tag == 194) {
         parse_vPanRegion:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_vpanregion()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(802)) goto parse_valid;
        break;
      }

      
      case 100: {
        if (tag == 802) {
         parse_valid:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_valid()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(808)) goto parse_color;
        break;
      }

      
      case 101: {
        if (tag == 808) {
         parse_color:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &color_)));
          set_has_color();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(816)) goto parse_filter;
        break;
      }

      
      case 102: {
        if (tag == 816) {
         parse_filter:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::mozilla::layers::layerscope::LayersPacket_Layer_Filter_IsValid(value)) {
            set_filter(static_cast< ::mozilla::layers::layerscope::LayersPacket_Layer_Filter >(value));
          } else {
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(824)) goto parse_refID;
        break;
      }

      
      case 103: {
        if (tag == 824) {
         parse_refID:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &refid_)));
          set_has_refid();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(834)) goto parse_size;
        break;
      }

      
      case 104: {
        if (tag == 834) {
         parse_size:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_size()));
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket_Layer::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }

  
  if (has_ptr()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(2, this->ptr(), output);
  }

  
  if (has_parentptr()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(3, this->parentptr(), output);
  }

  
  if (has_clip()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      10, this->clip(), output);
  }

  
  if (has_transform()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      11, this->transform(), output);
  }

  
  if (has_vregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      12, this->vregion(), output);
  }

  
  if (has_shadow()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      13, this->shadow(), output);
  }

  
  if (has_opacity()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(14, this->opacity(), output);
  }

  
  if (has_copaque()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(15, this->copaque(), output);
  }

  
  if (has_calpha()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(16, this->calpha(), output);
  }

  
  if (has_direct()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      17, this->direct(), output);
  }

  
  if (has_barid()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(18, this->barid(), output);
  }

  
  if (has_mask()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(19, this->mask(), output);
  }

  
  if (has_hitregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      20, this->hitregion(), output);
  }

  
  if (has_dispatchregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      21, this->dispatchregion(), output);
  }

  
  if (has_noactionregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      22, this->noactionregion(), output);
  }

  
  if (has_hpanregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      23, this->hpanregion(), output);
  }

  
  if (has_vpanregion()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      24, this->vpanregion(), output);
  }

  
  if (has_valid()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      100, this->valid(), output);
  }

  
  if (has_color()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(101, this->color(), output);
  }

  
  if (has_filter()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      102, this->filter(), output);
  }

  
  if (has_refid()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(103, this->refid(), output);
  }

  
  if (has_size()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      104, this->size(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket_Layer::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }

    
    if (has_ptr()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->ptr());
    }

    
    if (has_parentptr()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->parentptr());
    }

    
    if (has_clip()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->clip());
    }

    
    if (has_transform()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->transform());
    }

    
    if (has_vregion()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->vregion());
    }

    
    if (has_shadow()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->shadow());
    }

    
    if (has_opacity()) {
      total_size += 1 + 4;
    }

  }
  if (_has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    
    if (has_copaque()) {
      total_size += 1 + 1;
    }

    
    if (has_calpha()) {
      total_size += 2 + 1;
    }

    
    if (has_direct()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->direct());
    }

    
    if (has_barid()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->barid());
    }

    
    if (has_mask()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->mask());
    }

    
    if (has_hitregion()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->hitregion());
    }

    
    if (has_dispatchregion()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->dispatchregion());
    }

    
    if (has_noactionregion()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->noactionregion());
    }

  }
  if (_has_bits_[16 / 32] & (0xffu << (16 % 32))) {
    
    if (has_hpanregion()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->hpanregion());
    }

    
    if (has_vpanregion()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->vpanregion());
    }

    
    if (has_valid()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->valid());
    }

    
    if (has_color()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->color());
    }

    
    if (has_filter()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->filter());
    }

    
    if (has_refid()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->refid());
    }

    
    if (has_size()) {
      total_size += 2 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->size());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket_Layer::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket_Layer*>(&from));
}

void LayersPacket_Layer::MergeFrom(const LayersPacket_Layer& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_ptr()) {
      set_ptr(from.ptr());
    }
    if (from.has_parentptr()) {
      set_parentptr(from.parentptr());
    }
    if (from.has_clip()) {
      mutable_clip()->::mozilla::layers::layerscope::LayersPacket_Layer_Rect::MergeFrom(from.clip());
    }
    if (from.has_transform()) {
      mutable_transform()->::mozilla::layers::layerscope::LayersPacket_Layer_Matrix::MergeFrom(from.transform());
    }
    if (from.has_vregion()) {
      mutable_vregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.vregion());
    }
    if (from.has_shadow()) {
      mutable_shadow()->::mozilla::layers::layerscope::LayersPacket_Layer_Shadow::MergeFrom(from.shadow());
    }
    if (from.has_opacity()) {
      set_opacity(from.opacity());
    }
  }
  if (from._has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    if (from.has_copaque()) {
      set_copaque(from.copaque());
    }
    if (from.has_calpha()) {
      set_calpha(from.calpha());
    }
    if (from.has_direct()) {
      set_direct(from.direct());
    }
    if (from.has_barid()) {
      set_barid(from.barid());
    }
    if (from.has_mask()) {
      set_mask(from.mask());
    }
    if (from.has_hitregion()) {
      mutable_hitregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.hitregion());
    }
    if (from.has_dispatchregion()) {
      mutable_dispatchregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.dispatchregion());
    }
    if (from.has_noactionregion()) {
      mutable_noactionregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.noactionregion());
    }
  }
  if (from._has_bits_[16 / 32] & (0xffu << (16 % 32))) {
    if (from.has_hpanregion()) {
      mutable_hpanregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.hpanregion());
    }
    if (from.has_vpanregion()) {
      mutable_vpanregion()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.vpanregion());
    }
    if (from.has_valid()) {
      mutable_valid()->::mozilla::layers::layerscope::LayersPacket_Layer_Region::MergeFrom(from.valid());
    }
    if (from.has_color()) {
      set_color(from.color());
    }
    if (from.has_filter()) {
      set_filter(from.filter());
    }
    if (from.has_refid()) {
      set_refid(from.refid());
    }
    if (from.has_size()) {
      mutable_size()->::mozilla::layers::layerscope::LayersPacket_Layer_Size::MergeFrom(from.size());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket_Layer::CopyFrom(const LayersPacket_Layer& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket_Layer::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000007) != 0x00000007) return false;

  return true;
}

void LayersPacket_Layer::Swap(LayersPacket_Layer* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(ptr_, other->ptr_);
    std::swap(parentptr_, other->parentptr_);
    std::swap(clip_, other->clip_);
    std::swap(transform_, other->transform_);
    std::swap(vregion_, other->vregion_);
    std::swap(shadow_, other->shadow_);
    std::swap(opacity_, other->opacity_);
    std::swap(copaque_, other->copaque_);
    std::swap(calpha_, other->calpha_);
    std::swap(direct_, other->direct_);
    std::swap(barid_, other->barid_);
    std::swap(mask_, other->mask_);
    std::swap(hitregion_, other->hitregion_);
    std::swap(dispatchregion_, other->dispatchregion_);
    std::swap(noactionregion_, other->noactionregion_);
    std::swap(hpanregion_, other->hpanregion_);
    std::swap(vpanregion_, other->vpanregion_);
    std::swap(valid_, other->valid_);
    std::swap(color_, other->color_);
    std::swap(filter_, other->filter_);
    std::swap(refid_, other->refid_);
    std::swap(size_, other->size_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket_Layer::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket.Layer";
}




#ifndef _MSC_VER
const int LayersPacket::kLayerFieldNumber;
#endif  

LayersPacket::LayersPacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void LayersPacket::InitAsDefaultInstance() {
}

LayersPacket::LayersPacket(const LayersPacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void LayersPacket::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

LayersPacket::~LayersPacket() {
  
  SharedDtor();
}

void LayersPacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void LayersPacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const LayersPacket& LayersPacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

LayersPacket* LayersPacket::default_instance_ = NULL;

LayersPacket* LayersPacket::New() const {
  return new LayersPacket;
}

void LayersPacket::Clear() {
  layer_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool LayersPacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 10) {
         parse_layer:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_layer()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(10)) goto parse_layer;
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void LayersPacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  for (int i = 0; i < this->layer_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      1, this->layer(i), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int LayersPacket::ByteSize() const {
  int total_size = 0;

  
  total_size += 1 * this->layer_size();
  for (int i = 0; i < this->layer_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->layer(i));
  }

  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void LayersPacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const LayersPacket*>(&from));
}

void LayersPacket::MergeFrom(const LayersPacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  layer_.MergeFrom(from.layer_);
  mutable_unknown_fields()->append(from.unknown_fields());
}

void LayersPacket::CopyFrom(const LayersPacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LayersPacket::IsInitialized() const {

  if (!::google::protobuf::internal::AllAreInitialized(this->layer())) return false;
  return true;
}

void LayersPacket::Swap(LayersPacket* other) {
  if (other != this) {
    layer_.Swap(&other->layer_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string LayersPacket::GetTypeName() const {
  return "mozilla.layers.layerscope.LayersPacket";
}




#ifndef _MSC_VER
const int MetaPacket::kComposedByHwcFieldNumber;
#endif  

MetaPacket::MetaPacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void MetaPacket::InitAsDefaultInstance() {
}

MetaPacket::MetaPacket(const MetaPacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void MetaPacket::SharedCtor() {
  _cached_size_ = 0;
  composedbyhwc_ = false;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

MetaPacket::~MetaPacket() {
  
  SharedDtor();
}

void MetaPacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void MetaPacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const MetaPacket& MetaPacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

MetaPacket* MetaPacket::default_instance_ = NULL;

MetaPacket* MetaPacket::New() const {
  return new MetaPacket;
}

void MetaPacket::Clear() {
  composedbyhwc_ = false;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool MetaPacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &composedbyhwc_)));
          set_has_composedbyhwc();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void MetaPacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_composedbyhwc()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(1, this->composedbyhwc(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int MetaPacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_composedbyhwc()) {
      total_size += 1 + 1;
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void MetaPacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const MetaPacket*>(&from));
}

void MetaPacket::MergeFrom(const MetaPacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_composedbyhwc()) {
      set_composedbyhwc(from.composedbyhwc());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void MetaPacket::CopyFrom(const MetaPacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool MetaPacket::IsInitialized() const {

  return true;
}

void MetaPacket::Swap(MetaPacket* other) {
  if (other != this) {
    std::swap(composedbyhwc_, other->composedbyhwc_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string MetaPacket::GetTypeName() const {
  return "mozilla.layers.layerscope.MetaPacket";
}




#ifndef _MSC_VER
const int DrawPacket_Rect::kXFieldNumber;
const int DrawPacket_Rect::kYFieldNumber;
const int DrawPacket_Rect::kWFieldNumber;
const int DrawPacket_Rect::kHFieldNumber;
#endif  

DrawPacket_Rect::DrawPacket_Rect()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void DrawPacket_Rect::InitAsDefaultInstance() {
}

DrawPacket_Rect::DrawPacket_Rect(const DrawPacket_Rect& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void DrawPacket_Rect::SharedCtor() {
  _cached_size_ = 0;
  x_ = 0;
  y_ = 0;
  w_ = 0;
  h_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

DrawPacket_Rect::~DrawPacket_Rect() {
  
  SharedDtor();
}

void DrawPacket_Rect::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void DrawPacket_Rect::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const DrawPacket_Rect& DrawPacket_Rect::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

DrawPacket_Rect* DrawPacket_Rect::default_instance_ = NULL;

DrawPacket_Rect* DrawPacket_Rect::New() const {
  return new DrawPacket_Rect;
}

void DrawPacket_Rect::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<DrawPacket_Rect*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(x_, h_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool DrawPacket_Rect::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 13) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &x_)));
          set_has_x();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(21)) goto parse_y;
        break;
      }

      
      case 2: {
        if (tag == 21) {
         parse_y:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &y_)));
          set_has_y();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(29)) goto parse_w;
        break;
      }

      
      case 3: {
        if (tag == 29) {
         parse_w:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &w_)));
          set_has_w();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(37)) goto parse_h;
        break;
      }

      
      case 4: {
        if (tag == 37) {
         parse_h:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &h_)));
          set_has_h();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void DrawPacket_Rect::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_x()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(1, this->x(), output);
  }

  
  if (has_y()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(2, this->y(), output);
  }

  
  if (has_w()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(3, this->w(), output);
  }

  
  if (has_h()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(4, this->h(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int DrawPacket_Rect::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_x()) {
      total_size += 1 + 4;
    }

    
    if (has_y()) {
      total_size += 1 + 4;
    }

    
    if (has_w()) {
      total_size += 1 + 4;
    }

    
    if (has_h()) {
      total_size += 1 + 4;
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void DrawPacket_Rect::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const DrawPacket_Rect*>(&from));
}

void DrawPacket_Rect::MergeFrom(const DrawPacket_Rect& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_x()) {
      set_x(from.x());
    }
    if (from.has_y()) {
      set_y(from.y());
    }
    if (from.has_w()) {
      set_w(from.w());
    }
    if (from.has_h()) {
      set_h(from.h());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void DrawPacket_Rect::CopyFrom(const DrawPacket_Rect& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool DrawPacket_Rect::IsInitialized() const {
  if ((_has_bits_[0] & 0x0000000f) != 0x0000000f) return false;

  return true;
}

void DrawPacket_Rect::Swap(DrawPacket_Rect* other) {
  if (other != this) {
    std::swap(x_, other->x_);
    std::swap(y_, other->y_);
    std::swap(w_, other->w_);
    std::swap(h_, other->h_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string DrawPacket_Rect::GetTypeName() const {
  return "mozilla.layers.layerscope.DrawPacket.Rect";
}




#ifndef _MSC_VER
const int DrawPacket::kOffsetXFieldNumber;
const int DrawPacket::kOffsetYFieldNumber;
const int DrawPacket::kMvMatrixFieldNumber;
const int DrawPacket::kTotalRectsFieldNumber;
const int DrawPacket::kLayerRectFieldNumber;
const int DrawPacket::kLayerrefFieldNumber;
#endif  

DrawPacket::DrawPacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void DrawPacket::InitAsDefaultInstance() {
}

DrawPacket::DrawPacket(const DrawPacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void DrawPacket::SharedCtor() {
  _cached_size_ = 0;
  offsetx_ = 0;
  offsety_ = 0;
  totalrects_ = 0u;
  layerref_ = GOOGLE_ULONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

DrawPacket::~DrawPacket() {
  
  SharedDtor();
}

void DrawPacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void DrawPacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const DrawPacket& DrawPacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

DrawPacket* DrawPacket::default_instance_ = NULL;

DrawPacket* DrawPacket::New() const {
  return new DrawPacket;
}

void DrawPacket::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<DrawPacket*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(offsetx_, offsety_);
  ZR_(layerref_, totalrects_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  mvmatrix_.Clear();
  layerrect_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool DrawPacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 13) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &offsetx_)));
          set_has_offsetx();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(21)) goto parse_offsetY;
        break;
      }

      
      case 2: {
        if (tag == 21) {
         parse_offsetY:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &offsety_)));
          set_has_offsety();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(29)) goto parse_mvMatrix;
        break;
      }

      
      case 3: {
        if (tag == 29) {
         parse_mvMatrix:
          DO_((::google::protobuf::internal::WireFormatLite::ReadRepeatedPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 1, 29, input, this->mutable_mvmatrix())));
        } else if (tag == 26) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPackedPrimitiveNoInline<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, this->mutable_mvmatrix())));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(29)) goto parse_mvMatrix;
        if (input->ExpectTag(32)) goto parse_totalRects;
        break;
      }

      
      case 4: {
        if (tag == 32) {
         parse_totalRects:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &totalrects_)));
          set_has_totalrects();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(42)) goto parse_layerRect;
        break;
      }

      
      case 5: {
        if (tag == 42) {
         parse_layerRect:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_layerrect()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(42)) goto parse_layerRect;
        if (input->ExpectTag(48)) goto parse_layerref;
        break;
      }

      
      case 6: {
        if (tag == 48) {
         parse_layerref:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_UINT64>(
                 input, &layerref_)));
          set_has_layerref();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void DrawPacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_offsetx()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(1, this->offsetx(), output);
  }

  
  if (has_offsety()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(2, this->offsety(), output);
  }

  
  for (int i = 0; i < this->mvmatrix_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(
      3, this->mvmatrix(i), output);
  }

  
  if (has_totalrects()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(4, this->totalrects(), output);
  }

  
  for (int i = 0; i < this->layerrect_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      5, this->layerrect(i), output);
  }

  
  if (has_layerref()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt64(6, this->layerref(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int DrawPacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_offsetx()) {
      total_size += 1 + 4;
    }

    
    if (has_offsety()) {
      total_size += 1 + 4;
    }

    
    if (has_totalrects()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->totalrects());
    }

    
    if (has_layerref()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt64Size(
          this->layerref());
    }

  }
  
  {
    int data_size = 0;
    data_size = 4 * this->mvmatrix_size();
    total_size += 1 * this->mvmatrix_size() + data_size;
  }

  
  total_size += 1 * this->layerrect_size();
  for (int i = 0; i < this->layerrect_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->layerrect(i));
  }

  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void DrawPacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const DrawPacket*>(&from));
}

void DrawPacket::MergeFrom(const DrawPacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  mvmatrix_.MergeFrom(from.mvmatrix_);
  layerrect_.MergeFrom(from.layerrect_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_offsetx()) {
      set_offsetx(from.offsetx());
    }
    if (from.has_offsety()) {
      set_offsety(from.offsety());
    }
    if (from.has_totalrects()) {
      set_totalrects(from.totalrects());
    }
    if (from.has_layerref()) {
      set_layerref(from.layerref());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void DrawPacket::CopyFrom(const DrawPacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool DrawPacket::IsInitialized() const {
  if ((_has_bits_[0] & 0x0000002b) != 0x0000002b) return false;

  if (!::google::protobuf::internal::AllAreInitialized(this->layerrect())) return false;
  return true;
}

void DrawPacket::Swap(DrawPacket* other) {
  if (other != this) {
    std::swap(offsetx_, other->offsetx_);
    std::swap(offsety_, other->offsety_);
    mvmatrix_.Swap(&other->mvmatrix_);
    std::swap(totalrects_, other->totalrects_);
    layerrect_.Swap(&other->layerrect_);
    std::swap(layerref_, other->layerref_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string DrawPacket::GetTypeName() const {
  return "mozilla.layers.layerscope.DrawPacket";
}




bool Packet_DataType_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const Packet_DataType Packet::FRAMESTART;
const Packet_DataType Packet::FRAMEEND;
const Packet_DataType Packet::COLOR;
const Packet_DataType Packet::TEXTURE;
const Packet_DataType Packet::LAYERS;
const Packet_DataType Packet::META;
const Packet_DataType Packet::DRAW;
const Packet_DataType Packet::DataType_MIN;
const Packet_DataType Packet::DataType_MAX;
const int Packet::DataType_ARRAYSIZE;
#endif  
#ifndef _MSC_VER
const int Packet::kTypeFieldNumber;
const int Packet::kFrameFieldNumber;
const int Packet::kColorFieldNumber;
const int Packet::kTextureFieldNumber;
const int Packet::kLayersFieldNumber;
const int Packet::kMetaFieldNumber;
const int Packet::kDrawFieldNumber;
#endif  

Packet::Packet()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void Packet::InitAsDefaultInstance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  frame_ = const_cast< ::mozilla::layers::layerscope::FramePacket*>(
      ::mozilla::layers::layerscope::FramePacket::internal_default_instance());
#else
  frame_ = const_cast< ::mozilla::layers::layerscope::FramePacket*>(&::mozilla::layers::layerscope::FramePacket::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  color_ = const_cast< ::mozilla::layers::layerscope::ColorPacket*>(
      ::mozilla::layers::layerscope::ColorPacket::internal_default_instance());
#else
  color_ = const_cast< ::mozilla::layers::layerscope::ColorPacket*>(&::mozilla::layers::layerscope::ColorPacket::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  texture_ = const_cast< ::mozilla::layers::layerscope::TexturePacket*>(
      ::mozilla::layers::layerscope::TexturePacket::internal_default_instance());
#else
  texture_ = const_cast< ::mozilla::layers::layerscope::TexturePacket*>(&::mozilla::layers::layerscope::TexturePacket::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  layers_ = const_cast< ::mozilla::layers::layerscope::LayersPacket*>(
      ::mozilla::layers::layerscope::LayersPacket::internal_default_instance());
#else
  layers_ = const_cast< ::mozilla::layers::layerscope::LayersPacket*>(&::mozilla::layers::layerscope::LayersPacket::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  meta_ = const_cast< ::mozilla::layers::layerscope::MetaPacket*>(
      ::mozilla::layers::layerscope::MetaPacket::internal_default_instance());
#else
  meta_ = const_cast< ::mozilla::layers::layerscope::MetaPacket*>(&::mozilla::layers::layerscope::MetaPacket::default_instance());
#endif
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  draw_ = const_cast< ::mozilla::layers::layerscope::DrawPacket*>(
      ::mozilla::layers::layerscope::DrawPacket::internal_default_instance());
#else
  draw_ = const_cast< ::mozilla::layers::layerscope::DrawPacket*>(&::mozilla::layers::layerscope::DrawPacket::default_instance());
#endif
}

Packet::Packet(const Packet& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void Packet::SharedCtor() {
  _cached_size_ = 0;
  type_ = 1;
  frame_ = NULL;
  color_ = NULL;
  texture_ = NULL;
  layers_ = NULL;
  meta_ = NULL;
  draw_ = NULL;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Packet::~Packet() {
  
  SharedDtor();
}

void Packet::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
    delete frame_;
    delete color_;
    delete texture_;
    delete layers_;
    delete meta_;
    delete draw_;
  }
}

void Packet::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const Packet& Packet::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

Packet* Packet::default_instance_ = NULL;

Packet* Packet::New() const {
  return new Packet;
}

void Packet::Clear() {
  if (_has_bits_[0 / 32] & 127) {
    type_ = 1;
    if (has_frame()) {
      if (frame_ != NULL) frame_->::mozilla::layers::layerscope::FramePacket::Clear();
    }
    if (has_color()) {
      if (color_ != NULL) color_->::mozilla::layers::layerscope::ColorPacket::Clear();
    }
    if (has_texture()) {
      if (texture_ != NULL) texture_->::mozilla::layers::layerscope::TexturePacket::Clear();
    }
    if (has_layers()) {
      if (layers_ != NULL) layers_->::mozilla::layers::layerscope::LayersPacket::Clear();
    }
    if (has_meta()) {
      if (meta_ != NULL) meta_->::mozilla::layers::layerscope::MetaPacket::Clear();
    }
    if (has_draw()) {
      if (draw_ != NULL) draw_->::mozilla::layers::layerscope::DrawPacket::Clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool Packet::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::mozilla::layers::layerscope::Packet_DataType_IsValid(value)) {
            set_type(static_cast< ::mozilla::layers::layerscope::Packet_DataType >(value));
          } else {
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_frame;
        break;
      }

      
      case 2: {
        if (tag == 18) {
         parse_frame:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_frame()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(26)) goto parse_color;
        break;
      }

      
      case 3: {
        if (tag == 26) {
         parse_color:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_color()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(34)) goto parse_texture;
        break;
      }

      
      case 4: {
        if (tag == 34) {
         parse_texture:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_texture()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(42)) goto parse_layers;
        break;
      }

      
      case 5: {
        if (tag == 42) {
         parse_layers:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_layers()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(50)) goto parse_meta;
        break;
      }

      
      case 6: {
        if (tag == 50) {
         parse_meta:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_meta()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(58)) goto parse_draw;
        break;
      }

      
      case 7: {
        if (tag == 58) {
         parse_draw:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_draw()));
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void Packet::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }

  
  if (has_frame()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      2, this->frame(), output);
  }

  
  if (has_color()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      3, this->color(), output);
  }

  
  if (has_texture()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      4, this->texture(), output);
  }

  
  if (has_layers()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      5, this->layers(), output);
  }

  
  if (has_meta()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      6, this->meta(), output);
  }

  
  if (has_draw()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessage(
      7, this->draw(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int Packet::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }

    
    if (has_frame()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->frame());
    }

    
    if (has_color()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->color());
    }

    
    if (has_texture()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->texture());
    }

    
    if (has_layers()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->layers());
    }

    
    if (has_meta()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->meta());
    }

    
    if (has_draw()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->draw());
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Packet::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const Packet*>(&from));
}

void Packet::MergeFrom(const Packet& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_frame()) {
      mutable_frame()->::mozilla::layers::layerscope::FramePacket::MergeFrom(from.frame());
    }
    if (from.has_color()) {
      mutable_color()->::mozilla::layers::layerscope::ColorPacket::MergeFrom(from.color());
    }
    if (from.has_texture()) {
      mutable_texture()->::mozilla::layers::layerscope::TexturePacket::MergeFrom(from.texture());
    }
    if (from.has_layers()) {
      mutable_layers()->::mozilla::layers::layerscope::LayersPacket::MergeFrom(from.layers());
    }
    if (from.has_meta()) {
      mutable_meta()->::mozilla::layers::layerscope::MetaPacket::MergeFrom(from.meta());
    }
    if (from.has_draw()) {
      mutable_draw()->::mozilla::layers::layerscope::DrawPacket::MergeFrom(from.draw());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void Packet::CopyFrom(const Packet& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Packet::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  if (has_color()) {
    if (!this->color().IsInitialized()) return false;
  }
  if (has_texture()) {
    if (!this->texture().IsInitialized()) return false;
  }
  if (has_layers()) {
    if (!this->layers().IsInitialized()) return false;
  }
  if (has_draw()) {
    if (!this->draw().IsInitialized()) return false;
  }
  return true;
}

void Packet::Swap(Packet* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(frame_, other->frame_);
    std::swap(color_, other->color_);
    std::swap(texture_, other->texture_);
    std::swap(layers_, other->layers_);
    std::swap(meta_, other->meta_);
    std::swap(draw_, other->draw_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string Packet::GetTypeName() const {
  return "mozilla.layers.layerscope.Packet";
}




bool CommandPacket_CmdType_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const CommandPacket_CmdType CommandPacket::NO_OP;
const CommandPacket_CmdType CommandPacket::LAYERS_TREE;
const CommandPacket_CmdType CommandPacket::LAYERS_BUFFER;
const CommandPacket_CmdType CommandPacket::CmdType_MIN;
const CommandPacket_CmdType CommandPacket::CmdType_MAX;
const int CommandPacket::CmdType_ARRAYSIZE;
#endif  
#ifndef _MSC_VER
const int CommandPacket::kTypeFieldNumber;
const int CommandPacket::kValueFieldNumber;
#endif  

CommandPacket::CommandPacket()
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  
}

void CommandPacket::InitAsDefaultInstance() {
}

CommandPacket::CommandPacket(const CommandPacket& from)
  : ::google::protobuf::MessageLite() {
  SharedCtor();
  MergeFrom(from);
  
}

void CommandPacket::SharedCtor() {
  _cached_size_ = 0;
  type_ = 0;
  value_ = false;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CommandPacket::~CommandPacket() {
  
  SharedDtor();
}

void CommandPacket::SharedDtor() {
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  if (this != &default_instance()) {
  #else
  if (this != default_instance_) {
  #endif
  }
}

void CommandPacket::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const CommandPacket& CommandPacket::default_instance() {
#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  protobuf_AddDesc_LayerScopePacket_2eproto();
#else
  if (default_instance_ == NULL) protobuf_AddDesc_LayerScopePacket_2eproto();
#endif
  return *default_instance_;
}

CommandPacket* CommandPacket::default_instance_ = NULL;

CommandPacket* CommandPacket::New() const {
  return new CommandPacket;
}

void CommandPacket::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<CommandPacket*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  ZR_(type_, value_);

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->clear();
}

bool CommandPacket::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  ::google::protobuf::io::StringOutputStream unknown_fields_string(
      mutable_unknown_fields());
  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(
      &unknown_fields_string);
  
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      
      case 1: {
        if (tag == 8) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::mozilla::layers::layerscope::CommandPacket_CmdType_IsValid(value)) {
            set_type(static_cast< ::mozilla::layers::layerscope::CommandPacket_CmdType >(value));
          } else {
            unknown_fields_stream.WriteVarint32(tag);
            unknown_fields_stream.WriteVarint32(value);
          }
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(16)) goto parse_value;
        break;
      }

      
      case 2: {
        if (tag == 16) {
         parse_value:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   bool, ::google::protobuf::internal::WireFormatLite::TYPE_BOOL>(
                 input, &value_)));
          set_has_value();
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
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(
            input, tag, &unknown_fields_stream));
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

void CommandPacket::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  
  
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }

  
  if (has_value()) {
    ::google::protobuf::internal::WireFormatLite::WriteBool(2, this->value(), output);
  }

  output->WriteRaw(unknown_fields().data(),
                   unknown_fields().size());
  
}

int CommandPacket::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }

    
    if (has_value()) {
      total_size += 1 + 1;
    }

  }
  total_size += unknown_fields().size();

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void CommandPacket::CheckTypeAndMergeFrom(
    const ::google::protobuf::MessageLite& from) {
  MergeFrom(*::google::protobuf::down_cast<const CommandPacket*>(&from));
}

void CommandPacket::MergeFrom(const CommandPacket& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_value()) {
      set_value(from.value());
    }
  }
  mutable_unknown_fields()->append(from.unknown_fields());
}

void CommandPacket::CopyFrom(const CommandPacket& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CommandPacket::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void CommandPacket::Swap(CommandPacket* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(value_, other->value_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.swap(other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::std::string CommandPacket::GetTypeName() const {
  return "mozilla.layers.layerscope.CommandPacket";
}




}  
}  
}  


