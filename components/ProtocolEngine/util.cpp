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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include <core/debug/debug.hpp>

#include "util.hpp"

namespace nProtocolEngine {


FILE *                 _fp_stdin;
FILE **                _fpp_stdin  = &stdin;

FILE *                 _fp_stdout;
FILE **                _fpp_stdout = &stdout;

FILE *                 _fp_stderr;
FILE **                _fpp_stderr = &stderr;

unsigned long          ___debugf_line = 0;
char *                 ___debugf_file = NULL;

static   unsigned char ___debug_trigger = 1;

volatile unsigned long ___debugf_stop_at_num = 0xffffffff;
static char            ___dbg_name[128];

void ___dummy_exit(int val);
void ___dummy_exit(int val) { return; }

void ___dummy_abort(void);
void ___dummy_abort(void) { abort(); }

void (* ___exit) (int val) = ___dummy_exit;
void (* ___abort) (void)   = ___dummy_abort;

unsigned int
GetLog2(unsigned long x)
{
    unsigned long i;
    unsigned int  r = 0;

    for (i=1; i < x; i *= 2)
        r++;

    if (i !=  x)
        ___fatal("Error: GetLog2(%lu) is undefined \n", x);

    return r;
}

char *
get_date_string (time_t time_num)        // Time to parse, or 0 for current time
{
    static char date_str[50];
    struct tm * timestr;

    if (!time_num) {
        time_num = time (NULL);
    }
    timestr = localtime (&time_num);
    strcpy (date_str, asctime (timestr));
    if (date_str[strlen (date_str)-1]=='\n')
        date_str[strlen (date_str)-1]='\0';

    return (date_str);
}

FILE *
___stdin_from_file(char *name)
{
    _fp_stdin = fopen(name, "r");
    if (_fp_stdin == NULL)
        ___fatal("___stdin_to_file::");
    _fpp_stdin = &_fp_stdin;
    return _fp_stdin;
}

FILE *
___stdout_to_file(char *name)
{
    _fp_stdout = fopen(name, "w+");
    if (_fp_stdout == NULL)
        ___fatal("___stdout_to_file::");
    _fpp_stdout = &_fp_stdout;
    return _fp_stdout;
}

FILE *
___stderr_to_file(char *name)
{
    _fp_stderr = fopen(name, "w+");
    if (_fp_stderr == NULL)
        ___fatal("___stderr_to_file::");
    _fpp_stderr = &_fp_stderr;
    return _fp_stderr;
}

void
___stdxxx_close()
{
    if (_fpp_stdin != &stdin)
        fclose(_fp_stdin);
    if (_fpp_stdout != &stdout)
        fclose(_fp_stdout);
    if (_fpp_stderr != &stderr)
        fclose(_fp_stderr);
}

static char *
___advance_string(char *strp)
{
    while (*strp)
        strp++;
    return strp;
}

void
___debug_init(int argc, char *argv[], _tExitFuncPtr _exit_fnp, _tAbortFuncPtr _abort_fnp)
{
    ASSERT_ALWAYS(strlen(argv[0]) < 128);
    strcpy(___dbg_name, argv[0]);

    assert(_exit_fnp);
    ___exit  = _exit_fnp;

    assert(_abort_fnp);
    ___abort = _abort_fnp;

    return;
}

void
___debug_switch_off(void)
{
    ___debug_trigger = 0;
}

void
___debug_switch_on(void)
{
    ___debug_trigger = 1;
}

unsigned char
___debug_is_trigger_on(void)
{
    return (___debug_trigger == 1);
}

void
___debugf(const char *fmt, ...)
{
    static unsigned long debugf_num = 0;

    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    sprintf(bufp, "%s.%05lu: ", ___dbg_name, debugf_num);
    bufp = ___advance_string(bufp);
#   if (IS_DEBUG_LEVEL(FILE_LINE_INFO_DBG))
        sprintf(bufp, "%s.%05lu: ", ___debugf_file, ___debugf_line);
        bufp = ___advance_string(bufp);
#   endif
    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    if (IS_DEBUG_LEVEL(DEBUGF_STOP_AT_DBG)) {
        if (debugf_num == ___debugf_stop_at_num)
        {
            volatile unsigned long ___loop___ = 0;
            ___warning("HERE!!!\n");
            while (___loop___ == 0) ;
        }
    }

    debugf_num++;

    return;
}

void
___printf(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stdout);
    fflush(*_fpp_stdout);

    return;
}


void
___fatal(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    if (errno == 0)
        sprintf(bufp, "\n%s FATAL ERROR:: ", ___dbg_name);
    else if (errno > sys_nerr)
        sprintf(bufp, "\n%s FATAL ERROR: %d: ", ___dbg_name, errno);
    else
        sprintf(bufp, "\n%s FATAL ERROR: %s (%d): ", ___dbg_name, sys_errlist[errno], errno);
    bufp = ___advance_string(bufp);
    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    assert(___exit);
    if (errno > 0 && errno < sys_nerr)
        (*___exit)(errno);
    else
        (*___exit)(-1);
}

void
___memfatal(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    if (errno == 0)
        sprintf(bufp, "\n%s FATAL ERROR:: ", ___dbg_name);
    else if (errno > sys_nerr)
        sprintf(bufp, "\n%s FATAL ERROR: %d: ", ___dbg_name, errno);
    else
        sprintf(bufp, "\n%s FATAL ERROR: %s (%d): ", ___dbg_name, sys_errlist[errno], errno);
    bufp = ___advance_string(bufp);
    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    assert(___exit);
    if (errno > 0 && errno < sys_nerr)
        (*___exit)(errno);
    else
        (*___exit)(12);
}

void
___assert_failed(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    sprintf(bufp, "\n%s ASSERT FAILED: ", ___dbg_name);
    bufp = ___advance_string(bufp);
    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    return;
}

void
___warning(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    if (errno == 0)
        sprintf(bufp, "\n%s WARNING:: ", ___dbg_name);
    else if (errno > sys_nerr)
        sprintf(bufp, "\n%s WARNING: %d: ", ___dbg_name, errno);
    else
        sprintf(bufp, "\n%s WARNING: %s (%d): ", ___dbg_name, sys_errlist[errno], errno);
    bufp = ___advance_string(bufp);

    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    return;
}

void
___error(const char *fmt, ...)
{
    char buffer[8192];
    char *bufp = buffer;
    va_list args;

    if (errno == 0)
        sprintf(bufp, "\n%s ERROR:: ", ___dbg_name);
    else if (errno > sys_nerr)
        sprintf(bufp, "\n%s ERROR: %d: ", ___dbg_name, errno);
    else
        sprintf(bufp, "\n%s ERROR: %s (%d): ", ___dbg_name, sys_errlist[errno], errno);
    bufp = ___advance_string(bufp);

    va_start(args, fmt);
    vsprintf(bufp, fmt, args);
    va_end(args);

    fflush(*_fpp_stdout);
    fflush(*_fpp_stderr);
    fputs(buffer, *_fpp_stderr);
    fflush(*_fpp_stderr);

    return;
}


}  // namespace nProtocolEngine
