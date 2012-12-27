#ifndef LIBEDISTRUCT_H_INCLUDED
#define LIBEDISTRUCT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct edistruct_segment edistruct_segment_t;
typedef struct edistruct_composite edistruct_composite_t;
typedef struct edistruct_element edistruct_element_t;
typedef struct edistruct_coded edistruct_coded_t;
typedef struct edistruct_all edistruct_all_t;

struct edistruct_segment
{
	const char * name;
	const char * title;
	const char * title2;
	const char * function;
	const char ** children;
};

struct edistruct_composite /* exact copy of segment */
{
	const char * name;
	const char * title;
	const char * title2;
	const char * function;
	const char ** children;
};

struct edistruct_element
{
	const char * name;
	const char * title;
	const char * title2;
	const char * function;
	const char * format;
	int is_coded;
};

struct edistruct_coded
{
	const char * name;
	const char * title;
	const char * title2;
	const char * function;
};

struct edistruct_coded_elements {
	const char * el;
	const edistruct_coded_t * vals;
	unsigned int nvals;
};

struct edistruct_all {
	unsigned int nsegs, nelems, ncomps, ncodes;
	edistruct_segment_t * segs;
	edistruct_element_t * elems;
	edistruct_composite_t * comps;
	struct edistruct_coded_elements * codes;
};

const struct edistruct_segment   * find_edistruct_segment(const char * name);
const struct edistruct_composite * find_edistruct_composite(const char * name);
const struct edistruct_element   * find_edistruct_element(const char * name);
const struct edistruct_coded     * find_coded_value(const char * elem, const char * name);
int edistruct_load_xml_files(const char * dir);
void edistruct_set_struct(edistruct_all_t* n);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* LIBEDISTRUCT_H_INCLUDED */
