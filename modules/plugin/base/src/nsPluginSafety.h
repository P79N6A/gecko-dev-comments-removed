




































#ifndef nsPluginSafety_h__
#define nsPluginSafety_h__

#include "npapi.h"
#include "nsIPluginHost.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#if defined(XP_WIN) && !defined(WINCE)
#define CALL_SAFETY_ON
#endif

#ifdef CALL_SAFETY_ON

extern PRBool gSkipPluginSafeCalls;

#define NS_INIT_PLUGIN_SAFE_CALLS                               \
PR_BEGIN_MACRO                                                  \
  nsresult res;                                                 \
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &res)); \
  if(NS_SUCCEEDED(res) && pref)                                 \
    res = pref->GetBoolPref("plugin.dont_try_safe_calls", &gSkipPluginSafeCalls);\
PR_END_MACRO

#define NS_TRY_SAFE_CALL_RETURN(ret, fun, library, pluginInst) \
PR_BEGIN_MACRO                                     \
  if(gSkipPluginSafeCalls)                         \
    ret = fun;                                     \
  else                                             \
  {                                                \
    try                                            \
    {                                              \
      ret = fun;                                   \
    }                                              \
    catch(...)                                     \
    {                                              \
      nsresult res;                                \
      nsCOMPtr<nsIPluginHost> host(do_GetService(kCPluginManagerCID, &res));\
      if(NS_SUCCEEDED(res) && (host != nsnull))    \
        host->HandleBadPlugin(library, pluginInst);\
      ret = (NPError)NS_ERROR_FAILURE;             \
    }                                              \
  }                                                \
PR_END_MACRO

#define NS_TRY_SAFE_CALL_VOID(fun, library, pluginInst) \
PR_BEGIN_MACRO                              \
  if(gSkipPluginSafeCalls)                  \
    fun;                                    \
  else                                      \
  {                                         \
    try                                     \
    {                                       \
      fun;                                  \
    }                                       \
    catch(...)                              \
    {                                       \
      nsresult res;                         \
      nsCOMPtr<nsIPluginHost> host(do_GetService(kCPluginManagerCID, &res));\
      if(NS_SUCCEEDED(res) && (host != nsnull))\
        host->HandleBadPlugin(library, pluginInst);\
    }                                       \
  }                                         \
PR_END_MACRO

#else 

#define NS_TRY_SAFE_CALL_RETURN(ret, fun, library, pluginInst) \
PR_BEGIN_MACRO                                     \
  ret = fun;                                       \
PR_END_MACRO

#define NS_TRY_SAFE_CALL_VOID(fun, library, pluginInst) \
PR_BEGIN_MACRO                              \
  fun;                                      \
PR_END_MACRO

#endif 

#endif 

