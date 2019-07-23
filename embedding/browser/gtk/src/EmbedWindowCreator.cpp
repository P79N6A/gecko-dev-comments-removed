




































#include "EmbedWindowCreator.h"
#include "EmbedPrivate.h"
#include "EmbedWindow.h"


#include "gtkmozembedprivate.h"

EmbedWindowCreator::EmbedWindowCreator(void)
{
}

EmbedWindowCreator::~EmbedWindowCreator()
{
}

NS_IMPL_ISUPPORTS1(EmbedWindowCreator, nsIWindowCreator)

NS_IMETHODIMP
EmbedWindowCreator::CreateChromeWindow(nsIWebBrowserChrome *aParent,
				       PRUint32 aChromeFlags,
				       nsIWebBrowserChrome **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  GtkMozEmbed *newEmbed = nsnull;

  
  if (!aParent) {
    gtk_moz_embed_single_create_window(&newEmbed,
				       (guint)aChromeFlags);
  }
  else {
    
    EmbedPrivate *embedPrivate = EmbedPrivate::FindPrivateForBrowser(aParent);
    
    if (!embedPrivate)
      return NS_ERROR_FAILURE;
    
    g_signal_emit(G_OBJECT(embedPrivate->mOwningWidget),
                  moz_embed_signals[NEW_WINDOW], 0,
                  &newEmbed, (guint)aChromeFlags);
    
  }

  
  if (!newEmbed)
    return NS_ERROR_FAILURE;

  
  
  
  
  gtk_widget_realize(GTK_WIDGET(newEmbed));
  
  EmbedPrivate *newEmbedPrivate = static_cast<EmbedPrivate *>(newEmbed->data);

  
  if (aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
    newEmbedPrivate->mIsChrome = PR_TRUE;

  *_retval = static_cast<nsIWebBrowserChrome *>(newEmbedPrivate->mWindow);
  
  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}
