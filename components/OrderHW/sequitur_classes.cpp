// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 - 2008 by Eric Chung, Michael Ferdman, Brian Gold, Nikos   
// Hardavellas, Jangwoo Kim, Ippokratis Pandis, Minglong Shao, Jared Smolens,
// Stephen Somogyi, Evangelos Vlachos, Tom Wenisch, Anastassia Ailamaki,     
// Babak Falsafi and James C. Hoe for the SimFlex Project, Computer          
// Architecture Lab at Carnegie Mellon, Carnegie Mellon University.          
//                                                                           
// For more information, see the SimFlex project website at:                 
//   http://www.ece.cmu.edu/~simflex                                         
//                                                                           
// You may not use the name 'Carnegie Mellon University' or derivations      
// thereof to endorse or promote products derived from this software.        
//                                                                           
// If you modify the software you must place a notice on or within any       
// modified version provided or made available to any third party stating    
// that you have modified the software.  The notice shall include at least   
// your name, address, phone number, email address and the date and purpose  
// of the modification.                                                      
//                                                                           
// THE SOFTWARE IS PROVIDED 'AS-IS' WITHOUT ANY WARRANTY OF ANY KIND, EITHER 
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY  
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY 
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,  
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY 
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT, 
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN   
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY, 
// CONTRACT, TORT OR OTHERWISE).                                             
//                                     
// DO-NOT-REMOVE end-copyright-block   

/********************************************************************************

 classes.cc - Module containing (part of the) methods of 'rules' and 'symbols'
              classes, and functions for working with the hash table of digrams
              printing out the grammar.

 Notes:
    For the rest of 'symbols' and 'rules' methods, see classes.h .

 ********************************************************************************/

#include "sequitur_classes.hpp"
#include <ctype.h>

namespace sequitur {

extern int num_rules, delimiter;    // see sequitur.cc for explanation of these

rules::rules() {
  num_rules ++;
  guard = new symbols(this);
  guard->point_to_self();
  count = number = Usage = 0;
}

rules::~rules() {
  num_rules --;
  delete guard;
}

symbols *rules::first() { return guard->next(); }   // pointer to first symbol of rule's right hand
symbols *rules::last()  { return guard->prev(); }   // pointer to last symbol of rule's right hand

int symbols::check() {
  if (is_guard() || n->is_guard()) return 0;

  symbols **x = find_digram(this);
  if (!x) return 0;    // if either symbol of the digram is a delimiter -> do nothing

  int i;

  // if digram is not yet in the hash table -> put it there, and return
  for (i = 0; i < K; i ++)
    if (int(x[i]) <= 1) {
      x[i] = this;
      return 0;
    }

  // if repetitions overlap -> do nothing
  for (i = 0; i < K; i ++)
    if (x[i]->next() == this || next() == x[i])
      return 0;

  rules *r;

  // reuse an existing rule

  for (i = 0; i < K; i ++)
    if (x[i]->prev()->is_guard() && x[i]->next()->next()->is_guard()) {
      r = x[i]->prev()->rule();
      substitute(r);

      // check for an underused rule

      if (r->first()->nt() && r->first()->rule()->freq() == 1) r->first()->expand();
      // if (r->last()->nt() && r->last()->rule()->freq() == 1) r->last()->expand();

      return 1;
    }

  symbols *y[100];
  for (i = 0; i < K; i ++) y[i] = x[i];

  // make a copy of the pointers to digrams,
  // so that they don't change under our feet
  // especially when we create this replacement rule

  // create a new rule

  r = new rules;

  if (nt())
    r->last()->insert_after(new symbols(rule()));
  else
    r->last()->insert_after(new symbols(value()));

  if (next()->nt())
    r->last()->insert_after(new symbols(next()->rule()));
  else
    r->last()->insert_after(new symbols(next()->value()));

  for (i = 0; i < K; i ++) {
    if (y[i] == r->first()) continue;
    // check that this hasn't been deleted
    bool deleted = 1;
    for (int j = 0; j < K; j ++)
      if (y[i] == x[j]) {
        deleted = 0;
        break;
      }
    if (deleted) continue;

    y[i]->substitute(r);
    //    y[i] = (symbols *) 1; // should be x
  }

  x[0] = r->first();

  substitute(r);

  // check for an underused rule

  if (r->first()->nt() && r->first()->rule()->freq() == 1) r->first()->expand();
  //  if (r->last()->nt() && r->last()->rule()->freq() == 1) r->last()->expand();

  return 1;
}

void symbols::expand() {
  symbols *left = prev();
  symbols *right = next();
  symbols *f = rule()->first();
  symbols *l = rule()->last();

  //extern bool compression_initialized;
  if (! /*compression_initialized*/ false) {
    int i = 0;
    symbols *s;
    extern int max_rule_len;
    // first calculate length of this rule (the one we are adding symbols to)
    s = next();    // symbol 'this' should not be counted because it will be deleted
    do {
       if (!s->is_guard()) i++;
       s = s->next();
    } while(s != this);
    // then calculate length of what is to be added
    for (s = f; !s->is_guard(); s = s->next()) i++;
    if (i > max_rule_len) max_rule_len = i;
  }

  symbols **m = find_digram(this);
  if (!m) return;
  delete rule();

  for (int i = 0; i < K; i ++)
    if (m[i] == this) m[i] = (symbols *) 1;

  s = 0; // if we don't do this, deleting the symbol tries to deuse the rule!

  delete this;

  join(left, f);
  join(l, right);

  *find_digram(l) = l;
}

void symbols::substitute(rules *r)
{
  symbols *q = p;

  delete q->next();
  delete q->next();

  q->insert_after(new symbols(r));

  if (!q->check()) q->next()->check();
}




 #define PRIME 62265551


#define HASH(one, two) (((one) << 16 | (two)) % PRIME)
#define HASH2(one) (17 - ((one) % 17))

symbols **table = 0;

symbols **find_digram(symbols *s)
{
  if (!table) {
    table = (symbols **) malloc(PRIME * K * sizeof(symbols *));
    memset(table, 0, PRIME * K * sizeof(symbols *));
  }

  ulong one = s->raw_value();
  ulong two = s->next()->raw_value();

  if (one == delimiter || two == delimiter) return 0;

  int jump = HASH2(one) * K;
  int insert = -1;
  int i = HASH(one, two) * K;

  while (1) {
    symbols *m = table[i];
    if (!m) {
      if (insert == -1) insert = i;
      return &table[insert];
    }
    else if (int(m) == 1) insert = i;
    else if (m->raw_value() == one && m->next()->raw_value() == two) return &table[i];
    i = (i + jump) % PRIME;
  }
}

void rules::reproduce()
{
  // for each symbol of the rule, call symbols::reproduce()!
  for (symbols *p = first(); !p->is_guard(); p = p->next())
    p->reproduce();
}


ostream &operator << (ostream &o, symbols &s)
{
  extern int numbers;

  if (s.nt())
     o << s.rule()->index();
  //else if (numbers & do_uncompress) o << s.value() << endl;
  //else if (numbers) o << '&' << s.value();
  //else if (do_uncompress) o << char(s.value());
  else if (s.value() == '\n') o << "\\n";
  else if (s.value() == '\t') o << "\\t";
  else if (s.value() == ' ' ) o << '_';
  else if (s.value() == '\\' ||
       s.value() == '(' ||
       s.value() == ')' ||
       s.value() == '_' ||
       isdigit(s.value()))
    o << '\\' << char(s.value());
  else o << char(s.value());

  return o;
}

int current_rule = 1;

void rules::output()
{
  symbols *s;

  for (s = first(); !s->is_guard(); s = s->next())
     if (s->nt() && s->rule()->index() == 0)
        s->rule()->output();

  number = current_rule ++;

  for (s = first(); !s->is_guard(); s = s->next())
    cout << *s << ' ';

  extern int print_rule_usage;
  if (print_rule_usage) cout << "\t(" << Usage << ")";

  cout << endl;
}

} //End namespace sequitur

