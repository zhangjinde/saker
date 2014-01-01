#include "sysinfo/sysinfo.h"
#include <assert.h>

typedef struct {
    uint64_t ibytes;
    uint64_t ipackets;
    uint64_t ierr;
    uint64_t idrop;
    uint64_t obytes;
    uint64_t opackets;
    uint64_t oerr;
    uint64_t odrop;
    uint64_t colls;
}
net_stat_t;

//if_name not used now ,but maybe use in the futhre
static int  get_net_stat(const char* if_name, net_stat_t* result)
{
    int ret = UGERR;
    char    line[MAX_STRING_LEN], name[MAX_STRING_LEN], *p;
    FILE*    f;

    assert(result);
    net_stat_t  tmp_net_stat;
    memset(result, 0, sizeof(net_stat_t));   //you must memset it
    if (NULL != (f = fopen("/proc/net/dev", "r"))) {
        while (NULL != fgets(line, sizeof(line), f)) {
            if (NULL == (p = strstr(line, ":")))
                continue;

            *p = '\t';
            if (strstr(line, "eth") || strstr(line, "em")) {
                memset(&tmp_net_stat, 0, sizeof(net_stat_t));
                if (10 == sscanf(line, "%s\t" UG_FS_UI64 "\t" UG_FS_UI64 "\t"
                                 UG_FS_UI64 "\t" UG_FS_UI64 "\t%*s\t%*s\t%*s\t%*s\t"
                                 UG_FS_UI64 "\t" UG_FS_UI64 "\t" UG_FS_UI64 "\t"
                                 UG_FS_UI64 "\t%*s\t" UG_FS_UI64 "\t%*s\t%*s\n",
                                 name,
                                 &(tmp_net_stat.ibytes),    /* bytes */
                                 &(tmp_net_stat.ipackets),  /* packets */
                                 &(tmp_net_stat.ierr),  /* errs */
                                 &(tmp_net_stat.idrop), /* drop */
                                 &(tmp_net_stat.obytes),    /* bytes */
                                 &(tmp_net_stat.opackets),  /* packets*/
                                 &(tmp_net_stat.oerr),  /* errs */
                                 &(tmp_net_stat.odrop), /* drop */
                                 &(tmp_net_stat.colls))) {  /* icolls */
                    result->ibytes+=tmp_net_stat.ibytes;
                    result->ipackets+=tmp_net_stat.ipackets;
                    result->ierr+=tmp_net_stat.ierr;
                    result->idrop+=tmp_net_stat.idrop;
                    result->obytes+=tmp_net_stat.obytes;
                    result->opackets+=tmp_net_stat.opackets;
                    result->oerr+=tmp_net_stat.oerr;
                    result->odrop+=tmp_net_stat.odrop;
                    result->colls+=tmp_net_stat.colls;
                    ret = UGOK;
                }
            }
        }

        FILECLOSE(f);
    }


    if (ret != UGOK)
        memset(result, 0, sizeof(net_stat_t));

    return ret;
}

int NET_IF_IN(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    net_stat_t  ns;

    static const char* if_name = "lo";

    if (UGOK != get_net_stat(if_name, &ns)) {


        return UGERR;
    }

    SET_UI64_RESULT(result, ns.ibytes);

    return UGOK;
}

int NET_IF_OUT(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    net_stat_t  ns;

    static const char* if_name = "lo";

    if (UGOK != get_net_stat(if_name, &ns))
        return UGERR;

    SET_UI64_RESULT(result, ns.obytes);

    return UGOK;
}

int NET_IF_TOTAL(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    net_stat_t  ns;

    static const char* if_name = "lo";

    if (UGOK != get_net_stat(if_name, &ns))
        return UGERR;

    SET_UI64_RESULT(result, ns.ibytes + ns.obytes);

    return UGOK;
}

int NET_IF_COLLISIONS(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    net_stat_t  ns;

    static const char* if_name = "lo";

    if (UGOK != get_net_stat(if_name, &ns))
        return UGERR;

    SET_UI64_RESULT(result, ns.colls);

    return UGOK;
}
