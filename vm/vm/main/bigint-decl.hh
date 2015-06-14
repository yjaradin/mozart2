// Copyright © 2011, Université catholique de Louvain
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

#ifndef __BIGINT_DECL_H
#define __BIGINT_DECL_H

#include "mozartcore-decl.hh"
#include "bigintimplem-decl.hh"

namespace mozart {

#ifndef MOZART_GENERATOR
#include "BigInt-implem-decl.hh"
#endif

class BigInt: public DataType<BigInt>, WithValueBehavior {
public:
  static constexpr UUID uuid = "{00000000-0000-9e00-0000-000000000002}";

  static atom_t getTypeAtom(VM vm) {
    return vm->coreatoms.int_;
  }

  template <class T>
  BigInt(VM vm, T value):
    _value(vm->getEnvironment().newBigIntImplem(vm, value)) {}

  BigInt(VM vm, const std::shared_ptr<BigIntImplem>& p): _value(p) {}

  BigInt(VM vm, GR gr, BigInt& from): _value(std::move(from._value)) {}

public:
  std::shared_ptr<BigIntImplem> value() { return _value; }

  inline
  bool equals(VM vm, RichNode right);

  inline
  int compareFeatures(VM vm, RichNode right);

public:
  // Comparable interface

  inline
  int compare(VM vm, RichNode right);

public:
  // Numeric inteface

  bool isNumber(VM vm) {
    return true;
  }

  bool isInt(VM vm) {
    return true;
  }

  bool isFloat(VM vm) {
    return false;
  }

  inline
  UnstableNode opposite(VM vm);

  inline
  UnstableNode add(VM vm, RichNode right);

  inline
  UnstableNode add(VM vm, nativeint b);

  inline
  UnstableNode subtract(VM vm, RichNode right);

  inline
  UnstableNode multiply(VM vm, RichNode right);

  inline
  UnstableNode div(VM vm, RichNode right);

  inline
  UnstableNode mod(VM vm, RichNode right);

  inline
  UnstableNode abs(RichNode self, VM vm);

public:
  // Miscellaneous

  double doubleValue() {
    return value()->doubleValue();
  }

  bool asInt64(std::int64_t& dest) {
    return value()->asInt64(dest);
  }

  bool asUInt64(std::uint64_t& dest) {
    return value()->asUInt64(dest);
  }

  std::string str() {
    return value()->str();
  }

  void printReprToStream(VM vm, std::ostream& out, int depth, int width) {
    value()->printReprToStream(vm, out, depth, width);
  }

  bool serialize(VM vm, SerializerCallback* cb, pb::Value* val) {
    val->mutable_integer()->set_value(str());
    return true;
  }

private:
  friend UnstableNode deserialize(VM vm, Unserializer* un, const pb::Integer& from);

  inline
  static UnstableNode shrink(VM vm, const std::shared_ptr<BigIntImplem>& p);

  inline
  static std::shared_ptr<BigIntImplem> coerce(VM vm, RichNode value);

private:
  std::shared_ptr<BigIntImplem> _value;
};

inline
UnstableNode deserialize(VM vm, Unserializer* un, const pb::Integer& from);


#ifndef MOZART_GENERATOR
#include "BigInt-implem-decl-after.hh"
#endif

}

#endif // __BIGINT_DECL_H
