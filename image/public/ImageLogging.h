





#ifndef ImageLogging_h
#define ImageLogging_h



#if defined(PR_LOG)
#error "Must #include ImageLogging.h before before any IPDL-generated files or other files that #include prlog.h."
#endif

#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#include "prlog.h"
#include "prinrval.h"
#include "nsString.h"

#if defined(PR_LOGGING)

extern PRLogModuleInfo *GetImgLog();

#define GIVE_ME_MS_NOW() PR_IntervalToMilliseconds(PR_IntervalNow())

class LogScope {
public:
  LogScope(PRLogModuleInfo *aLog, void *from, const char *fn) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s {ENTER}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const char *fn,
           const char *paramName, const char *paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%s\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc,
                                   paramName, paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const char *fn,
           const char *paramName, const void *paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=%p) {ENTER}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc,
                                   paramName, paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const char *fn,
           const char *paramName, int32_t paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc,
                                   paramName, paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const char *fn,
           const char *paramName, uint32_t paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc,
                                   paramName, paramValue));
  }


  ~LogScope() {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s {EXIT}\n",
                                   GIVE_ME_MS_NOW(), mFrom, mFunc));
  }

private:
  PRLogModuleInfo *mLog;
  void *mFrom;
  const char *mFunc;
};


class LogFunc {
public:
  LogFunc(PRLogModuleInfo *aLog, void *from, const char *fn)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s\n",
                                GIVE_ME_MS_NOW(), from, fn));
  }

  LogFunc(PRLogModuleInfo *aLog, void *from, const char *fn,
          const char *paramName, const char *paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%s\")\n",
                                GIVE_ME_MS_NOW(), from, fn,
                                paramName, paramValue));
  }

  LogFunc(PRLogModuleInfo *aLog, void *from, const char *fn,
          const char *paramName, const void *paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%p\")\n",
                                GIVE_ME_MS_NOW(), from, fn,
                                paramName, paramValue));
  }


  LogFunc(PRLogModuleInfo *aLog, void *from, const char *fn,
          const char *paramName, uint32_t paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\")\n",
                                GIVE_ME_MS_NOW(), from,
                                fn,
                                paramName, paramValue));
  }

};


class LogMessage {
public:
  LogMessage(PRLogModuleInfo *aLog, void *from, const char *fn,
             const char *msg)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s -- %s\n",
                                GIVE_ME_MS_NOW(), from, fn, msg));
  }
};

#define LOG_SCOPE_APPEND_LINE_NUMBER_PASTE(id, line) id ## line
#define LOG_SCOPE_APPEND_LINE_NUMBER_EXPAND(id, line) LOG_SCOPE_APPEND_LINE_NUMBER_PASTE(id, line)
#define LOG_SCOPE_APPEND_LINE_NUMBER(id) LOG_SCOPE_APPEND_LINE_NUMBER_EXPAND(id, __LINE__)

#define LOG_SCOPE(l, s) \
  LogScope LOG_SCOPE_APPEND_LINE_NUMBER(LOG_SCOPE_TMP_VAR) (l, this, s)

#define LOG_SCOPE_WITH_PARAM(l, s, pn, pv) \
  LogScope LOG_SCOPE_APPEND_LINE_NUMBER(LOG_SCOPE_TMP_VAR) (l, this, s, pn, pv)

#define LOG_FUNC(l, s) LogFunc(l, this, s)

#define LOG_FUNC_WITH_PARAM(l, s, pn, pv) LogFunc(l, this, s, pn, pv)

#define LOG_STATIC_FUNC(l, s) LogFunc(l, nullptr, s)

#define LOG_STATIC_FUNC_WITH_PARAM(l, s, pn, pv) LogFunc(l, nullptr, s, pn, pv)



#define LOG_MSG(l, s, m) LogMessage(l, this, s, m)

#else

#define LOG_SCOPE(l, s)
#define LOG_SCOPE_WITH_PARAM(l, s, pn, pv)
#define LOG_FUNC(l, s)
#define LOG_FUNC_WITH_PARAM(l, s, pn, pv)
#define LOG_STATIC_FUNC(l, s)
#define LOG_STATIC_FUNC_WITH_PARAM(l, s, pn, pv)
#define LOG_MSG(l, s, m)

#endif 

#define LOG_MSG_WITH_PARAM LOG_FUNC_WITH_PARAM

#endif 
