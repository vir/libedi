#include "edistruct.h"
#include <stdio.h>

enum check_type { Segment, Composite };

int test()
{
	size_t i;
	int errors_count;
	struct {
		enum check_type what;
		const char * name;
		const char * title;
	} tab[] = {
		{ Segment, "ADR", "ADDRESS" },
		{ Segment, "MSG", "MESSAGE TYPE IDENTIFICATION" },
		{ Composite, "C236", "DANGEROUS GOODS LABEL" },
		{ Composite, "C961", "FORMULA COMPLEXITY" },
	};
	for(i = 0, errors_count = 0; i < sizeof(tab)/sizeof(tab[0]); ++i)
	{
		const char * found = NULL;
		switch(tab[i].what)
		{
			case Segment: {
				const struct edistruct_segment * s;
				s = find_edistruct_segment(tab[i].name);
				found = s->title;
				} break;
			case Composite: {
				const struct edistruct_composite * c;
				c = find_edistruct_composite(tab[i].name);
				found = c->title;
				} break;
			default:
				puts("Something weird happened");
				return -1;
		}
		if(0 != strcmp(tab[i].title, found))
		{
			printf(" %s: <<%s>> != <<%s>>\n", tab[i].name, found, tab[i].title);
			++errors_count;
		}
	}
	puts(errors_count ? "FAILED" : "PASSED");
	return errors_count;
}


int main(int argc, char * argv[])
{
	const struct edistruct_segment * s;
	char * n;
	if(argc != 2)
		return test();
	n = argv[1];
	s = find_edistruct_segment(n);
	printf("Found struct %p\n", s);
	if(s)
		printf("%s (%s)\n%s\n", s->title, s->title2, s->function);
	return 0;
}


