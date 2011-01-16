/*
 * instructions.h - Intermediate code instructions
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lmntal@ueda.info.waseda.ac.jp>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *    3. Neither the name of the Ueda Laboratory LMNtal Group nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: instruction.h,v 1.5 2008/09/19 05:18:17 taisuke Exp $
 */

#ifndef LMN_INSTRUCTION_H
#define LMN_INSTRUCTION_H

#include "lmntal.h"

enum LmnInstruction {
  INSTR_OPT,
  INSTR_DUMMY,
  INSTR_UNDEF,
  INSTR_DEREF,
  INSTR_DEREFATOM,
  INSTR_DEREFLINK,
  INSTR_FINDATOM,
  INSTR_LOCKMEM,
  INSTR_ANYMEM,
  INSTR_LOCK,
  INSTR_GETMEM,
  INSTR_GETPARENT,
  INSTR_TESTMEM,
  INSTR_NORULES,
  INSTR_NFREELINKS,
  INSTR_NATOMS,
  INSTR_NATOMSINDIRECT,
  INSTR_NMEMS,
  INSTR_EQMEM,
  INSTR_NEQMEM,
  INSTR_STABLE,
  INSTR_FUNC,
  INSTR_NOTFUNC,
  INSTR_EQATOM,
  INSTR_NEQATOM,
  INSTR_SAMEFUNC,
  INSTR_DEREFFUNC,
  INSTR_GETFUNC,
  INSTR_LOADFUNC,
  INSTR_EQFUNC,
  INSTR_NEQFUNC,
  INSTR_REMOVEATOM,
  INSTR_NEWATOM,
  INSTR_NEWATOMINDIRECT,
  INSTR_NEWATOM_INT,
  INSTR_NEWATOM_DOUBLE,
  INSTR_ENQUEUEATOM,
  INSTR_DEQUEUEATOM,
  INSTR_FREEATOM,
  INSTR_ALTERFUNC,
  INSTR_ALTERFUNCINDIRECT,
  INSTR_ALLOCATOM,
  INSTR_ALLOCATOMINDIRECT,
  INSTR_COPYATOM,
  INSTR_ADDATOM,
  INSTR_REMOVEMEM,
  INSTR_NEWMEM,
  INSTR_ALLOCMEM,
  INSTR_NEWROOT,
  INSTR_MOVECELLS,
  INSTR_ENQUEUEALLATOMS,
  INSTR_FREEMEM,
  INSTR_ADDMEM,
  INSTR_ENQUEUEMEM,
  INSTR_UNLOCKMEM,
  INSTR_SETMEMNAME,
  INSTR_GETLINK,
  INSTR_ALLOCLINK,
  INSTR_NEWLINK,
  INSTR_RELINK,
  INSTR_UNIFY,
  INSTR_INHERITLINK,
  INSTR_UNIFYLINKS,
  INSTR_REMOVEPROXIES,
  INSTR_REMOVETOPLEVELPROXIES,
  INSTR_INSERTPROXIES,
  INSTR_REMOVETEMPORARYPROXIES,
  INSTR_LOADRULESET,
  INSTR_COPYRULES,
  INSTR_CLEARRULES,
  INSTR_LOADMODULE,
  INSTR_RECURSIVELOCK,
  INSTR_RECURSIVEUNLOCK,
  INSTR_COPYCELLS,
  INSTR_DROPMEM,
  INSTR_LOOKUPLINK,
  INSTR_INSERTCONNECTORS,
  INSTR_INSERTCONNECTORSINNULL,
  INSTR_DELETECONNECTORS,
  INSTR_REACT,
  INSTR_JUMP,
  INSTR_COMMIT,
  INSTR_RESETVARS,
  INSTR_CHANGEVARS,
  INSTR_SPEC,
  INSTR_PROCEED,
  INSTR_STOP,
  INSTR_BRANCH,
  INSTR_LOOP,
  INSTR_RUN,
  INSTR_NOT,
  INSTR_INLINE,
  INSTR_CALLBACK,
  INSTR_BUILTIN,
  INSTR_GUARD_INLINE,
  INSTR_UNIQ,
  INSTR_NOT_UNIQ,
  INSTR_NEWHLINK,//seiji -->
  INSTR_MAKEHLINK,
  INSTR_ISHLINK,
  INSTR_GETNUM,
  INSTR_UNIFYHLINKS,
  INSTR_FINDPROCCXT,// --> seiji
  INSTR_EQGROUND,
  INSTR_NEQGROUND,
  INSTR_COPYGROUND,
  INSTR_REMOVEGROUND,
  INSTR_FREEGROUND,
  INSTR_ISGROUND,
  INSTR_ISUNARY,
  INSTR_ISUNARYFUNC,
  INSTR_ISINT,
  INSTR_ISFLOAT,
  INSTR_ISSTRING,
  INSTR_ISINTFUNC,
  INSTR_ISFLOATFUNC,
  INSTR_ISSTRINGFUNC,
  INSTR_GETCLASS,
  INSTR_GETCLASSFUNC,
  INSTR_GETRUNTIME,
  INSTR_CONNECTRUNTIME,
  INSTR_NEWSET,
  INSTR_ADDATOMTOSET,
  INSTR_NEWLIST,
  INSTR_ADDTOLIST,
  INSTR_GETFROMLIST,
  INSTR_IADD,
  INSTR_ISUB,
  INSTR_IMUL,
  INSTR_IDIV,
  INSTR_INEG,
  INSTR_IMOD,
  INSTR_INOT,
  INSTR_IAND,
  INSTR_IOR,
  INSTR_IXOR,
  INSTR_ISAL,
  INSTR_ISAR,
  INSTR_ISHR,
  INSTR_IADDFUNC,
  INSTR_ISUBFUNC,
  INSTR_IMULFUNC,
  INSTR_IDIVFUNC,
  INSTR_INEGFUNC,
  INSTR_IMODFUNC,
  INSTR_INOTFUNC,
  INSTR_IANDFUNC,
  INSTR_IORFUNC,
  INSTR_IXORFUNC,
  INSTR_ISALFUNC,
  INSTR_ISARFUNC,
  INSTR_ISHRFUNC,
  INSTR_ILT,
  INSTR_ILE,
  INSTR_IGT,
  INSTR_IGE,
  INSTR_IEQ,
  INSTR_INE,
  INSTR_ILTFUNC,
  INSTR_ILEFUNC,
  INSTR_IGTFUNC,
  INSTR_IGEFUNC,
  INSTR_FADD,
  INSTR_FSUB,
  INSTR_FMUL,
  INSTR_FDIV,
  INSTR_FNEG,
  INSTR_FADDFUNC,
  INSTR_FSUBFUNC,
  INSTR_FMULFUNC,
  INSTR_FDIVFUNC,
  INSTR_FNEGFUNC,
  INSTR_FLT,
  INSTR_FLE,
  INSTR_FGT,
  INSTR_FGE,
  INSTR_FEQ,
  INSTR_FNE,
  INSTR_FLTFUNC,
  INSTR_FLEFUNC,
  INSTR_FGTFUNC,
  INSTR_FGEFUNC,
  INSTR_FLOAT2INT,
  INSTR_INT2FLOAT,
  INSTR_FLOAT2INTFUNC,
  INSTR_INT2FLOATFUNC,
  INSTR_FINDATOM2,
  INSTR_ANYMEM2,
  INSTR_GROUP,
  INSTR_SYSTEMRULESETS,
  INSTR_SUBCLASS,
  INSTR_ISBUDDY,

  INSTR_PRINTINSTR
};


enum ArgType {
  ARG_END = 0,
  InstrVar = 1,
  Label,
  InstrVarList,
  String,
  LineNum,
  ArgFunctor,
  ArgRuleset,
  InstrList
};

struct InstrSpec {
  char *op_str;
  LmnInstrOp op;
  enum ArgType args[128];
};

extern struct InstrSpec spec[];

#endif
