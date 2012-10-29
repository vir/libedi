#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "libedi.h"
#include "edistruct.h"

static const char * namespaces = "xmlns:X=\"http://www.ctm.ru/edi/xchg\""
	" xmlns:S=\"http://www.ctm.ru/edi/segs\" xmlns:C=\"http://www.ctm.ru/edi/comps\""
	" xmlns:E=\"http://www.ctm.ru/edi/els\" xmlns:Z=\"http://www.ctm.ru/edi/codes\"";

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
	e = find_edistruct_element(name);
	if(e->is_coded)
	{
		const edistruct_coded_t * z;
		z = find_coded_value(name, val);
		if(z)
		{
			char * func = 0;
			size_t len;
			len = strlen(z->title2) + 8;
			func = escape_xml(z->function);
			if(func)
				len += 8 + strlen(func);
			text = malloc(len);
			if(func)
			{
				sprintf(text, "<Z:%s /><!-- %s -->", z->title2, func);
				free(func);
			}
			else
				sprintf(text, "<Z:%s />", z->title2);
		}
		else
			text = escape_xml(val);
	}
	else
		text = escape_xml(val);
	if(e)
		printf("<E:%s>%s</E:%s>\n", e->title2, text, e->title2);
	else
		printf("<E:e%s>%s</E:e%s>\n", name, text, name);
	free(text);
}

void element_missing_xml(const char * name)
{
#if 0
	const edistruct_element_t * e = find_edistruct_element(name);
	if(e)
		printf("<!-- [%s: %s] -->\n", name, e->title);
	else
		printf("<!-- [%s] -->\n", name);
#endif
}

void segment_to_xml(edi_segment_t * seg)
{
	size_t data_element_index;
	size_t i, data_child_index;
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

	printf("<S:%s><!-- %s -->\n", sstruct->title2, sstruct->name);

	/* check all possible chldren */
	for(i = 0; sstruct->children[i]; ++i)
	{
		const char * s;
		int is_optional, is_composite;
		s = sstruct->children[i];
		is_optional = s[0] == 'C';
		is_composite = s[1] == 'C' || s[1] == 'S';

		if(data_element_index >= seg->nelements)
		{
			if(is_optional)
				continue;
			else
			{
				printf("<!-- ERROR: mandatory element %s is missing -->", s + 1);
				break;
			}
		}

		if(seg->elements[data_element_index].type != (is_composite ? 'C' : 'S'))
		{
			if(is_optional)
			{
				element_missing_xml(s+1);
				continue;
			}
			else
			{
				printf("<!-- ERROR: mandatory element %s is missing -->", s + 1);
				break;
			}
		}

		if(is_composite)
		{
			size_t j;
			edi_element_t * el;
			const edistruct_composite_t * cstruct;

			cstruct = find_edistruct_composite(s + 1);
			el = &seg->elements[data_element_index];

			printf("<C:%s><!-- %s -->\n", cstruct->title2, s);
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
			element_to_xml(s + 1, seg->elements[data_element_index].simple.value);
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
	edi = load_whole_file(argv[1]);
	if(! edi)
		return 1;

	r = proc_edi(edi);

	free(edi);
	return r;
}

