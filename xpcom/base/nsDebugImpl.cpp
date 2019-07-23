







































#include "nsDebugImpl.h"
#include "nsDebug.h"
#include "prprf.h"
#include "prlog.h"
#include "prinit.h"
#include "plstr.h"
#include "nsError.h"
#include "prerror.h"
#include "prerr.h"
#include "prenv.h"
#include "pratom.h"

#if defined(XP_BEOS)

#include <Debug.h>
#endif

#if defined(XP_UNIX) || defined(_WIN32) || defined(XP_OS2) || defined(XP_BEOS)

#include <stdlib.h>
#endif

#include "nsTraceRefcntImpl.h"
#include "nsISupportsUtils.h"

#if defined(XP_UNIX)
#include <signal.h>
#endif

#if defined(XP_WIN)
#include <tchar.h>
#include "nsString.h"
#endif

static void
Abort(const char *aMsg);

static void
Break(const char *aMsg);

#if defined(XP_OS2)
#  define INCL_WINDIALOGS
#  include <os2.h>
#  include <string.h>
#endif 

#if defined(_WIN32)
#include <windows.h>
#include <signal.h>
#include <malloc.h> 
#elif defined(XP_UNIX)
#include <stdlib.h>
#endif

static PRInt32 gAssertionCount = 0;

NS_IMPL_QUERY_INTERFACE2(nsDebugImpl, nsIDebug, nsIDebug2)

NS_IMETHODIMP_(nsrefcnt)
nsDebugImpl::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsDebugImpl::Release()
{
  return 1;
}

NS_IMETHODIMP
nsDebugImpl::Assertion(const char *aStr, const char *aExpr,
                       const char *aFile, PRInt32 aLine)
{
  NS_DebugBreak(NS_DEBUG_ASSERTION, aStr, aExpr, aFile, aLine);
  return NS_OK;
}

NS_IMETHODIMP
nsDebugImpl::Warning(const char *aStr, const char *aFile, PRInt32 aLine)
{
  NS_DebugBreak(NS_DEBUG_WARNING, aStr, nsnull, aFile, aLine);
  return NS_OK;
}

NS_IMETHODIMP
nsDebugImpl::Break(const char *aFile, PRInt32 aLine)
{
  NS_DebugBreak(NS_DEBUG_BREAK, nsnull, nsnull, aFile, aLine);
  return NS_OK;
}

NS_IMETHODIMP
nsDebugImpl::Abort(const char *aFile, PRInt32 aLine)
{
  NS_DebugBreak(NS_DEBUG_ABORT, nsnull, nsnull, aFile, aLine);
  return NS_OK;
}

NS_IMETHODIMP
nsDebugImpl::GetIsDebugBuild(PRBool* aResult)
{
#ifdef DEBUG
  *aResult = PR_TRUE;
#else
  *aResult = PR_FALSE;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsDebugImpl::GetAssertionCount(PRInt32* aResult)
{
  *aResult = gAssertionCount;
  return NS_OK;
}






static PRLogModuleInfo* gDebugLog;

static void InitLog(void)
{
  if (0 == gDebugLog) {
    gDebugLog = PR_NewLogModule("nsDebug");
    gDebugLog->level = PR_LOG_DEBUG;
  }
}

enum nsAssertBehavior {
  NS_ASSERT_UNINITIALIZED,
  NS_ASSERT_WARN,
  NS_ASSERT_SUSPEND,
  NS_ASSERT_STACK,
  NS_ASSERT_TRAP,
  NS_ASSERT_ABORT,
  NS_ASSERT_STACK_AND_ABORT
};

static nsAssertBehavior GetAssertBehavior()
{
  static nsAssertBehavior gAssertBehavior = NS_ASSERT_UNINITIALIZED;
  if (gAssertBehavior != NS_ASSERT_UNINITIALIZED)
    return gAssertBehavior;

#if defined(XP_WIN) || defined(XP_OS2)
  gAssertBehavior = NS_ASSERT_TRAP;
#else
  gAssertBehavior = NS_ASSERT_WARN;
#endif

  const char *assertString = PR_GetEnv("XPCOM_DEBUG_BREAK");
  if (!assertString || !*assertString)
    return gAssertBehavior;

   if (!strcmp(assertString, "warn"))
     return gAssertBehavior = NS_ASSERT_WARN;

   if (!strcmp(assertString, "suspend"))
     return gAssertBehavior = NS_ASSERT_SUSPEND;

   if (!strcmp(assertString, "stack"))
     return gAssertBehavior = NS_ASSERT_STACK;

   if (!strcmp(assertString, "abort"))
     return gAssertBehavior = NS_ASSERT_ABORT;

   if (!strcmp(assertString, "trap") || !strcmp(assertString, "break"))
     return gAssertBehavior = NS_ASSERT_TRAP;

   if (!strcmp(assertString, "stack-and-abort"))
     return gAssertBehavior = NS_ASSERT_STACK_AND_ABORT;

   fprintf(stderr, "Unrecognized value of XPCOM_DEBUG_BREAK\n");
   return gAssertBehavior;
}

struct FixedBuffer
{
  FixedBuffer() : curlen(0) { buffer[0] = '\0'; }

  char buffer[1000];
  PRUint32 curlen;
};

static PRIntn
StuffFixedBuffer(void *closure, const char *buf, PRUint32 len)
{
  if (!len)
    return 0;
  
  FixedBuffer *fb = (FixedBuffer*) closure;

  
  if (buf[len - 1] == '\0')
    --len;

  if (fb->curlen + len >= sizeof(fb->buffer))
    len = sizeof(fb->buffer) - fb->curlen - 1;

  if (len) {
    memcpy(fb->buffer + fb->curlen, buf, len);
    fb->curlen += len;
    fb->buffer[fb->curlen] = '\0';
  }

  return len;
}

EXPORT_XPCOM_API(void)
NS_DebugBreak(PRUint32 aSeverity, const char *aStr, const char *aExpr,
              const char *aFile, PRInt32 aLine)
{
   InitLog();

   FixedBuffer buf;
   PRLogModuleLevel ll = PR_LOG_WARNING;
   const char *sevString = "WARNING";

   switch (aSeverity) {
   case NS_DEBUG_ASSERTION:
     sevString = "###!!! ASSERTION";
     ll = PR_LOG_ERROR;
     break;

   case NS_DEBUG_BREAK:
     sevString = "###!!! BREAK";
     ll = PR_LOG_ALWAYS;
     break;

   case NS_DEBUG_ABORT:
     sevString = "###!!! ABORT";
     ll = PR_LOG_ALWAYS;
     break;

   default:
     aSeverity = NS_DEBUG_WARNING;
   };

   PR_sxprintf(StuffFixedBuffer, &buf, "%s: ", sevString);

   if (aStr)
     PR_sxprintf(StuffFixedBuffer, &buf, "%s: ", aStr);

   if (aExpr)
     PR_sxprintf(StuffFixedBuffer, &buf, "'%s', ", aExpr);

   if (aFile)
     PR_sxprintf(StuffFixedBuffer, &buf, "file %s, ", aFile);

   if (aLine != -1)
     PR_sxprintf(StuffFixedBuffer, &buf, "line %d", aLine);

   
   PR_LOG(gDebugLog, ll, ("%s", buf.buffer));
   PR_LogFlush();

   
#if !defined(XP_WIN) && !defined(XP_OS2)
   if (ll != PR_LOG_WARNING)
     fprintf(stderr, "\07");
#endif

   
   fprintf(stderr, "%s\n", buf.buffer);
   fflush(stderr);

   switch (aSeverity) {
   case NS_DEBUG_WARNING:
     return;

   case NS_DEBUG_BREAK:
     Break(buf.buffer);
     return;

   case NS_DEBUG_ABORT:
#ifdef DEBUG
     Break(buf.buffer);
#endif
     nsTraceRefcntImpl::WalkTheStack(stderr);
     Abort(buf.buffer);
     return;
   }

   
   PR_AtomicIncrement(&gAssertionCount);

   switch (GetAssertBehavior()) {
   case NS_ASSERT_WARN:
     return;

   case NS_ASSERT_SUSPEND:
#ifdef XP_UNIX
      fprintf(stderr, "Suspending process; attach with the debugger.\n");
      kill(0, SIGSTOP);
#else
      Break(buf.buffer);
#endif
      return;

   case NS_ASSERT_STACK:
     nsTraceRefcntImpl::WalkTheStack(stderr);
     return;

   case NS_ASSERT_STACK_AND_ABORT:
     nsTraceRefcntImpl::WalkTheStack(stderr);
     

   case NS_ASSERT_ABORT:
     Abort(buf.buffer);
     return;

   case NS_ASSERT_TRAP:
     Break(buf.buffer);
   }   
}

static void
Abort(const char *aMsg)
{
#if defined(_WIN32)

#ifndef WINCE
  
  raise(SIGABRT);
#endif
  
  _exit(3);
#elif defined(XP_UNIX)
  PR_Abort();
#elif defined(XP_BEOS)
  {
#ifndef DEBUG_cls
	DEBUGGER(aMsg);
#endif
  }
#else
  
  Break(aMsg);
#endif

  
  
  gAssertionCount += *((PRInt32 *) 0); 
                                       

  
  PR_ProcessExit(127);
}


static void
Break(const char *aMsg)
{
#if defined(_WIN32)
#ifndef WINCE 
  static int ignoreDebugger;
  if (!ignoreDebugger) {
    const char *shouldIgnoreDebugger = getenv("XPCOM_DEBUG_DLG");
    ignoreDebugger = 1 + (shouldIgnoreDebugger && !strcmp(shouldIgnoreDebugger, "1"));
  }
  if ((ignoreDebugger == 2) || !::IsDebuggerPresent()) {
    DWORD code = IDRETRY;

    




    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    PRUnichar executable[MAX_PATH];
    PRUnichar* pName;

    memset(&pi, 0, sizeof(pi));

    memset(&si, 0, sizeof(si));
    si.cb          = sizeof(si);
    si.wShowWindow = SW_SHOW;

    
    PRUnichar *msgCopy = (PRUnichar*) _alloca((strlen(aMsg) + 1)*sizeof(PRUnichar));
    wcscpy(msgCopy  , (PRUnichar*)NS_ConvertUTF8toUTF16(aMsg).get());

    if(GetModuleFileNameW(GetModuleHandleW(L"xpcom.dll"), (LPWCH)executable, MAX_PATH) &&
       NULL != (pName = wcsrchr(executable, '\\')) &&
       NULL != 
       wcscpy((WCHAR*)
       pName+1, L"windbgdlg.exe") &&
       CreateProcessW((LPCWSTR)executable, (LPWSTR)msgCopy, NULL, NULL, PR_FALSE,
                     DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                     NULL, NULL, &si, &pi)) {
      WaitForSingleObject(pi.hProcess, INFINITE);
      GetExitCodeProcess(pi.hProcess, &code);
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }

    switch(code) {
    case IDABORT:
      
      raise(SIGABRT);
      
      _exit(3);
         
    case IDIGNORE:
      return;
    }
  }

  ::DebugBreak();

#endif 
#elif defined(XP_OS2)
   char msg[1200];
   PR_snprintf(msg, sizeof(msg),
               "%s\n\nClick Cancel to Debug Application.\n"
               "Click Enter to continue running the Application.", aMsg);
   ULONG code = MBID_ERROR;
   code = WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, msg, 
                        "NSGlue_Assertion", 0,
                        MB_ERROR | MB_ENTERCANCEL);

   







   if (( code == MBID_ENTER ) || (code == MBID_ERROR))
     return;

   asm("int $3");
#elif defined(XP_BEOS)
   DEBUGGER(aMsg);
#elif defined(XP_MACOSX)
   



   raise(SIGTRAP);
#elif defined(__GNUC__) && (defined(__i386__) || defined(__i386) || defined(__x86_64__))
   asm("int $3");
#else
   
#endif
}

static const nsDebugImpl kImpl;

NS_METHOD
nsDebugImpl::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
  NS_ENSURE_NO_AGGREGATION(outer);

  return const_cast<nsDebugImpl*>(&kImpl)->
    QueryInterface(aIID, aInstancePtr);
}



NS_COM nsresult
NS_ErrorAccordingToNSPR()
{
    PRErrorCode err = PR_GetError();
    switch (err) {
      case PR_OUT_OF_MEMORY_ERROR:              return NS_ERROR_OUT_OF_MEMORY;
      case PR_WOULD_BLOCK_ERROR:                return NS_BASE_STREAM_WOULD_BLOCK;
      case PR_FILE_NOT_FOUND_ERROR:             return NS_ERROR_FILE_NOT_FOUND;
      case PR_READ_ONLY_FILESYSTEM_ERROR:       return NS_ERROR_FILE_READ_ONLY;
      case PR_NOT_DIRECTORY_ERROR:              return NS_ERROR_FILE_NOT_DIRECTORY;
      case PR_IS_DIRECTORY_ERROR:               return NS_ERROR_FILE_IS_DIRECTORY;
      case PR_LOOP_ERROR:                       return NS_ERROR_FILE_UNRESOLVABLE_SYMLINK;
      case PR_FILE_EXISTS_ERROR:                return NS_ERROR_FILE_ALREADY_EXISTS;
      case PR_FILE_IS_LOCKED_ERROR:             return NS_ERROR_FILE_IS_LOCKED;
      case PR_FILE_TOO_BIG_ERROR:               return NS_ERROR_FILE_TOO_BIG;
      case PR_NO_DEVICE_SPACE_ERROR:            return NS_ERROR_FILE_NO_DEVICE_SPACE;
      case PR_NAME_TOO_LONG_ERROR:              return NS_ERROR_FILE_NAME_TOO_LONG;
      case PR_DIRECTORY_NOT_EMPTY_ERROR:        return NS_ERROR_FILE_DIR_NOT_EMPTY;
      case PR_NO_ACCESS_RIGHTS_ERROR:           return NS_ERROR_FILE_ACCESS_DENIED;
      default:                                  return NS_ERROR_FAILURE;
    }
}



