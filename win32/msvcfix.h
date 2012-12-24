
#if !defined(_W64)
# if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#  define _W64 __w64
# else
#  define _W64
# endif
#endif

#ifdef _WIN64
typedef __int64         ssize_t;
#else
typedef _w64 int        ssize_t;
#endif

#define inline __inline

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
#endif

