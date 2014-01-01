#include "sysinfo/sysinfo.h"
#include <assert.h>

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#define  statvfs statfs
#define  f_frsize f_bsize
#else
#include <sys/statvfs.h>
#endif


static int  get_fs_size_stat(const char* fsname, uint64_t* total, uint64_t* free,
                             uint64_t* used, double* pfree, double* pused)
{
    struct statvfs      s;
    assert(fsname);

    if (0 != statvfs(fsname, &s))
        return UGERR;

    if (total)
        *total = (uint64_t)s.f_blocks * s.f_frsize;
    if (free)
        *free  = (uint64_t)s.f_bavail * s.f_frsize;
    if (used)
        *used  = (uint64_t)(s.f_blocks - s.f_bfree) * s.f_frsize;
    if (pfree) {
        if (0 != s.f_blocks - s.f_bfree + s.f_bavail)
            *pfree = (double)(100.0 * s.f_bavail) /
                     (s.f_blocks - s.f_bfree + s.f_bavail);
        else
            *pfree = 0;
    }
    if (pused) {
        if (0 != s.f_blocks - s.f_bfree + s.f_bavail)
            *pused = 100.0 - (double)(100.0 * s.f_bavail) /
                     (s.f_blocks - s.f_bfree + s.f_bavail);
        else
            *pused = 0;
    }

    return UGOK;
}


int VFS_FS_SIZE(const char* cmd, int argc,const char** argv,SYSINFO_RESULT* result)
{
    char* mode = NULL;
    uint64_t      total, free, used;
    double      pfree, pused;
    if (argc == 0) {
        return UGERR;
    }
    mode = strrchr(cmd,'.')+1;

    if (UGOK != get_fs_size_stat(getParam(argc,argv,0), &total, &free, &used, &pfree, &pused))
        return UGERR;

    /* default parameter */
    if ('\0' == *mode || 0 == strcmp(mode, "total"))    /* default parameter */
        SET_UI64_RESULT(result, bytesConvert(total,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "free"))
        SET_UI64_RESULT(result,bytesConvert(free,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "used"))
        SET_UI64_RESULT(result, bytesConvert(used,getParam(argc,argv,1)));
    else if (0 == strcmp(mode, "pfree"))
        SET_DBL_RESULT(result, pfree);
    else if (0 == strcmp(mode, "pused"))
        SET_DBL_RESULT(result, pused);
    else
        return UGERR;

    return UGOK;
}



/***
int VFS_FS_TYPE(const char*cmd, int argc,const char** argv,SYSINFO_RESULT *result) {
  struct statfs statfs_buf;
  if (statfs(path.value().c_str(), &statfs_buf) < 0) {
    if (errno == ENOENT)
      return false;
    *type = FILE_SYSTEM_UNKNOWN;
    return true;
  }

  // While you would think the possible values of f_type would be available
  // in a header somewhere, it appears that is not the case.  These values
  // are copied from the statfs man page.
  switch (statfs_buf.f_type) {
    case 0:
      *type = FILE_SYSTEM_0;
      break;
    case 0xEF53:  // ext2, ext3.
    case 0x4D44:  // dos
    case 0x5346544E:  // NFTS
    case 0x52654973:  // reiser
    case 0x58465342:  // XFS
    case 0x9123683E:  // btrfs
    case 0x3153464A:  // JFS
      *type = FILE_SYSTEM_ORDINARY;
      break;
    case 0x6969:  // NFS
      *type = FILE_SYSTEM_NFS;
      break;
    case 0xFF534D42:  // CIFS
    case 0x517B:  // SMB
      *type = FILE_SYSTEM_SMB;
      break;
    case 0x73757245:  // Coda
      *type = FILE_SYSTEM_CODA;
      break;
    case 0x858458f6:  // ramfs
    case 0x01021994:  // tmpfs
      *type = FILE_SYSTEM_MEMORY;
      break;
    case 0x27e0eb: // CGROUP
      *type = FILE_SYSTEM_CGROUP;
      break;
    default:
      *type = FILE_SYSTEM_OTHER;
  }
  return true;
}

**/


