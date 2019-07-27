




#include "nsPerformanceStats.h"

#include "nsMemory.h"
#include "nsLiteralString.h"
#include "nsCRTGlue.h"
#include "nsServiceManagerUtils.h"

#include "nsCOMArray.h"
#include "nsIMutableArray.h"

#include "jsapi.h"
#include "nsJSUtils.h"
#include "xpcpublic.h"
#include "jspubtd.h"
#include "nsIJSRuntimeService.h"

#include "nsIDOMWindow.h"
#include "nsGlobalWindow.h"

class nsPerformanceStats: public nsIPerformanceStats {
public:
  nsPerformanceStats(const nsAString& aName,
                     const nsAString& aAddonId,
                     const nsAString& aTitle,
                     const uint64_t aWindowId,
                     const bool aIsSystem,
                     const js::PerformanceData& aPerformanceData)
    : mName(aName)
    , mAddonId(aAddonId)
    , mTitle(aTitle)
    , mWindowId(aWindowId)
    , mIsSystem(aIsSystem)
    , mPerformanceData(aPerformanceData)
  {
  }
  explicit nsPerformanceStats() {}

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetName(nsAString& aName) override {
    aName.Assign(mName);
    return NS_OK;
  };

  
  NS_IMETHOD GetAddonId(nsAString& aAddonId) override {
    aAddonId.Assign(mAddonId);
    return NS_OK;
  };

  
  NS_IMETHOD GetWindowId(uint64_t *aWindowId) override {
    *aWindowId = mWindowId;
    return NS_OK;
  }

  
  NS_IMETHOD GetTitle(nsAString & aTitle) override {
    aTitle.Assign(mTitle);
    return NS_OK;
  }

  
  NS_IMETHOD GetIsSystem(bool *_retval) override {
    *_retval = mIsSystem;
    return NS_OK;
  }

  
  NS_IMETHOD GetTotalUserTime(uint64_t *aTotalUserTime) override {
    *aTotalUserTime = mPerformanceData.totalUserTime;
    return NS_OK;
  };

  
  NS_IMETHOD GetTotalSystemTime(uint64_t *aTotalSystemTime) override {
    *aTotalSystemTime = mPerformanceData.totalSystemTime;
    return NS_OK;
  };

  
  NS_IMETHOD GetTotalCPOWTime(uint64_t *aCpowTime) override {
    *aCpowTime = mPerformanceData.totalCPOWTime;
    return NS_OK;
  };

  
  NS_IMETHOD GetTicks(uint64_t *aTicks) override {
    *aTicks = mPerformanceData.ticks;
    return NS_OK;
  };

  
  NS_IMETHODIMP GetDurations(uint32_t *aCount, uint64_t **aNumberOfOccurrences) override {
    const size_t length = mozilla::ArrayLength(mPerformanceData.durations);
    if (aCount) {
      *aCount = length;
    }
    *aNumberOfOccurrences = new uint64_t[length];
    for (size_t i = 0; i < length; ++i) {
      (*aNumberOfOccurrences)[i] = mPerformanceData.durations[i];
    }
    return NS_OK;
  };

private:
  nsString mName;
  nsString mAddonId;
  nsString mTitle;
  uint64_t mWindowId;
  bool mIsSystem;

  js::PerformanceData mPerformanceData;

  virtual ~nsPerformanceStats() {}
};

NS_IMPL_ISUPPORTS(nsPerformanceStats, nsIPerformanceStats)


class nsPerformanceSnapshot : public nsIPerformanceSnapshot
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERFORMANCESNAPSHOT

  nsPerformanceSnapshot();
  nsresult Init(JSContext*);
private:
  virtual ~nsPerformanceSnapshot();

  









  already_AddRefed<nsIPerformanceStats> ImportStats(JSContext* cx, const js::PerformanceData& data);

  


  bool IterPerformanceStatsCallbackInternal(JSContext* cx, const js::PerformanceData& stats);
  static bool IterPerformanceStatsCallback(JSContext* cx, const js::PerformanceData& stats, void* self);

  
  
  static void GetWindowData(JSContext*,
                            nsString& title,
                            uint64_t* windowId);

  
  
  static void GetAddonId(JSContext*,
                         JS::Handle<JSObject*> global,
                         nsAString& addonId);

  
  static bool GetIsSystem(JSContext*,
                          JS::Handle<JSObject*> global);

private:
  nsCOMArray<nsIPerformanceStats> mComponentsData;
  nsCOMPtr<nsIPerformanceStats> mProcessData;
};

NS_IMPL_ISUPPORTS(nsPerformanceSnapshot, nsIPerformanceSnapshot)

nsPerformanceSnapshot::nsPerformanceSnapshot()
{
}

nsPerformanceSnapshot::~nsPerformanceSnapshot()
{
}

 void
nsPerformanceSnapshot::GetWindowData(JSContext* cx,
                                     nsString& title,
                                     uint64_t* windowId)
{
  MOZ_ASSERT(windowId);

  title.SetIsVoid(true);
  *windowId = 0;

  nsCOMPtr<nsPIDOMWindow> win = xpc::CurrentWindowOrNull(cx);
  if (!win) {
    return;
  }

  nsCOMPtr<nsIDOMWindow> top;
  nsresult rv = win->GetTop(getter_AddRefs(top));
  if (!top || NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsPIDOMWindow> ptop = do_QueryInterface(top);
  if (!ptop) {
    return;
  }

  nsCOMPtr<nsIDocument> doc = ptop->GetExtantDoc();
  if (!doc) {
    return;
  }

  doc->GetTitle(title);
  *windowId = ptop->WindowID();
}

 void
nsPerformanceSnapshot::GetAddonId(JSContext*,
                                  JS::Handle<JSObject*> global,
                                  nsAString& addonId)
{
  addonId.AssignLiteral("");

  JSAddonId* jsid = AddonIdOfObject(global);
  if (!jsid) {
    return;
  }
  AssignJSFlatString(addonId, (JSFlatString*)jsid);
}

 bool
nsPerformanceSnapshot::GetIsSystem(JSContext*,
                                   JS::Handle<JSObject*> global)
{
  return nsContentUtils::IsSystemPrincipal(nsContentUtils::ObjectPrincipal(global));
}

already_AddRefed<nsIPerformanceStats>
nsPerformanceSnapshot::ImportStats(JSContext* cx, const js::PerformanceData& performance) {
  JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));

  if (!global) {
    
    
    return nullptr;
  }

  nsString addonId;
  GetAddonId(cx, global, addonId);

  nsString title;
  uint64_t windowId;
  GetWindowData(cx, title, &windowId);

  nsAutoString name;
  nsAutoCString cname;
  xpc::GetCurrentCompartmentName(cx, cname);
  name.Assign(NS_ConvertUTF8toUTF16(cname));

  bool isSystem = GetIsSystem(cx, global);

  nsCOMPtr<nsIPerformanceStats> result =
    new nsPerformanceStats(name, addonId, title, windowId, isSystem, performance);
  return result.forget();
}

 bool
nsPerformanceSnapshot::IterPerformanceStatsCallback(JSContext* cx, const js::PerformanceData& stats, void* self) {
  return reinterpret_cast<nsPerformanceSnapshot*>(self)->IterPerformanceStatsCallbackInternal(cx, stats);
}

bool
nsPerformanceSnapshot::IterPerformanceStatsCallbackInternal(JSContext* cx, const js::PerformanceData& stats) {
  nsCOMPtr<nsIPerformanceStats> result = ImportStats(cx, stats);
  if (result) {
    mComponentsData.AppendElement(result);
  }

  return true;
}

nsresult
nsPerformanceSnapshot::Init(JSContext* cx) {
  js::PerformanceData processStats;
  if (!js::IterPerformanceStats(cx, nsPerformanceSnapshot::IterPerformanceStatsCallback, &processStats, this)) {
    return NS_ERROR_UNEXPECTED;
  }

  mProcessData = new nsPerformanceStats(NS_LITERAL_STRING("<process>"), 
                                        NS_LITERAL_STRING(""),          
                                        NS_LITERAL_STRING(""),          
                                        0,                              
                                        true,                           
                                        processStats);
  return NS_OK;
}



NS_IMETHODIMP nsPerformanceSnapshot::GetComponentsData(nsIArray * *aComponents)
{
  const size_t length = mComponentsData.Length();
  nsCOMPtr<nsIMutableArray> components = do_CreateInstance(NS_ARRAY_CONTRACTID);
  for (size_t i = 0; i < length; ++i) {
    nsCOMPtr<nsIPerformanceStats> stats = mComponentsData[i];
    mozilla::DebugOnly<nsresult> rv = components->AppendElement(stats, false);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }
  components.forget(aComponents);
  return NS_OK;
}


NS_IMETHODIMP nsPerformanceSnapshot::GetProcessData(nsIPerformanceStats * *aProcess)
{
  NS_IF_ADDREF(*aProcess = mProcessData);
  return NS_OK;
}


NS_IMPL_ISUPPORTS(nsPerformanceStatsService, nsIPerformanceStatsService)

nsPerformanceStatsService::nsPerformanceStatsService()
{
}

nsPerformanceStatsService::~nsPerformanceStatsService()
{
}


NS_IMETHODIMP nsPerformanceStatsService::GetIsMonitoringCPOW(JSContext* cx, bool *aIsStopwatchActive)
{
  JSRuntime *runtime = JS_GetRuntime(cx);
  *aIsStopwatchActive = js::GetStopwatchIsMonitoringCPOW(runtime);
  return NS_OK;
}
NS_IMETHODIMP nsPerformanceStatsService::SetIsMonitoringCPOW(JSContext* cx, bool aIsStopwatchActive)
{
  JSRuntime *runtime = JS_GetRuntime(cx);
  if (!js::SetStopwatchIsMonitoringCPOW(runtime, aIsStopwatchActive)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}
NS_IMETHODIMP nsPerformanceStatsService::GetIsMonitoringJank(JSContext* cx, bool *aIsStopwatchActive)
{
  JSRuntime *runtime = JS_GetRuntime(cx);
  *aIsStopwatchActive = js::GetStopwatchIsMonitoringJank(runtime);
  return NS_OK;
}
NS_IMETHODIMP nsPerformanceStatsService::SetIsMonitoringJank(JSContext* cx, bool aIsStopwatchActive)
{
  JSRuntime *runtime = JS_GetRuntime(cx);
  if (!js::SetStopwatchIsMonitoringJank(runtime, aIsStopwatchActive)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}


NS_IMETHODIMP nsPerformanceStatsService::GetSnapshot(JSContext* cx, nsIPerformanceSnapshot * *aSnapshot)
{
  nsRefPtr<nsPerformanceSnapshot> snapshot = new nsPerformanceSnapshot();
  nsresult rv = snapshot->Init(cx);
  if (NS_FAILED(rv)) {
    return rv;
  }

  snapshot.forget(aSnapshot);
  return NS_OK;
}


