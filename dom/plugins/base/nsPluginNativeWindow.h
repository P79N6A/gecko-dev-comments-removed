




#ifndef _nsPluginNativeWindow_h_
#define _nsPluginNativeWindow_h_

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsNPAPIPluginInstance.h"
#include "npapi.h"
#include "nsIWidget.h"




class nsPluginNativeWindow : public NPWindow
{
public: 
  nsPluginNativeWindow() : NPWindow() {
    MOZ_COUNT_CTOR(nsPluginNativeWindow);
  }

  virtual ~nsPluginNativeWindow() {
    MOZ_COUNT_DTOR(nsPluginNativeWindow);
  }

  









public:
  nsresult GetPluginInstance(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance) { 
    aPluginInstance = mPluginInstance;
    return NS_OK;
  }
  nsresult SetPluginInstance(nsNPAPIPluginInstance *aPluginInstance) { 
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
  virtual nsresult CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance) {
    
    if (aPluginInstance)
      aPluginInstance->SetWindow(this);
    else if (mPluginInstance)
      mPluginInstance->SetWindow(nullptr);

    SetPluginInstance(aPluginInstance);
    return NS_OK;
  }

protected:
  nsRefPtr<nsNPAPIPluginInstance> mPluginInstance;
  nsCOMPtr<nsIWidget> mWidget;
};

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow);
nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow);

#endif 
