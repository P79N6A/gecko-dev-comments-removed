




















#include <log/logd.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#define kMaxTagLen  16      /* from the long-dead utils/Log.cpp */

#define kTagSetSize 16      /* arbitrary */

#if 0
#define TRACE(...) printf("fake_log_device: " __VA_ARGS__)
#else
#define TRACE(...) ((void)0)
#endif


typedef enum {
    FORMAT_OFF = 0,
    FORMAT_BRIEF,
    FORMAT_PROCESS,
    FORMAT_TAG,
    FORMAT_THREAD,
    FORMAT_RAW,
    FORMAT_TIME,
    FORMAT_THREADTIME,
    FORMAT_LONG
} LogFormat;





typedef struct LogState {
    
    int     fakeFd;

    
    char   *debugName;

    
    int     isBinary;

    
    int     globalMinPriority;

    
    LogFormat outputFormat;

    
    struct {
        char    tag[kMaxTagLen];
        int     minPriority;
    } tagSet[kTagSetSize];
} LogState;


#ifdef HAVE_PTHREADS






static pthread_mutex_t fakeLogDeviceLock = PTHREAD_MUTEX_INITIALIZER;

static void lock()
{
    pthread_mutex_lock(&fakeLogDeviceLock);
}

static void unlock()
{
    pthread_mutex_unlock(&fakeLogDeviceLock);
}
#else   
#define lock() ((void)0)
#define unlock() ((void)0)
#endif  





#define FAKE_FD_BASE 10000
#define MAX_OPEN_LOGS 16
static LogState *openLogTable[MAX_OPEN_LOGS];





static LogState *createLogState()
{
    size_t i;

    for (i = 0; i < sizeof(openLogTable); i++) {
        if (openLogTable[i] == NULL) {
            openLogTable[i] = calloc(1, sizeof(LogState));
            openLogTable[i]->fakeFd = FAKE_FD_BASE + i;
            return openLogTable[i];
        }
    }
    return NULL;
}




static LogState *fdToLogState(int fd)
{
    if (fd >= FAKE_FD_BASE && fd < FAKE_FD_BASE + MAX_OPEN_LOGS) {
        return openLogTable[fd - FAKE_FD_BASE];
    }
    return NULL;
}




static void deleteFakeFd(int fd)
{
    LogState *ls;

    lock();

    ls = fdToLogState(fd);
    if (ls != NULL) {
        openLogTable[fd - FAKE_FD_BASE] = NULL;
        free(ls->debugName);
        free(ls);
    }

    unlock();
}














static void configureInitialState(const char* pathName, LogState* logState)
{
    static const int kDevLogLen = sizeof("/dev/log/") - 1;

    logState->debugName = strdup(pathName);

    
    if (strcmp(pathName + kDevLogLen, "events") == 0) {
        logState->isBinary = 1;
    }

    
    logState->globalMinPriority = ANDROID_LOG_INFO;

    


    const char* tags = getenv("ANDROID_LOG_TAGS");
    TRACE("Found ANDROID_LOG_TAGS='%s'\n", tags);
    if (tags != NULL) {
        int entry = 0;

        while (*tags != '\0') {
            char tagName[kMaxTagLen];
            int i, minPrio;

            while (isspace(*tags))
                tags++;

            i = 0;
            while (*tags != '\0' && !isspace(*tags) && *tags != ':' &&
                i < kMaxTagLen)
            {
                tagName[i++] = *tags++;
            }
            if (i == kMaxTagLen) {
                TRACE("ERROR: env tag too long (%d chars max)\n", kMaxTagLen-1);
                return;
            }
            tagName[i] = '\0';

            
            minPrio = ANDROID_LOG_VERBOSE;
            if (tagName[0] == '*' && tagName[1] == '\0') {
                minPrio = ANDROID_LOG_DEBUG;
                tagName[0] = '\0';
            }

            if (*tags == ':') {
                tags++;
                if (*tags >= '0' && *tags <= '9') {
                    if (*tags >= ('0' + ANDROID_LOG_SILENT))
                        minPrio = ANDROID_LOG_VERBOSE;
                    else
                        minPrio = *tags - '\0';
                } else {
                    switch (*tags) {
                    case 'v':   minPrio = ANDROID_LOG_VERBOSE;  break;
                    case 'd':   minPrio = ANDROID_LOG_DEBUG;    break;
                    case 'i':   minPrio = ANDROID_LOG_INFO;     break;
                    case 'w':   minPrio = ANDROID_LOG_WARN;     break;
                    case 'e':   minPrio = ANDROID_LOG_ERROR;    break;
                    case 'f':   minPrio = ANDROID_LOG_FATAL;    break;
                    case 's':   minPrio = ANDROID_LOG_SILENT;   break;
                    default:    minPrio = ANDROID_LOG_DEFAULT;  break;
                    }
                }

                tags++;
                if (*tags != '\0' && !isspace(*tags)) {
                    TRACE("ERROR: garbage in tag env; expected whitespace\n");
                    TRACE("       env='%s'\n", tags);
                    return;
                }
            }

            if (tagName[0] == 0) {
                logState->globalMinPriority = minPrio;
                TRACE("+++ global min prio %d\n", logState->globalMinPriority);
            } else {
                logState->tagSet[entry].minPriority = minPrio;
                strcpy(logState->tagSet[entry].tag, tagName);
                TRACE("+++ entry %d: %s:%d\n",
                    entry,
                    logState->tagSet[entry].tag,
                    logState->tagSet[entry].minPriority);
                entry++;
            }
        }
    }


    


    const char* fstr = getenv("ANDROID_PRINTF_LOG");
    LogFormat format;
    if (fstr == NULL) {
        format = FORMAT_BRIEF;
    } else {
        if (strcmp(fstr, "brief") == 0)
            format = FORMAT_BRIEF;
        else if (strcmp(fstr, "process") == 0)
            format = FORMAT_PROCESS;
        else if (strcmp(fstr, "tag") == 0)
            format = FORMAT_PROCESS;
        else if (strcmp(fstr, "thread") == 0)
            format = FORMAT_PROCESS;
        else if (strcmp(fstr, "raw") == 0)
            format = FORMAT_PROCESS;
        else if (strcmp(fstr, "time") == 0)
            format = FORMAT_PROCESS;
        else if (strcmp(fstr, "long") == 0)
            format = FORMAT_PROCESS;
        else
            format = (LogFormat) atoi(fstr);        
    }

    logState->outputFormat = format;
}





static const char* getPriorityString(int priority)
{
    
    static const char* priorityStrings[] = {
        "Verbose", "Debug", "Info", "Warn", "Error", "Assert"
    };
    int idx;

    idx = (int) priority - (int) ANDROID_LOG_VERBOSE;
    if (idx < 0 ||
        idx >= (int) (sizeof(priorityStrings) / sizeof(priorityStrings[0])))
        return "?unknown?";
    return priorityStrings[idx];
}

#ifndef HAVE_WRITEV




static ssize_t fake_writev(int fd, const struct iovec *iov, int iovcnt) {
    int result = 0;
    struct iovec* end = iov + iovcnt;
    for (; iov < end; iov++) {
        int w = write(fd, iov->iov_base, iov->iov_len);
        if (w != iov->iov_len) {
            if (w < 0)
                return w;
            return result + w;
        }
        result += w;
    }
    return result;
}

#define writev fake_writev
#endif







static void showLog(LogState *state,
        int logPrio, const char* tag, const char* msg)
{
#if defined(HAVE_LOCALTIME_R)
    struct tm tmBuf;
#endif
    struct tm* ptm;
    char timeBuf[32];
    char prefixBuf[128], suffixBuf[128];
    char priChar;
    time_t when;
    pid_t pid, tid;

    TRACE("LOG %d: %s %s", logPrio, tag, msg);

    priChar = getPriorityString(logPrio)[0];
    when = time(NULL);
    pid = tid = getpid();       

    








#if defined(HAVE_LOCALTIME_R)
    ptm = localtime_r(&when, &tmBuf);
#else
    ptm = localtime(&when);
#endif
    
    strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);

    


    size_t prefixLen, suffixLen;

    switch (state->outputFormat) {
    case FORMAT_TAG:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%c/%-8s: ", priChar, tag);
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
    case FORMAT_PROCESS:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%c(%5d) ", priChar, pid);
        suffixLen = snprintf(suffixBuf, sizeof(suffixBuf),
            "  (%s)\n", tag);
        break;
    case FORMAT_THREAD:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%c(%5d:%5d) ", priChar, pid, tid);
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
    case FORMAT_RAW:
        prefixBuf[0] = 0; prefixLen = 0;
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
    case FORMAT_TIME:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%s %-8s\n\t", timeBuf, tag);
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
    case FORMAT_THREADTIME:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%s %5d %5d %c %-8s \n\t", timeBuf, pid, tid, priChar, tag);
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
    case FORMAT_LONG:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "[ %s %5d:%5d %c/%-8s ]\n",
            timeBuf, pid, tid, priChar, tag);
        strcpy(suffixBuf, "\n\n"); suffixLen = 2;
        break;
    default:
        prefixLen = snprintf(prefixBuf, sizeof(prefixBuf),
            "%c/%-8s(%5d): ", priChar, tag, pid);
        strcpy(suffixBuf, "\n"); suffixLen = 1;
        break;
     }

    


    const char* end = msg + strlen(msg);
    size_t numLines = 0;
    const char* p = msg;
    while (p < end) {
        if (*p++ == '\n') numLines++;
    }
    if (p > msg && *(p-1) != '\n') numLines++;

    



    const size_t INLINE_VECS = 6;
    const size_t MAX_LINES   = ((size_t)~0)/(3*sizeof(struct iovec*));
    struct iovec stackVec[INLINE_VECS];
    struct iovec* vec = stackVec;
    size_t numVecs;

    if (numLines > MAX_LINES)
        numLines = MAX_LINES;

    numVecs = numLines*3;  
    if (numVecs > INLINE_VECS) {
        vec = (struct iovec*)malloc(sizeof(struct iovec)*numVecs);
        if (vec == NULL) {
            msg = "LOG: write failed, no memory";
            numVecs = 3;
            numLines = 1;
            vec = stackVec;
        }
    }

    


    p = msg;
    struct iovec* v = vec;
    int totalLen = 0;
    while (numLines > 0 && p < end) {
        if (prefixLen > 0) {
            v->iov_base = prefixBuf;
            v->iov_len = prefixLen;
            totalLen += prefixLen;
            v++;
        }
        const char* start = p;
        while (p < end && *p != '\n') p++;
        if ((p-start) > 0) {
            v->iov_base = (void*)start;
            v->iov_len = p-start;
            totalLen += p-start;
            v++;
        }
        if (*p == '\n') p++;
        if (suffixLen > 0) {
            v->iov_base = suffixBuf;
            v->iov_len = suffixLen;
            totalLen += suffixLen;
            v++;
        }
        numLines -= 1;
    }
    
    













    for(;;) {
        int cc = writev(fileno(stderr), vec, v-vec);

        if (cc == totalLen) break;
        
        if (cc < 0) {
            if(errno == EINTR) continue;
            
                
            fprintf(stderr, "+++ LOG: write failed (errno=%d)\n", errno);
            break;
        } else {
                
            fprintf(stderr, "+++ LOG: write partial (%d of %d)\n", cc, totalLen);
            break;
        }
    }

    
    if (vec != stackVec)
        free(vec);
}









static ssize_t logWritev(int fd, const struct iovec* vector, int count)
{
    LogState* state;

    



    lock();

    state = fdToLogState(fd);
    if (state == NULL) {
        errno = EBADF;
        goto error;
    }

    if (state->isBinary) {
        TRACE("%s: ignoring binary log\n", state->debugName);
        goto bail;
    }

    if (count != 3) {
        TRACE("%s: writevLog with count=%d not expected\n",
            state->debugName, count);
        goto error;
    }

    
    int logPrio = *(const char*)vector[0].iov_base;
    const char* tag = (const char*) vector[1].iov_base;
    const char* msg = (const char*) vector[2].iov_base;

    
    int i;
    int minPrio = state->globalMinPriority;
    for (i = 0; i < kTagSetSize; i++) {
        if (state->tagSet[i].minPriority == ANDROID_LOG_UNKNOWN)
            break;      

        if (strcmp(state->tagSet[i].tag, tag) == 0) {
            
            minPrio = state->tagSet[i].minPriority;
            break;
        }
    }

    if (logPrio >= minPrio) {
        showLog(state, logPrio, tag, msg);
    } else {
        
    }

bail:
    unlock();
    return vector[0].iov_len + vector[1].iov_len + vector[2].iov_len;
error:
    unlock();
    return -1;
}




static int logClose(int fd)
{
    deleteFakeFd(fd);
    return 0;
}




static int logOpen(const char* pathName, int flags)
{
    LogState *logState;
    int fd = -1;

    lock();

    logState = createLogState();
    if (logState != NULL) {
        configureInitialState(pathName, logState);
        fd = logState->fakeFd;
    } else  {
        errno = ENFILE;
    }

    unlock();

    return fd;
}








static int (*redirectOpen)(const char *pathName, int flags) = NULL;
static int (*redirectClose)(int fd) = NULL;
static ssize_t (*redirectWritev)(int fd, const struct iovec* vector, int count)
        = NULL;

static void setRedirects()
{
    const char *ws;

    


    ws = getenv("ANDROID_WRAPSIM");
    if (ws != NULL && strcmp(ws, "1") == 0) {
        
        redirectOpen = (int (*)(const char *pathName, int flags))open;
        redirectClose = close;
        redirectWritev = writev;
    } else {
        
        redirectOpen = logOpen;
        redirectClose = logClose;
        redirectWritev = logWritev;
    }
}

int fakeLogOpen(const char *pathName, int flags)
{
    if (redirectOpen == NULL) {
        setRedirects();
    }
    return redirectOpen(pathName, flags);
}

int fakeLogClose(int fd)
{
    
    return redirectClose(fd);
}

ssize_t fakeLogWritev(int fd, const struct iovec* vector, int count)
{
    
    return redirectWritev(fd, vector, count);
}
