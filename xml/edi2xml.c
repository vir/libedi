#ifdef _MSC_VER
# include "../win32/msvcfix.h"
#endif
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "libedi.h"
#include "edistruct.h"

static const char * namespaces = "xmlns:X=\"http://www.ctm.ru/edi/xchg\""
	" xmlns:S=\"http://www.ctm.ru/edi/segs\" xmlns:C=\"http://www.ctm.ru/edi/comps\""
	" xmlns:E=\"http://www.ctm.ru/edi/els\" xmlns:Z=\"http://www.ctm.ru/edi/codes\"";

struct {
	int comments_segment_names:1;
	int comments_coded_values:1;
	int comments_element_names:1;
	int translate_coded_values:1;
} opts;

char * load_whole_file(const char * fname)
{
	long len;
	char * buf;
	FILE* f = fopen(fname, "rt");
	if(! f)
	{
		perror("fopen");
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc(len + 1);
	if(! buf)
	{
		perror("malloc");
		fclose(f);
		return NULL;
	}
	len = fread(buf, 1, len, f);
	buf[len] = '\0';
	fclose(f);
	return buf;
}

char * escape_xml(const char * src)
{
	char * r;
	size_t pos;
	size_t len = strlen(src) + 1;
	r = (char *)malloc(len);
	for(pos = 0; *src; ++src)
	{
		switch(*src)
		{
		case '<':
			len += 4;
			r = (char *)realloc(r, len);
			r[pos++] = '&';
			r[pos++] = 'l';
			r[pos++] = 't';
			r[pos++] = ';';
			break;
		case '&':
			len += 5;
			r = (char *)realloc(r, len);
			r[pos++] = '&';
			r[pos++] = 'a';
			r[pos++] = 'm';
			r[pos++] = 'p';
			r[pos++] = ';';
			break;
		default:
			r[pos++] = *src;
			break;
		}
	}
	r[pos] = '\0';
	return r;
}

void element_to_xml(const char * name, const char * val)
{
	char * text;
	const edistruct_element_t * e;
	if(opts.comments_element_names)
		printf("<!-- %s -->\n", name);
	e = find_edistruct_element(name);
	if(! e)
	{
		printf("<!-- ERROR: unknown element %s -->\n", name);
		return;
	}
	printf("<E:%s>", e->title2);
	text = NULL;
	if(e->is_coded)
	{
		const edistruct_coded_t * z;
		z = find_coded_value(name, val);
		if(z)
		{
			size_t len;

			if(opts.comments_coded_values)
				printf("<!-- %s: %s, %s -->", val, z->title, z->function);

			if(opts.translate_coded_values)
			{
				len = strlen(z->title2) + 8;
				text = malloc(len);
				sprintf(text, "<Z:%s />", z->title2);
			}
		}
	}

	if(! text)
		text = escape_xml(val);

	printf("%s</E:%s>\n", text, e->title2);
	free(text);
}

void element_missing_xml(const char * name)
{
	const edistruct_element_t * e;
	if(! opts.comments_element_names)
		return;
	e = find_edistruct_element(name);
	if(e)
	{
		char * t = escape_xml(e->title);
		printf("<!-- [no %s: %s] -->\n", name, t);
		free(t);
	}
	else
		printf("<!-- [no %s] -->\n", name);
}

static int can_be_truncated(const edistruct_composite_t * c)
{
	size_t i;
	for(i = 1; c->children[i]; i++)
		if(c->children[i][0] == 'M')
			return 0;
	return 1;
}

void segment_to_xml(edi_segment_t * seg)
{
	size_t data_element_index;
	size_t i;
	const edistruct_segment_t * sstruct;

	/* load structure desctiption */
	sstruct = find_edistruct_segment(seg->tag);
	if(! sstruct) /* fallback */
	{
		printf("<!-- ERROR: Unknown segment %s -->", seg->tag);
		return;
	}

	/* get first data element index */
	data_element_index = 0;
	if(seg->elements[data_element_index].type == 'S' && 0 == strcmp(seg->elements[data_element_index].simple.value, seg->tag))
		++data_element_index;

	if(opts.comments_segment_names)
		printf("<!-- %s -->\n", sstruct->name);
	printf("<S:%s>\n", sstruct->title2);

	/* check all possible chldren */
	for(i = 0; sstruct->children[i]; ++i)
	{
		const char * s;
		const edistruct_composite_t * cstruct;
		int is_optional, is_composite;
		s = sstruct->children[i];
		is_optional = s[0] == 'C';
		++s;
		is_composite = *s == 'C' || *s == 'S';
		if(is_composite)
			cstruct = find_edistruct_composite(s);
		else
			cstruct = NULL;


		if(data_element_index >= seg->nelements)
		{
			if(is_optional)
				continue;
			else
			{
				printf("<!-- ERROR: mandatory element %s is missing -->", s);
				break;
			}
		}

		if(seg->elements[data_element_index].type != (is_composite ? 'C' : 'S'))
		{
			/*
			 * Composite can be truncated so libedi parses it as simple element.
			 * Here we try to fix this unfairness by converting element to proper type
			 */
			if(is_composite && can_be_truncated(cstruct))
			{
				char ** vp;
				size_t * lp;
				vp = malloc(sizeof(char*) * 2);
				lp = malloc(sizeof(size_t) * 2);
				memset(vp, 0, sizeof(char*) * 2);
				memset(lp, 0, sizeof(size_t) * 2);
				*vp = seg->elements[data_element_index].simple.value;
				*lp = seg->elements[data_element_index].simple.valuelen;
				seg->elements[data_element_index].type = 'C';
				seg->elements[data_element_index].composite.nvalues = 1;
				seg->elements[data_element_index].composite.values = vp;
				seg->elements[data_element_index].composite.valuelens = lp;
			}
			else
			{
				if(is_optional)
				{
					element_missing_xml(s);
					continue;
				}
				else
				{
					printf("<!-- ERROR: mandatory element %s is missing -->", s);
					break;
				}
			}
		}

		if(is_composite)
		{
			size_t j;
			edi_element_t * el;

			el = &seg->elements[data_element_index];

			if(opts.comments_element_names)
				printf("<!-- %s -->", s);
			printf("<C:%s>\n", cstruct->title2);
			for(j = 0; cstruct->children[j]; ++j)
			{
				if(j < el->composite.nvalues && el->composite.valuelens[j])
					element_to_xml(cstruct->children[j] + 1, el->composite.values[j]);
				else
					element_missing_xml(cstruct->children[j] + 1);
			}
			printf("</C:%s>\n", cstruct->title2);
		}
		else
		{
			element_to_xml(s, seg->elements[data_element_index].simple.value);
		}
		++data_element_index;
	}

	printf("</S:%s>\n", sstruct->title2);
}

int proc_edi(const char * edi_text)
{
	edi_parser_t * p;
	edi_interchange_t * ichg;
	size_t i;

	p = edi_parser_create(NULL);
	ichg = edi_parser_parse(p, edi_text);

	for(i = 0; i < ichg->nsegments; ++i)
	{
		edi_segment_t * seg = &ichg->segments[i];
		const char * tag = seg->tag;
		if(0 == strcmp(tag, "UNB")) { printf("<X:interchange %s>", namespaces); }
		if(0 == strcmp(tag, "UNG")) { puts("<X:group>"); }
		if(0 == strcmp(tag, "UNH")) { puts("<X:message>"); }
		segment_to_xml(seg);
		if(0 == strcmp(tag, "UNZ")) { puts("</X:interchange>"); }
		if(0 == strcmp(tag, "UNE")) { puts("</X:group>"); }
		if(0 == strcmp(tag, "UNT")) { puts("</X:message>"); }
	}

	edi_interchange_destroy(ichg);
	edi_parser_destroy(p);

	return 0;
}

int main(int argc, char * argv[])
{
	int r;
	char * edi;
	if(argc < 2)
		return 0;

	memset(&opts, 0xFF, sizeof(opts));
	opts.comments_element_names = 0;
	opts.translate_coded_values = 0;

	edi = load_whole_file(argv[1]);
	if(! edi)
		return 1;

	r = proc_edi(edi);

	free(edi);
	return r;
}

