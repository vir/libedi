#include "excep.h"

jmp_buf edixml_exception;
const char * edixml_excp_text = 0;

void throw_exception(const char * t)
{
	edixml_excp_text = t;
	longjmp(edixml_exception, -1);
}

