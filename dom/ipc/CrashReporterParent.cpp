




#include "CrashReporterParent.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"

#include <time.h>

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
}

CrashReporterParent::CrashReporterParent()
    :
#ifdef MOZ_CRASHREPORTER
      mNotes(4),
#endif
      mStartTime(time(NULL))
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
    return ret;
}
#endif

} 
} 
