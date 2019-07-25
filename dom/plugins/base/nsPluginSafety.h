




































#ifndef nsPluginSafety_h_
#define nsPluginSafety_h_

#include "npapi.h"
#include "nsPluginHost.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include <prinrval.h>

#if defined(XP_WIN)
#define CALL_SAFETY_ON
#endif

void NS_NotifyPluginCall(PRIntervalTime);

#ifdef CALL_SAFETY_ON

extern bool gSkipPluginSafeCalls;

#define NS_INIT_PLUGIN_SAFE_CALLS                               \
PR_BEGIN_MACRO                                                  \
  nsresult res;                                                 \
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &res)); \
  if(NS_SUCCEEDED(res) && pref)                                 \
    res = pref->GetBoolPref("plugin.dont_try_safe_calls", &gSkipPluginSafeCalls);\
PR_END_MACRO

#define NS_TRY_SAFE_CALL_RETURN(ret, fun, pluginInst) \
PR_BEGIN_MACRO                                     \
  PRIntervalTime startTime = PR_IntervalNow();     \
  if(gSkipPluginSafeCalls)                         \
    ret = fun;                                     \
  else                                             \
  {                                                \
    MOZ_SEH_TRY                                    \
    {                                              \
      ret = fun;                                   \
    }                                              \
    MOZ_SEH_EXCEPT(true)                        \
    {                                              \
      nsresult res;                                \
      nsCOMPtr<nsIPluginHost> host(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &res));\
      if(NS_SUCCEEDED(res) && (host != nsnull))    \
        static_cast<nsPluginHost*>(host.get())->HandleBadPlugin(nsnull, pluginInst); \
      ret = (NPError)NS_ERROR_FAILURE;             \
    }                                              \
  }                                                \
  NS_NotifyPluginCall(startTime);		   \
PR_END_MACRO

#define NS_TRY_SAFE_CALL_VOID(fun, pluginInst) \
PR_BEGIN_MACRO                              \
  PRIntervalTime startTime = PR_IntervalNow();     \
  if(gSkipPluginSafeCalls)                  \
    fun;                                    \
  else                                      \
  {                                         \
    MOZ_SEH_TRY                             \
    {                                       \
      fun;                                  \
    }                                       \
    MOZ_SEH_EXCEPT(true)                 \
    {                                       \
      nsresult res;                         \
      nsCOMPtr<nsIPluginHost> host(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &res));\
      if(NS_SUCCEEDED(res) && (host != nsnull))\
        static_cast<nsPluginHost*>(host.get())->HandleBadPlugin(nsnull, pluginInst);\
    }                                       \
  }                                         \
  NS_NotifyPluginCall(startTime);		   \
PR_END_MACRO

#else 

#define NS_TRY_SAFE_CALL_RETURN(ret, fun, pluginInst) \
PR_BEGIN_MACRO                                     \
  PRIntervalTime startTime = PR_IntervalNow();     \
  ret = fun;                                       \
  NS_NotifyPluginCall(startTime);		   \
PR_END_MACRO

#define NS_TRY_SAFE_CALL_VOID(fun, pluginInst) \
PR_BEGIN_MACRO                              \
  PRIntervalTime startTime = PR_IntervalNow();     \
  fun;                                      \
  NS_NotifyPluginCall(startTime);		   \
PR_END_MACRO

#endif 

#endif 
