#ifndef EDI2XML_H_INCLUDED
#define EDI2XML_H_INCLUDED

struct edi2xml_config {
	int comments_segment_names:1;
	int comments_coded_values:1;
	int comments_element_names:1;
	int comments_errors:1;
	int translate_coded_values:1;
	int translate_coded_to_elements:1;
};

extern struct edi2xml_config edi2xml_opts;

int edi2xml_conv(const char * edi_text);


#endif /* EDI2XML_H_INCLUDED */
