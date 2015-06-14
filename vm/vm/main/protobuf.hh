// Copyright © 2015, Université catholique de Louvain
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// *  Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// *  Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef MOZART_PROTOBUF_H
#define MOZART_PROTOBUF_H

#include "mozartcore.hh"
#include <sstream>

#ifndef MOZART_GENERATOR

struct mozart_pbuf_null_deleter
{
    void operator()(void const *) const
    {
    }
};

namespace mozart {

#include "PBufPool-implem.hh"

inline
PBufPool::PBufPool(VM vm, bool generated):
  _isGeneratedPool(generated),
  _pool(generated?
    std::shared_ptr<const pbuf::DescriptorPool>(pbuf::DescriptorPool::generated_pool(), mozart_pbuf_null_deleter()):
    std::make_shared<pbuf::DescriptorPool>()),
  _factory(std::make_shared<pbuf::DynamicMessageFactory>()) {
  if (generated) _factory->SetDelegateToGeneratedFactory(true);
}

inline
PBufPool::PBufPool(VM vm, GR gr, PBufPool& from):
  _isGeneratedPool(from._isGeneratedPool),
  _pool(std::move(from._pool)),
  _factory(std::move(from._factory)) {
}
inline
UnstableNode PBufPool::newMessage(VM vm, RichNode msgType) {
  std::string  type = ozVSGetAsStdString(vm, msgType);
  const pbuf::Descriptor* d = _pool->FindMessageTypeByName(ozVSGetAsStdString(vm, msgType));
  return UnstableNode::build<PBufMessage>(vm, _factory, d);
}

inline
UnstableNode PBufPool::recordToBytes(VM vm, RichNode msgType, RichNode msg) {
  std::string  type = ozVSGetAsStdString(vm, msgType);
  const pbuf::Descriptor* d = _pool->FindMessageTypeByName(ozVSGetAsStdString(vm, msgType));
  return serializeProtobuf(vm, _factory.get(), d, msg);
}
inline
UnstableNode PBufPool::bytesToRecord(VM vm, RichNode msgType, RichNode msg) {
  std::string  type = ozVSGetAsStdString(vm, msgType);
  const pbuf::Descriptor* d = _pool->FindMessageTypeByName(ozVSGetAsStdString(vm, msgType));
  return deserializeProtobuf(vm, _factory.get(), d, msg);
}


inline
void serializePBuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value, pbuf::Message* msg);

inline
void serializePBufFieldAt(VM vm, pbuf::MessageFactory* factory, pbuf::Message* _msg,
  const pbuf::FieldDescriptor* fd, RichNode value, int at) {
  const pbuf::Reflection* r = _msg->GetReflection();
  switch(fd->cpp_type()) {
  case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
    r->SetRepeatedBool(_msg, fd, at, getArgument<bool>(vm, value));
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_ENUM: {
    atom_t v = getArgument<atom_t>(vm, value);
    r->SetRepeatedEnum(_msg, fd, at, fd->enum_type()->FindValueByName(std::string(v.contents(), v.length())));
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_INT32: {
    std::int64_t v = getArgument<std::int64_t>(vm, value);
    r->SetRepeatedInt32(_msg, fd, at, (pbuf::int32) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_INT64: {
    std::int64_t v = getArgument<std::int64_t>(vm, value);
    r->SetRepeatedInt64(_msg, fd, at, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_UINT32: {
    std::uint64_t v = getArgument<std::uint64_t>(vm, value);
    r->SetRepeatedUInt32(_msg, fd, at, (pbuf::uint32) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_UINT64: {
    std::uint64_t v = getArgument<std::uint64_t>(vm, value);
    r->SetRepeatedUInt64(_msg, fd, at, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
    double v = getArgument<double>(vm, value);
    r->SetRepeatedFloat(_msg, fd, at, (float) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
    double v = getArgument<double>(vm, value);
    r->SetRepeatedDouble(_msg, fd, at, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_STRING:
  {
    std::string v;
    if (fd->type()==pbuf::FieldDescriptor::TYPE_STRING)
      v = ozVSGetAsStdString(vm, value);
    else {
      std::vector<unsigned char> tmp;
      ozVBSGet(vm, value, tmp);
      v = std::string(reinterpret_cast<char*>(tmp.data()), tmp.size());
    }
    r->SetRepeatedString(_msg, fd, at, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_MESSAGE:
    serializePBuf(vm, factory, fd->message_type(), value, r->MutableRepeatedMessage(_msg, fd, at));
  }
}

inline
void serializePBufField(VM vm, pbuf::MessageFactory* factory, pbuf::Message* _msg,
  const pbuf::FieldDescriptor* fd, UnstableNode& value) {
  if (fd->label()==pbuf::FieldDescriptor::LABEL_REPEATED) {
    ozListForEach(vm, value, [&](RichNode v, int i){
      serializePBufFieldAt(vm, factory, _msg, fd, v, i);
    },"list of message-like records");
    return;
  }
  const pbuf::Reflection* r = _msg->GetReflection();
  switch(fd->cpp_type()) {
  case pbuf::FieldDescriptor::CPPTYPE_BOOL: {
    r->SetBool(_msg, fd, getArgument<bool>(vm, value));
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_ENUM: {
    atom_t v = getArgument<atom_t>(vm, value);
    r->SetEnum(_msg, fd, fd->enum_type()->FindValueByName(std::string(v.contents(), v.length())));
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_INT32: {
    std::int64_t v = getArgument<std::int64_t>(vm, value);
    r->SetInt32(_msg, fd, (pbuf::int32) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_INT64: {
    std::int64_t v = getArgument<std::int64_t>(vm, value);
    r->SetInt64(_msg, fd, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_UINT32: {
    std::uint64_t v = getArgument<std::uint64_t>(vm, value);
    r->SetUInt32(_msg, fd, (pbuf::uint32) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_UINT64: {
    std::uint64_t v = getArgument<std::uint64_t>(vm, value);
    r->SetUInt64(_msg, fd, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_FLOAT: {
    double v = getArgument<double>(vm, value);
    r->SetFloat(_msg, fd, (float) v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_DOUBLE: {
    double v = getArgument<double>(vm, value);
    r->SetDouble(_msg, fd, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_STRING:
  {
    std::string v;
    if (fd->type()==pbuf::FieldDescriptor::TYPE_STRING)
      v = ozVSGetAsStdString(vm, value);
    else {
      std::vector<unsigned char> tmp;
      ozVBSGet(vm, value, tmp);
      v = std::string(reinterpret_cast<char*>(tmp.data()), tmp.size());
    }
    r->SetString(_msg, fd, v);
    return;
  }
  case pbuf::FieldDescriptor::CPPTYPE_MESSAGE:
    serializePBuf(vm, factory, fd->message_type(), value, r->MutableMessage(_msg, fd, factory));
  }
}

inline
void serializePBuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value, pbuf::Message* msg) {
  using namespace patternmatching;
  UnstableNode arityNode = RecordLike(value).arityList(vm);
  RichNode arity = RichNode(arityNode);
  ozListForEach(vm, arity, [&](RichNode fieldName){
    atom_t atF;
    nativeint intF;
    const pbuf::FieldDescriptor* fd;
    if (matches(vm, fieldName, capture(atF))) {
      fd = descriptor->FindFieldByName(std::string(atF.contents(), atF.length()));
    } else if (matches(vm, fieldName, capture(intF))) {
      fd = descriptor->FindFieldByNumber(intF);
    } else {
      raiseTypeError(vm, "message-like record", value.type()->getTypeAtom(vm));
    }
    UnstableNode v = Dottable(value).dot(vm, fieldName);
    serializePBufField(vm, factory, msg, fd, v);
  }, "message-like record");
}

inline
UnstableNode serializeProtobuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value) {
  pbuf::Message* msg = factory->GetPrototype(descriptor)->New();
  MOZART_TRY(vm) {
    serializePBuf(vm, factory, descriptor, value, msg);
  } MOZART_CATCH(vm, kind, node) {
    delete msg;
    MOZART_RETHROW(vm);
  } MOZART_ENDTRY(vm);
  std::string str = msg->SerializeAsString();
  return UnstableNode::build<ByteString>(vm, newLString(vm,
    reinterpret_cast<const unsigned char*>(str.data()), str.length()));
}

inline
UnstableNode deserializePBuf(VM vm, const pbuf::Descriptor* descriptor, const pbuf::Message& msg);

inline
UnstableNode deserializePBufFieldAt(VM vm, const pbuf::Message& _msg, const pbuf::FieldDescriptor* fd, int at) {
  const pbuf::Reflection* r = _msg.GetReflection();
  switch (fd->cpp_type()) {
  case pbuf::FieldDescriptor::CPPTYPE_BOOL:
  return {vm, r->GetRepeatedBool(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_ENUM:
  return UnstableNode::build<Atom>(vm, r->GetRepeatedEnum(_msg, fd, at)->name().c_str());
  case pbuf::FieldDescriptor::CPPTYPE_INT32:
  return {vm, r->GetRepeatedInt32(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_INT64:
  return {vm, r->GetRepeatedInt64(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_UINT32:
  return {vm, r->GetRepeatedUInt32(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_UINT64:
  return {vm, r->GetRepeatedUInt64(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_FLOAT:
  return {vm, (double)r->GetRepeatedFloat(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_DOUBLE:
  return {vm, r->GetRepeatedDouble(_msg, fd, at)};
  case pbuf::FieldDescriptor::CPPTYPE_STRING:
  {
    std::string str = r->GetRepeatedString(_msg, fd, at);
    if (fd->type()==pbuf::FieldDescriptor::TYPE_STRING)
      return UnstableNode::build<String>(vm, newLString(vm, str));
    else
      return UnstableNode::build<ByteString>(vm, newLString(vm,
        reinterpret_cast<const unsigned char*>(str.data()), str.length()));
  }
  case pbuf::FieldDescriptor::CPPTYPE_MESSAGE:
  return deserializePBuf(vm, fd->message_type(), r->GetRepeatedMessage(_msg, fd, at));
  }
  return UnstableNode::build<Unit>(vm); //should never happen
}
inline
UnstableNode deserializePBufField(VM vm, const pbuf::Message& _msg, const pbuf::FieldDescriptor* fd) {
  const pbuf::Reflection* r = _msg.GetReflection();
  if (fd->label()==pbuf::FieldDescriptor::LABEL_REPEATED) {
    OzListBuilder b(vm);
    for(int i = 0; i < r->FieldSize(_msg, fd); i++){
      b.push_back(vm, deserializePBufFieldAt(vm, _msg, fd, i));
    }
    return b.get(vm);
  }
  switch (fd->cpp_type()) {
  case pbuf::FieldDescriptor::CPPTYPE_BOOL:
  return {vm, r->GetBool(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_ENUM:
  return UnstableNode::build<Atom>(vm, r->GetEnum(_msg, fd)->name().c_str());
  case pbuf::FieldDescriptor::CPPTYPE_INT32:
  return {vm, r->GetInt32(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_INT64:
  return {vm, r->GetInt64(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_UINT32:
  return {vm, r->GetUInt32(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_UINT64:
  return {vm, r->GetUInt64(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_FLOAT:
  return {vm, (double)r->GetFloat(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_DOUBLE:
  return {vm, r->GetDouble(_msg, fd)};
  case pbuf::FieldDescriptor::CPPTYPE_STRING:
  {
    std::string str = r->GetString(_msg, fd);
    if (fd->type()==pbuf::FieldDescriptor::TYPE_STRING)
      return UnstableNode::build<String>(vm, newLString(vm, str));
    else
      return UnstableNode::build<ByteString>(vm, newLString(vm,
        reinterpret_cast<const unsigned char*>(str.data()), str.length()));
  }
  case pbuf::FieldDescriptor::CPPTYPE_MESSAGE:
  return deserializePBuf(vm, fd->message_type(), r->GetMessage(_msg, fd));
  }
  return UnstableNode::build<Unit>(vm); //should never happen
}

inline
UnstableNode deserializePBuf(VM vm, const pbuf::Descriptor* descriptor, const pbuf::Message& msg) {
  const pbuf::Reflection* r = msg.GetReflection();
  std::vector<const pbuf::FieldDescriptor*> fields;
  r->ListFields(msg, &fields);
  std::vector<UnstableField> elems;
  elems.resize(fields.size());
  std::transform(fields.begin(), fields.end(), elems.begin(), [&](const pbuf::FieldDescriptor* fd)->UnstableField{
    return {build(vm, fd->name().c_str()), deserializePBufField(vm, msg, fd)};
  });
  UnstableNode label = UnstableNode::build<Atom>(vm, descriptor->name().c_str());
  return buildRecordDynamic(vm, label, elems.size(), elems.data());
}

inline
UnstableNode deserializeProtobuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value) {
  std::vector<unsigned char> in;
  ozVBSGet(vm, value, in);
  pbuf::Message* msg = factory->GetPrototype(descriptor)->New();
  msg->ParseFromArray(in.data(), in.size());
  UnstableNode result = deserializePBuf(vm, descriptor, *msg);
  delete msg;
  return result;
}


}

#endif // MOZART_GENERATOR

#endif // MOZART_PROTOBUF_H
