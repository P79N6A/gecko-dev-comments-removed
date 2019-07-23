




































#include "plugin.h"
#include "nsIServiceManager.h"
#include "nsIMemory.h"
#include "nsISupportsUtils.h" 



nsIServiceManager * gServiceManager = NULL;


#ifdef XP_UNIX
#define MIME_TYPES_HANDLED  "application/simple-plugin"
#define PLUGIN_NAME         "Simple Plugin Example for Mozilla"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED"::"PLUGIN_NAME
#define PLUGIN_DESCRIPTION  PLUGIN_NAME " (Plug-ins SDK sample)" 

char* NPP_GetMIMEDescription(void)
{
    return(MIME_TYPES_DESCRIPTION);
}


NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
      *((char **)aValue) = PLUGIN_NAME;
      break;
    case NPPVpluginDescriptionString:
      *((char **)aValue) = PLUGIN_DESCRIPTION;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;
}
#endif 





NPError NS_PluginInitialize()
{
  
  
  nsISupports * sm = NULL;
  
  NPN_GetValue(NULL, NPNVserviceManager, &sm);

  
  
  
  if(sm) {
    sm->QueryInterface(NS_GET_IID(nsIServiceManager), (void**)&gServiceManager);
    NS_RELEASE(sm);
  }
  
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
  
  NS_IF_RELEASE(gServiceManager);
  gServiceManager = NULL;
}





nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}





nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE),
  mScriptablePeer(NULL)
{
  mString[0] = '\0';
}

nsPluginInstance::~nsPluginInstance()
{
  
  
  
  
  mScriptablePeer->SetInstance(NULL);
  NS_IF_RELEASE(mScriptablePeer);
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mInitialized = TRUE;
  return TRUE;
}

void nsPluginInstance::shut()
{
  mInitialized = FALSE;
}

NPBool nsPluginInstance::isInitialized()
{
  return mInitialized;
}

void nsPluginInstance::getVersion(char* *aVersion)
{
  const char *ua = NPN_UserAgent(mInstance);
  char*& version = *aVersion;

  
  
  
  
  nsIMemory * nsMemoryService = NULL;
  
  if (gServiceManager) {
    
    gServiceManager->GetServiceByContractID("@mozilla.org/xpcom/memory-service;1", NS_GET_IID(nsIMemory), (void **)&nsMemoryService);
    if(nsMemoryService)
      version = (char *)nsMemoryService->Alloc(strlen(ua) + 1);
  }

  if (version)
    strcpy(version, ua);

  
  NS_IF_RELEASE(nsMemoryService);
}










NPError	nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
  NPError rv = NPERR_NO_ERROR;

  switch (aVariable) {
    case NPPVpluginScriptableInstance: {
      
      nsISimplePlugin * scriptablePeer = getScriptablePeer();
      if (scriptablePeer) {
        *(nsISupports **)aValue = scriptablePeer;
      } else
        rv = NPERR_OUT_OF_MEMORY_ERROR;
    }
    break;

    case NPPVpluginScriptableIID: {
      static nsIID scriptableIID = NS_ISIMPLEPLUGIN_IID;
      nsIID* ptr = (nsIID *)NPN_MemAlloc(sizeof(nsIID));
      if (ptr) {
          *ptr = scriptableIID;
          *(nsIID **)aValue = ptr;
      } else
        rv = NPERR_OUT_OF_MEMORY_ERROR;
    }
    break;

    default:
      break;
  }

  return rv;
}






nsScriptablePeer* nsPluginInstance::getScriptablePeer()
{
  if (!mScriptablePeer) {
    mScriptablePeer = new nsScriptablePeer(this);
    if(!mScriptablePeer)
      return NULL;

    NS_ADDREF(mScriptablePeer);
  }

  
  NS_ADDREF(mScriptablePeer);
  return mScriptablePeer;
}
