







































#include "EmbedWindowCreator.h"
#include "EmbedPrivate.h"
#include "EmbedWindow.h"


#include "gtkmozembedprivate.h"

EmbedWindowCreator::EmbedWindowCreator(PRBool *aOpenBlockPtr)
{
  mOpenBlock = aOpenBlockPtr;
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

  if (mOpenBlock) {
    if (*mOpenBlock == PR_TRUE) {
      *mOpenBlock = PR_FALSE;
      return NS_ERROR_FAILURE;
    }
  }
  GtkMozEmbed *newEmbed = nsnull;

  
  if (!aParent) {
    gtk_moz_embed_single_create_window(&newEmbed,
               (guint)aChromeFlags);
  } else {
    
    EmbedPrivate *embedPrivate = EmbedPrivate::FindPrivateForBrowser(aParent);

    if (!embedPrivate)
      return NS_ERROR_FAILURE;

    gtk_signal_emit(GTK_OBJECT(embedPrivate->mOwningWidget),
        moz_embed_signals[NEW_WINDOW],
        &newEmbed, (guint)aChromeFlags);

  }

  
  if (!newEmbed)
    return NS_ERROR_FAILURE;

  
  
  
  
  gtk_widget_realize(GTK_WIDGET(newEmbed));

  EmbedPrivate *newEmbedPrivate = NS_STATIC_CAST(EmbedPrivate *,
             newEmbed->data);

  
  if (aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
    newEmbedPrivate->mIsChrome = PR_TRUE;

  *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *,
            (newEmbedPrivate->mWindow));

  if (*_retval) {
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}
