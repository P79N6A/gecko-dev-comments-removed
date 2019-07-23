




































#ifndef __PLUGININSTANCEBASE_H__
#define __PLUGININSTANCEBASE_H__

#include "npplat.h"

struct nsPluginCreateData
{
  NPP instance;
  NPMIMEType type; 
  uint16 mode; 
  int16 argc; 
  char** argn; 
  char** argv; 
  NPSavedData* saved;
};

class nsPluginInstanceBase
{
public:
  
  
  virtual NPBool init(NPWindow* aWindow) = 0;
  virtual void shut() = 0;
  virtual NPBool isInitialized() = 0;

  
  
  virtual NPError SetWindow(NPWindow* pNPWindow)                    { return NPERR_NO_ERROR; }
  virtual NPError NewStream(NPMIMEType type, NPStream* stream, 
                            NPBool seekable, uint16* stype)         { return NPERR_NO_ERROR; }
  virtual NPError DestroyStream(NPStream *stream, NPError reason)   { return NPERR_NO_ERROR; }
  virtual void    StreamAsFile(NPStream* stream, const char* fname) { return; }
  virtual int32   WriteReady(NPStream *stream)                      { return 0x0fffffff; }
  virtual int32   Write(NPStream *stream, int32 offset, 
                        int32 len, void *buffer)                    { return len; }
  virtual void    Print(NPPrint* printInfo)                         { return; }
  virtual uint16  HandleEvent(void* event)                          { return 0; }
  virtual void    URLNotify(const char* url, NPReason reason, 
                            void* notifyData)                       { return; }
  virtual NPError GetValue(NPPVariable variable, void *value)       { return NPERR_NO_ERROR; }
  virtual NPError SetValue(NPNVariable variable, void *value)       { return NPERR_NO_ERROR; }
};




nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct);
void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin);


NPError NS_PluginInitialize();
void NS_PluginShutdown();

#ifdef XP_UNIX

NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue);
#endif

#endif 
