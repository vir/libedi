# @(#) $Id$

PERL=@PERL@

AM_CFLAGS = -I${top_srcdir}/include -I${top_builddir}/include -I@includedir@ -DDATADIR=\"$(datadir)\"
AM_LDFLAGS = -L@libdir@

lib_LTLIBRARIES = libedistruct.la

libedistructdir = $(datadir)/@PACKAGE@
libedistruct_DATA = coded.xml comps.xml elems.xml segs.xml

uncl/UNCL.99A: dl_specs.pl
	${PERL} dl_specs.pl
unsl/UNSL.99A: uncl/UNCL.99A
edsd/EDSD.99A: uncl/UNCL.99A
edcd/EDCD.99A: uncl/UNCL.99A
eded/EDED.99A: uncl/UNCL.99A
part4/D422.TXT: uncl/UNCL.99A

libedistruct_la_SOURCES = struct_loader.c lookup.c
libedistruct_la_LDFLAGS = -avoid-version

segs.xml: conv_specs.pl uncl/UNCL.99A unsl/UNSL.99A edsd/EDSD.99A edcd/EDCD.99A eded/EDED.99A part4/D422.TXT
	${PERL} conv_specs.pl
elems.xml: segs.xml
comps.xml: segs.xml
coded.xml: elems.xml

noinst_PROGRAMS = test1

test1_SOURCES = test1.c
test1_LDADD = -lexpat ../libedi/libedi.la libedistruct.la

