#include "excep.h"
#include <stdarg.h>
#include <assert.h>

jmp_buf edixml_exception;
char * edixml_excp_text = 0;

void throw_exception(const char * msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	assert(!edixml_excp_text);
	vasprintf(&edixml_excp_text, msg, ap);
	va_end(ap);
	longjmp(edixml_exception, -1);
}

