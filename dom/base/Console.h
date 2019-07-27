




#ifndef mozilla_dom_Console_h
#define mozilla_dom_Console_h

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/UnionConversions.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsWrapperCache.h"

class nsIConsoleAPIStorage;

namespace mozilla {
namespace dom {

class ConsoleCallData;
struct ConsoleStackEntry;

class Console MOZ_FINAL : public nsITimerCallback
                        , public nsIObserver
                        , public nsWrapperCache
{
  ~Console();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Console,
                                                         nsITimerCallback)
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIOBSERVER

  explicit Console(nsPIDOMWindow* aWindow);

  
  nsISupports* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

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
  Profile(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  ProfileEnd(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  Assert(JSContext* aCx, bool aCondition, const Sequence<JS::Value>& aData);

  void
  Count(JSContext* aCx, const Sequence<JS::Value>& aData);

  void
  __noSuchMethod__();

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
    MethodGroup,
    MethodGroupCollapsed,
    MethodGroupEnd,
    MethodTime,
    MethodTimeEnd,
    MethodAssert,
    MethodCount
  };

  void
  Method(JSContext* aCx, MethodName aName, const nsAString& aString,
         const Sequence<JS::Value>& aData);

  void
  AppendCallData(ConsoleCallData* aData);

  void
  ProcessCallData(ConsoleCallData* aData);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void
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

  
  void
  ArgumentsToValueList(const nsTArray<JS::Heap<JS::Value>>& aData,
                       Sequence<JS::Value>& aSequence);

  void
  ProfileMethod(JSContext* aCx, const nsAString& aAction,
                const Sequence<JS::Value>& aData);

  JS::Value
  IncreaseCounter(JSContext* aCx, const ConsoleStackEntry& aFrame,
                   const nsTArray<JS::Heap<JS::Value>>& aArguments);

  void
  ClearConsoleData();

  bool
  ShouldIncludeStackrace(MethodName aMethodName);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIConsoleAPIStorage> mStorage;

  LinkedList<ConsoleCallData> mQueuedCalls;
  nsDataHashtable<nsStringHashKey, DOMHighResTimeStamp> mTimerRegistry;
  nsDataHashtable<nsStringHashKey, uint32_t> mCounterRegistry;

  uint64_t mOuterID;
  uint64_t mInnerID;

  friend class ConsoleCallData;
  friend class ConsoleCallDataRunnable;
  friend class ConsoleProfileRunnable;
};

} 
} 

#endif
