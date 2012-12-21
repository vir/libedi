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
#include "edi2xml.h"
#include "output.h"

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



void help()
{
	puts("\nUsage: edi2xml [edi2xml_opts. edi_file.txt > file.xml");
	puts("edi2xml_opts.");
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

	memset(&edi2xml_opts, 0, sizeof(edi2xml_opts));
	edi2xml_opts.comments_errors = 1;

	output_init(NULL);

	load_struct("C:/Projects/libedi/libedistruct");

	while((ch = getopt(argc, argv, "?hc:dex:")) != -1) {
		switch (ch) {
			case 'c':
				while(*optarg)
				{
					switch(*optarg)
					{
					case 's': edi2xml_opts.comments_segment_names = 1; break;
					case 'e': edi2xml_opts.comments_element_names = 1; break;
					case 'c': edi2xml_opts.comments_coded_values = 1; break;
					default: fprintf(stderr, "Unknown comment option '%c'\n", *optarg); break;
					}
					++optarg;
				}
				break;
			case 'e':
				edi2xml_opts.translate_coded_to_elements = 1;
			case 'd':
				edi2xml_opts.translate_coded_values = 1;
				break;
			case 'x':
				output_set_enc(optarg);
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

	r = edi2xml_conv(edi);

	free(edi);
	output_shutdown();
	return r;
}

