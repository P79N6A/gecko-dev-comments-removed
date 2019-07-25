





































#include "mozilla/dom/PCrashReporterParent.h"
#include "mozilla/dom/TabMessageUtils.h"
#include "nsXULAppAPI.h"
#include "nsILocalFile.h"
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

namespace mozilla {
namespace dom {
class ProcessReporter;

class CrashReporterParent :
    public PCrashReporterParent
{
#ifdef MOZ_CRASHREPORTER
  typedef CrashReporter::AnnotationTable AnnotationTable;
#endif
public:
  CrashReporterParent();
  virtual ~CrashReporterParent();

#ifdef MOZ_CRASHREPORTER
  



  template<class Toplevel>
  bool
  GeneratePairedMinidump(Toplevel* t);

  



  bool
  GenerateHangCrashReport(const AnnotationTable* processNotes);

  



  template<class Toplevel>
  bool
  GenerateCrashReport(Toplevel* t, const AnnotationTable* processNotes);

  


  template<class Toplevel>
  static bool CreateCrashReporter(Toplevel* actor);
#endif
  
  void
    SetChildData(const NativeThreadId& id, const PRUint32& processType);

  


  const nsString& HangID() {
    return mHangID;
  }
  


  const nsString& ParentDumpID() {
    return mParentDumpID;
  }
  


  const nsString& ChildDumpID() {
    return mChildDumpID;
  }

 protected:
  virtual void ActorDestroy(ActorDestroyReason why);

  virtual bool
    RecvAddLibraryMappings(const InfallibleTArray<Mapping>& m);
  virtual bool
    RecvAnnotateCrashReport(const nsCString& key, const nsCString& data);
  virtual bool
    RecvAppendAppNotes(const nsCString& data);

#ifdef MOZ_CRASHREPORTER
  bool
  GenerateChildData(const AnnotationTable* processNotes);

  AnnotationTable mNotes;
#endif
  nsCString mAppNotes;
  nsString mHangID;
  nsString mChildDumpID;
  nsString mParentDumpID;
  NativeThreadId mMainThread;
  time_t mStartTime;
  PRUint32 mProcessType;
  bool mInitialized;
};

#ifdef MOZ_CRASHREPORTER
template<class Toplevel>
inline bool
CrashReporterParent::GeneratePairedMinidump(Toplevel* t)
{
  CrashReporter::ProcessHandle child;
#ifdef XP_MACOSX
  child = t->Process()->GetChildTask();
#else
  child = t->OtherProcess();
#endif
  nsCOMPtr<nsILocalFile> childDump;
  nsCOMPtr<nsILocalFile> parentDump;
  if (CrashReporter::CreatePairedMinidumps(child,
                                           mMainThread,
                                           &mHangID,
                                           getter_AddRefs(childDump),
                                           getter_AddRefs(parentDump)) &&
      CrashReporter::GetIDFromMinidump(childDump, mChildDumpID) &&
      CrashReporter::GetIDFromMinidump(parentDump, mParentDumpID)) {
    return true;
  }
  return false;
}

template<class Toplevel>
inline bool
CrashReporterParent::GenerateCrashReport(Toplevel* t,
                                         const AnnotationTable* processNotes)
{
  nsCOMPtr<nsILocalFile> crashDump;
  if (t->TakeMinidump(getter_AddRefs(crashDump)) &&
      CrashReporter::GetIDFromMinidump(crashDump, mChildDumpID)) {
    return GenerateChildData(processNotes);
  }
  return false;
}

template<class Toplevel>
 bool
CrashReporterParent::CreateCrashReporter(Toplevel* actor)
{
#ifdef MOZ_CRASHREPORTER
  NativeThreadId id;
  PRUint32 processType;
  PCrashReporterParent* p =
      actor->CallPCrashReporterConstructor(&id, &processType);
  if (p) {
    static_cast<CrashReporterParent*>(p)->SetChildData(id, processType);
  } else {
    NS_ERROR("Error creating crash reporter actor");
  }
  return !!p;
#endif
  return false;
}

#endif

} 
} 
