#include "libedistruct.h"
#include <string.h>

static edistruct_all_t* curstruc = NULL;

void edistruct_set_struct(edistruct_all_t* n)
{
	curstruc = n;
}

const struct edistruct_segment * find_edistruct_segment(const char * name)
{
	size_t begin = 0;
	size_t end = curstruc->nsegs;
	edistruct_segment_t * segments = curstruc->segs;
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
	size_t end = curstruc->nelems;
	edistruct_element_t * elements = curstruc->elems;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, elements[cur].name);
		if(cmp == 0) return & elements[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}

const struct edistruct_composite * find_edistruct_composite(const char * name)
{
	size_t begin = 0;
	size_t end = curstruc->ncomps;
	edistruct_composite_t * composites = curstruc->comps;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, composites[cur].name);
		if(cmp == 0) return & composites[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}

static const struct edistruct_coded * find_actual_value(const struct edistruct_coded_elements * tab, const char * name)
{
	size_t begin = 0;
	size_t end = tab->nvals;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, tab->vals[cur].name);
		if(cmp == 0) return &tab->vals[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}

const struct edistruct_coded * find_coded_value(const char * elem, const char * name)
{
	size_t begin = 0;
	size_t end = curstruc->ncodes;
	struct edistruct_coded_elements * coded_values_table = curstruc->codes;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(elem, coded_values_table[cur].el);
		if(cmp == 0) return find_actual_value(&coded_values_table[cur], name);
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}
