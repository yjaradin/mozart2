// Copyright © 2012, Université catholique de Louvain
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

#ifndef MOZART_SERIALIZER_H
#define MOZART_SERIALIZER_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

///////////////////////////
// SerializationCallback //
///////////////////////////

SerializationCallback::SerializationCallback(VM vm): vm(vm) {}

void SerializationCallback::copy(RichNode to, RichNode from) {
  todoFrom.push_back(vm, from);
  todoTo.push_back(vm, to);
}

////////////////
// Serialized //
////////////////

#include "Serialized-implem.hh"

///////////////////
// SerializerOld //
///////////////////

#include "SerializerOld-implem.hh"

SerializerOld::SerializerOld(VM vm):
  done(buildNil(vm)) {}

SerializerOld::SerializerOld(VM vm, GR gr, SerializerOld& from) {
  gr->copyUnstableNode(done, from.done);
}

////////////////////////
// SerializerCallback //
////////////////////////

SerializerCallback::SerializerCallback(VM vm, OzListBuilder* bRes, Serializer* ser): vm(vm), bRes(bRes), ser(ser) {}

void SerializerCallback::copy(pb::Ref* to, RichNode node) {
  todoNode.push_back(vm, node);
  todoRef.push_back(vm, to);
}

void SerializerCallback::fillResource(pb::Value* val, GlobalNode* gn, atom_t type, RichNode n) {
  auto res = val->mutable_resource();
  res->mutable_uuid()->set_low(gn->uuid.data0);
  res->mutable_uuid()->set_high(gn->uuid.data1);
  copy(res->mutable_proto(), gn->protocol);
  copyAndBuild(res->mutable_type(), type);
  RichNode proto(gn->protocol);
  if (proto.is<Atom>() 
      && (proto.as<Atom>().value() == vm->coreatoms.immediate)
      && ((n.type()->serializeImmediate(vm, this, n, res->mutable_immediate())))) {
  } else {
    UnstableNode t = buildTuple(vm, n.type()->getTypeAtom(vm), ser->nextRef, gn->reified);
    bRes->push_back(vm, t);
  }
}

////////////////
// Serializer //
////////////////

#include "Serializer-implem.hh"

Serializer::Serializer(VM vm, RichNode tag)
  : nextRef(0), pickle(new pb::Pickle()) {
  size_t tagSize = ozVSLengthForBuffer(vm, tag);
  std::string tagStr;
  ozVSGet(vm, tag, tagSize, tagStr);
  pickle->set_tag(tagStr);
}

Serializer::Serializer(VM vm, GR gr, Serializer& from)
  :nextRef(from.nextRef), pickle(from.pickle)
{
  for(auto p: from.done) {
    done.push_back(vm, nullptr);
    gr->copyStableRef(done.back(), p);
  }
}

void Serializer::putImmediate(VM vm, nativeint ref, nativeint to) {
  pb::Value* val = nullptr;
  for (int i = 0; i < pickle->values_size(); ++i) {
    if (pickle->values(i).ref() == ref) {
      val = pickle->mutable_values(i)->mutable_value();
      break;
    }
  }
  if(val && val->has_resource()){
    val->mutable_resource()->mutable_immediate()->mutable_highlevel()->mutable_ref()->set_id(to);
  }
}

void Serializer::setRoot(VM vm, nativeint ref) {
  pickle->mutable_root()->set_id(ref);
}

UnstableNode Serializer::getSerialized(VM vm) {
  int bufferSize = pickle->ByteSize();
  auto buffer = newLStringInit(vm, bufferSize, [this, bufferSize](unsigned char* buf){pickle->SerializeToArray(buf, bufferSize);});
  return ByteString::build(vm, buffer);
}

void Serializer::release(VM vm){
  done.clear(vm);
  delete pickle;
}

//////////////////
// Unserializer //
//////////////////

#include "Unserializer-implem.hh"

Unserializer::Unserializer(VM vm, RichNode bytes, UnstableNode& resources) {
  OzListBuilder bRes(vm);
  size_t size = ozVBSLengthForBuffer(vm, bytes);
  std::vector<unsigned char> b;
  ozVBSGet(vm, bytes, size, b);
  std::string msg(b.begin(), b.end());
  pickle = new pb::Pickle();
  pickle->ParseFromString(msg);
  tuple.init(vm, makeTuple(vm, vm->coreatoms.sharp, pickle->values_size()));
  auto t = RichNode(tuple).as<Tuple>();
  for (auto p: pickle->values()) {
    auto r = p.ref();
    auto v = p.value();
    UnstableNode n;
    if(v.has_integer()) { n = deserialize(vm, this, v.integer()); }
    else if(v.has_float_()) { n = deserialize(vm, this, v.float_()); }
    else if(v.has_boolean()) { n = deserialize(vm, this, v.boolean()); }
    else if(v.has_unit()) { n = deserialize(vm, this, v.unit()); }
    else if(v.has_atom()) { n = deserialize(vm, this, v.atom()); }
    else if(v.has_cons()) { n = deserialize(vm, this, v.cons()); }
    else if(v.has_tuple()) { n = deserialize(vm, this, v.tuple()); }
    else if(v.has_arity()) { n = deserialize(vm, this, v.arity()); }
    else if(v.has_record()) { n = deserialize(vm, this, v.record()); }
    else if(v.has_builtin()) { n = deserialize(vm, this, v.builtin()); }
    else if(v.has_patmatwildcard()) { n = deserialize(vm, this, v.patmatwildcard()); }
    else if(v.has_patmatcapture()) { n = deserialize(vm, this, v.patmatcapture()); }
    else if(v.has_patmatconjunction()) { n = deserialize(vm, this, v.patmatconjunction()); }
    else if(v.has_patmatopenrecord()) { n = deserialize(vm, this, v.patmatopenrecord()); }
    else if(v.has_uniquename()) { n = deserialize(vm, this, v.uniquename()); }
    else if(v.has_unicodestring()) { n = deserialize(vm, this, v.unicodestring()); }
    else if(v.has_bytestring()) { n = deserialize(vm, this, v.bytestring()); }
    else if(v.has_resource()) {
      pb::UUID uuid = v.resource().uuid();
      GlobalNode* gnode;
      if(!GlobalNode::get(vm, UUID(uuid.low(), uuid.high()), gnode))
      {
        gnode->protocol.init(vm, getFromRef(v.resource().proto()));
        auto imm = v.resource().immediate();
        if(imm.has_codearea()) { n = deserializeImmediate(vm, this, gnode, imm.codearea()); }
        else if(imm.has_abstraction()) { n = deserializeImmediate(vm, this, gnode, imm.abstraction()); }
        else if(imm.has_namedname()) { n = deserializeImmediate(vm, this, gnode, imm.namedname()); }
        else if(imm.has_name()) { n = deserializeImmediate(vm, this, gnode, imm.name()); }
        else if(imm.has_highlevel()) {
          n = OptVar::build(vm);
          bRes.push_back(vm, buildSharp(vm, gnode->reified, getFromRef(v.resource().type()), getFromRef(imm.highlevel().ref())));
        } else {
          n.init(vm);//TODO make it a failed value
        }
        gnode->self.init(vm, n);
      } else n.init(vm, gnode->self);
    } else {
      n.init(vm);//TODO make it a failed value
    }
    RichNode(t.getElement(r)).become(vm, n);
  }
  resources = bRes.get(vm);
}

Unserializer::Unserializer(VM vm, GR gr, Unserializer& from)
  : pickle(from.pickle) {
  gr->copyStableNode(tuple, from.tuple);
}

UnstableNode Unserializer::getRoot(VM vm) {
  return {vm, getFromRef(pickle->root())};
}

UnstableNode Unserializer::getValueAt(VM vm, nativeint ref) {
  return {vm, *RichNode(tuple).as<Tuple>().getElement(ref)};
}

StableNode& Unserializer::getFromRef(pb::Ref ref) {
  return *RichNode(tuple).as<Tuple>().getElement(ref.id());
}

UnstableNode Unserializer::getTag(VM vm) {
  return String::build(vm, newLString(vm, pickle->tag().c_str()));
}

void Unserializer::setImmediate(VM vm, RichNode bytes, GlobalNode* gnode) {
  size_t size = ozVBSLengthForBuffer(vm, bytes);
  std::vector<unsigned char> b;
  ozVBSGet(vm, bytes, size, b);
  UnstableNode n;
  {
    pb::ImmediateData imm;
    imm.ParseFromString(std::string(b.begin(), b.end()));
    if(imm.has_codearea()) { n = deserializeImmediate(vm, this, gnode, imm.codearea()); }
    else if(imm.has_abstraction()) { n = deserializeImmediate(vm, this, gnode, imm.abstraction()); }
    else if(imm.has_namedname()) { n = deserializeImmediate(vm, this, gnode, imm.namedname()); }
    else if(imm.has_name()) { n = deserializeImmediate(vm, this, gnode, imm.name()); }
    else {
      n.init(vm);//TODO make it a failed value
    }
  }
  DataflowVariable(gnode->self).bind(vm, RichNode(n));
}


void Unserializer::release(VM vm) {
  delete pickle;
}

}

#endif // MOZART_GENERATOR

#endif // __SERIALIZER_DECL_H
