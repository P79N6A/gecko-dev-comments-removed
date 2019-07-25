












































#include "nsDebug.h"
#include "nsPluginNativeWindow.h"
#include "npapi.h"




class nsPluginNativeWindowQt : public nsPluginNativeWindow
{
public:
  nsPluginNativeWindowQt();
  virtual ~nsPluginNativeWindowQt();

  virtual nsresult CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance);
private:

  NPSetWindowCallbackStruct mWsInfo;
};

nsPluginNativeWindowQt::nsPluginNativeWindowQt() : nsPluginNativeWindow()
{
  
#ifdef DEBUG
  fprintf(stderr,"\n\n\nCreating plugin native window %p\n\n\n", (void *) this);
#endif
  window = nsnull;
  x = 0;
  y = 0;
  width = 0;
  height = 0;
  memset(&clipRect, 0, sizeof(clipRect));
  ws_info = &mWsInfo;
  type = NPWindowTypeWindow;
  mWsInfo.type = 0;
  mWsInfo.display = nsnull;
  mWsInfo.visual = nsnull;
  mWsInfo.colormap = 0;
  mWsInfo.depth = 0;
}

nsPluginNativeWindowQt::~nsPluginNativeWindowQt()
{
#ifdef DEBUG
  fprintf(stderr,"\n\n\nDestoying plugin native window %p\n\n\n", (void *) this);
#endif
}

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow **aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  *aPluginNativeWindow = new nsPluginNativeWindowQt();
  return *aPluginNativeWindow ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  nsPluginNativeWindowQt *p = (nsPluginNativeWindowQt *)aPluginNativeWindow;
  delete p;
  return NS_OK;
}

nsresult nsPluginNativeWindowQt::CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance)
{
  if (aPluginInstance) {
    if (type == NPWindowTypeWindow) {
      return NS_ERROR_FAILURE;
    } 
    aPluginInstance->SetWindow(this);
  }
  else if (mPluginInstance)
    mPluginInstance->SetWindow(nsnull);

  SetPluginInstance(aPluginInstance);
  return NS_OK;
}
