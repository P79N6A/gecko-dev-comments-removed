





































#define XPCOM_TRANSLATE_NSGM_ENTRY_POINT
#include "base/histogram.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"
#include "nsIXPConnect.h"
#include "mozilla/Services.h"
#include "jsapi.h" 
#include "nsStringGlue.h"
#include "nsITelemetry.h"
#include "nsXULAppAPI.h"

namespace {

using namespace base;

class Telemetry : public nsITelemetry
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEMETRY

public:
  static Telemetry* GetSingleton();
};


StatisticsRecorder gStatisticsRecorder;

bool
FillRanges(JSContext *cx, JSObject *array, Histogram *h)
{
  for (size_t i = 0;i < h->bucket_count();i++) {
    if (!JS_DefineElement(cx, array, i, INT_TO_JSVAL(h->ranges(i)), NULL, NULL, JSPROP_ENUMERATE))
      return false;
  }
  return true;
}

JSBool
ReflectHistogramSnapshot(JSContext *cx, JSObject *obj, Histogram *h)
{
  Histogram::SampleSet ss;
  h->SnapshotSample(&ss);
  JSObject *counts_array;
  JSObject *rarray;
  const size_t count = h->bucket_count();
  if (!(JS_DefineProperty(cx, obj, "min", INT_TO_JSVAL(h->declared_min()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "max", INT_TO_JSVAL(h->declared_max()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "histogram_type", INT_TO_JSVAL(h->histogram_type()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "sum", INT_TO_JSVAL(ss.sum()), NULL, NULL, JSPROP_ENUMERATE)
        && (rarray = JS_NewArrayObject(cx, count, NULL))
        && JS_DefineProperty(cx, obj, "ranges", OBJECT_TO_JSVAL(rarray), NULL, NULL, JSPROP_ENUMERATE)
        && FillRanges(cx, rarray, h)
        && (counts_array = JS_NewArrayObject(cx, count, NULL))
        && JS_DefineProperty(cx, obj, "counts", OBJECT_TO_JSVAL(counts_array), NULL, NULL, JSPROP_ENUMERATE)
        )) {
    return JS_FALSE;
  }
  for (size_t i = 0;i < count;i++) {
    if (!JS_DefineElement(cx, counts_array, i, INT_TO_JSVAL(ss.counts(i)), NULL, NULL, JSPROP_ENUMERATE)) {
      return JS_FALSE;
    }
  }
  return JS_TRUE;
}

JSBool
JSHistogram_Add(JSContext *cx, uintN argc, jsval *vp)
{
  jsval *argv = JS_ARGV(cx, vp);
  JSString *str;
  if (!JS_ConvertArguments(cx, argc, argv, "i", &str))
    return JS_FALSE;
  if (!JSVAL_IS_INT(argv[0]))
    return JS_FALSE;
  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  Histogram *h = static_cast<Histogram*>(JS_GetPrivate(cx, obj));
  h->Add(JSVAL_TO_INT(argv[0]));
  return JS_TRUE;
}

JSBool
JSHistogram_Snapshot(JSContext *cx, uintN argc, jsval *vp)
{
  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  Histogram *h = static_cast<Histogram*>(JS_GetPrivate(cx, obj));
  JSObject *snapshot = JS_NewObject(cx, NULL, NULL, NULL);
  if (!snapshot)
    return NS_ERROR_FAILURE;
  JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(snapshot));
  return ReflectHistogramSnapshot(cx, snapshot, h);
}

nsresult 
WrapAndReturnHistogram(Histogram *h, JSContext *cx, jsval *ret)
{
  static JSClass JSHistogram_class = {
    "JSHistogram",  
    JSCLASS_HAS_PRIVATE, 
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };

  JSObject *obj = JS_NewObject(cx, &JSHistogram_class, NULL, NULL);
  if (!obj)
    return NS_ERROR_FAILURE;
  *ret = OBJECT_TO_JSVAL(obj);
  return (JS_SetPrivate(cx, obj, h)
          && JS_DefineFunction (cx, obj, "add", JSHistogram_Add, 1, 0)
          && JS_DefineFunction (cx, obj, "snapshot", JSHistogram_Snapshot, 1, 0)) ? NS_OK : NS_ERROR_FAILURE;
}
  
static nsresult
HistogramGet(const char *name, PRUint32 min, PRUint32 max, PRUint32 bucket_count,
             PRUint32 histogram_type, Histogram **aResult)
{
  if (min < 1)
    return NS_ERROR_ILLEGAL_VALUE;

  if (min >= max)
    return NS_ERROR_ILLEGAL_VALUE;

  if (bucket_count <= 2)
    return NS_ERROR_ILLEGAL_VALUE;

  if (histogram_type == nsITelemetry::HISTOGRAM_EXPONENTIAL) {
    *aResult = Histogram::FactoryGet(name, min, max, bucket_count, Histogram::kNoFlags);
  } else {
    *aResult = LinearHistogram::FactoryGet(name, min, max, bucket_count, Histogram::kNoFlags);
  }
  return NS_OK;
}

NS_IMETHODIMP
Telemetry::NewHistogram(const nsACString &name, PRUint32 min, PRUint32 max, PRUint32 bucket_count, PRUint32 histogram_type, JSContext *cx, jsval *ret)
{
  Histogram *h;
  nsresult rv = HistogramGet(name.BeginReading(), min, max, bucket_count, histogram_type, &h);
  if (NS_FAILED(rv))
    return rv;
  
  return WrapAndReturnHistogram(h, cx, ret);
}

NS_IMETHODIMP
Telemetry::GetHistogramSnapshots(JSContext *cx, jsval *ret)
{
  JSObject *root_obj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!root_obj)
    return NS_ERROR_FAILURE;
  *ret = OBJECT_TO_JSVAL(root_obj);

  StatisticsRecorder::Histograms h;
  StatisticsRecorder::GetHistograms(&h);
  for (StatisticsRecorder::Histograms::iterator it = h.begin(); it != h.end();++it) {
    Histogram *h = *it;
    JSObject *hobj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!(hobj
          && JS_DefineProperty(cx, root_obj, h->histogram_name().c_str(),
                               OBJECT_TO_JSVAL(hobj), NULL, NULL, JSPROP_ENUMERATE)
          && ReflectHistogramSnapshot(cx, hobj, h))) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(Telemetry, nsITelemetry)

Telemetry *gJarHandler = nsnull;

void ShutdownTelemetry()
{
  NS_IF_RELEASE(gJarHandler);
}

Telemetry* Telemetry::GetSingleton()
{
  if (!gJarHandler) {
    gJarHandler = new Telemetry();
    NS_ADDREF(gJarHandler);
  }
  NS_ADDREF(gJarHandler);
  return gJarHandler;
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(Telemetry, Telemetry::GetSingleton)

#define NS_TELEMETRY_CID \
  {0xaea477f2, 0xb3a2, 0x469c, {0xaa, 0x29, 0x0a, 0x82, 0xd1, 0x32, 0xb8, 0x29}}
NS_DEFINE_NAMED_CID(NS_TELEMETRY_CID);

const mozilla::Module::CIDEntry kTelemetryCIDs[] = {
  { &kNS_TELEMETRY_CID, false, NULL, TelemetryConstructor },
  { NULL }
};

const mozilla::Module::ContractIDEntry kTelemetryContracts[] = {
  { "@mozilla.org/base/telemetry;1", &kNS_TELEMETRY_CID },
  { NULL }
};

const mozilla::Module kTelemetryModule = {
  mozilla::Module::kVersion,
  kTelemetryCIDs,
  kTelemetryContracts,
  NULL,
  NULL,
  NULL,
  ShutdownTelemetry,
};

} 

NSMODULE_DEFN(nsTelemetryModule) = &kTelemetryModule;





nsresult
XRE_TelemetryAdd(const char *name, int sample, PRUint32 min, PRUint32 max,
                 PRUint32 bucket_count, HistogramTypes histogram_type)
{
  base::Histogram *h;
  nsresult rv = HistogramGet(name, min, max, bucket_count, histogram_type, &h);
  if (NS_FAILED(rv))
    return rv;
  h->Add(sample);
  return NS_OK;
}
