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

#ifndef MOZART_PORT_H
#define MOZART_PORT_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

//////////
// Port //
//////////

#include "Port-implem.hh"

Port::Port(VM vm, UnstableNode& stream): WithHome(vm), _gnode(nullptr) {
  _stream = ReadOnlyVariable::build(vm);
  stream.copy(vm, _stream);
}

Port::Port(VM vm, RichNode stream): WithHome(vm), _gnode(nullptr) {
  _stream.copy(vm, stream);
}

Port::Port(VM vm, GR gr, Port& from): WithHome(vm, gr, from) {
  gr->copyGNode(_gnode, from._gnode);
  gr->copyUnstableNode(_stream, from._stream);
}

void Port::send(VM vm, RichNode value) {
  // TODO Send to a parent space (no, the following test is not right)
  if (!isHomedInCurrentSpace(vm))
    raise(vm, "globalState", "port");

  sendToReadOnlyStream(vm, _stream, value);
}

UnstableNode Port::sendReceive(VM vm, RichNode value) {
  // TODO Send to a parent space (no, the following test is not right)
  if (!isHomedInCurrentSpace(vm))
    raise(vm, "globalState", "port");

  auto result = OptVar::build(vm);
  sendToReadOnlyStream(vm, _stream, buildSharp(vm, value, result));
  return result;
}

void Port::zombify(RichNode self, VM vm, RichNode proxyPort, UnstableNode& directPort) {
  directPort = Port::build(vm, RichNode(_stream));
  self.become(vm, DistributedPort::build(vm, _gnode, proxyPort));
}

GlobalNode* Port::globalize(RichNode self, VM vm) {
  if (_gnode == nullptr) {
    _gnode = GlobalNode::make(vm, self, vm->coreatoms.port);
  }
  return _gnode;
}

/////////////////////
// DistributedPort //
/////////////////////

#include "DistributedPort-implem.hh"

DistributedPort::DistributedPort(VM vm, GlobalNode* gnode, RichNode proxyPort): _gnode(gnode) {
  _proxyPort.init(vm, proxyPort);
  _sync = Unit::build(vm);
}

DistributedPort::DistributedPort(VM vm, GR gr, DistributedPort& from) {
  gr->copyGNode(_gnode, from._gnode);
  gr->copyStableNode(_proxyPort, from._proxyPort);
  gr->copyUnstableNode(_sync, from._sync);
}

void DistributedPort::send(VM vm, RichNode value) {
  if(RichNode(_sync).isTransient()) waitFor(vm, _sync);
  _sync = OptVar::build(vm);
  UnstableNode t = buildTuple(vm, "portSend", value, _sync);
  PortLike(_proxyPort).send(vm, t);
}

UnstableNode DistributedPort::sendReceive(VM vm, RichNode value) {
  if(RichNode(_sync).isTransient()) waitFor(vm, _sync);
  _sync = OptVar::build(vm);
  auto result = OptVar::build(vm);
  UnstableNode t = buildTuple(vm, "portSendRecv", value, result, _sync);
  PortLike(_proxyPort).send(vm, t);
  return result;
}

GlobalNode* DistributedPort::globalize(RichNode self, VM vm) {
  return _gnode;
}

////////////
// VMPort //
////////////

#include "VMPort-implem.hh"

bool VMPort::equals(VM vm, RichNode right) {
  return value() == right.as<VMPort>().value();
}

void VMPort::send(VM vm, RichNode value) {
  vm->getEnvironment().sendOnVMPort(vm, _identifier, value);
}

UnstableNode VMPort::sendReceive(VM vm, RichNode value) {
  raiseError(vm, "VMPort.sendReceive is not implemented. Send back an ack instead.");
}

}

#endif // MOZART_GENERATOR

#endif // MOZART_PORT_H
