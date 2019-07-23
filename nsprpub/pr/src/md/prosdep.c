




































#include "prbit.h"
#include "prsystem.h"

#ifdef XP_UNIX
#include <unistd.h>
#endif
#ifdef SUNOS4
#include "md/sunos4.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif 
#ifdef XP_BEOS
#include <OS.h>
#endif

PRInt32 _pr_pageShift;
PRInt32 _pr_pageSize;




static void GetPageSize(void)
{
	PRInt32 pageSize;

    
#ifdef XP_UNIX
#if defined SUNOS4 || defined BSDI || defined AIX \
        || defined LINUX || defined __GNU__ || defined __GLIBC__ \
        || defined FREEBSD || defined NETBSD || defined OPENBSD \
        || defined DARWIN || defined NEXTSTEP || defined SYMBIAN
    _pr_pageSize = getpagesize();
#elif defined(HPUX)
    
    _pr_pageSize = sysconf(_SC_PAGE_SIZE);
#else
    _pr_pageSize = sysconf(_SC_PAGESIZE);
#endif
#endif 

#ifdef XP_BEOS
    _pr_pageSize = B_PAGE_SIZE;
#endif

#ifdef XP_PC
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    _pr_pageSize = info.dwPageSize;
#else
    _pr_pageSize = 4096;
#endif
#endif 

	pageSize = _pr_pageSize;
	PR_CEILING_LOG2(_pr_pageShift, pageSize);
}

PR_IMPLEMENT(PRInt32) PR_GetPageShift(void)
{
    if (!_pr_pageSize) {
	GetPageSize();
    }
    return _pr_pageShift;
}

PR_IMPLEMENT(PRInt32) PR_GetPageSize(void)
{
    if (!_pr_pageSize) {
	GetPageSize();
    }
    return _pr_pageSize;
}
