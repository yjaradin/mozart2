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

#ifndef MOZART_SERIALIZER_H
#define MOZART_SERIALIZER_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

///////////////////////////
// SerializationCallback //
///////////////////////////

SerializationCallback::SerializationCallback(VM vm): vm(vm) {}

void SerializationCallback::copy(RichNode to, RichNode from) {
  todoFrom.push_back(vm, from);
  todoTo.push_back(vm, to);
}

////////////////
// Serialized //
////////////////

#include "Serialized-implem.hh"

///////////////////
// SerializerOld //
///////////////////

#include "SerializerOld-implem.hh"

SerializerOld::SerializerOld(VM vm):
  done(buildNil(vm)) {}

SerializerOld::SerializerOld(VM vm, GR gr, SerializerOld& from) {
  gr->copyUnstableNode(done, from.done);
}

}

#endif // MOZART_GENERATOR

#endif // __SERIALIZER_DECL_H
