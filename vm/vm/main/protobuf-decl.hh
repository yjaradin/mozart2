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

#ifndef MOZART_PROTOBUF_DECL_H
#define MOZART_PROTOBUF_DECL_H

#include "mozartcore-decl.hh"
#include <google/protobuf/dynamic_message.h>

namespace mozart {
namespace pbuf=::google::protobuf;


#ifndef MOZART_GENERATOR
#include "PBufPool-implem-decl.hh"
#endif

class PBufPool: public DataType<PBufPool> {
public:
  inline
  PBufPool(VM vm, bool generated);
  inline
  PBufPool(VM vm, GR g, PBufPool& from);

  inline
  UnstableNode newMessage(VM vm, RichNode msgType);

  inline
  UnstableNode recordToBytes(VM vm, RichNode msgType, RichNode msg);
  inline
  UnstableNode bytesToRecord(VM vm, RichNode msgType, RichNode msg);
private:
  bool _isGeneratedPool;
  std::shared_ptr<const pbuf::DescriptorPool> _pool;
  std::shared_ptr<pbuf::DynamicMessageFactory> _factory;
};

#ifndef MOZART_GENERATOR
#include "PBufPool-implem-decl-after.hh"
#endif

inline
UnstableNode serializeProtobuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value);
inline
UnstableNode deserializeProtobuf(VM vm, pbuf::MessageFactory* factory,
  const pbuf::Descriptor* descriptor, RichNode value);

#ifndef MOZART_GENERATOR
#include "PBufMessage-implem-decl.hh"
#endif

class PBufMessage: public DataType<PBufMessage> {
public:
  inline
  PBufMessage(VM vm,
    std::shared_ptr<pbuf::DynamicMessageFactory>& factory,
    const pbuf::Descriptor* descriptor);

  inline
  PBufMessage(VM vm, GR gr, PBufMessage& from);

  inline
  PBufMessage(VM vm, PBufMessage* parent, pbuf::Message* msg);

public:
  // Dottable interface

  inline
  bool lookupFeature(VM vm, RichNode feature,
                     nullable<UnstableNode&> value);

  inline
  bool lookupFeature(VM vm, nativeint feature,
                     nullable<UnstableNode&> value);

public:
  // DotAssignable interface

  inline
  void dotAssign(RichNode self, VM vm, RichNode feature, RichNode newValue);

  inline
  UnstableNode dotExchange(RichNode self, VM vm, RichNode feature,
                           RichNode newValue);

public:
  inline
  void addValue(VM vm, RichNode feature, RichNode value);

  inline
  UnstableNode addMessage(VM vm, RichNode feature);

  inline
  UnstableNode serializeToVBS(VM vm);

  inline
  void parseFromVBS(VM vm, RichNode bytes);

private:
  inline
  const pbuf::FieldDescriptor* getFieldDescriptor(VM vm, RichNode feature);

  inline
  UnstableNode getFeature(VM vm, const pbuf::FieldDescriptor* fd);

  inline
  UnstableNode getFeatureAt(VM vm, const pbuf::FieldDescriptor* fd, int i);

  inline
  void setFeature(VM vm, const pbuf::FieldDescriptor* fd, RichNode value);

  std::shared_ptr<pbuf::DynamicMessageFactory> _factory;
  std::shared_ptr<pbuf::Message> _rootMsg;
  pbuf::Message* _msg;
};

#ifndef MOZART_GENERATOR
#include "PBufMessage-implem-decl-after.hh"
#endif

}

#endif // MOZART_PROTOBUF_DECL_H
