
















































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

#if defined(XP_WIN)
# include <windows.h>
# include <direct.h>
# include <io.h>
# define F_OK 00
# define W_OK 02
# define R_OK 04
# define access _access
# define snprintf _snprintf
# define putenv _putenv
# define fchmod(a,b)
# define mkdir(path, perm) _mkdir(path)
# define chdir(path) _chdir(path)
#else
# include <sys/wait.h>
# include <unistd.h>
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
# ifdef MAX_PATH
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





extern "C" unsigned int BZ2_crc32Table[256];

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





class AutoFD
{
public:
  AutoFD(int fd = -1)
    : mFD(fd) {
  }

  ~AutoFD() {
    if (mFD != -1)
      close(mFD);
  }

  AutoFD &operator=(int fd) {
    if (mFD != -1)
      close(mFD);
    mFD = fd;
    return *this;
  }

  operator int() {
    return mFD;
  }

private:
  int mFD;
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

    unsigned threadID;
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



static char* gSourcePath;
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

  char logFile[MAXPATHLEN];
  snprintf(logFile, MAXPATHLEN, "%s/update.log", gSourcePath);

  gLogFP = fopen(logFile, "w");
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
  (void)_chmod(path, _S_IREAD | _S_IWRITE);
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

static int ensure_open(const char *path, int flags, int options)
{
  ensure_write_permissions(path);
  return open(path, flags, options);
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

  AutoFD sfd = open(spath, O_RDONLY | _O_BINARY);
  if (sfd < 0 || fstat(sfd, &ss)) {
    LOG(("copy_file: failed to open or stat: %d,%s,%d\n", (int) sfd, spath, errno));
    return READ_ERROR;
  }

  AutoFD dfd = ensure_open(dpath, O_WRONLY | O_TRUNC | O_CREAT | _O_BINARY, ss.st_mode);
  if (dfd < 0) {
    LOG(("copy_file: failed to open: %s,%d\n", dpath, errno));
    return WRITE_ERROR;
  }

  char buf[BUFSIZ];
  int sc;
  while ((sc = read(sfd, buf, sizeof(buf))) > 0) {
    int dc;
    char *bp = buf;
    while ((dc = write(dfd, bp, (unsigned int) sc)) > 0) {
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
  PatchFile() : mPatchIndex(-1), pfd(-1), buf(NULL) { }
  virtual ~PatchFile();

  virtual int Parse(char *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  int LoadSourceFile(int ofd);

  static int sPatchIndex;

  const char *mPatchFile;
  const char *mFile;
  int mPatchIndex;
  MBSPatchHeader header;
  int pfd;
  unsigned char *buf;
};

int PatchFile::sPatchIndex = 0;

PatchFile::~PatchFile()
{
  if (pfd >= 0)
    close(pfd);

  
  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%d.patch", gSourcePath, mPatchIndex);
  ensure_remove(spath);

  free(buf);
}

int
PatchFile::LoadSourceFile(int ofd)
{
  struct stat os;
  int rv = fstat(ofd, &os);
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
    int c = read(ofd, rb, mmin(BUFSIZ,r));
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

  char spath[MAXPATHLEN];
  snprintf(spath, MAXPATHLEN, "%s/%d.patch", gSourcePath, mPatchIndex);

  ensure_remove(spath);

  int rv = gArchiveReader.ExtractFile(mPatchFile, spath);
  if (rv)
    return rv;

  
  
  

  pfd = open(spath, O_RDONLY | _O_BINARY);
  if (pfd < 0)
    return READ_ERROR;

  rv = MBS_ReadHeader(pfd, &header);
  if (rv)
    return rv;

  AutoFD ofd = open(mFile, O_RDONLY | _O_BINARY);
  if (ofd < 0)
    return READ_ERROR;

  rv = LoadSourceFile(ofd);
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

  AutoFD ofd = ensure_open(mFile, O_WRONLY | O_TRUNC | O_CREAT | _O_BINARY, ss.st_mode);
  if (ofd < 0)
    return WRITE_ERROR;

  return MBS_ApplyPatch(&header, pfd, buf, ofd);
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
LaunchWinPostProcess(const char *appExe)
{
  
  
  char inifile[MAXPATHLEN];
  strcpy(inifile, appExe);

  char *slash = strrchr(inifile, '\\');
  if (!slash)
    return;

  strcpy(slash + 1, "updater.ini");

  char exefile[MAXPATHLEN];
  char exearg[MAXPATHLEN];
  if (!GetPrivateProfileString("PostUpdateWin", "ExeRelPath", NULL, exefile,
      sizeof(exefile), inifile))
    return;

  if (!GetPrivateProfileString("PostUpdateWin", "ExeArg", NULL, exearg,
      sizeof(exearg), inifile))
    return;

  char exefullpath[MAXPATHLEN];
  strcpy(exefullpath, appExe);

  slash = strrchr(exefullpath, '\\');
  strcpy(slash + 1, exefile);

  char dlogFile[MAXPATHLEN];
  strcpy(dlogFile, exefullpath);

  slash = strrchr(dlogFile, '\\');
  strcpy(slash + 1, "uninstall.update");

  char slogFile[MAXPATHLEN];
  snprintf(slogFile, MAXPATHLEN, "%s/update.log", gSourcePath);

  
  
  
  ensure_remove(dlogFile);
  copy_file(slogFile, dlogFile);

  static int    argc = 2;
  static char **argv = (char**) malloc(sizeof(char*) * (argc + 1));
  argv[0] = "argv0ignoredbywinlaunchchild";
  argv[1] = exearg;
  argv[2] = "\0";

  WinLaunchChild(exefullpath, argc, argv, 1);
  free(argv);
}
#endif

static void
LaunchCallbackApp(const char *workingDir, int argc, char **argv)
{
  putenv("NO_EM_RESTART=");
  putenv("MOZ_LAUNCHED_CHILD=1");

  
  chdir(workingDir);

#if defined(USE_EXECV)
  execv(argv[0], argv);
#elif defined(XP_MACOSX)
  LaunchChild(argc, argv);
#elif defined(XP_WIN)
  WinLaunchChild(argv[0], argc, argv, -1);
#else
# warning "Need implementaton of LaunchCallbackApp"
#endif
}

static void
WriteStatusFile(int status)
{
  

  char filename[MAXPATHLEN];
  snprintf(filename, MAXPATHLEN, "%s/update.status", gSourcePath);

  AutoFD fd = ensure_open(filename, O_WRONLY | O_TRUNC | O_CREAT | _O_BINARY, 0644);
  if (fd < 0)
    return;

  const char *text;

  char buf[32];
  if (status == OK) {
    text = "succeeded\n";
  } else {
    snprintf(buf, sizeof(buf), "failed: %d\n", status);
    text = buf;
  }
  write(fd, text, strlen(text));
}

static void
UpdateThreadFunc(void *param)
{
  

  char dataFile[MAXPATHLEN];
  snprintf(dataFile, MAXPATHLEN, "%s/update.mar", gSourcePath);

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

int main(int argc, char **argv)
{
  InitProgressUI(&argc, &argv);

  
  
  
  
  
  

  if (argc < 3) {
    fprintf(stderr, "Usage: updater <dir-path> <parent-pid> [working-dir callback args...]\n");
    return 1;
  }

  int pid = atoi(argv[2]);
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

  gSourcePath = argv[1];

#ifdef XP_WIN
  
  
  

  HANDLE exefile = NULL;

  if (argc > 5)
    exefile = CreateFile(argv[4], DELETE | GENERIC_WRITE,
                         0, 
                         NULL, OPEN_EXISTING, 0, NULL);
#endif

  LogInit();

  
  
  
  Thread t;
  if (t.Run(UpdateThreadFunc, NULL) == 0)
    ShowProgressUI();
  t.Join();

  LogFinish();

#ifdef XP_WIN
  if (exefile)
    CloseHandle(exefile);

  if (gSucceeded)
    LaunchWinPostProcess(argv[4]);
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
  char manifest[MAXPATHLEN];
  snprintf(manifest, MAXPATHLEN, "%s/update.manifest", gSourcePath);

  
  int rv = gArchiveReader.ExtractFile("update.manifest", manifest);
  if (rv)
    return rv;

  AutoFD mfd = open(manifest, O_RDONLY | _O_BINARY);
  if (mfd < 0)
    return READ_ERROR;

  struct stat ms;
  rv = fstat(mfd, &ms);
  if (rv)
    return READ_ERROR;

  char *mbuf = (char*) malloc(ms.st_size + 1);
  if (!mbuf)
    return MEM_ERROR;

  int r = ms.st_size;
  char *rb = mbuf;
  while (r) {
    int c = read(mfd, rb, mmin(SSIZE_MAX,r));
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

#if defined(XP_WIN) && !defined(DEBUG) && !defined(__GNUC__)


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
  
  return main(__argc, __argv);
}
#endif
