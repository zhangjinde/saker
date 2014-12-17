#ifndef _STRING__H_
#define _STRING__H_

#include <string.h>

int xstrrtrim(char *str,const char *charlist);

void xstrltrim(char *str, const char *charlist);

void xstrremove_chars(register char *str, const char *charlist);

void xstrrtrim_spaces(char *c);

void xstrltrim_spaces(char *c);

void xstrlrtrim_spaces(char *c);

void xstrdos2unix(char *str);

int xstrncasecmp(const char *sl, const char *s2, size_t n);

int xstrcasecmp(const char *s1, const char *s2);

int xstrisdigit(const char *str) ;

int xstrdigest_convert(unsigned char *sour, int sourlen, char *dest, int destlen);

char* xstrreplace(const char *str, const char *sub_str1, const char *sub_str2);

char* xstrdup(const char *str);

char* xstrprintf(const char *fmt, ...);

int xstrtoint(const char *str, int slen);

int xstrmatchlen(const char *pattern, int patternLen,
                     const char *string, int stringLen, int nocase);
                     
int xstrmatch(const char *pattern, const char *string, int nocase);

#endif
