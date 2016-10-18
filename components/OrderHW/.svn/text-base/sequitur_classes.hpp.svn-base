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


#ifndef SEQUITUR_CLASSES_INCLUDED
#define SEQUITUR_CLASSES_INCLUDED

#include "common.hpp"

namespace sequitur {

using namespace std;

extern int num_symbols, current_rule, K;       // defined in sequitur.cc, see there for explanation

class symbols;
class rules;
ostream &operator << (ostream &o, symbols &s);

typedef unsigned long ulong;

extern symbols **find_digram(symbols *s);     // defined in classes.cc


class rules {

  // the guard node in the linked list of symbols that make up the rule
  // It points forward to the first symbol in the rule, and backwards
  // to the last symbol in the rule. Its own value points to the rule data
  // structure, so that symbols can find out which rule they're in
  symbols *guard;

  // count keeps track of the number of times the rule is used in the grammar
  int count;

  // Usage stores the number of times a rule is used in the input.
  //    An example of the difference between count and Usage: in a grammar
  //    with rules S->abXcdXef , X->gAiAj , A->kl , rule A's count is 2
  //    (it is used two times in the grammar), while its Usage is 4 (there
  //    are two X's in the input sequence, and each of them uses A two times)
  int Usage;

  // number can serve two purposes:
  // (1) numbering the rules nicely for printing (in this case it's not essential for the algorithm)
  // (2) if this is a non-terminal symbol, assign a code to it (used in compression and forget_print())
  int number;

public:
  void output();     // output right hand of the rule, when printing out grammar
  void output2();    // output right hand of the rule, when compressing

  rules();
  ~rules();

  void reuse() { count ++; }
  void deuse() { count --; }

  symbols *first();     // pointer to first symbol of rule's right hand
  symbols *last();      // pointer to last symbol of rule's right hand

  int freq()           { return count; }
  int usage()          { return Usage; }
  void usage(int i)    { Usage += i; }
  int index()          { return number; }
  void index(int i)    { number = i; }

  void reproduce();    // reproduce full expansion of the rule
};

class symbols {
  symbols *n, *p;     // next and previous symbol within the rule
  ulong s;            // symbol value (e.g. ASCII code, or rule index)

public:

  // print out symbol, or, if it is non-terminal, rule's full expansion
  void reproduce() {
    if (nt()) rule()->reproduce();
    else cout << *this;
  }

  // initializes a new terminal symbol
  symbols(ulong sym) {
    s = sym * 2 + 1; // an odd number, so that they're a distinct
                     // space from the rule pointers, which are 4-byte aligned
    p = n = 0;
    num_symbols ++;
  }

  // initializes a new symbol to refer to a rule, and increments the reference
  // count of the corresponding rule
  symbols(rules *r) {
    s = (ulong) r;
    p = n = 0;
    rule()->reuse();
    num_symbols ++;
  }

  // links two symbols together, removing any old digram from the hash table
  static void join(symbols *left, symbols *right) {
    if (left->n) {
      left->delete_digram();

      // This is to deal with triples, where we only record the second
      // pair of the overlapping digrams. When we delete the second pair,
      // we insert the first pair into the hash table so that we don't
      // forget about it.  e.g. abbbabcbb

      if (right->p && right->n &&
          right->value() == right->p->value() &&
          right->value() == right->n->value()) {
        *find_digram(right) = right;
      }

      if (left->p && left->n &&
          left->value() == left->n->value() &&
          left->value() == left->p->value()) {
        *find_digram(left->p) = left->p;
      }
    }
    left->n = right; right->p = left;
  }

  // cleans up for symbol deletion: removes hash table entry and decrements
  // rule reference count
  ~symbols() {
    join(p, n);
    if (!is_guard()) {
      delete_digram();
      if (nt()) rule()->deuse();
    }
    num_symbols --;
  }

  // inserts a symbol after this one.
  void insert_after(symbols *y) {
    join(y, n);
    join(this, y);
  }

  // removes the digram from the hash table
  void delete_digram() {
    if (is_guard() || n->is_guard()) return;
    symbols **m = find_digram(this);
    if (m == 0) return;
    for (int i = 0; i < K; i ++) if (m[i] == this) m[i] = (symbols *) 1;
  }

  // is_guard() returns true if this is the guard node marking the beginning/end of a rule
  bool is_guard() { return nt() && rule()->first()->prev() == this; }

  // nt() returns true if a symbol is non-terminal.
  // We make sure that terminals have odd-numbered values.
  // Because the value of a non-terminal is a pointer to
  // the corresponding rule, they're guaranteed to be even
  // (more -- a multiple of 4) on modern architectures

  int nt() { return ((s % 2) == 0) && (s != 0);}

  symbols *next() { return n; }
  symbols *prev() { return p; }
  inline ulong raw_value() { return s; }
  inline ulong value() { return s / 2; }

  // assuming this is a non-terminal, rule() returns the corresponding rule
  rules *rule() { return (rules *) s; }

  void substitute(rules *r);        // substitute digram with non-terminal symbol

  int check();                      // check digram and enforce the Sequitur constraints if necessary

  void expand();                    // substitute non-terminal symbol with its rule's right hand

  void point_to_self() { join(this, this); }

};

} //End namespace sequitur

#endif //SEQUITUR_CLASSES_INCLUDED
