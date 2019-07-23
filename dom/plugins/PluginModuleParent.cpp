





































#include "mozilla/plugins/PluginModuleParent.h"
#include "mozilla/plugins/BrowserStreamParent.h"

#include "nsNPAPIPlugin.h"

using mozilla::PluginLibrary;

using mozilla::ipc::NPRemoteIdentifier;

using namespace mozilla::plugins;

PR_STATIC_ASSERT(sizeof(NPIdentifier) == sizeof(void*));


PluginLibrary*
PluginModuleParent::LoadModule(const char* aFilePath)
{
    _MOZ_LOG(__FUNCTION__);

    
    PluginModuleParent* parent = new PluginModuleParent(aFilePath);
    parent->mSubprocess->Launch();
    parent->Open(parent->mSubprocess->GetChannel(),
                 parent->mSubprocess->GetChildProcessHandle());

    return parent;
}


PluginModuleParent::PluginModuleParent(const char* aFilePath)
{
    mSubprocess = new PluginProcessParent(aFilePath);
    NS_ASSERTION(mSubprocess, "Out of memory!");

#ifdef DEBUG
    PRBool ok =
#endif
    mValidIdentifiers.Init();
    NS_ASSERTION(ok, "Out of memory!");
}

PluginModuleParent::~PluginModuleParent()
{
    if (mSubprocess) {
        mSubprocess->Delete();
        mSubprocess = nsnull;
    }
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
    _MOZ_LOG(__FUNCTION__);
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
                                NPSavedData** save)
{
    
    
    
    
    
    _MOZ_LOG(__FUNCTION__);

    PluginInstanceParent* parentInstance =
        static_cast<PluginInstanceParent*>(instance->pdata);

    if (!parentInstance)
        return NPERR_NO_ERROR;

    parentInstance->Destroy();

    NPError prv;
    if (!PPluginInstanceParent::Call__delete__(parentInstance, &prv)) {
        prv = NPERR_GENERIC_ERROR;
    }
    instance->pdata = nsnull;

    return prv;
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
    return InstCast(instance)->NPP_NewStream(type, stream, seekable,
                                             stype);
}

NPError
PluginModuleParent::NPP_SetWindow(NPP instance, NPWindow* window)
{
     return InstCast(instance)->NPP_SetWindow(window);
}

NPError
PluginModuleParent::NPP_DestroyStream(NPP instance,
                                      NPStream* stream,
                                      NPReason reason)
{
    return InstCast(instance)->NPP_DestroyStream(stream, reason);
}

int32_t
PluginModuleParent::NPP_WriteReady(NPP instance,
                                   NPStream* stream)
{
    return StreamCast(instance, stream)->WriteReady();
}

int32_t
PluginModuleParent::NPP_Write(NPP instance,
                              NPStream* stream,
                              int32_t offset,
                              int32_t len,
                              void* buffer)
{
    return StreamCast(instance, stream)->Write(offset, len, buffer);
}

void
PluginModuleParent::NPP_StreamAsFile(NPP instance,
                                     NPStream* stream,
                                     const char* fname)
{
    StreamCast(instance, stream)->StreamAsFile(fname);
}

void
PluginModuleParent::NPP_Print(NPP instance, NPPrint* platformPrint)
{
    InstCast(instance)->NPP_Print(platformPrint);
}

int16_t
PluginModuleParent::NPP_HandleEvent(NPP instance, void* event)
{
    return InstCast(instance)->NPP_HandleEvent(event);
}

void
PluginModuleParent::NPP_URLNotify(NPP instance, const char* url,
                                  NPReason reason, void* notifyData)
{
    return InstCast(instance)->NPP_URLNotify(url, reason, notifyData);
}

NPError
PluginModuleParent::NPP_GetValue(NPP instance,
                                 NPPVariable variable, void *ret_value)
{
    return InstCast(instance)->NPP_GetValue(variable, ret_value);
}

NPError
PluginModuleParent::NPP_SetValue(NPP instance, NPNVariable variable,
                                 void *value)
{
    return InstCast(instance)->NPP_SetValue(variable, value);
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

    nsAutoTArray<NPUTF8*, 10> buffers;
    nsAutoTArray<NPIdentifier, 10> ids;

    if (!(buffers.SetLength(count) &&
          ids.SetLength(count) &&
          aIds->SetCapacity(count))) {
        NS_ERROR("Out of memory?");
        return false;
    }

    for (PRUint32 index = 0; index < count; index++) {
        buffers[index] = const_cast<NPUTF8*>(aNames[index].BeginReading());
        NS_ASSERTION(buffers[index], "Null pointer should be impossible!");
    }

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
    _MOZ_LOG(__FUNCTION__);

    mNPNIface = bFuncs;

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
    _MOZ_LOG(__FUNCTION__);

    mNPNIface = bFuncs;

    if (!CallNP_Initialize(error))
        return NS_ERROR_FAILURE;

    return NS_OK;
}
#endif

nsresult
PluginModuleParent::NP_Shutdown(NPError* error)
{
    _MOZ_LOG(__FUNCTION__);

    

    bool ok = CallNP_Shutdown(error);
    Close();

    return ok ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
PluginModuleParent::NP_GetMIMEDescription(char** mimeDesc)
{
    _MOZ_LOG(__FUNCTION__);

    *mimeDesc = (char*)"application/x-foobar";
    return NS_OK;
}

nsresult
PluginModuleParent::NP_GetValue(void *future, NPPVariable aVariable,
                                   void *aValue, NPError* error)
{
    _MOZ_LOG(__FUNCTION__);

    
    printf("[%s] Not yet implemented\n", __FUNCTION__);

    *error = NPERR_GENERIC_ERROR;
    return NS_OK;
}

#if defined(XP_WIN) || defined(XP_MACOSX)
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
    _MOZ_LOG(__FUNCTION__);

    
    nsTArray<nsCString> names;
    nsTArray<nsCString> values;

    for (int i = 0; i < argc; ++i) {
        names.AppendElement(NullableString(argn[i]));
        values.AppendElement(NullableString(argv[i]));
    }

    PluginInstanceParent* parentInstance =
        new PluginInstanceParent(this, instance, mNPNIface);

    instance->pdata = parentInstance;

    if (!CallPPluginInstanceConstructor(parentInstance,
                                        nsDependentCString(pluginType), mode,
                                        names, values, error)) {
        
        instance->pdata = nsnull;
        
        
        
        if (NPERR_NO_ERROR == *error)
            *error = NPERR_GENERIC_ERROR;
        return NS_ERROR_FAILURE;
    }

    printf ("[PluginModuleParent] %s: got return value %hd\n", __FUNCTION__,
            *error);

    if (*error != NPERR_NO_ERROR) {
        PPluginInstanceParent::Call__delete__(parentInstance, error);
        instance->pdata = nsnull;
        return NS_ERROR_FAILURE;
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
