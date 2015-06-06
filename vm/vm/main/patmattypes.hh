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

#ifndef MOZART_PATMATTYPES_H
#define MOZART_PATMATTYPES_H

#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

///////////////////
// PatMatCapture //
///////////////////

#include "PatMatCapture-implem.hh"

void PatMatCapture::create(nativeint& self, VM vm, GR gr, PatMatCapture from) {
  self = from.index();
}

bool PatMatCapture::equals(VM vm, RichNode right) {
  return index() == right.as<PatMatCapture>().index();
}

void PatMatCapture::printReprToStream(VM vm, std::ostream& out,
                                      int depth, int width) {
  out << "<Capture/" << index() << ">";
}

UnstableNode PatMatCapture::serialize(VM vm, SE se) {
  if (index() == -1)
    return mozart::build(vm, vm->coreatoms.patmatwildcard);
  else
    return buildTuple(vm, vm->coreatoms.patmatcapture, index());
}

bool PatMatCapture::serialize(VM vm, SerializerCallback* cb, pb::Value* val) {
  if (index()==-1)
    val->mutable_patmatwildcard();
  else
    val->mutable_patmatcapture()->set_pos(index());
  return true;
}

///////////////////////
// PatMatConjunction //
///////////////////////

#include "PatMatConjunction-implem.hh"

PatMatConjunction::PatMatConjunction(VM vm, size_t count) {
  _count = count;

  // Initialize elements with non-random data
  // TODO An Uninitialized type?
  for (size_t i = 0; i < count; i++)
    getElements(i).init(vm);
}

PatMatConjunction::PatMatConjunction(VM vm, size_t count, GR gr,
                                     PatMatConjunction& from) {
  _count = count;

  gr->copyStableNodes(getElementsArray(), from.getElementsArray(), count);
}

PatMatConjunction::PatMatConjunction(VM vm, size_t count, Unserializer* un,
                                     const pb::PatMatConjunction& from) {
  _count = count;
  for (size_t i = 0; i < count; i++)
    getElements(i).init(vm, un->getFromRef(from.terms().Get(i)));
}

StableNode* PatMatConjunction::getElement(size_t index) {
  return &getElements(index);
}

bool PatMatConjunction::equals(VM vm, RichNode right, WalkStack& stack) {
  auto rhs = right.as<PatMatConjunction>();

  if (getCount() != rhs.getCount())
    return false;

  stack.pushArray(vm, getElementsArray(), rhs.getElementsArray(), getCount());

  return true;
}

void PatMatConjunction::printReprToStream(VM vm, std::ostream& out,
                                          int depth, int width) {
  out << "<PatMatConjunction>(";

  if (depth <= 0) {
    out << "...";
  } else {
    for (size_t i = 0; i < _count; i++) {
      if (i > 0)
        out << ", ";
      out << repr(vm, getElements(i), depth, width);
    }
  }

  out << ")";
}

UnstableNode PatMatConjunction::serialize(VM vm, SE se) {
  UnstableNode r = makeTuple(vm, vm->coreatoms.patmatconjunction, _count);
  auto elements = RichNode(r).as<Tuple>().getElementsArray();
  for (size_t i=0; i<_count; ++i) {
    se->copy(elements[i], getElements(i));
  }
  return r;
}

bool PatMatConjunction::serialize(VM vm, SerializerCallback* cb, pb::Value* val) {
  pb::PatMatConjunction* conj = val->mutable_patmatconjunction();
  for (size_t i=0; i<_count; ++i) {
    cb->copy(conj->add_terms(), getElements(i));
  }
  return true;
}
//////////////////////
// PatMatOpenRecord //
//////////////////////

#include "PatMatOpenRecord-implem.hh"

template <typename A>
PatMatOpenRecord::PatMatOpenRecord(VM vm, size_t width, A&& arity) {
  _arity.init(vm, std::forward<A>(arity));
  _width = width;

  assert(RichNode(_arity).is<Arity>());

  // Initialize elements with non-random data
  // TODO An Uninitialized type?
  for (size_t i = 0; i < width; i++)
    getElements(i).init(vm);
}

PatMatOpenRecord::PatMatOpenRecord(VM vm, size_t width, GR gr,
                                   PatMatOpenRecord& from) {
  gr->copyStableNode(_arity, from._arity);
  _width = width;

  gr->copyStableNodes(getElementsArray(), from.getElementsArray(), width);
}

PatMatOpenRecord::PatMatOpenRecord(VM vm, size_t width, Unserializer* un,
                                   const pb::PatMatOpenRecord& from) {
  _arity.init(vm, un->getFromRef(from.arity()));
  _width = width;
  for (size_t i = 0; i < width; i++)
    getElements(i).init(vm, un->getFromRef(from.fields().Get(i)));
}

StableNode* PatMatOpenRecord::getElement(size_t index) {
  return &getElements(index);
}

void PatMatOpenRecord::printReprToStream(VM vm, std::ostream& out,
                                         int depth, int width) {
  auto arity = RichNode(_arity).as<Arity>();

  out << "<PatMatOpenRecord " << repr(vm, *arity.getLabel(), depth+1, width);
  out << "(";

  for (size_t i = 0; i < _width; i++) {
    if ((nativeint) i >= width) {
      out << "... ";
      break;
    }

    out << repr(vm, *arity.getElement(i), depth, width) << ":";
    out << repr(vm, getElements(i), depth, width) << " ";
  }

  out << "...)>";
}

UnstableNode PatMatOpenRecord::serialize(VM vm, SE se) {
  UnstableNode r = makeTuple(vm, vm->coreatoms.patmatopenrecord, _width+1);
  auto elements = RichNode(r).as<Tuple>().getElementsArray();
  for (size_t i=0; i< _width; ++i) {
    se->copy(elements[i], getElements(i));
  }
  se->copy(elements[_width], _arity);
  return r;
}

bool PatMatOpenRecord::serialize(VM vm, SerializerCallback* cb, pb::Value* val) {
  pb::PatMatOpenRecord* orec = val->mutable_patmatopenrecord();
  for (size_t i=0; i<_width; ++i) {
    cb->copy(orec->add_fields(), getElements(i));
  }
  cb->copy(orec->mutable_arity(), _arity);
  return true;
}

nativeint PatMatUtils::maxX(VM vm, RichNode pattern) {
  nativeint max = -1;
  VMAllocatedList<RichNode> todo;
  VMAllocatedList<NodeBackup> backup;
  pattern.ensureStable(vm);
  todo.push_back(vm, pattern);
  while (!todo.empty()) {
    RichNode current = todo.pop_front(vm);
    bool recur = false;
    if (pattern.is<PatMatCapture>()) {
      nativeint cand = pattern.as<PatMatCapture>().index();
      if (cand > max) max = cand;
    } else if (pattern.is<PatMatOpenRecord>()) {
      auto openrec = pattern.as<PatMatOpenRecord>();
      for (size_t i = 0; i < openrec.getCount(); ++i) {
	todo.push_back(vm, *openrec.getElement(i));
	recur = true;
      }
    } else if (pattern.is<PatMatConjunction>()) {
      auto conj = pattern.as<PatMatConjunction>();
      for (size_t i = 0; i < conj.getCount(); ++i) {
	todo.push_back(vm, *conj.getElement(i));
	recur = true;
      }	  
    } else if (pattern.is<Record>()) {
      auto rec = pattern.as<Record>();
      for (size_t i = 0; i < rec.getWidth(); ++i) {
	todo.push_back(vm, *rec.getElement(i));
	recur = true;
      }	  
    } else if (pattern.is<Tuple>()) {
      auto tup = pattern.as<Tuple>();
      for (size_t i = 0; i < tup.getWidth(); ++i) {
	todo.push_back(vm, *tup.getElement(i));
	recur = true;
      }	  
    } else if (pattern.is<Cons>()) {
      auto cons = pattern.as<Cons>();
      todo.push_back(vm, *cons.getHead());
      todo.push_back(vm, *cons.getTail());
      recur = true;
    }
    if (recur) {
      backup.push_front(vm, current.makeBackup());
      current.reinit(vm, Unit::build(vm));
    }
  }
  while (!backup.empty()) {
    backup.pop_front(vm).restore();
  }
  return max;
}

}

#endif // MOZART_GENERATOR

#endif // MOZART_PATMATTYPES_H
