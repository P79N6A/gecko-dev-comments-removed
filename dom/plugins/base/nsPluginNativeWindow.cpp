













































#include "nsDebug.h"
#include "nsPluginNativeWindow.h"

class nsPluginNativeWindowPLATFORM : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowPLATFORM();
  virtual ~nsPluginNativeWindowPLATFORM();
};

nsPluginNativeWindowPLATFORM::nsPluginNativeWindowPLATFORM() : nsPluginNativeWindow()
{
  
  window = nsnull; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 
  memset(&clipRect, 0, sizeof(clipRect));
#if defined(XP_UNIX) && !defined(XP_MACOSX)
  ws_info = nsnull;
#endif
  type = NPWindowTypeWindow;
}

nsPluginNativeWindowPLATFORM::~nsPluginNativeWindowPLATFORM() 
{
}

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  *aPluginNativeWindow = new nsPluginNativeWindowPLATFORM();
  return *aPluginNativeWindow ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  nsPluginNativeWindowPLATFORM *p = (nsPluginNativeWindowPLATFORM *)aPluginNativeWindow;
  delete p;
  return NS_OK;
}
