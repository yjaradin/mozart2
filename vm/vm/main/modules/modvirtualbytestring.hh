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

#ifndef MOZART_MODVIRTUALBYTESTRING_H
#define MOZART_MODVIRTUALBYTESTRING_H

#include "../mozartcore.hh"

#include <sstream>
#include <cstdlib>
#include <boost/concept_check.hpp>

#ifndef MOZART_GENERATOR

namespace mozart {

namespace builtins {

//////////////////////////////
// VirtualByteString module //
//////////////////////////////

class ModVirtualByteString : public Module {
public:
  ModVirtualByteString() : Module("VirtualByteString") {}

  class Is : public Builtin<Is> {
  public:
    Is() : Builtin("is") {}

    static void call(VM vm, In value, Out result) {
      result = build(vm, ozIsVirtualByteString(vm, value));
    }
  };

  class ToCompactByteString : public Builtin<ToCompactByteString> {
  public:
    ToCompactByteString() : Builtin("toCompactByteString") {}

    static void call(VM vm, In value, Out result) {
      size_t bufSize = ozVBSLengthForBuffer(vm, value);

      if (value.is<ByteString>()) {
        result.copy(vm, value);
        return;
      }

      {
        std::vector<unsigned char> buffer;
        ozVBSGet(vm, value, bufSize, buffer);
        result = ByteString::build(vm, newLString(vm, buffer));
      }
    }
  };

  class ToByteList : public Builtin<ToByteList> {
  public:
    ToByteList() : Builtin("toByteList") {}

    static void call(VM vm, In value, In tail, Out result) {
      size_t bufSize = ozVBSLengthForBuffer(vm, value);

      if (value.is<Cons>() ||
          patternmatching::matches(vm, value, vm->coreatoms.nil)) {
        result.copy(vm, value);
        return;
      }

      {
        std::vector<unsigned char> buffer;
        ozVBSGet(vm, value, bufSize, buffer);

        OzListBuilder builder(vm);
        for (auto iter = buffer.rbegin(); iter != buffer.rend(); ++iter)
          builder.push_front(vm, (nativeint) *iter);
        result = builder.get(vm, tail);
      }
    }
  };

  class Length : public Builtin<Length> {
  public:
    Length() : Builtin("length") {}

    static void call(VM vm, In value, Out result) {
      result = build(vm, ozVBSLength(vm, value));
    }
  };

  class GetByteAt : public Builtin<GetByteAt> {
  public:
    GetByteAt() : Builtin("getByteAt") {}

    static void call(VM vm, In value, In at, Out result) {
      size_t pos = getArgument<size_t>(vm, at);
      result = build(vm, ozVBSGetAt(vm, value, pos));
    }
  };

  static constexpr char encoder[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";
  static int decoderInternal[256];
  static int *decoder;
  static void initDecoder(){
    for (int i=0;i<256;i++) decoderInternal[i] = -1;
    for (int i=0;i<64;i++) decoderInternal[(unsigned char) encoder[i]] = i;
    decoderInternal[(unsigned char) '='] = -2;
    decoder = decoderInternal;
  }
  class FromBase64: public Builtin<FromBase64> {
  public:
    FromBase64() : Builtin("fromBase64") {}

    static void call(VM vm, In value, Out result) {
      if(!decoder) initDecoder();
      size_t bufSize = ozVSLengthForBuffer(vm, value);
      std::vector<char> buffer;
      ozVSGet(vm, value, bufSize, buffer);

      OzListBuilder builder(vm);

      unsigned int tmp = 0;
      unsigned int count = 0;
      for (auto iter = buffer.begin(); iter != buffer.end(); ++iter) {
        int v = decoder[(unsigned char)*iter];
        if (v == -1) continue;
        if (v == -2) break;
        tmp <<= 6;
        tmp |= v;
        switch (count % 4) {
          case 0: break;
          case 1:
            builder.push_back(vm, (nativeint) tmp>>4);
            tmp &= 0xF;
            break;
          case 2:
            builder.push_back(vm, (nativeint) tmp>>2);
            tmp &= 0x3;
            break;
          case 3:
            builder.push_back(vm, (nativeint) tmp);
            tmp = 0;
            break;
        }
        count++;
      }
      result = builder.get(vm, vm->coreatoms.nil);
    }
  };

  class ToBase64: public Builtin<ToBase64> {
  public:
    ToBase64() : Builtin("toBase64") {}

    static void call(VM vm, In value, Out result) {
      size_t bufSize = ozVBSLengthForBuffer(vm, value);
      std::vector<unsigned char> buffer;
      ozVBSGet(vm, value, bufSize, buffer);
      OzListBuilder builder(vm);
      unsigned int tmp = 0;
      unsigned int count = 0;
      for (auto iter = buffer.begin(); iter != buffer.end(); ++iter) {
        tmp <<= 8;
        tmp |= *iter;
        switch (count % 3) {
          case 0:
            builder.push_back(vm, (nativeint) encoder[tmp>>2]);
            tmp &= 0x3;
            break;
          case 1:
            builder.push_back(vm, (nativeint) encoder[tmp>>4]);
            tmp &= 0xF;
            break;
          case 2:
            builder.push_back(vm, (nativeint) encoder[tmp>>6]);
            tmp &= 0x3F;
            builder.push_back(vm, (nativeint) encoder[tmp]);
            tmp = 0;
            break;
        }
        count++;
      }
      switch (count % 3) {
        case 2:
          builder.push_back(vm, (nativeint) encoder[tmp<<2]);
          builder.push_back(vm, (nativeint) '=');
          break;
        case 1:
          builder.push_back(vm, (nativeint) encoder[tmp<<4]);
          builder.push_back(vm, (nativeint) '=');
          builder.push_back(vm, (nativeint) '=');
          break;
        case 0:
          break;
      }
      result = builder.get(vm, vm->coreatoms.nil);
    }
  };
  
  class NewUUID: public Builtin<NewUUID> {
  public:
    NewUUID(): Builtin("newUUID") {}

    static void call(VM vm, Out result) {
      result = build(vm, vm->genUUID());
    }
  };
};

}

}

#endif

#endif // MOZART_MODVIRTUALBYTESTRING_H
