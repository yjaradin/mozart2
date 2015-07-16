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

#ifndef MOZART_MODPROTOBUF_H
#define MOZART_MODPROTOBUF_H

#include "../mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

namespace builtins {

class ModProtobuf: public Module {
public:
  ModProtobuf(): Module("Protobuf") {}

  class NewPool: public Builtin<NewPool> {
  public:
    NewPool(): Builtin("newPool") {}
    static void call(VM vm, Out result) {
      result = PBufPool::build(vm, true);
    }
  };

  class BytesToRecord: public Builtin<BytesToRecord> {
  public:
    BytesToRecord(): Builtin("bytesToRecord") {}
    static void call(VM vm, In pool, In type, In msg, Out result) {
      result = safeGet<PBufPool>(vm, pool, "pool").bytesToRecord(vm, type, msg);
    }
  };

  class RecordToBytes: public Builtin<RecordToBytes> {
  public:
    RecordToBytes(): Builtin("recordToBytes") {}
    static void call(VM vm, In pool, In type, In msg, Out result) {
      result = safeGet<PBufPool>(vm, pool, "pool").recordToBytes(vm, type, msg);
    }
  };

  class NewMessage: public Builtin<NewMessage> {
  public:
    NewMessage(): Builtin("newMessage") {}
    static void call(VM vm, In pool, In type, Out result) {
      result = safeGet<PBufPool>(vm, pool, "pool").newMessage(vm, type);
    }
  };

  class AddMessage: public Builtin<AddMessage> {
  public:
    AddMessage(): Builtin("addMessage") {}
    static void call(VM vm, In msg, In feature, Out result) {
    result = safeGet<PBufMessage>(vm, msg, "message").addMessage(vm, feature);
    }
  };

  class AddValue: public Builtin<AddValue> {
  public:
    AddValue(): Builtin("addValue") {}
    static void call(VM vm, In msg, In feature, In value) {
    safeGet<PBufMessage>(vm, msg, "message").addValue(vm, feature, value);
    }
  };

  class Serialize: public Builtin<Serialize> {
  public:
    Serialize(): Builtin("serialize") {}
    static void call(VM vm, In msg, Out result) {
    result = safeGet<PBufMessage>(vm, msg, "message").serializeToVBS(vm);
    }
  };

  class Parse: public Builtin<Parse> {
  public:
    Parse(): Builtin("parse") {}
    static void call(VM vm, In msg, In bytes) {
    safeGet<PBufMessage>(vm, msg, "message").parseFromVBS(vm, bytes);
    }
  };

};

}

}

#endif // MOZART_GENERATOR

#endif // MOZART_MODPROTOBUF_H

class stat;