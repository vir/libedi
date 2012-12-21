#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

void output_init(char ** bufptr);
int output_set_enc(const char * e);
void output_shutdown();
void output(const char * format, ...);
void error(const char * format, ...);
char * do_conv(const char * src);
char * escape_xml(const char * src);


#endif /* OUTPUT_H_INCLUDED */

