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

#ifndef MOZART_SERIALIZER_DECL_H
#define MOZART_SERIALIZER_DECL_H

#include "mozartcore-decl.hh"

#include "memmanlist.hh"

#include <cassert>

namespace mozart {

///////////////////////////
// SerializationCallback //
///////////////////////////

class SerializationCallback {
private:
  inline
  explicit SerializationCallback(VM vm);

public:
  inline
  void copy(RichNode to, RichNode from);

private:
  friend class SerializerOld;
  friend class Pickler;

  VM vm;
  VMAllocatedList<RichNode> todoFrom;
  VMAllocatedList<RichNode> todoTo;
};

////////////////
// Serialized //
////////////////

#ifndef MOZART_GENERATOR
#include "Serialized-implem-decl.hh"
#endif

class Serialized: public DataType<Serialized>, StoredAs<nativeint> {
public:
  explicit Serialized(nativeint n) : _n(n) {}

  static void create(nativeint& self, VM vm, nativeint n) {
    self = n;
  }

  static void create(nativeint& self, VM vm, GR gr, Serialized from) {
    assert(false);
  }

  nativeint n() const { return _n; }

private:
  nativeint _n;
};

#ifndef MOZART_GENERATOR
#include "Serialized-implem-decl-after.hh"
#endif

///////////////////
// SerializerOld //
///////////////////

#ifndef MOZART_GENERATOR
#include "SerializerOld-implem-decl.hh"
#endif

class SerializerOld: public DataType<SerializerOld> {
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom("serializer");
  }

  inline
  explicit SerializerOld(VM vm);

  inline
  SerializerOld(VM vm, GR gr, SerializerOld& from);

public:
  UnstableNode doSerialize(VM vm, RichNode todo);

  static UnstableNode extractByLabels(VM vm, RichNode object, RichNode labels);

private:
  UnstableNode done;
};

#ifndef MOZART_GENERATOR
#include "SerializerOld-implem-decl-after.hh"
#endif

///////////////////////////
// SerializerCallback //
///////////////////////////
class Serializer;
class SerializerCallback {
private:
  inline
  explicit SerializerCallback(VM vm, OzListBuilder* bRes, Serializer* ser);

public:
  inline
  void copy(pb::Ref* to, RichNode node);

  template <class T>
  void copyAndBuild(pb::Ref* to, T&& arg) {
    copy(to, RichNode(new (vm) StableNode(vm, std::move(arg))));
  }

  inline
  void fillResource(pb::Value* val, GlobalNode* gn, atom_t type, RichNode n);

private:
  friend class Serializer;

  VM vm;
  OzListBuilder* bRes;
  Serializer* ser;
  VMAllocatedList<RichNode> todoNode;
  VMAllocatedList<pb::Ref*> todoRef;
};

////////////////
// Serializer //
////////////////

#ifndef MOZART_GENERATOR
#include "Serializer-implem-decl.hh"
#endif

class Serializer: public DataType<Serializer> {
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom("serializer");
  }

  inline
  explicit Serializer(VM vm, RichNode tag);

  inline
  Serializer(VM vm, GR gr, Serializer& from);

public:
  UnstableNode add(VM vm, RichNode listAdd, RichNode listImm);

  inline
  void putImmediate(VM vm, nativeint ref, nativeint to);

  inline
  void setRoot(VM vm, nativeint ref);

  inline
  UnstableNode getSerialized(VM vm);

  inline
  void release(VM vm);

  pb::Pickle* getPickle() {
    return pickle;
  }

private:
  friend class SerializerCallback;
  nativeint nextRef;
  pb::Pickle* pickle;
  VMAllocatedList<StableNode*> done;
};

#ifndef MOZART_GENERATOR
#include "Serializer-implem-decl-after.hh"
#endif

}

#endif // MOZART_SERIALIZER_DECL_H
