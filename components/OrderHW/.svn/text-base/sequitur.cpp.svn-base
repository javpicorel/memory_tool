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

#include "sequitur.hpp"

#include <unistd.h>
#include <sys/times.h>

#include <limits.h>
#include "sequitur_classes.hpp"


/******************************************************************************

 sequitur.cc - Module containing the main() function, and functions for
               printing out the grammar.

    main() function contains: command line options parsing,
    program initialization, reading the input...

 Compilation notes:
    Define PLATFORM_MSWIN macro if compiling this program under Windows,
    or PLATFORM_UNIX if compiling it under Unix/Linux.

 Program usage (syntax):
    See "help" string below (or run "sequitur -h" after compiling the program).

 *******************************************************************************/

namespace sequitur {

using namespace std;

rules **S;                // pointer to main rules of the grammar
int num_root_rules;
bool seen_first_symbol;
bool * seen_first_symbol_for_rule;

int num_rules = 0;        // number of rules in the grammar
int num_symbols = 0;      // number of symbols in the grammar
tAddress min_terminal,        // minimum and maximum value among terminal symbols
        max_terminal;         //
int max_rule_len = 2;     // maximum rule length


void calculate_rule_usage(rules *r);


/* program options (values of these are affected by command line options, see there) */
int reproduce = 0,
    print_rule_freq = 0,
    print_rule_usage = 0,
    delimiter = -1,
    K = 1;   // minimum number of times a digram must occur to form rule, decreased by one
             // (e.g. if K is 1, two occurrences are required to form rule)

char *delimiter_string = 0;


void uncompress(), print(), number(), forget(symbols *s), forget_print(symbols *s);
void start_compress(bool), end_compress(), stop_forgetting();
ofstream *rule_S = 0;

void initialize( int aNumStrings ) {
  num_root_rules = aNumStrings;
  S = new  rules * [num_root_rules];
  seen_first_symbol_for_rule = new bool [num_root_rules];
  for (int i = 0; i < num_root_rules; ++i) {
    S[i] = new rules;
    seen_first_symbol_for_rule[i] = false;
  }
  seen_first_symbol = false;
}

void addSymbol( tAddress aSymbol, int aString ) {
  if (! seen_first_symbol) {
    min_terminal = max_terminal = aSymbol;
  } else {
    if (aSymbol < min_terminal) min_terminal = aSymbol;
    else if (aSymbol > max_terminal) max_terminal = aSymbol;
  }

  if (! seen_first_symbol_for_rule[ aString ] ) {
    S[aString]->last()->insert_after(new symbols(aSymbol));
  } else {
    // append read character to end of rule S, and enforce constraints
    S[aString]->last()->insert_after(new symbols(aSymbol));
    S[aString]->last()->prev()->check();
  }
}

void printRules() {

  for(int i = 0; i < num_root_rules; ++i) {
    calculate_rule_usage(S[i]);
  }

  number();
  print();

}


rules **R1;
int Ri;

void print()
{
  for (int i = 0; i < Ri; i ++) {
    cout << i << " -> ";
    for (symbols *p = R1[i]->first(); !p->is_guard(); p = p->next())
      if (p->nt()) cout << p->rule()->index() << ' ';
      else cout << *p << ' ';
    if (i > num_root_rules /*&& print_rule_freq*/) cout << '\t' << R1[i]->freq();
    if (i > num_root_rules /*&& print_rule_usage*/) cout << "\t(" << R1[i]->usage() << ")";
    if (/*reproduce &&*/ i > num_root_rules) {
      cout << '\t';
      R1[i]->reproduce();
    }
    cout << endl;
  }

  if (/*print_rule_freq*/ true)
    cout << (num_symbols - Ri) << " symbols, " << Ri << " rules " <<
      (num_symbols * (sizeof(symbols) + 4) + Ri * sizeof(rules)) << " total space\n";

}

void number()
{
  R1 = (rules **) malloc(sizeof(rules *) * num_rules);
  memset(R1, 0, sizeof(rules *) * num_rules);
  for (int i = 0; i < num_root_rules; ++i) {
    R1[i] = S[i];
  }
  Ri = num_root_rules;

  for (int i = 0; i < Ri; i ++)
    for (symbols *p = R1[i]->first(); !p->is_guard(); p = p->next())
      if (p->nt() && R1[p->rule()->index()] != p->rule()) {
         p->rule()->index(Ri);
         R1[Ri ++] = p->rule();
      }
}

/*
void forget_print(symbols *s)
{
  // open file to write rule S to, if not already open
  if (rule_S == 0) rule_S = new ofstream("S");

  // symbol is non-terminal
  if (s->nt()) {

    // remember the rule the symbol heads and delete the symbol
    rules *r = s->rule();
    delete s;

    // there are no more instances of this symbol in the grammar (rule contents has already been printed out),
    // so we print out the symbol and delete the rule
    if (r->freq() == 0) {
      *rule_S << r->index();
      while (r->first()->next() != r->first()) delete r->first();   // delete all symbols in the rule
      delete r;                                                     // delete rule itself
    }
    // there are still instances of this symbol in the grammar
    else {
      if (r->index() == 0) r->output();    // print rule's right hand (to stdout) (this will change r->index())
      *rule_S << r->index();               // print rule index, i.e. non-terminal symbol, to file for rule S
    }

  }
  // symbol is terminal (print it and delete it)
  else {
    *rule_S << *s;
    delete s;
  }

  // put space between symbols
  *rule_S << ' ';
}
*/

void calculate_rule_usage(rules *r)
{
  for (symbols *p = r->first(); !p->is_guard(); p = p->next()) {
    if (p->nt()) {
      p->rule()->usage(1);
      calculate_rule_usage(p->rule());
    }
  }
}


} //end namespace sequitur

