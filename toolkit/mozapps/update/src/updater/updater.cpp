

















































#if defined(XP_WIN)
# include <windows.h>
# include <direct.h>
# include <io.h>
# define F_OK 00
# define W_OK 02
# define R_OK 04
# define access _access
# define putenv _putenv
# define snprintf _snprintf
# define fchmod(a,b)
# define mkdir(path, perms) _mkdir(path)

# define NS_T(str) L ## str
# define NS_tsnprintf _snwprintf
# define NS_tstrrchr wcsrchr
# define NS_tchdir _wchdir
# define NS_tremove _wremove
# define NS_tfopen _wfopen
# define NS_tatoi _wtoi64
#else
# include <sys/wait.h>
# include <unistd.h>

# define NS_T(str) str
# define NS_tsnprintf snprintf
# define NS_tstrrchr strrchr
# define NS_tchdir chdir
# define NS_tremove remove
# define NS_tfopen fopen
# define NS_tatoi atoi
#endif

#include "bspatch.h"
#include "progressui.h"
#include "archivereader.h"
#include "errors.h"
#include "bzlib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#ifdef WINCE
#include "updater_wince.h"
#endif

#if defined(XP_MACOSX)

void LaunchChild(int argc, char **argv);
#endif

#ifndef _O_BINARY
# define _O_BINARY 0
#endif

#ifndef NULL
# define NULL (0)
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX LONG_MAX
#endif

#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# elif defined(MAX_PATH)
#  define MAXPATHLEN MAX_PATH
# elif defined(_MAX_PATH)
#  define MAXPATHLEN _MAX_PATH
# elif defined(CCHMAXPATH)
#  define MAXPATHLEN CCHMAXPATH
# else
#  define MAXPATHLEN 1024
# endif
#endif



#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_EXECV
#endif





#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
extern "C"  __attribute__((visibility("default"))) unsigned int BZ2_crc32Table[256];
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
extern "C" __global unsigned int BZ2_crc32Table[256];
#else
extern "C" unsigned int BZ2_crc32Table[256];
#endif

static unsigned int
crc32(const unsigned char *buf, unsigned int len)
{
  unsigned int crc = 0xffffffffL;

  const unsigned char *end = buf + len;
  for (; buf != end; ++buf)
    crc = (crc << 8) ^ BZ2_crc32Table[(crc >> 24) ^ *buf];

  crc = ~crc;
  return crc;
}





class AutoFile
{
public:
  AutoFile(FILE* file = NULL)
    : mFile(file) {
  }

  ~AutoFile() {
    if (mFile != NULL)
      fclose(mFile);
  }

  AutoFile &operator=(FILE* file) {
    if (mFile != 0)
      fclose(mFile);
    mFile = file;
    return *this;
  }

  operator FILE*() {
    return mFile;
  }

private:
  FILE* mFile;
};



typedef void (* ThreadFunc)(void *param);

#ifdef XP_WIN
#include <process.h>

class Thread
{
public:
  int Run(ThreadFunc func, void *param)
  {
    mThreadFunc = func;
    mThreadParam = param;

#ifdef WINCE
    DWORD threadID;
#else
    unsigned int threadID;
#endif
    mThread = (HANDLE) _beginthreadex(NULL, 0, ThreadMain, this, 0, &threadID);
    
    return mThread ? 0 : -1;
  }
  int Join()
  {
    WaitForSingleObject(mThread, INFINITE);
    CloseHandle(mThread);
    return 0;
  }
private:
  static unsigned __stdcall ThreadMain(void *p)
  {
    Thread *self = (Thread *) p;
    self->mThreadFunc(self->mThreadParam);
    return 0;
  }
  HANDLE     mThread;
  ThreadFunc mThreadFunc;
  void      *mThreadParam;
};

#elif defined(XP_UNIX)
#include <pthread.h>

class Thread
{
public:
  int Run(ThreadFunc func, void *param)
  {
    return pthread_create(&thr, NULL, (void* (*)(void *)) func, param);
  }
  int Join()
  {
    void *result;
    return pthread_join(thr, &result);
  }
private:
  pthread_t thr;
};

#elif defined(XP_OS2)

class Thread
{
public:
  int Run(ThreadFunc func, void *param)
  {
    mThreadFunc = func;
    mThreadParam = param;

    mThread = _beginthread(ThreadMain, NULL, 16384, (void *)this);
    
    return mThread ? 0 : -1;
  }
  int Join()
  {
    int status;
    waitpid(mThread, &status, 0);
    return 0;
  }
private:
  static void ThreadMain(void *p)
  {
    Thread *self = (Thread *) p;
    self->mThreadFunc(self->mThreadParam);
  }
  int        mThread;
  ThreadFunc mThreadFunc;
  void      *mThreadParam;
};

#else
#error "Unsupported platform"
#endif



static NS_tchar* gSourcePath;
static ArchiveReader gArchiveReader;
#ifdef XP_WIN
static bool gSucceeded = FALSE;
#endif

static const char kWhitespace[] = " \t";
static const char kNL[] = "\r\n";
static const char kQuote[] = "\"";




static FILE *gLogFP = NULL;

static void LogInit()
{
  if (gLogFP)
    return;

  NS_tchar logFile[MAXPATHLEN];
  NS_tsnprintf(logFile, MAXPATHLEN, NS_T("%s/update.log"), gSourcePath);

  gLogFP = NS_tfopen(logFile, NS_T("w"));
}

static void LogFinish()
{
  if (!gLogFP)
    return;

  fclose(gLogFP);
  gLogFP = NULL;
}

static void LogPrintf(const char *fmt, ... )
{
  if (!gLogFP)
    return;

  va_list ap;
  va_start(ap, fmt);
  vfprintf(gLogFP, fmt, ap);
  va_end(ap);
}

#define LOG(args) LogPrintf args



static inline PRUint32
mmin(PRUint32 a, PRUint32 b)
{
  return (a > b) ? b : a;
}

static char*
mstrtok(const char *delims, char **str)
{
  if (!*str || !**str)
    return NULL;

  
  char *ret = *str;
  const char *d;
  do {
    for (d = delims; *d != '\0'; ++d) {
      if (*ret == *d) {
        ++ret;
        break;
      }
    }
  } while (*d);

  if (!*ret) {
    *str = ret;
    return NULL;
  }

  char *i = ret;
  do {
    for (d = delims; *d != '\0'; ++d) {
      if (*i == *d) {
        *i = '\0';
        *str = ++i;
        return ret;
      }
    }
    ++i;
  } while (*i);

  *str = NULL;
  return ret;
}

static void ensure_write_permissions(const char *path)
{
#ifdef XP_WIN
  (void) chmod(path, _S_IREAD | _S_IWRITE);
#else
  struct stat fs;
  if (!stat(path, &fs) && !(fs.st_mode & S_IWUSR)) {
    (void)chmod(path, fs.st_mode | S_IWUSR);
  }
#endif
}

static int ensure_remove(const char *path)
{
  ensure_write_permissions(path);
  int rv = remove(path);
  if (rv)
    LOG(("remove failed: %d,%d (%s)\n", rv, errno, path));
  return rv;
}

static FILE* ensure_open(const char *path, char* flags)
{
  ensure_write_permissions(path);
  return fopen(path, flags);
}


static int ensure_parent_dir(const char *path)
{
  int rv = OK;

  char *slash = (char *) strrchr(path, '/');
  if (slash)
  {
    *slash = '\0';
    rv = ensure_parent_dir(path);
    if (rv == OK) {
      rv = mkdir(path, 0755);
      
      if (rv < 0 && errno != EEXIST) {
        rv = WRITE_ERROR;
      } else {
        rv = OK;
      }
    }
    *slash = '/';
  }
  return rv;
}

static int copy_file(const char *spath, const char *dpath)
{
  int rv = ensure_parent_dir(dpath);
  if (rv)
    return rv;

  struct stat ss;

  AutoFile sfile = fopen(spath, "rb");
  if (sfile == NULL || fstat(fileno(sfile), &ss)) {
    LOG(("copy_file: failed to open or stat: %p,%s,%d\n", sfile, spath, errno));
    return READ_ERROR;
  }

  AutoFile dfile = ensure_open(dpath, "wb+"); 
  if (dfile == NULL) {
    LOG(("copy_file: failed to open: %s,%d\n", dpath, errno));
    return WRITE_ERROR;
  }

  char buf[BUFSIZ];
  int sc;
  while ((sc = fread(buf, 1, sizeof(buf), sfile)) > 0) {
    int dc;
    char *bp = buf;
    while ((dc = fwrite(bp, 1, (unsigned int) sc, dfile)) > 0) {
      if ((sc -= dc) == 0)
        break;
      bp += dc;
    }
    if (dc < 0) {
      LOG(("copy_file: failed to write: %d\n", errno));
      return WRITE_ERROR;
    }
  }
  if (sc < 0) {
    LOG(("copy_file: failed to read: %d\n", errno));
    return READ_ERROR;
  }

  return OK;
}



#define BACKUP_EXT ".moz-backup"


static int backup_create(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  return copy_file(path, backup);
}




static int backup_restore(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  int rv = copy_file(backup, path);
  if (rv)
    return rv;

  rv = ensure_remove(backup);
  if (rv)
    return WRITE_ERROR;

  return OK;
}


static int backup_discard(const char *path)
{
  char backup[MAXPATHLEN];
  snprintf(backup, sizeof(backup), "%s" BACKUP_EXT, path);

  int rv = ensure_remove(backup);
  if (rv)
    return WRITE_ERROR;

  return OK;
}


static void backup_finish(const char *path, int status)
{
  if (status == OK)
    backup_discard(path);
  else
    backup_restore(path);
}



static int DoUpdate();

static const int ACTION_DESCRIPTION_BUFSIZE = 256;

class Action
{
public:
  Action() : mNext(NULL) { }
  virtual ~Action() { }

  virtual int Parse(char *line) = 0;

  
  
  virtual int Prepare() = 0;

  
  
  
  virtual int Execute() = 0;
  
  
  
  virtual void Finish(int status) = 0;

private:
  Action* mNext;

  friend class ActionList;
};

class RemoveFile : public Action
{
public:
  RemoveFile() : mFile(NULL), mSkip(0) { }

  int Parse(char *line);
  int Prepare();
  int Execute();
  void Finish(int status);

private:
  const char* mFile;
  int mSkip;
};

int
RemoveFile::Parse(char *line)
{
  

  mFile = mstrtok(kQuote, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
RemoveFile::Prepare()
{
  LOG(("PREPARE REMOVE %s\n", mFile));

  
  int rv = access(mFile, F_OK);
  if (rv) {
    LOG(("file cannot be removed because it does not exist; skipping\n"));
    mSkip = 1;
    return OK;
  }

  char *slash = (char *) strrchr(mFile, '/');
  if (slash) {
    *slash = '\0';
    rv = access(mFile, W_OK);
    *slash = '/';
  } else {
    rv = access(".", W_OK);
  }

  if (rv) {
    LOG(("access failed: %d\n", errno));
    return WRITE_ERROR;
  }

  return OK;
}

int
RemoveFile::Execute()
{
  LOG(("EXECUTE REMOVE %s\n", mFile));

  if (mSkip)
    return OK;

  
  
  
  int rv = access(mFile, F_OK);
  if (rv) {
    LOG(("file cannot be removed because it does not exist; skipping\n"));
    mSkip = 1;
    return OK;
  }

  
  

  rv = backup_create(mFile);
  if (rv) {
    LOG(("backup_create failed: %d\n", rv));
    return rv;
  }

  rv = ensure_remove(mFile);
  if (rv)
    return WRITE_ERROR;

  return OK;
}

void
RemoveFile::Finish(int status)
{
  LOG(("FINISH REMOVE %s\n", mFile));

  if (mSkip)
    return;

  backup_finish(mFile, status);
}

class AddFile : public Action
{
public:
  AddFile() : mFile(NULL) { }

  virtual int Parse(char *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  const char *mFile;
};

int
AddFile::Parse(char *line)
{
  

  mFile = mstrtok(kQuote, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
AddFile::Prepare()
{
  LOG(("PREPARE ADD %s\n", mFile));

  return OK;
}

int
AddFile::Execute()
{
  LOG(("EXECUTE ADD %s\n", mFile));

  int rv;

  
  if (access(mFile, F_OK) == 0)
  {
    rv = backup_create(mFile);
    if (rv)
      return rv;

    rv = ensure_remove(mFile);
    if (rv)
      return WRITE_ERROR;
  }
  else
  {
    rv = ensure_parent_dir(mFile);
    if (rv)
      return rv;
  }
    
  return gArchiveReader.ExtractFile(mFile, mFile);
}

void
AddFile::Finish(int status)
{
  LOG(("FINISH ADD %s\n", mFile));

  backup_finish(mFile, status);
}

class PatchFile : public Action
{
public:
  PatchFile() : mPatchIndex(-1), pfile(NULL), buf(NULL) { }
  virtual ~PatchFile();

  virtual int Parse(char *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  int LoadSourceFile(FILE* ofile);

  static int sPatchIndex;

  const char *mPatchFile;
  const char *mFile;
  int mPatchIndex;
  MBSPatchHeader header;
  FILE* pfile;
  unsigned char *buf;
};

int PatchFile::sPatchIndex = 0;

PatchFile::~PatchFile()
{
  if (pfile)
    fclose(pfile);

  
  NS_tchar spath[MAXPATHLEN];
  NS_tsnprintf(spath, MAXPATHLEN, NS_T("%s/%d.patch"),
               gSourcePath, mPatchIndex);

  NS_tremove(spath);

  free(buf);
}

int
PatchFile::LoadSourceFile(FILE* ofile)
{
  struct stat os;
  int rv = fstat(fileno(ofile), &os);
  if (rv)
    return READ_ERROR;

  if (PRUint32(os.st_size) != header.slen)
    return UNEXPECTED_ERROR;

  buf = (unsigned char*) malloc(header.slen);
  if (!buf)
    return MEM_ERROR;

  int r = header.slen;
  unsigned char *rb = buf;
  while (r) {
    int c = fread(rb, 1, mmin(BUFSIZ,r), ofile);
    if (c < 0)
      return READ_ERROR;

    r -= c;
    rb += c;

    if (c == 0 && r)
      return UNEXPECTED_ERROR;
  }

  

  unsigned int crc = crc32(buf, header.slen);

  if (crc != header.scrc32) {
    LOG(("CRC check failed\n"));
    return CRC_ERROR;
  }
  
  return OK;
}

int
PatchFile::Parse(char *line)
{
  

  mPatchFile = mstrtok(kQuote, &line);
  if (!mPatchFile)
    return PARSE_ERROR;

  
  char *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  mFile = mstrtok(kQuote, &line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
PatchFile::Prepare()
{
  LOG(("PREPARE PATCH %s\n", mFile));

  
  mPatchIndex = sPatchIndex++;

  NS_tchar spath[MAXPATHLEN];
  NS_tsnprintf(spath, MAXPATHLEN, NS_T("%s/%d.patch"),
               gSourcePath, mPatchIndex);

  NS_tremove(spath);

  FILE *fp = NS_tfopen(spath, NS_T("wb"));
  if (!fp)
    return WRITE_ERROR;

  int rv = gArchiveReader.ExtractFileToStream(mPatchFile, fp);
  fclose(fp);
  if (rv)
    return rv;

  
  
  

  pfile = NS_tfopen(spath, NS_T("rb"));
  if (pfile == NULL)
    return READ_ERROR;

  rv = MBS_ReadHeader(pfile, &header);
  if (rv)
    return rv;

  AutoFile ofile = fopen(mFile, "rb");
  if (ofile == NULL)
    return READ_ERROR;

  rv = LoadSourceFile(ofile);
  if (rv)
    LOG(("LoadSourceFile failed\n"));
  return rv;
}

int
PatchFile::Execute()
{
  LOG(("EXECUTE PATCH %s\n", mFile));

  

  struct stat ss;
  if (stat(mFile, &ss))
    return READ_ERROR;

  int rv = backup_create(mFile);
  if (rv)
    return rv;

  rv = ensure_remove(mFile);
  if (rv)
    return WRITE_ERROR;

  AutoFile ofile = ensure_open(mFile, "wb+");
  if (ofile == NULL)
    return WRITE_ERROR;

  return MBS_ApplyPatch(&header, pfile, buf, ofile);
}

void
PatchFile::Finish(int status)
{
  LOG(("FINISH PATCH %s\n", mFile));

  backup_finish(mFile, status);
}

class AddIfFile : public AddFile
{
public:
  AddIfFile() : mTestFile(NULL) { }

  virtual int Parse(char *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

protected:
  const char *mTestFile;
};

int
AddIfFile::Parse(char *line)
{
  

  mTestFile = mstrtok(kQuote, &line);
  if (!mTestFile)
    return PARSE_ERROR;

  
  char *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  return AddFile::Parse(line);
}

int
AddIfFile::Prepare()
{
  
  if (access(mTestFile, F_OK)) {
    mTestFile = NULL;
    return OK;
  }

  return AddFile::Prepare();
}

int
AddIfFile::Execute()
{
  if (!mTestFile)
    return OK;

  return AddFile::Execute();
}

void
AddIfFile::Finish(int status)
{
  if (!mTestFile)
    return;

  AddFile::Finish(status);
}

class PatchIfFile : public PatchFile
{
public:
  PatchIfFile() : mTestFile(NULL) { }

  virtual int Parse(char *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  const char *mTestFile;
};

int
PatchIfFile::Parse(char *line)
{
  

  mTestFile = mstrtok(kQuote, &line);
  if (!mTestFile)
    return PARSE_ERROR;

  
  char *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  return PatchFile::Parse(line);
}

int
PatchIfFile::Prepare()
{
  
  if (access(mTestFile, F_OK)) {
    mTestFile = NULL;
    return OK;
  }

  return PatchFile::Prepare();
}

int
PatchIfFile::Execute()
{
  if (!mTestFile)
    return OK;

  return PatchFile::Execute();
}

void
PatchIfFile::Finish(int status)
{
  if (!mTestFile)
    return;

  PatchFile::Finish(status);
}



#ifdef XP_WIN
#include "nsWindowsRestart.cpp"

static void
copyASCIItoWCHAR(WCHAR *dest, const char *src)
{
  while (*src) {
    *dest = *src;
    ++src; ++dest;
  }
}

static void
LaunchWinPostProcess(const WCHAR *appExe)
{
  
  
  WCHAR inifile[MAXPATHLEN];
  wcscpy(inifile, appExe);

  WCHAR *slash = wcsrchr(inifile, '\\');
  if (!slash)
    return;

  wcscpy(slash + 1, L"updater.ini");

  WCHAR exefile[MAXPATHLEN];
  WCHAR exearg[MAXPATHLEN];
  WCHAR exeasync[10];
  PRBool async = PR_TRUE;
#ifdef WINCE
  
#else
  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeRelPath", NULL, exefile,
                                MAXPATHLEN, inifile))
    return;

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeArg", NULL, exearg,
                                MAXPATHLEN, inifile))
    return;

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeAsync", L"TRUE", exeasync,
                                sizeof(exeasync)/sizeof(exeasync[0]), inifile))
    return;
#endif

  WCHAR exefullpath[MAXPATHLEN];
  wcscpy(exefullpath, appExe);

  slash = wcsrchr(exefullpath, '\\');
  wcscpy(slash + 1, exefile);

  WCHAR dlogFile[MAXPATHLEN];
  wcscpy(dlogFile, exefullpath);

  slash = wcsrchr(dlogFile, '\\');
  wcscpy(slash + 1, L"uninstall.update");

  WCHAR slogFile[MAXPATHLEN];
  _snwprintf(slogFile, MAXPATHLEN, L"%s/update.log", gSourcePath);

  WCHAR dummyArg[13];
  wcscpy(dummyArg, L"argv0ignored ");

  int len = wcslen(exearg) + wcslen(dummyArg);
  WCHAR *cmdline = (WCHAR *) malloc((len + 1) * sizeof(WCHAR));
  if (!cmdline)
    return;

  wcscpy(cmdline, dummyArg);
  wcscat(cmdline, exearg);

  if (!_wcsnicmp(exeasync, L"false", 6) || !_wcsnicmp(exeasync, L"0", 2))
    async = PR_FALSE;

  
  
  
  NS_tremove(dlogFile);
  CopyFile(slogFile, dlogFile, FALSE);

  STARTUPINFOW si = {sizeof(si), 0};
  PROCESS_INFORMATION pi = {0};

  BOOL ok = CreateProcessW(exefullpath,
                           cmdline,
                           NULL,  
                           NULL,  
                           FALSE, 
                           0,     
                           NULL,  
                           NULL,  
                           &si,
                           &pi);
  free(cmdline);

  if (ok) {
    if (!async)
      WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
}
#endif

static void
LaunchCallbackApp(const NS_tchar *workingDir, int argc, NS_tchar **argv)
{
  putenv("NO_EM_RESTART=");
  putenv("MOZ_LAUNCHED_CHILD=1");

  
  NS_tchdir(workingDir);

#if defined(USE_EXECV)
  execv(argv[0], argv);
#elif defined(XP_MACOSX)
  LaunchChild(argc, argv);
#elif defined(XP_WIN)
  WinLaunchChild(argv[0], argc, argv);
#else
# warning "Need implementaton of LaunchCallbackApp"
#endif
}

static void
WriteStatusFile(int status)
{
  

  NS_tchar filename[MAXPATHLEN];
  NS_tsnprintf(filename, MAXPATHLEN, NS_T("%s/update.status"), gSourcePath);

  AutoFile file = NS_tfopen(filename, NS_T("wb+"));
  if (file == NULL)
    return;

  const char *text;

  char buf[32];
  if (status == OK) {
    text = "succeeded\n";
  } else {
    snprintf(buf, sizeof(buf), "failed: %d\n", status);
    text = buf;
  }
  fwrite(text, strlen(text), 1, file);
}

static void
UpdateThreadFunc(void *param)
{
  

  NS_tchar dataFile[MAXPATHLEN];
  NS_tsnprintf(dataFile, MAXPATHLEN, NS_T("%s/update.mar"), gSourcePath);

  int rv = gArchiveReader.Open(dataFile);
  if (rv == OK) {
    rv = DoUpdate();
    gArchiveReader.Close();
  }

  if (rv)
    LOG(("failed: %d\n", rv));
  else
    LOG(("succeeded\n"));
  WriteStatusFile(rv);

  LOG(("calling QuitProgressUI\n"));
  QuitProgressUI();
}

int NS_main(int argc, NS_tchar **argv)
{
#ifndef WINCE
  InitProgressUI(&argc, &argv);
#endif
  
  
  
  
  
  

  if (argc < 2) {
    fprintf(stderr, "Usage: updater <dir-path> [parent-pid [working-dir callback args...]]\n");
    return 1;
  }

  if (argc > 2 ) {
    int pid = NS_tatoi(argv[2]);
    if (pid) {
#ifdef XP_WIN
      HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, (DWORD) pid);
      
      
      
      if (parent) {
        DWORD result = WaitForSingleObject(parent, 5000);
        CloseHandle(parent);
        if (result != WAIT_OBJECT_0)
          return 1;
        
        
        Sleep(50);
      }
#else
      int status;
      waitpid(pid, &status, 0);
#endif
    }
  }

#ifdef XP_WIN
  
  
  HANDLE updateLockFileHandle;
  NS_tchar elevatedLockFilePath[MAXPATHLEN];
  if (argc > 4) {
    NS_tchar updateLockFilePath[MAXPATHLEN];
    NS_tsnprintf(updateLockFilePath, MAXPATHLEN,
                 NS_T("%s.update_in_progress.lock"), argv[4]);

    
    
    
    if (!_waccess(updateLockFilePath, F_OK) &&
        NS_tremove(updateLockFilePath) != 0) {
      fprintf(stderr, "Update already in progress! Exiting\n");
      return 1;
    }

    updateLockFileHandle = CreateFileW(updateLockFilePath,
                                       GENERIC_READ | GENERIC_WRITE,
                                       0,
                                       NULL,
                                       OPEN_ALWAYS,
                                       FILE_FLAG_DELETE_ON_CLOSE,
                                       NULL);

    NS_tsnprintf(elevatedLockFilePath, MAXPATHLEN,
                 NS_T("%s/update_elevated.lock"), argv[1]);

    if (updateLockFileHandle == INVALID_HANDLE_VALUE) {
      if (!_waccess(elevatedLockFilePath, F_OK) &&
          NS_tremove(elevatedLockFilePath) != 0) {
        fprintf(stderr, "Update already elevated! Exiting\n");
        return 1;
      }

      HANDLE elevatedFileHandle;
      elevatedFileHandle = CreateFileW(elevatedLockFilePath,
                                       GENERIC_READ | GENERIC_WRITE,
                                       0,
                                       NULL,
                                       OPEN_ALWAYS,
                                       FILE_FLAG_DELETE_ON_CLOSE,
                                       NULL);

      if (elevatedFileHandle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Unable to create elevated lock file! Exiting\n");
        return 1;
      }

      PRUnichar *cmdLine = MakeCommandLine(argc - 1, argv + 1);
      if (!cmdLine) {
        CloseHandle(elevatedFileHandle);
        return 1;
      }

      SHELLEXECUTEINFO sinfo;
      memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
      sinfo.cbSize       = sizeof(SHELLEXECUTEINFO);
      sinfo.fMask        = SEE_MASK_FLAG_NO_UI |
#ifndef WINCE
                           SEE_MASK_FLAG_DDEWAIT |
#endif
                           SEE_MASK_NOCLOSEPROCESS;
      sinfo.hwnd         = NULL;
      sinfo.lpFile       = argv[0];
      sinfo.lpParameters = cmdLine;
      sinfo.lpVerb       = L"runas";
      sinfo.nShow        = SW_SHOWNORMAL;

      BOOL result = ShellExecuteEx(&sinfo);
      free(cmdLine);

      if (result) {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
      }

      if (argc > 4)
        LaunchCallbackApp(argv[3], argc - 4, argv + 4);

      CloseHandle(elevatedFileHandle);
      return 0;
    }
  }
#endif

  gSourcePath = argv[1];

  LogInit();

  
  
  
  Thread t;
  if (t.Run(UpdateThreadFunc, NULL) == 0)
    ShowProgressUI();
  t.Join();

  LogFinish();

#ifdef XP_WIN
  if (gSucceeded && argc > 4)
    LaunchWinPostProcess(argv[4]);

  if (argc > 4) {
    CloseHandle(updateLockFileHandle);
    
    
    if (!_waccess(elevatedLockFilePath, F_OK) &&
        NS_tremove(elevatedLockFilePath) != 0)
      return 0;
  }
#endif

  
  
  
  if (argc > 4)
    LaunchCallbackApp(argv[3], argc - 4, argv + 4);

  return 0;
}

class ActionList
{
public:
  ActionList() : mFirst(NULL), mLast(NULL), mCount(0) { }
  ~ActionList();

  void Append(Action* action);
  int Prepare();
  int Execute();
  void Finish(int status);

private:
  Action *mFirst;
  Action *mLast;
  int     mCount;
};

ActionList::~ActionList()
{
  Action* a = mFirst;
  while (a) {
    Action *b = a;
    a = a->mNext;
    delete b;
  }
}

void
ActionList::Append(Action *action)
{
  if (mLast)
    mLast->mNext = action;
  else
    mFirst = action;

  mLast = action;
  mCount++;
}

int
ActionList::Prepare()
{
  
  
  
  if (mCount == 0) {
    LOG(("empty action list\n"));
    return UNEXPECTED_ERROR;
  }

  Action *a = mFirst;
  while (a) {
    int rv = a->Prepare();
    if (rv)
      return rv;

    a = a->mNext;
  }

  UpdateProgressUI(1.0f);

  return OK;
}

int
ActionList::Execute()
{
  int i = 0;
  float divisor = mCount / 98.0f;

  Action *a = mFirst;
  while (a) {
    UpdateProgressUI(1.0f + float(i++) / divisor);

    int rv = a->Execute();
    if (rv)
    {
      LOG(("### execution failed\n"));
      return rv;
    }

    a = a->mNext;
  }

  return OK;
}

void
ActionList::Finish(int status)
{
  Action *a = mFirst;
  while (a) {
    a->Finish(status);
    a = a->mNext;
  }

#ifdef XP_WIN
  if (status == OK)
    gSucceeded = TRUE;
#endif

  UpdateProgressUI(100.0f);
}

int DoUpdate()
{
  NS_tchar manifest[MAXPATHLEN];
  NS_tsnprintf(manifest, MAXPATHLEN, NS_T("%s/update.manifest"), gSourcePath);

  
  FILE *fp = NS_tfopen(manifest, NS_T("wb"));
  if (!fp)
    return READ_ERROR;

  int rv = gArchiveReader.ExtractFileToStream("update.manifest", fp);
  fclose(fp);
  if (rv)
    return rv;

  AutoFile mfile = NS_tfopen(manifest, NS_T("rb"));
  if (mfile == NULL)
    return READ_ERROR;

  struct stat ms;
  rv = fstat(fileno(mfile), &ms);
  if (rv)
    return READ_ERROR;

  char *mbuf = (char*) malloc(ms.st_size + 1);
  if (!mbuf)
    return MEM_ERROR;

  int r = ms.st_size;
  char *rb = mbuf;
  while (r) {
    int c = fread(rb, 1, mmin(SSIZE_MAX,r), mfile);
    if (c < 0)
      return READ_ERROR;

    r -= c;
    rb += c;

    if (c == 0 && r)
      return UNEXPECTED_ERROR;
  }
  mbuf[ms.st_size] = '\0';

  ActionList list;

  rb = mbuf;
  char *line;
  while((line = mstrtok(kNL, &rb)) != 0) {
    
    if (*line == '#')
      continue;

    char *token = mstrtok(kWhitespace, &line);
    if (!token)
      return PARSE_ERROR;

    Action *action = NULL;
    if (strcmp(token, "remove") == 0) {
      action = new RemoveFile();
    }
    else if (strcmp(token, "add") == 0) {
      action = new AddFile();
    }
    else if (strcmp(token, "patch") == 0) {
      action = new PatchFile();
    }
    else if (strcmp(token, "add-if") == 0) {
      action = new AddIfFile();
    }
    else if (strcmp(token, "patch-if") == 0) {
      action = new PatchIfFile();
    }
    else {
      return PARSE_ERROR;
    }

    if (!action)
      return MEM_ERROR;

    rv = action->Parse(line);
    if (rv)
      return rv;

    list.Append(action);
  }

  rv = list.Prepare();
  if (rv)
    return rv;

  rv = list.Execute();

  list.Finish(rv);
  return rv;
}
