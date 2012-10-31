
AM_CPPFLAGS = -I${top_srcdir}/include -I${top_builddir}/include $(DEPS_CFLAGS)

bin_PROGRAMS = edi2xml

edi2xml_SOURCES = edi2xml.c

edi2xml_LDADD = ../libedi/libedi.la ../libedistruct/libedistruct.la $(DEPS_LIBS)

