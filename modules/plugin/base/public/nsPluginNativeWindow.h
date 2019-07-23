







































#ifndef _nsPluginNativeWindow_h_
#define _nsPluginNativeWindow_h_

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIPluginInstance.h"
#include "nsplugindefs.h"
#include "nsIWidget.h"




class nsPluginNativeWindow : public nsPluginWindow
{
public: 
  nsPluginNativeWindow() : nsPluginWindow() {}
  virtual ~nsPluginNativeWindow() {}

  









public:
  nsresult GetPluginInstance(nsCOMPtr<nsIPluginInstance> &aPluginInstance) { 
    aPluginInstance = mPluginInstance;
    return NS_OK;
  }
  nsresult SetPluginInstance(nsIPluginInstance *aPluginInstance) { 
    if (mPluginInstance != aPluginInstance)
      mPluginInstance = aPluginInstance;
    return NS_OK;
  }

  nsresult GetPluginWidget(nsIWidget **aWidget) {
    NS_IF_ADDREF(*aWidget = mWidget);
    return NS_OK;
  }
  nsresult SetPluginWidget(nsIWidget *aWidget) { 
    mWidget = aWidget;
    return NS_OK;
  }

public:
  virtual nsresult CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance) {
    
    if (aPluginInstance)
      aPluginInstance->SetWindow(this);
    else if (mPluginInstance)
      mPluginInstance->SetWindow(nsnull);

    SetPluginInstance(aPluginInstance);
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIPluginInstance> mPluginInstance;
  nsCOMPtr<nsIWidget>         mWidget;
};

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow);
nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow);

#endif 
