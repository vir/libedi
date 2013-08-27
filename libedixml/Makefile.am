# @(#) $Id$

AM_CFLAGS = -I${top_srcdir}/include -I${top_builddir}/include -I@includedir@
#AM_LDFLAGS = -L@libdir@ -lexpat

lib_LTLIBRARIES = libedixml.la

libedixml_la_SOURCES = conv.c output.c
libedixml_la_LDFLAGS = -avoid-version
libedixml_la_LIBADD = ../libedi/libedi.la ../libedistruct/libedistruct.la


