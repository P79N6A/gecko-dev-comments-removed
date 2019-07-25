





































#ifdef MOZ_WIDGET_GTK2
#include <glib.h>
#endif

#include "base/process_util.h"

#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/plugins/PluginModuleParent.h"
#include "mozilla/plugins/BrowserStreamParent.h"

#include "nsContentUtils.h"
#include "nsCRT.h"
#include "nsNPAPIPlugin.h"

using base::KillProcess;

using mozilla::PluginLibrary;
using mozilla::ipc::NPRemoteIdentifier;
using mozilla::ipc::SyncChannel;

using namespace mozilla::plugins;

static const char kTimeoutPref[] = "dom.ipc.plugins.timeoutSecs";

PR_STATIC_ASSERT(sizeof(NPIdentifier) == sizeof(void*));

template<>
struct RunnableMethodTraits<mozilla::plugins::PluginModuleParent>
{
    typedef mozilla::plugins::PluginModuleParent Class;
    static void RetainCallee(Class* obj) { }
    static void ReleaseCallee(Class* obj) { }
};


PluginLibrary*
PluginModuleParent::LoadModule(const char* aFilePath)
{
    PLUGIN_LOG_DEBUG_FUNCTION;

    
    PluginModuleParent* parent = new PluginModuleParent(aFilePath);
    parent->mSubprocess->Launch();
    parent->Open(parent->mSubprocess->GetChannel(),
                 parent->mSubprocess->GetChildProcessHandle());

    TimeoutChanged(kTimeoutPref, parent);

    return parent;
}


PluginModuleParent::PluginModuleParent(const char* aFilePath)
    : mSubprocess(new PluginProcessParent(aFilePath))
    , mShutdown(false)
    , mNPNIface(NULL)
    , mPlugin(NULL)
    , mProcessStartTime(time(NULL))
    , mPluginCrashedTask(NULL)
{
    NS_ASSERTION(mSubprocess, "Out of memory!");

    if (!mValidIdentifiers.Init()) {
        NS_ERROR("Out of memory");
    }

    nsContentUtils::RegisterPrefCallback(kTimeoutPref, TimeoutChanged, this);
}

PluginModuleParent::~PluginModuleParent()
{
    NS_ASSERTION(OkToCleanup(), "unsafe destruction");

    if (mPluginCrashedTask) {
        mPluginCrashedTask->Cancel();
        mPluginCrashedTask = 0;
    }

    if (!mShutdown) {
        NS_WARNING("Plugin host deleted the module without shutting down.");
        NPError err;
        NP_Shutdown(&err);
    }
    NS_ASSERTION(mShutdown, "NP_Shutdown didn't");

    if (mSubprocess) {
        mSubprocess->Delete();
        mSubprocess = nsnull;
    }

    nsContentUtils::UnregisterPrefCallback(kTimeoutPref, TimeoutChanged, this);
}

void
PluginModuleParent::WriteExtraDataEntry(nsIFileOutputStream* stream,
                                        const char* key,
                                        const char* value)
{
    PRUint32 written;
    stream->Write(key, strlen(key), &written);
    stream->Write("=", 1, &written);
    stream->Write(value, strlen(value), &written);
    stream->Write("\n", 1, &written);
}

void
PluginModuleParent::WriteExtraDataForMinidump(nsIFile* dumpFile)
{
    
    nsCOMPtr<nsIFile> extraFile;
    nsresult rv = dumpFile->Clone(getter_AddRefs(extraFile));
    if (NS_FAILED(rv))
        return;

    nsAutoString leafName;
    rv = extraFile->GetLeafName(leafName);
    if (NS_FAILED(rv))
        return;

    leafName.Replace(leafName.Length() - 3, 3,
                     NS_LITERAL_STRING("extra"));
    rv = extraFile->SetLeafName(leafName);
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsIFileOutputStream> stream =
        do_CreateInstance("@mozilla.org/network/file-output-stream;1");
    
    rv = stream->Init(extraFile, 0x12, 0600, 0);
    if (NS_FAILED(rv))
        return;
    WriteExtraDataEntry(stream, "ProcessType", "plugin");
    char startTime[32];
    sprintf(startTime, "%lld", static_cast<PRInt64>(mProcessStartTime));
    WriteExtraDataEntry(stream, "StartupTime", startTime);

    
    const std::string& pluginFile = mSubprocess->GetPluginFilePath();
    size_t filePos = pluginFile.rfind(FILE_PATH_SEPARATOR);
    if (filePos == std::string::npos)
        filePos = 0;
    else
        filePos++;
    WriteExtraDataEntry(stream, "PluginFilename",
                        pluginFile.substr(filePos).c_str());
    
    
    WriteExtraDataEntry(stream, "PluginName", "");
    WriteExtraDataEntry(stream, "PluginVersion", "");

    if (!mCrashNotes.IsEmpty()) {
        WriteExtraDataEntry(stream, "Notes", mCrashNotes.get());
    }

    stream->Close();
}


bool
PluginModuleParent::RecvAppendNotesToCrashReport(const nsCString& aNotes)
{
    mCrashNotes.Append(aNotes);
    return true;
}

int
PluginModuleParent::TimeoutChanged(const char* aPref, void* aModule)
{
    AssertPluginThread();
    NS_ABORT_IF_FALSE(!strcmp(aPref, kTimeoutPref),
                      "unexpected pref callback");

    PRInt32 timeoutSecs = nsContentUtils::GetIntPref(kTimeoutPref, 0);
    int32 timeoutMs = (timeoutSecs > 0) ? (1000 * timeoutSecs) :
                      SyncChannel::kNoTimeout;

    static_cast<PluginModuleParent*>(aModule)->SetReplyTimeoutMs(timeoutMs);
    return 0;
}

void
PluginModuleParent::CleanupFromTimeout()
{
    if (!mShutdown)
        Close();
}

bool
PluginModuleParent::ShouldContinueFromReplyTimeout()
{
    
    bool waitMoar = false;

    if (!waitMoar) {
        
        
        
        
        
        
        
        
        
        MessageLoop::current()->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &PluginModuleParent::CleanupFromTimeout));

        
        
        
        KillProcess(ChildProcessHandle(), 1, false);
    }
    

    return waitMoar;
}

void
PluginModuleParent::ActorDestroy(ActorDestroyReason why)
{
    switch (why) {
    case AbnormalShutdown: {
        nsCOMPtr<nsIFile> dump;
        if (GetMinidump(getter_AddRefs(dump))) {
            WriteExtraDataForMinidump(dump);
            if (NS_SUCCEEDED(dump->GetLeafName(mDumpID))) {
                mDumpID.Replace(mDumpID.Length() - 4, 4,
                                NS_LITERAL_STRING(""));
            }
        }
        else {
            NS_WARNING("[PluginModuleParent::ActorDestroy] abnormal shutdown without minidump!");
        }

        mShutdown = true;
        
        
        if (mPlugin) {
            mPluginCrashedTask = NewRunnableMethod(
                this, &PluginModuleParent::NotifyPluginCrashed);
            MessageLoop::current()->PostTask(FROM_HERE, mPluginCrashedTask);
        }
        break;
    }
    case NormalShutdown:
        mShutdown = true;
        break;

    default:
        NS_ERROR("Unexpected shutdown reason for toplevel actor.");
    }
}

void
PluginModuleParent::NotifyPluginCrashed()
{
    
    mPluginCrashedTask = NULL;

    if (mPlugin)
        mPlugin->PluginCrashed(mDumpID);
}

PPluginInstanceParent*
PluginModuleParent::AllocPPluginInstance(const nsCString& aMimeType,
                                         const uint16_t& aMode,
                                         const nsTArray<nsCString>& aNames,
                                         const nsTArray<nsCString>& aValues,
                                         NPError* rv)
{
    NS_ERROR("Not reachable!");
    return NULL;
}

bool
PluginModuleParent::DeallocPPluginInstance(PPluginInstanceParent* aActor)
{
    PLUGIN_LOG_DEBUG_METHOD;
    delete aActor;
    return true;
}

void
PluginModuleParent::SetPluginFuncs(NPPluginFuncs* aFuncs)
{
    aFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    aFuncs->javaClass = nsnull;

    aFuncs->newp = nsnull; 
    aFuncs->destroy = NPP_Destroy;
    aFuncs->setwindow = NPP_SetWindow;
    aFuncs->newstream = NPP_NewStream;
    aFuncs->destroystream = NPP_DestroyStream;
    aFuncs->asfile = NPP_StreamAsFile;
    aFuncs->writeready = NPP_WriteReady;
    aFuncs->write = NPP_Write;
    aFuncs->print = NPP_Print;
    aFuncs->event = NPP_HandleEvent;
    aFuncs->urlnotify = NPP_URLNotify;
    aFuncs->getvalue = NPP_GetValue;
    aFuncs->setvalue = NPP_SetValue;
}

NPError
PluginModuleParent::NPP_Destroy(NPP instance,
                                NPSavedData** )
{
    
    
    
    
    
    PLUGIN_LOG_DEBUG_FUNCTION;

    PluginInstanceParent* parentInstance =
        static_cast<PluginInstanceParent*>(instance->pdata);

    if (!parentInstance)
        return NPERR_NO_ERROR;

    NPError retval = parentInstance->Destroy();
    instance->pdata = nsnull;

    (void) PluginInstanceParent::Call__delete__(parentInstance);
    return retval;
}

bool
PluginModuleParent::EnsureValidNPIdentifier(NPIdentifier aIdentifier)
{
    if (!mValidIdentifiers.GetEntry(aIdentifier)) {
        nsVoidPtrHashKey* newEntry = mValidIdentifiers.PutEntry(aIdentifier);
        if (!newEntry) {
            NS_ERROR("Out of memory?");
            return false;
        }
    }
    return true;
}

NPIdentifier
PluginModuleParent::GetValidNPIdentifier(NPRemoteIdentifier aRemoteIdentifier)
{
    NS_ASSERTION(mValidIdentifiers.IsInitialized(), "Not initialized!");
    if (aRemoteIdentifier &&
        mValidIdentifiers.GetEntry((NPIdentifier)aRemoteIdentifier)) {
        return (NPIdentifier)aRemoteIdentifier;
    }
    return 0;
}

NPError
PluginModuleParent::NPP_NewStream(NPP instance, NPMIMEType type,
                                  NPStream* stream, NPBool seekable,
                                  uint16_t* stype)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_NewStream(type, stream, seekable,
                            stype);
}

NPError
PluginModuleParent::NPP_SetWindow(NPP instance, NPWindow* window)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_SetWindow(window);
}

NPError
PluginModuleParent::NPP_DestroyStream(NPP instance,
                                      NPStream* stream,
                                      NPReason reason)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_DestroyStream(stream, reason);
}

int32_t
PluginModuleParent::NPP_WriteReady(NPP instance,
                                   NPStream* stream)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return -1;

    return s->WriteReady();
}

int32_t
PluginModuleParent::NPP_Write(NPP instance,
                              NPStream* stream,
                              int32_t offset,
                              int32_t len,
                              void* buffer)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return -1;

    return s->Write(offset, len, buffer);
}

void
PluginModuleParent::NPP_StreamAsFile(NPP instance,
                                     NPStream* stream,
                                     const char* fname)
{
    BrowserStreamParent* s = StreamCast(instance, stream);
    if (!s)
        return;

    s->StreamAsFile(fname);
}

void
PluginModuleParent::NPP_Print(NPP instance, NPPrint* platformPrint)
{
    PluginInstanceParent* i = InstCast(instance);
    if (i)
        i->NPP_Print(platformPrint);
}

int16_t
PluginModuleParent::NPP_HandleEvent(NPP instance, void* event)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return false;

    return i->NPP_HandleEvent(event);
}

void
PluginModuleParent::NPP_URLNotify(NPP instance, const char* url,
                                  NPReason reason, void* notifyData)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return;

    i->NPP_URLNotify(url, reason, notifyData);
}

NPError
PluginModuleParent::NPP_GetValue(NPP instance,
                                 NPPVariable variable, void *ret_value)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_GetValue(variable, ret_value);
}

NPError
PluginModuleParent::NPP_SetValue(NPP instance, NPNVariable variable,
                                 void *value)
{
    PluginInstanceParent* i = InstCast(instance);
    if (!i)
        return NPERR_GENERIC_ERROR;

    return i->NPP_SetValue(variable, value);
}

bool
PluginModuleParent::AnswerNPN_UserAgent(nsCString* userAgent)
{
    *userAgent = NullableString(mNPNIface->uagent(nsnull));
    return true;
}

bool
PluginModuleParent::RecvNPN_GetStringIdentifier(const nsCString& aString,
                                                NPRemoteIdentifier* aId)
{
    if (aString.IsVoid()) {
        NS_ERROR("Someone sent over a void string?!");
        return false;
    }

    NPIdentifier ident =
        mozilla::plugins::parent::_getstringidentifier(aString.BeginReading());
    if (!ident) {
        *aId = 0;
        return true;
    }

    if (!EnsureValidNPIdentifier(ident)) {
        NS_ERROR("Out of memory?");
        return false;
    }

    *aId = (NPRemoteIdentifier)ident;
    return true;
}

bool
PluginModuleParent::RecvNPN_GetIntIdentifier(const int32_t& aInt,
                                             NPRemoteIdentifier* aId)
{
    NPIdentifier ident = mozilla::plugins::parent::_getintidentifier(aInt);
    if (!ident) {
        *aId = 0;
        return true;
    }

    if (!EnsureValidNPIdentifier(ident)) {
        NS_ERROR("Out of memory?");
        return false;
    }

    *aId = (NPRemoteIdentifier)ident;
    return true;
}

bool
PluginModuleParent::RecvNPN_UTF8FromIdentifier(const NPRemoteIdentifier& aId,
                                               NPError *err,
                                               nsCString* aString)
{
    NPIdentifier ident = GetValidNPIdentifier(aId);
    if (!ident) {
        *err = NPERR_INVALID_PARAM;
        return true;
    }

    NPUTF8* val = mozilla::plugins::parent::_utf8fromidentifier(ident);
    if (!val) {
        *err = NPERR_INVALID_PARAM;
        return true;
    }

    aString->Assign(val);
    *err = NPERR_NO_ERROR;
    return true;
}

bool
PluginModuleParent::RecvNPN_IntFromIdentifier(const NPRemoteIdentifier& aId,
                                              NPError* err,
                                              int32_t* aInt)
{
    NPIdentifier ident = GetValidNPIdentifier(aId);
    if (!ident) {
        *err = NPERR_INVALID_PARAM;
        return true;
    }

    *aInt = mozilla::plugins::parent::_intfromidentifier(ident);
    *err = NPERR_NO_ERROR;
    return true;
}

bool
PluginModuleParent::RecvNPN_IdentifierIsString(const NPRemoteIdentifier& aId,
                                               bool* aIsString)
{
    NPIdentifier ident = GetValidNPIdentifier(aId);
    if (!ident) {
        *aIsString = false;
        return true;
    }

    *aIsString = mozilla::plugins::parent::_identifierisstring(ident);
    return true;
}

bool
PluginModuleParent::RecvNPN_GetStringIdentifiers(const nsTArray<nsCString>& aNames,
                                                 nsTArray<NPRemoteIdentifier>* aIds)
{
    NS_ASSERTION(aIds->IsEmpty(), "Non-empty array!");

    PRUint32 count = aNames.Length();
    if (!count) {
        NS_ERROR("No names to get!");
        return false;
    }

    nsTArray<NPUTF8*> buffers;
    nsTArray<NPIdentifier> ids;

    if (!(buffers.SetLength(count) &&
          ids.SetLength(count))) {
        NS_ERROR("Out of memory?");
        return false;
    }

    for (PRUint32 index = 0; index < count; ++index)
        buffers[index] = const_cast<NPUTF8*>(NullableStringGet(aNames[index]));

    mozilla::plugins::parent::_getstringidentifiers(
        const_cast<const NPUTF8**>(buffers.Elements()), count, ids.Elements());

    for (PRUint32 index = 0; index < count; index++) {
        NPIdentifier& id = ids[index];
        if (id) {
            if (!EnsureValidNPIdentifier(id)) {
                NS_ERROR("Out of memory?");
                return false;
            }
        }
        aIds->AppendElement((NPRemoteIdentifier)id);
    }

    return true;
}

PluginInstanceParent*
PluginModuleParent::InstCast(NPP instance)
{
    PluginInstanceParent* ip =
        static_cast<PluginInstanceParent*>(instance->pdata);

    
    
    if (!ip)
        return NULL;

    if (instance != ip->mNPP) {
        NS_RUNTIMEABORT("Corrupted plugin data.");
    }
    return ip;
}

BrowserStreamParent*
PluginModuleParent::StreamCast(NPP instance,
                               NPStream* s)
{
    PluginInstanceParent* ip = InstCast(instance);
    if (!ip)
        return NULL;

    BrowserStreamParent* sp =
        static_cast<BrowserStreamParent*>(static_cast<AStream*>(s->pdata));
    if (sp->mNPP != ip || s != sp->mStream) {
        NS_RUNTIMEABORT("Corrupted plugin stream data.");
    }
    return sp;
}

bool
PluginModuleParent::HasRequiredFunctions()
{
    return true;
}

#if defined(XP_UNIX) && !defined(XP_MACOSX)
nsresult
PluginModuleParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    mNPNIface = bFuncs;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (!CallNP_Initialize(error)) {
        return NS_ERROR_FAILURE;
    }
    else if (*error != NPERR_NO_ERROR) {
        return NS_OK;
    }

    SetPluginFuncs(pFuncs);
    return NS_OK;
}
#else
nsresult
PluginModuleParent::NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    mNPNIface = bFuncs;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (!CallNP_Initialize(error))
        return NS_ERROR_FAILURE;

    return NS_OK;
}
#endif

nsresult
PluginModuleParent::NP_Shutdown(NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    bool ok = CallNP_Shutdown(error);

    
    
    
    
    Close();

    return ok ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
PluginModuleParent::NP_GetMIMEDescription(const char** mimeDesc)
{
    PLUGIN_LOG_DEBUG_METHOD;

    *mimeDesc = "application/x-foobar";
    return NS_OK;
}

nsresult
PluginModuleParent::NP_GetValue(void *future, NPPVariable aVariable,
                                   void *aValue, NPError* error)
{
    PR_LOG(gPluginLog, PR_LOG_WARNING, ("%s Not implemented, requested variable %i", __FUNCTION__,
                                        (int) aVariable));

    
    *error = NPERR_GENERIC_ERROR;
    return NS_OK;
}

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
nsresult
PluginModuleParent::NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error)
{
    NS_ASSERTION(pFuncs, "Null pointer!");

    SetPluginFuncs(pFuncs);
    *error = NPERR_NO_ERROR;
    return NS_OK;
}
#endif

nsresult
PluginModuleParent::NPP_New(NPMIMEType pluginType, NPP instance,
                            uint16_t mode, int16_t argc, char* argn[],
                            char* argv[], NPSavedData* saved,
                            NPError* error)
{
    PLUGIN_LOG_DEBUG_METHOD;

    if (mShutdown) {
        *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    
    nsTArray<nsCString> names;
    nsTArray<nsCString> values;

    for (int i = 0; i < argc; ++i) {
        names.AppendElement(NullableString(argn[i]));
        values.AppendElement(NullableString(argv[i]));
    }

    PluginInstanceParent* parentInstance =
        new PluginInstanceParent(this, instance, mNPNIface);

    if (!parentInstance->Init()) {
        delete parentInstance;
        return NS_ERROR_FAILURE;
    }

    instance->pdata = parentInstance;

    if (!CallPPluginInstanceConstructor(parentInstance,
                                        nsDependentCString(pluginType), mode,
                                        names, values, error)) {
        
        instance->pdata = nsnull;
        
        
        
        if (NPERR_NO_ERROR == *error)
            *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    if (*error != NPERR_NO_ERROR) {
        NPP_Destroy(instance, 0);
        return *error;
    }

    return NS_OK;
}

bool
PluginModuleParent::AnswerNPN_GetValue_WithBoolReturn(const NPNVariable& aVariable,
                                                      NPError* aError,
                                                      bool* aBoolVal)
{
    NPBool boolVal = false;
    *aError = mozilla::plugins::parent::_getvalue(nsnull, aVariable, &boolVal);
    *aBoolVal = boolVal ? true : false;
    return true;
}

#if !defined(MOZ_WIDGET_GTK2)
bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    NS_RUNTIMEABORT("unreached");
    return false;
}

#else
static const int kMaxChancesToProcessEvents = 20;

bool
PluginModuleParent::AnswerProcessSomeEvents()
{
    PLUGIN_LOG_DEBUG(("Spinning mini nested loop ..."));

    int i = 0;
    for (; i < kMaxChancesToProcessEvents; ++i)
        if (!g_main_context_iteration(NULL, FALSE))
            break;

    PLUGIN_LOG_DEBUG(("... quitting mini nested loop; processed %i tasks", i));

    return true;
}
#endif
