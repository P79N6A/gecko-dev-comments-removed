





#include "PluginAsyncSurrogate.h"

#include "base/message_loop.h"
#include "base/message_pump_default.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/plugins/PluginInstanceParent.h"
#include "mozilla/plugins/PluginModuleParent.h"
#include "mozilla/plugins/PluginScriptableObjectParent.h"
#include "mozilla/Telemetry.h"
#include "nsJSNPRuntime.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsPluginInstanceOwner.h"
#include "nsPluginStreamListenerPeer.h"
#include "npruntime.h"
#include "nsThreadUtils.h"
#include "PluginMessageUtils.h"

namespace mozilla {
namespace plugins {

AsyncNPObject::AsyncNPObject(PluginAsyncSurrogate* aSurrogate)
  : NPObject()
  , mSurrogate(aSurrogate)
  , mRealObject(nullptr)
{
}

AsyncNPObject::~AsyncNPObject()
{
  if (mRealObject) {
    parent::_releaseobject(mRealObject);
    mRealObject = nullptr;
  }
}

NPObject*
AsyncNPObject::GetRealObject()
{
  if (mRealObject) {
    return mRealObject;
  }
  PluginInstanceParent* instance = PluginInstanceParent::Cast(mSurrogate->GetNPP());
  if (!instance) {
    return nullptr;
  }
  NPError err = instance->NPP_GetValue(NPPVpluginScriptableNPObject,
                                       &mRealObject);
  if (err != NPERR_NO_ERROR) {
    return nullptr;
  }
  return mRealObject;
}

class MOZ_STACK_CLASS RecursionGuard
{
public:
  RecursionGuard()
    : mIsRecursive(sHasEntered)
  {
    if (!mIsRecursive) {
      sHasEntered = true;
    }
  }

  ~RecursionGuard()
  {
    if (!mIsRecursive) {
      sHasEntered = false;
    }
  }

  inline bool
  IsRecursive()
  {
    return mIsRecursive;
  }

private:
  bool        mIsRecursive;
  static bool sHasEntered;
};

bool RecursionGuard::sHasEntered = false;

PluginAsyncSurrogate::PluginAsyncSurrogate(PluginModuleParent* aParent)
  : mParent(aParent)
  , mInstance(nullptr)
  , mMode(0)
  , mWindow(nullptr)
  , mAcceptCalls(false)
  , mInstantiated(false)
  , mAsyncSetWindow(false)
  , mInitCancelled(false)
  , mAsyncCallsInFlight(0)
{
  MOZ_ASSERT(aParent);
}

PluginAsyncSurrogate::~PluginAsyncSurrogate()
{
}

bool
PluginAsyncSurrogate::Init(NPMIMEType aPluginType, NPP aInstance, uint16_t aMode,
                           int16_t aArgc, char* aArgn[], char* aArgv[])
{
  mMimeType = aPluginType;
  mInstance = aInstance;
  mMode = aMode;
  for (int i = 0; i < aArgc; ++i) {
    mNames.AppendElement(NullableString(aArgn[i]));
    mValues.AppendElement(NullableString(aArgv[i]));
  }
  return true;
}

 bool
PluginAsyncSurrogate::Create(PluginModuleParent* aParent, NPMIMEType aPluginType,
                             NPP aInstance, uint16_t aMode, int16_t aArgc,
                             char* aArgn[], char* aArgv[])
{
  nsRefPtr<PluginAsyncSurrogate> surrogate(new PluginAsyncSurrogate(aParent));
  if (!surrogate->Init(aPluginType, aInstance, aMode, aArgc, aArgn, aArgv)) {
    return false;
  }
  PluginAsyncSurrogate* rawSurrogate = nullptr;
  surrogate.forget(&rawSurrogate);
  aInstance->pdata = static_cast<PluginDataResolver*>(rawSurrogate);
  return true;
}

 PluginAsyncSurrogate*
PluginAsyncSurrogate::Cast(NPP aInstance)
{
  MOZ_ASSERT(aInstance);
  PluginDataResolver* resolver =
    reinterpret_cast<PluginDataResolver*>(aInstance->pdata);
  if (!resolver) {
    return nullptr;
  }
  return resolver->GetAsyncSurrogate();
}

nsresult
PluginAsyncSurrogate::NPP_New(NPError* aError)
{
  nsresult rv = mParent->NPP_NewInternal(mMimeType.BeginWriting(), mInstance,
                                         mMode, mNames, mValues, nullptr,
                                         aError);
  if (NS_FAILED(rv)) {
    return rv;
  }
  return NS_OK;
}

void
PluginAsyncSurrogate::NP_GetEntryPoints(NPPluginFuncs* aFuncs)
{
  aFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  aFuncs->destroy = &NPP_Destroy;
  aFuncs->getvalue = &NPP_GetValue;
  aFuncs->setvalue = &NPP_SetValue;
  aFuncs->newstream = &NPP_NewStream;
  aFuncs->setwindow = &NPP_SetWindow;
  aFuncs->writeready = &NPP_WriteReady;
  aFuncs->event = &NPP_HandleEvent;
  
  
  aFuncs->write = &PluginModuleParent::NPP_Write;
  aFuncs->asfile = &PluginModuleParent::NPP_StreamAsFile;
  aFuncs->destroystream = &PluginModuleParent::NPP_DestroyStream;
}

NPError
PluginAsyncSurrogate::NPP_Destroy(NPSavedData** aSave)
{
  if (!WaitForInit()) {
    return NPERR_GENERIC_ERROR;
  }
  return PluginModuleParent::NPP_Destroy(mInstance, aSave);
}

NPError
PluginAsyncSurrogate::NPP_GetValue(NPPVariable aVariable, void* aRetval)
{
  if (aVariable != NPPVpluginScriptableNPObject) {
    if (!WaitForInit()) {
      return NPERR_GENERIC_ERROR;
    }
    PluginInstanceParent* instance = PluginInstanceParent::Cast(mInstance);
    MOZ_ASSERT(instance);
    return instance->NPP_GetValue(aVariable, aRetval);
  }

  NPObject* npobject = parent::_createobject(mInstance,
                                             const_cast<NPClass*>(GetClass()));
  MOZ_ASSERT(npobject);
  MOZ_ASSERT(npobject->_class == GetClass());
  MOZ_ASSERT(npobject->referenceCount == 1);
  *(NPObject**)aRetval = npobject;
  return npobject ? NPERR_NO_ERROR : NPERR_GENERIC_ERROR;
}

NPError
PluginAsyncSurrogate::NPP_SetValue(NPNVariable aVariable, void* aValue)
{
  if (!WaitForInit()) {
    return NPERR_GENERIC_ERROR;
  }
  return PluginModuleParent::NPP_SetValue(mInstance, aVariable, aValue);
}

NPError
PluginAsyncSurrogate::NPP_NewStream(NPMIMEType aType, NPStream* aStream,
                                    NPBool aSeekable, uint16_t* aStype)
{
  mPendingNewStreamCalls.AppendElement(PendingNewStreamCall(aType, aStream,
                                                            aSeekable));
  if (aStype) {
    *aStype = nsPluginStreamListenerPeer::STREAM_TYPE_UNKNOWN;
  }
  return NPERR_NO_ERROR;
}

NPError
PluginAsyncSurrogate::NPP_SetWindow(NPWindow* aWindow)
{
  mWindow = aWindow;
  mAsyncSetWindow = false;
  return NPERR_NO_ERROR;
}

nsresult
PluginAsyncSurrogate::AsyncSetWindow(NPWindow* aWindow)
{
  mWindow = aWindow;
  mAsyncSetWindow = true;
  return NS_OK;
}

void
PluginAsyncSurrogate::NPP_Print(NPPrint* aPrintInfo)
{
  
}

int16_t
PluginAsyncSurrogate::NPP_HandleEvent(void* event)
{
  
  return false;
}

int32_t
PluginAsyncSurrogate::NPP_WriteReady(NPStream* aStream)
{
  
  
  return 0;
}

 NPError
PluginAsyncSurrogate::NPP_Destroy(NPP aInstance, NPSavedData** aSave)
{
  PluginAsyncSurrogate* rawSurrogate = Cast(aInstance);
  MOZ_ASSERT(rawSurrogate);
  PluginModuleParent* module = rawSurrogate->GetParent();
  if (module && !module->IsInitialized()) {
    
    nsRefPtr<PluginAsyncSurrogate> surrogate(dont_AddRef(rawSurrogate));
    aInstance->pdata = nullptr;
    
    
    bool removeOk = module->RemovePendingSurrogate(surrogate);
    MOZ_ASSERT(removeOk);
    if (!removeOk) {
      return NPERR_GENERIC_ERROR;
    }
    surrogate->mInitCancelled = true;
    return NPERR_NO_ERROR;
  }
  return rawSurrogate->NPP_Destroy(aSave);
}

 NPError
PluginAsyncSurrogate::NPP_GetValue(NPP aInstance, NPPVariable aVariable,
                                   void* aRetval)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_GetValue(aVariable, aRetval);
}

 NPError
PluginAsyncSurrogate::NPP_SetValue(NPP aInstance, NPNVariable aVariable,
                                   void* aValue)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_SetValue(aVariable, aValue);
}

 NPError
PluginAsyncSurrogate::NPP_NewStream(NPP aInstance, NPMIMEType aType,
                                    NPStream* aStream, NPBool aSeekable,
                                    uint16_t* aStype)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_NewStream(aType, aStream, aSeekable, aStype);
}

 NPError
PluginAsyncSurrogate::NPP_SetWindow(NPP aInstance, NPWindow* aWindow)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_SetWindow(aWindow);
}

 void
PluginAsyncSurrogate::NPP_Print(NPP aInstance, NPPrint* aPrintInfo)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  surrogate->NPP_Print(aPrintInfo);
}

 int16_t
PluginAsyncSurrogate::NPP_HandleEvent(NPP aInstance, void* aEvent)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_HandleEvent(aEvent);
}

 int32_t
PluginAsyncSurrogate::NPP_WriteReady(NPP aInstance, NPStream* aStream)
{
  PluginAsyncSurrogate* surrogate = Cast(aInstance);
  MOZ_ASSERT(surrogate);
  return surrogate->NPP_WriteReady(aStream);
}

PluginAsyncSurrogate::PendingNewStreamCall::PendingNewStreamCall(
    NPMIMEType aType, NPStream* aStream, NPBool aSeekable)
  : mType(NullableString(aType))
  , mStream(aStream)
  , mSeekable(aSeekable)
{
}

 bool
PluginAsyncSurrogate::SetStreamType(NPStream* aStream, uint16_t aStreamType)
{
  nsNPAPIStreamWrapper* wrapper =
    reinterpret_cast<nsNPAPIStreamWrapper*>(aStream->ndata);
  if (!wrapper) {
    return false;
  }
  nsNPAPIPluginStreamListener* streamListener = wrapper->GetStreamListener();
  if (!streamListener) {
    return false;
  }
  return streamListener->SetStreamType(aStreamType);
}

void
PluginAsyncSurrogate::OnInstanceCreated(PluginInstanceParent* aInstance)
{
  for (PRUint32 i = 0, len = mPendingNewStreamCalls.Length(); i < len; ++i) {
    PendingNewStreamCall& curPendingCall = mPendingNewStreamCalls[i];
    uint16_t streamType = NP_NORMAL;
    NPError curError = aInstance->NPP_NewStream(
                    const_cast<char*>(NullableStringGet(curPendingCall.mType)),
                    curPendingCall.mStream, curPendingCall.mSeekable,
                    &streamType);
    if (curError != NPERR_NO_ERROR) {
      
      parent::_destroystream(mInstance, curPendingCall.mStream, NPRES_DONE);
    }
  }
  mPendingNewStreamCalls.Clear();
  mInstantiated = true;
}











bool
PluginAsyncSurrogate::WaitForInit()
{
  if (mInitCancelled) {
    return false;
  }
  if (mAcceptCalls) {
    return true;
  }
  Telemetry::AutoTimer<Telemetry::BLOCKED_ON_PLUGINASYNCSURROGATE_WAITFORINIT_MS>
    timer(mParent->GetHistogramKey());
  bool result = false;
  MOZ_ASSERT(mParent);
  if (mParent->IsChrome()) {
    PluginProcessParent* process = static_cast<PluginModuleChromeParent*>(mParent)->Process();
    MOZ_ASSERT(process);
    process->SetCallRunnableImmediately(true);
    if (!process->WaitUntilConnected()) {
      return false;
    }
  }
  if (!mParent->WaitForIPCConnection()) {
    return false;
  }
  if (!mParent->IsChrome()) {
    
    
    dom::ContentChild* cp = dom::ContentChild::GetSingleton();
    mozilla::ipc::MessageChannel* contentChannel = cp->GetIPCChannel();
    MOZ_ASSERT(contentChannel);
    while (!mParent->mNPInitialized) {
      result = contentChannel->WaitForIncomingMessage();
      if (!result) {
        return result;
      }
    }
  }
  mozilla::ipc::MessageChannel* channel = mParent->GetIPCChannel();
  MOZ_ASSERT(channel);
  while (!mAcceptCalls) {
    result = channel->WaitForIncomingMessage();
    if (!result) {
      break;
    }
  }
  return result;
}

void
PluginAsyncSurrogate::AsyncCallDeparting()
{
  ++mAsyncCallsInFlight;
  if (!mPluginDestructionGuard) {
    mPluginDestructionGuard = MakeUnique<PluginDestructionGuard>(this);
  }
}

void
PluginAsyncSurrogate::AsyncCallArriving()
{
  MOZ_ASSERT(mAsyncCallsInFlight > 0);
  if (--mAsyncCallsInFlight == 0) {
    mPluginDestructionGuard.reset(nullptr);
  }
}

void
PluginAsyncSurrogate::NotifyAsyncInitFailed()
{
  
  for (uint32_t i = 0, len = mPendingNewStreamCalls.Length(); i < len; ++i) {
    PendingNewStreamCall& curPendingCall = mPendingNewStreamCalls[i];
    parent::_destroystream(mInstance, curPendingCall.mStream, NPRES_DONE);
  }
  mPendingNewStreamCalls.Clear();

  nsNPAPIPluginInstance* inst =
    static_cast<nsNPAPIPluginInstance*>(mInstance->ndata);
  if (!inst) {
      return;
  }
  nsPluginInstanceOwner* owner = inst->GetOwner();
  if (!owner) {
      return;
  }
  owner->NotifyHostAsyncInitFailed();
}


NPObject*
PluginAsyncSurrogate::ScriptableAllocate(NPP aInstance, NPClass* aClass)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aClass != GetClass()) {
    NS_ERROR("Huh?! Wrong class!");
    return nullptr;
  }

  return new AsyncNPObject(Cast(aInstance));
}


void
PluginAsyncSurrogate::ScriptableInvalidate(NPObject* aObject)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return;
  }
  realObject->_class->invalidate(realObject);
}


void
PluginAsyncSurrogate::ScriptableDeallocate(NPObject* aObject)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  delete object;
}


bool
PluginAsyncSurrogate::ScriptableHasMethod(NPObject* aObject,
                                                  NPIdentifier aName)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  RecursionGuard guard;
  if (guard.IsRecursive()) {
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  MOZ_ASSERT(object);
  bool checkPluginObject = !object->mSurrogate->mInstantiated &&
                           !object->mSurrogate->mAcceptCalls;

  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  bool result = realObject->_class->hasMethod(realObject, aName);
  if (!result && checkPluginObject) {
    
    
    
    const NPNetscapeFuncs* npn = object->mSurrogate->mParent->GetNetscapeFuncs();
    NPObject* pluginObject = nullptr;
    NPError nperror = npn->getvalue(object->mSurrogate->mInstance,
                                    NPNVPluginElementNPObject,
                                    (void*)&pluginObject);
    if (nperror == NPERR_NO_ERROR) {
      NPPAutoPusher nppPusher(object->mSurrogate->mInstance);
      result = pluginObject->_class->hasMethod(pluginObject, aName);
      npn->releaseobject(pluginObject);
      NPUTF8* idstr = npn->utf8fromidentifier(aName);
      npn->memfree(idstr);
    }
  }
  return result;
}

bool
PluginAsyncSurrogate::GetPropertyHelper(NPObject* aObject, NPIdentifier aName,
                                        bool* aHasProperty, bool* aHasMethod,
                                        NPVariant* aResult)
{
  PLUGIN_LOG_DEBUG_FUNCTION;

  if (!aObject) {
    return false;
  }

  RecursionGuard guard;
  if (guard.IsRecursive()) {
    return false;
  }

  WaitForInit();

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  NPObject* realObject = object->GetRealObject();
  if (realObject->_class != PluginScriptableObjectParent::GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  PluginScriptableObjectParent* actor =
    static_cast<ParentNPObject*>(realObject)->parent;
  bool success = actor->GetPropertyHelper(aName, aHasProperty, aHasMethod, aResult);
  if (!success) {
    const NPNetscapeFuncs* npn = mParent->GetNetscapeFuncs();
    NPObject* pluginObject = nullptr;
    NPError nperror = npn->getvalue(mInstance, NPNVPluginElementNPObject,
                                    (void*)&pluginObject);
    if (nperror == NPERR_NO_ERROR) {
      NPPAutoPusher nppPusher(mInstance);
      bool hasProperty = nsJSObjWrapper::HasOwnProperty(pluginObject, aName);
      NPUTF8* idstr = npn->utf8fromidentifier(aName);
      npn->memfree(idstr);
      bool hasMethod = false;
      if (hasProperty) {
        hasMethod = pluginObject->_class->hasMethod(pluginObject, aName);
        success = pluginObject->_class->getProperty(pluginObject, aName, aResult);
        idstr = npn->utf8fromidentifier(aName);
        npn->memfree(idstr);
      }
      *aHasProperty = hasProperty;
      *aHasMethod = hasMethod;
      npn->releaseobject(pluginObject);
    }
  }
  return success;
}


bool
PluginAsyncSurrogate::ScriptableInvoke(NPObject* aObject,
                                               NPIdentifier aName,
                                               const NPVariant* aArgs,
                                               uint32_t aArgCount,
                                               NPVariant* aResult)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->invoke(realObject, aName, aArgs, aArgCount, aResult);
}


bool
PluginAsyncSurrogate::ScriptableInvokeDefault(NPObject* aObject,
                                                      const NPVariant* aArgs,
                                                      uint32_t aArgCount,
                                                      NPVariant* aResult)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->invokeDefault(realObject, aArgs, aArgCount, aResult);
}


bool
PluginAsyncSurrogate::ScriptableHasProperty(NPObject* aObject,
                                                    NPIdentifier aName)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  RecursionGuard guard;
  if (guard.IsRecursive()) {
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  MOZ_ASSERT(object);
  bool checkPluginObject = !object->mSurrogate->mInstantiated &&
                           !object->mSurrogate->mAcceptCalls;

  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  bool result = realObject->_class->hasProperty(realObject, aName);
  const NPNetscapeFuncs* npn = object->mSurrogate->mParent->GetNetscapeFuncs();
  NPUTF8* idstr = npn->utf8fromidentifier(aName);
  npn->memfree(idstr);
  if (!result && checkPluginObject) {
    
    
    
    NPObject* pluginObject = nullptr;
    NPError nperror = npn->getvalue(object->mSurrogate->mInstance,
                                    NPNVPluginElementNPObject,
                                    (void*)&pluginObject);
    if (nperror == NPERR_NO_ERROR) {
      NPPAutoPusher nppPusher(object->mSurrogate->mInstance);
      result = nsJSObjWrapper::HasOwnProperty(pluginObject, aName);
      npn->releaseobject(pluginObject);
      idstr = npn->utf8fromidentifier(aName);
      npn->memfree(idstr);
    }
  }
  return result;
}


bool
PluginAsyncSurrogate::ScriptableGetProperty(NPObject* aObject,
                                                    NPIdentifier aName,
                                                    NPVariant* aResult)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  
  NS_NOTREACHED("Shouldn't ever call this directly!");
  return false;
}


bool
PluginAsyncSurrogate::ScriptableSetProperty(NPObject* aObject,
                                                    NPIdentifier aName,
                                                    const NPVariant* aValue)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->setProperty(realObject, aName, aValue);
}


bool
PluginAsyncSurrogate::ScriptableRemoveProperty(NPObject* aObject,
                                                       NPIdentifier aName)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->removeProperty(realObject, aName);
}


bool
PluginAsyncSurrogate::ScriptableEnumerate(NPObject* aObject,
                                                  NPIdentifier** aIdentifiers,
                                                  uint32_t* aCount)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->enumerate(realObject, aIdentifiers, aCount);
}


bool
PluginAsyncSurrogate::ScriptableConstruct(NPObject* aObject,
                                                  const NPVariant* aArgs,
                                                  uint32_t aArgCount,
                                                  NPVariant* aResult)
{
  PLUGIN_LOG_DEBUG_FUNCTION;
  if (aObject->_class != GetClass()) {
    NS_ERROR("Don't know what kind of object this is!");
    return false;
  }

  AsyncNPObject* object = static_cast<AsyncNPObject*>(aObject);
  if (!object->mSurrogate->WaitForInit()) {
    return false;
  }
  NPObject* realObject = object->GetRealObject();
  if (!realObject) {
    return false;
  }
  return realObject->_class->construct(realObject, aArgs, aArgCount, aResult);
}

const NPClass PluginAsyncSurrogate::sNPClass = {
  NP_CLASS_STRUCT_VERSION,
  PluginAsyncSurrogate::ScriptableAllocate,
  PluginAsyncSurrogate::ScriptableDeallocate,
  PluginAsyncSurrogate::ScriptableInvalidate,
  PluginAsyncSurrogate::ScriptableHasMethod,
  PluginAsyncSurrogate::ScriptableInvoke,
  PluginAsyncSurrogate::ScriptableInvokeDefault,
  PluginAsyncSurrogate::ScriptableHasProperty,
  PluginAsyncSurrogate::ScriptableGetProperty,
  PluginAsyncSurrogate::ScriptableSetProperty,
  PluginAsyncSurrogate::ScriptableRemoveProperty,
  PluginAsyncSurrogate::ScriptableEnumerate,
  PluginAsyncSurrogate::ScriptableConstruct
};

PushSurrogateAcceptCalls::PushSurrogateAcceptCalls(PluginInstanceParent* aInstance)
  : mSurrogate(nullptr)
  , mPrevAcceptCallsState(false)
{
  MOZ_ASSERT(aInstance);
  mSurrogate = aInstance->GetAsyncSurrogate();
  if (mSurrogate) {
    mPrevAcceptCallsState = mSurrogate->SetAcceptingCalls(true);
  }
}

PushSurrogateAcceptCalls::~PushSurrogateAcceptCalls()
{
  if (mSurrogate) {
    mSurrogate->SetAcceptingCalls(mPrevAcceptCallsState);
  }
}

} 
} 
