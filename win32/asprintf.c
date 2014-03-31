#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

int vasprintf( char **sptr, char *fmt, va_list argv )
{
	int wanted = _vsnprintf( *sptr = NULL, 0, fmt, argv );
	if( (wanted > 0) && ((*sptr = (char*)malloc( 1 + wanted )) != NULL) )
		return vsprintf( *sptr, fmt, argv );
	return wanted;
}

int asprintf( char **sptr, char *fmt, ... )
{
	int retval;
	va_list argv;
	va_start( argv, fmt );
	retval = vasprintf( sptr, fmt, argv );
	va_end( argv );
	return retval;
}
