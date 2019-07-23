











































#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIChannel.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsIStringEnumerator.h"
#include "nsMemory.h"
#include "nsIStreamListener.h"
#include "nsIMIMEService.h"
#include "nsILoadGroup.h"
#include "nsIWebProgressListener.h"
#include "nsITransfer.h"
#include "nsReadableUtils.h"
#include "nsIRequest.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIInterfaceRequestor.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsIMutableArray.h"


#include "nsIHandlerService.h"
#include "nsIMIMEInfo.h"
#include "nsIRefreshURI.h" 
#include "nsIDocumentLoader.h" 
#include "nsIHelperAppLauncherDialog.h"
#include "nsIContentDispatchChooser.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsChannelProperties.h"

#include "nsMimeTypes.h"

#include "nsIHttpChannel.h"
#include "nsIEncodedChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIFileChannel.h"
#include "nsIObserverService.h" 
#include "nsIPropertyBag2.h" 

#ifdef XP_MACOSX
#include "nsILocalFileMac.h"
#ifndef __LP64__
#include "nsIAppleFileDecoder.h"
#endif
#elif defined(XP_OS2)
#include "nsILocalFileOS2.h"
#endif

#include "nsIPluginHost.h" 
#include "nsEscape.h"

#include "nsIStringBundle.h" 
#include "nsIPrompt.h"

#include "nsITextToSubURI.h" 
#include "nsIMIMEHeaderParam.h"

#include "nsIPrefService.h"
#include "nsIWindowWatcher.h"

#include "nsIDownloadHistory.h" 
#include "nsDocShellCID.h"

#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocShell.h"

#include "nsCRT.h"

#include "nsLocalHandlerApp.h"

#include "nsIRandomGenerator.h"
#include "plbase64.h"
#include "prmem.h"

#include "nsIPrivateBrowsingService.h"


#define BUFFERED_OUTPUT_SIZE (1024 * 32)


#define NS_PREF_DOWNLOAD_DIR        "browser.download.dir"
#define NS_PREF_DOWNLOAD_FOLDERLIST "browser.download.folderList"
enum {
  NS_FOLDER_VALUE_DESKTOP = 0
, NS_FOLDER_VALUE_DOWNLOADS = 1
, NS_FOLDER_VALUE_CUSTOM = 2
};

#ifdef PR_LOGGING
PRLogModuleInfo* nsExternalHelperAppService::mLog = nsnull;
#endif




#define LOG(args) PR_LOG(mLog, 3, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, 3)

static const char NEVER_ASK_PREF_BRANCH[] = "browser.helperApps.neverAsk.";
static const char NEVER_ASK_FOR_SAVE_TO_DISK_PREF[] = "saveToDisk";
static const char NEVER_ASK_FOR_OPEN_FILE_PREF[]    = "openFile";




nsExternalHelperAppService* gExtProtSvc;










static nsresult UnescapeFragment(const nsACString& aFragment, nsIURI* aURI,
                                 nsAString& aResult)
{
  
  nsCAutoString originCharset;
  nsresult rv = aURI->GetOriginCharset(originCharset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITextToSubURI> textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return textToSubURI->UnEscapeURIForUI(originCharset, aFragment, aResult);
}










static nsresult UnescapeFragment(const nsACString& aFragment, nsIURI* aURI,
                                 nsACString& aResult)
{
  nsAutoString result;
  nsresult rv = UnescapeFragment(aFragment, aURI, result);
  if (NS_SUCCEEDED(rv))
    CopyUTF16toUTF8(result, aResult);
  return rv;
}






static void ExtractDisposition(nsIChannel* aChannel, nsACString& aDisposition)
{
  aDisposition.Truncate();
  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
  if (httpChannel) 
  {
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-disposition"), aDisposition);
  }
  if (aDisposition.IsEmpty())
  {
    nsCOMPtr<nsIMultiPartChannel> multipartChannel(do_QueryInterface(aChannel));
    if (multipartChannel)
    {
      multipartChannel->GetContentDisposition(aDisposition);
    }
  }

}









static void GetFilenameFromDisposition(nsAString& aFilename,
                                       const nsACString& aDisposition,
                                       nsIURI* aURI = nsnull,
                                       nsIMIMEHeaderParam* aMIMEHeaderParam = nsnull)
{
  aFilename.Truncate();
  nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar(aMIMEHeaderParam);
  if (!mimehdrpar) {
    mimehdrpar = do_GetService(NS_MIMEHEADERPARAM_CONTRACTID);
    if (!mimehdrpar)
      return;
  }

  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);

  nsCAutoString fallbackCharset;
  if (url)
    url->GetOriginCharset(fallbackCharset);
  
  nsresult rv = mimehdrpar->GetParameter(aDisposition, "filename", fallbackCharset, 
                                         PR_TRUE, nsnull, aFilename);
  if (NS_FAILED(rv) || aFilename.IsEmpty())
    
    rv = mimehdrpar->GetParameter(aDisposition, "name", fallbackCharset, PR_TRUE, 
                                  nsnull, aFilename);
}



















static PRBool GetFilenameAndExtensionFromChannel(nsIChannel* aChannel,
                                                 nsString& aFileName,
                                                 nsCString& aExtension,
                                                 PRBool aAllowURLExtension = PR_TRUE)
{
  aExtension.Truncate();
  






  nsCAutoString disp;
  ExtractDisposition(aChannel, disp);
  PRBool handleExternally = PR_FALSE;
  nsCOMPtr<nsIURI> uri;
  nsresult rv;
  aChannel->GetURI(getter_AddRefs(uri));
  
  
  if (!disp.IsEmpty()) 
  {
    nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar = do_GetService(NS_MIMEHEADERPARAM_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return PR_FALSE;

    nsCAutoString fallbackCharset;
    uri->GetOriginCharset(fallbackCharset);
    
    nsAutoString dispToken;
    rv = mimehdrpar->GetParameter(disp, "", fallbackCharset, PR_TRUE, 
                                  nsnull, dispToken);
    
    
    
    
    if (NS_FAILED(rv) || 
        (!dispToken.IsEmpty() &&
         !dispToken.LowerCaseEqualsLiteral("inline") &&
         
         
         
         !dispToken.EqualsIgnoreCase("filename", 8) &&
         
         !dispToken.EqualsIgnoreCase("name", 4)))
    {
      
      handleExternally = PR_TRUE;
    }

    
    
    GetFilenameFromDisposition(aFileName, disp, uri, mimehdrpar);

  } 

  
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url && aFileName.IsEmpty())
  {
    if (aAllowURLExtension) {
      url->GetFileExtension(aExtension);
      UnescapeFragment(aExtension, url, aExtension);

      
      
      
      
      aExtension.Trim(".", PR_FALSE);
    }

    
    
    nsCAutoString leafName;
    url->GetFileName(leafName);
    if (!leafName.IsEmpty())
    {
      rv = UnescapeFragment(leafName, url, aFileName);
      if (NS_FAILED(rv))
      {
        CopyUTF8toUTF16(leafName, aFileName); 
      }
    }
  }

  
  
  if (aExtension.IsEmpty()) {
    if (!aFileName.IsEmpty())
    {
      
      
      aFileName.Trim(".", PR_FALSE);

      
      nsAutoString fileNameStr(aFileName);
      PRInt32 idx = fileNameStr.RFindChar(PRUnichar('.'));
      if (idx != kNotFound)
        CopyUTF16toUTF8(StringTail(fileNameStr, fileNameStr.Length() - idx - 1), aExtension);
    }
  }


  return handleExternally;
}





static nsresult GetDownloadDirectory(nsIFile **_directory)
{
  nsCOMPtr<nsIFile> dir;
#ifdef XP_MACOSX
  
  nsCOMPtr<nsIPrefBranch> prefs =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(prefs, NS_ERROR_UNEXPECTED);

  PRInt32 folderValue = -1;
  (void) prefs->GetIntPref(NS_PREF_DOWNLOAD_FOLDERLIST, &folderValue);
  switch (folderValue) {
    case NS_FOLDER_VALUE_DESKTOP:
      (void) NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(dir));
      break;
    case NS_FOLDER_VALUE_CUSTOM:
      {
        (void) prefs->GetComplexValue(NS_PREF_DOWNLOAD_DIR,
                                      NS_GET_IID(nsILocalFile),
                                      getter_AddRefs(dir));
        if (!dir) break;

        
        PRBool dirExists = PR_FALSE;
        (void) dir->Exists(&dirExists);
        if (dirExists) break;

        nsresult rv = dir->Create(nsIFile::DIRECTORY_TYPE, 0755);
        if (NS_FAILED(rv)) {
          dir = nsnull;
          break;
        }
      }
      break;
    case NS_FOLDER_VALUE_DOWNLOADS:
      
      break;
  }

  if (!dir) {
    
    nsresult rv = NS_GetSpecialDirectory(NS_OSX_DEFAULT_DOWNLOAD_DIR,
                                         getter_AddRefs(dir));
    NS_ENSURE_SUCCESS(rv, rv);
  }
#else
  
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dir));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  NS_ASSERTION(dir, "Somehow we didn't get a download directory!");
  dir.forget(_directory);
  return NS_OK;
}





struct nsDefaultMimeTypeEntry {
  const char* mMimeType;
  const char* mFileExtension;
};





static nsDefaultMimeTypeEntry defaultMimeEntries [] = 
{
  
  
  { IMAGE_GIF, "gif" },
  { TEXT_XML, "xml" },
  { APPLICATION_RDF, "rdf" },
  { TEXT_XUL, "xul" },
  { IMAGE_PNG, "png" },
  
  { TEXT_CSS, "css" },
  { IMAGE_JPG, "jpeg" },
  { IMAGE_JPG, "jpg" },
  { TEXT_HTML, "html" },
  { TEXT_HTML, "htm" },
  { APPLICATION_XPINSTALL, "xpi" },
  { "application/xhtml+xml", "xhtml" },
  { "application/xhtml+xml", "xht" },
  { TEXT_PLAIN, "txt" }
};





struct nsExtraMimeTypeEntry {
  const char* mMimeType; 
  const char* mFileExtensions;
  const char* mDescription;
};

#ifdef XP_MACOSX
#define MAC_TYPE(x) x
#else
#define MAC_TYPE(x) 0
#endif








static nsExtraMimeTypeEntry extraMimeEntries [] =
{
#if defined(VMS)
  { APPLICATION_OCTET_STREAM, "exe,com,bin,sav,bck,pcsi,dcx_axpexe,dcx_vaxexe,sfx_axpexe,sfx_vaxexe", "Binary File" },
#elif defined(XP_MACOSX) 
  { APPLICATION_OCTET_STREAM, "exe,com", "Binary File" },
#else
  { APPLICATION_OCTET_STREAM, "exe,com,bin", "Binary File" },
#endif
  { APPLICATION_GZIP2, "gz", "gzip" },
  { "application/x-arj", "arj", "ARJ file" },
  { APPLICATION_XPINSTALL, "xpi", "XPInstall Install" },
  { APPLICATION_POSTSCRIPT, "ps,eps,ai", "Postscript File" },
  { APPLICATION_JAVASCRIPT, "js", "Javascript Source File" },
  { IMAGE_ART, "art", "ART Image" },
  { IMAGE_BMP, "bmp", "BMP Image" },
  { IMAGE_GIF, "gif", "GIF Image" },
  { IMAGE_ICO, "ico,cur", "ICO Image" },
  { IMAGE_JPG, "jpeg,jpg,jfif,pjpeg,pjp", "JPEG Image" },
  { IMAGE_PNG, "png", "PNG Image" },
  { IMAGE_TIFF, "tiff,tif", "TIFF Image" },
  { IMAGE_XBM, "xbm", "XBM Image" },
  { "image/svg+xml", "svg", "Scalable Vector Graphics" },
  { MESSAGE_RFC822, "eml", "RFC-822 data" },
  { TEXT_PLAIN, "txt,text", "Text File" },
  { TEXT_HTML, "html,htm,shtml,ehtml", "HyperText Markup Language" },
  { "application/xhtml+xml", "xhtml,xht", "Extensible HyperText Markup Language" },
  { APPLICATION_RDF, "rdf", "Resource Description Framework" },
  { TEXT_XUL, "xul", "XML-Based User Interface Language" },
  { TEXT_XML, "xml,xsl,xbl", "Extensible Markup Language" },
  { TEXT_CSS, "css", "Style Sheet" },
  { VIDEO_OGG, "ogv", "Ogg Video" },
  { VIDEO_OGG, "ogg", "Ogg Video" },
  { APPLICATION_OGG, "ogg", "Ogg Video"},
  { AUDIO_OGG, "oga", "Ogg Audio" },
  { AUDIO_WAV, "wav", "Waveform Audio" }
};

#undef MAC_TYPE





static nsDefaultMimeTypeEntry nonDecodableExtensions [] = {
  { APPLICATION_GZIP, "gz" }, 
  { APPLICATION_GZIP, "tgz" },
  { APPLICATION_ZIP, "zip" },
  { APPLICATION_COMPRESS, "z" },
  { APPLICATION_GZIP, "svgz" }
};

NS_IMPL_ISUPPORTS6(
  nsExternalHelperAppService,
  nsIExternalHelperAppService,
  nsPIExternalAppLauncher,
  nsIExternalProtocolService,
  nsIMIMEService,
  nsIObserver,
  nsISupportsWeakReference)

nsExternalHelperAppService::nsExternalHelperAppService() :
  mInPrivateBrowsing(PR_FALSE)
{
  gExtProtSvc = this;
}
nsresult nsExternalHelperAppService::Init()
{
  nsCOMPtr<nsIPrivateBrowsingService> pbs =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbs) {
    pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
  }

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
  if (!mLog) {
    mLog = PR_NewLogModule("HelperAppService");
    if (!mLog)
      return NS_ERROR_OUT_OF_MEMORY;
  }
#endif

  rv = obs->AddObserver(this, "profile-before-change", PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  return obs->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);
}

nsExternalHelperAppService::~nsExternalHelperAppService()
{
  gExtProtSvc = nsnull;
}

NS_IMETHODIMP nsExternalHelperAppService::DoContent(const nsACString& aMimeContentType,
                                                    nsIRequest *aRequest,
                                                    nsIInterfaceRequestor *aWindowContext,
                                                    PRBool aForceSave,
                                                    nsIStreamListener ** aStreamListener)
{
  nsAutoString fileName;
  nsCAutoString fileExtension;
  PRUint32 reason = nsIHelperAppLauncherDialog::REASON_CANTHANDLE;
  nsresult rv;

  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (channel) {
    
    
    PRBool allowURLExt = PR_TRUE;
    nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(channel);
    if (httpChan) {
      nsCAutoString requestMethod;
      httpChan->GetRequestMethod(requestMethod);
      allowURLExt = !requestMethod.Equals("POST");
    }

    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));

    
    
    
    
    if (uri && allowURLExt) {
      nsCOMPtr<nsIURL> url = do_QueryInterface(uri);

      if (url) {
        nsCAutoString query;

        
        PRBool isHTTP, isHTTPS;
        rv = uri->SchemeIs("http", &isHTTP);
        if (NS_FAILED(rv))
          isHTTP = PR_FALSE;
        rv = uri->SchemeIs("https", &isHTTPS);
        if (NS_FAILED(rv))
          isHTTPS = PR_FALSE;

        if (isHTTP || isHTTPS)
          url->GetQuery(query);

        
        
        allowURLExt = query.IsEmpty();
      }
    }
    
    PRBool isAttachment = GetFilenameAndExtensionFromChannel(channel, fileName,
                                                             fileExtension,
                                                             allowURLExt);
    LOG(("Found extension '%s' (filename is '%s', handling attachment: %i)",
         fileExtension.get(), NS_ConvertUTF16toUTF8(fileName).get(),
         isAttachment));
    if (isAttachment)
      reason = nsIHelperAppLauncherDialog::REASON_SERVERREQUEST;
  }

  LOG(("HelperAppService::DoContent: mime '%s', extension '%s'\n",
       PromiseFlatCString(aMimeContentType).get(), fileExtension.get()));

  
  
  
  nsCOMPtr<nsIMIMEService> mimeSvc(do_GetService(NS_MIMESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(mimeSvc, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIMIMEInfo> mimeInfo;
  if (aMimeContentType.Equals(APPLICATION_GUESS_FROM_EXT, nsCaseInsensitiveCStringComparator())) {
    nsCAutoString mimeType;
    if (!fileExtension.IsEmpty()) {
      mimeSvc->GetFromTypeAndExtension(EmptyCString(), fileExtension, getter_AddRefs(mimeInfo));
      if (mimeInfo) {
        mimeInfo->GetMIMEType(mimeType);

        LOG(("OS-Provided mime type '%s' for extension '%s'\n", 
             mimeType.get(), fileExtension.get()));
      }
    }

    if (fileExtension.IsEmpty() || mimeType.IsEmpty()) {
      
      mimeSvc->GetFromTypeAndExtension(NS_LITERAL_CSTRING(APPLICATION_OCTET_STREAM), fileExtension,
                                       getter_AddRefs(mimeInfo));
      mimeType.AssignLiteral(APPLICATION_OCTET_STREAM);
    }
    if (channel)
      channel->SetContentType(mimeType);
    
    if (reason == nsIHelperAppLauncherDialog::REASON_CANTHANDLE)
      reason = nsIHelperAppLauncherDialog::REASON_TYPESNIFFED;
  } 
  else {
    mimeSvc->GetFromTypeAndExtension(aMimeContentType, fileExtension,
                                     getter_AddRefs(mimeInfo));
  } 
  LOG(("Type/Ext lookup found 0x%p\n", mimeInfo.get()));

  
  if (!mimeInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  *aStreamListener = nsnull;
  
  
  nsCAutoString buf;
  mimeInfo->GetPrimaryExtension(buf);

  nsExternalAppHandler * handler = new nsExternalAppHandler(mimeInfo,
                                                            buf,
                                                            aWindowContext,
                                                            fileName,
                                                            reason,
                                                            aForceSave);
  if (!handler)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aStreamListener = handler);
  
  return NS_OK;
}

NS_IMETHODIMP nsExternalHelperAppService::ApplyDecodingForExtension(const nsACString& aExtension,
                                                                    const nsACString& aEncodingType,
                                                                    PRBool *aApplyDecoding)
{
  *aApplyDecoding = PR_TRUE;
  PRUint32 i;
  for(i = 0; i < NS_ARRAY_LENGTH(nonDecodableExtensions); ++i) {
    if (aExtension.LowerCaseEqualsASCII(nonDecodableExtensions[i].mFileExtension) &&
        aEncodingType.LowerCaseEqualsASCII(nonDecodableExtensions[i].mMimeType)) {
      *aApplyDecoding = PR_FALSE;
      break;
    }
  }
  return NS_OK;
}

nsresult nsExternalHelperAppService::GetFileTokenForPath(const PRUnichar * aPlatformAppPath,
                                                         nsIFile ** aFile)
{
  nsDependentString platformAppPath(aPlatformAppPath);
  
  nsILocalFile* localFile = nsnull;
  nsresult rv = NS_NewLocalFile(platformAppPath, PR_TRUE, &localFile);
  if (NS_SUCCEEDED(rv)) {
    *aFile = localFile;
    PRBool exists;
    if (NS_FAILED((*aFile)->Exists(&exists)) || !exists) {
      NS_RELEASE(*aFile);
      return NS_ERROR_FILE_NOT_FOUND;
    }
    return NS_OK;
  }


  
  rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, aFile);
  if (NS_SUCCEEDED(rv)) {
    rv = (*aFile)->Append(platformAppPath);
    if (NS_SUCCEEDED(rv)) {
      PRBool exists = PR_FALSE;
      rv = (*aFile)->Exists(&exists);
      if (NS_SUCCEEDED(rv) && exists)
        return NS_OK;
    }
    NS_RELEASE(*aFile);
  }


  return NS_ERROR_NOT_AVAILABLE;
}




NS_IMETHODIMP nsExternalHelperAppService::ExternalProtocolHandlerExists(const char * aProtocolScheme,
                                                                        PRBool * aHandlerExists)
{
  nsCOMPtr<nsIHandlerInfo> handlerInfo;
  nsresult rv = GetProtocolHandlerInfo(nsDependentCString(aProtocolScheme), 
                                       getter_AddRefs(handlerInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIMutableArray> possibleHandlers;
  handlerInfo->GetPossibleApplicationHandlers(getter_AddRefs(possibleHandlers));

  PRUint32 length;
  possibleHandlers->GetLength(&length);
  if (length) {
    *aHandlerExists = PR_TRUE;
    return NS_OK;
  }

  
  return OSProtocolHandlerExists(aProtocolScheme, aHandlerExists);
}

NS_IMETHODIMP nsExternalHelperAppService::IsExposedProtocol(const char * aProtocolScheme, PRBool * aResult)
{
  
  
  
  *aResult = PR_FALSE;

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs)
  {
    PRBool val;
    nsresult rv;

    
    

    nsCAutoString name;
    name = NS_LITERAL_CSTRING("network.protocol-handler.expose.")
         + nsDependentCString(aProtocolScheme);
    rv = prefs->GetBoolPref(name.get(), &val);
    if (NS_SUCCEEDED(rv))
    {
      *aResult = val;
    }
    else
    {
      rv = prefs->GetBoolPref("network.protocol-handler.expose-all", &val);
      if (NS_SUCCEEDED(rv) && val)
        *aResult = PR_TRUE;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsExternalHelperAppService::LoadUrl(nsIURI * aURL)
{
  return LoadURI(aURL, nsnull);
}

static const char kExternalProtocolPrefPrefix[]  = "network.protocol-handler.external.";
static const char kExternalProtocolDefaultPref[] = "network.protocol-handler.external-default";

NS_IMETHODIMP 
nsExternalHelperAppService::LoadURI(nsIURI *aURI,
                                    nsIInterfaceRequestor *aWindowContext)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCAutoString spec;
  aURI->GetSpec(spec);

  if (spec.Find("%00") != -1)
    return NS_ERROR_MALFORMED_URI;

  spec.ReplaceSubstring("\"", "%22");
  spec.ReplaceSubstring("`", "%60");
  
  nsCOMPtr<nsIIOService> ios(do_GetIOService());
  nsCOMPtr<nsIURI> uri;
  nsresult rv = ios->NewURI(spec, nsnull, nsnull, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString scheme;
  uri->GetScheme(scheme);
  if (scheme.IsEmpty())
    return NS_OK; 

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs)
    return NS_OK; 

  
  nsCAutoString externalPref(kExternalProtocolPrefPrefix);
  externalPref += scheme;
  PRBool allowLoad  = PR_FALSE;
  rv = prefs->GetBoolPref(externalPref.get(), &allowLoad);
  if (NS_FAILED(rv))
  {
    
    rv = prefs->GetBoolPref(kExternalProtocolDefaultPref, &allowLoad);
  }
  if (NS_FAILED(rv) || !allowLoad)
    return NS_OK; 

 
  nsCOMPtr<nsIHandlerInfo> handler;
  rv = GetProtocolHandlerInfo(scheme, getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsHandlerInfoAction preferredAction;
  handler->GetPreferredAction(&preferredAction);
  PRBool alwaysAsk = PR_TRUE;
  handler->GetAlwaysAskBeforeHandling(&alwaysAsk);

  
  
  if (!alwaysAsk && (preferredAction == nsIHandlerInfo::useHelperApp ||
                     preferredAction == nsIHandlerInfo::useSystemDefault))
    return handler->LaunchWithURI(uri, aWindowContext);
  
  nsCOMPtr<nsIContentDispatchChooser> chooser =
    do_CreateInstance("@mozilla.org/content-dispatch-chooser;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return chooser->Ask(handler, aWindowContext, uri,
                      nsIContentDispatchChooser::REASON_CANNOT_HANDLE);
}

NS_IMETHODIMP nsExternalHelperAppService::GetApplicationDescription(const nsACString& aScheme, nsAString& _retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsExternalHelperAppService::DeleteTemporaryFileOnExit(nsIFile * aTemporaryFile)
{
  nsresult rv = NS_OK;
  PRBool isFile = PR_FALSE;
  nsCOMPtr<nsILocalFile> localFile (do_QueryInterface(aTemporaryFile, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  localFile->IsFile(&isFile);
  if (!isFile) return NS_OK;

  if (mInPrivateBrowsing)
    mTemporaryPrivateFilesList.AppendObject(localFile);
  else
    mTemporaryFilesList.AppendObject(localFile);

  return NS_OK;
}

void nsExternalHelperAppService::FixFilePermissions(nsILocalFile* aFile)
{
  
}

void nsExternalHelperAppService::ExpungeTemporaryFilesHelper(nsCOMArray<nsILocalFile> &fileList)
{
  PRInt32 numEntries = fileList.Count();
  nsILocalFile* localFile;
  for (PRInt32 index = 0; index < numEntries; index++)
  {
    localFile = fileList[index];
    if (localFile) {
      
      localFile->SetPermissions(0600);
      localFile->Remove(PR_FALSE);
    }
  }

  fileList.Clear();
}

void nsExternalHelperAppService::ExpungeTemporaryFiles()
{
  ExpungeTemporaryFilesHelper(mTemporaryFilesList);
}

void nsExternalHelperAppService::ExpungeTemporaryPrivateFiles()
{
  ExpungeTemporaryFilesHelper(mTemporaryPrivateFilesList);
}

static const char kExternalWarningPrefPrefix[] = 
  "network.protocol-handler.warn-external.";
static const char kExternalWarningDefaultPref[] = 
  "network.protocol-handler.warn-external-default";

NS_IMETHODIMP
nsExternalHelperAppService::GetProtocolHandlerInfo(const nsACString &aScheme,
                                                   nsIHandlerInfo **aHandlerInfo)
{
  
  

  PRBool exists;
  nsresult rv = GetProtocolHandlerInfoFromOS(aScheme, &exists, aHandlerInfo);
  if (NS_FAILED(rv)) {
    
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
  if (handlerSvc) {
    PRBool hasHandler = PR_FALSE;
    (void) handlerSvc->Exists(*aHandlerInfo, &hasHandler);
    if (hasHandler) {
      rv = handlerSvc->FillHandlerInfo(*aHandlerInfo, EmptyCString());
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }
  
  return SetProtocolHandlerDefaults(*aHandlerInfo, exists);
}

NS_IMETHODIMP
nsExternalHelperAppService::GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                                         PRBool *found,
                                                         nsIHandlerInfo **aHandlerInfo)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsExternalHelperAppService::SetProtocolHandlerDefaults(nsIHandlerInfo *aHandlerInfo,
                                                       PRBool aOSHandlerExists)
{
  
  

  if (aOSHandlerExists) {
    
    aHandlerInfo->SetPreferredAction(nsIHandlerInfo::useSystemDefault);

    
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefs)
      return NS_OK; 

    nsCAutoString scheme;
    aHandlerInfo->GetType(scheme);
    
    nsCAutoString warningPref(kExternalWarningPrefPrefix);
    warningPref += scheme;
    PRBool warn = PR_TRUE;
    nsresult rv = prefs->GetBoolPref(warningPref.get(), &warn);
    if (NS_FAILED(rv)) {
      
      prefs->GetBoolPref(kExternalWarningDefaultPref, &warn);
    }
    aHandlerInfo->SetAlwaysAskBeforeHandling(warn);
  } else {
    
    
    
    aHandlerInfo->SetPreferredAction(nsIHandlerInfo::alwaysAsk);
  }

  return NS_OK;
}
 

NS_IMETHODIMP
nsExternalHelperAppService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData )
{
  if (!strcmp(aTopic, "profile-before-change")) {
    ExpungeTemporaryFiles();
  } else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(someData))
      mInPrivateBrowsing = PR_TRUE;
    else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(someData)) {
      mInPrivateBrowsing = PR_FALSE;
      ExpungeTemporaryPrivateFiles();
    }
  }
  return NS_OK;
}





NS_IMPL_THREADSAFE_ADDREF(nsExternalAppHandler)
NS_IMPL_THREADSAFE_RELEASE(nsExternalAppHandler)

NS_INTERFACE_MAP_BEGIN(nsExternalAppHandler)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamListener)
   NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
   NS_INTERFACE_MAP_ENTRY(nsIHelperAppLauncher)   
   NS_INTERFACE_MAP_ENTRY(nsICancelable)
   NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_THREADSAFE

nsExternalAppHandler::nsExternalAppHandler(nsIMIMEInfo * aMIMEInfo,
                                           const nsCSubstring& aTempFileExtension,
                                           nsIInterfaceRequestor* aWindowContext,
                                           const nsAString& aSuggestedFilename,
                                           PRUint32 aReason, PRBool aForceSave)
: mMimeInfo(aMIMEInfo)
, mWindowContext(aWindowContext)
, mWindowToClose(nsnull)
, mSuggestedFileName(aSuggestedFilename)
, mForceSave(aForceSave)
, mCanceled(PR_FALSE)
, mShouldCloseWindow(PR_FALSE)
, mReceivedDispositionInfo(PR_FALSE)
, mStopRequestIssued(PR_FALSE)
, mProgressListenerInitialized(PR_FALSE)
, mReason(aReason)
, mContentLength(-1)
, mProgress(0)
, mRequest(nsnull)
{

  
  if (!aTempFileExtension.IsEmpty() && aTempFileExtension.First() != '.')
    mTempFileExtension = PRUnichar('.');
  AppendUTF8toUTF16(aTempFileExtension, mTempFileExtension);

  
  mSuggestedFileName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');
  mTempFileExtension.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');
  
  
  EnsureSuggestedFileName();

  gExtProtSvc->AddRef();
}

nsExternalAppHandler::~nsExternalAppHandler()
{
  
  gExtProtSvc->Release();
}

NS_IMETHODIMP nsExternalAppHandler::SetWebProgressListener(nsIWebProgressListener2 * aWebProgressListener)
{ 
  
  
  
  
  if (mReceivedDispositionInfo)
    mProgressListenerInitialized = PR_TRUE;

  
  mWebProgressListener = aWebProgressListener;

  
  
  
  if (mStopRequestIssued && aWebProgressListener)
  {
    return ExecuteDesiredAction();
  }

  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetTargetFile(nsIFile** aTarget)
{
  if (mFinalFileDestination)
    *aTarget = mFinalFileDestination;
  else
    *aTarget = mTempFile;

  NS_IF_ADDREF(*aTarget);
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetTargetFileIsExecutable(PRBool *aExec)
{
  
  if (mFinalFileDestination)
    return mFinalFileDestination->IsExecutable(aExec);

  
  *aExec = mTempFileIsExecutable;
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetTimeDownloadStarted(PRTime* aTime)
{
  *aTime = mTimeDownloadStarted;
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetContentLength(PRInt64 *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::CloseProgressWindow()
{
  
  mWebProgressListener = nsnull;
  return NS_OK;
}

void nsExternalAppHandler::RetargetLoadNotifications(nsIRequest *request)
{
  
  
  nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(request);
  if (!aChannel)
    return;

  
  
  
  
  
  
  
  

  
  
  nsCOMPtr<nsIDocumentLoader> origContextLoader =
    do_GetInterface(mWindowContext);
  if (origContextLoader)
    origContextLoader->GetDocumentChannel(getter_AddRefs(mOriginalChannel));

  nsCOMPtr<nsILoadGroup> oldLoadGroup;
  aChannel->GetLoadGroup(getter_AddRefs(oldLoadGroup));

  if(oldLoadGroup)
     oldLoadGroup->RemoveRequest(request, nsnull, NS_BINDING_RETARGETED);
      
  aChannel->SetLoadGroup(nsnull);
  aChannel->SetNotificationCallbacks(nsnull);

}














void nsExternalAppHandler::EnsureSuggestedFileName()
{
  
  
  
  if (mTempFileExtension.Length() > 1)
  {
    
    nsAutoString fileExt;
    PRInt32 pos = mSuggestedFileName.RFindChar('.');
    if (pos != kNotFound)
      mSuggestedFileName.Right(fileExt, mSuggestedFileName.Length() - pos);

    
    if (fileExt.Equals(mTempFileExtension, nsCaseInsensitiveStringComparator()))
    {
      
      mTempFileExtension.Truncate();
    }
  }
}

nsresult nsExternalAppHandler::SetUpTempFile(nsIChannel * aChannel)
{
  
  
  nsresult rv = GetDownloadDirectory(getter_AddRefs(mTempFile));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  

  const PRUint32 wantedFileNameLength = 8;
  const PRUint32 requiredBytesLength =
    static_cast<PRUint32>((wantedFileNameLength + 1) / 4 * 3);

  nsCOMPtr<nsIRandomGenerator> rg =
    do_GetService("@mozilla.org/security/random-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint8 *buffer;
  rv = rg->GenerateRandomBytes(requiredBytesLength, &buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  char *b64 = PL_Base64Encode(reinterpret_cast<const char *>(buffer),
                              requiredBytesLength, nsnull);
  NS_Free(buffer);
  buffer = nsnull;

  if (!b64)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ASSERTION(strlen(b64) >= wantedFileNameLength,
               "not enough bytes produced for conversion!");

  nsCAutoString tempLeafName(b64, wantedFileNameLength);
  PR_Free(b64);
  b64 = nsnull;

  
  
  tempLeafName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');

  
  nsCAutoString ext;
  mMimeInfo->GetPrimaryExtension(ext);
  if (!ext.IsEmpty()) {
    ext.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');
    if (ext.First() != '.')
      tempLeafName.Append('.');
    tempLeafName.Append(ext);
  }

#ifdef XP_WIN
  
  
  
  nsCOMPtr<nsIFile> dummyFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dummyFile));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = dummyFile->Append(NS_ConvertUTF8toUTF16(tempLeafName));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dummyFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  dummyFile->IsExecutable(&mTempFileIsExecutable);
  dummyFile->Remove(PR_FALSE);
#endif

  
  
  tempLeafName.Append(NS_LITERAL_CSTRING(".part"));

  rv = mTempFile->Append(NS_ConvertUTF8toUTF16(tempLeafName));
  
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

#ifndef XP_WIN
  
  
  mTempFile->IsExecutable(&mTempFileIsExecutable);
#endif

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), mTempFile,
                                   PR_WRONLY | PR_CREATE_FILE, 0600);
  if (NS_FAILED(rv)) {
    mTempFile->Remove(PR_FALSE);
    return rv;
  }

  mOutStream = NS_BufferOutputStream(outputStream, BUFFERED_OUTPUT_SIZE);

#if defined(XP_MACOSX) && !defined(__LP64__)
    nsCAutoString contentType;
    mMimeInfo->GetMIMEType(contentType);
    if (contentType.LowerCaseEqualsLiteral(APPLICATION_APPLEFILE) ||
        contentType.LowerCaseEqualsLiteral(MULTIPART_APPLEDOUBLE))
    {
      nsCOMPtr<nsIAppleFileDecoder> appleFileDecoder = do_CreateInstance(NS_IAPPLEFILEDECODER_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv))
      {
        rv = appleFileDecoder->Initialize(mOutStream, mTempFile);
        if (NS_SUCCEEDED(rv))
          mOutStream = do_QueryInterface(appleFileDecoder, &rv);
      }
    }
#endif

  return rv;
}

NS_IMETHODIMP nsExternalAppHandler::OnStartRequest(nsIRequest *request, nsISupports * aCtxt)
{
  NS_PRECONDITION(request, "OnStartRequest without request?");

  
  
  mTimeDownloadStarted = PR_Now();

  mRequest = request;

  nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(request);
  
  nsresult rv;
  
  nsCOMPtr<nsIFileChannel> fileChan(do_QueryInterface(request));
  mIsFileChannel = fileChan != nsnull;

  
  nsCOMPtr<nsIPropertyBag2> props(do_QueryInterface(request, &rv));
  if (props) {
    rv = props->GetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH,
                                   &mContentLength.mValue);
  }
  
  if (NS_FAILED(rv) && aChannel) {
    PRInt32 len;
    aChannel->GetContentLength(&len);
    mContentLength = len;
  }

  
  if (props) {
    PRBool tmp = PR_FALSE;
    props->GetPropertyAsBool(NS_LITERAL_STRING("docshell.newWindowTarget"),
                             &tmp);
    mShouldCloseWindow = tmp;
  }

  
  if (aChannel)
  {
    aChannel->GetURI(getter_AddRefs(mSourceUrl));
  }

  rv = SetUpTempFile(aChannel);
  if (NS_FAILED(rv)) {
    mCanceled = PR_TRUE;
    request->Cancel(rv);
    nsAutoString path;
    if (mTempFile)
      mTempFile->GetPath(path);
    SendStatusChange(kWriteError, rv, request, path);
    return NS_OK;
  }

  
  nsCAutoString MIMEType;
  mMimeInfo->GetMIMEType(MIMEType);

  
  RetargetLoadNotifications(request);

  
  if (mOriginalChannel) {
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mOriginalChannel));
    if (httpChannel) {
      nsCAutoString refreshHeader;
      httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("refresh"),
                                     refreshHeader);
      if (!refreshHeader.IsEmpty()) {
        mShouldCloseWindow = PR_FALSE;
      }
    }
  }

  
  
  MaybeCloseWindow();

  nsCOMPtr<nsIEncodedChannel> encChannel = do_QueryInterface( aChannel );
  if (encChannel) 
  {
    
    PRBool applyConversion = PR_TRUE;

    nsCOMPtr<nsIURL> sourceURL(do_QueryInterface(mSourceUrl));
    if (sourceURL)
    {
      nsCAutoString extension;
      sourceURL->GetFileExtension(extension);
      if (!extension.IsEmpty())
      {
        nsCOMPtr<nsIUTF8StringEnumerator> encEnum;
        encChannel->GetContentEncodings(getter_AddRefs(encEnum));
        if (encEnum)
        {
          PRBool hasMore;
          rv = encEnum->HasMore(&hasMore);
          if (NS_SUCCEEDED(rv) && hasMore)
          {
            nsCAutoString encType;
            rv = encEnum->GetNext(encType);
            if (NS_SUCCEEDED(rv) && !encType.IsEmpty())
            {
              NS_ASSERTION(gExtProtSvc, "Where did the service go?");
              gExtProtSvc->ApplyDecodingForExtension(extension, encType,
                                                     &applyConversion);
            }
          }
        }
      }    
    }

    encChannel->SetApplyConversion( applyConversion );
  }

  
  

  
  
  
  
  
  
  

  
  

  PRBool alwaysAsk = PR_TRUE;
  mMimeInfo->GetAlwaysAskBeforeHandling(&alwaysAsk);
  if (alwaysAsk)
  {
    
    
    
    
    
    NS_ASSERTION(gExtProtSvc, "Service gone away!?");

    PRBool mimeTypeIsInDatastore = PR_FALSE;
    nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
    if (handlerSvc)
      handlerSvc->Exists(mMimeInfo, &mimeTypeIsInDatastore);
    if (!handlerSvc || !mimeTypeIsInDatastore)
    {
      if (!GetNeverAskFlagFromPref(NEVER_ASK_FOR_SAVE_TO_DISK_PREF, MIMEType.get()))
      {
        
        alwaysAsk = PR_FALSE;
        
        mMimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);
      }
      else if (!GetNeverAskFlagFromPref(NEVER_ASK_FOR_OPEN_FILE_PREF, MIMEType.get()))
      {
        
        alwaysAsk = PR_FALSE;
      }
    }
  }

  PRInt32 action = nsIMIMEInfo::saveToDisk;
  mMimeInfo->GetPreferredAction( &action );

  
  if (!alwaysAsk && mReason != nsIHelperAppLauncherDialog::REASON_CANTHANDLE) {
    
    
    alwaysAsk = (action != nsIMIMEInfo::saveToDisk);
  }

  
  
  if (mForceSave) {
    alwaysAsk = PR_FALSE;
    action = nsIMIMEInfo::saveToDisk;
  }
  
  if (alwaysAsk)
  {
    
    
    mReceivedDispositionInfo = PR_FALSE; 

    
    mDialog = do_CreateInstance( NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, &rv );
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    rv = mDialog->Show( this, mWindowContext, mReason );

    
  }
  else
  {
    mReceivedDispositionInfo = PR_TRUE; 

    
#ifdef XP_WIN
    








    nsCOMPtr<nsIHandlerApp> prefApp;
    mMimeInfo->GetPreferredApplicationHandler(getter_AddRefs(prefApp));
    if (action != nsIMIMEInfo::useHelperApp || !prefApp) {
      nsCOMPtr<nsIFile> fileToTest;
      GetTargetFile(getter_AddRefs(fileToTest));
      if (fileToTest) {
        PRBool isExecutable;
        rv = fileToTest->IsExecutable(&isExecutable);
        if (NS_FAILED(rv) || isExecutable) {  
          action = nsIMIMEInfo::saveToDisk;
        }
      } else {   
        NS_WARNING("GetDownloadInfo returned a null file after the temp file has been set up! ");
        action = nsIMIMEInfo::saveToDisk;
      }
    }

#endif
    if (action == nsIMIMEInfo::useHelperApp ||
        action == nsIMIMEInfo::useSystemDefault)
    {
        rv = LaunchWithApplication(nsnull, PR_FALSE);
    }
    else 
    {
        rv = SaveToDisk(nsnull, PR_FALSE);
    }
  }

  
  nsCOMPtr<nsIDownloadHistory> dh(do_GetService(NS_DOWNLOADHISTORY_CONTRACTID));
  if (dh) {
    nsCOMPtr<nsIURI> referrer;
    if (aChannel)
      NS_GetReferrerFromChannel(aChannel, getter_AddRefs(referrer));
    dh->AddDownload(mSourceUrl, referrer, mTimeDownloadStarted);
  }

  return NS_OK;
}



void nsExternalAppHandler::SendStatusChange(ErrorType type, nsresult rv, nsIRequest *aRequest, const nsAFlatString &path)
{
    nsAutoString msgId;
    switch(rv)
    {
    case NS_ERROR_OUT_OF_MEMORY:
        
        msgId.AssignLiteral("noMemory");
        break;

    case NS_ERROR_FILE_DISK_FULL:
    case NS_ERROR_FILE_NO_DEVICE_SPACE:
        
        msgId.AssignLiteral("diskFull");
        break;

    case NS_ERROR_FILE_READ_ONLY:
        
        msgId.AssignLiteral("readOnly");
        break;

    case NS_ERROR_FILE_ACCESS_DENIED:
        if (type == kWriteError) {
          
          msgId.AssignLiteral("accessError");
        }
        else
        {
          msgId.AssignLiteral("launchError");
        }
        break;

    case NS_ERROR_FILE_NOT_FOUND:
    case NS_ERROR_FILE_TARGET_DOES_NOT_EXIST:
    case NS_ERROR_FILE_UNRECOGNIZED_PATH:
        
        if (type == kLaunchError) {
          msgId.AssignLiteral("helperAppNotFound");
          break;
        }
        

    default:
        
        switch(type)
        {
        case kReadError:
          msgId.AssignLiteral("readError");
          break;
        case kWriteError:
          msgId.AssignLiteral("writeError");
          break;
        case kLaunchError:
          msgId.AssignLiteral("launchError");
          break;
        }
        break;
    }
    PR_LOG(nsExternalHelperAppService::mLog, PR_LOG_ERROR,
        ("Error: %s, type=%i, listener=0x%p, rv=0x%08X\n",
         NS_LossyConvertUTF16toASCII(msgId).get(), type, mWebProgressListener.get(), rv));
    PR_LOG(nsExternalHelperAppService::mLog, PR_LOG_ERROR,
        ("       path='%s'\n", NS_ConvertUTF16toUTF8(path).get()));

    
    nsCOMPtr<nsIStringBundleService> s = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (s)
    {
        nsCOMPtr<nsIStringBundle> bundle;
        if (NS_SUCCEEDED(s->CreateBundle("chrome://global/locale/nsWebBrowserPersist.properties", getter_AddRefs(bundle))))
        {
            nsXPIDLString msgText;
            const PRUnichar *strings[] = { path.get() };
            if(NS_SUCCEEDED(bundle->FormatStringFromName(msgId.get(), strings, 1, getter_Copies(msgText))))
            {
              if (mWebProgressListener)
              {
                
                mWebProgressListener->OnStatusChange(nsnull, (type == kReadError) ? aRequest : nsnull, rv, msgText);
              }
              else
              {
                
                nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mWindowContext));
                nsXPIDLString title;
                bundle->FormatStringFromName(NS_LITERAL_STRING("title").get(),
                                             strings,
                                             1,
                                             getter_Copies(title));
                if (prompter)
                {
                  prompter->Alert(title, msgText);
                }
              }
            }
        }
    }
}

NS_IMETHODIMP nsExternalAppHandler::OnDataAvailable(nsIRequest *request, nsISupports * aCtxt,
                                                  nsIInputStream * inStr, PRUint32 sourceOffset, PRUint32 count)
{
  nsresult rv = NS_OK;
  
  if (mCanceled) 
    return request->Cancel(NS_BINDING_ABORTED);

  
  if (mOutStream && count > 0)
  {
    PRUint32 numBytesRead = 0; 
    PRUint32 numBytesWritten = 0;
    mProgress += count;
    PRBool readError = PR_TRUE;
    while (NS_SUCCEEDED(rv) && count > 0) 
    {
      readError = PR_TRUE;
      rv = inStr->Read(mDataBuffer, PR_MIN(count, DATA_BUFFER_SIZE - 1), &numBytesRead);
      if (NS_SUCCEEDED(rv))
      {
        if (count >= numBytesRead)
          count -= numBytesRead; 
        else
          count = 0;
        readError = PR_FALSE;
        
        
        
        
        
        const char *bufPtr = mDataBuffer; 
        while (NS_SUCCEEDED(rv) && numBytesRead)
        {
          numBytesWritten = 0;
          rv = mOutStream->Write(bufPtr, numBytesRead, &numBytesWritten);
          if (NS_SUCCEEDED(rv))
          {
            numBytesRead -= numBytesWritten;
            bufPtr += numBytesWritten;
            
            
            if (!numBytesWritten)
            {
              rv = NS_ERROR_FAILURE;
            }
          }
        }
      }
    }
    if (NS_SUCCEEDED(rv))
    {
      
      if (mWebProgressListener)
      {
        mWebProgressListener->OnProgressChange64(nsnull, request, mProgress, mContentLength, mProgress, mContentLength);
      }
    }
    else
    {
      
      nsAutoString tempFilePath;
      if (mTempFile)
        mTempFile->GetPath(tempFilePath);
      SendStatusChange(readError ? kReadError : kWriteError, rv, request, tempFilePath);

      
      Cancel(rv);
    }
  }
  return rv;
}

NS_IMETHODIMP nsExternalAppHandler::OnStopRequest(nsIRequest *request, nsISupports *aCtxt, 
                                                  nsresult aStatus)
{
  mStopRequestIssued = PR_TRUE;
  mRequest = nsnull;
  
  if (!mCanceled && NS_FAILED(aStatus))
  {
    
    nsAutoString tempFilePath;
    if (mTempFile)
      mTempFile->GetPath(tempFilePath);
    SendStatusChange( kReadError, aStatus, request, tempFilePath );

    Cancel(aStatus);
  }

  
  if (mCanceled)
    return NS_OK;

  
  if (mOutStream)
  {
    mOutStream->Close();
    mOutStream = nsnull;
  }

  
  ExecuteDesiredAction();

  
  
  
  
  
  mWebProgressListener = nsnull;

  return NS_OK;
}

nsresult nsExternalAppHandler::ExecuteDesiredAction()
{
  nsresult rv = NS_OK;
  if (mProgressListenerInitialized && !mCanceled)
  {
    nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
    mMimeInfo->GetPreferredAction(&action);
    if (action == nsIMIMEInfo::useHelperApp ||
        action == nsIMIMEInfo::useSystemDefault)
    {
      
      
      
      rv = mFinalFileDestination->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
      if (NS_SUCCEEDED(rv))
      {
        
        rv = MoveFile(mFinalFileDestination);
        if (NS_SUCCEEDED(rv))
          rv = OpenWithApplication();
      }
    }
    else 
    {
      
      
      rv = MoveFile(mFinalFileDestination);
      if (NS_SUCCEEDED(rv) && action == nsIMIMEInfo::saveToDisk)
      {
        nsCOMPtr<nsILocalFile> destfile(do_QueryInterface(mFinalFileDestination));
        gExtProtSvc->FixFilePermissions(destfile);
      }
    }
    
    
    
    
    if(mWebProgressListener)
    {
      if (!mCanceled)
      {
        mWebProgressListener->OnProgressChange64(nsnull, nsnull, mProgress, mContentLength, mProgress, mContentLength);
      }
      mWebProgressListener->OnStateChange(nsnull, nsnull,
        nsIWebProgressListener::STATE_STOP |
        nsIWebProgressListener::STATE_IS_REQUEST |
        nsIWebProgressListener::STATE_IS_NETWORK, NS_OK);
    }
  }
  
  return rv;
}

NS_IMETHODIMP nsExternalAppHandler::GetMIMEInfo(nsIMIMEInfo ** aMIMEInfo)
{
  *aMIMEInfo = mMimeInfo;
  NS_ADDREF(*aMIMEInfo);
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetSource(nsIURI ** aSourceURI)
{
  NS_ENSURE_ARG(aSourceURI);
  *aSourceURI = mSourceUrl;
  NS_IF_ADDREF(*aSourceURI);
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::GetSuggestedFileName(nsAString& aSuggestedFileName)
{
  aSuggestedFileName = mSuggestedFileName;
  return NS_OK;
}

nsresult nsExternalAppHandler::InitializeDownload(nsITransfer* aTransfer)
{
  nsresult rv;
  
  nsCOMPtr<nsIURI> target;
  rv = NS_NewFileURI(getter_AddRefs(target), mFinalFileDestination);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsILocalFile> lf(do_QueryInterface(mTempFile));
  rv = aTransfer->Init(mSourceUrl, target, EmptyString(),
                       mMimeInfo, mTimeDownloadStarted, lf, this);
  if (NS_FAILED(rv)) return rv;

  return rv;
}

nsresult nsExternalAppHandler::CreateProgressListener()
{
  
  
  
  
  mDialog = nsnull;
  nsresult rv;
  
  nsCOMPtr<nsITransfer> tr = do_CreateInstance(NS_TRANSFER_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
    InitializeDownload(tr);

  if (tr)
    tr->OnStateChange(nsnull, mRequest, nsIWebProgressListener::STATE_START |
      nsIWebProgressListener::STATE_IS_REQUEST |
      nsIWebProgressListener::STATE_IS_NETWORK, NS_OK);

  
  
  
  
  
  
  SetWebProgressListener(tr);

  return rv;
}

nsresult nsExternalAppHandler::PromptForSaveToFile(nsILocalFile ** aNewFile, const nsAFlatString &aDefaultFile, const nsAFlatString &aFileExtension)
{
  
  
  
  nsresult rv = NS_OK;
  if (!mDialog)
  {
    
    mDialog = do_CreateInstance( NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, &rv );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  

  
  
  
  
  
  nsRefPtr<nsExternalAppHandler> kungFuDeathGrip(this);
  nsCOMPtr<nsIHelperAppLauncherDialog> dlg(mDialog);
  rv = mDialog->PromptForSaveToFile(this, 
                                    mWindowContext,
                                    aDefaultFile.get(),
                                    aFileExtension.get(),
                                    mForceSave, aNewFile);
  return rv;
}

nsresult nsExternalAppHandler::MoveFile(nsIFile * aNewFileLocation)
{
  nsresult rv = NS_OK;
  NS_ASSERTION(mStopRequestIssued, "uhoh, how did we get here if we aren't done getting data?");
 
  nsCOMPtr<nsILocalFile> fileToUse = do_QueryInterface(aNewFileLocation);

  
  if (mStopRequestIssued && fileToUse)
  {
    
    
    
    
    PRBool equalToTempFile = PR_FALSE;
    PRBool filetoUseAlreadyExists = PR_FALSE;
    fileToUse->Equals(mTempFile, &equalToTempFile);
    fileToUse->Exists(&filetoUseAlreadyExists);
    if (filetoUseAlreadyExists && !equalToTempFile)
      fileToUse->Remove(PR_FALSE);

     
     nsAutoString fileName;
     fileToUse->GetLeafName(fileName);
     nsCOMPtr<nsIFile> directoryLocation;
     rv = fileToUse->GetParent(getter_AddRefs(directoryLocation));
     if (directoryLocation)
     {
       rv = mTempFile->MoveTo(directoryLocation, fileName);
     }
     if (NS_FAILED(rv))
     {
       
       nsAutoString path;
       fileToUse->GetPath(path);
       SendStatusChange(kWriteError, rv, nsnull, path);
       Cancel(rv); 
     }
#if defined(XP_OS2)
     else
     {
       
       nsCOMPtr<nsILocalFileOS2> localFileOS2 = do_QueryInterface(fileToUse);
       if (localFileOS2)
       {
         nsCAutoString url;
         mSourceUrl->GetSpec(url);
         localFileOS2->SetFileSource(url);
       }
     }
#endif
  }

  return rv;
}









NS_IMETHODIMP nsExternalAppHandler::SaveToDisk(nsIFile * aNewFileLocation, PRBool aRememberThisPreference)
{
  nsresult rv = NS_OK;
  if (mCanceled)
    return NS_OK;

  mMimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);

  
  mReceivedDispositionInfo = PR_TRUE;

  nsCOMPtr<nsILocalFile> fileToUse = do_QueryInterface(aNewFileLocation);
  if (!fileToUse)
  {
    nsAutoString leafName;
    mTempFile->GetLeafName(leafName);
    if (mSuggestedFileName.IsEmpty())
      rv = PromptForSaveToFile(getter_AddRefs(fileToUse), leafName, mTempFileExtension);
    else
    {
      nsAutoString fileExt;
      PRInt32 pos = mSuggestedFileName.RFindChar('.');
      if (pos >= 0)
        mSuggestedFileName.Right(fileExt, mSuggestedFileName.Length() - pos);
      if (fileExt.IsEmpty())
        fileExt = mTempFileExtension;

      rv = PromptForSaveToFile(getter_AddRefs(fileToUse), mSuggestedFileName, fileExt);
    }

    if (NS_FAILED(rv) || !fileToUse) {
      Cancel(NS_BINDING_ABORTED);
      return NS_ERROR_FAILURE;
    }
  }
  
  mFinalFileDestination = do_QueryInterface(fileToUse);

  
  
  
  if (mFinalFileDestination && !mStopRequestIssued)
  {
    nsCOMPtr<nsIFile> movedFile;
    mFinalFileDestination->Clone(getter_AddRefs(movedFile));
    if (movedFile) {
      
      nsAutoString name;
      mFinalFileDestination->GetLeafName(name);
      name.AppendLiteral(".part");
      movedFile->SetLeafName(name);

      nsCOMPtr<nsIFile> dir;
      movedFile->GetParent(getter_AddRefs(dir));

      mOutStream->Close();

      rv = mTempFile->MoveTo(dir, name);
      if (NS_SUCCEEDED(rv)) 
        mTempFile = movedFile;

      nsCOMPtr<nsIOutputStream> outputStream;
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), mTempFile,
                                       PR_WRONLY | PR_APPEND, 0600);
      if (NS_FAILED(rv)) { 
        nsAutoString path;
        mTempFile->GetPath(path);
        SendStatusChange(kWriteError, rv, nsnull, path);
        Cancel(rv);
        return NS_OK;
      }

      mOutStream = NS_BufferOutputStream(outputStream, BUFFERED_OUTPUT_SIZE);
    }
  }

  if (!mProgressListenerInitialized)
    CreateProgressListener();

  
  
  
  
  ProcessAnyRefreshTags();

  return NS_OK;
}


nsresult nsExternalAppHandler::OpenWithApplication()
{
  nsresult rv = NS_OK;
  if (mCanceled)
    return NS_OK;
  
  

  NS_ASSERTION(mStopRequestIssued, "uhoh, how did we get here if we aren't done getting data?");
  
  if (mStopRequestIssued)
  {
    PRBool deleteTempFileOnExit;
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefs || NS_FAILED(prefs->GetBoolPref(
        "browser.helperApps.deleteTempFileOnExit", &deleteTempFileOnExit))) {
      
#if !defined(XP_MACOSX)
      
      
      
      deleteTempFileOnExit = PR_TRUE;
#else
      deleteTempFileOnExit = PR_FALSE;
#endif
    }

    
    
    if (deleteTempFileOnExit || gExtProtSvc->InPrivateBrowsing())
      mFinalFileDestination->SetPermissions(0400);

    rv = mMimeInfo->LaunchWithFile(mFinalFileDestination);        
    if (NS_FAILED(rv))
    {
      
      nsAutoString path;
      mFinalFileDestination->GetPath(path);
      SendStatusChange(kLaunchError, rv, nsnull, path);
      Cancel(rv); 
    }
    
    
    else if (deleteTempFileOnExit || gExtProtSvc->InPrivateBrowsing()) {
      NS_ASSERTION(gExtProtSvc, "Service gone away!?");
      gExtProtSvc->DeleteTemporaryFileOnExit(mFinalFileDestination);
    }
  }

  return rv;
}






NS_IMETHODIMP nsExternalAppHandler::LaunchWithApplication(nsIFile * aApplication, PRBool aRememberThisPreference)
{
  if (mCanceled)
    return NS_OK;

  
  ProcessAnyRefreshTags(); 
  
  mReceivedDispositionInfo = PR_TRUE; 
  if (mMimeInfo && aApplication) {
    PlatformLocalHandlerApp_t *handlerApp =
      new PlatformLocalHandlerApp_t(EmptyString(), aApplication);
    mMimeInfo->SetPreferredApplicationHandler(handlerApp);
  }

  
  
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(mSourceUrl));
  if (fileUrl && mIsFileChannel)
  {
    Cancel(NS_BINDING_ABORTED);
    nsCOMPtr<nsIFile> file;
    nsresult rv = fileUrl->GetFile(getter_AddRefs(file));

    if (NS_SUCCEEDED(rv))
    {
      rv = mMimeInfo->LaunchWithFile(file);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
    nsAutoString path;
    if (file)
      file->GetPath(path);
    
    SendStatusChange(kLaunchError, rv, nsnull, path);
    return rv;
  }

  
  
  
  
  

  nsCOMPtr<nsIFile> fileToUse;
  (void) GetDownloadDirectory(getter_AddRefs(fileToUse));

  if (mSuggestedFileName.IsEmpty())
  {
    
    mTempFile->GetLeafName(mSuggestedFileName);
  }

#ifdef XP_WIN
  fileToUse->Append(mSuggestedFileName + mTempFileExtension);
#else
  fileToUse->Append(mSuggestedFileName);  
#endif
  
  

  mFinalFileDestination = do_QueryInterface(fileToUse);

  
  if (!mProgressListenerInitialized)
   CreateProgressListener();

  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::Cancel(nsresult aReason)
{
  NS_ENSURE_ARG(NS_FAILED(aReason));
  

  mCanceled = PR_TRUE;
  
  
  mDialog = nsnull;
  
  if (mOutStream)
  {
    mOutStream->Close();
    mOutStream = nsnull;
  }

  
  
  
  
  if (mTempFile && !mReceivedDispositionInfo)
  {
    mTempFile->Remove(PR_FALSE);
    mTempFile = nsnull;
  }

  
  
  mWebProgressListener = nsnull;

  return NS_OK;
}

void nsExternalAppHandler::ProcessAnyRefreshTags()
{
   
   
   
   
   
   
   
   if (mWindowContext && mOriginalChannel)
   {
     nsCOMPtr<nsIRefreshURI> refreshHandler (do_GetInterface(mWindowContext));
     if (refreshHandler) {
        refreshHandler->SetupRefreshURI(mOriginalChannel);
     }
     mOriginalChannel = nsnull;
   }
}

PRBool nsExternalAppHandler::GetNeverAskFlagFromPref(const char * prefName, const char * aContentType)
{
  
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  if (prefs)
    rv = prefs->GetBranch(NEVER_ASK_PREF_BRANCH, getter_AddRefs(prefBranch));
  if (NS_SUCCEEDED(rv) && prefBranch)
  {
    nsXPIDLCString prefCString;
    nsCAutoString prefValue;
    rv = prefBranch->GetCharPref(prefName, getter_Copies(prefCString));
    if (NS_SUCCEEDED(rv) && !prefCString.IsEmpty())
    {
      NS_UnescapeURL(prefCString);
      nsACString::const_iterator start, end;
      prefCString.BeginReading(start);
      prefCString.EndReading(end);
      if (CaseInsensitiveFindInReadable(nsDependentCString(aContentType), start, end))
        return PR_FALSE;
    }
  }
  
  return PR_TRUE;
}

nsresult nsExternalAppHandler::MaybeCloseWindow()
{
  nsCOMPtr<nsIDOMWindow> window(do_GetInterface(mWindowContext));
  nsCOMPtr<nsIDOMWindowInternal> internalWindow = do_QueryInterface(window);
  NS_ENSURE_STATE(internalWindow);

  if (mShouldCloseWindow) {
    
    
    nsCOMPtr<nsIDOMWindowInternal> opener;
    internalWindow->GetOpener(getter_AddRefs(opener));

    PRBool isClosed;
    if (opener && NS_SUCCEEDED(opener->GetClosed(&isClosed)) && !isClosed) {
      mWindowContext = do_GetInterface(opener);

      
      
      NS_ASSERTION(!mTimer, "mTimer was already initialized once!");
      mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (!mTimer) {
        return NS_ERROR_FAILURE;
      }

      mTimer->InitWithCallback(this, 0, nsITimer::TYPE_ONE_SHOT);
      mWindowToClose = internalWindow;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsExternalAppHandler::Notify(nsITimer* timer)
{
  NS_ASSERTION(mWindowToClose, "No window to close after timer fired");

  mWindowToClose->Close();
  mWindowToClose = nsnull;
  mTimer = nsnull;

  return NS_OK;
}






NS_IMETHODIMP nsExternalHelperAppService::GetFromTypeAndExtension(const nsACString& aMIMEType, const nsACString& aFileExt, nsIMIMEInfo **_retval) 
{
  NS_PRECONDITION(!aMIMEType.IsEmpty() ||
                  !aFileExt.IsEmpty(), 
                  "Give me something to work with");
  LOG(("Getting mimeinfo from type '%s' ext '%s'\n",
        PromiseFlatCString(aMIMEType).get(), PromiseFlatCString(aFileExt).get()));

  *_retval = nsnull;

  
  nsCAutoString typeToUse(aMIMEType);
  if (typeToUse.IsEmpty()) {
    nsresult rv = GetTypeFromExtension(aFileExt, typeToUse);
    if (NS_FAILED(rv))
      return NS_ERROR_NOT_AVAILABLE;
  }

  
  ToLowerCase(typeToUse);

  
  PRBool found;
  *_retval = GetMIMEInfoFromOS(typeToUse, aFileExt, &found).get();
  LOG(("OS gave back 0x%p - found: %i\n", *_retval, found));
  
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  nsresult rv;
  nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
  if (handlerSvc) {
    PRBool hasHandler = PR_FALSE;
    (void) handlerSvc->Exists(*_retval, &hasHandler);
    if (hasHandler) {
      rv = handlerSvc->FillHandlerInfo(*_retval, EmptyCString());
      LOG(("Data source: Via type: retval 0x%08x\n", rv));
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
 
    found = found || NS_SUCCEEDED(rv);

    if (!found || NS_FAILED(rv)) {
      
      if (!aFileExt.IsEmpty()) {
        nsCAutoString overrideType;
        rv = handlerSvc->GetTypeFromExtension(aFileExt, overrideType);
        if (NS_SUCCEEDED(rv) && !overrideType.IsEmpty()) {
          
          
          
          rv = handlerSvc->FillHandlerInfo(*_retval, overrideType);
          LOG(("Data source: Via ext: retval 0x%08x\n", rv));
          found = found || NS_SUCCEEDED(rv);
        }
      }
    }
  }

  
  if (!found) {
    rv = NS_ERROR_FAILURE;
#ifdef XP_WIN
    






    if (!typeToUse.Equals(APPLICATION_OCTET_STREAM, nsCaseInsensitiveCStringComparator()))
#endif
      rv = FillMIMEInfoForMimeTypeFromExtras(typeToUse, *_retval);
    LOG(("Searched extras (by type), rv 0x%08X\n", rv));
    
    if (NS_FAILED(rv) && !aFileExt.IsEmpty()) {
      rv = FillMIMEInfoForExtensionFromExtras(aFileExt, *_retval);
      LOG(("Searched extras (by ext), rv 0x%08X\n", rv));
    }
    
    if (NS_FAILED(rv) && !aFileExt.IsEmpty()) {
      
      nsCAutoString desc(aFileExt);
      desc.Append(" File");
      (*_retval)->SetDescription(NS_ConvertASCIItoUTF16(desc));
      LOG(("Falling back to 'File' file description\n"));
    }
  }

  
  
  if (!aFileExt.IsEmpty()) {
    PRBool matches = PR_FALSE;
    (*_retval)->ExtensionExists(aFileExt, &matches);
    LOG(("Extension '%s' matches mime info: %i\n", PromiseFlatCString(aFileExt).get(), matches));
    if (matches)
      (*_retval)->SetPrimaryExtension(aFileExt);
  }

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString type;
    (*_retval)->GetMIMEType(type);

    nsCAutoString ext;
    (*_retval)->GetPrimaryExtension(ext);
    LOG(("MIME Info Summary: Type '%s', Primary Ext '%s'\n", type.get(), ext.get()));
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP nsExternalHelperAppService::GetTypeFromExtension(const nsACString& aFileExt, nsACString& aContentType) 
{
  
  
  
  
  
  
  

  nsresult rv = NS_OK;
  
  for (size_t i = 0; i < NS_ARRAY_LENGTH(defaultMimeEntries); i++)
  {
    if (aFileExt.LowerCaseEqualsASCII(defaultMimeEntries[i].mFileExtension)) {
      aContentType = defaultMimeEntries[i].mMimeType;
      return rv;
    }
  }

  
  nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
  if (handlerSvc)
    rv = handlerSvc->GetTypeFromExtension(aFileExt, aContentType);
  if (NS_SUCCEEDED(rv) && !aContentType.IsEmpty())
    return NS_OK;

  
  PRBool found = PR_FALSE;
  nsCOMPtr<nsIMIMEInfo> mi = GetMIMEInfoFromOS(EmptyCString(), aFileExt, &found);
  if (mi && found)
    return mi->GetMIMEType(aContentType);

  
  found = GetTypeFromExtras(aFileExt, aContentType);
  if (found)
    return NS_OK;

  const nsCString& flatExt = PromiseFlatCString(aFileExt);
  
  const char* mimeType;
  nsCOMPtr<nsIPluginHost> pluginHost (do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
    if (NS_SUCCEEDED(pluginHost->IsPluginEnabledForExtension(flatExt.get(), mimeType))) {
      aContentType = mimeType;
      return NS_OK;
    }
  }
  
  rv = NS_OK;
  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService("@mozilla.org/categorymanager;1"));
  if (catMan) {
    
    nsCAutoString lowercaseFileExt(aFileExt);
    ToLowerCase(lowercaseFileExt);
    
    nsXPIDLCString type;
    rv = catMan->GetCategoryEntry("ext-to-type-mapping", lowercaseFileExt.get(),
                                  getter_Copies(type));
    aContentType = type;
  }
  else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }
  
  return rv;
}

NS_IMETHODIMP nsExternalHelperAppService::GetPrimaryExtension(const nsACString& aMIMEType, const nsACString& aFileExt, nsACString& _retval)
{
  NS_ENSURE_ARG(!aMIMEType.IsEmpty());

  nsCOMPtr<nsIMIMEInfo> mi;
  nsresult rv = GetFromTypeAndExtension(aMIMEType, aFileExt, getter_AddRefs(mi));
  if (NS_FAILED(rv))
    return rv;

  return mi->GetPrimaryExtension(_retval);
}

NS_IMETHODIMP nsExternalHelperAppService::GetTypeFromURI(nsIURI *aURI, nsACString& aContentType) 
{
  nsresult rv = NS_ERROR_NOT_AVAILABLE;
  aContentType.Truncate();

  
  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(aURI);
  if (fileUrl) {
    nsCOMPtr<nsIFile> file;
    rv = fileUrl->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
      rv = GetTypeFromFile(file, aContentType);
      if (NS_SUCCEEDED(rv)) {
        
        return rv;
      }
    }
  }

  
  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
  if (url) {
    nsCAutoString ext;
    rv = url->GetFileExtension(ext);
    if (NS_FAILED(rv))
      return rv;
    if (ext.IsEmpty())
      return NS_ERROR_NOT_AVAILABLE;

    UnescapeFragment(ext, url, ext);

    return GetTypeFromExtension(ext, aContentType);
  }
    
  
  nsCAutoString specStr;
  rv = aURI->GetSpec(specStr);
  if (NS_FAILED(rv))
    return rv;
  UnescapeFragment(specStr, aURI, specStr);

  
  PRInt32 extLoc = specStr.RFindChar('.');
  PRInt32 specLength = specStr.Length();
  if (-1 != extLoc &&
      extLoc != specLength - 1 &&
      
      
      specLength - extLoc < 20) 
  {
    return GetTypeFromExtension(Substring(specStr, extLoc + 1), aContentType);
  }

  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP nsExternalHelperAppService::GetTypeFromFile(nsIFile* aFile, nsACString& aContentType)
{
  nsresult rv;
  nsCOMPtr<nsIMIMEInfo> info;

  
  nsAutoString fileName;
  rv = aFile->GetLeafName(fileName);
  if (NS_FAILED(rv)) return rv;
 
  nsCAutoString fileExt;
  if (!fileName.IsEmpty())
  {
    PRInt32 len = fileName.Length(); 
    for (PRInt32 i = len; i >= 0; i--) 
    {
      if (fileName[i] == PRUnichar('.'))
      {
        CopyUTF16toUTF8(fileName.get() + i + 1, fileExt);
        break;
      }
    }
  }

  if (fileExt.IsEmpty())
    return NS_ERROR_FAILURE;

  return GetTypeFromExtension(fileExt, aContentType);
}

nsresult nsExternalHelperAppService::FillMIMEInfoForMimeTypeFromExtras(
  const nsACString& aContentType, nsIMIMEInfo * aMIMEInfo)
{
  NS_ENSURE_ARG( aMIMEInfo );

  NS_ENSURE_ARG( !aContentType.IsEmpty() );

  
  nsCAutoString MIMEType(aContentType);
  ToLowerCase(MIMEType);
  PRInt32 numEntries = NS_ARRAY_LENGTH(extraMimeEntries);
  for (PRInt32 index = 0; index < numEntries; index++)
  {
      if ( MIMEType.Equals(extraMimeEntries[index].mMimeType) )
      {
          
          aMIMEInfo->SetFileExtensions(nsDependentCString(extraMimeEntries[index].mFileExtensions));
          aMIMEInfo->SetDescription(NS_ConvertASCIItoUTF16(extraMimeEntries[index].mDescription));
          return NS_OK;
      }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult nsExternalHelperAppService::FillMIMEInfoForExtensionFromExtras(
  const nsACString& aExtension, nsIMIMEInfo * aMIMEInfo)
{
  nsCAutoString type;
  PRBool found = GetTypeFromExtras(aExtension, type);
  if (!found)
    return NS_ERROR_NOT_AVAILABLE;
  return FillMIMEInfoForMimeTypeFromExtras(type, aMIMEInfo);
}

PRBool nsExternalHelperAppService::GetTypeFromExtras(const nsACString& aExtension, nsACString& aMIMEType)
{
  NS_ASSERTION(!aExtension.IsEmpty(), "Empty aExtension parameter!");

  
  nsDependentCString::const_iterator start, end, iter;
  PRInt32 numEntries = NS_ARRAY_LENGTH(extraMimeEntries);
  for (PRInt32 index = 0; index < numEntries; index++)
  {
      nsDependentCString extList(extraMimeEntries[index].mFileExtensions);
      extList.BeginReading(start);
      extList.EndReading(end);
      iter = start;
      while (start != end)
      {
          FindCharInReadable(',', iter, end);
          if (Substring(start, iter).Equals(aExtension,
                                            nsCaseInsensitiveCStringComparator()))
          {
              aMIMEType = extraMimeEntries[index].mMimeType;
              return PR_TRUE;
          }
          if (iter != end) {
            ++iter;
          }
          start = iter;
      }
  }

  return PR_FALSE;
}
