// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: Broadcast.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "Broadcast.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace Docapost {
namespace IA {
namespace Tesseract {
namespace Proto {

namespace {

const ::google::protobuf::Descriptor* Broadcast_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Broadcast_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_Broadcast_2eproto() {
  protobuf_AddDesc_Broadcast_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "Broadcast.proto");
  GOOGLE_CHECK(file != NULL);
  Broadcast_descriptor_ = file->message_type(0);
  static const int Broadcast_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Broadcast, version_),
  };
  Broadcast_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Broadcast_descriptor_,
      Broadcast::default_instance_,
      Broadcast_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Broadcast, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Broadcast, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Broadcast));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_Broadcast_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Broadcast_descriptor_, &Broadcast::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_Broadcast_2eproto() {
  delete Broadcast::default_instance_;
  delete Broadcast_reflection_;
}

void protobuf_AddDesc_Broadcast_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\017Broadcast.proto\022\033Docapost.IA.Tesseract"
    ".Proto\"\034\n\tBroadcast\022\017\n\007version\030\002 \002(\t", 76);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "Broadcast.proto", &protobuf_RegisterTypes);
  Broadcast::default_instance_ = new Broadcast();
  Broadcast::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_Broadcast_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_Broadcast_2eproto {
  StaticDescriptorInitializer_Broadcast_2eproto() {
    protobuf_AddDesc_Broadcast_2eproto();
  }
} static_descriptor_initializer_Broadcast_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int Broadcast::kVersionFieldNumber;
#endif  // !_MSC_VER

Broadcast::Broadcast()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:Docapost.IA.Tesseract.Proto.Broadcast)
}

void Broadcast::InitAsDefaultInstance() {
}

Broadcast::Broadcast(const Broadcast& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:Docapost.IA.Tesseract.Proto.Broadcast)
}

void Broadcast::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  version_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Broadcast::~Broadcast() {
  // @@protoc_insertion_point(destructor:Docapost.IA.Tesseract.Proto.Broadcast)
  SharedDtor();
}

void Broadcast::SharedDtor() {
  if (version_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete version_;
  }
  if (this != default_instance_) {
  }
}

void Broadcast::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Broadcast::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Broadcast_descriptor_;
}

const Broadcast& Broadcast::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_Broadcast_2eproto();
  return *default_instance_;
}

Broadcast* Broadcast::default_instance_ = NULL;

Broadcast* Broadcast::New() const {
  return new Broadcast;
}

void Broadcast::Clear() {
  if (has_version()) {
    if (version_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
      version_->clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Broadcast::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:Docapost.IA.Tesseract.Proto.Broadcast)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required string version = 2;
      case 2: {
        if (tag == 18) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_version()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->version().data(), this->version().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "version");
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
  // @@protoc_insertion_point(parse_success:Docapost.IA.Tesseract.Proto.Broadcast)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:Docapost.IA.Tesseract.Proto.Broadcast)
  return false;
#undef DO_
}

void Broadcast::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:Docapost.IA.Tesseract.Proto.Broadcast)
  // required string version = 2;
  if (has_version()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->version().data(), this->version().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "version");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->version(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:Docapost.IA.Tesseract.Proto.Broadcast)
}

::google::protobuf::uint8* Broadcast::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:Docapost.IA.Tesseract.Proto.Broadcast)
  // required string version = 2;
  if (has_version()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->version().data(), this->version().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "version");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->version(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Docapost.IA.Tesseract.Proto.Broadcast)
  return target;
}

int Broadcast::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required string version = 2;
    if (has_version()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->version());
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

void Broadcast::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Broadcast* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Broadcast*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Broadcast::MergeFrom(const Broadcast& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_version()) {
      set_version(from.version());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Broadcast::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Broadcast::CopyFrom(const Broadcast& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Broadcast::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000001) != 0x00000001) return false;

  return true;
}

void Broadcast::Swap(Broadcast* other) {
  if (other != this) {
    std::swap(version_, other->version_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Broadcast::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Broadcast_descriptor_;
  metadata.reflection = Broadcast_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace Proto
}  // namespace Tesseract
}  // namespace IA
}  // namespace Docapost

// @@protoc_insertion_point(global_scope)
