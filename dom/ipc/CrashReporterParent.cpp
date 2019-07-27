




#include "CrashReporterParent.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"
#include <time.h>

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#include "nsICrashService.h"
#include "mozilla/SyncRunnable.h"
#include "nsThreadUtils.h"
#endif

using namespace base;

namespace mozilla {
namespace dom {

void
CrashReporterParent::AnnotateCrashReport(const nsCString& key,
                                         const nsCString& data)
{
#ifdef MOZ_CRASHREPORTER
    mNotes.Put(key, data);
#endif
}

void
CrashReporterParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
CrashReporterParent::RecvAppendAppNotes(const nsCString& data)
{
    mAppNotes.Append(data);
    return true;
}

mozilla::ipc::IProtocol*
CrashReporterParent::CloneProtocol(Channel* aChannel,
                                   mozilla::ipc::ProtocolCloneContext* aCtx)
{
#ifdef MOZ_CRASHREPORTER
    ContentParent* contentParent = aCtx->GetContentParent();
    CrashReporter::ThreadId childThreadId = contentParent->Pid();
    GeckoProcessType childProcessType =
        contentParent->Process()->GetProcessType();

    nsAutoPtr<PCrashReporterParent> actor(
        contentParent->AllocPCrashReporterParent(childThreadId,
                                                 childProcessType)
    );
    if (!actor ||
        !contentParent->RecvPCrashReporterConstructor(actor,
                                                      childThreadId,
                                                      childThreadId)) {
      return nullptr;
    }

    return actor.forget();
#else
    MOZ_CRASH("Not Implemented");
    return nullptr;
#endif
}

CrashReporterParent::CrashReporterParent()
    :
#ifdef MOZ_CRASHREPORTER
      mNotes(4),
#endif
      mStartTime(::time(nullptr))
    , mInitialized(false)
{
    MOZ_COUNT_CTOR(CrashReporterParent);
}

CrashReporterParent::~CrashReporterParent()
{
    MOZ_COUNT_DTOR(CrashReporterParent);
}

void
CrashReporterParent::SetChildData(const NativeThreadId& tid,
                                  const uint32_t& processType)
{
    mInitialized = true;
    mMainThread = tid;
    mProcessType = processType;
}

#ifdef MOZ_CRASHREPORTER
bool
CrashReporterParent::GenerateCrashReportForMinidump(nsIFile* minidump,
    const AnnotationTable* processNotes)
{
    if (!CrashReporter::GetIDFromMinidump(minidump, mChildDumpID))
        return false;
    return GenerateChildData(processNotes);
}

bool
CrashReporterParent::GenerateChildData(const AnnotationTable* processNotes)
{
    MOZ_ASSERT(mInitialized);

    nsAutoCString type;
    switch (mProcessType) {
        case GeckoProcessType_Content:
            type = NS_LITERAL_CSTRING("content");
            break;
        case GeckoProcessType_Plugin:
        case GeckoProcessType_GMPlugin:
            type = NS_LITERAL_CSTRING("plugin");
            break;
        default:
            NS_ERROR("unknown process type");
            break;
    }
    mNotes.Put(NS_LITERAL_CSTRING("ProcessType"), type);

    char startTime[32];
    sprintf(startTime, "%lld", static_cast<long long>(mStartTime));
    mNotes.Put(NS_LITERAL_CSTRING("StartupTime"), nsDependentCString(startTime));

    if (!mAppNotes.IsEmpty())
        mNotes.Put(NS_LITERAL_CSTRING("Notes"), mAppNotes);

    bool ret = CrashReporter::AppendExtraData(mChildDumpID, mNotes);
    if (ret && processNotes)
        ret = CrashReporter::AppendExtraData(mChildDumpID, *processNotes);
    if (!ret)
        NS_WARNING("problem appending child data to .extra");

    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    class NotifyOnMainThread : public nsRunnable
    {
    public:
        NotifyOnMainThread(CrashReporterParent* aCR)
            : mCR(aCR)
        { }

        NS_IMETHOD Run() {
            mCR->NotifyCrashService();
            return NS_OK;
        }
    private:
        CrashReporterParent* mCR;
    };
    SyncRunnable::DispatchToThread(mainThread, new NotifyOnMainThread(this));
    return ret;
}

void
CrashReporterParent::NotifyCrashService()
{
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsICrashService> crashService =
        do_GetService("@mozilla.org/crashservice;1");
    if (!crashService) {
        return;
    }

    int32_t processType;
    int32_t crashType = nsICrashService::CRASH_TYPE_CRASH;

    switch (mProcessType) {
        case GeckoProcessType_Content:
            processType = nsICrashService::PROCESS_TYPE_CONTENT;
            break;
        case GeckoProcessType_Plugin: {
            processType = nsICrashService::PROCESS_TYPE_PLUGIN;
            nsAutoCString val;
            if (mNotes.Get(NS_LITERAL_CSTRING("PluginHang"), &val) &&
                val.Equals(NS_LITERAL_CSTRING("1"))) {
                crashType = nsICrashService::CRASH_TYPE_HANG;
            }
            break;
        }
        case GeckoProcessType_GMPlugin:
            processType = nsICrashService::PROCESS_TYPE_GMPLUGIN;
            break;
        default:
            NS_ERROR("unknown process type");
            return;
    }

    crashService->AddCrash(processType, crashType, mChildDumpID);
}
#endif

} 
} 
