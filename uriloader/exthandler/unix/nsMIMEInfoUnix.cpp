





#ifdef MOZ_WIDGET_QT
#if (MOZ_ENABLE_CONTENTACTION)
#include <contentaction/contentaction.h>
#include "nsContentHandlerApp.h"
#endif
#endif

#include "nsMIMEInfoUnix.h"
#include "nsGNOMERegistry.h"
#include "nsIGIOService.h"
#include "nsNetCID.h"
#include "nsIIOService.h"
#include "nsIGnomeVFSService.h"
#include "nsAutoPtr.h"
#ifdef MOZ_ENABLE_DBUS
#include "nsDBusHandlerApp.h"
#endif
#ifdef MOZ_WIDGET_QT
#include "nsMIMEInfoQt.h"
#endif

nsresult
nsMIMEInfoUnix::LoadUriInternal(nsIURI * aURI)
{
  nsresult rv = nsGNOMERegistry::LoadURL(aURI);

#ifdef MOZ_WIDGET_QT
  if (NS_FAILED(rv)) {
    rv = nsMIMEInfoQt::LoadUriInternal(aURI);
  }
#endif

  return rv;
}

NS_IMETHODIMP
nsMIMEInfoUnix::GetHasDefaultHandler(bool *_retval)
{
  
  
  
  if (mDefaultApplication)
    return nsMIMEInfoImpl::GetHasDefaultHandler(_retval);

  *_retval = false;

  if (mClass ==  eProtocolInfo) {
    *_retval = nsGNOMERegistry::HandlerExists(mSchemeOrType.get());
  } else {
    nsRefPtr<nsMIMEInfoBase> mimeInfo = nsGNOMERegistry::GetFromType(mSchemeOrType);
    if (!mimeInfo) {
      nsAutoCString ext;
      nsresult rv = GetPrimaryExtension(ext);
      if (NS_SUCCEEDED(rv)) {
        mimeInfo = nsGNOMERegistry::GetFromExtension(ext);
      }
    }
    if (mimeInfo)
      *_retval = true;
  }

  if (*_retval)
    return NS_OK;

#if defined(MOZ_ENABLE_CONTENTACTION)
  ContentAction::Action action = 
    ContentAction::Action::defaultActionForFile(QUrl(), QString(mSchemeOrType.get()));
  if (action.isValid()) {
    *_retval = true;
    return NS_OK;
  }
#endif

  return NS_OK;
}

nsresult
nsMIMEInfoUnix::LaunchDefaultWithFile(nsIFile *aFile)
{
  
  
  
  if (mDefaultApplication)
    return nsMIMEInfoImpl::LaunchDefaultWithFile(aFile);

  nsAutoCString nativePath;
  aFile->GetNativePath(nativePath);

#if defined(MOZ_ENABLE_CONTENTACTION)
  QUrl uri = QUrl::fromLocalFile(QString::fromUtf8(nativePath.get()));
  ContentAction::Action action =
    ContentAction::Action::defaultActionForFile(uri, QString(mSchemeOrType.get()));
  if (action.isValid()) {
    action.trigger();
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
#endif

  nsCOMPtr<nsIGIOService> giovfs = do_GetService(NS_GIOSERVICE_CONTRACTID);
  nsAutoCString uriSpec;
  if (giovfs) {
    
    nsresult rv;
    nsCOMPtr<nsIIOService> ioservice = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> uri;
    rv = ioservice->NewFileURI(aFile, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    uri->GetSpec(uriSpec);
  }

  nsCOMPtr<nsIGnomeVFSService> gnomevfs = do_GetService(NS_GNOMEVFSSERVICE_CONTRACTID);
  if (giovfs) {
    nsCOMPtr<nsIGIOMimeApp> app;
    if (NS_SUCCEEDED(giovfs->GetAppForMimeType(mSchemeOrType, getter_AddRefs(app))) && app)
      return app->Launch(uriSpec);
  } else if (gnomevfs) {
    
    nsCOMPtr<nsIGnomeVFSMimeApp> app;
    if (NS_SUCCEEDED(gnomevfs->GetAppForMimeType(mSchemeOrType, getter_AddRefs(app))) && app)
      return app->Launch(nativePath);
  }

  
  
  nsRefPtr<nsMIMEInfoBase> mimeInfo = nsGNOMERegistry::GetFromExtension(nativePath);
  if (mimeInfo) {
    nsAutoCString type;
    mimeInfo->GetType(type);
    if (giovfs) {
      nsCOMPtr<nsIGIOMimeApp> app;
      if (NS_SUCCEEDED(giovfs->GetAppForMimeType(type, getter_AddRefs(app))) && app)
        return app->Launch(uriSpec);
    } else if (gnomevfs) {
      nsCOMPtr<nsIGnomeVFSMimeApp> app;
      if (NS_SUCCEEDED(gnomevfs->GetAppForMimeType(type, getter_AddRefs(app))) && app)
        return app->Launch(nativePath);
    }
  }

  if (!mDefaultApplication)
    return NS_ERROR_FILE_NOT_FOUND;

  return LaunchWithIProcess(mDefaultApplication, nativePath);
}

#if defined(MOZ_ENABLE_CONTENTACTION)
NS_IMETHODIMP
nsMIMEInfoUnix::GetPossibleApplicationHandlers(nsIMutableArray ** aPossibleAppHandlers)
{
  if (!mPossibleApplications) {
    mPossibleApplications = do_CreateInstance(NS_ARRAY_CONTRACTID);

    if (!mPossibleApplications)
      return NS_ERROR_OUT_OF_MEMORY;

    QList<ContentAction::Action> actions =
      ContentAction::Action::actionsForFile(QUrl(), QString(mSchemeOrType.get()));

    for (int i = 0; i < actions.size(); ++i) {
      nsContentHandlerApp* app =
        new nsContentHandlerApp(nsString((char16_t*)actions[i].name().data()), 
                                mSchemeOrType, actions[i]);
      mPossibleApplications->AppendElement(app, false);
    }
  }

  *aPossibleAppHandlers = mPossibleApplications;
  NS_ADDREF(*aPossibleAppHandlers);
  return NS_OK;
}
#endif

