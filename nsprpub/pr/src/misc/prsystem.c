




































#include "primpl.h"
#include "prsystem.h"
#include "prprf.h"
#include "prlong.h"

#if defined(BEOS)
#include <kernel/OS.h>
#endif

#if defined(OS2)
#define INCL_DOS
#define INCL_DOSMISC
#include <os2.h>

#ifndef QSV_NUMPROCESSORS
#define QSV_NUMPROCESSORS 26
#endif
#endif


#if defined(BSDI) || defined(FREEBSD) || defined(NETBSD) \
    || defined(OPENBSD) || defined(DARWIN)
#define _PR_HAVE_SYSCTL
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(DARWIN)
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

#if defined(HPUX)
#include <sys/mpctl.h>
#include <sys/pstat.h>
#endif

#if defined(XP_UNIX)
#include <unistd.h>
#include <sys/utsname.h>
#endif

#if defined(AIX)
#include <cf.h>
#include <sys/cfgodm.h>
#endif

#if defined(WIN32)

typedef struct {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullToalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} PR_MEMORYSTATUSEX;


typedef BOOL (WINAPI *GlobalMemoryStatusExFn)(PR_MEMORYSTATUSEX *);
#endif

PR_IMPLEMENT(char) PR_GetDirectorySeparator(void)
{
    return PR_DIRECTORY_SEPARATOR;
}  




PR_IMPLEMENT(char) PR_GetDirectorySepartor(void)
{
#if defined(DEBUG)
    static PRBool warn = PR_TRUE;
    if (warn) {
        warn = _PR_Obsolete("PR_GetDirectorySepartor()",
                "PR_GetDirectorySeparator()");
    }
#endif
    return PR_GetDirectorySeparator();
}  

PR_IMPLEMENT(char) PR_GetPathSeparator(void)
{
    return PR_PATH_SEPARATOR;
}  

PR_IMPLEMENT(PRStatus) PR_GetSystemInfo(PRSysInfo cmd, char *buf, PRUint32 buflen)
{
    PRUintn len = 0;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    switch(cmd)
    {
      case PR_SI_HOSTNAME:
      case PR_SI_HOSTNAME_UNTRUNCATED:
        if (PR_FAILURE == _PR_MD_GETHOSTNAME(buf, (PRUintn)buflen))
            return PR_FAILURE;

        if (cmd == PR_SI_HOSTNAME_UNTRUNCATED)
            break;
        




#ifndef _PR_GET_HOST_ADDR_AS_NAME
        
            while (buf[len] && (len < buflen)) {
                if (buf[len] == '.') {
                    buf[len] = '\0';
                    break;
                }
                len += 1;
            }    
#endif
         break;

      case PR_SI_SYSNAME:
        
#if defined(XP_UNIX) || defined(WIN32)
        if (PR_FAILURE == _PR_MD_GETSYSINFO(cmd, buf, (PRUintn)buflen))
            return PR_FAILURE;
#else
        (void)PR_snprintf(buf, buflen, _PR_SI_SYSNAME);
#endif
        break;

      case PR_SI_RELEASE:
        
#if defined(XP_UNIX) || defined(WIN32)
        if (PR_FAILURE == _PR_MD_GETSYSINFO(cmd, buf, (PRUintn)buflen))
            return PR_FAILURE;
#endif
#if defined(XP_OS2)
        {
            ULONG os2ver[2] = {0};
            DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_REVISION,
                            &os2ver, sizeof(os2ver));
            

            if (os2ver[0] < 30)
              (void)PR_snprintf(buf, buflen, "%s%lu",
                                "2.", os2ver[0]);
            else if (os2ver[0] < 45)
              (void)PR_snprintf(buf, buflen, "%lu%s%lu",
                                os2ver[0]/10, ".", os2ver[1]);
            else
              (void)PR_snprintf(buf, buflen, "%.1f",
                                os2ver[0]/10.0);
        }
#endif 
        break;

      case PR_SI_ARCHITECTURE:
        
        (void)PR_snprintf(buf, buflen, _PR_SI_ARCHITECTURE);
        break;
	  default:
			PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
			return PR_FAILURE;
    }
    return PR_SUCCESS;
}















PR_IMPLEMENT(PRInt32) PR_GetNumberOfProcessors( void )
{
    PRInt32     numCpus;
#if defined(WIN32)
    SYSTEM_INFO     info;

    GetSystemInfo( &info );
    numCpus = info.dwNumberOfProcessors;
#elif defined(BEOS)
    system_info sysInfo;

    get_system_info(&sysInfo);
    numCpus = sysInfo.cpu_count;
#elif defined(OS2)
    DosQuerySysInfo( QSV_NUMPROCESSORS, QSV_NUMPROCESSORS, &numCpus, sizeof(numCpus));
#elif defined(_PR_HAVE_SYSCTL)
    int mib[2];
    int rc;
    size_t len = sizeof(numCpus);

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    rc = sysctl( mib, 2, &numCpus, &len, NULL, 0 );
    if ( -1 == rc )  {
        numCpus = -1; 
        _PR_MD_MAP_DEFAULT_ERROR( _MD_ERRNO() );
    }
#elif defined(HPUX)
    numCpus = mpctl( MPC_GETNUMSPUS, 0, 0 );
    if ( numCpus < 1 )  {
        numCpus = -1; 
        _PR_MD_MAP_DEFAULT_ERROR( _MD_ERRNO() );
    }
#elif defined(IRIX)
    numCpus = sysconf( _SC_NPROC_ONLN );
#elif defined(RISCOS) || defined(SYMBIAN)
    numCpus = 1;
#elif defined(XP_UNIX)
    numCpus = sysconf( _SC_NPROCESSORS_ONLN );
#else
#error "An implementation is required"
#endif
    return(numCpus);
} 












PR_IMPLEMENT(PRUint64) PR_GetPhysicalMemorySize(void)
{
    PRUint64 bytes = 0;

#if defined(LINUX) || defined(SOLARIS)

    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    if (pageSize >= 0 && pageCount >= 0)
        bytes = (PRUint64) pageSize * pageCount;

#elif defined(NETBSD) || defined(OPENBSD)

    int mib[2];
    int rc;
    uint64_t memSize;
    size_t len = sizeof(memSize);

    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM64;
    rc = sysctl(mib, 2, &memSize, &len, NULL, 0);
    if (-1 != rc)  {
        bytes = memSize;
    }

#elif defined(HPUX)

    struct pst_static info;
    int result = pstat_getstatic(&info, sizeof(info), 1, 0);
    if (result == 1)
        bytes = (PRUint64) info.physical_memory * info.page_size;

#elif defined(DARWIN)

    struct host_basic_info hInfo;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;

    int result = host_info(mach_host_self(),
                           HOST_BASIC_INFO,
                           (host_info_t) &hInfo,
                           &count);
    if (result == KERN_SUCCESS)
        bytes = hInfo.max_mem;

#elif defined(WIN32)

    
    GlobalMemoryStatusExFn globalMemory = (GlobalMemoryStatusExFn) NULL;
    HMODULE module = GetModuleHandleW(L"kernel32.dll");

    if (module) {
        globalMemory = (GlobalMemoryStatusExFn)GetProcAddress(module, "GlobalMemoryStatusEx");

        if (globalMemory) {
            PR_MEMORYSTATUSEX memStat;
            memStat.dwLength = sizeof(memStat);

            if (globalMemory(&memStat))
                bytes = memStat.ullTotalPhys;
        }
    }

    if (!bytes) {
        
        MEMORYSTATUS memStat;
        memset(&memStat, 0, sizeof(memStat));
        GlobalMemoryStatus(&memStat);
        bytes = memStat.dwTotalPhys;
    }

#elif defined(OS2)

    ULONG ulPhysMem;
    DosQuerySysInfo(QSV_TOTPHYSMEM,
                    QSV_TOTPHYSMEM,
                    &ulPhysMem,
                    sizeof(ulPhysMem));
    bytes = ulPhysMem;

#elif defined(AIX)

    if (odm_initialize() == 0) {
        int how_many;
        struct CuAt *obj = getattr("sys0", "realmem", 0, &how_many);
        if (obj != NULL) {
            bytes = (PRUint64) atoi(obj->value) * 1024;
            free(obj);
        }
        odm_terminate();
    }

#else

    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);

#endif

    return bytes;
} 
