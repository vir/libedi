AM_CFLAGS = -I${top_srcdir}/include -I${top_builddir}/include $(DEPS_CFLAGS) -I@includedir@ -DDATADIR=\"$(datadir)/@PACKAGE@\"
AM_LDFLAGS = -L@libdir@

bin_PROGRAMS = edi2xml

edi2xml_SOURCES = edi2xml.c
edi2xml_LDADD = -lexpat -liconv ../libedi/libedi.la ../libedistruct/libedistruct.la ../libedixml/libedixml.la $(DEPS_LIBS)

