





#ifndef mozilla_dom_Console_h
#define mozilla_dom_Console_h

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIObserver.h"
#include "nsWrapperCache.h"
#include "nsDOMNavigationTiming.h"
#include "nsPIDOMWindow.h"

class nsIConsoleAPIStorage;
class nsIPrincipal;
class nsIProfiler;
class nsIXPConnectJSObjectHolder;

namespace mozilla {
namespace dom {

class ConsoleCallData;
struct ConsoleStackEntry;

class Console final : public nsIObserver
                    , public nsWrapperCache
{
  ~Console();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Console)
  NS_DECL_NSIOBSERVER

  explicit Console(nsPIDOMWindow* aWindow);

  
  nsISupports* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void
  Log(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Info(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Warn(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Error(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Exception(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Debug(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Table(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Trace(JSContext* aCx);

  void
  Dir(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Dirxml(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Group(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  GroupCollapsed(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  GroupEnd(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Time(JSContext* aCx, const JS::Handle<JS::Value> aTime);

  void
  TimeEnd(JSContext* aCx, const JS::Handle<JS::Value> aTime);

  void
  TimeStamp(JSContext* aCx, const JS::Handle<JS::Value> aData);

  void
  Profile(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  ProfileEnd(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Assert(JSContext* aCx, bool aCondition, const Sequence<JS::Value>& aData);

  void
  Count(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  NoopMethod();

private:
  enum MethodName
  {
    MethodLog,
    MethodInfo,
    MethodWarn,
    MethodError,
    MethodException,
    MethodDebug,
    MethodTable,
    MethodTrace,
    MethodDir,
    MethodDirxml,
    MethodGroup,
    MethodGroupCollapsed,
    MethodGroupEnd,
    MethodTime,
    MethodTimeEnd,
    MethodTimeStamp,
    MethodAssert,
    MethodCount
  };

  void
  Method(JSContext* aCx, MethodName aName, const nsAString& aString,
         const Sequence<JS::Value>& aData);

  void
  ProcessCallData(ConsoleCallData* aData);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool
  ProcessArguments(JSContext* aCx, const nsTArray<JS::Heap<JS::Value>>& aData,
                   Sequence<JS::Value>& aSequence,
                   Sequence<JS::Value>& aStyles);

  void
  MakeFormatString(nsCString& aFormat, int32_t aInteger, int32_t aMantissa,
                   char aCh);

  
  
  void
  ComposeGroupName(JSContext* aCx, const nsTArray<JS::Heap<JS::Value>>& aData,
                   nsAString& aName);

  JS::Value
  StartTimer(JSContext* aCx, const JS::Value& aName,
             DOMHighResTimeStamp aTimestamp);

  JS::Value
  StopTimer(JSContext* aCx, const JS::Value& aName,
            DOMHighResTimeStamp aTimestamp);

  
  bool
  ArgumentsToValueList(const nsTArray<JS::Heap<JS::Value>>& aData,
                       Sequence<JS::Value>& aSequence);

  void
  ProfileMethod(JSContext* aCx, const nsAString& aAction,
                const Sequence<JS::Value>& aData);

  JS::Value
  IncreaseCounter(JSContext* aCx, const ConsoleStackEntry& aFrame,
                   const nsTArray<JS::Heap<JS::Value>>& aArguments);

  bool
  ShouldIncludeStackTrace(MethodName aMethodName);

  nsIXPConnectJSObjectHolder*
  GetOrCreateSandbox(JSContext* aCx, nsIPrincipal* aPrincipal);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsIConsoleAPIStorage> mStorage;
  nsCOMPtr<nsIXPConnectJSObjectHolder> mSandbox;
#ifdef MOZ_ENABLE_PROFILER_SPS
  nsCOMPtr<nsIProfiler> mProfiler;
#endif

  nsDataHashtable<nsStringHashKey, DOMHighResTimeStamp> mTimerRegistry;
  nsDataHashtable<nsStringHashKey, uint32_t> mCounterRegistry;

  uint64_t mOuterID;
  uint64_t mInnerID;

  friend class ConsoleCallData;
  friend class ConsoleRunnable;
  friend class ConsoleCallDataRunnable;
  friend class ConsoleProfileRunnable;
};

} 
} 

#endif
