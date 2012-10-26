
#if defined(__cplusplus)
extern "C" {
#endif

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


const struct edistruct_segment * find_edistruct_segment(const char * name);
const struct edistruct_composite * find_edistruct_composite(const char * name);

#if defined(__cplusplus)
} /* extern "C" */
#endif
