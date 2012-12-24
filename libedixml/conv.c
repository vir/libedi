#include "libedixml.h"
#include "output.h"
#ifdef _MSC_VER
# include <STDDEF.H>
#endif
#include <malloc.h>
#include <string.h>
#include "libedi.h"
#include "libedistruct.h"
#include <stdio.h>
#include <stdarg.h>

static const char * namespaces = "xmlns:X=\"http://www.ctm.ru/edi/xchg\""
	" xmlns:S=\"http://www.ctm.ru/edi/segs\" xmlns:C=\"http://www.ctm.ru/edi/comps\""
	" xmlns:E=\"http://www.ctm.ru/edi/els\" xmlns:Z=\"http://www.ctm.ru/edi/codes\"";

struct edi2xml_config edi2xml_opts;

void error(const char * format, ...)
{
	int len;
	va_list ap;
	va_start(ap, format);
	len = vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	if(edi2xml_opts.comments_errors)
	{
		char * buf = (char *)malloc(len + 1);
		output("<!-- ERROR: ");
		vsprintf(buf, format, ap);
		output("%s", buf);
		output(" -->\n");
		free(buf);
	}
	va_end(ap);
}

static char * coded_to_xml(const char * name, const char * val)
{
	size_t len;
	const edistruct_coded_t * z;
	char * text;

	z = find_coded_value(name, val);
	if(! z)
		return NULL;

	if(edi2xml_opts.comments_coded_values)
		output("<!-- %s: %s, %s -->", val, z->title, z->function);

	if(! edi2xml_opts.translate_coded_values)
		return NULL;

	if(edi2xml_opts.translate_coded_to_elements)
	{
		len = strlen(z->title2) + 8;
		text = malloc(len);
		strcpy(text, "<Z:");
		strcat(text, z->title2);
		strcat(text, " />");
	}
	else
	{
		char * tit = escape_xml(z->title);
		len = strlen(tit) + strlen(val) + 28;
		text = malloc(len);
		strcpy(text, "<X:coded code=\"");
		strcat(text, val);
		strcat(text, "\">");
		strcat(text, tit);
		strcat(text, "</X:coded>");
		free(tit);
	}
	return text;
}

static void element_to_xml(const char * name, const char * val)
{
	char * text;
	const edistruct_element_t * e;
	if(edi2xml_opts.comments_element_names)
		output("<!-- %s -->\n", name);
	e = find_edistruct_element(name);
	if(! e)
	{
		error("unknown element %s", name);
		return;
	}
	output("<E:%s>", e->title2);
	text = NULL;
	if(e->is_coded)
		text = coded_to_xml(name, val);

	if(! text)
		text = escape_xml(val);

	output("%s</E:%s>\n", text, e->title2);
	free(text);
}

static void element_missing_xml(const char * name)
{
	const edistruct_element_t * e;
	if(! edi2xml_opts.comments_element_names)
		return;
	e = find_edistruct_element(name);
	if(e)
	{
		char * t = escape_xml(e->title);
		output("<!-- [no %s: %s] -->\n", name, t);
		free(t);
	}
	else
		output("<!-- [no %s] -->\n", name);
}

static int can_be_truncated(const edistruct_composite_t * c)
{
	size_t i;
	for(i = 1; c->children[i]; i++)
		if(c->children[i][0] == 'M')
			return 0;
	return 1;
}

static void segment_to_xml(edi_segment_t * seg)
{
	size_t data_element_index;
	size_t i;
	const edistruct_segment_t * sstruct;

	/* load structure desctiption */
	sstruct = find_edistruct_segment(seg->tag);
	if(! sstruct) /* fallback */
	{
		error("unknown segment %s", seg->tag);
		return;
	}

	/* get first data element index */
	data_element_index = 0;
	if(seg->elements[data_element_index].type == 'S' && 0 == strcmp(seg->elements[data_element_index].simple.value, seg->tag))
		++data_element_index;

	if(edi2xml_opts.comments_segment_names)
		output("<!-- %s -->\n", sstruct->name);
	output("<S:%s>\n", sstruct->title2);

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
				error("mandatory element %s is missing", s);
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
					++data_element_index;
					continue;
				}
				else
				{
					error("mandatory element %s is missing", s);
					break;
				}
			}
		}

		if(is_composite)
		{
			size_t j;
			edi_element_t * el;

			el = &seg->elements[data_element_index];

			if(edi2xml_opts.comments_element_names)
				output("<!-- %s -->", s);
			output("<C:%s>\n", cstruct->title2);
			for(j = 0; cstruct->children[j]; ++j)
			{
				if(j < el->composite.nvalues && el->composite.valuelens[j])
					element_to_xml(cstruct->children[j] + 1, el->composite.values[j]);
				else
					element_missing_xml(cstruct->children[j] + 1);
			}
			output("</C:%s>\n", cstruct->title2);
		}
		else
		{
			element_to_xml(s, seg->elements[data_element_index].simple.value);
		}
		++data_element_index;
	}

	if(data_element_index < seg->nelements)
		error("extra data (%d element%s) at segment %s tail", seg->nelements - data_element_index, seg->nelements - data_element_index > 1 ? "s" : "", seg->tag);

	output("</S:%s>\n", sstruct->title2);
}

static void parse_message_header(edi_segment_t * seg)
{
	struct {
		const char * msg_type;
		const char * msg_ver;
		const char * msg_release;
		const char * msg_agency;
	} h;
	edi_element_t* msgid;
	if(seg->nelements != 3 || seg->elements[2].type != 'C' || seg->elements[2].composite.nvalues < 4)
	{
		error("Can not parse message header format, %d elements", seg->nelements);
		return;
	}
	msgid = &seg->elements[2];
	h.msg_type    = msgid->composite.values[0];
	h.msg_ver     = msgid->composite.values[1];
	h.msg_release = msgid->composite.values[2];
	h.msg_agency  = msgid->composite.values[3];
}

int edi2xml_conv(const char * edi_text)
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
		if(0 == strcmp(tag, "UNB")) { output("<X:interchange %s>\n", namespaces); }
		if(0 == strcmp(tag, "UNG")) { output("<X:group>\n"); }
		if(0 == strcmp(tag, "UNH"))
		{
			parse_message_header(seg);
			output("<X:message>\n");
		}
		segment_to_xml(seg);
		if(0 == strcmp(tag, "UNZ")) { output("</X:interchange>\n"); }
		if(0 == strcmp(tag, "UNE")) { output("</X:group>\n"); }
		if(0 == strcmp(tag, "UNT")) { output("</X:message>\n"); }
	}

	edi_interchange_destroy(ichg);
	edi_parser_destroy(p);

	return 0;
}


