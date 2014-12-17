#include "xstring.h"
#include "common/common.h"
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <strings.h>

int xstrrtrim(char *str,const char *charlist) {
    char    *p;
    int count = 0;

    if (NULL == str || '\0' == *str)
        return count;

    for (p = str + strlen(str) - 1; p >= str && NULL != strchr(charlist, *p); p--) {
        *p = '\0';
        count++;
    }

    return count;
}

void xstrltrim(char *str, const char *charlist) {
    char    *p;

    if (NULL == str || '\0' == *str)
        return;

    for (p = str; '\0' != *p && NULL != strchr(charlist, *p); p++)
        ;

    if (p == str)
        return;

    while ('\0' != *p)
        *str++ = *p++;

    *str = '\0';
}

void xstrdos2unix(char *str) {
    char    *o = str;

    while ('\0' != *str) {
        if ('\r' == str[0] && '\n' == str[1])   /* CR+LF (Windows) */
            str++;
        *o++ = *str++;
    }
    *o = '\0';
}

void xstrremove_chars(register char *str, const char *charlist) {
    register char *p;

    if (NULL == str || NULL == charlist || '\0' == *str || '\0' == *charlist)
        return;

    for (p = str; '\0' != *p; p++) {
        if (NULL == strchr(charlist, *p))
            *str++ = *p;
    }

    *str = '\0';
}

void xstrrtrim_spaces(char *c) {
    int i,len;

    len = (int)strlen(c);
    for (i=len-1; i>=0; i--) {
        if ( c[i] == ' ') {
            c[i]=0;
        } else  break;
    }
}

void xstrltrim_spaces(char *c) {
    int i;
    /* Number of left spaces */
    int spaces=0;

    for (i=0; c[i]!=0; i++) {
        if ( c[i] == ' ') {
            spaces++;
        } else  break;
    }
    for (i=0; c[i+spaces]!=0; i++) {
        c[i]=c[i+spaces];
    }

    c[strlen(c)-spaces]=0;
}

void xstrlrtrim_spaces(char *c) {
    xstrltrim_spaces(c);
    xstrrtrim_spaces(c);
}

int   xstrncasecmp(const char *s1,const char *s2,size_t n) {
    return strncasecmp(s1,s2,n);
}


int xstrcasecmp(const char *s1,const char *s2) {
    int slen = strlen(s1) - strlen(s2);
    if(slen == 0) {
        return xstrncasecmp(s1,s2,strlen(s1));
    }
    return slen;
}

int xstrisdigit(const char *str) {
    if (!str) return UGERR;
    while (*str) {
        if (*str <'0' || *str>'9') {
            return UGERR;
        }
        ++str;
    }
    return UGOK;
}

int xstrdigest_convert(unsigned char *sour,int sourlen,char *dest,int destlen) {
    int idx=0;
    static const char digits[] = "0123456789abcdef";
    if (sourlen*2>destlen) {
        return UGERR;
    }
    memset(dest,0,destlen);
    while (idx<sourlen) {
        unsigned char c = sour[idx];
        dest[idx*2]=digits[(c >> 4) & 0xF];
        dest[idx*2+1]=digits[c & 0xF];
        idx++;
    }
    return UGOK;
}

/* Has to be rewritten to avoid malloc */
char *xstrreplace(const char *str, const char *sub_str1, const char *sub_str2) {
    char *new_str = NULL;
    const char *p;
    const char *q;
    const char *r;
    char *t;
    long len;
    long diff;
    unsigned long count = 0;

    assert(str);
    assert(sub_str1);
    assert(sub_str2);

    len = (long)strlen(sub_str1);

    /* count the number of occurrences of sub_str1 */
    for ( p=str; (p = strstr(p, sub_str1)); p+=len, count++ );

    if ( 0 == count )   return xstrdup(str);

    diff = (long)strlen(sub_str2) - len;

    /* allocate new memory */

    new_str = (char *)zmalloc((size_t)(strlen(str) + count*diff + 1)*sizeof(char));

    for (q=str,t=new_str,p=str; (p = strstr(p, sub_str1)); ) {
        /* copy until next occurrence of sub_str1 */
        for ( ; q < p; *t++ = *q++);
        q += len;
        p = q;
        for ( r = sub_str2; (*t++ = *r++); );
        --t;
    }
    /* copy the tail of str */
    for ( ; *q ; *t++ = *q++ );

    *t = '\0';

    return new_str;
}

char *xstrdup(const char *s) {
    size_t l = strlen(s)+1;
    char *p = zmalloc(l);
    memcpy(p,s,l);
    return p;
}

char *xstrprintf(const char *fmt, ...) {
    va_list ap;
    va_list cpy;
    char *buf;
    size_t buflen = 16;
    va_start(ap, fmt);
#if !defined(va_copy)
#define va_copy(d,s)  d = (s)
#endif
    while(1) {
        buf = zmalloc(buflen);
        if (buf == NULL) return NULL;
        buf[buflen-2] = '\0';
        va_copy(cpy,ap);
        vsnprintf(buf, buflen, fmt, cpy);
        if (buf[buflen-2] != '\0') {
            zfree(buf);
            buflen *= 2;
            continue;
        }
        break;
    }

    va_end(ap);
    return buf;
}

int xstrtoint(const char *str, int slen) {
    int len = 0;
    int idx = 0;
    char *p=(char *)str;
    for (; idx<slen; idx++) {
        len = (len*10)+(*p - '0');
        p++;
    }

    return len;
}

/* Glob-style pattern matching. */
int xstrmatchlen(const char *pattern, int patternLen,
                 const char *string, int stringLen, int nocase) {
    while (patternLen) {
        switch (pattern[0]) {
        case '*':
            while (pattern[1] == '*') {
                pattern++;
                patternLen--;
            }
            if (patternLen == 1)
                return 1; /* match */
            while(stringLen) {
                if (xstrmatchlen(pattern+1, patternLen-1,
                                 string, stringLen, nocase))
                    return 1; /* match */
                string++;
                stringLen--;
            }
            return 0; /* no match */
            break;
        case '?':
            if (stringLen == 0)
                return 0; /* no match */
            string++;
            stringLen--;
            break;
        case '[': {
            int not, match;

            pattern++;
            patternLen--;
            not = pattern[0] == '^';
            if (not) {
                pattern++;
                patternLen--;
            }
            match = 0;
            while(1) {
                if (pattern[0] == '\\') {
                    pattern++;
                    patternLen--;
                    if (pattern[0] == string[0])
                        match = 1;
                } else if (pattern[0] == ']') {
                    break;
                } else if (patternLen == 0) {
                    pattern--;
                    patternLen++;
                    break;
                } else if (pattern[1] == '-' && patternLen >= 3) {
                    int start = pattern[0];
                    int end = pattern[2];
                    int c = string[0];
                    if (start > end) {
                        int t = start;
                        start = end;
                        end = t;
                    }
                    if (nocase) {
                        start = tolower(start);
                        end = tolower(end);
                        c = tolower(c);
                    }
                    pattern += 2;
                    patternLen -= 2;
                    if (c >= start && c <= end)
                        match = 1;
                } else {
                    if (!nocase) {
                        if (pattern[0] == string[0])
                            match = 1;
                    } else {
                        if (tolower((int)pattern[0]) == tolower((int)string[0]))
                            match = 1;
                    }
                }
                pattern++;
                patternLen--;
            }
            if (not)
                match = !match;
            if (!match)
                return 0; /* no match */
            string++;
            stringLen--;
            break;
        }
        case '\\':
            if (patternLen >= 2) {
                pattern++;
                patternLen--;
            }
            /* fall through */
        default:
            if (!nocase) {
                if (pattern[0] != string[0])
                    return 0; /* no match */
            } else {
                if (tolower((int)pattern[0]) != tolower((int)string[0]))
                    return 0; /* no match */
            }
            string++;
            stringLen--;
            break;
        }
        pattern++;
        patternLen--;
        if (stringLen == 0) {
            while(*pattern == '*') {
                pattern++;
                patternLen--;
            }
            break;
        }
    }
    if (patternLen == 0 && stringLen == 0)
        return 1;
    return 0;
}

int xstrmatch(const char *pattern, const char *string, int nocase) {
    return xstrmatchlen(pattern,strlen(pattern),string,strlen(string),nocase);
}
