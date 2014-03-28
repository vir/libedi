#ifndef EXCEP_H_INCLUDED
#define EXCEP_H_INCLUDED

#include <setjmp.h>

extern jmp_buf edixml_exception;
extern const char * edixml_excp_text;
void throw_exception(const char * t);

#endif /* EXCEP_H_INCLUDED */
