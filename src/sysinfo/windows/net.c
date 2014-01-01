#include "sysinfo/sysinfo.h"
#include <assert.h>
#include "utils/string.h"
#include "utils/error.h"
#include "utils/logger.h"


static int  get_if_stats(const char* if_name, MIB_IFROW* pIfRow)
{
    DWORD       dwSize, dwRetVal, i;
    int     ret = UGERR;
    /* variables used for GetIfTable and GetIfEntry */
    MIB_IFTABLE* pIfTable = NULL;
    MIB_IFROW   TmpIfRow;
    /* Allocate memory for our pointers. */
    dwSize = sizeof(MIB_IFTABLE);
    pIfTable = (MIB_IFTABLE*)zmalloc( dwSize);

    /* Before calling GetIfEntry, we call GetIfTable to make
       sure there are entries to get and retrieve the interface index.
       Make an initial call to GetIfTable to get the necessary size into dwSize */
    if (ERROR_INSUFFICIENT_BUFFER == GetIfTable(pIfTable, &dwSize, 0))
        pIfTable = (MIB_IFTABLE*)zrealloc(pIfTable, dwSize);

    /* Make a second call to GetIfTable to get the actual data we want. */
    if (NO_ERROR != (dwRetVal = GetIfTable(pIfTable, &dwSize, 0))) {
        LOG_DEBUG("GetIfTable failed with error: %s", xerrmsg());
        goto clean;
    }

    memset(pIfRow,0,sizeof(MIB_IFROW));
    for (i = 0; i < pIfTable->dwNumEntries; i++) {
        memset(&TmpIfRow,0,sizeof(MIB_IFROW));
        TmpIfRow.dwIndex = pIfTable->table[i].dwIndex;
        if (NO_ERROR != (dwRetVal = GetIfEntry(&TmpIfRow))) {
            LOG_DEBUG("GetIfEntry failed with error: %s",
                      xerrmsg());
            continue;
        }

        /* ignore loopback addr */
        if (0 == strcmp(if_name, TmpIfRow.bDescr))
            continue;
        pIfRow->dwInOctets+=TmpIfRow.dwInOctets;
        pIfRow->dwOutOctets+=TmpIfRow.dwOutOctets;
        pIfRow->dwInErrors+=TmpIfRow.dwInErrors;
        pIfRow->dwOutErrors+=TmpIfRow.dwOutErrors;
        pIfRow->dwInUcastPkts+=TmpIfRow.dwInUcastPkts;
        pIfRow->dwInNUcastPkts+=TmpIfRow.dwInNUcastPkts;
        pIfRow->dwOutUcastPkts+=TmpIfRow.dwOutUcastPkts;
        pIfRow->dwOutNUcastPkts+=TmpIfRow.dwOutNUcastPkts;
        ret = UGOK;
    }
clean:
    zfree(pIfTable);
    return ret;
}



int NET_IF_IN(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    const char* if_name = "MS TCP Loopback interface";
    MIB_IFROW   pIfRow;
    if (UGERR == get_if_stats(if_name, &pIfRow))
        return UGERR;
    SET_UI64_RESULT(result,  bytesConvert(pIfRow.dwInOctets,getParam(argc,argv,0)));

    return UGOK;
}

int NET_IF_OUT(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    const char* if_name = "MS TCP Loopback interface";
    MIB_IFROW   pIfRow;
    if (UGERR == get_if_stats(if_name, &pIfRow))
        return UGERR;
    SET_UI64_RESULT(result, bytesConvert(pIfRow.dwOutOctets,getParam(argc,argv,0)));

    return UGOK;
}

int NET_IF_TOTAL(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    const char* if_name = "MS TCP Loopback interface";
    MIB_IFROW   pIfRow;
    if (UGERR == get_if_stats(if_name, &pIfRow))
        return UGERR;
    SET_UI64_RESULT(result,bytesConvert( pIfRow.dwInOctets + pIfRow.dwOutOctets,getParam(argc,argv,0)));

    return UGOK;
}

int NET_IF_COLLISIONS(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    SET_MSG_RESULT(result, xstrdup("not implemented"));
    return UGERR;
}
