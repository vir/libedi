#ifndef EXCEP_H_INCLUDED
#define EXCEP_H_INCLUDED

#include <setjmp.h>

extern jmp_buf edixml_exception;
extern char * edixml_excp_text; ///< Malloc'ed exception text
void throw_exception(const char * msg, ...);

#endif /* EXCEP_H_INCLUDED */
