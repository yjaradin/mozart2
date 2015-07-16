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

#ifndef MOZART_PORT_DECL_H
#define MOZART_PORT_DECL_H

#include "mozartcore-decl.hh"

namespace mozart {

//////////
// Port //
//////////

#ifndef MOZART_GENERATOR
#include "Port-implem-decl.hh"
#endif

class Port: public DataType<Port>, public WithHome {
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom("port");
  }

  inline
  Port(VM vm, UnstableNode& stream);

  inline
  Port(VM vm, RichNode stream);

  inline
  Port(VM vm, GR gr, Port& from);

public:
  // PortLike interface

  bool isPort(VM vm) {
    return true;
  }

  inline
  void send(VM vm, RichNode value);

  inline
  UnstableNode sendReceive(VM vm, RichNode value);

public:
  inline
  void zombify(RichNode self, VM vm, RichNode proxyPort, UnstableNode& directPort);

public:
  // Miscellaneous

  void printReprToStream(VM vm, std::ostream& out, int depth, int width) {
    out << "<Port>";
  }

  inline
  GlobalNode* globalize(RichNode self, VM vm);


private:
  GlobalNode* _gnode;
  UnstableNode _stream;
};

#ifndef MOZART_GENERATOR
#include "Port-implem-decl-after.hh"
#endif

/////////////////////
// DistributedPort //
/////////////////////

#ifndef MOZART_GENERATOR
#include "DistributedPort-implem-decl.hh"
#endif

class DistributedPort: public DataType<DistributedPort> {
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom("port");
  }

  inline
  DistributedPort(VM vm, GlobalNode* gnode, RichNode proxyPort);

  inline
  DistributedPort(VM vm, GR gr, DistributedPort& from);

public:
  // PortLike interface

  bool isPort(VM vm) {
    return true;
  }

  inline
  void send(VM vm, RichNode value);

  inline
  UnstableNode sendReceive(VM vm, RichNode value);

public:
  // Miscellaneous

  void printReprToStream(VM vm, std::ostream& out, int depth, int width) {
    out << "<Port>";
  }

  inline
  GlobalNode* globalize(RichNode self, VM vm);


private:
  GlobalNode* _gnode;
  StableNode _proxyPort;
  UnstableNode _sync;
};

#ifndef MOZART_GENERATOR
#include "DistributedPort-implem-decl-after.hh"
#endif

////////////
// VMPort //
////////////

#ifndef MOZART_GENERATOR
#include "VMPort-implem-decl.hh"
#endif

class VMPort: public DataType<VMPort>,
  StoredAs<VMIdentifier>, WithValueBehavior {
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom("port");
  }

  explicit VMPort(VMIdentifier targetVM): _identifier(targetVM) {}

  static void create(VMIdentifier& self, VM vm, VMIdentifier target) {
    self = target;
  }

  static void create(VMIdentifier& self, VM vm, GR gr, VMPort from) {
    self = from._identifier;
  }

public:
  VMIdentifier value() {
    return _identifier;
  }

public:
  inline
  bool equals(VM vm, RichNode right);

public:
  // PortLike interface

  bool isPort(VM vm) {
    return true;
  }

  inline
  void send(VM vm, RichNode value);

  inline
  UnstableNode sendReceive(VM vm, RichNode value);

public:
  // Miscellaneous

  void printReprToStream(VM vm, std::ostream& out, int depth, int width) {
    out << "<Port>";
  }

private:
  VMIdentifier _identifier;
};

#ifndef MOZART_GENERATOR
#include "VMPort-implem-decl-after.hh"
#endif

}

#endif // MOZART_PORT_DECL_H
