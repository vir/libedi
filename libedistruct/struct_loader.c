#include "libedistruct.h"
#include "expat.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable:4996) /* The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. */
#endif

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

//#define open _open

#define PARSE_BUFFER_SIZE 8192
#define XPATH_MAX 512

typedef void (XMLCALL *XML_CharacterDataHandlerEx) (void *userData, const char * xpath, const XML_Char *s, int len);
struct parser_helper {
	char path[XPATH_MAX];
	XML_StartElementHandler start;
	XML_EndElementHandler end;
	XML_CharacterDataHandlerEx cdata;
	void * ud;
};

static void XMLCALL startElement(void *userData, const char *name, const char **attrs)
{
	int i;
	struct parser_helper* self = (struct parser_helper*)userData;
	strncat(self->path, "/", sizeof(self->path));
	strncat(self->path, name, sizeof(self->path));
	if(self->start)
		self->start(self->ud, self->path, attrs);
	if(self->cdata)
	{
		for(i = 0; attrs[i]; i+=2) {
			char p[XPATH_MAX];
			strcpy(p, self->path);
			strncat(p, "/@", sizeof(p));
			strncat(p, attrs[i], sizeof(p));
			self->cdata(self->ud, p, attrs[i+1], strlen(attrs[i+1]));
		}
	}
}

static void XMLCALL endElement(void *userData, const char *name)
{
	struct parser_helper* self = (struct parser_helper*)userData;
	int plen = strlen(self->path);
	int nlen = strlen(name);
#ifdef _DEBUG
	if(0 != strcmp(self->path + (plen - nlen), name))
		fprintf(stderr, "Invalid closing tag %s, should be %s\n", name, self->path + (plen - nlen));
#endif
	if(self->end)
		self->end(self->ud, self->path);
	self->path[plen - nlen - 1] = '\0';
}

static void XMLCALL characterData(void *userData, const XML_Char *s, int len)
{
	struct parser_helper* self = (struct parser_helper*)userData;
	if(self->cdata)
		self->cdata(self->ud, self->path, s, len);
}


static int parse_xml_file(const char * fn, XML_StartElementHandler start, XML_EndElementHandler end, XML_CharacterDataHandlerEx cdata, void * ud)
{
	int fd, r;
	char buf[PARSE_BUFFER_SIZE];
	struct parser_helper self;
	XML_Parser parser;
	int done;

	memset(&self, 0, sizeof(self));
	self.start = start;
	self.end = end;
	self.cdata = cdata;
	self.ud = ud;

	parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &self);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, characterData);

	fd = open(fn, O_RDONLY|O_BINARY);
	if(fd < 0)
		return -1;

	do {
		r = read(fd, buf, sizeof(buf));
		done = r < sizeof(buf);
		if(XML_Parse(parser, buf, r, done) == XML_STATUS_ERROR) {
			fprintf(stderr, "%s at line %u\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser));
			return 1;
		}
	} while(! done);
	XML_ParserFree(parser);
	return 0;
}

#ifndef HAVE_STRNDUP
char * strndup(const char * s, int len)
{
	char * r = malloc(len + 1);
	memcpy(r, s, len);
	r[len] = '\0';
	return r;
}
#endif




/* ============================== segments ============================== */

struct segments_parse_helper
{
	const char * basepath;
	edistruct_segment_t * segs;
	unsigned int index;
	unsigned int alloc;
	char ** children;
	unsigned int chld_index;
	unsigned int chld_alloc;
};
static void XMLCALL segments_tstart(void *userData, const char * xpath, const char **attrs)
{
	struct segments_parse_helper * self = (struct segments_parse_helper *)userData;
	if(0 == strcmp(xpath, self->basepath))
	{
		if(self->index == self->alloc)
		{
			self->alloc += 25;
			self->segs = realloc(self->segs, self->alloc * sizeof(*self->segs));
			memset(&self->segs[self->index], 0, 10 * sizeof(*self->segs));
		}
	}
}
static void XMLCALL segments_tend(void *userData, const char * xpath)
{
	struct segments_parse_helper * self = (struct segments_parse_helper *)userData;
	const char * p = xpath + strlen(self->basepath);
	if(0 == strcmp(xpath, self->basepath))
	{
		++self->index;
	}
	else if(0 == strncmp(xpath, self->basepath, strlen(self->basepath)) && 0 == strcmp(p, "/children"))
	{
		if(self->chld_index == self->chld_alloc)
		{
			self->chld_alloc += 8;
			self->children = realloc(self->children, self->chld_alloc * sizeof(*self->children));
			self->segs[self->index].children = self->children;
		}
		self->children[self->chld_index++] = NULL;
		self->children = NULL;
		self->chld_alloc = 0;
		self->chld_index = 0;
	}
}
static void XMLCALL segments_cdata(void *userData, const char * xpath, const XML_Char *s, int len)
{
	struct segments_parse_helper * self = (struct segments_parse_helper *)userData;
	edistruct_segment_t * cur;
	const char * p = xpath + strlen(self->basepath);
	if(0 != strncmp(xpath, self->basepath, strlen(self->basepath)))
		return;
	cur = &(self->segs[self->index]);
	if(0 == strcmp(p, "/@code"))
		cur->name = strndup(s, len);
	else if(0 == strcmp(p, "/title1"))
		cur->title = strndup(s, len);
	else if(0 == strcmp(p, "/title2"))
		cur->title2 = strndup(s, len);
	else if(0 == strcmp(p, "/function"))
		cur->function = strndup(s, len);
	else if(0 == strcmp(p, "/children/child"))
	{
		if(self->chld_index == self->chld_alloc)
		{
			self->chld_alloc += 8;
			self->children = realloc(self->children, self->chld_alloc * sizeof(*self->children));
			cur->children = self->children;
		}
		self->children[self->chld_index++] = strndup(s, len);
	}
}

static int load_segments(const char * fn, edistruct_segment_t ** out_segs, unsigned int * out_count, const char * basepath)
{
	int r;
	struct segments_parse_helper self;
	memset(&self, 0, sizeof(self));
	self.basepath = basepath;
	r = parse_xml_file(fn, segments_tstart, segments_tend, segments_cdata, &self);
	if(r == 0)
	{
		*out_count = self.index;
		*out_segs = self.segs;
	}
	return r;
}

/* ============================== elements ============================== */

struct elements_parse_helper
{
	edistruct_element_t * elems;
	unsigned int index;
	unsigned int alloc;
};
static void XMLCALL elements_tstart(void *userData, const char * xpath, const char **attrs)
{
	struct elements_parse_helper * self = (struct elements_parse_helper *)userData;
	if(0 == strcmp(xpath, "/elements/element"))
	{
		if(self->index == self->alloc)
		{
			self->alloc += 25;
			self->elems = realloc(self->elems, self->alloc * sizeof(*self->elems));
			memset(&self->elems[self->index], 0, 10 * sizeof(*self->elems));
		}
	}
}
static void XMLCALL elements_tend(void *userData, const char * xpath)
{
	struct elements_parse_helper * self = (struct elements_parse_helper *)userData;
	if(0 == strcmp(xpath, "/elements/element"))
		++self->index;
}
static void XMLCALL elements_cdata(void *userData, const char * xpath, const XML_Char *s, int len)
{
	struct elements_parse_helper * self = (struct elements_parse_helper *)userData;
	edistruct_element_t * cur = &self->elems[self->index];
	if(0 == strcmp(xpath, "/elements/element/@code"))
		cur->name = strndup(s, len);
	else if(0 == strcmp(xpath, "/elements/element/title1"))
		cur->title = strndup(s, len);
	else if(0 == strcmp(xpath, "/elements/element/title2"))
		cur->title2 = strndup(s, len);
	else if(0 == strcmp(xpath, "/elements/element/function"))
		cur->function = strndup(s, len);
	else if(0 == strcmp(xpath, "/elements/element/format"))
		cur->format = strndup(s, len);
	else if(0 == strcmp(xpath, "/elements/element/is_coded"))
		cur->is_coded = (*s == '1') ? 1 : 0;
}

static int load_elements(const char * fn, edistruct_element_t ** out_elems, unsigned int * out_count)
{
	int r;
	struct elements_parse_helper self;
	memset(&self, 0, sizeof(self));
	r = parse_xml_file(fn, elements_tstart, elements_tend, elements_cdata, &self);
	if(r == 0)
	{
		*out_count = self.index;
		*out_elems = self.elems;
	}
	return r;
}


/* ============================== codes ============================== */

struct codes_parse_helper
{
	struct edistruct_coded_elements * cv;
	unsigned int cv_index, cv_alloc;
	edistruct_coded_t * cc;
	unsigned int cc_index, cc_alloc;
};
static void XMLCALL codes_tstart(void *userData, const char * xpath, const char **attrs)
{
	struct codes_parse_helper * self = (struct codes_parse_helper *)userData;
	if(0 == strcmp(xpath, "/coded_values/coded_element"))
	{
		if(self->cv_index == self->cv_alloc)
		{
			self->cv_alloc += 10;
			self->cv = realloc(self->cv, self->cv_alloc * sizeof(*self->cv));
		}
	}
	else if(0 == strcmp(xpath, "/coded_values/coded_element/value"))
	{
		if(self->cc_index == self->cc_alloc)
		{
			self->cc_alloc += 10;
			self->cc = realloc(self->cc, self->cc_alloc * sizeof(*self->cc));
			self->cv[self->cv_index].vals = self->cc;
		}
	}
}
static void XMLCALL codes_tend(void *userData, const char * xpath)
{
	struct codes_parse_helper * self = (struct codes_parse_helper *)userData;
	if(0 == strcmp(xpath, "/coded_values/coded_element"))
	{
		self->cv[self->cv_index].vals = self->cc;
		self->cv[self->cv_index].nvals = self->cc_index;
		++self->cv_index;
		self->cc = NULL;
		self->cc_index = self->cc_alloc = 0;
	}
	else if(0 == strcmp(xpath, "/coded_values/coded_element/value"))
		++self->cc_index;
}
static void XMLCALL codes_cdata(void *userData, const char * xpath, const XML_Char *s, int len)
{
	const char * p;
	struct codes_parse_helper * self = (struct codes_parse_helper *)userData;
	edistruct_coded_t * cur = &self->cc[self->cc_index];

	if(0 == strcmp(xpath, "/coded_values/coded_element/@code"))
		self->cv[self->cv_index].el = strndup(s, len);
	else if(0 != strncmp(xpath, "/coded_values/coded_element/value", 33))
		return;

	p = xpath + 33;
	if(0 == strcmp(p, "/@code"))
		cur->name = strndup(s, len);
	else if(0 == strcmp(p, "/title1"))
		cur->title = strndup(s, len);
	else if(0 == strcmp(p, "/title2"))
		cur->title2 = strndup(s, len);
	else if(0 == strcmp(p, "/function"))
		cur->function = strndup(s, len);
}

static int load_codes(const char * fn, struct edistruct_coded_elements ** out_elems, unsigned int * out_count)
{
	int r;
	struct codes_parse_helper self;
	memset(&self, 0, sizeof(self));
	r = parse_xml_file(fn, codes_tstart, codes_tend, codes_cdata, &self);
	if(r == 0)
	{
		*out_count = self.cv_index;
		*out_elems = self.cv;
	}
	return r;
}





/* ================================================================= */

static char * concat_path(const char * dir, const char * fn)
{
	unsigned int len;
	char * buf;
	len = strlen(dir);
	buf = (char*)malloc(len + 15);
	strcpy(buf, dir);
	buf[len] = '/';
	strcpy(buf + len + 1, fn);
	return buf;
}

static edistruct_all_t loaded_structs;

int edistruct_load_xml_files(const char * dir)
{
	int r;
	char * fn;
	fn = concat_path(dir, "segs.xml");
	r = load_segments(fn, &loaded_structs.segs, &loaded_structs.nsegs, "/segments/segment");
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	fn = concat_path(dir, "comps.xml");
	r = load_segments(fn, (edistruct_segment_t**)&loaded_structs.comps, &loaded_structs.ncomps, "/composites/composite");
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	fn = concat_path(dir, "elems.xml");
	r = load_elements(fn, &loaded_structs.elems, &loaded_structs.nelems);
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	fn = concat_path(dir, "coded.xml");
	r = load_codes(fn, &loaded_structs.codes, &loaded_structs.ncodes);
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	edistruct_set_struct(&loaded_structs);

	return r;
}

/* ================================================================= */

