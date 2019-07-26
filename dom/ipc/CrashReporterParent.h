




#include "mozilla/dom/PCrashReporterParent.h"
#include "mozilla/dom/TabMessageUtils.h"
#include "nsXULAppAPI.h"
#include "nsIFile.h"
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

  



  template<class Toplevel>
  bool
  GenerateCrashReport(Toplevel* t, const AnnotationTable* processNotes);

  


  bool
  GenerateChildData(const AnnotationTable* processNotes);

  bool
  GenerateCrashReportForMinidump(nsIFile* minidump,
                                 const AnnotationTable* processNotes);

  


  template<class Toplevel>
  static bool CreateCrashReporter(Toplevel* actor);
#endif
  
  void
    SetChildData(const NativeThreadId& id, const uint32_t& processType);

  


  const nsString& ChildDumpID() {
    return mChildDumpID;
  }

  void
  AnnotateCrashReport(const nsCString& key, const nsCString& data);

 protected:
  virtual bool
    RecvAnnotateCrashReport(const nsCString& key, const nsCString& data) {
    AnnotateCrashReport(key, data);
    return true;
  }
  virtual bool
    RecvAppendAppNotes(const nsCString& data);

#ifdef MOZ_CRASHREPORTER
  AnnotationTable mNotes;
#endif
  nsCString mAppNotes;
  nsString mChildDumpID;
  NativeThreadId mMainThread;
  time_t mStartTime;
  uint32_t mProcessType;
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
  nsCOMPtr<nsIFile> childDump;
  if (CrashReporter::CreatePairedMinidumps(child,
                                           mMainThread,
                                           getter_AddRefs(childDump)) &&
      CrashReporter::GetIDFromMinidump(childDump, mChildDumpID)) {
    return true;
  }
  return false;
}

template<class Toplevel>
inline bool
CrashReporterParent::GenerateCrashReport(Toplevel* t,
                                         const AnnotationTable* processNotes)
{
  nsCOMPtr<nsIFile> crashDump;
  if (t->TakeMinidump(getter_AddRefs(crashDump), NULL) &&
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
  uint32_t processType;
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
