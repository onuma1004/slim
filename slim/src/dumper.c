/*
 * dumper.c - dump membrane
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
 * $Id: dumper.c,v 1.15 2008/09/19 18:03:22 taisuke Exp $
 */

#include <ctype.h>
#include "dumper.h"
#include "internal_hash.h"
#include "vector.h"
#include "rule.h"
#include "membrane.h"
#include "atom.h"
#include "symbol.h"
#include "functor.h"

#define MAX_DEPTH 1000
#define LINK_FORMAT "L%d"

struct AtomRec {
  BOOL done;
  SimpleHashtbl args;
  int link_num; /* 一連のプロキシに割り当てられた番号, proxy only */
};

struct DumpState {
  int link_num;
};

static BOOL dump_atom(LmnAtomPtr atom,
                      SimpleHashtbl *ht,
                      LmnLinkAttr attr,
                      struct DumpState *s,
                      int indent,
                      int call_depth);
static void lmn_dump_cell_internal(LmnMembrane *mem,
                                   SimpleHashtbl *ht,
                                   struct DumpState *s,
                                   int indent);

static struct AtomRec *atomrec_make()
{
  struct AtomRec *a = LMN_MALLOC(struct AtomRec);
  a->done = FALSE;
  hashtbl_init(&a->args, 16);
  a->link_num = -1;
  return a;
}

static void atomrec_free(struct AtomRec *a)
{
  hashtbl_destroy(&a->args);
  LMN_FREE(a);
}

static void dump_state_init(struct DumpState *s)
{
  s->link_num = 0;
}

static BOOL is_direct_printable(LmnFunctor f)
{
  const char *s;

  if (LMN_IS_PROXY_FUNCTOR(f) ||
      f == LMN_NIL_FUNCTOR) return TRUE;

  s = LMN_FUNCTOR_STR(f);
  if (!(isalpha(*s) && islower(*s))) return FALSE;
  while (*(++s)) {
    if (!(isalpha(*s) || isdigit(*s) || *s=='_')) return FALSE;
  }
  return TRUE;
}

/* htからatomに対応するAtomRecを取得。なければ追加してから返す */
static struct AtomRec *get_atomrec(SimpleHashtbl *ht, LmnAtomPtr atom)
{
  if (hashtbl_contains(ht, (HashKeyType)atom)) {
    return (struct AtomRec *)hashtbl_get(ht, (HashKeyType)atom);
  } else {
    struct AtomRec *t;
    t = atomrec_make();
    hashtbl_put(ht, (HashKeyType)atom, (HashValueType)t);
    return t;
  }
}

static void dump_atomname(LmnFunctor f)
{
  /* dump module name */
  if (LMN_FUNCTOR_MODULE_ID(f) != ANONYMOUS) {
    fprintf(stdout, "%s.", lmn_id_to_name(LMN_FUNCTOR_MODULE_ID(f)));
  }

  /* dump atom name */
  {
    const char *atom_name = lmn_id_to_name(LMN_FUNCTOR_NAME_ID(f));

    if (is_direct_printable(f)) {
      fprintf(stdout, "%s", atom_name);
    } else {
      fprintf(stdout, "'%s'", atom_name);
    }
  }
}

static void dump_link(LmnAtomPtr atom, int i, SimpleHashtbl *ht, struct DumpState *s)
{
  int link;
  struct AtomRec *t;

  t = get_atomrec(ht, atom);
  if (hashtbl_contains(&t->args, i)) {
    /* リンク名画決まっている */
    link = hashtbl_get(&t->args, i);
  } else {
    /* リンク名が決まっていないので新たに作る */
    link = s->link_num++;
    hashtbl_put(&t->args, i, link);
  }
  fprintf(stdout, LINK_FORMAT, link);
}

static BOOL dump_data_atom(LmnWord data,
                           LmnLinkAttr attr,
                           int indent)
{
  /* print only data (no link) */
  switch (attr) {
  case  LMN_INT_ATTR:
    fprintf(stdout, "%d", (int)data);
    break;
  case  LMN_DBL_ATTR:
    fprintf(stdout, "%f", *(double*)data);
    break;
  default:
    fprintf(stdout, "*[%d]", attr);
    LMN_ASSERT(FALSE);
    break;
  }
  return TRUE;
}

static BOOL dump_list(LmnAtomPtr atom,
                      SimpleHashtbl *ht,
                      struct DumpState *s,
                      int indent,
                      int call_depth)
{
  BOOL first = TRUE;
  LmnLinkAttr attr;

  if (get_atomrec(ht, atom)->done) {
    dump_link(atom, 2, ht, s);
    return TRUE;
  }

  attr = LMN_ATTR_MAKE_LINK(2); /* 2 is output link position */

  fprintf(stdout, "[");
  while (TRUE) {
    LmnFunctor f = LMN_ATOM_GET_FUNCTOR(atom);
    if (!LMN_ATTR_IS_DATA(attr) &&
        f == LMN_LIST_FUNCTOR   &&
        LMN_ATTR_GET_VALUE(attr) == 2) {
      struct AtomRec *rec;

      rec = get_atomrec(ht, atom);
      
      if (rec->done) { /* cyclic */
        int link = s->link_num++;
        fprintf(stdout, "|");
        hashtbl_put(&rec->args, LMN_ATTR_GET_VALUE(attr), link);
        fprintf(stdout, LINK_FORMAT, link);
        break;
      }
      rec->done = TRUE;

      if (!first) fprintf(stdout, ",");
      first = FALSE;

      if (hashtbl_contains(&rec->args, 0)) {
        /* link 0 was already printed */
        int link = hashtbl_get(&rec->args, 0);
        fprintf(stdout, LINK_FORMAT, link);
      }
      else {
        dump_atom(LMN_ATOM(LMN_ATOM_GET_LINK(atom, 0)),
                  ht,
                  LMN_ATOM_GET_ATTR(atom, 0),
                  s,
                  indent,
                  call_depth + 1);
      }
      attr = LMN_ATOM_GET_ATTR(atom, 1);
      atom = LMN_ATOM(LMN_ATOM_GET_LINK(atom, 1));
    }
    else if (!LMN_ATTR_IS_DATA(attr) &&
             f == LMN_NIL_FUNCTOR) {
      struct AtomRec *rec;
      rec = atomrec_make();
      rec->done = TRUE;
      hashtbl_put(ht, (HashKeyType)atom, (HashValueType)rec);
      break;
    }
    else { /* list ends with non nil data */
      fprintf(stdout, "|");
      dump_atom(atom, ht, LMN_ATTR_GET_VALUE(attr), s, indent, call_depth + 1);
      break;
    }
  }
  fprintf(stdout, "]");
  return TRUE;
}

/* propagate a link number to connected proxies */
static void propagate_proxy_link(LmnAtomPtr atom,
                                 LmnLinkAttr attr,
                                 SimpleHashtbl *ht,
                                 int link_num)
{
  struct AtomRec *t;
  int i;

  if (LMN_ATTR_IS_DATA(attr)) return;
  if (LMN_ATOM_GET_FUNCTOR(atom) != LMN_IN_PROXY_FUNCTOR &&
      LMN_ATOM_GET_FUNCTOR(atom) != LMN_OUT_PROXY_FUNCTOR) return;
  t = get_atomrec(ht, atom);
  if (t->link_num >= 0) return;
  
  t->link_num = link_num;
  for (i = 0; i < 2; i++) {
    propagate_proxy_link(LMN_ATOM(LMN_ATOM_GET_LINK(atom, i)),
                         LMN_ATOM_GET_ATTR(atom, i),
                         ht,
                         link_num);
  }
}

/* assign a link number to all connected proxies */
static void assign_link_to_proxy(LmnAtomPtr atom, SimpleHashtbl *ht, struct DumpState *s)
{
  struct AtomRec *t;

  t = get_atomrec(ht, atom);
  if (t->link_num < 0) {
    int link_num = s->link_num++;
    propagate_proxy_link(atom, LMN_ATTR_MAKE_LINK(0), ht, link_num);
  }
}
   
static BOOL dump_proxy(LmnAtomPtr atom,
                       SimpleHashtbl *ht,
                       int link_pos,
                       struct DumpState *s,
                       int indent,
                       int call_depth)
{
  struct AtomRec *t;
  t = get_atomrec(ht, atom);
  t->done = TRUE;

  if (call_depth == 0) {
    LmnLinkAttr attr = LMN_ATOM_GET_ATTR(atom, 1);
    if (LMN_ATTR_IS_DATA(attr)) {
      dump_data_atom((LmnWord)LMN_ATOM_GET_LINK(atom, 1), attr, indent);
      fprintf(stdout, "(" LINK_FORMAT ")", t->link_num);
    } else {
      /* symbol atom has dumped */
      return FALSE;
    }
  }
  else {
    fprintf(stdout, LINK_FORMAT, t->link_num);
  }
  return TRUE;
}

static BOOL dump_symbol_atom(LmnAtomPtr atom,
                             SimpleHashtbl *ht,
                             int link_pos,
                             struct DumpState *s,
                             int indent,
                             int call_depth)
{
  LmnFunctor f;
  LmnArity arity;
  int i;
  int limit;
  struct AtomRec *t;
  
  f = LMN_ATOM_GET_FUNCTOR(atom);
  arity = LMN_FUNCTOR_ARITY(f);
  if (LMN_IS_PROXY_FUNCTOR(f)) arity--;
  
  t = get_atomrec(ht, atom);

  if ((call_depth > 0 && link_pos != arity - 1) || /* not last link */
      (call_depth > 0 && t->done)               || /* already printed */
      call_depth > MAX_DEPTH) {                    /* limit overflow */
    dump_link(atom, link_pos, ht, s);
    return TRUE;
  }
  
  if (t->done) return FALSE;
  t->done = TRUE;

  dump_atomname(f);
  limit = arity;
  if (call_depth > 0) limit--;

  if (limit > 0) {
    fprintf(stdout, "(");
    for (i = 0; i < limit; i++) {
      if (i > 0) fprintf(stdout, ",");

      if (hashtbl_contains(&t->args, i)) {
        /* argument has link number */
        int link = hashtbl_get(&t->args, i);
        fprintf(stdout, LINK_FORMAT, link);
      }
      else {
        dump_atom((LmnAtomPtr)LMN_ATOM_GET_LINK(atom, i),
                  ht,
                  LMN_ATOM_GET_ATTR(atom, i),
                  s,
                  indent,
                  call_depth + 1);
      }
    }
    fprintf(stdout, ")");
  }

  return TRUE;
}

static BOOL dump_atom(LmnAtomPtr atom,
                      SimpleHashtbl *ht,
                      LmnLinkAttr attr,
                      struct DumpState *s,
                      int indent,
                      int call_depth)
{
  if (LMN_ATTR_IS_DATA(attr)) {
    return dump_data_atom((LmnWord)atom, attr, indent);
  }
  else {
    LmnFunctor f = LMN_ATOM_GET_FUNCTOR(atom);
    LmnLinkAttr link_pos = LMN_ATTR_GET_VALUE(attr);
    if (!lmn_env.show_proxy &&
        (f == LMN_IN_PROXY_FUNCTOR ||
         f == LMN_OUT_PROXY_FUNCTOR)) {
      return dump_proxy(atom, ht, attr, s, indent, call_depth);
    }
    else if (f == LMN_LIST_FUNCTOR &&
             link_pos == 2) {
      return dump_list(atom, ht, s, indent, call_depth);
    }
    else {
      return dump_symbol_atom(atom, ht, link_pos, s, indent, call_depth);
    }
  }
}

/* atom must be a symbol atom */
static BOOL dump_toplevel_atom(LmnAtomPtr atom,
                               SimpleHashtbl *ht,
                               struct DumpState *s,
                               int indent)
{
  if (!lmn_env.show_proxy &&
      (LMN_ATOM_GET_FUNCTOR(atom) == LMN_IN_PROXY_FUNCTOR ||
       LMN_ATOM_GET_FUNCTOR(atom) == LMN_OUT_PROXY_FUNCTOR)) {
    return dump_proxy(atom, ht, LMN_ATTR_MAKE_LINK(0), s, indent, 0);
  }
  else {
    return dump_symbol_atom(atom, ht, LMN_ATTR_MAKE_LINK(0), s, indent, 0);
  }
}


static void dump_ruleset(struct Vector *v, int indent)
{
  unsigned int i;

  for (i = 0; i < v->num; i++) {
    if (i > 0) fprintf(stdout, ",");
    fprintf(stdout, "@%d", lmn_ruleset_get_id((LmnRuleSet)vec_get(v, i)));
  }
}
                  
#define INDENT_INCR 2

static void lmn_dump_mem_internal(LmnMembrane *mem,
                                  SimpleHashtbl *ht,
                                  struct DumpState *s,
                                  int indent)
{
  if (mem->name != ANONYMOUS) {
    fprintf(stdout, "%s", lmn_id_to_name(mem->name));
  }
  fprintf(stdout, "{");
  lmn_dump_cell_internal(mem, ht, s, indent);
  fprintf(stdout, "}");
}

static void lmn_dump_cell_internal(LmnMembrane *mem,
                                  SimpleHashtbl *ht,
                                  struct DumpState *s,
                                  int indent)
{
  unsigned int i, j;
  enum {P0, P1, P2, P3, PROXY, PRI_NUM};
  Vector pred_atoms[PRI_NUM];
  HashIterator iter;
  BOOL printed;

  if (!mem) return;

  if (hashtbl_contains(ht, (HashKeyType)mem)) return;

  for (i = 0; i < PRI_NUM; i++) {
    vec_init(&pred_atoms[i], 16);
  }

  /* 優先順位に応じて起点となるアトムを振り分ける */

  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
    LmnFunctor f = hashtbliter_entry(&iter)->key;
    LmnAtomPtr atom;
    LMN_ASSERT(ent);
    for (atom = atomlist_head(ent);
         atom != lmn_atomlist_end(ent);
         atom = LMN_ATOM_GET_NEXT(atom)) {
      int arity = LMN_ATOM_GET_ARITY(atom);
      if(LMN_ATOM_GET_FUNCTOR(atom)==LMN_RESUME_FUNCTOR)
        continue;
      if (f == LMN_IN_PROXY_FUNCTOR ||
          f == LMN_OUT_PROXY_FUNCTOR) {
        vec_push(&pred_atoms[PROXY], (LmnWord)atom);
      }
      /* 0 argument atom */
      else if (arity == 0) { 
        vec_push(&pred_atoms[P0], (LmnWord)atom);
      }
      /* 1 argument, link to the last argument */
      else if (arity == 1 &&
               f != LMN_NIL_FUNCTOR &&
               (LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(atom, 0)) ||
                (int)LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(atom, 0)) ==
                LMN_ATOM_GET_ARITY(LMN_ATOM_GET_LINK(atom, 0)) - 1)) {
        vec_push(&pred_atoms[P1], (LmnWord)atom);
      }
      /* link to the last argument */
      else if (arity > 1 &&
               (LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(atom, arity-1)) ||
                (int)LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(atom, arity-1)) ==
                LMN_ATOM_GET_ARITY(LMN_ATOM_GET_LINK(atom, arity-1)) - 1)) {
        vec_push(&pred_atoms[P2], (LmnWord)atom);
      }
      else {
        vec_push(&pred_atoms[P3], (LmnWord)atom);
      }
    }
  }

  if (!lmn_env.show_proxy) {
    /* assign link to proxies */
    for (i = 0; i < pred_atoms[PROXY].num; i++) {
      assign_link_to_proxy(LMN_ATOM(vec_get(&pred_atoms[PROXY], i)), ht, s);
    }
  }

  printed = FALSE;
  { /* dump atoms */
    for (i = 0; i < PRI_NUM; i++) {
      for (j = 0; j < pred_atoms[i].num; j++) {
        LmnAtomPtr atom = LMN_ATOM(vec_get(&pred_atoms[i], j));
        if (dump_toplevel_atom(atom, ht, s, indent + INDENT_INCR)) {
          /* TODO アトムの出力の後には常に ". "が入ってしまう.
             アトムの間に ", "を挟んだ方が見栄えが良い */
          fprintf(stdout, ". ");
          printed = TRUE;
        }
      }
    }
  }
  for (i = 0; i < PRI_NUM; i++) {
    vec_destroy(&pred_atoms[i]);
  }

  { /* dump chidren */
    LmnMembrane *m;
    for (m = mem->child_head; m; m = m->next) {
      lmn_dump_mem_internal(m, ht, s, indent);
      if (m->next)
        fprintf(stdout, ", ");
    }
    if (mem->child_head) {
      /* 最後の膜の後に ". "を出力 */
      fprintf(stdout, ". ");
    }
  }

  if (lmn_env.show_ruleset) {
    dump_ruleset(&mem->rulesets, indent);
  }
}

static void lmn_dump_cell_nonewline(LmnMembrane *mem)
{
  SimpleHashtbl ht;
  struct DumpState s;

  dump_state_init(&s);

  hashtbl_init(&ht, 128);
  lmn_dump_cell_internal(mem, &ht, &s, 0);

  { /* hashtblの解放 */
    HashIterator iter;

    /* 開放処理. 今のところdataに0以外が入っていた場合
       struct AtomRecのポインタが格納されている */
    for (iter = hashtbl_iterator(&ht); !hashtbliter_isend(&iter); hashtbliter_next(&iter)) {
      if (hashtbliter_entry(&iter)->data) {
        atomrec_free((struct AtomRec *)hashtbliter_entry(&iter)->data);
      }
    }
    hashtbl_destroy(&ht);
  }
}

void lmn_dump_cell(LmnMembrane *mem)
{
  switch (lmn_env.output_format) {
  case DEFAULT:
    lmn_dump_cell_nonewline(mem);
    fprintf(stdout, "\n");
    break;
  case DOT:
    lmn_dump_dot(mem);
    break;
  case DEV:
    lmn_dump_mem_dev(mem);
    break;
  default:
    assert(FALSE);
    exit(EXIT_FAILURE);
  }
}

/* print membrane structure */
void lmn_dump_mem(LmnMembrane *mem)
{
  switch (lmn_env.output_format) {
  case DEFAULT:
    fprintf(stdout, "{");
    lmn_dump_cell_nonewline(mem);
    fprintf(stdout, "}\n");
    break;
  case DOT:
    lmn_dump_dot(mem);
    break;
  case DEV:
    lmn_dump_mem_dev(mem);
    break;
  default:
    assert(FALSE);
    exit(EXIT_FAILURE);
  }
}

static void dump_atom_dev(LmnAtomPtr atom)
{
  LmnFunctor f;
  LmnArity arity;
  unsigned int i;
  
  f = LMN_ATOM_GET_FUNCTOR(atom);
  arity = LMN_FUNCTOR_ARITY(f);
  fprintf(stdout, "Func[%u], Name[%s], A[%u], Addr[%p], ", f, lmn_id_to_name(LMN_FUNCTOR_NAME_ID(f)), arity, (void*)atom);

  for (i = 0; i < arity; i++) {
    LmnLinkAttr attr;

    fprintf(stdout, "%u: ", i);
    attr = LMN_ATOM_GET_ATTR(atom,i);
    if (i == 2 && LMN_IS_PROXY_FUNCTOR(f)) { /* membrane */
      fprintf(stdout, "mem[%p], ", (void*)LMN_PROXY_GET_MEM(atom));
    }
    else if (!LMN_ATTR_IS_DATA(attr)) { /* symbol atom */
      fprintf(stdout, "link[%d, %p], ", LMN_ATTR_GET_VALUE(attr), (void*)LMN_ATOM_GET_LINK(atom, i));
    } else {
      switch (attr) {
        case  LMN_INT_ATTR:
          fprintf(stdout, "int[%lu], ", LMN_ATOM_GET_LINK(atom,i));
          break;
        case  LMN_DBL_ATTR:
          fprintf(stdout, "double[%f], ", *(double*)LMN_ATOM_GET_LINK(atom,i));
          break;
        default:
          fprintf(stdout, "unknown data type[%d], ", attr);
          break;
      }
    }
  }

  fprintf(stdout, "\n");
}

static void dump_ruleset_dev(struct Vector *v)
{
  unsigned int i;
  fprintf(stdout, "ruleset[");
  for (i = 0;i < v->num; i++) {
     fprintf(stdout, "%d ", lmn_ruleset_get_id((LmnRuleSet)vec_get(v, i)));
  }
  fprintf(stdout, "]\n");
}
                  

void lmn_dump_mem_dev(LmnMembrane *mem)
{
  HashIterator iter;

  if (!mem) return;
  
  fprintf(stdout, "{\n");
  fprintf(stdout, "Mem[%u], Addr[%p]\n", LMN_MEM_NAME_ID(mem), (void*)mem);
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
    LmnAtomPtr atom;

    for (atom = atomlist_head(ent);
         atom != lmn_atomlist_end(ent);
         atom = LMN_ATOM_GET_NEXT(atom)) {
      dump_atom_dev(atom);
    }
  }

  dump_ruleset_dev(&mem->rulesets);
  lmn_dump_mem_dev(mem->child_head);
  lmn_dump_mem_dev(mem->next);
  
  fprintf(stdout, "}\n");
}


/*----------------------------------------------------------------------
 * dump dot
 */

static void dump_dot_cell(LmnMembrane *mem,
                          SimpleHashtbl *ht,
                          int *data_id,
                          int *cluster_id)
{
  unsigned int i;
  HashIterator iter;

  if (!mem) return;

  /* dump node labels */
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
    LmnAtomPtr atom;
    LMN_ASSERT(ent);
    for (atom = atomlist_head(ent);
         atom != lmn_atomlist_end(ent);
         atom = LMN_ATOM_GET_NEXT(atom)) {
      fprintf(stdout, "%lu [label = \"", (LmnWord)atom);
      dump_atomname(LMN_ATOM_GET_FUNCTOR(atom));
      fprintf(stdout, "\", shape = circle];\n");
      for (i = 0; i < LMN_FUNCTOR_GET_LINK_NUM(LMN_ATOM_GET_FUNCTOR(atom)); i++) {
        LmnLinkAttr attr = LMN_ATOM_GET_ATTR(atom, i);
        if (LMN_ATTR_IS_DATA(attr)) {
          fprintf(stdout, "%lu [label = \"", (LmnWord)LMN_ATOM_PLINK(atom, i));
          dump_data_atom(LMN_ATOM_GET_LINK(atom, i), attr, 0);
          fprintf(stdout, "\", shape = box];\n");
        }
      }
    }
  }

  /* dump connections */
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
/*     LmnFunctor f = hashtbliter_entry(&iter)->key; */
    LmnAtomPtr atom;
    LMN_ASSERT(ent);
    for (atom = atomlist_head(ent);
         atom != lmn_atomlist_end(ent);
         atom = LMN_ATOM_GET_NEXT(atom)) {
      struct AtomRec *ar = (struct AtomRec *)hashtbl_get_default(ht, (HashKeyType)atom, 0);
      unsigned int arity = LMN_FUNCTOR_GET_LINK_NUM(LMN_ATOM_GET_FUNCTOR(atom));

      
      for (i = 0; i < arity; i++) {
        LmnLinkAttr attr = LMN_ATOM_GET_ATTR(LMN_ATOM(atom), i);
        
        if (ar && hashtbl_contains(&ar->args, i)) continue;
        fprintf(stdout, "%lu -- ", (LmnWord)atom);
        if (LMN_ATTR_IS_DATA(attr)) {
          fprintf(stdout, " %lu", (LmnWord)LMN_ATOM_PLINK(atom, i));
          (*data_id)++;
        }
        else { /* symbol atom */
          struct AtomRec *ar;
          LmnWord atom2 = LMN_ATOM_GET_LINK(atom, i);
          if (hashtbl_contains(ht, atom2)) {
             ar = (struct AtomRec *)hashtbl_get(ht, atom2);
          } else {
            ar = atomrec_make();
            hashtbl_put(ht, (HashKeyType)atom2, (HashValueType)ar);
          }
          hashtbl_put(&ar->args, LMN_ATTR_GET_VALUE(attr), 1);
          fprintf(stdout, "%lu", atom2);
        }
        fprintf(stdout, "\n");
      }
    }
  }


  { /* dump chidren */
    LmnMembrane *m;
    for (m = mem->child_head; m; m = m->next) {
      fprintf(stdout, "subgraph cluster%d {\n", *cluster_id);
      (*cluster_id)++;
      dump_dot_cell(m, ht, data_id, cluster_id);
      fprintf(stdout, "}\n");
    }
  }
}

void lmn_dump_dot(LmnMembrane *mem)
{
  int cluster_id = 0, data_id = 0;
  struct DumpState s;
  SimpleHashtbl ht;

  dump_state_init(&s);
  hashtbl_init(&ht, 128);

  fprintf(stdout, "// This is an auto generated file by SLIM\n\n"
                  "graph {\n"
                  "node [bgcolor=\"trasnparent\",truecolor=true,color=\"#000000\",style=filled,fillcolor=\"#ffd49b50\"];\n"
                  "edge [color=\"#000080\"];\n"
          
          );

  dump_dot_cell(mem, &ht, &data_id, &cluster_id);
  
  fprintf(stdout, "}\n");

  {
    HashIterator iter;

    /* 開放処理. 今のところdataに0以外が入っていた場合
       struct AtomRecのポインタが格納されている */
    for (iter = hashtbl_iterator(&ht); !hashtbliter_isend(&iter); hashtbliter_next(&iter)) {
      if (hashtbliter_entry(&iter)->data) {
        atomrec_free((struct AtomRec *)hashtbliter_entry(&iter)->data);
      }
    }
    hashtbl_destroy(&ht);
  }
}
