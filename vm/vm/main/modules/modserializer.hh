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

#ifndef MOZART_MODSERIALIZER_H
#define MOZART_MODSERIALIZER_H

#include "../mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

namespace builtins {

///////////////////////
// Serializer module //
///////////////////////

class ModSerializer: public Module {
public:
  ModSerializer(): Module("Serializer") {}

  class New: public Builtin<New> {
  public:
    New(): Builtin("new") {}

    static void call(VM vm, Out result) {
      result = SerializerOld::build(vm);
    }
  };

  class Serialize: public Builtin<Serialize> {
  public:
    Serialize(): Builtin("serialize") {}

    static void call(VM vm, In serializer, In todo, Out result) {
      if (serializer.is<SerializerOld>()) {
        auto s=serializer.as<SerializerOld>();
        result = s.doSerialize(vm, todo);
      } else if (serializer.isTransient()) {
        waitFor(vm, serializer);
      } else {
        raiseError(vm, "serializer", serializer);
      }
    }
  };

  class ExtractByLabels: public Builtin<ExtractByLabels> {
  public:
    ExtractByLabels(): Builtin("extractByLabels") {}

    static void call(VM vm, In object, In labelsRecord, Out result) {
      result = SerializerOld::extractByLabels(vm, object, labelsRecord);
    }
  };

  class NewSerializer: public Builtin<NewSerializer> {
  public:
    NewSerializer(): Builtin("newSerializer") {}

    static void call(VM vm, In tag, Out result) {
      result = Serializer::build(vm, tag);
    }
  };
  class Add: public Builtin<Add> {
  public:
    Add(): Builtin("add") {}
    static void call(VM vm, In ser, In add, In imm,  Out result) {
      auto s = safeGet<Serializer>(vm, ser, "serializer");
      result = s.add(vm, add, imm);
    }
  };
  class PutImmediate: public Builtin<PutImmediate> {
  public:
    PutImmediate(): Builtin("putImmediate") {}
    static void call(VM vm, In ser, In ref, In to) {
      auto s = safeGet<Serializer>(vm, ser, "serializer");
      s.putImmediate(vm, getArgument<nativeint>(vm, ref), getArgument<nativeint>(vm, to));
    }
  };
  class SetRoot: public Builtin<SetRoot> {
  public:
    SetRoot(): Builtin("setRoot") {}
    static void call(VM vm, In ser, In ref) {
      auto s = safeGet<Serializer>(vm, ser, "serializer");
      s.setRoot(vm, getArgument<nativeint>(vm, ref));
    }
  };
  class GetSerialized: public Builtin<GetSerialized> {
  public:
    GetSerialized(): Builtin("getSerialized") {}
    static void call(VM vm, In ser, Out result) {
      auto s = safeGet<Serializer>(vm, ser, "serializer");
      result = s.getSerialized(vm);
    }
  };
  class ReleaseSerializer: public Builtin<ReleaseSerializer> {
  public:
    ReleaseSerializer(): Builtin("releaseSerializer") {}
    static void call(VM vm, In ser) {
      auto s = safeGet<Serializer>(vm, ser, "serializer");
      s.release(vm);
    }
  };

  class NewDeserializer: public Builtin<NewDeserializer> {
  public:
    NewDeserializer(): Builtin("newDeserializer") {}

    static void call(VM vm, In bytes, Out res, Out result) {
      result = Unserializer::build(vm, bytes, res);
    }
  };
  class GetRoot: public Builtin<GetRoot> {
  public:
    GetRoot(): Builtin("getRoot") {}
    static void call(VM vm, In deser, Out result) {
      auto un = safeGet<Unserializer>(vm, deser, "deserializer");
      result = un.getRoot(vm);
    }
  };
  class GetTag: public Builtin<GetTag> {
  public:
    GetTag(): Builtin("getTag") {}
    static void call(VM vm, In deser, Out result) {
      auto un = safeGet<Unserializer>(vm, deser, "deserializer");
      result = un.getTag(vm);
    }
  };
  class SetImmediate: public Builtin<SetImmediate> {
  public:
    SetImmediate(): Builtin("setImmediate") {}
    static void call(VM vm, In deser, In gnode, In bytes) {
      auto un = safeGet<Unserializer>(vm, deser, "deserializer");
      un.setImmediate(vm, bytes, getArgument<GlobalNode*>(vm, gnode));
    }
  };
  class ReleaseDeserializer: public Builtin<ReleaseDeserializer> {
  public:
    ReleaseDeserializer(): Builtin("releaseDeserializer") {}
    static void call(VM vm, In deser) {
      auto un = safeGet<Unserializer>(vm, deser, "deserializer");
      un.release(vm);
    }
  };
  class GetGNodeAndType: public Builtin<GetGNodeAndType> {
  public:
    GetGNodeAndType(): Builtin("getGNodeAndType") {}
    static void call(VM vm, In value, Out gnode, Out type) {
      GlobalNode* gn = value.type()->globalize(vm, value);
      value.update();
      gnode.init(vm, gn->reified);
      type = build(vm, value.type()->getTypeAtom(vm));
    }
  };
};

}

}

#endif // MOZART_GENERATOR

#endif // MOZART_MODSERIALIZER_H
