































#include "bspatch.h"
#include "progressui.h"
#include "archivereader.h"
#include "readstrings.h"
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

#include "updatelogging.h"



#define PROGRESS_PREPARE_SIZE 20.0f
#define PROGRESS_EXECUTE_SIZE 75.0f
#define PROGRESS_FINISH_SIZE   5.0f

#if defined(XP_MACOSX)

void LaunchChild(int argc, char **argv);
void LaunchMacPostProcess(const char* aAppExe);
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



#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_EXECV
#endif

#if defined(MOZ_WIDGET_GONK)
# include "automounter_gonk.h"
#endif

#ifdef XP_WIN
#include "updatehelper.h"




#define EXIT_WHEN_ELEVATED(path, handle, retCode) \
  { \
      if (handle != INVALID_HANDLE_VALUE) { \
        CloseHandle(handle); \
      } \
      if (_waccess(path, F_OK) == 0 && NS_tremove(path) != 0) { \
        return retCode; \
      } \
  }
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

  FILE* get() {
    return mFile;
  }

private:
  FILE* mFile;
};

struct MARChannelStringTable {
  MARChannelStringTable() 
  {
    MARChannelID[0] = '\0';
  }

  char MARChannelID[MAX_TEXT_LEN];
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

    unsigned int threadID;

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
static NS_tchar gDestinationPath[MAXPATHLEN];
static ArchiveReader gArchiveReader;
static bool gSucceeded = false;
static bool sBackgroundUpdate = false;
static bool sReplaceRequest = false;
static bool sUsingService = false;

#ifdef XP_WIN

static NS_tchar* gDestPath;
static NS_tchar gCallbackRelPath[MAXPATHLEN];
static NS_tchar gCallbackBackupPath[MAXPATHLEN];
#endif

static const NS_tchar kWhitespace[] = NS_T(" \t");
static const NS_tchar kNL[] = NS_T("\r\n");
static const NS_tchar kQuote[] = NS_T("\"");

static inline size_t
mmin(size_t a, size_t b)
{
  return (a > b) ? b : a;
}

static NS_tchar*
mstrtok(const NS_tchar *delims, NS_tchar **str)
{
  if (!*str || !**str)
    return NULL;

  
  NS_tchar *ret = *str;
  const NS_tchar *d;
  do {
    for (d = delims; *d != NS_T('\0'); ++d) {
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

  NS_tchar *i = ret;
  do {
    for (d = delims; *d != NS_T('\0'); ++d) {
      if (*i == *d) {
        *i = NS_T('\0');
        *str = ++i;
        return ret;
      }
    }
    ++i;
  } while (*i);

  *str = NULL;
  return ret;
}

#ifdef XP_WIN







static NS_tchar*
get_full_path(const NS_tchar *relpath)
{
  size_t lendestpath = NS_tstrlen(gDestPath);
  size_t lenrelpath = NS_tstrlen(relpath);
  NS_tchar *s = (NS_tchar *) malloc((lendestpath + lenrelpath + 1) * sizeof(NS_tchar));
  if (!s)
    return NULL;

  NS_tchar *c = s;

  NS_tstrcpy(c, gDestPath);
  c += lendestpath;
  NS_tstrcat(c, relpath);
  c += lenrelpath;
  *c = NS_T('\0');
  c++;
  return s;
}
#endif











static NS_tchar*
get_valid_path(NS_tchar **line, bool isdir = false)
{
  NS_tchar *path = mstrtok(kQuote, line);
  if (!path) {
    LOG(("get_valid_path: unable to determine path: " LOG_S "\n", line));
    return NULL;
  }

  
  if (path[0] == NS_T('/')) {
    LOG(("get_valid_path: path must be relative: " LOG_S "\n", path));
    return NULL;
  }

#ifdef XP_WIN
  
  if (path[0] == NS_T('\\') || path[1] == NS_T(':')) {
    LOG(("get_valid_path: path must be relative: " LOG_S "\n", path));
    return NULL;
  }
#endif

  if (isdir) {
    
    if (path[NS_tstrlen(path) - 1] != NS_T('/')) {
      LOG(("get_valid_path: directory paths must have a trailing forward " \
           "slash: " LOG_S "\n", path));
      return NULL;
    }

    
    
    path[NS_tstrlen(path) - 1] = NS_T('\0');
  }

  
  if (NS_tstrstr(path, NS_T("..")) != NULL) {
    LOG(("get_valid_path: paths must not contain '..': " LOG_S "\n", path));
    return NULL;
  }

  return path;
}

static NS_tchar*
get_quoted_path(const NS_tchar *path)
{
  size_t lenQuote = NS_tstrlen(kQuote);
  size_t lenPath = NS_tstrlen(path);
  size_t len = lenQuote + lenPath + lenQuote + 1;

  NS_tchar *s = (NS_tchar *) malloc(len * sizeof(NS_tchar));
  if (!s)
    return NULL;

  NS_tchar *c = s;
  NS_tstrcpy(c, kQuote);
  c += lenQuote;
  NS_tstrcat(c, path);
  c += lenPath;
  NS_tstrcat(c, kQuote);
  c += lenQuote;
  *c = NS_T('\0');
  c++;
  return s;
}

static void ensure_write_permissions(const NS_tchar *path)
{
#ifdef XP_WIN
  (void) _wchmod(path, _S_IREAD | _S_IWRITE);
#else
  struct stat fs;
  if (!stat(path, &fs) && !(fs.st_mode & S_IWUSR)) {
    (void)chmod(path, fs.st_mode | S_IWUSR);
  }
#endif
}

static int ensure_remove(const NS_tchar *path)
{
  ensure_write_permissions(path);
  int rv = NS_tremove(path);
  if (rv)
    LOG(("ensure_remove: failed to remove file: " LOG_S ", rv: %d, err: %d\n",
         path, rv, errno));
  return rv;
}


static int ensure_remove_recursive(const NS_tchar *path)
{
  
  
  struct stat sInfo;
  int rv = NS_tlstat(path, &sInfo);
  if (rv) {
    
    return rv;
  }
  if (!S_ISDIR(sInfo.st_mode)) {
    return ensure_remove(path);
  }

  NS_tDIR *dir;
  NS_tdirent *entry;

  dir = NS_topendir(path);
  if (!dir) {
    LOG(("ensure_remove_recursive: path is not a directory: " LOG_S ", rv: %d, err: %d\n",
          path, rv, errno));
    return rv;
  }

  while ((entry = NS_treaddir(dir)) != 0) {
    if (NS_tstrcmp(entry->d_name, NS_T(".")) &&
        NS_tstrcmp(entry->d_name, NS_T(".."))) {
      NS_tchar childPath[MAXPATHLEN];
      NS_tsnprintf(childPath, sizeof(childPath)/sizeof(childPath[0]),
                   NS_T("%s/%s"), path, entry->d_name);
      rv = ensure_remove_recursive(childPath);
      if (rv) {
        break;
      }
    }
  }

  NS_tclosedir(dir);

  if (rv == OK) {
    ensure_write_permissions(path);
    rv = NS_trmdir(path);
    if (rv) {
      LOG(("ensure_remove_recursive: path is not a directory: " LOG_S ", rv: %d, err: %d\n",
            path, rv, errno));
    }
  }
  return rv;
}

static bool is_read_only(const NS_tchar *flags)
{
  size_t length = NS_tstrlen(flags);
  if (length == 0)
    return false;

  
  if (flags[0] != NS_T('r'))
    return false;

  
  if (length > 1 && flags[1] == NS_T('+'))
    return false;

  
  if (NS_tstrcmp(flags, NS_T("rb+")) == 0)
    return false;

  return true;
}

static FILE* ensure_open(const NS_tchar *path, const NS_tchar *flags, unsigned int options)
{
  ensure_write_permissions(path);
  FILE* f = NS_tfopen(path, flags);
  if (is_read_only(flags)) {
    
    
    return f;
  }
  if (NS_tchmod(path, options) != 0) {
    if (f != NULL) {
      fclose(f);
    }
    return NULL;
  }
  struct stat ss;
  if (NS_tstat(path, &ss) != 0 || ss.st_mode != options) {
    if (f != NULL) {
      fclose(f);
    }
    return NULL;
  }
  return f;
}


static int ensure_parent_dir(const NS_tchar *path)
{
  int rv = OK;

  NS_tchar *slash = (NS_tchar *) NS_tstrrchr(path, NS_T('/'));
  if (slash) {
    *slash = NS_T('\0');
    rv = ensure_parent_dir(path);
    
    if (rv == OK && *path) {
      rv = NS_tmkdir(path, 0755);
      
      if (rv < 0 && errno != EEXIST) {
        LOG(("ensure_parent_dir: failed to create directory: " LOG_S ", " \
             "err: %d\n", path, errno));
        rv = WRITE_ERROR;
      } else {
        rv = OK;
      }
    }
    *slash = NS_T('/');
  }
  return rv;
}

#ifdef XP_UNIX
static int ensure_copy_symlink(const NS_tchar *path, const NS_tchar *dest)
{
  
  NS_tchar target[MAXPATHLEN + 1] = {NS_T('\0')};
  int rv = readlink(path, target, MAXPATHLEN);
  if (rv == -1) {
    LOG(("ensure_copy_symlink: failed to read the link: " LOG_S ", err: %d\n",
         path, errno));
    return READ_ERROR;
  }
  rv = symlink(target, dest);
  if (rv == -1) {
    LOG(("ensure_copy_symlink: failed to create the new link: " LOG_S ", target: " LOG_S " err: %d\n",
         dest, target, errno));
    return READ_ERROR;
  }
  return 0;
}
#endif


static int ensure_copy(const NS_tchar *path, const NS_tchar *dest)
{
#ifdef XP_WIN
  
  bool result = CopyFileW(path, dest, false);
  if (!result) {
    LOG(("ensure_copy: failed to copy the file " LOG_S " over to " LOG_S ", lasterr: %x\n",
         path, dest, GetLastError()));
    return WRITE_ERROR;
  }
  return 0;
#else
  struct stat ss;
  int rv = NS_tlstat(path, &ss);
  if (rv) {
    LOG(("ensure_copy: failed to read file status info: " LOG_S ", err: %d\n",
          path, errno));
    return READ_ERROR;
  }

#ifdef XP_UNIX
  if (S_ISLNK(ss.st_mode)) {
    return ensure_copy_symlink(path, dest);
  }
#endif

  AutoFile infile = ensure_open(path, NS_T("rb"), ss.st_mode);
  if (!infile) {
    LOG(("ensure_copy: failed to open the file for reading: " LOG_S ", err: %d\n",
          path, errno));
    return READ_ERROR;
  }
  AutoFile outfile = ensure_open(dest, NS_T("wb"), ss.st_mode);
  if (!outfile) {
    LOG(("ensure_copy: failed to open the file for writing: " LOG_S ", err: %d\n",
          dest, errno));
    return WRITE_ERROR;
  }

  void* buffer = malloc(ss.st_size);
  if (!buffer)
    return UPDATER_MEM_ERROR;

  size_t left = ss.st_size;
  while (left) {
    size_t read = fread(buffer, 1, left, infile);
    if (ferror(infile.get())) {
      LOG(("ensure_copy: failed to read the file: " LOG_S ", err: %d\n",
           path, errno));
      free(buffer);
      return READ_ERROR;
    }

    left -= read;
    size_t written = 0;

    while (written < read) {
      size_t chunkWritten = fwrite(buffer, 1, read - written, outfile);
      if (chunkWritten <= 0) {
        LOG(("ensure_copy: failed to write the file: " LOG_S ", err: %d\n",
             dest, errno));
        free(buffer);
        return WRITE_ERROR;
      }

      written += chunkWritten;
    }
  }

  rv = NS_tchmod(dest, ss.st_mode);

  free(buffer);
  return rv;
#endif
}

template <unsigned N>
struct copy_recursive_skiplist {
  NS_tchar paths[N][MAXPATHLEN];

  void append(unsigned index, const NS_tchar *path, const NS_tchar *suffix) {
    NS_tsnprintf(paths[index], MAXPATHLEN, NS_T("%s/%s"), path, suffix);
  }
  bool find(const NS_tchar *path) {
    for (unsigned i = 0; i < N; ++i) {
      if (!NS_tstricmp(paths[i], path)) {
        return true;
      }
    }
    return false;
  }
};



template <unsigned N>
static int ensure_copy_recursive(const NS_tchar *path, const NS_tchar *dest,
                                 copy_recursive_skiplist<N>& skiplist)
{
  struct stat sInfo;
  int rv = NS_tlstat(path, &sInfo);
  if (rv) {
    LOG(("ensure_copy_recursive: path doesn't exist: " LOG_S ", rv: %d, err: %d\n",
          path, rv, errno));
    return READ_ERROR;
  }

#ifdef XP_UNIX
  if (S_ISLNK(sInfo.st_mode)) {
    return ensure_copy_symlink(path, dest);
  }
#endif

  if (!S_ISDIR(sInfo.st_mode)) {
    return ensure_copy(path, dest);
  }

  rv = NS_tmkdir(dest, sInfo.st_mode);
  if (rv < 0 && errno != EEXIST) {
    LOG(("ensure_copy_recursive: could not create destination directory: " LOG_S ", rv: %d, err: %d\n",
         path, rv, errno));
    return WRITE_ERROR;
  }

  NS_tDIR *dir;
  NS_tdirent *entry;

  dir = NS_topendir(path);
  if (!dir) {
    LOG(("ensure_copy_recursive: path is not a directory: " LOG_S ", rv: %d, err: %d\n",
          path, rv, errno));
    return READ_ERROR;
  }

  while ((entry = NS_treaddir(dir)) != 0) {
    if (NS_tstrcmp(entry->d_name, NS_T(".")) &&
        NS_tstrcmp(entry->d_name, NS_T(".."))) {
      NS_tchar childPath[MAXPATHLEN];
      NS_tsnprintf(childPath, sizeof(childPath)/sizeof(childPath[0]),
                   NS_T("%s/%s"), path, entry->d_name);
      if (skiplist.find(childPath)) {
        continue;
      }
      NS_tchar childPathDest[MAXPATHLEN];
      NS_tsnprintf(childPathDest, sizeof(childPathDest)/sizeof(childPathDest[0]),
                   NS_T("%s/%s"), dest, entry->d_name);
      rv = ensure_copy_recursive(childPath, childPathDest, skiplist);
      if (rv) {
        break;
      }
    }
  }

  return rv;
}



static int rename_file(const NS_tchar *spath, const NS_tchar *dpath,
                       bool allowDirs = false)
{
  int rv = ensure_parent_dir(dpath);
  if (rv)
    return rv;

  struct stat spathInfo;
  rv = NS_tstat(spath, &spathInfo);
  if (rv) {
    LOG(("rename_file: failed to read file status info: " LOG_S ", " \
         "err: %d\n", spath, errno));
    return READ_ERROR;
  }

  if (!S_ISREG(spathInfo.st_mode)) {
    if (allowDirs && !S_ISDIR(spathInfo.st_mode)) {
      LOG(("rename_file: path present, but not a file: " LOG_S ", err: %d\n",
           spath, errno));
      return UNEXPECTED_FILE_OPERATION_ERROR;
    } else {
      LOG(("rename_file: proceeding to rename the directory\n"));
    }
  }

  if (!NS_taccess(dpath, F_OK)) {
    if (ensure_remove(dpath)) {
      LOG(("rename_file: destination file exists and could not be " \
           "removed: " LOG_S "\n", dpath));
      return WRITE_ERROR;
    }
  }

  if (NS_trename(spath, dpath) != 0) {
    LOG(("rename_file: failed to rename file - src: " LOG_S ", " \
         "dst:" LOG_S ", err: %d\n", spath, dpath, errno));
    return WRITE_ERROR;
  }

  return OK;
}




static int backup_create(const NS_tchar *path)
{
  NS_tchar backup[MAXPATHLEN];
  NS_tsnprintf(backup, sizeof(backup)/sizeof(backup[0]),
               NS_T("%s") BACKUP_EXT, path);

  return rename_file(path, backup);
}



static int backup_restore(const NS_tchar *path)
{
  NS_tchar backup[MAXPATHLEN];
  NS_tsnprintf(backup, sizeof(backup)/sizeof(backup[0]),
               NS_T("%s") BACKUP_EXT, path);

  if (NS_taccess(backup, F_OK)) {
    LOG(("backup_restore: backup file doesn't exist: " LOG_S "\n", backup));
    return OK;
  }

  return rename_file(backup, path);
}


static int backup_discard(const NS_tchar *path)
{
  NS_tchar backup[MAXPATHLEN];
  NS_tsnprintf(backup, sizeof(backup)/sizeof(backup[0]),
               NS_T("%s") BACKUP_EXT, path);

  
  if (NS_taccess(backup, F_OK)) {
    return OK;
  }

  int rv = ensure_remove(backup);
#if defined(XP_WIN)
  if (rv && !sBackgroundUpdate && !sReplaceRequest) {
    LOG(("backup_discard: unable to remove: " LOG_S "\n", backup));
    NS_tchar path[MAXPATHLEN];
    GetTempFileNameW(DELETE_DIR, L"moz", 0, path);
    if (rename_file(backup, path)) {
      LOG(("backup_discard: failed to rename file:" LOG_S ", dst:" LOG_S "\n",
           backup, path));
      return WRITE_ERROR;
    }
    
    
    
    
    
    if (MoveFileEx(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
      LOG(("backup_discard: file renamed and will be removed on OS " \
           "reboot: " LOG_S "\n", path));
    } else {
      LOG(("backup_discard: failed to schedule OS reboot removal of " \
           "file: " LOG_S "\n", path));
    }
  }
#else
  if (rv)
    return WRITE_ERROR;
#endif

  return OK;
}


static void backup_finish(const NS_tchar *path, int status)
{
  if (status == OK)
    backup_discard(path);
  else
    backup_restore(path);
}



static int DoUpdate();

class Action
{
public:
  Action() : mProgressCost(1), mNext(NULL) { }
  virtual ~Action() { }

  virtual int Parse(NS_tchar *line) = 0;

  
  
  virtual int Prepare() = 0;

  
  
  
  virtual int Execute() = 0;

  
  
  virtual void Finish(int status) = 0;

  int mProgressCost;
private:
  Action* mNext;

  friend class ActionList;
};

class RemoveFile : public Action
{
public:
  RemoveFile() : mFile(NULL), mSkip(0) { }

  int Parse(NS_tchar *line);
  int Prepare();
  int Execute();
  void Finish(int status);

private:
  const NS_tchar *mFile;
  int mSkip;
};

int
RemoveFile::Parse(NS_tchar *line)
{
  

  mFile = get_valid_path(&line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
RemoveFile::Prepare()
{
  
  int rv = NS_taccess(mFile, F_OK);
  if (rv) {
    mSkip = 1;
    mProgressCost = 0;
    return OK;
  }

  LOG(("PREPARE REMOVEFILE " LOG_S "\n", mFile));

  
  struct stat fileInfo;
  rv = NS_tstat(mFile, &fileInfo);
  if (rv) {
    LOG(("failed to read file status info: " LOG_S ", err: %d\n", mFile,
         errno));
    return READ_ERROR;
  }

  if (!S_ISREG(fileInfo.st_mode)) {
    LOG(("path present, but not a file: " LOG_S "\n", mFile));
    return UNEXPECTED_FILE_OPERATION_ERROR;
  }

  NS_tchar *slash = (NS_tchar *) NS_tstrrchr(mFile, NS_T('/'));
  if (slash) {
    *slash = NS_T('\0');
    rv = NS_taccess(mFile, W_OK);
    *slash = NS_T('/');
  } else {
    rv = NS_taccess(NS_T("."), W_OK);
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
  if (mSkip)
    return OK;

  LOG(("EXECUTE REMOVEFILE " LOG_S "\n", mFile));

  
  
  int rv = NS_taccess(mFile, F_OK);
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

  return OK;
}

void
RemoveFile::Finish(int status)
{
  if (mSkip)
    return;

  LOG(("FINISH REMOVEFILE " LOG_S "\n", mFile));

  backup_finish(mFile, status);
}

class RemoveDir : public Action
{
public:
  RemoveDir() : mDir(NULL), mSkip(0) { }

  virtual int Parse(NS_tchar *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  const NS_tchar *mDir;
  int mSkip;
};

int
RemoveDir::Parse(NS_tchar *line)
{
  

  mDir = get_valid_path(&line, true);
  if (!mDir)
    return PARSE_ERROR;

  return OK;
}

int
RemoveDir::Prepare()
{
  
  int rv = NS_taccess(mDir, F_OK);
  if (rv) {
    mSkip = 1;
    mProgressCost = 0;
    return OK;
  }

  LOG(("PREPARE REMOVEDIR " LOG_S "/\n", mDir));

  
  struct stat dirInfo;
  rv = NS_tstat(mDir, &dirInfo);
  if (rv) {
    LOG(("failed to read directory status info: " LOG_S ", err: %d\n", mDir,
         errno));
    return READ_ERROR;
  }

  if (!S_ISDIR(dirInfo.st_mode)) {
    LOG(("path present, but not a directory: " LOG_S "\n", mDir));
    return UNEXPECTED_FILE_OPERATION_ERROR;
  }

  rv = NS_taccess(mDir, W_OK);
  if (rv) {
    LOG(("access failed: %d, %d\n", rv, errno));
    return WRITE_ERROR;
  }

  return OK;
}

int
RemoveDir::Execute()
{
  if (mSkip)
    return OK;

  LOG(("EXECUTE REMOVEDIR " LOG_S "/\n", mDir));

  
  
  int rv = NS_taccess(mDir, F_OK);
  if (rv) {
    LOG(("directory no longer exists; skipping\n"));
    mSkip = 1;
  }

  return OK;
}

void
RemoveDir::Finish(int status)
{
  if (mSkip || status != OK)
    return;

  LOG(("FINISH REMOVEDIR " LOG_S "/\n", mDir));

  
  
  int rv = NS_taccess(mDir, F_OK);
  if (rv) {
    LOG(("directory no longer exists; skipping\n"));
    return;
  }


  if (status == OK) {
    if (NS_trmdir(mDir)) {
      LOG(("non-fatal error removing directory: " LOG_S "/, rv: %d, err: %d\n",
           mDir, rv, errno));
    }
  }
}

class AddFile : public Action
{
public:
  AddFile() : mFile(NULL)
            , mAdded(false)
            { }

  virtual int Parse(NS_tchar *line);
  virtual int Prepare();
  virtual int Execute();
  virtual void Finish(int status);

private:
  const NS_tchar *mFile;
  bool mAdded;
};

int
AddFile::Parse(NS_tchar *line)
{
  

  mFile = get_valid_path(&line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
AddFile::Prepare()
{
  LOG(("PREPARE ADD " LOG_S "\n", mFile));

  return OK;
}

int
AddFile::Execute()
{
  LOG(("EXECUTE ADD " LOG_S "\n", mFile));

  int rv;

  
  rv = NS_taccess(mFile, F_OK);
  if (rv == 0) {
    rv = backup_create(mFile);
    if (rv)
      return rv;
  } else {
    rv = ensure_parent_dir(mFile);
    if (rv)
      return rv;
  }

#ifdef XP_WIN
  char sourcefile[MAXPATHLEN];
  if (!WideCharToMultiByte(CP_UTF8, 0, mFile, -1, sourcefile, MAXPATHLEN,
                           NULL, NULL)) {
    LOG(("error converting wchar to utf8: %d\n", GetLastError()));
    return STRING_CONVERSION_ERROR;
  }

  rv = gArchiveReader.ExtractFile(sourcefile, mFile);
#else
  rv = gArchiveReader.ExtractFile(mFile, mFile);
#endif
  if (!rv) {
    mAdded = true;
  }
  return rv;
}

void
AddFile::Finish(int status)
{
  LOG(("FINISH ADD " LOG_S "\n", mFile));
  
  
  if (status && mAdded)
    NS_tremove(mFile);
  backup_finish(mFile, status);
}

class PatchFile : public Action
{
public:
  PatchFile() : mPatchIndex(-1), buf(NULL) { }

  virtual ~PatchFile();

  virtual int Parse(NS_tchar *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  int LoadSourceFile(FILE* ofile);

  static int sPatchIndex;

  const NS_tchar *mPatchFile;
  const NS_tchar *mFile;
  int mPatchIndex;
  MBSPatchHeader header;
  unsigned char *buf;
  NS_tchar spath[MAXPATHLEN];
};

int PatchFile::sPatchIndex = 0;

PatchFile::~PatchFile()
{
  
  if (spath[0])
    NS_tremove(spath);

  if (buf)
    free(buf);
}

int
PatchFile::LoadSourceFile(FILE* ofile)
{
  struct stat os;
  int rv = fstat(fileno((FILE *)ofile), &os);
  if (rv) {
    LOG(("LoadSourceFile: unable to stat destination file: " LOG_S ", " \
         "err: %d\n", mFile, errno));
    return READ_ERROR;
  }

  if (uint32_t(os.st_size) != header.slen) {
    LOG(("LoadSourceFile: destination file size %d does not match expected size %d\n",
         uint32_t(os.st_size), header.slen));
    return UNEXPECTED_FILE_OPERATION_ERROR;
  }

  buf = (unsigned char *) malloc(header.slen);
  if (!buf)
    return UPDATER_MEM_ERROR;

  size_t r = header.slen;
  unsigned char *rb = buf;
  while (r) {
    const size_t count = mmin(SSIZE_MAX, r);
    size_t c = fread(rb, 1, count, ofile);
    if (c != count) {
      LOG(("LoadSourceFile: error reading destination file: " LOG_S "\n",
           mFile));
      return READ_ERROR;
    }

    r -= c;
    rb += c;
  }

  

  unsigned int crc = crc32(buf, header.slen);

  if (crc != header.scrc32) {
    LOG(("LoadSourceFile: destination file crc %d does not match expected " \
         "crc %d\n", crc, header.scrc32));
    return CRC_ERROR;
  }

  return OK;
}

int
PatchFile::Parse(NS_tchar *line)
{
  

  
  mPatchFile = mstrtok(kQuote, &line);
  if (!mPatchFile)
    return PARSE_ERROR;

  
  NS_tchar *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  mFile = get_valid_path(&line);
  if (!mFile)
    return PARSE_ERROR;

  return OK;
}

int
PatchFile::Prepare()
{
  LOG(("PREPARE PATCH " LOG_S "\n", mFile));

  
  mPatchIndex = sPatchIndex++;

  NS_tsnprintf(spath, sizeof(spath)/sizeof(spath[0]),
               NS_T("%s/%d.patch"), gSourcePath, mPatchIndex);

  NS_tremove(spath);

  FILE *fp = NS_tfopen(spath, NS_T("wb"));
  if (!fp)
    return WRITE_ERROR;

#ifdef XP_WIN
  char sourcefile[MAXPATHLEN];
  if (!WideCharToMultiByte(CP_UTF8, 0, mPatchFile, -1, sourcefile, MAXPATHLEN,
                           NULL, NULL)) {
    LOG(("error converting wchar to utf8: %d\n", GetLastError()));
    return STRING_CONVERSION_ERROR;
  }

  int rv = gArchiveReader.ExtractFileToStream(sourcefile, fp);
#else
  int rv = gArchiveReader.ExtractFileToStream(mPatchFile, fp);
#endif
  fclose(fp);
  return rv;
}

int
PatchFile::Execute()
{
  LOG(("EXECUTE PATCH " LOG_S "\n", mFile));

  AutoFile pfile = NS_tfopen(spath, NS_T("rb"));
  if (pfile == NULL)
    return READ_ERROR;

  int rv = MBS_ReadHeader(pfile, &header);
  if (rv)
    return rv;

  FILE *origfile = NULL;
#ifdef XP_WIN
  if (NS_tstrcmp(mFile, gCallbackRelPath) == 0) {
    
    
    origfile = NS_tfopen(gCallbackBackupPath, NS_T("rb"));
  } else {
    origfile = NS_tfopen(mFile, NS_T("rb"));
  }
#else
  origfile = NS_tfopen(mFile, NS_T("rb"));
#endif

  if (!origfile) {
    LOG(("unable to open destination file: " LOG_S ", err: %d\n", mFile,
         errno));
    return READ_ERROR;
  }

  rv = LoadSourceFile(origfile);
  fclose(origfile);
  if (rv) {
    LOG(("LoadSourceFile failed\n"));
    return rv;
  }

  
  
  struct stat ss;
  rv = NS_tstat(mFile, &ss);
  if (rv) {
    LOG(("failed to read file status info: " LOG_S ", err: %d\n", mFile,
         errno));
    return READ_ERROR;
  }

  rv = backup_create(mFile);
  if (rv)
    return rv;

#if defined(HAVE_POSIX_FALLOCATE)
  AutoFile ofile = ensure_open(mFile, NS_T("wb+"), ss.st_mode);
  posix_fallocate(fileno((FILE *)ofile), 0, header.dlen);
#elif defined(XP_WIN)
  bool shouldTruncate = true;
  
  
  
  
  
  
  
  HANDLE hfile = CreateFileW(mFile,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

  if (hfile != INVALID_HANDLE_VALUE) {
    if (SetFilePointer(hfile, header.dlen, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
        SetEndOfFile(hfile) != 0) {
      shouldTruncate = false;
    }
    CloseHandle(hfile);
  }

  AutoFile ofile = ensure_open(mFile, shouldTruncate ? NS_T("wb+") : NS_T("rb+"), ss.st_mode);
#elif defined(XP_MACOSX)
  AutoFile ofile = ensure_open(mFile, NS_T("wb+"), ss.st_mode);
  
  fstore_t store = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, header.dlen};
  
  rv = fcntl(fileno((FILE *)ofile), F_PREALLOCATE, &store);
  if (rv == -1) {
    
    store.fst_flags = F_ALLOCATEALL;
    rv = fcntl(fileno((FILE *)ofile), F_PREALLOCATE, &store);
  }

  if (rv != -1) {
    ftruncate(fileno((FILE *)ofile), header.dlen);
  }
#else
  AutoFile ofile = ensure_open(mFile, NS_T("wb+"), ss.st_mode);
#endif

  if (ofile == NULL) {
    LOG(("unable to create new file: " LOG_S ", err: %d\n", mFile, errno));
    return WRITE_ERROR;
  }

#ifdef XP_WIN
  if (!shouldTruncate) {
    fseek(ofile, 0, SEEK_SET);
  }
#endif

  rv = MBS_ApplyPatch(&header, pfile, buf, ofile);

  
  
  
  pfile = NULL;
  NS_tremove(spath);
  spath[0] = NS_T('\0');
  free(buf);
  buf = NULL;

  return rv;
}

void
PatchFile::Finish(int status)
{
  LOG(("FINISH PATCH " LOG_S "\n", mFile));

  backup_finish(mFile, status);
}

class AddIfFile : public AddFile
{
public:
  AddIfFile() : mTestFile(NULL) { }

  virtual int Parse(NS_tchar *line);
  virtual int Prepare();
  virtual int Execute();
  virtual void Finish(int status);

protected:
  const NS_tchar *mTestFile;
};

int
AddIfFile::Parse(NS_tchar *line)
{
  

  mTestFile = get_valid_path(&line);
  if (!mTestFile)
    return PARSE_ERROR;

  
  NS_tchar *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  return AddFile::Parse(line);
}

int
AddIfFile::Prepare()
{
  
  if (NS_taccess(mTestFile, F_OK)) {
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

  virtual int Parse(NS_tchar *line);
  virtual int Prepare(); 
  virtual int Execute();
  virtual void Finish(int status);

private:
  const NS_tchar *mTestFile;
};

int
PatchIfFile::Parse(NS_tchar *line)
{
  

  mTestFile = get_valid_path(&line);
  if (!mTestFile)
    return PARSE_ERROR;

  
  NS_tchar *q = mstrtok(kQuote, &line);
  if (!q)
    return PARSE_ERROR;

  return PatchFile::Parse(line);
}

int
PatchIfFile::Prepare()
{
  
  if (NS_taccess(mTestFile, F_OK)) {
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
#include "uachelper.h"
#include "pathhash.h"
#endif

static void
LaunchCallbackApp(const NS_tchar *workingDir, 
                  int argc, 
                  NS_tchar **argv, 
                  bool usingService)
{
  putenv(const_cast<char*>("NO_EM_RESTART="));
  putenv(const_cast<char*>("MOZ_LAUNCHED_CHILD=1"));

  
  
  
  if (NS_tchdir(workingDir) != 0) {
    LOG(("Warning: chdir failed\n"));
  }

#if defined(USE_EXECV)
  execv(argv[0], argv);
#elif defined(XP_MACOSX)
  LaunchChild(argc, argv);
#elif defined(XP_WIN)
  
  
  if (!usingService) {
    WinLaunchChild(argv[0], argc, argv, NULL);
  }
#else
# warning "Need implementaton of LaunchCallbackApp"
#endif
}

static bool
WriteStatusFile(const char* aStatus)
{
  NS_tchar filename[MAXPATHLEN];
  NS_tsnprintf(filename, sizeof(filename)/sizeof(filename[0]),
               NS_T("%s/update.status"), gSourcePath);

  
  if (ensure_parent_dir(filename))
    return false;

  AutoFile file = NS_tfopen(filename, NS_T("wb+"));
  if (file == NULL)
    return false;

  if (fwrite(aStatus, strlen(aStatus), 1, file) != 1)
    return false;

  return true;
}

static void
WriteStatusFile(int status)
{
  const char *text;

  char buf[32];
  if (status == OK) {
    if (sBackgroundUpdate) {
      text = "applied\n";
    } else {
      text = "succeeded\n";
    }
  } else {
    snprintf(buf, sizeof(buf)/sizeof(buf[0]), "failed: %d\n", status);
    text = buf;
  }

  WriteStatusFile(text);
}

#ifdef MOZ_MAINTENANCE_SERVICE









static bool
IsUpdateStatusPendingService()
{
  NS_tchar filename[MAXPATHLEN];
  NS_tsnprintf(filename, sizeof(filename)/sizeof(filename[0]),
               NS_T("%s/update.status"), gSourcePath);

  AutoFile file = NS_tfopen(filename, NS_T("rb"));
  if (file == NULL)
    return false;

  char buf[32] = { 0 };
  fread(buf, sizeof(buf), 1, file);

  const char kPendingService[] = "pending-service";
  const char kAppliedService[] = "applied-service";

  return (strncmp(buf, kPendingService, 
                  sizeof(kPendingService) - 1) == 0) ||
         (strncmp(buf, kAppliedService,
                  sizeof(kAppliedService) - 1) == 0);
}
#endif

#ifdef XP_WIN








static bool
IsUpdateStatusSucceeded(bool &isSucceeded)
{
  isSucceeded = false;
  NS_tchar filename[MAXPATHLEN];
  NS_tsnprintf(filename, sizeof(filename)/sizeof(filename[0]),
               NS_T("%s/update.status"), gSourcePath);

  AutoFile file = NS_tfopen(filename, NS_T("rb"));
  if (file == NULL)
    return false;

  char buf[32] = { 0 };
  fread(buf, sizeof(buf), 1, file);

  const char kSucceeded[] = "succeeded";
  isSucceeded = strncmp(buf, kSucceeded, 
                        sizeof(kSucceeded) - 1) == 0;
  return true;
}
#endif







template <size_t N>
static bool
GetInstallationDir(NS_tchar (&installDir)[N])
{
  NS_tsnprintf(installDir, N, NS_T("%s"), gDestinationPath);
  if (!sBackgroundUpdate && !sReplaceRequest) {
    
    return true;
  }

  NS_tchar *slash = (NS_tchar *) NS_tstrrchr(installDir, NS_SLASH);
  
  if (slash && slash[1] == NS_T('\0')) {
    *slash = NS_T('\0');
    slash = (NS_tchar *) NS_tstrrchr(installDir, NS_SLASH);
  }
  if (slash) {
    *slash = NS_T('\0');
  } else {
    return false;
  }
  return true;
}







static int
CopyInstallDirToDestDir()
{
  
  
  NS_tchar installDir[MAXPATHLEN];
  if (!GetInstallationDir(installDir)) {
    return NO_INSTALLDIR_ERROR;
  }

  
#ifdef XP_WIN
#define SKIPLIST_COUNT 3
#else
#define SKIPLIST_COUNT 2
#endif
  copy_recursive_skiplist<SKIPLIST_COUNT> skiplist;
#ifdef XP_MACOSX
  skiplist.append(0, installDir, NS_T("Updated.app"));
  skiplist.append(1, installDir, NS_T("Contents/MacOS/updates/0"));
#else
  skiplist.append(0, installDir, NS_T("updated"));
  skiplist.append(1, installDir, NS_T("updates/0"));
#ifdef XP_WIN
  skiplist.append(2, installDir, NS_T("updated.update_in_progress.lock"));
#endif
#endif

  return ensure_copy_recursive(installDir, gDestinationPath, skiplist);
}







static int
ProcessReplaceRequest()
{
  
  
  
  

  NS_tchar installDir[MAXPATHLEN];
  if (!GetInstallationDir(installDir)) {
    return NO_INSTALLDIR_ERROR;
  }

#ifdef XP_MACOSX
  NS_tchar sourceDir[MAXPATHLEN];
  NS_tsnprintf(sourceDir, sizeof(sourceDir)/sizeof(sourceDir[0]),
               NS_T("%s/Contents"), installDir);
#elif XP_WIN
  
  
  
  
  NS_tchar sourceDir[MAXPATHLEN];
  if (!GetLongPathNameW(installDir, sourceDir, sizeof(sourceDir)/sizeof(sourceDir[0]))) {
    return NO_INSTALLDIR_ERROR;
  }
#else
  NS_tchar* sourceDir = installDir;
#endif

  NS_tchar tmpDir[MAXPATHLEN];
  NS_tsnprintf(tmpDir, sizeof(tmpDir)/sizeof(tmpDir[0]),
               NS_T("%s.bak"), sourceDir);

  NS_tchar newDir[MAXPATHLEN];
  NS_tsnprintf(newDir, sizeof(newDir)/sizeof(newDir[0]),
#ifdef XP_MACOSX
               NS_T("%s/Updated.app/Contents"),
#else
               NS_T("%s.bak/updated"),
#endif
               installDir);

  
  
  
  
  ensure_remove_recursive(tmpDir);

  LOG(("Begin moving sourceDir (" LOG_S ") to tmpDir (" LOG_S ")\n",
       sourceDir, tmpDir));
  int rv = rename_file(sourceDir, tmpDir, true);
#ifdef XP_WIN
  
  
  
  
  const int max_retries = 10;
  int retries = 0;
  while (rv == WRITE_ERROR && (retries++ < max_retries)) {
    LOG(("PerformReplaceRequest: sourceDir rename attempt %d failed. " \
         "File: " LOG_S ". Last error: %d, err: %d\n", retries,
         sourceDir, GetLastError(), rv));

    Sleep(100);

    rv = rename_file(sourceDir, tmpDir, true);
  }
#endif
  if (rv) {
    LOG(("Moving sourceDir to tmpDir failed, err: %d\n", rv));
    return rv;
  }

  LOG(("Begin moving newDir (" LOG_S ") to sourceDir (" LOG_S ")\n",
       newDir, sourceDir));
  rv = rename_file(newDir, sourceDir, true);
  if (rv) {
    LOG(("Moving newDir to sourceDir failed, err: %d\n", rv));
    LOG(("Now, try to move tmpDir back to sourceDir\n"));
    ensure_remove_recursive(sourceDir);
    int rv2 = rename_file(tmpDir, sourceDir, true);
    if (rv2) {
      LOG(("Moving tmpDir back to sourceDir failed, err: %d\n", rv2));
    }
    return rv;
  }

  LOG(("Now, remove the tmpDir\n"));
  rv = ensure_remove_recursive(tmpDir);
  if (rv) {
    LOG(("Removing tmpDir failed, err: %d\n", rv));
#ifdef XP_WIN
    if (MoveFileExW(tmpDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
      LOG(("tmpDir will be removed on OS reboot: " LOG_S "\n", tmpDir));
    } else {
      LOG(("Failed to schedule OS reboot removal of directory: " LOG_S "\n",
           tmpDir));
    }
#endif
  }

#ifdef XP_MACOSX
  
  
  
  NS_tchar updatedAppDir[MAXPATHLEN];
  NS_tsnprintf(updatedAppDir, sizeof(updatedAppDir)/sizeof(updatedAppDir[0]),
               NS_T("%s/Updated.app"), installDir);
  NS_tDIR *dir = NS_topendir(updatedAppDir);
  if (dir) {
    NS_tdirent *entry;
    while ((entry = NS_treaddir(dir)) != 0) {
      if (NS_tstrcmp(entry->d_name, NS_T(".")) &&
          NS_tstrcmp(entry->d_name, NS_T(".."))) {
        NS_tchar childSrcPath[MAXPATHLEN];
        NS_tsnprintf(childSrcPath, sizeof(childSrcPath)/sizeof(childSrcPath[0]),
                     NS_T("%s/%s"), updatedAppDir, entry->d_name);
        NS_tchar childDstPath[MAXPATHLEN];
        NS_tsnprintf(childDstPath, sizeof(childDstPath)/sizeof(childDstPath[0]),
                     NS_T("%s/%s"), installDir, entry->d_name);
        ensure_remove_recursive(childDstPath);
        rv = rename_file(childSrcPath, childDstPath, true);
        if (rv) {
          LOG(("Moving " LOG_S " to " LOG_S " failed, err: %d\n",
               childSrcPath, childDstPath, errno));
        }
      }
    }

    NS_tclosedir(dir);
  } else {
    LOG(("Updated.app dir can't be found: " LOG_S ", err: %d\n",
         updatedAppDir, errno));
  }
  ensure_remove_recursive(updatedAppDir);

  LOG(("Moving the precomplete file\n"));

  
  NS_tchar precompleteSource[MAXPATHLEN];
  NS_tsnprintf(precompleteSource, sizeof(precompleteSource)/sizeof(precompleteSource[0]),
               NS_T("%s/precomplete"), installDir);

  NS_tchar precompleteTmp[MAXPATHLEN];
  NS_tsnprintf(precompleteTmp, sizeof(precompleteTmp)/sizeof(precompleteTmp[0]),
               NS_T("%s/precomplete.bak"), installDir);

  NS_tchar precompleteNew[MAXPATHLEN];
  NS_tsnprintf(precompleteNew, sizeof(precompleteNew)/sizeof(precompleteNew[0]),
               NS_T("%s/Updated.app/precomplete"), installDir);

  ensure_remove(precompleteTmp);
  LOG(("Begin moving precompleteSrc to precompleteTmp\n"));
  rv = rename_file(precompleteSource, precompleteTmp);
  LOG(("Moved precompleteSrc to precompleteTmp, err: %d\n", rv));
  LOG(("Begin moving precompleteNew to precompleteSrc\n"));
  int rv2 = rename_file(precompleteNew, precompleteSource);
  LOG(("Moved precompleteNew to precompleteSrc, err: %d\n", rv2));

  
  
  
  
  
  if (!rv && rv2) {
    LOG(("Begin trying to recover precompleteSrc\n"));
    rv = rename_file(precompleteTmp, precompleteSource);
    LOG(("Moved precompleteTmp to precompleteSrc, err: %d\n", rv));
  }

  LOG(("Finished moving the precomplete file\n"));
#endif

  gSucceeded = true;

  return 0;
}

#ifdef XP_WIN
static void 
WaitForServiceFinishThread(void *param)
{
  
  
  WaitForServiceStop(SVC_NAME, 595);
  LOG(("calling QuitProgressUI\n"));
  QuitProgressUI();
}
#endif








static int
ReadMARChannelIDs(const NS_tchar *path, MARChannelStringTable *results)
{
  const unsigned int kNumStrings = 1;
  const char *kUpdaterKeys = "ACCEPTED_MAR_CHANNEL_IDS\0";
  char updater_strings[kNumStrings][MAX_TEXT_LEN];

  int result = ReadStrings(path, kUpdaterKeys, kNumStrings,
                           updater_strings, "Settings");

  strncpy(results->MARChannelID, updater_strings[0], MAX_TEXT_LEN - 1);
  results->MARChannelID[MAX_TEXT_LEN - 1] = 0;

  return result;
}

static void
UpdateThreadFunc(void *param)
{
  
  int rv;
  if (sReplaceRequest) {
    rv = ProcessReplaceRequest();
  } else {
    NS_tchar dataFile[MAXPATHLEN];
    NS_tsnprintf(dataFile, sizeof(dataFile)/sizeof(dataFile[0]),
                 NS_T("%s/update.mar"), gSourcePath);

    rv = gArchiveReader.Open(dataFile);

#ifdef MOZ_VERIFY_MAR_SIGNATURE
    if (rv == OK) {
      rv = gArchiveReader.VerifySignature();
    }

    if (rv == OK) {
      NS_tchar installDir[MAXPATHLEN];
      if (sBackgroundUpdate) {
        if (!GetInstallationDir(installDir)) {
          rv = NO_INSTALLDIR_ERROR;
        }
      } else {
        NS_tstrcpy(installDir, gDestinationPath);
      }
      if (rv == OK) {
        NS_tchar updateSettingsPath[MAX_TEXT_LEN];
        NS_tsnprintf(updateSettingsPath,
                     sizeof(updateSettingsPath) / sizeof(updateSettingsPath[0]),
                     NS_T("%s/update-settings.ini"), installDir);
        MARChannelStringTable MARStrings;
        if (ReadMARChannelIDs(updateSettingsPath, &MARStrings) != OK) {
          
          
          MARStrings.MARChannelID[0] = '\0';
        }

        rv = gArchiveReader.VerifyProductInformation(MARStrings.MARChannelID,
                                                     MOZ_APP_VERSION);
      }
    }
#endif

    if (rv == OK && sBackgroundUpdate) {
      rv = CopyInstallDirToDestDir();
    }

    if (rv == OK) {
      rv = DoUpdate();
      gArchiveReader.Close();
    }
  }

  bool reportRealResults = true;
  if (sReplaceRequest && rv && !getenv("MOZ_NO_REPLACE_FALLBACK")) {
    
    
    
    
    
    
    
    
    
    
    
    NS_tchar installDir[MAXPATHLEN];
    if (GetInstallationDir(installDir)) {
      NS_tchar stageDir[MAXPATHLEN];
      NS_tsnprintf(stageDir, sizeof(stageDir)/sizeof(stageDir[0]),
#ifdef XP_MACOSX
                   NS_T("%s/Updated.app"),
#else
                   NS_T("%s/updated"),
#endif
                   installDir);

      ensure_remove_recursive(stageDir);
      WriteStatusFile(sUsingService ? "pending-service" : "pending");
      putenv("MOZ_PROCESS_UPDATES="); 
      reportRealResults = false; 
    }
  }

  if (reportRealResults) {
    if (rv) {
      LOG(("failed: %d\n", rv));
    }
    else {
#ifdef XP_MACOSX
      
      
      
      
      char* cwd = getcwd(NULL, 0);
      if (cwd) {
        if (utimes(cwd, NULL) != 0) {
          LOG(("Couldn't set access/modification time on application bundle.\n"));
        }
        free(cwd);
      }
      else {
        LOG(("Couldn't get current working directory for setting "
             "access/modification time on application bundle.\n"));
      }
#endif

      LOG(("succeeded\n"));
    }
    WriteStatusFile(rv);
  }

  LOG(("calling QuitProgressUI\n"));
  QuitProgressUI();
}

int NS_main(int argc, NS_tchar **argv)
{
  InitProgressUI(&argc, &argv);

  
  
  
  
  
  
  
  
  
  
  
  
  if (argc < 3) {
    fprintf(stderr, "Usage: updater update-dir apply-to-dir [wait-pid [callback-working-dir callback-path args...]]\n");
    return 1;
  }

  
  gSourcePath = argv[1];
  
  
  
  
  NS_tstrncpy(gDestinationPath, argv[2], MAXPATHLEN);
  gDestinationPath[MAXPATHLEN - 1] = NS_T('\0');
  NS_tchar *slash = NS_tstrrchr(gDestinationPath, NS_SLASH);
  if (slash && !slash[1]) {
    *slash = NS_T('\0');
  }

#ifdef XP_WIN
  bool useService = false;
  bool testOnlyFallbackKeyExists = false;
  bool noServiceFallback = getenv("MOZ_NO_SERVICE_FALLBACK") != NULL;
  putenv(const_cast<char*>("MOZ_NO_SERVICE_FALLBACK="));

  
  
#ifdef MOZ_MAINTENANCE_SERVICE
  useService = IsUpdateStatusPendingService();
  
  
  
  testOnlyFallbackKeyExists = DoesFallbackKeyExist();
#endif

  
  {
    HKEY hkApp;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Applications",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL,
                    &hkApp, NULL);
    RegCloseKey(hkApp);
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Classes\\Applications\\updater.exe",
                        0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE, NULL,
                        &hkApp, NULL) == ERROR_SUCCESS) {
      RegSetValueExW(hkApp, L"IsHostApp", 0, REG_NONE, 0, 0);
      RegSetValueExW(hkApp, L"NoOpenWith", 0, REG_NONE, 0, 0);
      RegSetValueExW(hkApp, L"NoStartPage", 0, REG_NONE, 0, 0);
      RegCloseKey(hkApp);
    }
  }
#endif

  
#ifdef XP_WIN
  __int64 pid = 0;
#else
  int pid = 0;
#endif
  if (argc > 3) {
#ifdef XP_WIN
    pid = _wtoi64(argv[3]);
#else
    pid = atoi(argv[3]);
#endif
    if (pid == -1) {
      
      
      sBackgroundUpdate = true;
    } else if (NS_tstrstr(argv[3], NS_T("/replace"))) {
      
      
      sReplaceRequest = true;
    }
  }

  if (sReplaceRequest) {
    
    
    NS_tchar installDir[MAXPATHLEN];
    if (!GetInstallationDir(installDir)) {
      fprintf(stderr, "Could not get the installation directory\n");
      return 1;
    }

#ifdef XP_WIN
    NS_tchar* logDir = gSourcePath;
#else
    NS_tchar logDir[MAXPATHLEN];
    NS_tsnprintf(logDir, sizeof(logDir)/sizeof(logDir[0]),
#ifdef XP_MACOSX
                 NS_T("%s/Updated.app/Contents/MacOS/updates"),
#else
                 NS_T("%s/updated/updates"),
#endif
                 installDir);
#endif

    LogInitAppend(logDir, NS_T("last-update.log"), NS_T("update.log"));
  } else {
    LogInit(gSourcePath, NS_T("update.log"));
  }

  if (!WriteStatusFile("applying")) {
    LOG(("failed setting status to 'applying'\n"));
    return 1;
  }

  if (sBackgroundUpdate) {
    LOG(("Performing a background update\n"));
  } else if (sReplaceRequest) {
    LOG(("Performing a replace request\n"));
  }

#ifdef XP_WIN
  if (pid > 0) {
    HANDLE parent = OpenProcess(SYNCHRONIZE, false, (DWORD) pid);
    
    
    
    if (parent) {
      DWORD result = WaitForSingleObject(parent, 5000);
      CloseHandle(parent);
      if (result != WAIT_OBJECT_0)
        return 1;
    }
  }
#else
  if (pid > 0)
    waitpid(pid, NULL, 0);
#endif

  if (sReplaceRequest) {
#ifdef XP_WIN
    
    
    NS_tchar tmpDir[MAXPATHLEN];
    if (GetTempPathW(MAXPATHLEN, tmpDir)) {
      NS_tchdir(tmpDir);
    }
#endif
  }

  
  
  
  const int callbackIndex = 5;

#if defined(XP_WIN)
  sUsingService = getenv("MOZ_USING_SERVICE") != NULL;
  putenv(const_cast<char*>("MOZ_USING_SERVICE="));
  
  
  
  
  
  int lastFallbackError = FALLBACKKEY_UNKNOWN_ERROR;

  
  
  HANDLE updateLockFileHandle = INVALID_HANDLE_VALUE;
  NS_tchar elevatedLockFilePath[MAXPATHLEN] = {NS_T('\0')};
  if (!sUsingService &&
      (argc > callbackIndex || sBackgroundUpdate || sReplaceRequest)) {
    NS_tchar updateLockFilePath[MAXPATHLEN];
    if (sBackgroundUpdate) {
      
      
      NS_tsnprintf(updateLockFilePath,
                   sizeof(updateLockFilePath)/sizeof(updateLockFilePath[0]),
                   NS_T("%s.update_in_progress.lock"), gDestinationPath);
    } else if (sReplaceRequest) {
      
      
      NS_tchar installDir[MAXPATHLEN];
      if (!GetInstallationDir(installDir)) {
        return 1;
      }
      NS_tchar *slash = (NS_tchar *) NS_tstrrchr(installDir, NS_SLASH);
      *slash = NS_T('\0');
      NS_tsnprintf(updateLockFilePath,
                   sizeof(updateLockFilePath)/sizeof(updateLockFilePath[0]),
                   NS_T("%s\\moz_update_in_progress.lock"), installDir);
    } else {
      
      
      NS_tsnprintf(updateLockFilePath,
                   sizeof(updateLockFilePath)/sizeof(updateLockFilePath[0]),
                   NS_T("%s.update_in_progress.lock"), argv[callbackIndex]);
    }

    
    
    
    if (!_waccess(updateLockFilePath, F_OK) &&
        NS_tremove(updateLockFilePath) != 0) {
      
      
      if (sBackgroundUpdate || sReplaceRequest) {
        
        
        WriteStatusFile("pending");
      }
      LOG(("Update already in progress! Exiting\n"));
      return 1;
    }

    updateLockFileHandle = CreateFileW(updateLockFilePath,
                                       GENERIC_READ | GENERIC_WRITE,
                                       0,
                                       NULL,
                                       OPEN_ALWAYS,
                                       FILE_FLAG_DELETE_ON_CLOSE,
                                       NULL);

    NS_tsnprintf(elevatedLockFilePath,
                 sizeof(elevatedLockFilePath)/sizeof(elevatedLockFilePath[0]),
                 NS_T("%s/update_elevated.lock"), gSourcePath);


    
    bool startedFromUnelevatedUpdater =
      GetFileAttributesW(elevatedLockFilePath) != INVALID_FILE_ATTRIBUTES;
    
    
    
    
    
    
    
    if(startedFromUnelevatedUpdater) {
      
      
      UACHelper::DisablePrivileges(NULL);
    }

    if (updateLockFileHandle == INVALID_HANDLE_VALUE || 
        (useService && testOnlyFallbackKeyExists && noServiceFallback)) {
      if (!_waccess(elevatedLockFilePath, F_OK) &&
          NS_tremove(elevatedLockFilePath) != 0) {
        fprintf(stderr, "Unable to create elevated lock file! Exiting\n");
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
        LOG(("Unable to create elevated lock file! Exiting\n"));
        return 1;
      }

      PRUnichar *cmdLine = MakeCommandLine(argc - 1, argv + 1);
      if (!cmdLine) {
        CloseHandle(elevatedFileHandle);
        return 1;
      }

      NS_tchar installDir[MAXPATHLEN];
      if (!GetInstallationDir(installDir)) {
        return 1;
      }

      
      
      
      if (useService) {
        BOOL isLocal = FALSE;
        useService = IsLocalFile(argv[0], isLocal) && isLocal;
      }

      
      
      
      
      
      
      if (useService) {
        BOOL unpromptedElevation;
        if (IsUnpromptedElevation(unpromptedElevation)) {
          useService = !unpromptedElevation;
        }
      }

      
      
      if (useService) {
        WCHAR maintenanceServiceKey[MAX_PATH + 1];
        if (CalculateRegistryPathFromFilePath(installDir, maintenanceServiceKey)) {
          HKEY baseKey;
          if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                            maintenanceServiceKey, 0, 
                            KEY_READ | KEY_WOW64_64KEY, 
                            &baseKey) == ERROR_SUCCESS) {
            RegCloseKey(baseKey);
          } else {
            useService = testOnlyFallbackKeyExists;
            if (!useService) {
              lastFallbackError = FALLBACKKEY_NOKEY_ERROR;
            }
          }
        } else {
          useService = false;
          lastFallbackError = FALLBACKKEY_REGPATH_ERROR;
        }
      }

      
      
      
      
      

      
      
      if (useService) {
        
        
        DWORD ret = LaunchServiceSoftwareUpdateCommand(argc, (LPCWSTR *)argv);
        useService = (ret == ERROR_SUCCESS);
        
        if (useService) {
          bool showProgressUI = false;
          
          if (!sBackgroundUpdate) {
            
            
            
            showProgressUI = !InitProgressUIStrings();
          }

          
          
          DWORD lastState = WaitForServiceStop(SVC_NAME, 5);
          if (lastState != SERVICE_STOPPED) {
            Thread t1;
            if (t1.Run(WaitForServiceFinishThread, NULL) == 0 && 
                showProgressUI) {
              ShowProgressUI(true, false);
            }
            t1.Join();
          }

          lastState = WaitForServiceStop(SVC_NAME, 1);
          if (lastState != SERVICE_STOPPED) {
            
            
            lastFallbackError = FALLBACKKEY_SERVICE_NO_STOP_ERROR;
            useService = false;
          }
        } else {
          lastFallbackError = FALLBACKKEY_LAUNCH_ERROR;
        }
      }

      
      
      
      
      if (!useService && sBackgroundUpdate) {
        if (updateLockFileHandle != INVALID_HANDLE_VALUE) {
          CloseHandle(updateLockFileHandle);
        }
        WriteStatusPending(gSourcePath);
        return 0;
      }

      
      
      
      
      
      
      
      
      if (useService && !sBackgroundUpdate) {
        bool updateStatusSucceeded = false;
        if (IsUpdateStatusSucceeded(updateStatusSucceeded) && 
            updateStatusSucceeded) {
          if (!LaunchWinPostProcess(installDir, gSourcePath, false, NULL)) {
            fprintf(stderr, "The post update process which runs as the user"
                    " for service update could not be launched.");
          }
        }
      }

      
      
      
      
      
      
      if (!useService && !noServiceFallback && 
          updateLockFileHandle == INVALID_HANDLE_VALUE) {
        SHELLEXECUTEINFO sinfo;
        memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
        sinfo.cbSize       = sizeof(SHELLEXECUTEINFO);
        sinfo.fMask        = SEE_MASK_FLAG_NO_UI |
                             SEE_MASK_FLAG_DDEWAIT |
                             SEE_MASK_NOCLOSEPROCESS;
        sinfo.hwnd         = NULL;
        sinfo.lpFile       = argv[0];
        sinfo.lpParameters = cmdLine;
        sinfo.lpVerb       = L"runas";
        sinfo.nShow        = SW_SHOWNORMAL;

        bool result = ShellExecuteEx(&sinfo);
        free(cmdLine);

        if (result) {
          WaitForSingleObject(sinfo.hProcess, INFINITE);
          CloseHandle(sinfo.hProcess);
        } else {
          WriteStatusFile(ELEVATION_CANCELED);
        }
      }

      if (argc > callbackIndex) {
        LaunchCallbackApp(argv[4], argc - callbackIndex,
                          argv + callbackIndex, sUsingService);
      }

      CloseHandle(elevatedFileHandle);

      if (!useService && !noServiceFallback &&
          INVALID_HANDLE_VALUE == updateLockFileHandle) {
        
        
        
        return 0;
      } else if(useService) {
        
        
        if (updateLockFileHandle != INVALID_HANDLE_VALUE) {
          CloseHandle(updateLockFileHandle);
        }
        return 0;
      } else {
        
        
        
        
        
        CloseHandle(updateLockFileHandle);
        WriteStatusFile(lastFallbackError);
        return 0;
      }
    }
  }
#endif

#if defined(MOZ_WIDGET_GONK)
  
  
  
  

  {
    GonkAutoMounter mounter;
    if (mounter.GetAccess() != MountAccess::ReadWrite) {
      WriteStatusFile(FILESYSTEM_MOUNT_READWRITE_ERROR);
      return 1;
    }
#endif

  if (sBackgroundUpdate) {
    
    
    ensure_remove_recursive(gDestinationPath);
  }
  if (!sReplaceRequest) {
    
    if (NS_tchdir(gDestinationPath) != 0) {
      
      int rv = NS_tmkdir(gDestinationPath, 0755);
      if (rv == OK && errno != EEXIST) {
        
        if (NS_tchdir(gDestinationPath) != 0) {
          
          return 1;
        }
      } else {
        
        return 1;
      }
    }
  }

  LOG(("SOURCE DIRECTORY " LOG_S "\n", gSourcePath));
  LOG(("DESTINATION DIRECTORY " LOG_S "\n", gDestinationPath));

#ifdef XP_WIN
  
  
  if (!sReplaceRequest) {
    
    
    NS_tchar *destpath = (NS_tchar *) malloc((NS_tstrlen(gDestinationPath) + 2) * sizeof(NS_tchar));
    if (!destpath)
      return 1;

    NS_tchar *c = destpath;
    NS_tstrcpy(c, gDestinationPath);
    c += NS_tstrlen(gDestinationPath);
    if (gDestinationPath[NS_tstrlen(gDestinationPath) - 1] != NS_T('/') &&
        gDestinationPath[NS_tstrlen(gDestinationPath) - 1] != NS_T('\\')) {
      NS_tstrcat(c, NS_T("/"));
      c += NS_tstrlen(NS_T("/"));
    }
    *c = NS_T('\0');
    c++;

    gDestPath = destpath;
  }

  NS_tchar applyDirLongPath[MAXPATHLEN];
  if (!GetLongPathNameW(gDestinationPath, applyDirLongPath,
                        sizeof(applyDirLongPath)/sizeof(applyDirLongPath[0]))) {
    LOG(("NS_main: unable to find apply to dir: " LOG_S "\n", gDestinationPath));
    LogFinish();
    WriteStatusFile(WRITE_ERROR);
    EXIT_WHEN_ELEVATED(elevatedLockFilePath, updateLockFileHandle, 1);
    if (argc > callbackIndex) {
      LaunchCallbackApp(argv[4], argc - callbackIndex,
                        argv + callbackIndex, sUsingService);
    }
    return 1;
  }

  HANDLE callbackFile = INVALID_HANDLE_VALUE;
  if (argc > callbackIndex) {
    
    
    
    
    
    NS_tchar callbackLongPath[MAXPATHLEN];
    ZeroMemory(callbackLongPath, sizeof(callbackLongPath));
    NS_tchar *targetPath = argv[callbackIndex];
    NS_tchar buffer[MAXPATHLEN*2];
    if (sReplaceRequest) {
      
      
      size_t commonPrefixLength = PathCommonPrefixW(argv[callbackIndex], gDestinationPath, NULL);
      NS_tchar *p = buffer;
      NS_tstrncpy(p, argv[callbackIndex], commonPrefixLength);
      p += commonPrefixLength;
      NS_tstrcpy(p, gDestinationPath + commonPrefixLength);
      p += NS_tstrlen(gDestinationPath + commonPrefixLength);
      *p = NS_T('\\');
      ++p;
      *p = NS_T('\0');
      NS_tchar installDir[MAXPATHLEN];
      if (!GetInstallationDir(installDir))
        return 1;
      size_t callbackPrefixLength = PathCommonPrefixW(argv[callbackIndex], installDir, NULL);
      NS_tstrcpy(p, argv[callbackIndex] + max(callbackPrefixLength, commonPrefixLength));
      targetPath = buffer;
    }
    if (!GetLongPathNameW(targetPath, callbackLongPath,
                          sizeof(callbackLongPath)/sizeof(callbackLongPath[0]))) {
      LOG(("NS_main: unable to find callback file: " LOG_S "\n", targetPath));
      LogFinish();
      WriteStatusFile(WRITE_ERROR);
      EXIT_WHEN_ELEVATED(elevatedLockFilePath, updateLockFileHandle, 1);
      if (argc > callbackIndex) {
        LaunchCallbackApp(argv[4], 
                          argc - callbackIndex, 
                          argv + callbackIndex, 
                          sUsingService);
      }
      return 1;
    }

    
    if (!sReplaceRequest) {
      int len = NS_tstrlen(applyDirLongPath);
      NS_tchar *s = callbackLongPath;
      NS_tchar *d = gCallbackRelPath;
      
      
      s += len;
      if (*s == NS_T('\\'))
        ++s;

      
      
      do {
        if (*s == NS_T('\\'))
          *d = NS_T('/');
        else
          *d = *s;
        ++s;
        ++d;
      } while (*s);
      *d = NS_T('\0');
      ++d;

      
      NS_tsnprintf(gCallbackBackupPath,
                   sizeof(gCallbackBackupPath)/sizeof(gCallbackBackupPath[0]),
                   NS_T("%s" CALLBACK_BACKUP_EXT), argv[callbackIndex]);
      NS_tremove(gCallbackBackupPath);
      CopyFileW(argv[callbackIndex], gCallbackBackupPath, false);

      
      
      
      const int max_retries = 10;
      int retries = 1;
      DWORD lastWriteError = 0;
      do {
        
        
        
        callbackFile = CreateFileW(targetPath,
                                   DELETE | GENERIC_WRITE,
                                   
                                   FILE_SHARE_DELETE | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, 0, NULL);
        if (callbackFile != INVALID_HANDLE_VALUE)
          break;

        lastWriteError = GetLastError();
        LOG(("NS_main: callback app open attempt %d failed. " \
             "File: " LOG_S ". Last error: %d\n", retries,
             targetPath, lastWriteError));

        Sleep(100);
      } while (++retries <= max_retries);

      
      
      if (callbackFile == INVALID_HANDLE_VALUE) {
        LOG(("NS_main: file in use - failed to exclusively open executable " \
             "file: " LOG_S "\n", argv[callbackIndex]));
        LogFinish();
        if (ERROR_ACCESS_DENIED == lastWriteError) {
          WriteStatusFile(WRITE_ERROR_ACCESS_DENIED);
        } else if (ERROR_SHARING_VIOLATION == lastWriteError) {
          WriteStatusFile(WRITE_ERROR_SHARING_VIOLATION);
        } else {
          WriteStatusFile(WRITE_ERROR_CALLBACK_APP);
        }
        NS_tremove(gCallbackBackupPath);
        EXIT_WHEN_ELEVATED(elevatedLockFilePath, updateLockFileHandle, 1);
        LaunchCallbackApp(argv[4],
                          argc - callbackIndex,
                          argv + callbackIndex,
                          sUsingService);
        return 1;
      }
    }
  }

  
  if (!sBackgroundUpdate && !sReplaceRequest) {
    
    
    
    if (NS_taccess(DELETE_DIR, F_OK)) {
      NS_tmkdir(DELETE_DIR, 0755);
    }
  }
#endif 

  
  
  
  Thread t;
  if (t.Run(UpdateThreadFunc, NULL) == 0) {
    if (!sBackgroundUpdate && !sReplaceRequest) {
      ShowProgressUI();
    }
  }
  t.Join();

#ifdef XP_WIN
  if (argc > callbackIndex && !sReplaceRequest) {
    CloseHandle(callbackFile);
    
    NS_tremove(gCallbackBackupPath);
  }

  if (!sBackgroundUpdate && !sReplaceRequest && _wrmdir(DELETE_DIR)) {
    LOG(("NS_main: unable to remove directory: " LOG_S ", err: %d\n",
         DELETE_DIR, errno));
    
    
    
    
    
    
    
    
    
    if (MoveFileEx(DELETE_DIR, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
      LOG(("NS_main: directory will be removed on OS reboot: " LOG_S "\n",
           DELETE_DIR));
    } else {
      LOG(("NS_main: failed to schedule OS reboot removal of " \
           "directory: " LOG_S "\n", DELETE_DIR));
    }
  }
#endif 

#if defined(MOZ_WIDGET_GONK)
  } 
#endif

  LogFinish();

  if (argc > callbackIndex) {
#if defined(XP_WIN)
    if (gSucceeded) {
      
      
      
      
      
      
      
      if (!sUsingService) {
        NS_tchar installDir[MAXPATHLEN];
        if (GetInstallationDir(installDir)) {
          if (!LaunchWinPostProcess(installDir, gSourcePath, false, NULL)) {
            LOG(("NS_main: The post update process could not be launched.\n"));
          }

          StartServiceUpdate(installDir);
        }
      }
    }
    EXIT_WHEN_ELEVATED(elevatedLockFilePath, updateLockFileHandle, 0);
#endif 
#ifdef XP_MACOSX
    if (gSucceeded) {
      LaunchMacPostProcess(argv[callbackIndex]);
    }
#endif 

    LaunchCallbackApp(argv[4], 
                      argc - callbackIndex, 
                      argv + callbackIndex, 
                      sUsingService);
  }

  return gSucceeded ? 0 : 1;
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
    return UNEXPECTED_MAR_ERROR;
  }

  Action *a = mFirst;
  int i = 0;
  while (a) {
    int rv = a->Prepare();
    if (rv)
      return rv;

    float percent = float(++i) / float(mCount);
    UpdateProgressUI(PROGRESS_PREPARE_SIZE * percent);

    a = a->mNext;
  }

  return OK;
}

int
ActionList::Execute()
{
  int currentProgress = 0, maxProgress = 0;
  Action *a = mFirst;
  while (a) {
    maxProgress += a->mProgressCost;
    a = a->mNext;
  }

  a = mFirst;
  while (a) {
    int rv = a->Execute();
    if (rv) {
      LOG(("### execution failed\n"));
      return rv;
    }

    currentProgress += a->mProgressCost;
    float percent = float(currentProgress) / float(maxProgress);
    UpdateProgressUI(PROGRESS_PREPARE_SIZE +
                     PROGRESS_EXECUTE_SIZE * percent);

    a = a->mNext;
  }

  return OK;
}

void
ActionList::Finish(int status)
{
  Action *a = mFirst;
  int i = 0;
  while (a) {
    a->Finish(status);

    float percent = float(++i) / float(mCount);
    UpdateProgressUI(PROGRESS_PREPARE_SIZE +
                     PROGRESS_EXECUTE_SIZE +
                     PROGRESS_FINISH_SIZE * percent);

    a = a->mNext;
  }

  if (status == OK)
    gSucceeded = true;
}


#ifdef XP_WIN
int add_dir_entries(const NS_tchar *dirpath, ActionList *list)
{
  int rv = OK;
  WIN32_FIND_DATAW finddata;
  HANDLE hFindFile;
  NS_tchar searchspec[MAXPATHLEN];
  NS_tchar foundpath[MAXPATHLEN];

  NS_tsnprintf(searchspec, sizeof(searchspec)/sizeof(searchspec[0]),
               NS_T("%s*"), dirpath);
  const NS_tchar *pszSpec = get_full_path(searchspec);

  hFindFile = FindFirstFileW(pszSpec, &finddata);
  if (hFindFile != INVALID_HANDLE_VALUE) {
    do {
      
      if (NS_tstrcmp(finddata.cFileName, NS_T(".")) == 0 ||
          NS_tstrcmp(finddata.cFileName, NS_T("..")) == 0)
        continue;

      NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                   NS_T("%s%s"), dirpath, finddata.cFileName);
      if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                     NS_T("%s/"), foundpath);
        
        rv = add_dir_entries(foundpath, list);
        if (rv) {
          LOG(("add_dir_entries error: " LOG_S ", err: %d\n", foundpath, rv));
          return rv;
        }
      } else {
        
        NS_tchar *quotedpath = get_quoted_path(foundpath);
        if (!quotedpath)
          return PARSE_ERROR;

        Action *action = new RemoveFile();
        rv = action->Parse(quotedpath);
        if (rv) {
          LOG(("add_dir_entries Parse error on recurse: " LOG_S ", err: %d\n", quotedpath, rv));
          return rv;
        }

        list->Append(action);
      }
    } while (FindNextFileW(hFindFile, &finddata) != 0);

    FindClose(hFindFile);
    {
      
      NS_tchar *quotedpath = get_quoted_path(dirpath);
      if (!quotedpath)
        return PARSE_ERROR;

      Action *action = new RemoveDir();
      rv = action->Parse(quotedpath);
      if (rv)
        LOG(("add_dir_entries Parse error on close: " LOG_S ", err: %d\n", quotedpath, rv));
      else
        list->Append(action);
    }
  }

  return rv;
}

#elif defined(SOLARIS)
int add_dir_entries(const NS_tchar *dirpath, ActionList *list)
{
  int rv = OK;
  NS_tchar searchpath[MAXPATHLEN];
  NS_tchar foundpath[MAXPATHLEN];
  struct {
    dirent dent_buffer;
    char chars[MAXNAMLEN];
  } ent_buf;
  struct dirent* ent;


  NS_tsnprintf(searchpath, sizeof(searchpath)/sizeof(searchpath[0]), NS_T("%s"),
               dirpath);
  
  
  searchpath[NS_tstrlen(searchpath) - 1] = NS_T('\0');

  DIR* dir = opendir(searchpath);
  if (!dir) {
    LOG(("add_dir_entries error on opendir: " LOG_S ", err: %d\n", searchpath,
         errno));
    return UNEXPECTED_FILE_OPERATION_ERROR;
  }

  while (readdir_r(dir, (dirent *)&ent_buf, &ent) == 0 && ent) {
    if ((strcmp(ent->d_name, ".") == 0) ||
        (strcmp(ent->d_name, "..") == 0))
      continue;

    NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                 NS_T("%s%s"), dirpath, ent->d_name);
    struct stat64 st_buf;
    int test = stat64(foundpath, &st_buf);
    if (test) {
      closedir(dir);
      return UNEXPECTED_FILE_OPERATION_ERROR;
    }
    if (S_ISDIR(st_buf.st_mode)) {
      NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                   NS_T("%s/"), foundpath);
      
      rv = add_dir_entries(foundpath, list);
      if (rv) {
        LOG(("add_dir_entries error: " LOG_S ", err: %d\n", foundpath, rv));
        closedir(dir);
        return rv;
      }
    } else {
      
      NS_tchar *quotedpath = get_quoted_path(foundpath);
      if (!quotedpath) {
        closedir(dir);
        return PARSE_ERROR;
      }

      Action *action = new RemoveFile();
      rv = action->Parse(quotedpath);
      if (rv) {
        LOG(("add_dir_entries Parse error on recurse: " LOG_S ", err: %d\n",
             quotedpath, rv));
        closedir(dir);
        return rv;
      }

      list->Append(action);
    }
  }
  closedir(dir);

  
  NS_tchar *quotedpath = get_quoted_path(dirpath);
  if (!quotedpath)
    return PARSE_ERROR;

  Action *action = new RemoveDir();
  rv = action->Parse(quotedpath);
  if (rv) {
    LOG(("add_dir_entries Parse error on close: " LOG_S ", err: %d\n",
         quotedpath, rv));
  }
  else {
    list->Append(action);
  }

  return rv;
}

#else

int add_dir_entries(const NS_tchar *dirpath, ActionList *list)
{
  int rv = OK;
  FTS *ftsdir;
  FTSENT *ftsdirEntry;
  NS_tchar searchpath[MAXPATHLEN];

  NS_tsnprintf(searchpath, sizeof(searchpath)/sizeof(searchpath[0]), NS_T("%s"),
               dirpath);
  
  
  searchpath[NS_tstrlen(searchpath) - 1] = NS_T('\0');
  char* const pathargv[] = {searchpath, NULL};

  
  
  if (!(ftsdir = fts_open(pathargv,
                          FTS_PHYSICAL | FTS_NOSTAT | FTS_XDEV | FTS_NOCHDIR,
                          NULL)))
    return UNEXPECTED_FILE_OPERATION_ERROR;

  while ((ftsdirEntry = fts_read(ftsdir)) != NULL) {
    NS_tchar foundpath[MAXPATHLEN];
    NS_tchar *quotedpath;
    Action *action = NULL;

    switch (ftsdirEntry->fts_info) {
      
      case FTS_SL:
      case FTS_SLNONE:
      case FTS_DEFAULT:
        LOG(("add_dir_entries: found a non-standard file: " LOG_S "\n",
             ftsdirEntry->fts_path));
        

      
      case FTS_F:
      case FTS_NSOK:
        
        NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                     NS_T("%s"), ftsdirEntry->fts_accpath);
        quotedpath = get_quoted_path(foundpath);
        if (!quotedpath) {
          rv = UPDATER_QUOTED_PATH_MEM_ERROR;
          break;
        }
        action = new RemoveFile();
        rv = action->Parse(quotedpath);
        if (!rv)
          list->Append(action);
        break;

      
      case FTS_DP:
        rv = OK;
        
        NS_tsnprintf(foundpath, sizeof(foundpath)/sizeof(foundpath[0]),
                     NS_T("%s/"), ftsdirEntry->fts_accpath);
        quotedpath = get_quoted_path(foundpath);
        if (!quotedpath) {
          rv = UPDATER_QUOTED_PATH_MEM_ERROR;
          break;
        }

        action = new RemoveDir();
        rv = action->Parse(quotedpath);
        if (!rv)
          list->Append(action);
        break;

      
      case FTS_DNR:
      case FTS_NS:
        
        
        
        if (ENOENT == ftsdirEntry->fts_errno) {
          rv = OK;
          break;
        }
        

      case FTS_ERR:
        rv = UNEXPECTED_FILE_OPERATION_ERROR;
        LOG(("add_dir_entries: fts_read() error: " LOG_S ", err: %d\n",
             ftsdirEntry->fts_path, ftsdirEntry->fts_errno));
        break;

      case FTS_DC:
        rv = UNEXPECTED_FILE_OPERATION_ERROR;
        LOG(("add_dir_entries: fts_read() returned FT_DC: " LOG_S "\n",
             ftsdirEntry->fts_path));
        break;

      default:
        
        rv = OK;
        break;
    }

    if (rv != OK)
      break;
  }

  fts_close(ftsdir);

  return rv;
}
#endif

static NS_tchar*
GetManifestContents(const NS_tchar *manifest)
{
  AutoFile mfile = NS_tfopen(manifest, NS_T("rb"));
  if (mfile == NULL) {
    LOG(("GetManifestContents: error opening manifest file: " LOG_S "\n", manifest));
    return NULL;
  }

  struct stat ms;
  int rv = fstat(fileno((FILE *)mfile), &ms);
  if (rv) {
    LOG(("GetManifestContents: error stating manifest file: " LOG_S "\n", manifest));
    return NULL;
  }

  char *mbuf = (char *) malloc(ms.st_size + 1);
  if (!mbuf)
    return NULL;

  size_t r = ms.st_size;
  char *rb = mbuf;
  while (r) {
    const size_t count = mmin(SSIZE_MAX, r);
    size_t c = fread(rb, 1, count, mfile);
    if (c != count) {
      LOG(("GetManifestContents: error reading manifest file: " LOG_S "\n", manifest));
      return NULL;
    }

    r -= c;
    rb += c;
  }
  mbuf[ms.st_size] = '\0';
  rb = mbuf;

#ifndef XP_WIN
  return rb;
#else
  NS_tchar *wrb = (NS_tchar *) malloc((ms.st_size + 1) * sizeof(NS_tchar));
  if (!wrb)
    return NULL;

  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, rb, -1, wrb,
                           ms.st_size + 1)) {
    LOG(("GetManifestContents: error converting utf8 to utf16le: %d\n", GetLastError()));
    free(mbuf);
    free(wrb);
    return NULL;
  }
  free(mbuf);

  return wrb;
#endif
}

int AddPreCompleteActions(ActionList *list)
{
  NS_tchar *rb = GetManifestContents(NS_T("precomplete"));
  if (rb == NULL) {
    LOG(("AddPreCompleteActions: error getting contents of precomplete " \
         "manifest\n"));
    
    return OK;
  }

  int rv;
  NS_tchar *line;
  while((line = mstrtok(kNL, &rb)) != 0) {
    
    if (*line == NS_T('#'))
      continue;

    NS_tchar *token = mstrtok(kWhitespace, &line);
    if (!token) {
      LOG(("AddPreCompleteActions: token not found in manifest\n"));
      return PARSE_ERROR;
    }

    Action *action = NULL;
    if (NS_tstrcmp(token, NS_T("remove")) == 0) { 
      action = new RemoveFile();
    }
    else if (NS_tstrcmp(token, NS_T("remove-cc")) == 0) { 
      continue;
    }
    else if (NS_tstrcmp(token, NS_T("rmdir")) == 0) { 
      action = new RemoveDir();
    }
    else {
      LOG(("AddPreCompleteActions: unknown token: " LOG_S "\n", token));
      return PARSE_ERROR;
    }

    if (!action)
      return BAD_ACTION_ERROR;

    rv = action->Parse(line);
    if (rv)
      return rv;

    list->Append(action);
  }

  return OK;
}

int DoUpdate()
{
  NS_tchar manifest[MAXPATHLEN];
  NS_tsnprintf(manifest, sizeof(manifest)/sizeof(manifest[0]),
               NS_T("%s/update.manifest"), gSourcePath);

  
  int rv = gArchiveReader.ExtractFile("updatev2.manifest", manifest);
  if (rv) {
    rv = gArchiveReader.ExtractFile("update.manifest", manifest);
    if (rv) {
      LOG(("DoUpdate: error extracting manifest file\n"));
      return rv;
    }
  }

  NS_tchar *rb = GetManifestContents(manifest);
  if (rb == NULL) {
    LOG(("DoUpdate: error opening manifest file: " LOG_S "\n", manifest));
    return READ_ERROR;
  }


  ActionList list;
  NS_tchar *line;
  bool isFirstAction = true;

  while((line = mstrtok(kNL, &rb)) != 0) {
    
    if (*line == NS_T('#'))
      continue;

    NS_tchar *token = mstrtok(kWhitespace, &line);
    if (!token) {
      LOG(("DoUpdate: token not found in manifest\n"));
      return PARSE_ERROR;
    }

    if (isFirstAction && NS_tstrcmp(token, NS_T("type")) == 0) {
      const NS_tchar *type = mstrtok(kQuote, &line);
      LOG(("UPDATE TYPE " LOG_S "\n", type));
      if (NS_tstrcmp(type, NS_T("complete")) == 0) {
        rv = AddPreCompleteActions(&list);
        if (rv)
          return rv;
      }
      isFirstAction = false;
      continue;
    }

    isFirstAction = false;

    Action *action = NULL;
    if (NS_tstrcmp(token, NS_T("remove")) == 0) { 
      action = new RemoveFile();
    }
    else if (NS_tstrcmp(token, NS_T("rmdir")) == 0) { 
      action = new RemoveDir();
    }
    else if (NS_tstrcmp(token, NS_T("rmrfdir")) == 0) { 
      const NS_tchar *reldirpath = mstrtok(kQuote, &line);
      if (!reldirpath)
        return PARSE_ERROR;

      if (reldirpath[NS_tstrlen(reldirpath) - 1] != NS_T('/'))
        return PARSE_ERROR;

      rv = add_dir_entries(reldirpath, &list);
      if (rv)
        return rv;

      continue;
    }
    else if (NS_tstrcmp(token, NS_T("add")) == 0) {
      action = new AddFile();
    }
    else if (NS_tstrcmp(token, NS_T("patch")) == 0) {
      action = new PatchFile();
    }
    else if (NS_tstrcmp(token, NS_T("add-if")) == 0) { 
      action = new AddIfFile();
    }
    else if (NS_tstrcmp(token, NS_T("patch-if")) == 0) { 
      action = new PatchIfFile();
    }
    else if (NS_tstrcmp(token, NS_T("add-cc")) == 0) { 
      continue;
    }
    else {
      LOG(("DoUpdate: unknown token: " LOG_S "\n", token));
      return PARSE_ERROR;
    }

    if (!action)
      return BAD_ACTION_ERROR;

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
