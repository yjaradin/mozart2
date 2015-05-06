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

#ifndef MOZART_CALLABLES_H
#define MOZART_CALLABLES_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

//////////////////////
// BuiltinProcedure //
//////////////////////

#include "BuiltinProcedure-implem.hh"

void BuiltinProcedure::create(Builtin*& self, VM vm, GR gr,
                              BuiltinProcedure from) {
  self = from._builtin;
}

bool BuiltinProcedure::equals(VM vm, RichNode right) {
  return value() == right.as<BuiltinProcedure>().value();
}

atom_t BuiltinProcedure::getPrintName(VM vm) {
  return _builtin->getPrintName(vm);
}

void BuiltinProcedure::callBuiltin(VM vm, size_t argc, UnstableNode* args[]) {
  assert(argc == getArity());
  return _builtin->callBuiltin(vm, args);
}

template <class... Args>
void BuiltinProcedure::callBuiltin(VM vm, Args&&... args) {
  assert(sizeof...(args) == getArity());
  return _builtin->callBuiltin(vm, std::forward<Args>(args)...);
}

size_t BuiltinProcedure::procedureArity(VM vm) {
  return getArity();
}

void BuiltinProcedure::getCallInfo(
  RichNode self, VM vm, size_t& arity, ProgramCounter& start,
  size_t& Xcount, StaticArray<StableNode>& Gs, StaticArray<StableNode>& Ks) {

  return _builtin->getCallInfo(self, vm, arity, start, Xcount, Gs, Ks);
}

void BuiltinProcedure::getDebugInfo(
  RichNode self, VM vm, atom_t& printName, UnstableNode& debugData) {

  printName = _builtin->getPrintName(vm);
  debugData = mozart::build(vm, unit);
}

UnstableNode BuiltinProcedure::serialize(VM vm, SE se) {
  return buildTuple(vm, vm->coreatoms.builtin,
                    _builtin->getModuleNameAtom(vm), _builtin->getNameAtom(vm));
}

bool BuiltinProcedure::serialize(VM vm, SerializerCallback* cb, pb::Value* val) {
  cb->copyAndBuild(val->mutable_builtin()->mutable_module(), _builtin->getModuleNameAtom(vm));
  cb->copyAndBuild(val->mutable_builtin()->mutable_module(), _builtin->getNameAtom(vm));
  return true;
}

void BuiltinProcedure::printReprToStream(VM vm, std::ostream& out,
                                         int depth, int width) {
  atom_t printName = _builtin->getPrintName(vm);

  out << "<P/" << _builtin->getArity();
  if (printName != vm->coreatoms.empty)
    out << " " << makeLString(printName.contents(), printName.length());
  out << ">";
}

/////////////////
// Abstraction //
/////////////////

#include "Abstraction-implem.hh"

Abstraction::Abstraction(VM vm, size_t Gc, RichNode body)
  : WithHome(vm), _gnode(nullptr), _Gc(Gc) {

  _body.init(vm, body);
  _codeAreaCacheValid = false;

  // Initialize elements with non-random data
  // TODO An Uninitialized type?
  for (size_t i = 0; i < Gc; i++)
    getElements(i).init(vm);
}

Abstraction::Abstraction(VM vm, size_t Gc, GR gr, Abstraction& from):
  WithHome(vm, gr, from) {

  gr->copyGNode(_gnode, from._gnode);
  gr->copyStableNode(_body, from._body);
  _Gc = Gc;

  _codeAreaCacheValid = false;

  gr->copyStableNodes(getElementsArray(), from.getElementsArray(), Gc);
}

atom_t Abstraction::getPrintName(VM vm) {
  atom_t result;
  UnstableNode dummy;
  getDebugInfo(vm, result, dummy);
  return result;
}

size_t Abstraction::procedureArity(VM vm) {
  ensureCodeAreaCacheValid(vm);
  return _arity;
}

void Abstraction::getCallInfo(
  VM vm, size_t& arity, ProgramCounter& start,
  size_t& Xcount, StaticArray<StableNode>& Gs, StaticArray<StableNode>& Ks) {

  ensureCodeAreaCacheValid(vm);

  arity = _arity;
  start = _start;
  Xcount = _Xcount;
  Gs = getElementsArray();
  Ks = _Ks;
}

void Abstraction::getDebugInfo(VM vm, atom_t& printName,
                               UnstableNode& debugData) {
  return CodeAreaProvider(_body).getCodeAreaDebugInfo(vm, printName, debugData);
}

void Abstraction::printReprToStream(VM vm, std::ostream& out,
                                    int depth, int width) {
  MOZART_TRY(vm) {
    ensureCodeAreaCacheValid(vm);
    atom_t printName = getPrintName(vm);

    out << "<P/" << _arity;
    if (printName != vm->coreatoms.empty)
      out << " " << makeLString(printName.contents(), printName.length());
    out << ">";
  } MOZART_CATCH(vm, kind, node) {
    out << "<P/?>";
  } MOZART_ENDTRY(vm);
}

UnstableNode Abstraction::serialize(VM vm, SE se) {
  UnstableNode r = makeTuple(vm, vm->coreatoms.abstraction, _Gc+1);
  auto elements=RichNode(r).as<Tuple>().getElementsArray();
  for (size_t i=0; i< _Gc; ++i) {
    se->copy(elements[i], getElements(i));
  }
  se->copy(elements[_Gc], _body);
  return r;
}

bool Abstraction::serialize(RichNode self, VM vm, SerializerCallback* cb, pb::Value* val) {
  globalize(self, vm);
  cb->fillResource(val, _gnode, vm->coreatoms.abstraction, self);
  return true;
}

bool Abstraction::serializeImmediate(RichNode self, VM vm, SerializerCallback* cb, pb::ImmediateData* data){
  auto imm = data->mutable_abstraction();
  cb->copy(imm->mutable_codearea(), _body);
  for (size_t i=0; i< _Gc; ++i) {
    cb->copy(imm->add_gregs(), getElements(i));
  }
  return true;
}

GlobalNode* Abstraction::globalize(RichNode self, VM vm) {
  if (_gnode == nullptr) {
    _gnode = GlobalNode::make(vm, self, vm->coreatoms.immediate);
  }
  return _gnode;
}

void Abstraction::setUUID(RichNode self, VM vm, const UUID& uuid) {
  assert(_gnode == nullptr);
  _gnode = GlobalNode::make(vm, uuid, self, vm->coreatoms.immediate);
}

void Abstraction::ensureCodeAreaCacheValid(VM vm) {
  if (!_codeAreaCacheValid)
    fillCodeAreaCache(vm);
}

void Abstraction::fillCodeAreaCache(VM vm) {
  CodeAreaProvider(_body).getCodeAreaInfo(vm, _arity, _start, _Xcount, _Ks);
  _codeAreaCacheValid = true;
}

}

#endif // MOZART_GENERATOR

#endif // MOZART_CALLABLES_H
