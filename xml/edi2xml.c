#ifdef _MSC_VER
# include "../win32/msvcfix.h"
# include "../win32/wingetopt.h"
#else
# include <unistd.h> /* for getopt */
#endif
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "libedi.h"
#include "libedistruct.h"
#include <iconv.h>
#include <stdarg.h>

static const char * namespaces = "xmlns:X=\"http://www.ctm.ru/edi/xchg\""
	" xmlns:S=\"http://www.ctm.ru/edi/segs\" xmlns:C=\"http://www.ctm.ru/edi/comps\""
	" xmlns:E=\"http://www.ctm.ru/edi/els\" xmlns:Z=\"http://www.ctm.ru/edi/codes\"";

iconv_t conv;

struct {
	int comments_segment_names:1;
	int comments_coded_values:1;
	int comments_element_names:1;
	int comments_errors:1;
	int translate_coded_values:1;
	int translate_coded_to_elements:1;
} opts;

void error(const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	if(opts.comments_errors)
	{
		printf("<!-- ERROR: ");
		vprintf(format, ap);
		printf(" -->\n");
	}
	va_end(ap);
}

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

char * do_conv(const char * src)
{
	char * dst, * dstbuf;
	size_t srclen, dstlen, r;
	size_t srcleft, dstleft;
	srclen = strlen(src);
	dstlen = 2 * srclen;
	dstbuf = (char *)malloc(dstlen + 1);

	dst = dstbuf;
	srcleft = srclen;
	dstleft = dstlen;
	while(dstlen < 64 * 1024 * 1024) /* sanity limit 64Mb - just in case */
	{
		size_t dstpos;
		r = iconv(conv, (char**)&src, &srcleft, &dst, &dstleft);
		if(r != (size_t)-1 && !srcleft)
		{
			*dst = '\0';
			return dstbuf;
		}
		dstpos = dst - dstbuf;
		dstlen += 100;
		dstbuf = realloc(dstbuf, dstlen);
		dstleft += 100;
		dst += dstpos;
	}
	free(dstbuf);
	return NULL;
}

char * escape_xml(const char * src)
{
	char * r;
	size_t pos;
	char * converted;
	size_t len;
	if(conv != (iconv_t)-1)
		src = converted = do_conv(src);
	else
		converted = NULL;
	len = strlen(src) + 1;
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
	if(converted)
		free(converted);
	return r;
}

char * coded_to_xml(const char * name, const char * val)
{
	size_t len;
	const edistruct_coded_t * z;
	char * text;

	z = find_coded_value(name, val);
	if(! z)
		return NULL;

	if(opts.comments_coded_values)
		printf("<!-- %s: %s, %s -->", val, z->title, z->function);

	if(! opts.translate_coded_values)
		return NULL;

	if(opts.translate_coded_to_elements)
	{
		len = strlen(z->title2) + 8;
		text = malloc(len);
		sprintf(text, "<Z:%s />", z->title2);
	}
	else
	{
		char * tit = escape_xml(z->title);
		len = strlen(tit) + strlen(val) + 28;
		text = malloc(len);
		sprintf(text, "<X:coded code=\"%s\">%s</X:coded>", val, tit);
		free(tit);
	}
	return text;
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
		error("unknown element %s", name);
		return;
	}
	printf("<E:%s>", e->title2);
	text = NULL;
	if(e->is_coded)
		text = coded_to_xml(name, val);

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
		error("unknown segment %s", seg->tag);
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

	if(data_element_index < seg->nelements)
		error("extra data at segment %s tail", seg->tag);

	printf("</S:%s>\n", sstruct->title2);
}

int proc_edi(const char * edi_text)
{
	edi_parser_t * p;
	edi_interchange_t * ichg;
	size_t i;

	p = edi_parser_create(NULL);
	ichg = edi_parser_parse(p, edi_text);

	if(conv != (iconv_t)-1)
		puts("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

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

void help()
{
	puts("\nUsage: edi2xml [opts] edi_file.txt > file.xml");
	puts("Opts:");
	puts("\t-h : this help");
	puts("\t-c [s][e][c] : turn on xml comments for segment names, element names and coded values respectively.");
	puts("\t-d : translate coded values");
	puts("\t-e : translate coded values into empty elements (implies -d)");
	puts("\t-x CHARSET : decode strings from given charset");
}

int main(int argc, char * argv[])
{
	char ch;
	int r;
	char * edi;

	memset(&opts, 0, sizeof(opts));
	conv = (iconv_t)-1;
	opts.comments_errors = 1;

	while((ch = getopt(argc, argv, "?hc:dex:")) != -1) {
		switch (ch) {
			case 'c':
				while(*optarg)
				{
					switch(*optarg)
					{
					case 's': opts.comments_segment_names = 1; break;
					case 'e': opts.comments_element_names = 1; break;
					case 'c': opts.comments_coded_values = 1; break;
					default: fprintf(stderr, "Unknown comment option '%c'\n", *optarg); break;
					}
					++optarg;
				}
				break;
			case 'e':
				opts.translate_coded_to_elements = 1;
			case 'd':
				opts.translate_coded_values = 1;
				break;
			case 'x':
				conv = iconv_open("UTF-8", optarg);
				if(conv == (iconv_t)-1)
					perror("iconv_open");
				break;
			case 'h':
			case '?':
			default:
				help();
				return 0;
		}
	}
	argc -= optind;
	argv += optind;

	if(argc != 1)
	{
		help();
		return 0;
	}

	edi = load_whole_file(argv[0]);
	if(! edi)
		return 1;

	r = proc_edi(edi);

	free(edi);
	if(conv != (iconv_t)-1)
		iconv_close(conv);
	return r;
}

