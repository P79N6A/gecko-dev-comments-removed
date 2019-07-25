






































#include "base/histogram.h"
#include "base/pickle.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"
#include "nsIXPConnect.h"
#include "mozilla/Services.h"
#include "jsapi.h" 
#include "nsStringGlue.h"
#include "nsITelemetry.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "Telemetry.h" 
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsBaseHashtable.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "mozilla/Mutex.h"
#include "mozilla/FileUtils.h"

namespace {

using namespace base;
using namespace mozilla;

class TelemetryImpl : public nsITelemetry
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEMETRY

public:
  TelemetryImpl();
  ~TelemetryImpl();
  
  static bool CanRecord();
  static already_AddRefed<nsITelemetry> CreateTelemetryInstance();
  static void ShutdownTelemetry();
  static void RecordSlowStatement(const nsACString &statement,
                                  const nsACString &dbName,
                                  PRUint32 delay);
  static nsresult GetHistogramEnumId(const char *name, Telemetry::ID *id);
  struct StmtStats {
    PRUint32 hitCount;
    PRUint32 totalTime;
  };
  typedef nsBaseHashtableET<nsCStringHashKey, StmtStats> SlowSQLEntryType;

private:
  bool AddSQLInfo(JSContext *cx, JSObject *rootObj, bool mainThread);

  
  nsresult GetHistogramByName(const nsACString &name, Histogram **ret);
  bool ShouldReflectHistogram(Histogram *h);
  void IdentifyCorruptHistograms(StatisticsRecorder::Histograms &hs);
  typedef StatisticsRecorder::Histograms::iterator HistogramIterator;
  
  typedef nsBaseHashtableET<nsCharPtrHashKey, Telemetry::ID> CharPtrEntryType;
  typedef nsTHashtable<CharPtrEntryType> HistogramMapType;
  HistogramMapType mHistogramMap;
  bool mCanRecord;
  static TelemetryImpl *sTelemetry;
  nsTHashtable<SlowSQLEntryType> mSlowSQLOnMainThread;
  nsTHashtable<SlowSQLEntryType> mSlowSQLOnOtherThread;
  nsTHashtable<nsCStringHashKey> mTrackedDBs;
  Mutex mHashMutex;
};

TelemetryImpl*  TelemetryImpl::sTelemetry = NULL;


StatisticsRecorder gStatisticsRecorder;


struct TelemetryHistogram {
  const char *id;
  PRUint32 min;
  PRUint32 max;
  PRUint32 bucketCount;
  PRUint32 histogramType;
  const char *comment;
};




#define HISTOGRAM(id, min, max, bucket_count, histogram_type, b) \
  PR_STATIC_ASSERT(nsITelemetry::HISTOGRAM_ ## histogram_type == nsITelemetry::HISTOGRAM_BOOLEAN || \
                   (min < max && bucket_count > 2 && min >= 1));

#include "TelemetryHistograms.h"

#undef HISTOGRAM

const TelemetryHistogram gHistograms[] = {
#define HISTOGRAM(id, min, max, bucket_count, histogram_type, comment) \
  { NS_STRINGIFY(id), min, max, bucket_count, \
    nsITelemetry::HISTOGRAM_ ## histogram_type, comment },

#include "TelemetryHistograms.h"

#undef HISTOGRAM
};
bool gCorruptHistograms[Telemetry::HistogramCount];

bool
TelemetryHistogramType(Histogram *h, PRUint32 *result)
{
  switch (h->histogram_type()) {
  case Histogram::HISTOGRAM:
    *result = nsITelemetry::HISTOGRAM_EXPONENTIAL;
    break;
  case Histogram::LINEAR_HISTOGRAM:
    *result = nsITelemetry::HISTOGRAM_LINEAR;
    break;
  case Histogram::BOOLEAN_HISTOGRAM:
    *result = nsITelemetry::HISTOGRAM_BOOLEAN;
    break;
  default:
    return false;
  }
  return true;
}

nsresult
HistogramGet(const char *name, PRUint32 min, PRUint32 max, PRUint32 bucketCount,
             PRUint32 histogramType, Histogram **result)
{
  if (histogramType != nsITelemetry::HISTOGRAM_BOOLEAN) {
    
    if (min >= max)
      return NS_ERROR_ILLEGAL_VALUE;

    if (bucketCount <= 2)
      return NS_ERROR_ILLEGAL_VALUE;

    if (min < 1)
      return NS_ERROR_ILLEGAL_VALUE;
  }

  switch (histogramType) {
  case nsITelemetry::HISTOGRAM_EXPONENTIAL:
    *result = Histogram::FactoryGet(name, min, max, bucketCount, Histogram::kUmaTargetedHistogramFlag);
    break;
  case nsITelemetry::HISTOGRAM_LINEAR:
    *result = LinearHistogram::FactoryGet(name, min, max, bucketCount, Histogram::kUmaTargetedHistogramFlag);
    break;
  case nsITelemetry::HISTOGRAM_BOOLEAN:
    *result = BooleanHistogram::FactoryGet(name, Histogram::kUmaTargetedHistogramFlag);
    break;
  default:
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}


nsresult
GetHistogramByEnumId(Telemetry::ID id, Histogram **ret)
{
  static Histogram* knownHistograms[Telemetry::HistogramCount] = {0};
  Histogram *h = knownHistograms[id];
  if (h) {
    *ret = h;
    return NS_OK;
  }

  const TelemetryHistogram &p = gHistograms[id];
  nsresult rv = HistogramGet(p.id, p.min, p.max, p.bucketCount, p.histogramType, &h);
  if (NS_FAILED(rv))
    return rv;

  *ret = knownHistograms[id] = h;
  return NS_OK;
}

bool
FillRanges(JSContext *cx, JSObject *array, Histogram *h)
{
  for (size_t i = 0; i < h->bucket_count(); i++) {
    if (!JS_DefineElement(cx, array, i, INT_TO_JSVAL(h->ranges(i)), NULL, NULL, JSPROP_ENUMERATE))
      return false;
  }
  return true;
}

enum reflectStatus {
  REFLECT_OK,
  REFLECT_CORRUPT,
  REFLECT_FAILURE
};

enum reflectStatus
ReflectHistogramAndSamples(JSContext *cx, JSObject *obj, Histogram *h,
                           const Histogram::SampleSet &ss)
{
  
  if (h->FindCorruption(ss) != Histogram::NO_INCONSISTENCIES) {
    return REFLECT_CORRUPT;
  }

  JSObject *counts_array;
  JSObject *rarray;
  const size_t count = h->bucket_count();
  if (!(JS_DefineProperty(cx, obj, "min", INT_TO_JSVAL(h->declared_min()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "max", INT_TO_JSVAL(h->declared_max()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "histogram_type", INT_TO_JSVAL(h->histogram_type()), NULL, NULL, JSPROP_ENUMERATE)
        && JS_DefineProperty(cx, obj, "sum", DOUBLE_TO_JSVAL(ss.sum()), NULL, NULL, JSPROP_ENUMERATE)
        && (rarray = JS_NewArrayObject(cx, count, NULL))
        && JS_DefineProperty(cx, obj, "ranges", OBJECT_TO_JSVAL(rarray), NULL, NULL, JSPROP_ENUMERATE)
        && FillRanges(cx, rarray, h)
        && (counts_array = JS_NewArrayObject(cx, count, NULL))
        && JS_DefineProperty(cx, obj, "counts", OBJECT_TO_JSVAL(counts_array), NULL, NULL, JSPROP_ENUMERATE)
        )) {
    return REFLECT_FAILURE;
  }
  for (size_t i = 0; i < count; i++) {
    if (!JS_DefineElement(cx, counts_array, i, INT_TO_JSVAL(ss.counts(i)), NULL, NULL, JSPROP_ENUMERATE)) {
      return REFLECT_FAILURE;
    }
  }
  return REFLECT_OK;
}

enum reflectStatus
ReflectHistogramSnapshot(JSContext *cx, JSObject *obj, Histogram *h)
{
  Histogram::SampleSet ss;
  h->SnapshotSample(&ss);
  return ReflectHistogramAndSamples(cx, obj, h, ss);
}

JSBool
JSHistogram_Add(JSContext *cx, uintN argc, jsval *vp)
{
  if (!argc) {
    JS_ReportError(cx, "Expected one argument");
    return JS_FALSE;
  }

  jsval v = JS_ARGV(cx, vp)[0];
  int32 value;

  if (!(JSVAL_IS_NUMBER(v) || JSVAL_IS_BOOLEAN(v))) {
    JS_ReportError(cx, "Not a number");
    return JS_FALSE;
  }

  if (!JS_ValueToECMAInt32(cx, v, &value)) {
    return JS_FALSE;
  }

  if (TelemetryImpl::CanRecord()) {
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj) {
      return JS_FALSE;
    }

    Histogram *h = static_cast<Histogram*>(JS_GetPrivate(cx, obj));
    if (h->histogram_type() == Histogram::BOOLEAN_HISTOGRAM)
      h->Add(!!value);
    else
      h->Add(value);
  }
  return JS_TRUE;
}

JSBool
JSHistogram_Snapshot(JSContext *cx, uintN argc, jsval *vp)
{
  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  if (!obj) {
    return JS_FALSE;
  }

  Histogram *h = static_cast<Histogram*>(JS_GetPrivate(cx, obj));
  JSObject *snapshot = JS_NewObject(cx, NULL, NULL, NULL);
  if (!snapshot)
    return JS_FALSE;

  switch (ReflectHistogramSnapshot(cx, snapshot, h)) {
  case REFLECT_FAILURE:
    return JS_FALSE;
  case REFLECT_CORRUPT:
    JS_ReportError(cx, "Histogram is corrupt");
    return JS_FALSE;
  case REFLECT_OK:
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(snapshot));
    return JS_TRUE;
  default:
    MOZ_NOT_REACHED("unhandled reflection status");
    return JS_FALSE;
  }
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

TelemetryImpl::TelemetryImpl():
mCanRecord(XRE_GetProcessType() == GeckoProcessType_Default),
mHashMutex("Telemetry::mHashMutex")
{
  
  const char *trackedDBs[] = {
    "addons.sqlite", "chromeappsstore.sqlite", "content-prefs.sqlite",
    "cookies.sqlite", "downloads.sqlite", "extensions.sqlite",
    "formhistory.sqlite", "index.sqlite", "permissions.sqlite", "places.sqlite",
    "search.sqlite", "signons.sqlite", "urlclassifier3.sqlite",
    "webappsstore.sqlite"
  };

  mTrackedDBs.Init();
  for (size_t i = 0; i < ArrayLength(trackedDBs); i++)
    mTrackedDBs.PutEntry(nsDependentCString(trackedDBs[i]));

#ifdef DEBUG
  
  mTrackedDBs.MarkImmutable();
#endif

  mSlowSQLOnMainThread.Init();
  mSlowSQLOnOtherThread.Init();
  mHistogramMap.Init(Telemetry::HistogramCount);
}

TelemetryImpl::~TelemetryImpl() {
  mSlowSQLOnMainThread.Clear();
  mSlowSQLOnOtherThread.Clear();
  mHistogramMap.Clear();
}

NS_IMETHODIMP
TelemetryImpl::NewHistogram(const nsACString &name, PRUint32 min, PRUint32 max, PRUint32 bucketCount, PRUint32 histogramType, JSContext *cx, jsval *ret)
{
  Histogram *h;
  nsresult rv = HistogramGet(PromiseFlatCString(name).get(), min, max, bucketCount, histogramType, &h);
  if (NS_FAILED(rv))
    return rv;
  h->ClearFlags(Histogram::kUmaTargetedHistogramFlag);
  return WrapAndReturnHistogram(h, cx, ret);
}

struct EnumeratorArgs {
  JSContext *cx;
  JSObject *statsObj;
};

PLDHashOperator
StatementEnumerator(TelemetryImpl::SlowSQLEntryType *entry, void *arg)
{
  EnumeratorArgs *args = static_cast<EnumeratorArgs *>(arg);
  const nsACString &sql = entry->GetKey();
  jsval hitCount = UINT_TO_JSVAL(entry->mData.hitCount);
  jsval totalTime = UINT_TO_JSVAL(entry->mData.totalTime);

  JSObject *arrayObj = JS_NewArrayObject(args->cx, 2, nsnull);
  if (!arrayObj ||
      !JS_SetElement(args->cx, arrayObj, 0, &hitCount) ||
      !JS_SetElement(args->cx, arrayObj, 1, &totalTime))
    return PL_DHASH_STOP;

  JSBool success = JS_DefineProperty(args->cx, args->statsObj,
                                     sql.BeginReading(),
                                     OBJECT_TO_JSVAL(arrayObj),
                                     NULL, NULL, JSPROP_ENUMERATE);
  if (!success)
    return PL_DHASH_STOP;

  return PL_DHASH_NEXT;
}

bool
TelemetryImpl::AddSQLInfo(JSContext *cx, JSObject *rootObj, bool mainThread)
{
  JSObject *statsObj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!statsObj)
    return false;

  JSBool ok = JS_DefineProperty(cx, rootObj,
                                mainThread ? "mainThread" : "otherThreads",
                                OBJECT_TO_JSVAL(statsObj),
                                NULL, NULL, JSPROP_ENUMERATE);
  if (!ok)
    return false;

  EnumeratorArgs args = { cx, statsObj };
  nsTHashtable<SlowSQLEntryType> *sqlMap;
  sqlMap = (mainThread ? &mSlowSQLOnMainThread : &mSlowSQLOnOtherThread);
  PRUint32 num = sqlMap->EnumerateEntries(StatementEnumerator,
                                          static_cast<void*>(&args));
  if (num != sqlMap->Count())
    return false;

  return true;
}

nsresult
TelemetryImpl::GetHistogramEnumId(const char *name, Telemetry::ID *id)
{
  if (!sTelemetry) {
    return NS_ERROR_FAILURE;
  }

  
  
  TelemetryImpl::HistogramMapType *map = &sTelemetry->mHistogramMap;
  if (!map->Count()) {
    for (PRUint32 i = 0; i < Telemetry::HistogramCount; i++) {
      CharPtrEntryType *entry = map->PutEntry(gHistograms[i].id);
      if (NS_UNLIKELY(!entry)) {
        map->Clear();
        return NS_ERROR_OUT_OF_MEMORY;
      }
      entry->mData = (Telemetry::ID) i;
    }
  }

  CharPtrEntryType *entry = map->GetEntry(name);
  if (!entry) {
    return NS_ERROR_INVALID_ARG;
  }
  *id = entry->mData;
  return NS_OK;
}

nsresult
TelemetryImpl::GetHistogramByName(const nsACString &name, Histogram **ret)
{
  Telemetry::ID id;
  nsresult rv = GetHistogramEnumId(PromiseFlatCString(name).get(), &id);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = GetHistogramByEnumId(id, ret);
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

NS_IMETHODIMP
TelemetryImpl::HistogramFrom(const nsACString &name, const nsACString &existing_name,
                             JSContext *cx, jsval *ret)
{
  Histogram *existing;
  nsresult rv = GetHistogramByName(existing_name, &existing);
  if (NS_FAILED(rv))
    return rv;

  PRUint32 histogramType;
  bool success = TelemetryHistogramType(existing, &histogramType);
  if (!success)
    return NS_ERROR_INVALID_ARG;

  Histogram *clone;
  rv = HistogramGet(PromiseFlatCString(name).get(), existing->declared_min(),
                    existing->declared_max(), existing->bucket_count(),
                    histogramType, &clone);
  if (NS_FAILED(rv))
    return rv;

  Histogram::SampleSet ss;
  existing->SnapshotSample(&ss);
  clone->AddSampleSet(ss);
  return WrapAndReturnHistogram(clone, cx, ret);
}

void
TelemetryImpl::IdentifyCorruptHistograms(StatisticsRecorder::Histograms &hs)
{
  for (HistogramIterator it = hs.begin(); it != hs.end(); ++it) {
    Histogram *h = *it;

    Telemetry::ID id;
    nsresult rv = GetHistogramEnumId(h->histogram_name().c_str(), &id);
    
    if (NS_FAILED(rv)) {
      continue;
    }

    if (gCorruptHistograms[id]) {
      continue;
    }

    Histogram::SampleSet ss;
    h->SnapshotSample(&ss);
    Histogram::Inconsistencies check = h->FindCorruption(ss);
    bool corrupt = (check != Histogram::NO_INCONSISTENCIES);

    if (corrupt) {
      Telemetry::ID corruptID = Telemetry::HistogramCount;
      if (check & Histogram::RANGE_CHECKSUM_ERROR) {
        corruptID = Telemetry::RANGE_CHECKSUM_ERRORS;
      } else if (check & Histogram::BUCKET_ORDER_ERROR) {
        corruptID = Telemetry::BUCKET_ORDER_ERRORS;
      } else if (check & Histogram::COUNT_HIGH_ERROR) {
        corruptID = Telemetry::TOTAL_COUNT_HIGH_ERRORS;
      } else if (check & Histogram::COUNT_LOW_ERROR) {
        corruptID = Telemetry::TOTAL_COUNT_LOW_ERRORS;
      }
      Telemetry::Accumulate(corruptID, 1);
    }

    gCorruptHistograms[id] = corrupt;
  }
}

bool
TelemetryImpl::ShouldReflectHistogram(Histogram *h)
{
  const char *name = h->histogram_name().c_str();
  Telemetry::ID id;
  nsresult rv = GetHistogramEnumId(name, &id);
  if (NS_FAILED(rv)) {
    
    
    
    
    
    
    if (strcmp(name, "Histogram.InconsistentCountHigh") == 0
        || strcmp(name, "Histogram.InconsistentCountLow") == 0) {
      return false;
    }
    return true;
  } else {
    return !gCorruptHistograms[id];
  }
}

NS_IMETHODIMP
TelemetryImpl::GetHistogramSnapshots(JSContext *cx, jsval *ret)
{
  JSObject *root_obj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!root_obj)
    return NS_ERROR_FAILURE;
  *ret = OBJECT_TO_JSVAL(root_obj);

  StatisticsRecorder::Histograms hs;
  StatisticsRecorder::GetHistograms(&hs);

  
  
  
  
  
  
  IdentifyCorruptHistograms(hs);

  
  for (HistogramIterator it = hs.begin(); it != hs.end(); ++it) {
    Histogram *h = *it;
    if (!ShouldReflectHistogram(h)) {
      continue;
    }

    JSObject *hobj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!hobj) {
      return NS_ERROR_FAILURE;
    }
    switch (ReflectHistogramSnapshot(cx, hobj, h)) {
    case REFLECT_CORRUPT:
      
      
      
      continue;
    case REFLECT_FAILURE:
      return NS_ERROR_FAILURE;
    case REFLECT_OK:
      if (!JS_DefineProperty(cx, root_obj, h->histogram_name().c_str(),
                             OBJECT_TO_JSVAL(hobj), NULL, NULL, JSPROP_ENUMERATE)) {
        return NS_ERROR_FAILURE;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
TelemetryImpl::GetSlowSQL(JSContext *cx, jsval *ret)
{
  JSObject *root_obj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!root_obj)
    return NS_ERROR_FAILURE;
  *ret = OBJECT_TO_JSVAL(root_obj);

  MutexAutoLock hashMutex(mHashMutex);
  
  if (!AddSQLInfo(cx, root_obj, true))
    return NS_ERROR_FAILURE;
  
  if (!AddSQLInfo(cx, root_obj, false))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
TelemetryImpl::GetRegisteredHistograms(JSContext *cx, jsval *ret)
{
  size_t count = ArrayLength(gHistograms);
  JSObject *info = JS_NewObject(cx, NULL, NULL, NULL);
  if (!info)
    return NS_ERROR_FAILURE;

  for (size_t i = 0; i < count; ++i) {
    JSString *comment = JS_InternString(cx, gHistograms[i].comment);
    
    if (!(comment
          && JS_DefineProperty(cx, info, gHistograms[i].id,
                               STRING_TO_JSVAL(comment), NULL, NULL,
                               JSPROP_ENUMERATE))) {
      return NS_ERROR_FAILURE;
    }
  }

  *ret = OBJECT_TO_JSVAL(info);
  return NS_OK;
}

NS_IMETHODIMP
TelemetryImpl::GetHistogramById(const nsACString &name, JSContext *cx, jsval *ret)
{
  Histogram *h;
  nsresult rv = GetHistogramByName(name, &h);
  if (NS_FAILED(rv))
    return rv;

  return WrapAndReturnHistogram(h, cx, ret);
}

class TelemetrySessionData : public nsITelemetrySessionData
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEMETRYSESSIONDATA

public:
  static nsresult LoadFromDisk(nsIFile *, TelemetrySessionData **ptr);
  static nsresult SaveToDisk(nsIFile *, const nsACString &uuid);

  TelemetrySessionData(const char *uuid);
  ~TelemetrySessionData();

private:
  struct EnumeratorArgs {
    JSContext *cx;
    JSObject *snapshots;
  };
  typedef nsBaseHashtableET<nsUint32HashKey, Histogram::SampleSet> EntryType;
  typedef nsTHashtable<EntryType> SessionMapType;
  static PLDHashOperator ReflectSamples(EntryType *entry, void *arg);
  SessionMapType mSampleSetMap;
  nsCString mUUID;

  bool DeserializeHistogramData(Pickle &pickle, void **iter);
  static bool SerializeHistogramData(Pickle &pickle);

  
  
  static const unsigned int sVersion = 1;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(TelemetrySessionData, nsITelemetrySessionData)

TelemetrySessionData::TelemetrySessionData(const char *uuid)
  : mUUID(uuid)
{
  mSampleSetMap.Init();
}

TelemetrySessionData::~TelemetrySessionData()
{
  mSampleSetMap.Clear();
}

NS_IMETHODIMP
TelemetrySessionData::GetUuid(nsACString &uuid)
{
  uuid = mUUID;
  return NS_OK;
}

PLDHashOperator
TelemetrySessionData::ReflectSamples(EntryType *entry, void *arg)
{
  struct EnumeratorArgs *args = static_cast<struct EnumeratorArgs *>(arg);
  
  
  
  Histogram *h = nsnull;
  nsresult rv = GetHistogramByEnumId(Telemetry::ID(entry->GetKey()), &h);
  if (NS_FAILED(rv)) {
    return PL_DHASH_STOP;
  }

  
  if (entry->mData.sum() == 0) {
    return PL_DHASH_NEXT;
  }

  JSObject *snapshot = JS_NewObject(args->cx, NULL, NULL, NULL);
  if (!(snapshot
        && ReflectHistogramAndSamples(args->cx, snapshot, h, entry->mData)
        && JS_DefineProperty(args->cx, args->snapshots,
                             h->histogram_name().c_str(),
                             OBJECT_TO_JSVAL(snapshot), NULL, NULL,
                             JSPROP_ENUMERATE))) {
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
TelemetrySessionData::GetSnapshots(JSContext *cx, jsval *ret)
{
  JSObject *snapshots = JS_NewObject(cx, NULL, NULL, NULL);
  if (!snapshots) {
    return NS_ERROR_FAILURE;
  }

  struct EnumeratorArgs args = { cx, snapshots };
  PRUint32 count = mSampleSetMap.EnumerateEntries(ReflectSamples,
                                                  static_cast<void*>(&args));
  if (count != mSampleSetMap.Count()) {
    return NS_ERROR_FAILURE;
  }

  *ret = OBJECT_TO_JSVAL(snapshots);
  return NS_OK;
}

bool
TelemetrySessionData::DeserializeHistogramData(Pickle &pickle, void **iter)
{
  PRUint32 count = 0;
  if (!pickle.ReadUInt32(iter, &count)) {
    return false;
  }

  for (size_t i = 0; i < count; ++i) {
    int stored_length;
    const char *name;
    if (!pickle.ReadData(iter, &name, &stored_length)) {
      return false;
    }

    Telemetry::ID id;
    nsresult rv = TelemetryImpl::GetHistogramEnumId(name, &id);
    if (NS_FAILED(rv)) {
      
      
      
      Histogram::SampleSet ss;
      if (!ss.Deserialize(iter, pickle)) {
        return false;
      }
    }

    EntryType *entry = mSampleSetMap.GetEntry(id);
    if (!entry) {
      entry = mSampleSetMap.PutEntry(id);
      if (NS_UNLIKELY(!entry)) {
        return false;
      }
      if (!entry->mData.Deserialize(iter, pickle)) {
        return false;
      }
    }
  }

  return true;
}

nsresult
TelemetrySessionData::LoadFromDisk(nsIFile *file, TelemetrySessionData **ptr)
{
  *ptr = nsnull;
  nsresult rv;
  nsCOMPtr<nsILocalFile> f(do_QueryInterface(file, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  AutoFDClose fd;
  rv = f->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 size = PR_Available(fd);
  if (size == -1) {
    return NS_ERROR_FAILURE;
  }

  nsAutoArrayPtr<char> data(new char[size]);
  PRInt32 amount = PR_Read(fd, data, size);
  if (amount != size) {
    return NS_ERROR_FAILURE;
  }

  Pickle pickle(data, size);
  void *iter = NULL;

  unsigned int storedVersion;
  if (!(pickle.ReadUInt32(&iter, &storedVersion)
        && storedVersion == sVersion)) {
    return NS_ERROR_FAILURE;
  }

  const char *uuid;
  int uuidLength;
  if (!pickle.ReadData(&iter, &uuid, &uuidLength)) {
    return NS_ERROR_FAILURE;
  }

  nsAutoPtr<TelemetrySessionData> sessionData(new TelemetrySessionData(uuid));
  if (!sessionData->DeserializeHistogramData(pickle, &iter)) {
    return NS_ERROR_FAILURE;
  }

  *ptr = sessionData.forget();
  return NS_OK;
}

bool
TelemetrySessionData::SerializeHistogramData(Pickle &pickle)
{
  StatisticsRecorder::Histograms hs;
  StatisticsRecorder::GetHistograms(&hs);

  if (!pickle.WriteUInt32(hs.size())) {
    return false;
  }

  for (StatisticsRecorder::Histograms::const_iterator it = hs.begin();
       it != hs.end();
       ++it) {
    const Histogram *h = *it;
    const char *name = h->histogram_name().c_str();

    Histogram::SampleSet ss;
    h->SnapshotSample(&ss);

    if (!(pickle.WriteData(name, strlen(name)+1)
          && ss.Serialize(&pickle))) {
      return false;
    }
  }

  return true;
}

nsresult
TelemetrySessionData::SaveToDisk(nsIFile *file, const nsACString &uuid)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> f(do_QueryInterface(file, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  AutoFDClose fd;
  rv = f->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
  if (NS_FAILED(rv)) {
    return rv;
  }

  Pickle pickle;
  if (!pickle.WriteUInt32(sVersion)) {
    return NS_ERROR_FAILURE;
  }

  
  const char *data;
  size_t length = uuid.GetData(&data);
  if (!(pickle.WriteData(data, length+1)
        && SerializeHistogramData(pickle))) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 amount = PR_Write(fd, static_cast<const char*>(pickle.data()),
                            pickle.size());
  if (amount != pickle.size()) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

class SaveHistogramEvent : public nsRunnable
{
public:
  SaveHistogramEvent(nsIFile *file, const nsACString &uuid,
                     nsITelemetrySaveSessionDataCallback *callback)
    : mFile(file), mUUID(uuid), mCallback(callback)
  {}

  NS_IMETHOD Run()
  {
    nsresult rv = TelemetrySessionData::SaveToDisk(mFile, mUUID);
    mCallback->Handle(!!NS_SUCCEEDED(rv));
    return rv;
  }

private:
  nsCOMPtr<nsIFile> mFile;
  nsCString mUUID;
  nsCOMPtr<nsITelemetrySaveSessionDataCallback> mCallback;
};

NS_IMETHODIMP
TelemetryImpl::SaveHistograms(nsIFile *file, const nsACString &uuid,
                              nsITelemetrySaveSessionDataCallback *callback,
                              bool isSynchronous)
{
  nsCOMPtr<nsIRunnable> event = new SaveHistogramEvent(file, uuid, callback);
  if (isSynchronous) {
    return event ? event->Run() : NS_ERROR_FAILURE;
  } else {
    return NS_DispatchToCurrentThread(event);
  }
}

class LoadHistogramEvent : public nsRunnable
{
public:
  LoadHistogramEvent(nsIFile *file,
                     nsITelemetryLoadSessionDataCallback *callback)
    : mFile(file), mCallback(callback)
  {}

  NS_IMETHOD Run()
  {
    TelemetrySessionData *sessionData = nsnull;
    nsresult rv = TelemetrySessionData::LoadFromDisk(mFile, &sessionData);
    if (NS_FAILED(rv)) {
      mCallback->Handle(nsnull);
    } else {
      nsCOMPtr<nsITelemetrySessionData> data(sessionData);
      mCallback->Handle(data);
    }
    return rv;
  }

private:
  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsITelemetryLoadSessionDataCallback> mCallback;
};

NS_IMETHODIMP
TelemetryImpl::LoadHistograms(nsIFile *file,
                              nsITelemetryLoadSessionDataCallback *callback)
{
  nsCOMPtr<nsIRunnable> event = new LoadHistogramEvent(file, callback);
  return NS_DispatchToCurrentThread(event);
}

NS_IMETHODIMP
TelemetryImpl::GetCanRecord(bool *ret) {
  *ret = mCanRecord;
  return NS_OK;
}

NS_IMETHODIMP
TelemetryImpl::SetCanRecord(bool canRecord) {
  mCanRecord = !!canRecord;
  return NS_OK;
}

bool
TelemetryImpl::CanRecord() {
  return !sTelemetry || sTelemetry->mCanRecord;
}

already_AddRefed<nsITelemetry>
TelemetryImpl::CreateTelemetryInstance()
{
  NS_ABORT_IF_FALSE(sTelemetry == NULL, "CreateTelemetryInstance may only be called once, via GetService()");
  sTelemetry = new TelemetryImpl(); 
  
  NS_ADDREF(sTelemetry);
  
  NS_ADDREF(sTelemetry);
  return sTelemetry;
}

void
TelemetryImpl::ShutdownTelemetry()
{
  NS_IF_RELEASE(sTelemetry);
}

void
TelemetryImpl::RecordSlowStatement(const nsACString &statement,
                                   const nsACString &dbName,
                                   PRUint32 delay)
{
  MOZ_ASSERT(sTelemetry);
  if (!sTelemetry->mCanRecord || !sTelemetry->mTrackedDBs.GetEntry(dbName))
    return;

  nsTHashtable<SlowSQLEntryType> *slowSQLMap = NULL;
  if (NS_IsMainThread())
    slowSQLMap = &(sTelemetry->mSlowSQLOnMainThread);
  else
    slowSQLMap = &(sTelemetry->mSlowSQLOnOtherThread);

  MutexAutoLock hashMutex(sTelemetry->mHashMutex);
  SlowSQLEntryType *entry = slowSQLMap->GetEntry(statement);
  if (!entry) {
    entry = slowSQLMap->PutEntry(statement);
    if (NS_UNLIKELY(!entry))
      return;
    entry->mData.hitCount = 0;
    entry->mData.totalTime = 0;
  }
  entry->mData.hitCount++;
  entry->mData.totalTime += delay;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(TelemetryImpl, nsITelemetry)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsITelemetry, TelemetryImpl::CreateTelemetryInstance)

#define NS_TELEMETRY_CID \
  {0xaea477f2, 0xb3a2, 0x469c, {0xaa, 0x29, 0x0a, 0x82, 0xd1, 0x32, 0xb8, 0x29}}
NS_DEFINE_NAMED_CID(NS_TELEMETRY_CID);

const Module::CIDEntry kTelemetryCIDs[] = {
  { &kNS_TELEMETRY_CID, false, NULL, nsITelemetryConstructor },
  { NULL }
};

const Module::ContractIDEntry kTelemetryContracts[] = {
  { "@mozilla.org/base/telemetry;1", &kNS_TELEMETRY_CID },
  { NULL }
};

const Module kTelemetryModule = {
  Module::kVersion,
  kTelemetryCIDs,
  kTelemetryContracts,
  NULL,
  NULL,
  NULL,
  TelemetryImpl::ShutdownTelemetry
};

} 

namespace mozilla {
namespace Telemetry {

void
Accumulate(ID aHistogram, PRUint32 aSample)
{
  if (!TelemetryImpl::CanRecord()) {
    return;
  }
  Histogram *h;
  nsresult rv = GetHistogramByEnumId(aHistogram, &h);
  if (NS_SUCCEEDED(rv))
    h->Add(aSample);
}

void
AccumulateTimeDelta(ID aHistogram, TimeStamp start, TimeStamp end)
{
  Accumulate(aHistogram,
             static_cast<PRUint32>((end - start).ToMilliseconds()));
}

base::Histogram*
GetHistogramById(ID id)
{
  Histogram *h = NULL;
  GetHistogramByEnumId(id, &h);
  return h;
}

void
RecordSlowSQLStatement(const nsACString &statement,
                       const nsACString &dbName,
                       PRUint32 delay)
{
  TelemetryImpl::RecordSlowStatement(statement, dbName, delay);
}

void Init()
{
  
  nsCOMPtr<nsITelemetry> telemetryService =
    do_GetService("@mozilla.org/base/telemetry;1");
  MOZ_ASSERT(telemetryService);
}

} 
} 

NSMODULE_DEFN(nsTelemetryModule) = &kTelemetryModule;





void
XRE_TelemetryAccumulate(int aID, PRUint32 aSample)
{
  mozilla::Telemetry::Accumulate((mozilla::Telemetry::ID) aID, aSample);
}
