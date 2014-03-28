#ifndef LIBEDIXML_H_INCLUDED
#define LIBEDIXML_H_INCLUDED

struct edi2xml_config {
	int comments_segment_names:1;
	int comments_coded_values:1;
	int comments_element_names:1;
	int comments_errors:1;
	int translate_coded_values:1;
	int translate_coded_to_elements:1;
};

extern struct edi2xml_config edi2xml_opts;

#ifdef __cplusplus
extern "C" {
#endif

int edi2xml_conv(const char * edi_text);
char * edi2xml_conv2(const char * edi_text, const char * enc);
const char * edi2xml_error();

#ifdef __cplusplus
};
#endif

#endif /* LIBEDIXML_H_INCLUDED */
