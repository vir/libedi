# @(#) $Id$

PERL=@PERL@

AM_CFLAGS = -I${top_srcdir}/include -I${top_builddir}/include

lib_LTLIBRARIES = libedistruct.la

uncl/UNCL.99A: dl_specs.pl
	${PERL} dl_specs.pl
unsl/UNSL.99A: uncl/UNCL.99A
edsd/EDSD.99A: uncl/UNCL.99A
edcd/EDCD.99A: uncl/UNCL.99A
eded/EDED.99A: uncl/UNCL.99A
part4/D422.TXT: uncl/UNCL.99A

libedistruct_la_CPPFLAGS = -I${top_srcdir}/include -I${top_builddir}/include
libedistruct_la_SOURCES = coded.c  comps.c  elems.c  segs.c
libedistruct_la_LDFLAGS = -avoid-version

segs.c: conv_specs.pl uncl/UNCL.99A unsl/UNSL.99A edsd/EDSD.99A edcd/EDCD.99A eded/EDED.99A part4/D422.TXT
	${PERL} conv_specs.pl
elems.c: segs.c
comps.c: segs.c
coded.c: elems.c

noinst_PROGRAMS = test1

test1_SOURCES = test1.c
test1_LDADD = ../libedi/libedi.la libedistruct.la

