#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

void output_init(char ** bufptr);
int output_set_enc(const char * e);
void output_shutdown();
void output(const char * format, ...);
char * escape_xml(const char * src);

#endif /* OUTPUT_H_INCLUDED */

