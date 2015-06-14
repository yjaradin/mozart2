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

#ifndef MOZART_BOOSTENVUTILS_H
#define MOZART_BOOSTENVUTILS_H

#include "boostenvutils-decl.hh"

#include "boostenv-decl.hh"

#ifndef MOZART_GENERATOR

namespace mozart { namespace boostenv {

//////////////////////////
// BaseSocketConnection //
//////////////////////////

template <typename T, typename P>
BaseSocketConnection<T, P>::BaseSocketConnection(BoostEnvironment& env, VMIdentifier vm):
  env(env), vm(vm), _socket(env.io_service) {
}

template <typename T, typename P>
void BaseSocketConnection<T, P>::startAsyncRead(
  const ProtectedNode& statusNode) {

  pointer self = this->shared_from_this();
  auto handler = [=] (const boost::system::error_code& error,
                      size_t bytes_transferred) {
    self->readHandler(error, bytes_transferred, statusNode);
  };

  boost::asio::async_read(_socket, boost::asio::buffer(_readData), handler);
}

template <typename T, typename P>
void BaseSocketConnection<T, P>::startAsyncReadSome(
  const ProtectedNode& statusNode) {

  pointer self = this->shared_from_this();
  auto handler = [=] (const boost::system::error_code& error,
                      size_t bytes_transferred) {
    self->readHandler(error, bytes_transferred, statusNode);
  };

  _socket.async_read_some(boost::asio::buffer(_readData), handler);
}

template <typename T, typename P>
void BaseSocketConnection<T, P>::startAsyncWrite(
  const ProtectedNode& statusNode) {

  pointer self = this->shared_from_this();
  auto handler = [=] (const boost::system::error_code& error,
                      size_t bytes_transferred) {
    self->env.postVMEvent(self->vm, [=] (BoostVM& boostVM) {
      if (!error) {
        boostVM.bindAndReleaseAsyncIOFeedbackNode(
          statusNode, bytes_transferred);
      } else {
        boostVM.raiseAndReleaseAsyncIOFeedbackNode(
          statusNode, "socketOrPipe", "write", error.value());
      }
    });
  };

  boost::asio::async_write(_socket, boost::asio::buffer(_writeData), handler);
}

template <typename T, typename P>
void BaseSocketConnection<T, P>::readHandler(
  const boost::system::error_code& error, size_t bytes_transferred,
  const ProtectedNode& statusNode) {

  pointer self = this->shared_from_this();
  env.postVMEvent(vm, [=] (BoostVM& boostVM) {
    if (!error) {
      VM vm = boostVM.vm;
      boostVM.bindAndReleaseAsyncIOFeedbackNode(
        statusNode, "succeeded", bytes_transferred,
        UnstableNode::build<ByteString>(vm, 
          newLString(vm, reinterpret_cast<unsigned char*>(_readData.data()),
            bytes_transferred)));
    } else {
      boostVM.raiseAndReleaseAsyncIOFeedbackNode(
        statusNode, "socketOrPipe", "read", error.value());
    }
  });
}

} }

#endif

#endif // MOZART_BOOSTENVUTILS_H
