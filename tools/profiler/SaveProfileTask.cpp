




#include "SaveProfileTask.h"
#include "GeckoProfilerImpl.h"

static JSBool
WriteCallback(const jschar *buf, uint32_t len, void *data)
{
  std::ofstream& stream = *static_cast<std::ofstream*>(data);
  nsAutoCString profile = NS_ConvertUTF16toUTF8(buf, len);
  stream << profile.Data();
  return true;
}

nsresult
SaveProfileTask::Run() {
  
#if defined(SPS_PLAT_arm_android) && !defined(MOZ_WIDGET_GONK)
  nsCString tmpPath;
  tmpPath.AppendPrintf("/sdcard/profile_%i_%i.txt", XRE_GetProcessType(), getpid());
#else
  nsCOMPtr<nsIFile> tmpFile;
  nsAutoCString tmpPath;
  if (NS_FAILED(NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile)))) {
    LOG("Failed to find temporary directory.");
    return NS_ERROR_FAILURE;
  }
  tmpPath.AppendPrintf("profile_%i_%i.txt", XRE_GetProcessType(), getpid());

  nsresult rv = tmpFile->AppendNative(tmpPath);
  if (NS_FAILED(rv))
    return rv;

  rv = tmpFile->GetNativePath(tmpPath);
  if (NS_FAILED(rv))
    return rv;
#endif

  
  
  JSRuntime *rt;
  JSContext *cx;
  nsCOMPtr<nsIJSRuntimeService> rtsvc
    = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
  if (!rtsvc || NS_FAILED(rtsvc->GetRuntime(&rt)) || !rt) {
    LOG("failed to get RuntimeService");
    return NS_ERROR_FAILURE;;
  }

  cx = JS_NewContext(rt, 8192);
  if (!cx) {
    LOG("Failed to get context");
    return NS_ERROR_FAILURE;
  }

  {
    JSAutoRequest ar(cx);
    static JSClass c = {
      "global", JSCLASS_GLOBAL_FLAGS,
      JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
      JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
    };
    JSObject *obj = JS_NewGlobalObject(cx, &c, NULL, JS::FireOnNewGlobalHook);

    std::ofstream stream;
    stream.open(tmpPath.get());
    if (stream.is_open()) {
      JSAutoCompartment autoComp(cx, obj);
      JSObject* profileObj = profiler_get_profile_jsobject(cx);
      JS::Rooted<JS::Value> val(cx, OBJECT_TO_JSVAL(profileObj));
      JS_Stringify(cx, val.address(), nullptr, JSVAL_NULL, WriteCallback, &stream);
      stream.close();
      LOGF("Saved to %s", tmpPath.get());
    } else {
      LOG("Fail to open profile log file.");
    }
  }
  JS_DestroyContext(cx);

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(ProfileSaveEvent, nsIProfileSaveEvent)

nsresult
ProfileSaveEvent::AddSubProfile(const char* aProfile) {
  mFunc(aProfile, mClosure);
  return NS_OK;
}

