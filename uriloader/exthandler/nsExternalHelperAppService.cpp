





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "base/basictypes.h"


#include "mozilla/Base64.h"
#include "mozilla/Util.h"

#include "mozilla/dom/ContentChild.h"
#include "nsXULAppAPI.h"

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
#include "nsDependentSubstring.h"
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
#include "nsIHttpChannelInternal.h"
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
#include "nsPluginHost.h"
#include "nsEscape.h"

#include "nsIStringBundle.h" 
#include "nsIPrompt.h"

#include "nsITextToSubURI.h" 
#include "nsIMIMEHeaderParam.h"

#include "nsIWindowWatcher.h"

#include "nsIDownloadHistory.h" 
#include "nsDocShellCID.h"

#include "nsIDOMWindow.h"
#include "nsIDocShell.h"

#include "nsCRT.h"

#include "nsLocalHandlerApp.h"

#include "nsIRandomGenerator.h"

#include "ContentChild.h"
#include "nsXULAppAPI.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "ExternalHelperAppChild.h"

#ifdef XP_WIN
#include "nsWindowsHelpers.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

#include "mozilla/Preferences.h"
#include "mozilla/ipc/URIUtils.h"

using namespace mozilla;
using namespace mozilla::ipc;


#define BUFFERED_OUTPUT_SIZE (1024 * 32)


#define NS_PREF_DOWNLOAD_DIR        "browser.download.dir"
#define NS_PREF_DOWNLOAD_FOLDERLIST "browser.download.folderList"
enum {
  NS_FOLDER_VALUE_DESKTOP = 0
, NS_FOLDER_VALUE_DOWNLOADS = 1
, NS_FOLDER_VALUE_CUSTOM = 2
};

#ifdef PR_LOGGING
PRLogModuleInfo* nsExternalHelperAppService::mLog = nullptr;
#endif




#undef LOG
#define LOG(args) PR_LOG(mLog, 3, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, 3)

static const char NEVER_ASK_FOR_SAVE_TO_DISK_PREF[] =
  "browser.helperApps.neverAsk.saveToDisk";
static const char NEVER_ASK_FOR_OPEN_FILE_PREF[] =
  "browser.helperApps.neverAsk.openFile";










static nsresult UnescapeFragment(const nsACString& aFragment, nsIURI* aURI,
                                 nsAString& aResult)
{
  
  nsAutoCString originCharset;
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



















static bool GetFilenameAndExtensionFromChannel(nsIChannel* aChannel,
                                                 nsString& aFileName,
                                                 nsCString& aExtension,
                                                 bool aAllowURLExtension = true)
{
  aExtension.Truncate();
  






  bool handleExternally = false;
  uint32_t disp;
  nsresult rv = aChannel->GetContentDisposition(&disp);
  if (NS_SUCCEEDED(rv))
  {
    aChannel->GetContentDispositionFilename(aFileName);
    if (disp == nsIChannel::DISPOSITION_ATTACHMENT)
      handleExternally = true;
  }

  
  nsCOMPtr<nsIURI> uri;
  aChannel->GetURI(getter_AddRefs(uri));
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url && aFileName.IsEmpty())
  {
    if (aAllowURLExtension) {
      url->GetFileExtension(aExtension);
      UnescapeFragment(aExtension, url, aExtension);

      
      
      
      
      aExtension.Trim(".", false);
    }

    
    
    nsAutoCString leafName;
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
      
      
      aFileName.Trim(".", false);

      
      nsAutoString fileNameStr(aFileName);
      int32_t idx = fileNameStr.RFindChar(PRUnichar('.'));
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
  
  switch (Preferences::GetInt(NS_PREF_DOWNLOAD_FOLDERLIST, -1)) {
    case NS_FOLDER_VALUE_DESKTOP:
      (void) NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(dir));
      break;
    case NS_FOLDER_VALUE_CUSTOM:
      {
        Preferences::GetComplex(NS_PREF_DOWNLOAD_DIR,
                                NS_GET_IID(nsIFile),
                                getter_AddRefs(dir));
        if (!dir) break;

        
        bool dirExists = false;
        (void) dir->Exists(&dirExists);
        if (dirExists) break;

        nsresult rv = dir->Create(nsIFile::DIRECTORY_TYPE, 0755);
        if (NS_FAILED(rv)) {
          dir = nullptr;
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
#elif defined(ANDROID)
  
  

  
  char* downloadDir = getenv("DOWNLOADS_DIRECTORY");
  nsresult rv;
  if (downloadDir) {
    nsCOMPtr<nsIFile> ldir; 
    rv = NS_NewNativeLocalFile(nsDependentCString(downloadDir),
                               true, getter_AddRefs(ldir));
    NS_ENSURE_SUCCESS(rv, rv);
    dir = do_QueryInterface(ldir);
  }
  else {
    return NS_ERROR_FAILURE;
  }
#elif defined(MOZ_PLATFORM_MAEMO)
  nsresult rv = NS_GetSpecialDirectory(NS_UNIX_XDG_DOCUMENTS_DIR, getter_AddRefs(dir));
  NS_ENSURE_SUCCESS(rv, rv);
#elif defined(XP_WIN)
  
  
  nsresult rv;
  if (IsRunningInWindowsMetro()) {
    rv = NS_GetSpecialDirectory(NS_WIN_DEFAULT_DOWNLOAD_DIR, getter_AddRefs(dir));
  } else {
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dir));
  }
  NS_ENSURE_SUCCESS(rv, rv);
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
  { IMAGE_JPEG, "jpeg" },
  { IMAGE_JPEG, "jpg" },
  { TEXT_HTML, "html" },
  { TEXT_HTML, "htm" },
  { APPLICATION_XPINSTALL, "xpi" },
  { "application/xhtml+xml", "xhtml" },
  { "application/xhtml+xml", "xht" },
  { TEXT_PLAIN, "txt" },
#ifdef MOZ_OGG
  { VIDEO_OGG, "ogv" },
  { VIDEO_OGG, "ogg" },
  { APPLICATION_OGG, "ogg" },
  { AUDIO_OGG, "oga" },
#ifdef MOZ_OPUS
  { AUDIO_OGG, "opus" },
#endif
#endif
#ifdef MOZ_WEBM
  { VIDEO_WEBM, "webm" },
  { AUDIO_WEBM, "webm" },
#endif
#ifdef MOZ_DASH
  { APPLICATION_DASH, "mpd" },
#endif
#if defined(MOZ_GSTREAMER) || defined(MOZ_WMF)
  { VIDEO_MP4, "mp4" },
  { AUDIO_MP4, "m4a" },
  { AUDIO_MP3, "mp3" },
#endif
#ifdef MOZ_RAW
  { VIDEO_RAW, "yuv" }
#endif
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
  { "application/rtf", "rtf", "Rich Text Format File" },
  { APPLICATION_XPINSTALL, "xpi", "XPInstall Install" },
  { APPLICATION_POSTSCRIPT, "ps,eps,ai", "Postscript File" },
  { APPLICATION_XJAVASCRIPT, "js", "Javascript Source File" },
#ifdef MOZ_WIDGET_ANDROID
  { "application/vnd.android.package-archive", "apk", "Android Package" },
#endif
  { IMAGE_ART, "art", "ART Image" },
  { IMAGE_BMP, "bmp", "BMP Image" },
  { IMAGE_GIF, "gif", "GIF Image" },
  { IMAGE_ICO, "ico,cur", "ICO Image" },
  { IMAGE_JPEG, "jpeg,jpg,jfif,pjpeg,pjp", "JPEG Image" },
  { IMAGE_PNG, "png", "PNG Image" },
  { IMAGE_TIFF, "tiff,tif", "TIFF Image" },
  { IMAGE_XBM, "xbm", "XBM Image" },
#ifdef MOZ_WBMP
  { IMAGE_WBMP, "wbmp", "WBMP Image" },
#endif
  { "image/svg+xml", "svg", "Scalable Vector Graphics" },
  { MESSAGE_RFC822, "eml", "RFC-822 data" },
  { TEXT_PLAIN, "txt,text", "Text File" },
  { TEXT_HTML, "html,htm,shtml,ehtml", "HyperText Markup Language" },
  { "application/xhtml+xml", "xhtml,xht", "Extensible HyperText Markup Language" },
  { APPLICATION_MATHML_XML, "mml", "Mathematical Markup Language" },
  { APPLICATION_RDF, "rdf", "Resource Description Framework" },
  { TEXT_XUL, "xul", "XML-Based User Interface Language" },
  { TEXT_XML, "xml,xsl,xbl", "Extensible Markup Language" },
  { TEXT_CSS, "css", "Style Sheet" },
  { VIDEO_OGG, "ogv", "Ogg Video" },
  { VIDEO_OGG, "ogg", "Ogg Video" },
  { APPLICATION_OGG, "ogg", "Ogg Video"},
  { AUDIO_OGG, "oga", "Ogg Audio" },
  { AUDIO_OGG, "opus", "Opus Audio" },
#ifdef MOZ_WIDGET_GONK
  { AUDIO_AMR, "amr", "Adaptive Multi-Rate Audio" },
#endif
  { VIDEO_WEBM, "webm", "Web Media Video" },
  { AUDIO_WEBM, "webm", "Web Media Audio" },
#ifdef MOZ_DASH
  { APPLICATION_DASH, "mpd", "DASH Media Presentation Description" },
#endif
  { AUDIO_MP3, "mp3", "MPEG Audio" },
  { VIDEO_MP4, "mp4", "MPEG-4 Video" },
  { AUDIO_MP4, "m4a", "MPEG-4 Audio" },
  { VIDEO_RAW, "yuv", "Raw YUV Video" },
  { AUDIO_WAV, "wav", "Waveform Audio" },
  { VIDEO_3GPP, "3gpp,3gp", "3GPP Video" }
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

nsExternalHelperAppService::nsExternalHelperAppService()
{
}
nsresult nsExternalHelperAppService::Init()
{
  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

#ifdef PR_LOGGING
  if (!mLog) {
    mLog = PR_NewLogModule("HelperAppService");
    if (!mLog)
      return NS_ERROR_OUT_OF_MEMORY;
  }
#endif

  nsresult rv = obs->AddObserver(this, "profile-before-change", true);
  NS_ENSURE_SUCCESS(rv, rv);
  return obs->AddObserver(this, "last-pb-context-exited", true);
}

nsExternalHelperAppService::~nsExternalHelperAppService()
{
}

NS_IMETHODIMP nsExternalHelperAppService::DoContent(const nsACString& aMimeContentType,
                                                    nsIRequest *aRequest,
                                                    nsIInterfaceRequestor *aWindowContext,
                                                    bool aForceSave,
                                                    nsIStreamListener ** aStreamListener)
{
  nsAutoString fileName;
  nsAutoCString fileExtension;
  uint32_t reason = nsIHelperAppLauncherDialog::REASON_CANTHANDLE;
  nsresult rv;

  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  nsCOMPtr<nsIURI> uri;
  int64_t contentLength = -1;
  if (channel) {
    channel->GetURI(getter_AddRefs(uri));
    channel->GetContentLength(&contentLength);
  }
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    
    
    
    
    
    using mozilla::dom::ContentChild;
    using mozilla::dom::ExternalHelperAppChild;
    ContentChild *child = ContentChild::GetSingleton();
    if (!child)
      return NS_ERROR_FAILURE;

    nsCString disp;
    if (channel)
      channel->GetContentDispositionHeader(disp);

    nsCOMPtr<nsIURI> referrer;
    rv = NS_GetReferrerFromChannel(channel, getter_AddRefs(referrer));

    OptionalURIParams uriParams, referrerParams;
    SerializeURI(uri, uriParams);
    SerializeURI(referrer, referrerParams);

    
    
    
    
    mozilla::dom::PExternalHelperAppChild *pc =
      child->SendPExternalHelperAppConstructor(uriParams,
                                               nsCString(aMimeContentType),
                                               disp, aForceSave, contentLength,
                                               referrerParams);
    ExternalHelperAppChild *childListener = static_cast<ExternalHelperAppChild *>(pc);

    NS_ADDREF(*aStreamListener = childListener);

    nsRefPtr<nsExternalAppHandler> handler =
      new nsExternalAppHandler(nullptr, EmptyCString(), aWindowContext, this,
                               fileName,
                               reason, aForceSave);
    if (!handler)
      return NS_ERROR_OUT_OF_MEMORY;
    
    childListener->SetHandler(handler);

    return NS_OK;
  }

  if (channel) {
    
    
    bool allowURLExt = true;
    nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(channel);
    if (httpChan) {
      nsAutoCString requestMethod;
      httpChan->GetRequestMethod(requestMethod);
      allowURLExt = !requestMethod.Equals("POST");
    }

    
    
    
    
    if (uri && allowURLExt) {
      nsCOMPtr<nsIURL> url = do_QueryInterface(uri);

      if (url) {
        nsAutoCString query;

        
        bool isHTTP, isHTTPS;
        rv = uri->SchemeIs("http", &isHTTP);
        if (NS_FAILED(rv))
          isHTTP = false;
        rv = uri->SchemeIs("https", &isHTTPS);
        if (NS_FAILED(rv))
          isHTTPS = false;

        if (isHTTP || isHTTPS)
          url->GetQuery(query);

        
        
        allowURLExt = query.IsEmpty();
      }
    }
    
    bool isAttachment = GetFilenameAndExtensionFromChannel(channel, fileName,
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
    nsAutoCString mimeType;
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

  *aStreamListener = nullptr;
  
  
  nsAutoCString buf;
  mimeInfo->GetPrimaryExtension(buf);

  nsExternalAppHandler * handler = new nsExternalAppHandler(mimeInfo,
                                                            buf,
                                                            aWindowContext,
                                                            this,
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
                                                                    bool *aApplyDecoding)
{
  *aApplyDecoding = true;
  uint32_t i;
  for(i = 0; i < ArrayLength(nonDecodableExtensions); ++i) {
    if (aExtension.LowerCaseEqualsASCII(nonDecodableExtensions[i].mFileExtension) &&
        aEncodingType.LowerCaseEqualsASCII(nonDecodableExtensions[i].mMimeType)) {
      *aApplyDecoding = false;
      break;
    }
  }
  return NS_OK;
}

nsresult nsExternalHelperAppService::GetFileTokenForPath(const PRUnichar * aPlatformAppPath,
                                                         nsIFile ** aFile)
{
  nsDependentString platformAppPath(aPlatformAppPath);
  
  nsIFile* localFile = nullptr;
  nsresult rv = NS_NewLocalFile(platformAppPath, true, &localFile);
  if (NS_SUCCEEDED(rv)) {
    *aFile = localFile;
    bool exists;
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
      bool exists = false;
      rv = (*aFile)->Exists(&exists);
      if (NS_SUCCEEDED(rv) && exists)
        return NS_OK;
    }
    NS_RELEASE(*aFile);
  }


  return NS_ERROR_NOT_AVAILABLE;
}




NS_IMETHODIMP nsExternalHelperAppService::ExternalProtocolHandlerExists(const char * aProtocolScheme,
                                                                        bool * aHandlerExists)
{
  nsCOMPtr<nsIHandlerInfo> handlerInfo;
  nsresult rv = GetProtocolHandlerInfo(nsDependentCString(aProtocolScheme), 
                                       getter_AddRefs(handlerInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIMutableArray> possibleHandlers;
  handlerInfo->GetPossibleApplicationHandlers(getter_AddRefs(possibleHandlers));

  uint32_t length;
  possibleHandlers->GetLength(&length);
  if (length) {
    *aHandlerExists = true;
    return NS_OK;
  }

  
  return OSProtocolHandlerExists(aProtocolScheme, aHandlerExists);
}

NS_IMETHODIMP nsExternalHelperAppService::IsExposedProtocol(const char * aProtocolScheme, bool * aResult)
{
  
  

  nsAutoCString prefName("network.protocol-handler.expose.");
  prefName += aProtocolScheme;
  bool val;
  if (NS_SUCCEEDED(Preferences::GetBool(prefName.get(), &val))) {
    *aResult = val;
    return NS_OK;
  }

  
  
  
  *aResult =
    Preferences::GetBool("network.protocol-handler.expose-all", false);

  return NS_OK;
}

NS_IMETHODIMP nsExternalHelperAppService::LoadUrl(nsIURI * aURL)
{
  return LoadURI(aURL, nullptr);
}

static const char kExternalProtocolPrefPrefix[]  = "network.protocol-handler.external.";
static const char kExternalProtocolDefaultPref[] = "network.protocol-handler.external-default";

NS_IMETHODIMP 
nsExternalHelperAppService::LoadURI(nsIURI *aURI,
                                    nsIInterfaceRequestor *aWindowContext)
{
  NS_ENSURE_ARG_POINTER(aURI);

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    URIParams uri;
    SerializeURI(aURI, uri);

    mozilla::dom::ContentChild::GetSingleton()->SendLoadURIExternal(uri);
    return NS_OK;
  }

  nsAutoCString spec;
  aURI->GetSpec(spec);

  if (spec.Find("%00") != -1)
    return NS_ERROR_MALFORMED_URI;

  spec.ReplaceSubstring("\"", "%22");
  spec.ReplaceSubstring("`", "%60");
  
  nsCOMPtr<nsIIOService> ios(do_GetIOService());
  nsCOMPtr<nsIURI> uri;
  nsresult rv = ios->NewURI(spec, nullptr, nullptr, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString scheme;
  uri->GetScheme(scheme);
  if (scheme.IsEmpty())
    return NS_OK; 

  
  nsAutoCString externalPref(kExternalProtocolPrefPrefix);
  externalPref += scheme;
  bool allowLoad  = false;
  if (NS_FAILED(Preferences::GetBool(externalPref.get(), &allowLoad))) {
    
    if (NS_FAILED(Preferences::GetBool(kExternalProtocolDefaultPref,
                                       &allowLoad))) {
      return NS_OK; 
    }
  }

  if (!allowLoad) {
    return NS_OK; 
  }

 
  nsCOMPtr<nsIHandlerInfo> handler;
  rv = GetProtocolHandlerInfo(scheme, getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsHandlerInfoAction preferredAction;
  handler->GetPreferredAction(&preferredAction);
  bool alwaysAsk = true;
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







nsresult
nsExternalHelperAppService::DeleteTemporaryFileHelper(nsIFile * aTemporaryFile,
                                                      nsCOMArray<nsIFile> &aFileList)
{
  bool isFile = false;

  
  aTemporaryFile->IsFile(&isFile);
  if (!isFile) return NS_OK;

  aFileList.AppendObject(aTemporaryFile);

  return NS_OK;
}

NS_IMETHODIMP
nsExternalHelperAppService::DeleteTemporaryFileOnExit(nsIFile* aTemporaryFile)
{
  return DeleteTemporaryFileHelper(aTemporaryFile, mTemporaryFilesList);
}

NS_IMETHODIMP
nsExternalHelperAppService::DeleteTemporaryPrivateFileWhenPossible(nsIFile* aTemporaryFile)
{
  return DeleteTemporaryFileHelper(aTemporaryFile, mTemporaryPrivateFilesList);
}

void nsExternalHelperAppService::FixFilePermissions(nsIFile* aFile)
{
  
}

void nsExternalHelperAppService::ExpungeTemporaryFilesHelper(nsCOMArray<nsIFile> &fileList)
{
  int32_t numEntries = fileList.Count();
  nsIFile* localFile;
  for (int32_t index = 0; index < numEntries; index++)
  {
    localFile = fileList[index];
    if (localFile) {
      
      localFile->SetPermissions(0600);
      localFile->Remove(false);
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
  
  

  bool exists;
  nsresult rv = GetProtocolHandlerInfoFromOS(aScheme, &exists, aHandlerInfo);
  if (NS_FAILED(rv)) {
    
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
  if (handlerSvc) {
    bool hasHandler = false;
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
                                                         bool *found,
                                                         nsIHandlerInfo **aHandlerInfo)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsExternalHelperAppService::SetProtocolHandlerDefaults(nsIHandlerInfo *aHandlerInfo,
                                                       bool aOSHandlerExists)
{
  
  

  if (aOSHandlerExists) {
    
    aHandlerInfo->SetPreferredAction(nsIHandlerInfo::useSystemDefault);

    
    nsAutoCString scheme;
    aHandlerInfo->GetType(scheme);
    
    nsAutoCString warningPref(kExternalWarningPrefPrefix);
    warningPref += scheme;
    bool warn;
    if (NS_FAILED(Preferences::GetBool(warningPref.get(), &warn))) {
      
      warn = Preferences::GetBool(kExternalWarningDefaultPref, true);
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
  } else if (!strcmp(aTopic, "last-pb-context-exited")) {
    ExpungeTemporaryPrivateFiles();
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
   NS_INTERFACE_MAP_ENTRY(nsIBackgroundFileSaverObserver)   
NS_INTERFACE_MAP_END_THREADSAFE

nsExternalAppHandler::nsExternalAppHandler(nsIMIMEInfo * aMIMEInfo,
                                           const nsCSubstring& aTempFileExtension,
                                           nsIInterfaceRequestor* aWindowContext,
                                           nsExternalHelperAppService *aExtProtSvc,
                                           const nsAString& aSuggestedFilename,
                                           uint32_t aReason, bool aForceSave)
: mMimeInfo(aMIMEInfo)
, mWindowContext(aWindowContext)
, mWindowToClose(nullptr)
, mSuggestedFileName(aSuggestedFilename)
, mForceSave(aForceSave)
, mCanceled(false)
, mShouldCloseWindow(false)
, mReceivedDispositionInfo(false)
, mStopRequestIssued(false)
, mProgressListenerInitialized(false)
, mReason(aReason)
, mContentLength(-1)
, mProgress(0)
, mSaver(nullptr)
, mKeepRequestAlive(false)
, mRequest(nullptr)
, mExtProtSvc(aExtProtSvc)
{

  
  if (!aTempFileExtension.IsEmpty() && aTempFileExtension.First() != '.')
    mTempFileExtension = PRUnichar('.');
  AppendUTF8toUTF16(aTempFileExtension, mTempFileExtension);

  
  mSuggestedFileName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');
  mTempFileExtension.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');

  
  const PRUnichar unsafeBidiCharacters[] = {
    PRUnichar(0x202a), 
    PRUnichar(0x202b), 
    PRUnichar(0x202c), 
    PRUnichar(0x202d), 
    PRUnichar(0x202e)  
  };
  for (uint32_t i = 0; i < ArrayLength(unsafeBidiCharacters); ++i) {
    mSuggestedFileName.ReplaceChar(unsafeBidiCharacters[i], '_');
    mTempFileExtension.ReplaceChar(unsafeBidiCharacters[i], '_');
  }
  
  
  EnsureSuggestedFileName();

  mBufferSize = Preferences::GetUint("network.buffer.cache.size", 4096);
}

nsExternalAppHandler::~nsExternalAppHandler()
{
  MOZ_ASSERT(!mSaver, "Saver should hold a reference to us until deleted");
}

NS_IMETHODIMP nsExternalAppHandler::SetWebProgressListener(nsIWebProgressListener2 * aWebProgressListener)
{ 
  
  
  
  
  if (mReceivedDispositionInfo)
    mProgressListenerInitialized = true;

  
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

NS_IMETHODIMP nsExternalAppHandler::GetTargetFileIsExecutable(bool *aExec)
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

NS_IMETHODIMP nsExternalAppHandler::GetContentLength(int64_t *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP nsExternalAppHandler::CloseProgressWindow()
{
  
  mWebProgressListener = nullptr;
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

  bool isPrivate = NS_UsePrivateBrowsing(aChannel);

  nsCOMPtr<nsILoadGroup> oldLoadGroup;
  aChannel->GetLoadGroup(getter_AddRefs(oldLoadGroup));

  if(oldLoadGroup)
     oldLoadGroup->RemoveRequest(request, nullptr, NS_BINDING_RETARGETED);
      
  aChannel->SetLoadGroup(nullptr);
  aChannel->SetNotificationCallbacks(nullptr);

  nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(aChannel);
  if (pbChannel) {
    pbChannel->SetPrivate(isPrivate);
  }
}














void nsExternalAppHandler::EnsureSuggestedFileName()
{
  
  
  
  if (mTempFileExtension.Length() > 1)
  {
    
    nsAutoString fileExt;
    int32_t pos = mSuggestedFileName.RFindChar('.');
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

  
  
  
  
  
  
  

  const uint32_t wantedFileNameLength = 8;
  const uint32_t requiredBytesLength =
    static_cast<uint32_t>((wantedFileNameLength + 1) / 4 * 3);

  nsCOMPtr<nsIRandomGenerator> rg =
    do_GetService("@mozilla.org/security/random-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  uint8_t *buffer;
  rv = rg->GenerateRandomBytes(requiredBytesLength, &buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString tempLeafName;
  nsDependentCSubstring randomData(reinterpret_cast<const char*>(buffer), requiredBytesLength);
  rv = Base64Encode(randomData, tempLeafName);
  NS_Free(buffer);
  buffer = nullptr;
  NS_ENSURE_SUCCESS(rv, rv);

  tempLeafName.Truncate(wantedFileNameLength);

  
  
  tempLeafName.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');

  
  nsAutoCString ext;
  mMimeInfo->GetPrimaryExtension(ext);
  if (!ext.IsEmpty()) {
    ext.ReplaceChar(FILE_PATH_SEPARATOR FILE_ILLEGAL_CHARACTERS, '_');
    if (ext.First() != '.')
      tempLeafName.Append('.');
    tempLeafName.Append(ext);
  }

  
  
  
  nsCOMPtr<nsIFile> dummyFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(dummyFile));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = dummyFile->Append(NS_ConvertUTF8toUTF16(tempLeafName));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dummyFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  dummyFile->IsExecutable(&mTempFileIsExecutable);
  dummyFile->Remove(false);

  
  
  tempLeafName.Append(NS_LITERAL_CSTRING(".part"));

  rv = mTempFile->Append(NS_ConvertUTF8toUTF16(tempLeafName));
  
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  rv = mTempFile->GetLeafName(mTempLeafName);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(StringEndsWith(mTempLeafName, NS_LITERAL_STRING(".part")),
                 NS_ERROR_UNEXPECTED);

  
  mTempLeafName.Truncate(mTempLeafName.Length() - ArrayLength(".part") + 1);

  MOZ_ASSERT(!mSaver, "Output file initialization called more than once!");
  mSaver = do_CreateInstance(NS_BACKGROUNDFILESAVERSTREAMLISTENER_CONTRACTID,
                             &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSaver->SetObserver(this);
  if (NS_FAILED(rv)) {
    mSaver = nullptr;
    return rv;
  }

  rv = mSaver->SetTarget(mTempFile, false);
  NS_ENSURE_SUCCESS(rv, rv);

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
  mIsFileChannel = fileChan != nullptr;

  
  if (aChannel) {
    aChannel->GetContentLength(&mContentLength);
  }

  nsCOMPtr<nsIPropertyBag2> props(do_QueryInterface(request, &rv));
  
  if (props) {
    bool tmp = false;
    props->GetPropertyAsBool(NS_LITERAL_STRING("docshell.newWindowTarget"),
                             &tmp);
    mShouldCloseWindow = tmp;
  }

  
  if (aChannel)
  {
    aChannel->GetURI(getter_AddRefs(mSourceUrl));
  }

  
  RetargetLoadNotifications(request);

  
  if (mOriginalChannel) {
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mOriginalChannel));
    if (httpChannel) {
      nsAutoCString refreshHeader;
      httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("refresh"),
                                     refreshHeader);
      if (!refreshHeader.IsEmpty()) {
        mShouldCloseWindow = false;
      }
    }
  }

  
  
  MaybeCloseWindow();

  
  
  
  
  
  
  nsCOMPtr<nsIEncodedChannel> encChannel = do_QueryInterface( aChannel );
  if (encChannel) 
  {
    
    bool applyConversion = true;

    nsCOMPtr<nsIURL> sourceURL(do_QueryInterface(mSourceUrl));
    if (sourceURL)
    {
      nsAutoCString extension;
      sourceURL->GetFileExtension(extension);
      if (!extension.IsEmpty())
      {
        nsCOMPtr<nsIUTF8StringEnumerator> encEnum;
        encChannel->GetContentEncodings(getter_AddRefs(encEnum));
        if (encEnum)
        {
          bool hasMore;
          rv = encEnum->HasMore(&hasMore);
          if (NS_SUCCEEDED(rv) && hasMore)
          {
            nsAutoCString encType;
            rv = encEnum->GetNext(encType);
            if (NS_SUCCEEDED(rv) && !encType.IsEmpty())
            {
              mExtProtSvc->ApplyDecodingForExtension(extension, encType,
                                                     &applyConversion);
            }
          }
        }
      }    
    }

    encChannel->SetApplyConversion( applyConversion );
  }

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content)
     return NS_OK;

  rv = SetUpTempFile(aChannel);
  if (NS_FAILED(rv)) {
    mCanceled = true;
    request->Cancel(rv);
    nsAutoString path;
    if (mTempFile)
      mTempFile->GetPath(path);
    SendStatusChange(kWriteError, rv, request, path);
    return NS_OK;
  }

  
  nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aChannel);
  if (httpInternal) {
    httpInternal->SetChannelIsForDownload(true);
  }

  
  

  
  
  
  
  
  
  

  
  

  bool alwaysAsk = true;
  mMimeInfo->GetAlwaysAskBeforeHandling(&alwaysAsk);
  if (alwaysAsk)
  {
    
    
    
    
    

    bool mimeTypeIsInDatastore = false;
    nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
    if (handlerSvc)
      handlerSvc->Exists(mMimeInfo, &mimeTypeIsInDatastore);
    if (!handlerSvc || !mimeTypeIsInDatastore)
    {
      nsAutoCString MIMEType;
      mMimeInfo->GetMIMEType(MIMEType);

      if (!GetNeverAskFlagFromPref(NEVER_ASK_FOR_SAVE_TO_DISK_PREF, MIMEType.get()))
      {
        
        alwaysAsk = false;
        
        mMimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);
      }
      else if (!GetNeverAskFlagFromPref(NEVER_ASK_FOR_OPEN_FILE_PREF, MIMEType.get()))
      {
        
        alwaysAsk = false;
      }
    }
  }

  int32_t action = nsIMIMEInfo::saveToDisk;
  mMimeInfo->GetPreferredAction( &action );

  
  if (!alwaysAsk && mReason != nsIHelperAppLauncherDialog::REASON_CANTHANDLE) {
    
    
    alwaysAsk = (action != nsIMIMEInfo::saveToDisk);
  }

  
  
  if (mForceSave) {
    alwaysAsk = false;
    action = nsIMIMEInfo::saveToDisk;
  }
  
  if (alwaysAsk)
  {
    
    
    mReceivedDispositionInfo = false; 
    mKeepRequestAlive = true;

    
    mDialog = do_CreateInstance( NS_HELPERAPPLAUNCHERDLG_CONTRACTID, &rv );
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    rv = mDialog->Show( this, mWindowContext, mReason );

    
  }
  else
  {
    mReceivedDispositionInfo = true; 

    
#ifdef XP_WIN
    








    nsCOMPtr<nsIHandlerApp> prefApp;
    mMimeInfo->GetPreferredApplicationHandler(getter_AddRefs(prefApp));
    if (action != nsIMIMEInfo::useHelperApp || !prefApp) {
      nsCOMPtr<nsIFile> fileToTest;
      GetTargetFile(getter_AddRefs(fileToTest));
      if (fileToTest) {
        bool isExecutable;
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
        rv = LaunchWithApplication(nullptr, false);
    }
    else 
    {
        rv = SaveToDisk(nullptr, false);
    }
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
          
#if defined(ANDROID)
          
          msgId.AssignLiteral("accessErrorSD");
#else
          msgId.AssignLiteral("accessError");
#endif
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

    
    nsCOMPtr<nsIStringBundleService> stringService =
        mozilla::services::GetStringBundleService();
    if (stringService)
    {
        nsCOMPtr<nsIStringBundle> bundle;
        if (NS_SUCCEEDED(stringService->CreateBundle("chrome://global/locale/nsWebBrowserPersist.properties", getter_AddRefs(bundle))))
        {
            nsXPIDLString msgText;
            const PRUnichar *strings[] = { path.get() };
            if(NS_SUCCEEDED(bundle->FormatStringFromName(msgId.get(), strings, 1, getter_Copies(msgText))))
            {
              if (mWebProgressListener)
              {
                
                mWebProgressListener->OnStatusChange(nullptr, (type == kReadError) ? aRequest : nullptr, rv, msgText);
              }
              else
              if (XRE_GetProcessType() == GeckoProcessType_Default) {
                
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

NS_IMETHODIMP
nsExternalAppHandler::OnDataAvailable(nsIRequest *request, nsISupports * aCtxt,
                                      nsIInputStream * inStr,
                                      uint64_t sourceOffset, uint32_t count)
{
  nsresult rv = NS_OK;
  
  if (mCanceled || !mSaver) 
    return request->Cancel(NS_BINDING_ABORTED);

  
  if (count > 0)
  {
    mProgress += count;

    nsCOMPtr<nsIStreamListener> saver = do_QueryInterface(mSaver);
    rv = saver->OnDataAvailable(request, aCtxt, inStr, sourceOffset, count);
    if (NS_SUCCEEDED(rv))
    {
      
      if (mWebProgressListener)
      {
        mWebProgressListener->OnProgressChange64(nullptr, request, mProgress,
                                                 mContentLength, mProgress,
                                                 mContentLength);
      }
    }
    else
    {
      
      nsAutoString tempFilePath;
      if (mTempFile)
        mTempFile->GetPath(tempFilePath);
      SendStatusChange(kReadError, rv, request, tempFilePath);

      
      Cancel(rv);
    }
  }
  return rv;
}

NS_IMETHODIMP nsExternalAppHandler::OnStopRequest(nsIRequest *request, nsISupports *aCtxt, 
                                                  nsresult aStatus)
{
  mStopRequestIssued = true;

  if (!mKeepRequestAlive)
    mRequest = nullptr;

  
  if (!mCanceled && NS_FAILED(aStatus))
  {
    
    nsAutoString tempFilePath;
    if (mTempFile)
      mTempFile->GetPath(tempFilePath);
    SendStatusChange( kReadError, aStatus, request, tempFilePath );

    Cancel(aStatus);
  }

  
  if (mCanceled || !mSaver)
    return NS_OK;

  return mSaver->Finish(NS_OK);
}

NS_IMETHODIMP
nsExternalAppHandler::OnTargetChange(nsIBackgroundFileSaver *aSaver,
                                     nsIFile *aTarget)
{
  return NS_OK;
}

NS_IMETHODIMP
nsExternalAppHandler::OnSaveComplete(nsIBackgroundFileSaver *aSaver,
                                     nsresult aStatus)
{
  
  mSaver = nullptr;

  if (mCanceled)
    return NS_OK;

  if (NS_FAILED(aStatus)) {
    nsAutoString path;
    mTempFile->GetPath(path);
    SendStatusChange(kWriteError, aStatus, nullptr, path);
    if (!mCanceled)
      Cancel(aStatus);
    return NS_OK;
  }

  
  ExecuteDesiredAction();

  
  
  
  mWebProgressListener = nullptr;

  return NS_OK;
}

nsresult nsExternalAppHandler::ExecuteDesiredAction()
{
  nsresult rv = NS_OK;
  if (mProgressListenerInitialized && !mCanceled)
  {
    rv = MoveFile(mFinalFileDestination);
    if (NS_SUCCEEDED(rv))
    {
      nsHandlerInfoAction action = nsIMIMEInfo::saveToDisk;
      mMimeInfo->GetPreferredAction(&action);
      if (action == nsIMIMEInfo::useHelperApp ||
          action == nsIMIMEInfo::useSystemDefault)
      {
        rv = OpenWithApplication();
      }
      else if(action == nsIMIMEInfo::saveToDisk)
      {
        mExtProtSvc->FixFilePermissions(mFinalFileDestination);
      }
    }

    
    
    
    if(mWebProgressListener)
    {
      if (!mCanceled)
      {
        mWebProgressListener->OnProgressChange64(nullptr, nullptr, mProgress, mContentLength, mProgress, mContentLength);
      }
      mWebProgressListener->OnStateChange(nullptr, nullptr,
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

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(mRequest);

  rv = aTransfer->Init(mSourceUrl, target, EmptyString(),
                       mMimeInfo, mTimeDownloadStarted, mTempFile, this,
                       channel && NS_UsePrivateBrowsing(channel));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIDownloadHistory> dh(do_GetService(NS_DOWNLOADHISTORY_CONTRACTID));
  if (dh) {
    nsCOMPtr<nsIURI> referrer;
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(mRequest);
    if (channel) {
      NS_GetReferrerFromChannel(channel, getter_AddRefs(referrer));
    }

    if (channel && !NS_UsePrivateBrowsing(channel)) {
      dh->AddDownload(mSourceUrl, referrer, mTimeDownloadStarted, target);
    }
  }

  return rv;
}

nsresult nsExternalAppHandler::CreateProgressListener()
{
  
  
  
  
  mDialog = nullptr;
  nsresult rv;
  
  nsCOMPtr<nsITransfer> tr = do_CreateInstance(NS_TRANSFER_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
    InitializeDownload(tr);

  if (tr)
    tr->OnStateChange(nullptr, mRequest, nsIWebProgressListener::STATE_START |
      nsIWebProgressListener::STATE_IS_REQUEST |
      nsIWebProgressListener::STATE_IS_NETWORK, NS_OK);

  
  
  
  
  
  
  SetWebProgressListener(tr);

  mRequest = nullptr;

  return rv;
}

nsresult nsExternalAppHandler::SaveDestinationAvailable(nsIFile * aFile)
{
  if (aFile)
    ContinueSave(aFile);
  else
    Cancel(NS_BINDING_ABORTED);

  return NS_OK;
}

void nsExternalAppHandler::RequestSaveDestination(const nsAFlatString &aDefaultFile, const nsAFlatString &aFileExtension)
{
  
  
  
  nsresult rv = NS_OK;
  if (!mDialog)
  {
    
    mDialog = do_CreateInstance( NS_HELPERAPPLAUNCHERDLG_CONTRACTID, &rv );
    if (rv != NS_OK) {
      Cancel(NS_BINDING_ABORTED);
      return;
    }
  }

  
  

  
  
  
  
  
  nsIFile* fileToUse;
  nsRefPtr<nsExternalAppHandler> kungFuDeathGrip(this);
  nsCOMPtr<nsIHelperAppLauncherDialog> dlg(mDialog);
  rv = mDialog->PromptForSaveToFile(this,
                                    mWindowContext,
                                    aDefaultFile.get(),
                                    aFileExtension.get(),
                                    mForceSave, &fileToUse);

  if (rv == NS_ERROR_NOT_AVAILABLE) {
    
    rv = mDialog->PromptForSaveToFileAsync(this, 
                                           mWindowContext,
                                           aDefaultFile.get(),
                                           aFileExtension.get(),
                                           mForceSave);
  } else {
    SaveDestinationAvailable(rv == NS_OK ? fileToUse : nullptr);
  }
}

nsresult nsExternalAppHandler::MoveFile(nsIFile * aNewFileLocation)
{
  nsresult rv = NS_OK;
  NS_ASSERTION(mStopRequestIssued, "uhoh, how did we get here if we aren't done getting data?");
 
  
  if (mStopRequestIssued && aNewFileLocation)
  {
    
    
    
    
    bool equalToTempFile = false;
    bool filetoUseAlreadyExists = false;
    aNewFileLocation->Equals(mTempFile, &equalToTempFile);
    aNewFileLocation->Exists(&filetoUseAlreadyExists);
    if (filetoUseAlreadyExists && !equalToTempFile)
      aNewFileLocation->Remove(false);

     
     nsAutoString fileName;
     aNewFileLocation->GetLeafName(fileName);
     nsCOMPtr<nsIFile> directoryLocation;
     rv = aNewFileLocation->GetParent(getter_AddRefs(directoryLocation));
     if (directoryLocation)
     {
       rv = mTempFile->MoveTo(directoryLocation, fileName);
     }
     if (NS_FAILED(rv))
     {
       
       nsAutoString path;
       aNewFileLocation->GetPath(path);
       SendStatusChange(kWriteError, rv, nullptr, path);
       Cancel(rv); 
     }
#if defined(XP_OS2)
     else
     {
       
       nsCOMPtr<nsILocalFileOS2> localFileOS2 = do_QueryInterface(aNewFileLocation);
       if (localFileOS2)
       {
         nsAutoCString url;
         mSourceUrl->GetSpec(url);
         localFileOS2->SetFileSource(url);
       }
     }
#endif
  }

  return rv;
}









NS_IMETHODIMP nsExternalAppHandler::SaveToDisk(nsIFile * aNewFileLocation, bool aRememberThisPreference)
{
  if (mCanceled)
    return NS_OK;

  mMimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);

  
  mReceivedDispositionInfo = true;

  if (!aNewFileLocation) {
    if (mSuggestedFileName.IsEmpty())
      RequestSaveDestination(mTempLeafName, mTempFileExtension);
    else
    {
      nsAutoString fileExt;
      int32_t pos = mSuggestedFileName.RFindChar('.');
      if (pos >= 0)
        mSuggestedFileName.Right(fileExt, mSuggestedFileName.Length() - pos);
      if (fileExt.IsEmpty())
        fileExt = mTempFileExtension;

      RequestSaveDestination(mSuggestedFileName, fileExt);
    }
  } else {
    ContinueSave(aNewFileLocation);
  }

  return NS_OK;
}
nsresult nsExternalAppHandler::ContinueSave(nsIFile * aNewFileLocation)
{
  NS_PRECONDITION(aNewFileLocation, "Must be called with a non-null file");

  nsresult rv = NS_OK;
  nsCOMPtr<nsIFile> fileToUse = do_QueryInterface(aNewFileLocation);
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

      if (mSaver)
      {
        rv = mSaver->SetTarget(movedFile, true);
        if (NS_FAILED(rv)) {
          nsAutoString path;
          mTempFile->GetPath(path);
          SendStatusChange(kWriteError, rv, nullptr, path);
          Cancel(rv);
          return NS_OK;
        }
      }

      mTempFile = movedFile;
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

    
    
    
    
    bool deleteTempFileOnExit =
      Preferences::GetBool("browser.helperApps.deleteTempFileOnExit",
#if !defined(XP_MACOSX)
                           true);
#else
                           false);
#endif


    NS_ASSERTION(mRequest, "This should never be called with a null request");
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(mRequest);
    bool inPrivateBrowsing = channel && NS_UsePrivateBrowsing(channel);

    
    
    if (deleteTempFileOnExit || inPrivateBrowsing)
      mFinalFileDestination->SetPermissions(0400);

    rv = mMimeInfo->LaunchWithFile(mFinalFileDestination);
    if (NS_FAILED(rv))
    {
      
      nsAutoString path;
      mFinalFileDestination->GetPath(path);
      SendStatusChange(kLaunchError, rv, nullptr, path);
      Cancel(rv); 
    }
    
    
    else if (deleteTempFileOnExit) {
      mExtProtSvc->DeleteTemporaryFileOnExit(mFinalFileDestination);
    }
    else if (inPrivateBrowsing) {
      mExtProtSvc->DeleteTemporaryPrivateFileWhenPossible(mFinalFileDestination);
    }
  }

  return rv;
}






NS_IMETHODIMP nsExternalAppHandler::LaunchWithApplication(nsIFile * aApplication, bool aRememberThisPreference)
{
  if (mCanceled)
    return NS_OK;

  
  ProcessAnyRefreshTags(); 
  
  mReceivedDispositionInfo = true; 
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
    
    SendStatusChange(kLaunchError, rv, nullptr, path);
    return rv;
  }

  
  
  
  
  

  nsCOMPtr<nsIFile> fileToUse;
  (void) GetDownloadDirectory(getter_AddRefs(fileToUse));

  if (mSuggestedFileName.IsEmpty())
  {
    
    mSuggestedFileName = mTempLeafName;
  }

#ifdef XP_WIN
  fileToUse->Append(mSuggestedFileName + mTempFileExtension);
#else
  fileToUse->Append(mSuggestedFileName);  
#endif

  nsresult rv = fileToUse->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  if(NS_SUCCEEDED(rv))
  {
    mFinalFileDestination = do_QueryInterface(fileToUse);
    
    if (!mProgressListenerInitialized)
      CreateProgressListener();
  }
  else
  {
    
    
    
    nsAutoString path;
    mTempFile->GetPath(path);
    SendStatusChange(kWriteError, rv, nullptr, path);
    Cancel(rv);
  }
  return rv;
}

NS_IMETHODIMP nsExternalAppHandler::Cancel(nsresult aReason)
{
  NS_ENSURE_ARG(NS_FAILED(aReason));
  

  mCanceled = true;
  if (mSaver)
    mSaver->Finish(aReason);

  
  
  mDialog = nullptr;

  mRequest = nullptr;

  
  
  mWebProgressListener = nullptr;

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
     mOriginalChannel = nullptr;
   }
}

bool nsExternalAppHandler::GetNeverAskFlagFromPref(const char * prefName, const char * aContentType)
{
  
  nsAdoptingCString prefCString = Preferences::GetCString(prefName);
  if (prefCString.IsEmpty()) {
    
    return true;
  }

  NS_UnescapeURL(prefCString);
  nsACString::const_iterator start, end;
  prefCString.BeginReading(start);
  prefCString.EndReading(end);
  return !CaseInsensitiveFindInReadable(nsDependentCString(aContentType),
                                        start, end);
}

nsresult nsExternalAppHandler::MaybeCloseWindow()
{
  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mWindowContext);
  NS_ENSURE_STATE(window);

  if (mShouldCloseWindow) {
    
    
    nsCOMPtr<nsIDOMWindow> opener;
    window->GetOpener(getter_AddRefs(opener));

    bool isClosed;
    if (opener && NS_SUCCEEDED(opener->GetClosed(&isClosed)) && !isClosed) {
      mWindowContext = do_GetInterface(opener);

      
      
      NS_ASSERTION(!mTimer, "mTimer was already initialized once!");
      mTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (!mTimer) {
        return NS_ERROR_FAILURE;
      }

      mTimer->InitWithCallback(this, 0, nsITimer::TYPE_ONE_SHOT);
      mWindowToClose = window;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsExternalAppHandler::Notify(nsITimer* timer)
{
  NS_ASSERTION(mWindowToClose, "No window to close after timer fired");

  mWindowToClose->Close();
  mWindowToClose = nullptr;
  mTimer = nullptr;

  return NS_OK;
}






NS_IMETHODIMP nsExternalHelperAppService::GetFromTypeAndExtension(const nsACString& aMIMEType, const nsACString& aFileExt, nsIMIMEInfo **_retval) 
{
  NS_PRECONDITION(!aMIMEType.IsEmpty() ||
                  !aFileExt.IsEmpty(), 
                  "Give me something to work with");
  LOG(("Getting mimeinfo from type '%s' ext '%s'\n",
        PromiseFlatCString(aMIMEType).get(), PromiseFlatCString(aFileExt).get()));

  *_retval = nullptr;

  
  nsAutoCString typeToUse(aMIMEType);
  if (typeToUse.IsEmpty()) {
    nsresult rv = GetTypeFromExtension(aFileExt, typeToUse);
    if (NS_FAILED(rv))
      return NS_ERROR_NOT_AVAILABLE;
  }

  
  ToLowerCase(typeToUse);

  
  bool found;
  *_retval = GetMIMEInfoFromOS(typeToUse, aFileExt, &found).get();
  LOG(("OS gave back 0x%p - found: %i\n", *_retval, found));
  
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  nsresult rv;
  nsCOMPtr<nsIHandlerService> handlerSvc = do_GetService(NS_HANDLERSERVICE_CONTRACTID);
  if (handlerSvc) {
    bool hasHandler = false;
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
        nsAutoCString overrideType;
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
      
      nsAutoCString desc(aFileExt);
      desc.Append(" File");
      (*_retval)->SetDescription(NS_ConvertASCIItoUTF16(desc));
      LOG(("Falling back to 'File' file description\n"));
    }
  }

  
  
  if (!aFileExt.IsEmpty()) {
    bool matches = false;
    (*_retval)->ExtensionExists(aFileExt, &matches);
    LOG(("Extension '%s' matches mime info: %i\n", PromiseFlatCString(aFileExt).get(), matches));
    if (matches)
      (*_retval)->SetPrimaryExtension(aFileExt);
  }

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsAutoCString type;
    (*_retval)->GetMIMEType(type);

    nsAutoCString ext;
    (*_retval)->GetPrimaryExtension(ext);
    LOG(("MIME Info Summary: Type '%s', Primary Ext '%s'\n", type.get(), ext.get()));
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP nsExternalHelperAppService::GetTypeFromExtension(const nsACString& aFileExt, nsACString& aContentType) 
{
  
  
  
  
  
  
  

  nsresult rv = NS_OK;
  
  for (size_t i = 0; i < ArrayLength(defaultMimeEntries); i++)
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

  
  bool found = false;
  nsCOMPtr<nsIMIMEInfo> mi = GetMIMEInfoFromOS(EmptyCString(), aFileExt, &found);
  if (mi && found)
    return mi->GetMIMEType(aContentType);

  
  found = GetTypeFromExtras(aFileExt, aContentType);
  if (found)
    return NS_OK;

  const nsCString& flatExt = PromiseFlatCString(aFileExt);
  
  const char* mimeType;
  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv));
  nsPluginHost* pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if (NS_SUCCEEDED(rv)) {
    if (NS_SUCCEEDED(pluginHost->IsPluginEnabledForExtension(flatExt.get(), mimeType))) {
      aContentType = mimeType;
      return NS_OK;
    }
  }
  
  rv = NS_OK;
  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService("@mozilla.org/categorymanager;1"));
  if (catMan) {
    
    nsAutoCString lowercaseFileExt(aFileExt);
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
  NS_ENSURE_ARG_POINTER(aURI);
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
    nsAutoCString ext;
    rv = url->GetFileExtension(ext);
    if (NS_FAILED(rv))
      return rv;
    if (ext.IsEmpty())
      return NS_ERROR_NOT_AVAILABLE;

    UnescapeFragment(ext, url, ext);

    return GetTypeFromExtension(ext, aContentType);
  }
    
  
  nsAutoCString specStr;
  rv = aURI->GetSpec(specStr);
  if (NS_FAILED(rv))
    return rv;
  UnescapeFragment(specStr, aURI, specStr);

  
  int32_t extLoc = specStr.RFindChar('.');
  int32_t specLength = specStr.Length();
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
  NS_ENSURE_ARG_POINTER(aFile);
  nsresult rv;
  nsCOMPtr<nsIMIMEInfo> info;

  
  nsAutoString fileName;
  rv = aFile->GetLeafName(fileName);
  if (NS_FAILED(rv)) return rv;
 
  nsAutoCString fileExt;
  if (!fileName.IsEmpty())
  {
    int32_t len = fileName.Length(); 
    for (int32_t i = len; i >= 0; i--) 
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

  
  nsAutoCString MIMEType(aContentType);
  ToLowerCase(MIMEType);
  int32_t numEntries = ArrayLength(extraMimeEntries);
  for (int32_t index = 0; index < numEntries; index++)
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
  nsAutoCString type;
  bool found = GetTypeFromExtras(aExtension, type);
  if (!found)
    return NS_ERROR_NOT_AVAILABLE;
  return FillMIMEInfoForMimeTypeFromExtras(type, aMIMEInfo);
}

bool nsExternalHelperAppService::GetTypeFromExtras(const nsACString& aExtension, nsACString& aMIMEType)
{
  NS_ASSERTION(!aExtension.IsEmpty(), "Empty aExtension parameter!");

  
  nsDependentCString::const_iterator start, end, iter;
  int32_t numEntries = ArrayLength(extraMimeEntries);
  for (int32_t index = 0; index < numEntries; index++)
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
              return true;
          }
          if (iter != end) {
            ++iter;
          }
          start = iter;
      }
  }

  return false;
}
