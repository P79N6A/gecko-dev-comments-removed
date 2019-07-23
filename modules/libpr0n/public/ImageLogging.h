






































#include "prlog.h"
#include "prinrval.h"

#include "nsString.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo *gImgLog;

#define GIVE_ME_MS_NOW() PR_IntervalToMilliseconds(PR_IntervalNow())

class LogScope {
public:
  LogScope(PRLogModuleInfo *aLog, void *from, const nsACString &fn) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s {ENTER}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get()));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const nsACString &fn,
           const nsDependentCString &paramName, const char *paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%s\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get(),
                                   paramName.get(),
                                   paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const nsACString &fn,
           const nsDependentCString &paramName, const void *paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=%p) {ENTER}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get(),
                                   paramName.get(),
                                   paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const nsACString &fn,
           const nsDependentCString &paramName, PRInt32 paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get(),
                                   paramName.get(),
                                   paramValue));
  }

  
  LogScope(PRLogModuleInfo *aLog, void *from, const nsACString &fn,
           const nsDependentCString &paramName, PRUint32 paramValue) :
    mLog(aLog), mFrom(from), mFunc(fn)
  {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\") {ENTER}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get(),
                                   paramName.get(),
                                   paramValue));
  }


  ~LogScope() {
    PR_LOG(mLog, PR_LOG_DEBUG, ("%d [this=%p] %s {EXIT}\n",
                                   GIVE_ME_MS_NOW(),
                                   mFrom, mFunc.get()));
  }

private:
  PRLogModuleInfo *mLog;
  void *mFrom;
  nsCAutoString mFunc;
};


class LogFunc {
public:
  LogFunc(PRLogModuleInfo *aLog, void *from, const nsDependentCString &fn)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s\n",
                                GIVE_ME_MS_NOW(), from,
                                fn.get()));
  }

  LogFunc(PRLogModuleInfo *aLog, void *from, const nsDependentCString &fn,
          const nsDependentCString &paramName, const char *paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%s\")\n",
                                GIVE_ME_MS_NOW(), from,
                                fn.get(),
                                paramName.get(), paramValue));
  }

  LogFunc(PRLogModuleInfo *aLog, void *from, const nsDependentCString &fn,
          const nsDependentCString &paramName, const void *paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%p\")\n",
                                GIVE_ME_MS_NOW(), from,
                                fn.get(),
                                paramName.get(), paramValue));
  }


  LogFunc(PRLogModuleInfo *aLog, void *from, const nsDependentCString &fn,
          const nsDependentCString &paramName, PRUint32 paramValue)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s (%s=\"%d\")\n",
                                GIVE_ME_MS_NOW(), from,
                                fn.get(),
                                paramName.get(), paramValue));
  }

};


class LogMessage {
public:
  LogMessage(PRLogModuleInfo *aLog, void *from, const nsDependentCString &fn,
             const nsDependentCString &msg)
  {
    PR_LOG(aLog, PR_LOG_DEBUG, ("%d [this=%p] %s -- %s\n",
                                GIVE_ME_MS_NOW(), from,
                                fn.get(),
                                msg.get()));
  }
};

#define LOG_SCOPE(l, s) \
  LogScope LOG_SCOPE_TMP_VAR ##__LINE__ (l,                            \
                                         static_cast<void *>(this), \
                                         NS_LITERAL_CSTRING(s))

#define LOG_SCOPE_WITH_PARAM(l, s, pn, pv) \
  LogScope LOG_SCOPE_TMP_VAR ##__LINE__ (l,                            \
                                         static_cast<void *>(this), \
                                         NS_LITERAL_CSTRING(s),        \
                                         NS_LITERAL_CSTRING(pn), pv)

#define LOG_FUNC(l, s)                  \
  LogFunc(l,                            \
          static_cast<void *>(this), \
          NS_LITERAL_CSTRING(s))

#define LOG_FUNC_WITH_PARAM(l, s, pn, pv) \
  LogFunc(l,                              \
          static_cast<void *>(this),   \
          NS_LITERAL_CSTRING(s),          \
          NS_LITERAL_CSTRING(pn), pv)

#define LOG_STATIC_FUNC(l, s)           \
  LogFunc(l,                            \
          nsnull,                       \
          NS_LITERAL_CSTRING(s))

#define LOG_STATIC_FUNC_WITH_PARAM(l, s, pn, pv) \
  LogFunc(l,                             \
          nsnull,                        \
          NS_LITERAL_CSTRING(s),         \
          NS_LITERAL_CSTRING(pn), pv)



#define LOG_MSG(l, s, m)                   \
  LogMessage(l,                            \
             static_cast<void *>(this), \
             NS_LITERAL_CSTRING(s),        \
             NS_LITERAL_CSTRING(m))


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
