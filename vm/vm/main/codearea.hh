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

#ifndef MOZART_CODEAREA_H
#define MOZART_CODEAREA_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

//////////////
// CodeArea //
//////////////

#include "CodeArea-implem.hh"

CodeArea::CodeArea(
  VM vm, size_t Kc, ByteCode* codeBlock, size_t size, size_t arity,
  size_t Xcount, atom_t printName, RichNode debugData)

  : _gnode(nullptr), _size(size), _arity(arity), _Xcount(Xcount), _Kc(Kc),
    _printName(printName) {

  _setCodeBlock(vm, codeBlock, size);

  _debugData.init(vm, debugData);

  // Initialize elements with non-random data
  // TODO An Uninitialized type?
  for (size_t i = 0; i < Kc; i++)
    getElements(i).init(vm);
}

CodeArea::CodeArea(VM vm, size_t Kc, GR gr, CodeArea& from) {
  gr->copyGNode(_gnode, from._gnode);

  _size = from._size;
  _arity = from._arity;
  _Xcount = from._Xcount;
  _Kc = Kc;

  _setCodeBlock(vm, from._codeBlock, _size);

  _printName = gr->copyAtom(from._printName);
  gr->copyStableNode(_debugData, from._debugData);

  gr->copyStableNodes(getElementsArray(), from.getElementsArray(), Kc);
}

CodeArea::CodeArea(VM vm, size_t Kc, Unserializer* un, GlobalNode* gnode, const pb::CodeAreaData& from)
: _gnode(gnode), _size(from.instrs_size() * sizeof(ByteCode)), _arity(from.arity()),
  _Xcount(from.xcount()), _Kc(Kc), _printName(vm->getAtom(from.printname())) {

  _debugData.init(vm, un->getFromRef(from.debug()));
  for (size_t i = 0; i < Kc; i++)
    getElements(i).init(vm, un->getFromRef(from.kregs().Get(i)));
  _codeBlock = new (vm) ByteCode[_size / sizeof(ByteCode)];
  for (int i = 0; i < from.instrs_size(); i++)
    _codeBlock[i] = from.instrs().Get(i);
}

void CodeArea::getCodeAreaInfo(
  VM vm, size_t& arity, ProgramCounter& start, size_t& Xcount,
  StaticArray<StableNode>& Ks) {

  arity = _arity;
  start = _codeBlock;
  Xcount = _Xcount;
  Ks = getElementsArray();
}

void CodeArea::getCodeAreaDebugInfo(VM vm, atom_t& printName,
                                    UnstableNode& debugData) {
  printName = _printName;
  debugData.copy(vm, _debugData);
}

void CodeArea::printReprToStream(VM vm, std::ostream& out,
                                 int depth, int width) {
  out << "<CodeArea for <P/" << _arity;
  if (_printName != vm->coreatoms.empty)
    out << " " << _printName;
  out << ">";
  if (!RichNode(_debugData).is<Unit>())
    out << " " << repr(vm, _debugData, depth, width);
  out << ">";
}

UnstableNode CodeArea::serialize(VM vm, SE se) {
  UnstableNode codeAtom = mozart::build(vm, "code");
  UnstableNode block = buildTupleDynamic(
    vm, codeAtom, _size / sizeof(ByteCode), _codeBlock,
    [=](ByteCode b) {
      return mozart::build(vm, (nativeint) b);
    });

  UnstableNode Ks = makeTuple(vm, "registers", _Kc);
  if (_Kc != 0) {
    auto elements = RichNode(Ks).as<Tuple>().getElementsArray();
    for (size_t i = 0; i< _Kc; ++i) {
      se->copy(elements[i], getElements(i));
    }
  }

  UnstableNode result = buildTuple(vm, vm->coreatoms.codearea,
                                   std::move(block), _arity, _Xcount,
                                   std::move(Ks), _printName, OptVar::build(vm));

  se->copy(RichNode(result).as<Tuple>().getElements(5), _debugData);

  return result;
}

bool CodeArea::serialize(RichNode self, VM vm, SerializerCallback* cb, pb::Value* val) {
  globalize(self, vm);
  cb->fillResource(val, _gnode, vm->coreatoms.codearea, self);
  return true;
}

bool CodeArea::serializeImmediate(RichNode self, VM vm, SerializerCallback* cb, pb::ImmediateData* data){
  auto imm = data->mutable_codearea();
  for (unsigned int i=0; i<_size / sizeof(ByteCode); ++i) {
    imm->add_instrs((int) _codeBlock[i]);
  }
  imm->set_arity(_arity);
  imm->set_xcount(_Xcount);
  imm->set_printname(_printName.contents(), _printName.length());
  cb->copy(imm->mutable_debug(), _debugData);
  for (size_t i=0; i< _Kc; ++i) {
    cb->copy(imm->add_kregs(), getElements(i));
  }
  return true;
}

GlobalNode* CodeArea::globalize(RichNode self, VM vm) {
  if (_gnode == nullptr) {
    _gnode = GlobalNode::make(vm, self, vm->coreatoms.immediate);
  }
  return _gnode;
}

void CodeArea::setUUID(RichNode self, VM vm, const UUID& uuid) {
  assert(_gnode == nullptr);
  _gnode = GlobalNode::make(vm, uuid, self, vm->coreatoms.immediate);
}

}

#endif // MOZART_GENERATOR

#endif // MOZART_CODEAREA_H
