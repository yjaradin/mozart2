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

#include "mozart.hh"

#include <iostream>

namespace mozart {

UnstableNode SerializerOld::doSerialize(VM vm, RichNode todo) {
  SerializationCallback cb(vm);
  using namespace patternmatching;
  VMAllocatedList<NodeBackup> nodeBackups;
  nativeint count = 0;

  while (true) {
    RichNode head, tail, v, n;
    if (matchesCons(vm, todo, capture(head), capture(tail))) {
      if (matchesSharp(vm, head, capture(v), capture(n))) {
        cb.copy(n, v);
      } else {
        raiseTypeError(vm, "list of pairs", todo);
      }
      todo=tail;
    } else if (matches(vm, todo, vm->coreatoms.nil)) {
      break;
    } else {
      raiseTypeError(vm, "list of pairs", todo);
    }
  }
  UnstableNode next;
  RichNode d=done;
  while (true) {
    nativeint n;
    UnstableNode v;
    RichNode r;
    if (matchesSharp(vm, d, capture(n), capture(v), wildcard(), capture(next))) {
      r = v;
      nodeBackups.push_front(vm, r.makeBackup());
      r.reinit(vm, Serialized::build(vm, n));
      count = count<n ? n : count;
      d = next;
    } else if (matches(vm, d, vm->coreatoms.nil)) {
      break;
    } else {
      assert(false);
    }
  }
  while (!cb.todoFrom.empty()) {
    RichNode from=cb.todoFrom.pop_front(vm);
    RichNode to=cb.todoTo.pop_front(vm);
    if (from.is<Serialized>()) {
      UnstableNode n = mozart::build(vm, from.as<Serialized>().n());
      DataflowVariable(to).bind(vm, n);
    } else {
      done=buildTuple(vm, vm->coreatoms.sharp,
                      ++count,
                      from,
                      from.type()->serialize(vm, &cb, from),
                      std::move(done));
      nodeBackups.push_front(vm, from.makeBackup());
      from.reinit(vm, Serialized::build(vm, count));
      UnstableNode n = mozart::build(vm, count);
      DataflowVariable(to).bind(vm, n);
    }
  }
  while (!nodeBackups.empty()) {
    nodeBackups.front().restore();
    nodeBackups.remove_front(vm);
  }
  return UnstableNode(vm, done);
}

UnstableNode SerializerOld::extractByLabels(VM vm, RichNode object, RichNode labels) {
  SerializationCallback cb(vm);

  Dottable validLabels(labels);
  OzListBuilder resultList(vm);

  VMAllocatedList<NodeBackup> nodeBackups;

  auto n = OptVar::build(vm);
  cb.copy(n, object);

  while (!cb.todoFrom.empty()) {
    auto from = cb.todoFrom.pop_front(vm);
    if (!from.is<Serialized>()) {
      auto serialized = from.type()->serialize(vm, &cb, from);
      auto label = RecordLike(serialized).label(vm);
      if (validLabels.hasFeature(vm, label)) {
        resultList.push_back(vm, buildSharp(vm, from, serialized));
      }
      nodeBackups.push_front(vm, from.makeBackup());
      from.reinit(vm, Serialized::build(vm, 0));
    }
  }

  for (auto& nodeBackup : nodeBackups) {
    nodeBackup.restore();
  }
  nodeBackups.clear(vm);

  return resultList.get(vm);
}

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UnstableNode Serializer::add(VM vm, RichNode listAdd, RichNode listImm) {
  VMAllocatedList<NodeBackup> backup;
  VMAllocatedList<pb::ImmediateData*> imms;
  OzListBuilder bRes(vm);
  SerializerCallback cb(vm, &bRes, this);

  ozListForEach(vm, listAdd, [&cb](RichNode x){ cb.copy(nullptr, x); }, "list of values");
  ozListForEach(vm, listImm, [&cb, &vm, &imms](RichNode x){
      auto imm = new pb::ImmediateData();
      x.type()->serializeImmediate(vm, &cb, x, imm);
      imms.push_back(vm, imm);
    }, "list of values");
  nativeint i = 0;
  for (auto& x: done) {
    RichNode r(*x);
    backup.push_front(vm, r.makeBackup());
    r.reinit(vm, Serialized::build(vm, i++));
  }
  while (!cb.todoNode.empty()) {
    RichNode n = cb.todoNode.pop_front(vm);
    pb::Ref* r = cb.todoRef.pop_front(vm);
    if (n.is<Serialized>()) {
      r->set_id(n.as<Serialized>().n());
    } else {
      pb::RefValue* rv = pickle->add_values();
      rv->set_ref(nextRef);
      if (!n.type()->serialize(vm, &cb, n, rv->mutable_value())) {
	GlobalNode* gn = n.type()->globalize(vm, n);
	n.update();
	UnstableNode t = buildTuple(vm, n.type()->getTypeAtom(vm), nextRef, gn->reified);
	bRes.push_back(vm, t);
	pb::Resource* res = rv->mutable_value()->mutable_resource();
	res->mutable_uuid()->set_low(gn->uuid.data0);
	res->mutable_uuid()->set_high(gn->uuid.data1);
	cb.copy(res->mutable_proto(), gn->protocol);
	cb.copy(res->mutable_type(), RichNode(*RichNode(t).as<Tuple>().getLabel()));
      }
      done.push_back(vm, n.getStableRef(vm));
      RichNode r(n);
      backup.push_front(vm, r.makeBackup());
      r.reinit(vm, Serialized::build(vm, nextRef));
      nextRef++;
    }
  }
  OzListBuilder bAdd(vm);
  ozListForEach(vm, listAdd, [&bAdd, &vm](RichNode x){
      bAdd.push_back(vm, x.as<Serialized>().n());
    }, "list of serialized (Don't serialize the list of values to serialize...)");
  OzListBuilder bImm(vm);
  for(auto imm: imms) {
    if (imm) {
      int bufferSize = imm->ByteSize();
      auto buffer = newLStringInit(vm, bufferSize, [&imm, bufferSize](unsigned char* buf){
	  imm->SerializeToArray(buf, bufferSize);
	});
      delete imm;
      bImm.push_back(vm, ByteString::build(vm, buffer));
    } else {
      bImm.push_back(vm, Unit::build(vm));
    }
  }
  for (auto& x: backup) {
    x.restore();
  }
  backup.clear(vm);
  imms.clear(vm);
  return buildSharp(vm, bAdd.get(vm), bImm.get(vm), bRes.get(vm));
}

}
