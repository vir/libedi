#include "libedistruct.h"
#include "expat.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable:4996) /* The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. */
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

char * strndup(const char * s, int len)
{
	char * r = malloc(len + 1);
	memcpy(r, s, len);
	r[len] = '\0';
	return r;
}




/* ============================== segments ============================== */

struct segments_parse_helper
{
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
	if(0 == strcmp(xpath, "/segments/segment"))
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
	if(0 == strcmp(xpath, "/segments/segment"))
	{
		++self->index;
	}
	else if(0 == strcmp(xpath, "/segments/segment/children"))
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
	edistruct_segment_t * cur = &self->segs[self->index];
	if(0 == strcmp(xpath, "/segments/segment/@code"))
		cur->name = strndup(s, len);
	else if(0 == strcmp(xpath, "/segments/segment/title1"))
		cur->title = strndup(s, len);
	else if(0 == strcmp(xpath, "/segments/segment/title2"))
		cur->title2 = strndup(s, len);
	else if(0 == strcmp(xpath, "/segments/segment/function"))
		cur->function = strndup(s, len);
	else if(0 == strcmp(xpath, "/segments/segment/children/child"))
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

static int load_segments(const char * fn, edistruct_segment_t ** out_segs, unsigned int * out_count)
{
	int r;
	struct segments_parse_helper self;
	memset(&self, 0, sizeof(self));
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

static edistruct_all_t global_struc;

int load_struct(const char * dir)
{
	int r;
	char * fn;
	fn = concat_path(dir, "segs.xml");
	r = load_segments(fn, &global_struc.segs, &global_struc.nsegs);
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	fn = concat_path(dir, "elems.xml");
	r = load_elements(fn, &global_struc.elems, &global_struc.nelems);
	if(r != 0)
		fprintf(stderr, "Error loading %s\n", fn);
	free(fn);

	return r;
}

const struct edistruct_segment * find_edistruct_segment(const char * name)
{
	size_t begin = 0;
	size_t end = global_struc.nsegs;
	edistruct_segment_t * segments = global_struc.segs;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, segments[cur].name);
		if(cmp == 0) return & segments[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}

const struct edistruct_element * find_edistruct_element(const char * name)
{
	size_t begin = 0;
	size_t end = global_struc.nelems;
	edistruct_element_t * elements = global_struc.elems;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, elements[cur].name);
		if(cmp == 0) return & elements[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}
