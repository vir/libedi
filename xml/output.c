#include "edi2xml.h"
#include "output.h"
#include <stdarg.h>
#include <stdio.h>
#include <iconv.h>
#include <malloc.h>
#include <string.h>

iconv_t conv;
char ** outbufptr = NULL;
unsigned int outbufsize, outbufpos;

void output_init(char ** bufptr)
{
	conv = (iconv_t)-1;
	outbufptr = bufptr;
	outbufsize = outbufpos = 0;
}

int output_set_enc(const char * e)
{
	conv = iconv_open("UTF-8", e);
	if(conv == (iconv_t)-1)
	{
		perror("iconv_open");
		return -1;
	}
	output("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	return 0;
}

void output_shutdown()
{
	if(conv != (iconv_t)-1)
		iconv_close(conv);
}

void output(const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	if(outbufptr)
	{
		int len = 0;
		do {
			if(outbufpos == outbufsize || len < 0)
			{
				outbufsize += 1024;
				*outbufptr = (char*)realloc(*outbufptr, outbufsize);
			}
			len = vsnprintf(*outbufptr + outbufpos, outbufsize - outbufpos - 1, format, ap);
		} while(len < 0);
		outbufpos += len;
	}
	else
		vfprintf(stdout, format, ap);
	va_end(ap);
}

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

