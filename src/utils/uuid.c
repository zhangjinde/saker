
#include "uuid.h"
#include <stdio.h>
#include <string.h>
#include "common/types.h"

#ifdef OS_WIN
#include <Objbase.h>
#endif

#ifdef OS_UNIX

static uint64_t read_andom_uint64()
{
    uint64_t result;
    FILE* rnd = fopen("/dev/urandom", "rb");
    fread(&result, sizeof(result), 1, rnd);
    fclose(rnd);
    return result;
}
#endif


int uuidgen(char* uuidbyte)
{
#if OS_WIN
    unsigned char bytes[16];
    if (NULL == uuidbyte) {
        return UGERR;
    }
    CoCreateGuid((GUID*)bytes);
    sprintf(uuidbyte, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5],
            bytes[6], bytes[7],
            bytes[8], bytes[9],
            bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);

#else
    uint64_t sixteen_bytes[2] = {read_andom_uint64(), read_andom_uint64() };
    if (NULL == uuidbyte) {
        return UGERR;
    }

    sprintf(uuidbyte,"%08X-%04X-%04X-%04X-%012llX",
            (unsigned int)(sixteen_bytes[0] >> 32),
            (unsigned int)((sixteen_bytes[0] >> 16) & 0x0000ffff),
            (unsigned int)(sixteen_bytes[0] & 0x0000ffff),
            (unsigned int)(sixteen_bytes[1] >> 48),
            sixteen_bytes[1] & 0x0000ffffffffffffULL);
#endif
    return UGOK;
}



int uuidisvaild(const char* uuidbyte)
{

    const char*  hexchars   = "0123456789ABCDEF";
    const size_t uuidlength = 36U;
    size_t idx;
    if (NULL==uuidbyte || strlen(uuidbyte)!=uuidlength) {
        return UGERR;
    }
    for (idx= 0; idx < uuidlength; ++idx) {
        char current = uuidbyte[idx];
        if (idx == 8 || idx == 13 || idx == 18 || idx == 23) {
            if (current != '-')
                return UGERR;
        } else {
            if (strchr(hexchars,current) == NULL)
                return UGERR;
        }
    }

    return UGOK;
}
