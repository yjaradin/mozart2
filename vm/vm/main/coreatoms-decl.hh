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

#ifndef MOZART_COREATOMS_DECL_H
#define MOZART_COREATOMS_DECL_H

#include "core-forward-decl.hh"

#include "atomtable.hh"

namespace mozart {

struct CoreAtoms {
  inline
  void initialize(VM vm, AtomTable& atomTable);

  // ''
  atom_t empty;

  // nil, '|' and '#'
  atom_t nil;
  atom_t pipe;
  atom_t sharp;

  // Serialization types
  atom_t int_;
  atom_t float_;
  atom_t bool_;
  atom_t unit;
  atom_t atom;
  atom_t cons;
  atom_t tuple;
  atom_t arity;
  atom_t record;
  atom_t builtin;
  atom_t codearea;
  atom_t patmatwildcard;
  atom_t patmatcapture;
  atom_t patmatconjunction;
  atom_t patmatopenrecord;
  atom_t abstraction;
  atom_t chunk;
  atom_t uniquename;
  atom_t name;
  atom_t namedname;
  atom_t unicodeString;

  //Serialization protocols
  atom_t immediate;
  atom_t port;

  // Object Orientation
  unique_name_t ooMeth;
  unique_name_t ooFastMeth;
  unique_name_t ooDefaults;
  unique_name_t ooAttr;
  unique_name_t ooFeat;
  unique_name_t ooFreeFeat;
  unique_name_t ooFreeFlag;
  unique_name_t ooMethSrc;
  unique_name_t ooAttrSrc;
  unique_name_t ooFeatSrc;
  unique_name_t ooPrintName;
  unique_name_t ooFallback;

  // Space status
  atom_t succeeded;
  atom_t entailed;
  atom_t stuck;
  atom_t suspended;
  atom_t alternatives;
  atom_t failed;
  atom_t merged;

  // Unicode error types
  atom_t outOfRange;
  atom_t surrogate;
  atom_t invalidUTF8;
  atom_t invalidUTF16;
  atom_t truncated;

  // Exceptions
  atom_t debug;
  atom_t error;
  atom_t system;
  atom_t failure;
  atom_t kernel;
  atom_t illegalFieldSelection;
  atom_t unicodeError;
  atom_t spaceAdmissible;
  atom_t spaceNoChoice;
  atom_t spaceAltRange;
  atom_t spaceMerged;
  atom_t indexOutOfBounds;
};

}

#endif // MOZART_COREATOMS_DECL_H
