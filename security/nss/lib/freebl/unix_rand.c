



































#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "secrng.h"
#include "secerr.h"
#include "prerror.h"
#include "prthread.h"
#include "prprf.h"

size_t RNG_FileUpdate(const char *fileName, size_t limit);










    
static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen)
{
    union endianness {
	int32 i;
	char c[4];
    } u;

    if (srclen <= dstlen) {
	memcpy(dst, src, srclen);
	return srclen;
    }
    u.i = 0x01020304;
    if (u.c[0] == 0x01) {
	
	memcpy(dst, (char*)src + (srclen - dstlen), dstlen);
    } else {
	
	memcpy(dst, src, dstlen);
    }
    return dstlen;
}

#ifdef SOLARIS

#include <kstat.h>

static const PRUint32 entropy_buf_len = 4096; 





static SECStatus BufferEntropy(char* inbuf, PRUint32 inlen,
                                char* entropy_buf, PRUint32* entropy_buffered,
                                PRUint32* total_fed)
{
    PRUint32 tocopy = 0;
    PRUint32 avail = 0;
    SECStatus rv = SECSuccess;

    while (inlen) {
        avail = entropy_buf_len - *entropy_buffered;
        if (!avail) {
            
            rv = RNG_RandomUpdate(entropy_buf, entropy_buf_len);
            if (SECSuccess != rv) {
                break;
            }
            *entropy_buffered = 0;
            avail = entropy_buf_len;
        }
        tocopy = PR_MIN(avail, inlen);
        memcpy(entropy_buf + *entropy_buffered, inbuf, tocopy);
        *entropy_buffered += tocopy;
        inlen -= tocopy;
        inbuf += tocopy;
        *total_fed += tocopy;
    }
    return rv;
}




static SECStatus RNG_kstat(PRUint32* fed)
{
    kstat_ctl_t*    kc = NULL;
    kstat_t*        ksp = NULL;
    PRUint32        entropy_buffered = 0;
    char*           entropy_buf = NULL;
    SECStatus       rv = SECSuccess;

    PORT_Assert(fed);
    if (!fed) {
        return SECFailure;
    }
    *fed = 0;

    kc = kstat_open();
    PORT_Assert(kc);
    if (!kc) {
        return SECFailure;
    }
    entropy_buf = (char*) PORT_Alloc(entropy_buf_len);
    PORT_Assert(entropy_buf);
    if (entropy_buf) {
        for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
            if (-1 == kstat_read(kc, ksp, NULL)) {
                
                continue;
            }
            rv = BufferEntropy((char*)ksp, sizeof(kstat_t),
                                    entropy_buf, &entropy_buffered,
                                    fed);
            if (SECSuccess != rv) {
                break;
            }

            if (ksp->ks_data && ksp->ks_data_size>0 && ksp->ks_ndata>0) {
                rv = BufferEntropy((char*)ksp->ks_data, ksp->ks_data_size,
                                        entropy_buf, &entropy_buffered,
                                        fed);
                if (SECSuccess != rv) {
                    break;
                }
            }
        }
        if (SECSuccess == rv && entropy_buffered) {
            
            rv = RNG_RandomUpdate(entropy_buf, entropy_buffered);
        }
        PORT_Free(entropy_buf);
    } else {
        rv = SECFailure;
    }
    if (kstat_close(kc)) {
        PORT_Assert(0);
        rv = SECFailure;
    }
    return rv;
}

#endif

#if defined(SCO) || defined(UNIXWARE) || defined(BSDI) || defined(FREEBSD) \
    || defined(NETBSD) || defined(DARWIN) || defined(OPENBSD) \
    || defined(NTO) || defined(__riscos__)
#include <sys/times.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    int ticks;
    struct tms buffer;

    ticks=times(&buffer);
    return CopyLowBits(buf, maxbytes, &ticks, sizeof(ticks));
}

static void
GiveSystemInfo(void)
{
    long si;

    


    si = sysconf(_SC_CHILD_MAX);
    RNG_RandomUpdate(&si, sizeof(si));

    si = sysconf(_SC_STREAM_MAX);
    RNG_RandomUpdate(&si, sizeof(si));

    si = sysconf(_SC_OPEN_MAX);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif

#if defined(__sun)
#if defined(__svr4) || defined(SVR4)
#include <sys/systeminfo.h>
#include <sys/times.h>
#include <wait.h>

int gettimeofday(struct timeval *);
int gethostname(char *, int);

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    hrtime_t t;
    t = gethrtime();
    if (t) {
	return CopyLowBits(buf, maxbytes, &t, sizeof(t));
    }
    return 0;
}
#else 

extern long sysconf(int name);

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    long si;

    
    si = sysconf(_SC_CHILD_MAX);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif
#endif 

#if defined(__hpux)
#include <sys/unistd.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

#if defined(__ia64)
#include <ia64/sys/inline.h>

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    PRUint64 t;

    t = _Asm_mov_from_ar(_AREG44);
    return CopyLowBits(buf, maxbytes, &t, sizeof(t));
}
#else
static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    extern int ret_cr16();
    int cr16val;

    cr16val = ret_cr16();
    return CopyLowBits(buf, maxbytes, &cr16val, sizeof(cr16val));
}
#endif

static void
GiveSystemInfo(void)
{
    long si;

    
    si = sysconf(_AES_OS_VERSION);
    RNG_RandomUpdate(&si, sizeof(si));
    si = sysconf(_SC_CPU_VERSION);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif 

#if defined(OSF1)
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/systeminfo.h>
#include <c_asm.h>

static void
GiveSystemInfo(void)
{
    char buf[BUFSIZ];
    int rv;
    int off = 0;

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}






static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    unsigned long t;

    t = asm("rpcc %v0");
    return CopyLowBits(buf, maxbytes, &t, sizeof(t));
}

#endif 

#if defined(_IBMR2)
static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    
}
#endif 

#if defined(LINUX)
#include <sys/sysinfo.h>

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
	RNG_RandomUpdate(&si, sizeof(si));
    }
}
#endif 

#if defined(NCR)

#include <sys/utsname.h>
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

#endif 

#if defined(sgi)
#include <fcntl.h>
#undef PRIVATE
#include <sys/mman.h>
#include <sys/syssgi.h>
#include <sys/immu.h>
#include <sys/systeminfo.h>
#include <sys/utsname.h>
#include <wait.h>

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[4096];

    rv = syssgi(SGI_SYSID, &buf[0]);
    if (rv > 0) {
	RNG_RandomUpdate(buf, MAXSYSIDSIZE);
    }
#ifdef SGI_RDUBLK
    rv = syssgi(SGI_RDUBLK, getpid(), &buf[0], sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, sizeof(buf));
    }
#endif 
    rv = syssgi(SGI_INVENT, SGI_INV_READ, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, sizeof(buf));
    }
    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

static size_t GetHighResClock(void *buf, size_t maxbuf)
{
    unsigned phys_addr, raddr, cycleval;
    static volatile unsigned *iotimer_addr = NULL;
    static int tries = 0;
    static int cntr_size;
    int mfd;
    long s0[2];
    struct timeval tv;

#ifndef SGI_CYCLECNTR_SIZE
#define SGI_CYCLECNTR_SIZE      165     /* Size user needs to use to read CC */
#endif

    if (iotimer_addr == NULL) {
	if (tries++ > 1) {
	    
	    return 0;
	}

	



	phys_addr = syssgi(SGI_QUERY_CYCLECNTR, &cycleval);
	if (phys_addr) {
	    int pgsz = getpagesize();
	    int pgoffmask = pgsz - 1;

	    raddr = phys_addr & ~pgoffmask;
	    mfd = open("/dev/mmem", O_RDONLY);
	    if (mfd < 0) {
		return 0;
	    }
	    iotimer_addr = (unsigned *)
		mmap(0, pgoffmask, PROT_READ, MAP_PRIVATE, mfd, (int)raddr);
	    if (iotimer_addr == (void*)-1) {
		close(mfd);
		iotimer_addr = NULL;
		return 0;
	    }
	    iotimer_addr = (unsigned*)
		((__psint_t)iotimer_addr | (phys_addr & pgoffmask));
	    


	    cntr_size = syssgi(SGI_CYCLECNTR_SIZE);
	    if (cntr_size < 0) {
		struct utsname utsinfo;

		






		uname(&utsinfo);
		if (!strncmp(utsinfo.machine, "IP19", 4) ||
		    !strncmp(utsinfo.machine, "IP21", 4))
			cntr_size = 64;
		else
			cntr_size = 32;
	    }
	    cntr_size /= 8;	
	}
    }

    s0[0] = *iotimer_addr;
    if (cntr_size > 4)
	s0[1] = *(iotimer_addr + 1);
    memcpy(buf, (char *)&s0[0], cntr_size);
    return CopyLowBits(buf, maxbuf, &s0, cntr_size);
}
#endif

#if defined(sony)
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif 

#if defined(sinix)
#include <sys/systeminfo.h>
#include <sys/times.h>

int gettimeofday(struct timeval *, struct timezone *);
int gethostname(char *, int);

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    int ticks;
    struct tms buffer;

    ticks=times(&buffer);
    return CopyLowBits(buf, maxbytes, &ticks, sizeof(ticks));
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif 


#ifdef BEOS
#include <be/kernel/OS.h>

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    bigtime_t bigtime; 

    bigtime = real_time_clock_usecs();
    return CopyLowBits(buf, maxbytes, &bigtime, sizeof(bigtime));
}

static void
GiveSystemInfo(void)
{
    system_info *info = NULL;
    int32 val;                     
    get_system_info(info);
    if (info) {
        val = info->boot_time;
        RNG_RandomUpdate(&val, sizeof(val));
        val = info->used_pages;
        RNG_RandomUpdate(&val, sizeof(val));
        val = info->used_ports;
        RNG_RandomUpdate(&val, sizeof(val));
        val = info->used_threads;
        RNG_RandomUpdate(&val, sizeof(val));
        val = info->used_teams;
        RNG_RandomUpdate(&val, sizeof(val));
    }
}
#endif 

#if defined(nec_ews)
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif 

size_t RNG_GetNoise(void *buf, size_t maxbytes)
{
    struct timeval tv;
    int n = 0;
    int c;

    n = GetHighResClock(buf, maxbytes);
    maxbytes -= n;

#if defined(__sun) && (defined(_svr4) || defined(SVR4)) || defined(sony)
    (void)gettimeofday(&tv);
#else
    (void)gettimeofday(&tv, 0);
#endif
    c = CopyLowBits((char*)buf+n, maxbytes, &tv.tv_usec, sizeof(tv.tv_usec));
    n += c;
    maxbytes -= c;
    c = CopyLowBits((char*)buf+n, maxbytes, &tv.tv_sec, sizeof(tv.tv_sec));
    n += c;
    return n;
}

#define SAFE_POPEN_MAXARGS	10	/* must be at least 2 */






static pid_t safe_popen_pid;
static struct sigaction oldact;

static FILE *
safe_popen(char *cmd)
{
    int p[2], fd, argc;
    pid_t pid;
    char *argv[SAFE_POPEN_MAXARGS + 1];
    FILE *fp;
    static char blank[] = " \t";
    static struct sigaction newact;

    if (pipe(p) < 0)
	return 0;

    fp = fdopen(p[0], "r");
    if (fp == 0) {
	close(p[0]);
	close(p[1]);
	return 0;
    }

    
    newact.sa_handler = SIG_DFL;
    newact.sa_flags = 0;
    sigfillset(&newact.sa_mask);
    sigaction (SIGCHLD, &newact, &oldact);

    pid = fork();
    switch (pid) {
      int ndesc;

      case -1:
	fclose(fp); 
	close(p[1]);
	sigaction (SIGCHLD, &oldact, NULL);
	return 0;

      case 0:
	
	if (p[1] != 1) dup2(p[1], 1);
	if (p[1] != 2) dup2(p[1], 2);

	



	if (!freopen("/dev/null", "r", stdin))
	    close(0);
	ndesc = getdtablesize();
	for (fd = PR_MIN(65536, ndesc); --fd > 2; close(fd));

	
	putenv("PATH=/bin:/usr/bin:/sbin:/usr/sbin:/etc:/usr/etc");
	putenv("SHELL=/bin/sh");
	putenv("IFS= \t");

	



	cmd = strdup(cmd);
	
	argv[0] = strtok(cmd, blank);
	argc = 1;
	while ((argv[argc] = strtok(0, blank)) != 0) {
	    if (++argc == SAFE_POPEN_MAXARGS) {
		argv[argc] = 0;
		break;
	    }
	}

	
	execvp(argv[0], argv);
	exit(127);
	break;

      default:
	close(p[1]);
	break;
    }

    
    safe_popen_pid = pid;
    return fp;
}

static int
safe_pclose(FILE *fp)
{
    pid_t pid;
    int status = -1, rv;

    if ((pid = safe_popen_pid) == 0)
	return -1;
    safe_popen_pid = 0;

    fclose(fp);

    
    PR_Sleep(PR_INTERVAL_NO_WAIT);

    
    while ((rv = waitpid(pid, &status, WNOHANG)) == -1 && errno == EINTR)
	;
    if (rv == 0) {
	kill(pid, SIGKILL);
	while ((rv = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
	    ;
    }

    
    sigaction(SIGCHLD, &oldact, NULL);

    return status;
}

#ifdef DARWIN
#include <crt_externs.h>
#endif




#define DO_NETSTAT 1

void RNG_SystemInfoForRNG(void)
{
    FILE *fp;
    char buf[BUFSIZ];
    size_t bytes;
    const char * const *cp;
    char *randfile;
#ifdef DARWIN
    char **environ = *_NSGetEnviron();
#else
    extern char **environ;
#endif
#ifdef BEOS
    static const char * const files[] = {
	"/boot/var/swap",
	"/boot/var/log/syslog",
	"/boot/var/tmp",
	"/boot/home/config/settings",
	"/boot/home",
	0
    };
#else
    static const char * const files[] = {
	"/etc/passwd",
	"/etc/utmp",
	"/tmp",
	"/var/tmp",
	"/usr/tmp",
	0
    };
#endif

#if defined(BSDI)
    static char netstat_ni_cmd[] = "netstat -nis";
#else
    static char netstat_ni_cmd[] = "netstat -ni";
#endif

    GiveSystemInfo();

    bytes = RNG_GetNoise(buf, sizeof(buf));
    RNG_RandomUpdate(buf, bytes);

    





    if (environ != NULL) {
        cp = (const char * const *) environ;
        while (*cp) {
	    RNG_RandomUpdate(*cp, strlen(*cp));
	    cp++;
        }
        RNG_RandomUpdate(environ, (char*)cp - (char*)environ);
    }

    
    if (gethostname(buf, sizeof(buf)) == 0) {
	RNG_RandomUpdate(buf, strlen(buf));
    }
    GiveSystemInfo();

    
    bytes = RNG_FileUpdate("/dev/urandom", SYSTEM_RNG_SEED_COUNT);

    
    randfile = getenv("NSRANDFILE");
    if ( ( randfile != NULL ) && ( randfile[0] != '\0') ) {
	char *randCountString = getenv("NSRANDCOUNT");
	int randCount = randCountString ? atoi(randCountString) : 0;
	if (randCount != 0) {
	    RNG_FileUpdate(randfile, randCount);
	} else {
	    RNG_FileForRNG(randfile);
	}
    }

    
    for (cp = files; *cp; cp++)
	RNG_FileForRNG(*cp);











#if defined(BSDI) || defined(LINUX)
    if (bytes)
        return;
#endif

#ifdef SOLARIS










#undef DO_NETSTAT
    if (!bytes) {
        
        PRUint32 kstat_bytes = 0;
        if (SECSuccess != RNG_kstat(&kstat_bytes)) {
            PORT_Assert(0);
        }
        bytes += kstat_bytes;
        PORT_Assert(bytes);
    }
#endif

#ifdef DO_NETSTAT
    fp = safe_popen(netstat_ni_cmd);
    if (fp != NULL) {
	while ((bytes = fread(buf, 1, sizeof(buf), fp)) > 0)
	    RNG_RandomUpdate(buf, bytes);
	safe_pclose(fp);
    }
#endif

}

#define TOTAL_FILE_LIMIT 1000000	/* one million */

size_t RNG_FileUpdate(const char *fileName, size_t limit)
{
    FILE *        file;
    size_t        bytes;
    size_t        fileBytes = 0;
    struct stat   stat_buf;
    unsigned char buffer[BUFSIZ];
    static size_t totalFileBytes = 0;
    
    
    memset(&stat_buf, 0, sizeof(stat_buf));

    if (stat((char *)fileName, &stat_buf) < 0)
	return fileBytes;
    RNG_RandomUpdate(&stat_buf, sizeof(stat_buf));
    
    file = fopen((char *)fileName, "r");
    if (file != NULL) {
	while (limit > fileBytes) {
	    bytes = PR_MIN(sizeof buffer, limit - fileBytes);
	    bytes = fread(buffer, 1, bytes, file);
	    if (bytes == 0) 
		break;
	    RNG_RandomUpdate(buffer, bytes);
	    fileBytes      += bytes;
	    totalFileBytes += bytes;
	    


	    if (totalFileBytes > TOTAL_FILE_LIMIT) 
		break;
	}
	fclose(file);
    }
    



    bytes = RNG_GetNoise(buffer, sizeof(buffer));
    RNG_RandomUpdate(buffer, bytes);
    return fileBytes;
}

void RNG_FileForRNG(const char *fileName)
{
    RNG_FileUpdate(fileName, TOTAL_FILE_LIMIT);
}

void ReadSingleFile(const char *fileName)
{
    FILE *        file;
    unsigned char buffer[BUFSIZ];
    
    file = fopen((char *)fileName, "rb");
    if (file != NULL) {
	while (fread(buffer, 1, sizeof(buffer), file) > 0)
	    ;
	fclose(file);
    } 
}

#define _POSIX_PTHREAD_SEMANTICS
#include <dirent.h>

PRBool
ReadFileOK(char *dir, char *file)
{
    struct stat   stat_buf;
    char filename[PATH_MAX];
    int count = snprintf(filename, sizeof filename, "%s/%s",dir, file);

    if (count <= 0) {
	return PR_FALSE; 
    }
    
    if (stat(filename, &stat_buf) < 0)
	return PR_FALSE; 
    return S_ISREG(stat_buf.st_mode) ? PR_TRUE : PR_FALSE;
}







int ReadOneFile(int fileToRead)
{
    char *dir = "/etc";
    DIR *fd = opendir(dir);
    int resetCount = 0;
#ifdef SOLARIS
     
    typedef union {
	unsigned char space[sizeof(struct dirent) + MAXNAMELEN];
	struct dirent dir;
    } dirent_hack;
    dirent_hack entry, firstEntry;

#define entry_dir entry.dir
#else
    struct dirent entry, firstEntry;
#define entry_dir entry
#endif

    int i, error = -1;

    if (fd == NULL) {
	dir = getenv("HOME");
	if (dir) {
	    fd = opendir(dir);
	}
    }
    if (fd == NULL) {
	return 1;
    }

    for (i=0; i <= fileToRead; i++) {
	struct dirent *result = NULL;
	do {
	    error = readdir_r(fd, &entry_dir, &result);
	} while (error == 0 && result != NULL  &&
					!ReadFileOK(dir,&result->d_name[0]));
	if (error != 0 || result == NULL)  {
	    resetCount = 1; 
	    if (i != 0) {
		
	 	entry = firstEntry;
	 	error = 0;
	 	break;
	    }
	    
	    break;
	}
	if (i==0) {
	    
	    firstEntry = entry;
	}
    }

    if (error == 0) {
	char filename[PATH_MAX];
	int count = snprintf(filename, sizeof filename, 
				"%s/%s",dir, &entry_dir.d_name[0]);
	if (count >= 1) {
	    ReadSingleFile(filename);
	}
    } 

    closedir(fd);
    return resetCount;
}




static void rng_systemJitter(void)
{
   static int fileToRead = 1;

   if (ReadOneFile(fileToRead)) {
	fileToRead = 1;
   } else {
	fileToRead++;
   }
}

size_t RNG_SystemRNG(void *dest, size_t maxLen)
{
    FILE *file;
    size_t bytes;
    size_t fileBytes = 0;
    unsigned char *buffer = dest;

    file = fopen("/dev/urandom", "r");
    if (file == NULL) {
	return rng_systemFromNoise(dest, maxLen);
    }
    while (maxLen > fileBytes) {
	bytes = maxLen - fileBytes;
	bytes = fread(buffer, 1, bytes, file);
	if (bytes == 0) 
	    break;
	fileBytes += bytes;
	buffer += bytes;
    }
    fclose(file);
    if (fileBytes != maxLen) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);  
	fileBytes = 0;
    }
    return fileBytes;
}
