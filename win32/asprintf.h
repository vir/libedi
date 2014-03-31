#ifndef ASPRINTF_H_INCLUDED
#define ASPRINTF_H_INCLUDED

int vasprintf( char **sptr, char *fmt, va_list argv );
int asprintf( char **sptr, char *fmt, ... );

#endif /* ASPRINTF_H_INCLUDED */
