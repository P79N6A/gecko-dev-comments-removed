









































#include "nscore.h"
#include "nsPluginHost.h"

#include <stdio.h>
#include "prio.h"
#include "prmem.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginStreamListener.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIUploadChannel.h"
#include "nsIByteRangeRequest.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIProtocolProxyService.h"
#include "nsIStreamConverterService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsISeekableStream.h"
#include "nsNetUtil.h"
#include "nsIProgressEventSink.h"
#include "nsIDocument.h"
#include "nsICachingChannel.h"
#include "nsHashtable.h"
#include "nsIProxyInfo.h"
#include "nsObsoleteModuleLoading.h"
#include "nsIComponentRegistrar.h"
#include "nsPluginLogging.h"
#include "nsIPrefBranch2.h"
#include "nsIScriptChannel.h"
#include "nsPrintfCString.h"
#include "nsIBlocklistService.h"
#include "nsVersionComparator.h"
#include "nsIPrivateBrowsingService.h"

#include "nsEnumeratorUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"


#include "nsIStringBundle.h"
#include "nsIWindowWatcher.h"
#include "nsPIDOMWindow.h"

#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIPrincipal.h"

#include "nsNetCID.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeType.h"
#include "nsMimeTypes.h"
#include "prprf.h"
#include "nsThreadUtils.h"
#include "nsIInputStreamTee.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"

#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsILocalFile.h"
#include "nsIFileChannel.h"

#include "nsPluginSafety.h"

#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"

#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsPluginDirServiceProvider.h"
#include "nsInt64.h"
#include "nsPluginError.h"

#include "nsUnicharUtils.h"
#include "nsPluginManifestLineReader.h"

#include "nsDefaultPlugin.h"
#include "nsWeakReference.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIWebNavigation.h"
#include "nsISupportsArray.h"
#include "nsIDocShell.h"
#include "nsPluginNativeWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"

#if defined(XP_WIN)
#include "windows.h"
#include "winbase.h"
#endif

#if defined(XP_UNIX) && defined(MOZ_WIDGET_GTK2) & defined(MOZ_X11)
#include <gdk/gdkx.h> 
#endif



#define NS_ITERATIVE_UNREF_LIST(type_, list_, mNext_)                \
  {                                                                  \
    while (list_) {                                                  \
      type_ temp = list_->mNext_;                                    \
      list_->mNext_ = nsnull;                                        \
      list_ = temp;                                                  \
    }                                                                \
  }



#define kPluginTmpDirName NS_LITERAL_CSTRING("plugtmp")














static const char *kPluginRegistryVersion = "0.11";

static const char *kMinimumRegistryVersion = "0.9";

static NS_DEFINE_IID(kIPluginTagInfoIID, NS_IPLUGINTAGINFO_IID);
static const char kDirectoryServiceContractID[] = "@mozilla.org/file/directory_service;1";


static const char kPluginsRootKey[] = "software/plugins";
static const char kPluginsNameKey[] = "name";
static const char kPluginsDescKey[] = "description";
static const char kPluginsFilenameKey[] = "filename";
static const char kPluginsFullpathKey[] = "fullpath";
static const char kPluginsModTimeKey[] = "lastModTimeStamp";
static const char kPluginsCanUnload[] = "canUnload";
static const char kPluginsVersionKey[] = "version";
static const char kPluginsMimeTypeKey[] = "mimetype";
static const char kPluginsMimeDescKey[] = "description";
static const char kPluginsMimeExtKey[] = "extension";

#define kPluginRegistryFilename NS_LITERAL_CSTRING("pluginreg.dat")

#ifdef PLUGIN_LOGGING
PRLogModuleInfo* nsPluginLogging::gNPNLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gNPPLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gPluginLog = nsnull;
#endif

#define BRAND_PROPERTIES_URL "chrome://branding/locale/brand.properties"
#define PLUGIN_PROPERTIES_URL "chrome://global/locale/downloadProgress.properties"


#define NS_PREF_MAX_NUM_CACHED_PLUGINS "browser.plugins.max_num_cached_plugins"
#define DEFAULT_NUMBER_OF_STOPPED_PLUGINS 10

#define MAGIC_REQUEST_CONTEXT 0x01020304

nsresult PostPluginUnloadEvent(PRLibrary * aLibrary);

static nsPluginInstanceTagList *gActivePluginList;

#ifdef CALL_SAFETY_ON
PRBool gSkipPluginSafeCalls = PR_FALSE;
#endif

nsIFile *nsPluginHost::sPluginTempDir;
nsPluginHost *nsPluginHost::sInst;


static
PRBool ReadSectionHeader(nsPluginManifestLineReader& reader, const char *token)
{
  do {
    if (*reader.LinePtr() == '[') {
      char* p = reader.LinePtr() + (reader.LineLength() - 1);
      if (*p != ']')
        break;
      *p = 0;

      char* values[1];
      if (1 != reader.ParseLine(values, 1))
        break;
      
      if (PL_strcmp(values[0]+1, token)) {
        break; 
      }
      return PR_TRUE;
    }
  } while (reader.NextLine());
  return PR_FALSE;
}




class nsPluginDocReframeEvent: public nsRunnable {
public:
  nsPluginDocReframeEvent(nsISupportsArray* aDocs) { mDocs = aDocs; }

  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsISupportsArray> mDocs;
};

NS_IMETHODIMP nsPluginDocReframeEvent::Run() {
  NS_ENSURE_TRUE(mDocs, NS_ERROR_FAILURE);

  PRUint32 c;
  mDocs->Count(&c);

  
  
  for (PRUint32 i = 0; i < c; i++) {
    nsCOMPtr<nsIDocument> doc (do_QueryElementAt(mDocs, i));
    if (doc) {
      nsIPresShell *shell = doc->GetPrimaryShell();

      
      if (shell) {
        








        shell->ReconstructFrames(); 
      } else {  

        NS_NOTREACHED("all plugins should have a pres shell!");

      }
    }
  }

  return mDocs->Clear();
}

nsPluginInstanceTag::nsPluginInstanceTag(nsPluginTag* aPluginTag,
                               nsIPluginInstance* aInstance,
                               const char * url,
                               PRBool aDefaultPlugin)
{
  mNext = nsnull;
  mPluginTag = aPluginTag;

  mURL = PL_strdup(url);
  mInstance = aInstance;
  if (aInstance)
    NS_ADDREF(aInstance);
  mXPConnected = PR_FALSE;
  mDefaultPlugin = aDefaultPlugin;
  mStopped = PR_FALSE;
  mllStopTime = LL_ZERO;
}

nsPluginInstanceTag::~nsPluginInstanceTag()
{
  mPluginTag = nsnull;
  if (mInstance) {
    nsCOMPtr<nsIPluginInstanceOwner> owner;
    mInstance->GetOwner(getter_AddRefs(owner));
    if (owner)
      owner->SetInstance(nsnull);
    mInstance->InvalidateOwner();

    NS_RELEASE(mInstance);
  }
  PL_strfree(mURL);
}

void nsPluginInstanceTag::setStopped(PRBool stopped)
{
  mStopped = stopped;
  if (mStopped) 
    mllStopTime = PR_Now();
  else
    mllStopTime = LL_ZERO;
}

nsPluginInstanceTagList::nsPluginInstanceTagList()
{
  mFirst = nsnull;
  mLast = nsnull;
  mCount = 0;
}

nsPluginInstanceTagList::~nsPluginInstanceTagList()
{
  if (!mFirst)
    return;
  shutdown();
}

void nsPluginInstanceTagList::shutdown()
{
  if (!mFirst)
    return;

  for (nsPluginInstanceTag * plugin = mFirst; plugin != nsnull;) {
    nsPluginInstanceTag * next = plugin->mNext;
    remove(plugin);
    plugin = next;
  }
  mFirst = nsnull;
  mLast = nsnull;
}

PRInt32 nsPluginInstanceTagList::add(nsPluginInstanceTag * plugin)
{
  if (!mFirst) {
    mFirst = plugin;
    mLast = plugin;
    mFirst->mNext = nsnull;
  }
  else {
    mLast->mNext = plugin;
    mLast = plugin;
  }
  mLast->mNext = nsnull;
  mCount++;
  return mCount;
}

PRBool nsPluginInstanceTagList::IsLastInstance(nsPluginInstanceTag * plugin)
{
  if (!plugin)
    return PR_FALSE;

  if (!plugin->mPluginTag)
    return PR_FALSE;

  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if ((p->mPluginTag == plugin->mPluginTag) && (p != plugin))
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool nsPluginInstanceTagList::remove(nsPluginInstanceTag * plugin)
{
  if (!mFirst)
    return PR_FALSE;

  nsPluginInstanceTag * prev = nsnull;
  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (p == plugin) {
      PRBool lastInstance = IsLastInstance(p);

      if (p == mFirst)
        mFirst = p->mNext;
      else
        prev->mNext = p->mNext;

      if (prev && !prev->mNext)
        mLast = prev;

      if (lastInstance) {
        nsRefPtr<nsPluginTag> pluginTag = p->mPluginTag;

        delete p;

        if (pluginTag) {
          nsresult rv;
          nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
          NS_ENSURE_SUCCESS(rv, rv);

          PRBool unloadPluginsASAP = PR_FALSE;
          rv = pref->GetBoolPref("plugins.unloadASAP", &unloadPluginsASAP);
          if (NS_SUCCEEDED(rv) && unloadPluginsASAP)
            pluginTag->TryUnloadPlugin();
        } 
        else
          NS_ASSERTION(pluginTag, "pluginTag was not set, plugin not shutdown");
      }
      else
        delete p;

      mCount--;
      return PR_TRUE;
    }
    prev = p;
  }
  return PR_FALSE;
}





void nsPluginInstanceTagList::stopRunning(nsISupportsArray* aReloadDocs,
                                     nsPluginTag* aPluginTag)
{
  if (!mFirst)
    return;

  PRBool doCallSetWindowAfterDestroy = PR_FALSE;

  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (!p->mStopped && p->mInstance &&
       (!aPluginTag || aPluginTag == p->mPluginTag)) {
      
      
      p->mInstance->GetValue(nsPluginInstanceVariable_CallSetWindowAfterDestroyBool,
                             (void *) &doCallSetWindowAfterDestroy);
      if (doCallSetWindowAfterDestroy) {
        p->mInstance->Stop();
        p->mInstance->SetWindow(nsnull);
      }
      else {
        p->mInstance->SetWindow(nsnull);
        p->mInstance->Stop();
      }
      doCallSetWindowAfterDestroy = PR_FALSE;
      p->setStopped(PR_TRUE);

      
      
      
      if (aReloadDocs && p->mInstance) {
        nsCOMPtr<nsIPluginInstanceOwner> owner;
        p->mInstance->GetOwner(getter_AddRefs(owner));
        if (owner) {
          nsCOMPtr<nsIDocument> doc;
          owner->GetDocument(getter_AddRefs(doc));
          if (doc && aReloadDocs->IndexOf(doc) == -1)  
            aReloadDocs->AppendElement(doc);
        }
      }
    }
  }
}

void nsPluginInstanceTagList::removeAllStopped()
{
  if (!mFirst)
    return;

  nsPluginInstanceTag * next = nsnull;

  for (nsPluginInstanceTag * p = mFirst; p != nsnull;) {
    next = p->mNext;

    if (p->mStopped)
      remove(p);

    p = next;
  }
  return;
}

nsPluginInstanceTag * nsPluginInstanceTagList::find(nsIPluginInstance* instance)
{
  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (p->mInstance == instance) {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(!p->mStopped || doCache, "This plugin is not supposed to be cached!");
#endif
      return p;
    }
  }
  return nsnull;
}

nsPluginInstanceTag * nsPluginInstanceTagList::find(const char * mimetype)
{
  PRBool defaultplugin = (PL_strcmp(mimetype, "*") == 0);

  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    
    
    if (defaultplugin && p->mDefaultPlugin)
      return p;

    if (!p->mInstance)
      continue;

    const char* mt;
    nsresult rv = p->mInstance->GetMIMEType(&mt);
    if (NS_FAILED(rv))
      continue;

    if (PL_strcasecmp(mt, mimetype) == 0) {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(!p->mStopped || doCache, "This plugin is not supposed to be cached!");
#endif
      return p;
    }
  }
  return nsnull;
}

nsPluginInstanceTag * nsPluginInstanceTagList::findStopped(const char * url)
{
  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (!PL_strcmp(url, p->mURL) && p->mStopped) {
#ifdef NS_DEBUG
      PRBool doCache = PR_TRUE;
      p->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
      NS_ASSERTION(doCache, "This plugin is not supposed to be cached!");
#endif
       return p;
    }
  }
  return nsnull;
}

PRUint32 nsPluginInstanceTagList::getStoppedCount()
{
  PRUint32 stoppedCount = 0;
  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (p->mStopped)
      stoppedCount++;
  }
  return stoppedCount;
}

nsPluginInstanceTag * nsPluginInstanceTagList::findOldestStopped()
{
  nsPluginInstanceTag * res = nsnull;
  PRInt64 llTime = LL_MAXINT;
  for (nsPluginInstanceTag * p = mFirst; p != nsnull; p = p->mNext) {
    if (!p->mStopped)
      continue;

    if (LL_CMP(p->mllStopTime, <, llTime)) {
      llTime = p->mllStopTime;
      res = p;
    }
  }

#ifdef NS_DEBUG
  if (res) {
    PRBool doCache = PR_TRUE;
    res->mInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);
    NS_ASSERTION(doCache, "This plugin is not supposed to be cached!");
  }
#endif

  return res;
}

inline char* new_str(const char* str)
{
  if (str == nsnull)
    return nsnull;

  char* result = new char[strlen(str) + 1];
  if (result != nsnull)
    return strcpy(result, str);
  return result;
}

nsPluginTag::nsPluginTag(nsPluginTag* aPluginTag)
  : mPluginHost(nsnull),
    mName(aPluginTag->mName),
    mDescription(aPluginTag->mDescription),
    mVariants(aPluginTag->mVariants),
    mMimeTypeArray(nsnull),
    mMimeDescriptionArray(aPluginTag->mMimeDescriptionArray),
    mExtensionsArray(nsnull),
    mLibrary(nsnull),
    mEntryPoint(nsnull),
    mCanUnloadLibrary(PR_TRUE),
    mXPConnected(PR_FALSE),
    mIsJavaPlugin(aPluginTag->mIsJavaPlugin),
    mIsNPRuntimeEnabledJavaPlugin(aPluginTag->mIsNPRuntimeEnabledJavaPlugin),
    mFileName(aPluginTag->mFileName),
    mFullPath(aPluginTag->mFullPath),
    mVersion(aPluginTag->mVersion),
    mLastModifiedTime(0),
    mFlags(NS_PLUGIN_FLAG_ENABLED)
{
  if (aPluginTag->mMimeTypeArray != nsnull) {
    mMimeTypeArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mMimeTypeArray[i] = new_str(aPluginTag->mMimeTypeArray[i]);
  }

  if (aPluginTag->mExtensionsArray != nsnull) {
    mExtensionsArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mExtensionsArray[i] = new_str(aPluginTag->mExtensionsArray[i]);
  }
}

nsPluginTag::nsPluginTag(nsPluginInfo* aPluginInfo)
  : mPluginHost(nsnull),
    mName(aPluginInfo->fName),
    mDescription(aPluginInfo->fDescription),
    mVariants(aPluginInfo->fVariantCount),
    mMimeTypeArray(nsnull),
    mExtensionsArray(nsnull),
    mLibrary(nsnull),
    mEntryPoint(nsnull),
#ifdef XP_MACOSX
    mCanUnloadLibrary(!aPluginInfo->fBundle),
#else
    mCanUnloadLibrary(PR_TRUE),
#endif
    mXPConnected(PR_FALSE),
    mIsJavaPlugin(PR_FALSE),
    mIsNPRuntimeEnabledJavaPlugin(PR_FALSE),
    mFileName(aPluginInfo->fFileName),
    mFullPath(aPluginInfo->fFullPath),
    mVersion(aPluginInfo->fVersion),
    mLastModifiedTime(0),
    mFlags(NS_PLUGIN_FLAG_ENABLED)
{
  if (aPluginInfo->fMimeTypeArray != nsnull) {
    mMimeTypeArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++) {
      if (mIsJavaPlugin && aPluginInfo->fMimeTypeArray[i] &&
          strcmp(aPluginInfo->fMimeTypeArray[i],
                 "application/x-java-vm-npruntime") == 0) {
        mIsNPRuntimeEnabledJavaPlugin = PR_TRUE;

        
        
        mVariants = i;

        break;
      }

      mMimeTypeArray[i] = new_str(aPluginInfo->fMimeTypeArray[i]);
      if (nsPluginHost::IsJavaMIMEType(mMimeTypeArray[i]))
        mIsJavaPlugin = PR_TRUE;
    }
  }

  if (aPluginInfo->fMimeDescriptionArray != nsnull) {
    for (int i = 0; i < mVariants; i++) {
      
      
      
      
      char cur = '\0';
      char pre = '\0';
      char * p = PL_strrchr(aPluginInfo->fMimeDescriptionArray[i], '(');
      if (p && (p != aPluginInfo->fMimeDescriptionArray[i])) {
        if ((p - 1) && *(p - 1) == ' ') {
          pre = *(p - 1);
          *(p - 1) = '\0';
        } else {
          cur = *p;
          *p = '\0';
        }

      }
      mMimeDescriptionArray.AppendElement(
        aPluginInfo->fMimeDescriptionArray[i]);
      
      if (cur != '\0')
        *p = cur;
      if (pre != '\0')
        *(p - 1) = pre;
    }
  } else {
    mMimeDescriptionArray.SetLength(mVariants);
  }

  if (aPluginInfo->fExtensionArray != nsnull) {
    mExtensionsArray = new char*[mVariants];
    for (int i = 0; i < mVariants; i++)
      mExtensionsArray[i] = new_str(aPluginInfo->fExtensionArray[i]);
  }

  EnsureMembersAreUTF8();
}

nsPluginTag::nsPluginTag(const char* aName,
                         const char* aDescription,
                         const char* aFileName,
                         const char* aFullPath,
                         const char* aVersion,
                         const char* const* aMimeTypes,
                         const char* const* aMimeDescriptions,
                         const char* const* aExtensions,
                         PRInt32 aVariants,
                         PRInt64 aLastModifiedTime,
                         PRBool aCanUnload,
                         PRBool aArgsAreUTF8)
  : mPluginHost(nsnull),
    mName(aName),
    mDescription(aDescription),
    mVariants(aVariants),
    mMimeTypeArray(nsnull),
    mExtensionsArray(nsnull),
    mLibrary(nsnull),
    mEntryPoint(nsnull),
    mCanUnloadLibrary(aCanUnload),
    mXPConnected(PR_FALSE),
    mIsJavaPlugin(PR_FALSE),
    mIsNPRuntimeEnabledJavaPlugin(PR_FALSE),
    mFileName(aFileName),
    mFullPath(aFullPath),
    mVersion(aVersion),
    mLastModifiedTime(aLastModifiedTime),
    mFlags(0) 
{
  if (aVariants) {
    mMimeTypeArray        = new char*[mVariants];
    mExtensionsArray      = new char*[mVariants];

    for (PRInt32 i = 0; i < aVariants; ++i) {
      if (mIsJavaPlugin && aMimeTypes[i] &&
          strcmp(aMimeTypes[i], "application/x-java-vm-npruntime") == 0) {
        mIsNPRuntimeEnabledJavaPlugin = PR_TRUE;

        
        
        mVariants = i;

        break;
      }

      mMimeTypeArray[i]        = new_str(aMimeTypes[i]);
      mMimeDescriptionArray.AppendElement(aMimeDescriptions[i]);
      mExtensionsArray[i]      = new_str(aExtensions[i]);
      if (nsPluginHost::IsJavaMIMEType(mMimeTypeArray[i]))
        mIsJavaPlugin = PR_TRUE;
    }
  }

  if (!aArgsAreUTF8)
    EnsureMembersAreUTF8();
}

nsPluginTag::~nsPluginTag()
{
  NS_ASSERTION(!mNext, "Risk of exhausting the stack space, bug 486349");

  TryUnloadPlugin();

  
  
  if (mPluginHost) {
    RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
  }

  if (mMimeTypeArray) {
    for (int i = 0; i < mVariants; i++)
      delete[] mMimeTypeArray[i];

    delete[] (mMimeTypeArray);
    mMimeTypeArray = nsnull;
  }

  if (mExtensionsArray) {
    for (int i = 0; i < mVariants; i++)
      delete[] mExtensionsArray[i];

    delete[] (mExtensionsArray);
    mExtensionsArray = nsnull;
  }
}

NS_IMPL_ISUPPORTS1(nsPluginTag, nsIPluginTag)

static nsresult ConvertToUTF8(nsIUnicodeDecoder *aUnicodeDecoder,
                              nsAFlatCString& aString)
{
  PRInt32 numberOfBytes = aString.Length();
  PRInt32 outUnicodeLen;
  nsAutoString buffer;
  nsresult rv = aUnicodeDecoder->GetMaxLength(aString.get(), numberOfBytes,
                                              &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!EnsureStringLength(buffer, outUnicodeLen))
    return NS_ERROR_OUT_OF_MEMORY;
  rv = aUnicodeDecoder->Convert(aString.get(), &numberOfBytes,
                                buffer.BeginWriting(), &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);
  buffer.SetLength(outUnicodeLen);
  CopyUTF16toUTF8(buffer, aString);

  return NS_OK;
}

nsresult nsPluginTag::EnsureMembersAreUTF8()
{
  nsresult rv;

  nsCOMPtr<nsIPlatformCharset> pcs =
    do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIUnicodeDecoder> decoder;
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString charset;
  rv = pcs->GetCharset(kPlatformCharsetSel_FileName, charset);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!charset.LowerCaseEqualsLiteral("utf-8")) {
    rv = ccm->GetUnicodeDecoderRaw(charset.get(), getter_AddRefs(decoder));
    NS_ENSURE_SUCCESS(rv, rv);

    ConvertToUTF8(decoder, mFileName);
    ConvertToUTF8(decoder, mFullPath);
  }

  
  
  
  rv = pcs->GetCharset(kPlatformCharsetSel_PlainTextInFile, charset);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!charset.LowerCaseEqualsLiteral("utf-8")) {
    rv = ccm->GetUnicodeDecoderRaw(charset.get(), getter_AddRefs(decoder));
    NS_ENSURE_SUCCESS(rv, rv);

    ConvertToUTF8(decoder, mName);
    ConvertToUTF8(decoder, mDescription);
    for (PRUint32 i = 0; i < mMimeDescriptionArray.Length(); ++i) {
      ConvertToUTF8(decoder, mMimeDescriptionArray[i]);
    }
  }
  return NS_OK;
}

void nsPluginTag::SetHost(nsPluginHost * aHost)
{
  mPluginHost = aHost;
}

NS_IMETHODIMP
nsPluginTag::GetDescription(nsACString& aDescription)
{
  aDescription = mDescription;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::GetFilename(nsACString& aFileName)
{
  aFileName = mFileName;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::GetVersion(nsACString& aVersion)
{
  aVersion = mVersion;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::GetName(nsACString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::GetDisabled(PRBool* aDisabled)
{
  *aDisabled = !HasFlag(NS_PLUGIN_FLAG_ENABLED);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::SetDisabled(PRBool aDisabled)
{
  if (HasFlag(NS_PLUGIN_FLAG_ENABLED) == !aDisabled)
    return NS_OK;

  if (aDisabled)
    UnMark(NS_PLUGIN_FLAG_ENABLED);
  else
    Mark(NS_PLUGIN_FLAG_ENABLED);

  mPluginHost->UpdatePluginInfo(this);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::GetBlocklisted(PRBool* aBlocklisted)
{
  *aBlocklisted = HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::SetBlocklisted(PRBool aBlocklisted)
{
  if (HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED) == aBlocklisted)
    return NS_OK;

  if (aBlocklisted)
    Mark(NS_PLUGIN_FLAG_BLOCKLISTED);
  else
    UnMark(NS_PLUGIN_FLAG_BLOCKLISTED);

  mPluginHost->UpdatePluginInfo(nsnull);
  return NS_OK;
}


class nsPluginUnloadEvent : public nsRunnable {
public:
  nsPluginUnloadEvent(PRLibrary* aLibrary)
    : mLibrary(aLibrary)
  {}
 
  NS_DECL_NSIRUNNABLE
 
  PRLibrary* mLibrary;
};

NS_IMETHODIMP nsPluginUnloadEvent::Run()
{
  if (mLibrary) {
    
    NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(mLibrary), nsnull, nsnull);
  } else {
    NS_WARNING("missing library from nsPluginUnloadEvent");
  }
  return NS_OK;
}


nsresult PostPluginUnloadEvent(PRLibrary* aLibrary)
{
  nsCOMPtr<nsIRunnable> ev = new nsPluginUnloadEvent(aLibrary);
  if (ev && NS_SUCCEEDED(NS_DispatchToCurrentThread(ev)))
    return NS_OK;

  
  NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(aLibrary), nsnull, nsnull);

  return NS_ERROR_FAILURE;
}

void nsPluginTag::TryUnloadPlugin()
{
  if (mEntryPoint) {
    mEntryPoint->Shutdown();
    mEntryPoint->Release();
    mEntryPoint = nsnull;
  }

  
  if (mLibrary && mCanUnloadLibrary) {
    
    if (!mXPConnected) {
      
      PostPluginUnloadEvent(mLibrary);
    }
    else {
      
      if (mPluginHost)
        mPluginHost->AddUnusedLibrary(mLibrary);
    }
  }

  
  
  
  
  mLibrary = nsnull;
}

void nsPluginTag::Mark(PRUint32 mask)
{
  PRBool wasEnabled = IsEnabled();
  mFlags |= mask;
  
  if (mPluginHost && wasEnabled != IsEnabled()) {
    if (wasEnabled)
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
    else
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginRegister);
  }
}

void nsPluginTag::UnMark(PRUint32 mask)
{
  PRBool wasEnabled = IsEnabled();
  mFlags &= ~mask;
  
  if (mPluginHost && wasEnabled != IsEnabled()) {
    if (wasEnabled)
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
    else
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginRegister);
  }
}

PRBool nsPluginTag::HasFlag(PRUint32 flag)
{
  return (mFlags & flag) != 0;
}

PRUint32 nsPluginTag::Flags()
{
  return mFlags;
}

PRBool nsPluginTag::IsEnabled()
{
  return HasFlag(NS_PLUGIN_FLAG_ENABLED) && !HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED);
}

PRBool nsPluginTag::Equals(nsPluginTag *aPluginTag)
{
  NS_ENSURE_TRUE(aPluginTag, PR_FALSE);

  if ((!mName.Equals(aPluginTag->mName)) ||
      (!mDescription.Equals(aPluginTag->mDescription)) ||
      (mVariants != aPluginTag->mVariants))
    return PR_FALSE;

  if (mVariants && mMimeTypeArray && aPluginTag->mMimeTypeArray) {
    for (PRInt32 i = 0; i < mVariants; i++) {
      if (PL_strcmp(mMimeTypeArray[i], aPluginTag->mMimeTypeArray[i]) != 0)
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

class nsPluginStreamListenerPeer;

class nsPluginStreamInfo : public nsINPAPIPluginStreamInfo
{
public:
  nsPluginStreamInfo();
  virtual ~nsPluginStreamInfo();

  NS_DECL_ISUPPORTS

  
 
  NS_IMETHOD
  GetContentType(char **result);

  NS_IMETHOD
  IsSeekable(PRBool* result);

  NS_IMETHOD
  GetLength(PRUint32* result);

  NS_IMETHOD
  GetLastModified(PRUint32* result);

  NS_IMETHOD
  GetURL(const char** result);

  NS_IMETHOD
  RequestRead(nsByteRange* rangeList);

  NS_IMETHOD
  GetStreamOffset(PRInt32 *result);

  NS_IMETHOD
  SetStreamOffset(PRInt32 result);

  

  void
  SetContentType(const char* contentType);

  void
  SetSeekable(const PRBool seekable);

  void
  SetLength(const PRUint32 length);

  void
  SetLastModified(const PRUint32 modified);

  void
  SetURL(const char* url);

  void
  SetPluginInstance(nsIPluginInstance * aPluginInstance);

  void
  SetPluginStreamListenerPeer(nsPluginStreamListenerPeer * aPluginStreamListenerPeer);

  void
  MakeByteRangeString(nsByteRange* aRangeList, nsACString &string, PRInt32 *numRequests);

  PRBool
  UseExistingPluginCacheFile(nsPluginStreamInfo* psi);

  void
  SetStreamComplete(const PRBool complete);

  void
  SetRequest(nsIRequest *request)
  {
    mRequest = request;
  }

private:

  char* mContentType;
  char* mURL;
  PRBool mSeekable;
  PRUint32 mLength;
  PRUint32 mModified;
  nsIPluginInstance * mPluginInstance;
  nsPluginStreamListenerPeer * mPluginStreamListenerPeer;
  PRInt32 mStreamOffset;
  PRBool mStreamComplete;
};

class nsPluginStreamListenerPeer : public nsIStreamListener,
                                   public nsIProgressEventSink,
                                   public nsIHttpHeaderVisitor,
                                   public nsSupportsWeakReference
{
public:
  nsPluginStreamListenerPeer();
  virtual ~nsPluginStreamListenerPeer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIHTTPHEADERVISITOR

  
  nsresult Initialize(nsIURI *aURL,
                      nsIPluginInstance *aInstance,
                      nsIPluginStreamListener *aListener,
                      PRInt32 requestCount = 1);

  nsresult InitializeEmbedded(nsIURI *aURL,
                             nsIPluginInstance* aInstance,
                             nsIPluginInstanceOwner *aOwner = nsnull,
                             nsIPluginHost *aHost = nsnull);

  nsresult InitializeFullPage(nsIPluginInstance *aInstance);

  nsresult OnFileAvailable(nsIFile* aFile);

  nsresult ServeStreamAsFile(nsIRequest *request, nsISupports *ctxt);
  
  nsIPluginInstance *GetPluginInstance() { return mInstance; }

private:
  nsresult SetUpCache(nsIURI* aURL); 
  nsresult SetUpStreamListener(nsIRequest* request, nsIURI* aURL);
  nsresult SetupPluginCacheFile(nsIChannel* channel);

  nsIURI                  *mURL;
  nsIPluginInstanceOwner  *mOwner;
  nsIPluginInstance       *mInstance;
  nsIPluginStreamListener *mPStreamListener;
  nsRefPtr<nsPluginStreamInfo> mPluginStreamInfo;

  
  PRPackedBool            mRequestFailed;

  





  PRPackedBool      mStartBinding;
  PRPackedBool      mHaveFiredOnStartRequest;
  
  char                    *mMIMEType;
  PRUint32                mLength;
  nsPluginStreamType      mStreamType;
  nsIPluginHost           *mHost;

  
  
  nsIFile                 *mLocalCachedFile;
  nsCOMPtr<nsIOutputStream> mFileCacheOutputStream;
  nsHashtable             *mDataForwardToRequest;

public:
  PRBool                  mAbort;
  PRInt32                 mPendingRequests;
  nsWeakPtr               mWeakPtrChannelCallbacks;
  nsWeakPtr               mWeakPtrChannelLoadGroup;
};

class nsPluginByteRangeStreamListener : public nsIStreamListener {
public:
  nsPluginByteRangeStreamListener(nsIWeakReference* aWeakPtr);
  virtual ~nsPluginByteRangeStreamListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  nsCOMPtr<nsIStreamListener> mStreamConverter;
  nsWeakPtr mWeakPtrPluginStreamListenerPeer;
  PRBool mRemoveMagicNumber;
};

nsPluginStreamInfo::nsPluginStreamInfo()
{
  mPluginInstance = nsnull;
  mPluginStreamListenerPeer = nsnull;

  mContentType = nsnull;
  mURL = nsnull;
  mSeekable = PR_FALSE;
  mLength = 0;
  mModified = 0;
  mStreamOffset = 0;
  mStreamComplete = PR_FALSE;
}

nsPluginStreamInfo::~nsPluginStreamInfo()
{
  if (mContentType)
    PL_strfree(mContentType);
  if (mURL)
    PL_strfree(mURL);

  NS_IF_RELEASE(mPluginInstance);
}

NS_IMPL_ISUPPORTS2(nsPluginStreamInfo, nsIPluginStreamInfo,
                   nsINPAPIPluginStreamInfo)

NS_IMETHODIMP
nsPluginStreamInfo::GetContentType(char **result)
{
  *result = mContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::IsSeekable(PRBool* result)
{
  *result = mSeekable;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetLength(PRUint32* result)
{
  *result = mLength;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetLastModified(PRUint32* result)
{
  *result = mModified;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::GetURL(const char** result)
{
  *result = mURL;
  return NS_OK;
}

void
nsPluginStreamInfo::MakeByteRangeString(nsByteRange* aRangeList, nsACString &rangeRequest, PRInt32 *numRequests)
{
  rangeRequest.Truncate();
  *numRequests  = 0;
  
  if (!aRangeList)
    return;

  PRInt32 requestCnt = 0;
  nsCAutoString string("bytes=");

  for (nsByteRange * range = aRangeList; range != nsnull; range = range->next) {
    
    if (!range->length)
      continue;

    
    string.AppendInt(range->offset);
    string.Append("-");
    string.AppendInt(range->offset + range->length - 1);
    if (range->next)
      string += ",";

    requestCnt++;
  }

  
  string.Trim(",", PR_FALSE);

  rangeRequest = string;
  *numRequests  = requestCnt;
  return;
}

NS_IMETHODIMP
nsPluginStreamInfo::RequestRead(nsByteRange* rangeList)
{
  nsCAutoString rangeString;
  PRInt32 numRequests;

  
  nsCOMPtr<nsISupportsWeakReference> suppWeakRef(
    do_QueryInterface((nsISupportsWeakReference *)(mPluginStreamListenerPeer)));
  if (!suppWeakRef)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWeakReference> pWeakRefPluginStreamListenerPeer =
           do_GetWeakReference(suppWeakRef);
  if (!pWeakRefPluginStreamListenerPeer)
    return NS_ERROR_FAILURE;

  MakeByteRangeString(rangeList, rangeString, &numRequests);

  if (numRequests == 0)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> url;

  rv = NS_NewURI(getter_AddRefs(url), nsDependentCString(mURL));

  nsCOMPtr<nsIInterfaceRequestor> callbacks = do_QueryReferent(mPluginStreamListenerPeer->mWeakPtrChannelCallbacks);
  nsCOMPtr<nsILoadGroup> loadGroup = do_QueryReferent(mPluginStreamListenerPeer->mWeakPtrChannelLoadGroup);
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull, loadGroup, callbacks);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (!httpChannel)
    return NS_ERROR_FAILURE;

  httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Range"), rangeString, PR_FALSE);

  mPluginStreamListenerPeer->mAbort = PR_TRUE; 
                                               

  nsCOMPtr<nsIStreamListener> converter;

  if (numRequests == 1) {
    converter = mPluginStreamListenerPeer;

    
    
    
    SetStreamOffset(rangeList->offset);
  } else {
    nsPluginByteRangeStreamListener *brrListener =
      new nsPluginByteRangeStreamListener(pWeakRefPluginStreamListenerPeer);
    if (brrListener)
      converter = brrListener;
    else
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mPluginStreamListenerPeer->mPendingRequests += numRequests;

  nsCOMPtr<nsISupportsPRUint32> container = do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  rv = container->SetData(MAGIC_REQUEST_CONTEXT);
  if (NS_FAILED(rv))
    return rv;

  return channel->AsyncOpen(converter, container);
}

NS_IMETHODIMP
nsPluginStreamInfo::GetStreamOffset(PRInt32 *result)
{
  *result = mStreamOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsPluginStreamInfo::SetStreamOffset(PRInt32 offset)
{
  mStreamOffset = offset;
  return NS_OK;
}

void
nsPluginStreamInfo::SetContentType(const char* contentType)
{
  if (mContentType != nsnull)
    PL_strfree(mContentType);

  mContentType = PL_strdup(contentType);
}

void
nsPluginStreamInfo::SetSeekable(const PRBool seekable)
{
  mSeekable = seekable;
}

void
nsPluginStreamInfo::SetLength(const PRUint32 length)
{
  mLength = length;
}

void
nsPluginStreamInfo::SetLastModified(const PRUint32 modified)
{
  mModified = modified;
}

void
nsPluginStreamInfo::SetURL(const char* url)
{
  if (mURL)
    PL_strfree(mURL);

  mURL = PL_strdup(url);
}

void
nsPluginStreamInfo::SetPluginInstance(nsIPluginInstance * aPluginInstance)
{
  NS_IF_ADDREF(mPluginInstance = aPluginInstance);
}

void
nsPluginStreamInfo::SetPluginStreamListenerPeer(nsPluginStreamListenerPeer * aPluginStreamListenerPeer)
{
  
  mPluginStreamListenerPeer = aPluginStreamListenerPeer;
}

class nsPluginCacheListener : public nsIStreamListener
{
public:
  nsPluginCacheListener(nsPluginStreamListenerPeer* aListener);
  virtual ~nsPluginCacheListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  nsPluginStreamListenerPeer* mListener;
};

nsPluginCacheListener::nsPluginCacheListener(nsPluginStreamListenerPeer* aListener)
{
  mListener = aListener;
  NS_ADDREF(mListener);
}

nsPluginCacheListener::~nsPluginCacheListener()
{
  NS_IF_RELEASE(mListener);
}

NS_IMPL_ISUPPORTS1(nsPluginCacheListener, nsIStreamListener)

NS_IMETHODIMP
nsPluginCacheListener::OnStartRequest(nsIRequest *request, nsISupports* ctxt)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPluginCacheListener::OnDataAvailable(nsIRequest *request, nsISupports* ctxt,
                                       nsIInputStream* aIStream,
                                       PRUint32 sourceOffset,
                                       PRUint32 aLength)
{

  PRUint32 readlen;
  char* buffer = (char*) PR_Malloc(aLength);

  
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = aIStream->Read(buffer, aLength, &readlen);

  NS_ASSERTION(aLength == readlen, "nsCacheListener->OnDataAvailable: "
               "readlen != aLength");

  PR_Free(buffer);
  return rv;
}

NS_IMETHODIMP
nsPluginCacheListener::OnStopRequest(nsIRequest *request,
                                     nsISupports* aContext,
                                     nsresult aStatus)
{
  return NS_OK;
}

nsPluginStreamListenerPeer::nsPluginStreamListenerPeer()
{
  mURL = nsnull;
  mOwner = nsnull;
  mInstance = nsnull;
  mPStreamListener = nsnull;
  mHost = nsnull;
  mStreamType = nsPluginStreamType_Normal;
  mStartBinding = PR_FALSE;
  mAbort = PR_FALSE;
  mRequestFailed = PR_FALSE;

  mPendingRequests = 0;
  mHaveFiredOnStartRequest = PR_FALSE;
  mDataForwardToRequest = nsnull;
  mLocalCachedFile = nsnull;
}

nsPluginStreamListenerPeer::~nsPluginStreamListenerPeer()
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (mURL != nsnull) mURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
    ("nsPluginStreamListenerPeer::dtor this=%p, url=%s%c",this, urlSpec.get(), mLocalCachedFile?',':'\n'));
#endif

  NS_IF_RELEASE(mURL);
  NS_IF_RELEASE(mOwner);
  NS_IF_RELEASE(mInstance);
  NS_IF_RELEASE(mPStreamListener);
  NS_IF_RELEASE(mHost);

  
  
  if (mFileCacheOutputStream)
    mFileCacheOutputStream = nsnull;

  
  
  if (mLocalCachedFile) {
    nsrefcnt refcnt;
    NS_RELEASE2(mLocalCachedFile, refcnt);

#ifdef PLUGIN_LOGGING
    nsCAutoString filePath;
    mLocalCachedFile->GetNativePath(filePath);

    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
      ("LocalyCachedFile=%s has %d refcnt and will %s be deleted now\n",filePath.get(),refcnt,refcnt==1?"":"NOT"));
#endif

    if (refcnt == 1) {
      mLocalCachedFile->Remove(PR_FALSE);
      NS_RELEASE(mLocalCachedFile);
    }
  }

  delete mDataForwardToRequest;
}

NS_IMPL_ISUPPORTS4(nsPluginStreamListenerPeer,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIHttpHeaderVisitor,
                   nsISupportsWeakReference)


nsresult nsPluginStreamListenerPeer::Initialize(nsIURI *aURL,
                                                nsIPluginInstance *aInstance,
                                                nsIPluginStreamListener* aListener,
                                                PRInt32 requestCount)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (aURL != nsnull) aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginStreamListenerPeer::Initialize instance=%p, url=%s\n", aInstance, urlSpec.get()));

  PR_LogFlush();
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  mInstance = aInstance;
  NS_ADDREF(mInstance);

  mPStreamListener = aListener;
  NS_ADDREF(mPStreamListener);

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mPendingRequests = requestCount;

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest)
      return NS_ERROR_FAILURE;

  return NS_OK;
}







nsresult nsPluginStreamListenerPeer::InitializeEmbedded(nsIURI *aURL,
                                                        nsIPluginInstance* aInstance,
                                                        nsIPluginInstanceOwner *aOwner,
                                                        nsIPluginHost *aHost)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginStreamListenerPeer::InitializeEmbedded url=%s\n", urlSpec.get()));

  PR_LogFlush();
#endif

  mURL = aURL;
  NS_ADDREF(mURL);

  if (aInstance) {
    NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeEmbedded mInstance != nsnull");
    mInstance = aInstance;
    NS_ADDREF(mInstance);
  } else {
    mOwner = aOwner;
    NS_IF_ADDREF(mOwner);

    mHost = aHost;
    NS_IF_ADDREF(mHost);
  }

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest)
      return NS_ERROR_FAILURE;

  return NS_OK;
}



nsresult nsPluginStreamListenerPeer::InitializeFullPage(nsIPluginInstance *aInstance)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginStreamListenerPeer::InitializeFullPage instance=%p\n",aInstance));

  NS_ASSERTION(mInstance == nsnull, "nsPluginStreamListenerPeer::InitializeFullPage mInstance != nsnull");
  mInstance = aInstance;
  NS_ADDREF(mInstance);

  mPluginStreamInfo = new nsPluginStreamInfo();
  if (!mPluginStreamInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  mPluginStreamInfo->SetPluginInstance(aInstance);
  mPluginStreamInfo->SetPluginStreamListenerPeer(this);

  mDataForwardToRequest = new nsHashtable(16, PR_FALSE);
  if (!mDataForwardToRequest)
      return NS_ERROR_FAILURE;

  return NS_OK;
}








nsresult
nsPluginStreamListenerPeer::SetupPluginCacheFile(nsIChannel* channel)
{
  nsresult rv = NS_OK;
  
  
  
  
  
  
  
  PRBool useExistingCacheFile = PR_FALSE;
  nsPluginInstanceTag *pActivePlugins = gActivePluginList->mFirst;
  while (pActivePlugins && pActivePlugins->mStreams && !useExistingCacheFile) {
    
    PRInt32 cnt;
    pActivePlugins->mStreams->Count((PRUint32*)&cnt);
    while (--cnt >= 0 && !useExistingCacheFile) {
      nsPluginStreamListenerPeer *lp =
        reinterpret_cast<nsPluginStreamListenerPeer *>(pActivePlugins->mStreams->ElementAt(cnt));
      if (lp) {
        if (lp->mLocalCachedFile &&
            lp->mPluginStreamInfo &&
            (useExistingCacheFile =
             lp->mPluginStreamInfo->UseExistingPluginCacheFile(mPluginStreamInfo))) {
            NS_ADDREF(mLocalCachedFile = lp->mLocalCachedFile);
        }
        NS_RELEASE(lp);
      }
    }
    pActivePlugins = pActivePlugins->mNext;
  }

  if (!useExistingCacheFile) {
    nsCOMPtr<nsIFile> pluginTmp;
    rv = nsPluginHost::GetPluginTempDir(getter_AddRefs(pluginTmp));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
    if (!url)
      return NS_ERROR_FAILURE;

    nsCAutoString filename;
    url->GetFileName(filename);
    if (NS_FAILED(rv))
      return rv;

    
    filename.Insert(NS_LITERAL_CSTRING("plugin-"), 0);
    rv = pluginTmp->AppendNative(filename);
    if (NS_FAILED(rv))
      return rv;

    
    rv = pluginTmp->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv))
      return rv;

    
    nsCOMPtr<nsIOutputStream> outstream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(mFileCacheOutputStream), pluginTmp, -1, 00600);
    if (NS_FAILED(rv))
      return rv;

    
    CallQueryInterface(pluginTmp, &mLocalCachedFile); 
    
    
    NS_ADDREF(mLocalCachedFile);
  }

  
  
  
  pActivePlugins = gActivePluginList->find(mInstance);
  if (pActivePlugins) {
    if (!pActivePlugins->mStreams &&
       (NS_FAILED(rv = NS_NewISupportsArray(getter_AddRefs(pActivePlugins->mStreams))))) {
      return rv;
    }

    nsISupports* supports = static_cast<nsISupports*>((static_cast<nsIStreamListener*>(this)));
    pActivePlugins->mStreams->AppendElement(supports);
  }

  return rv;
}

NS_IMETHODIMP
nsPluginStreamListenerPeer::OnStartRequest(nsIRequest *request,
                                           nsISupports* aContext)
{
  nsresult  rv = NS_OK;

  if (mHaveFiredOnStartRequest) {
      return NS_OK;
  }

  mHaveFiredOnStartRequest = PR_TRUE;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    PRUint32 responseCode = 0;
    rv = httpChannel->GetResponseStatus(&responseCode);
    if (NS_FAILED(rv)) {
      
      
      
      
      mRequestFailed = PR_TRUE;
      return NS_ERROR_FAILURE;
    }

    if (responseCode > 206) { 
      PRBool bWantsAllNetworkStreams = PR_FALSE;
      mInstance->GetValue(nsPluginInstanceVariable_WantsAllNetworkStreams,
                          (void *)&bWantsAllNetworkStreams);
      if (!bWantsAllNetworkStreams) {
        mRequestFailed = PR_TRUE;
        return NS_ERROR_FAILURE;
      }
    }
  }

  
  
  
  if (mOwner) {
    nsCOMPtr<nsIPluginTagInfo> pti = do_QueryInterface(mOwner);
    NS_ENSURE_TRUE(pti, NS_ERROR_FAILURE);
    nsPluginTagType tagType;
    if (NS_FAILED(pti->GetTagType(&tagType)))
      return NS_ERROR_FAILURE;  
  }

  
  
  
  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  channel->GetNotificationCallbacks(getter_AddRefs(callbacks));
  if (callbacks)
    mWeakPtrChannelCallbacks = do_GetWeakReference(callbacks);

  nsCOMPtr<nsILoadGroup> loadGroup;
  channel->GetLoadGroup(getter_AddRefs(loadGroup));
  if (loadGroup)
    mWeakPtrChannelLoadGroup = do_GetWeakReference(loadGroup);

  PRInt32 length;
  rv = channel->GetContentLength(&length);

  
  
  if (NS_FAILED(rv) || length == -1) {
    
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(channel);
    if (fileChannel) {
      
      mRequestFailed = PR_TRUE;
      return NS_ERROR_FAILURE;
    }
    mPluginStreamInfo->SetLength(PRUint32(0));
  }
  else {
    mPluginStreamInfo->SetLength(length);
  }

  mPluginStreamInfo->SetRequest(request);

  nsCAutoString aContentType; 
  rv = channel->GetContentType(aContentType);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString urlSpec;
  aURL->GetSpec(urlSpec);
  mPluginStreamInfo->SetURL(urlSpec.get());

  if (!aContentType.IsEmpty())
    mPluginStreamInfo->SetContentType(aContentType.get());

#ifdef PLUGIN_LOGGING
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnStartRequest this=%p request=%p mime=%s, url=%s\n",
  this, request, aContentType.get(), urlSpec.get()));

  PR_LogFlush();
#endif

  nsPluginWindow    *window = nsnull;

  
  
  
  
  
  

  if (!mInstance && mOwner && !aContentType.IsEmpty()) {
    mOwner->GetInstance(mInstance);
    mOwner->GetWindow(window);
    if (!mInstance && mHost && window) {
      
      nsPluginMode mode;
      mOwner->GetMode(&mode);
      if (mode == nsPluginMode_Embedded)
        rv = mHost->InstantiateEmbeddedPlugin(aContentType.get(), aURL, mOwner);
      else
        rv = mHost->SetUpPluginInstance(aContentType.get(), aURL, mOwner);

      if (NS_OK == rv) {
        
        mOwner->GetInstance(mInstance);
        if (mInstance) {
          mInstance->Start();
          mOwner->CreateWidget();
          
          if (window->window) {
            nsCOMPtr<nsIPluginInstance> inst = mInstance;
            ((nsPluginNativeWindow*)window)->CallSetWindow(inst);
          }
        }
      }
    }
  }

  
  rv = SetUpStreamListener(request, aURL);
  if (NS_FAILED(rv)) return rv;

  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnProgress(nsIRequest *request,
                                                     nsISupports* aContext,
                                                     PRUint64 aProgress,
                                                     PRUint64 aProgressMax)
{
  nsresult rv = NS_OK;
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStatus(nsIRequest *request,
                                                   nsISupports* aContext,
                                                   nsresult aStatus,
                                                   const PRUnichar* aStatusArg)
{
  return NS_OK;
}

class nsPRUintKey : public nsHashKey {
protected:
  PRUint32 mKey;
public:
  nsPRUintKey(PRUint32 key) : mKey(key) {}

  PRUint32 HashCode() const {
    return mKey;
  }

  PRBool Equals(const nsHashKey *aKey) const {
    return mKey == ((const nsPRUintKey*)aKey)->mKey;
  }
  nsHashKey *Clone() const {
    return new nsPRUintKey(mKey);
  }
  PRUint32 GetValue() { return mKey; }
};

NS_IMETHODIMP nsPluginStreamListenerPeer::OnDataAvailable(nsIRequest *request,
                                                          nsISupports* aContext,
                                                          nsIInputStream *aIStream,
                                                          PRUint32 sourceOffset,
                                                          PRUint32 aLength)
{
  if (mRequestFailed)
    return NS_ERROR_FAILURE;

  if (mAbort) {
      PRUint32 magicNumber = 0;  
      nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
      if (container)
        container->GetData(&magicNumber);

      if (magicNumber != MAGIC_REQUEST_CONTEXT) {
        
        mAbort = PR_FALSE;
        return NS_BINDING_ABORTED;
      }
  }

  nsresult rv = NS_OK;

  if (!mPStreamListener || !mPluginStreamInfo)
    return NS_ERROR_FAILURE;

  mPluginStreamInfo->SetRequest(request);

  const char * url = nsnull;
  mPluginStreamInfo->GetURL(&url);

  PLUGIN_LOG(PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnDataAvailable this=%p request=%p, offset=%d, length=%d, url=%s\n",
  this, request, sourceOffset, aLength, url ? url : "no url set"));

  
  
  if (mStreamType != nsPluginStreamType_AsFileOnly) {
    
    nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
    if (brr) {
      if (!mDataForwardToRequest)
        return NS_ERROR_FAILURE;

      PRInt64 absoluteOffset64 = LL_ZERO;
      brr->GetStartRange(&absoluteOffset64);

      
      PRInt32 absoluteOffset = (PRInt32)nsInt64(absoluteOffset64);

      
      

      
      
      
      
      nsPRUintKey key(absoluteOffset);
      PRInt32 amtForwardToPlugin =
        NS_PTR_TO_INT32(mDataForwardToRequest->Get(&key));
      mDataForwardToRequest->Put(&key, NS_INT32_TO_PTR(amtForwardToPlugin + aLength));

      mPluginStreamInfo->SetStreamOffset(absoluteOffset + amtForwardToPlugin);
    }

    nsCOMPtr<nsIInputStream> stream = aIStream;

    
    
    

    if (mFileCacheOutputStream) {
        rv = NS_NewInputStreamTee(getter_AddRefs(stream), aIStream, mFileCacheOutputStream);
        if (NS_FAILED(rv))
            return rv;
    }

    rv =  mPStreamListener->OnDataAvailable(mPluginStreamInfo,
                                            stream,
                                            aLength);

    
    
    if (NS_FAILED(rv))
      request->Cancel(rv);
  }
  else
  {
    
    char* buffer = new char[aLength];
    PRUint32 amountRead, amountWrote = 0;
    rv = aIStream->Read(buffer, aLength, &amountRead);

    
    if (mFileCacheOutputStream) {
      while (amountWrote < amountRead && NS_SUCCEEDED(rv)) {
        rv = mFileCacheOutputStream->Write(buffer, amountRead, &amountWrote);
      }
    }
    delete [] buffer;
  }
  return rv;
}

NS_IMETHODIMP nsPluginStreamListenerPeer::OnStopRequest(nsIRequest *request,
                                                        nsISupports* aContext,
                                                        nsresult aStatus)
{
  nsresult rv = NS_OK;

  PLUGIN_LOG(PLUGIN_LOG_NOISY,
  ("nsPluginStreamListenerPeer::OnStopRequest this=%p aStatus=%d request=%p\n",
  this, aStatus, request));

  
  nsCOMPtr<nsIByteRangeRequest> brr = do_QueryInterface(request);
  if (brr) {
    PRInt64 absoluteOffset64 = LL_ZERO;
    brr->GetStartRange(&absoluteOffset64);
    
    PRInt32 absoluteOffset = (PRInt32)nsInt64(absoluteOffset64);

    nsPRUintKey key(absoluteOffset);

    
    mDataForwardToRequest->Remove(&key);


    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("                          ::OnStopRequest for ByteRangeRequest Started=%d\n",
    absoluteOffset));
  } else {
    
    
    
    mFileCacheOutputStream = nsnull;
  }

  
  if (--mPendingRequests > 0)
      return NS_OK;

  
  nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(aContext);
  if (container) {
    PRUint32 magicNumber = 0;  
    container->GetData(&magicNumber);
    if (magicNumber == MAGIC_REQUEST_CONTEXT) {
      
      return NS_OK;
    }
  }

  if (!mPStreamListener)
      return NS_ERROR_FAILURE;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel)
    return NS_ERROR_FAILURE;
  
  nsCAutoString aContentType;
  rv = channel->GetContentType(aContentType);
  if (NS_FAILED(rv) && !mRequestFailed)
    return rv;

  if (!aContentType.IsEmpty())
    mPluginStreamInfo->SetContentType(aContentType.get());

  
  if (mRequestFailed)
    aStatus = NS_ERROR_FAILURE;

  if (NS_FAILED(aStatus)) {
    
    
    mPStreamListener->OnStopBinding(mPluginStreamInfo, aStatus);
    return NS_OK;
  }

  
  if (mStreamType >= nsPluginStreamType_AsFile) {
    nsCOMPtr<nsIFile> localFile = do_QueryInterface(mLocalCachedFile);
    if (!localFile) {
      nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(request);
      if (cacheChannel) {
        cacheChannel->GetCacheFile(getter_AddRefs(localFile));
      } else {
        
        nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(request);
        if (fileChannel) {
          fileChannel->GetFile(getter_AddRefs(localFile));
        }
      }
    }

    if (localFile) {
      OnFileAvailable(localFile);
    }
  }

  if (mStartBinding) {
    
    mPStreamListener->OnStopBinding(mPluginStreamInfo, aStatus);
  } else {
    
    mPStreamListener->OnStartBinding(mPluginStreamInfo);
    mPStreamListener->OnStopBinding(mPluginStreamInfo, aStatus);
  }

  if (NS_SUCCEEDED(aStatus))
    mPluginStreamInfo->SetStreamComplete(PR_TRUE);

  return NS_OK;
}


nsresult nsPluginStreamListenerPeer::SetUpCache(nsIURI* aURL)
{
  nsPluginCacheListener* cacheListener = new nsPluginCacheListener(this);
  
  return NS_OpenURI(cacheListener, nsnull, aURL, nsnull);
}

nsresult nsPluginStreamListenerPeer::SetUpStreamListener(nsIRequest *request,
                                                         nsIURI* aURL)
{
  nsresult rv = NS_OK;

  
  
  
  
  
  if (!mPStreamListener && mInstance)
    rv = mInstance->NewStreamToPlugin(&mPStreamListener);

  if (NS_FAILED(rv))
    return rv;

  if (!mPStreamListener)
    return NS_ERROR_NULL_POINTER;

  PRBool useLocalCache = PR_FALSE;

  
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);

  




  if (httpChannel) {
    
    
    
    nsCOMPtr<nsIHTTPHeaderListener> listener =
      do_QueryInterface(mPStreamListener);
    if (listener) {
      
      PRUint32 statusNum;
      if (NS_SUCCEEDED(httpChannel->GetResponseStatus(&statusNum)) &&
          statusNum < 1000) {
        
        nsCString ver;
        nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
          do_QueryInterface(channel);
        if (httpChannelInternal) {
          PRUint32 major, minor;
          if (NS_SUCCEEDED(httpChannelInternal->GetResponseVersion(&major,
                                                                   &minor))) {
            ver = nsPrintfCString("/%lu.%lu", major, minor);
          }
        }

        
        nsCString statusText;
        if (NS_FAILED(httpChannel->GetResponseStatusText(statusText))) {
          statusText = "OK";
        }

        
        nsPrintfCString status(100, "HTTP%s %lu %s", ver.get(), statusNum,
                               statusText.get());
        listener->StatusLine(status.get());
      }
    }

    
    httpChannel->VisitResponseHeaders(this);

    PRBool bSeekable = PR_FALSE;
    
    
    
    
    
    
    
    nsCAutoString contentEncoding;
    if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Encoding"),
                                                    contentEncoding))) {
      useLocalCache = PR_TRUE;
    } else {
      
      
      PRUint32 length;
      mPluginStreamInfo->GetLength(&length);
      if (length) {
        nsCAutoString range;
        if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("accept-ranges"), range)) &&
          range.Equals(NS_LITERAL_CSTRING("bytes"), nsCaseInsensitiveCStringComparator())) {
          bSeekable = PR_TRUE;
          
          
          mPluginStreamInfo->SetSeekable(bSeekable);
        }
      }
    }

    
    
    nsCAutoString lastModified;
    if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"), lastModified)) &&
        !lastModified.IsEmpty()) {
      PRTime time64;
      PR_ParseTimeString(lastModified.get(), PR_TRUE, &time64);  

      
      double fpTime;
      LL_L2D(fpTime, time64);
      mPluginStreamInfo->SetLastModified((PRUint32)(fpTime * 1e-6 + 0.5));
    }
  }

  rv = mPStreamListener->OnStartBinding(mPluginStreamInfo);

  mStartBinding = PR_TRUE;

  if (NS_FAILED(rv))
    return rv;

  mPStreamListener->GetStreamType(&mStreamType);

  if (!useLocalCache && mStreamType >= nsPluginStreamType_AsFile) {
    
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(request);
    if (!fileChannel) {
      
      nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(request);
      if (!(cacheChannel && (NS_SUCCEEDED(cacheChannel->SetCacheAsFile(PR_TRUE))))) {
        useLocalCache = PR_TRUE;
      }
    }
  }

  if (useLocalCache) {
    SetupPluginCacheFile(channel);
  }

  return NS_OK;
}

nsresult
nsPluginStreamListenerPeer::OnFileAvailable(nsIFile* aFile)
{
  nsresult rv;
  if (!mPStreamListener)
    return NS_ERROR_FAILURE;

  nsCAutoString path;
  rv = aFile->GetNativePath(path);
  if (NS_FAILED(rv)) return rv;

  if (path.IsEmpty()) {
    NS_WARNING("empty path");
    return NS_OK;
  }

  rv = mPStreamListener->OnFileAvailable(mPluginStreamInfo, path.get());
  return rv;
}

NS_IMETHODIMP
nsPluginStreamListenerPeer::VisitHeader(const nsACString &header, const nsACString &value)
{
  nsCOMPtr<nsIHTTPHeaderListener> listener = do_QueryInterface(mPStreamListener);
  if (!listener)
    return NS_ERROR_FAILURE;

  return listener->NewResponseHeader(PromiseFlatCString(header).get(),
                                     PromiseFlatCString(value).get());
}

nsPluginHost::nsPluginHost()
  
  
{
  gActivePluginList = &mPluginInstanceTagList;

  
  
  mPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (mPrefService) {
    PRBool tmp;
    nsresult rv = mPrefService->GetBoolPref("plugin.override_internal_types",
                                            &tmp);
    if (NS_SUCCEEDED(rv)) {
      mOverrideInternalTypes = tmp;
    }

    rv = mPrefService->GetBoolPref("plugin.allow_alien_star_handler", &tmp);
    if (NS_SUCCEEDED(rv)) {
      mAllowAlienStarHandler = tmp;
    }

    rv = mPrefService->GetBoolPref("plugin.default_plugin_disabled", &tmp);
    if (NS_SUCCEEDED(rv)) {
      mDefaultPluginDisabled = tmp;
    }

#ifdef WINCE
    mDefaultPluginDisabled = PR_TRUE;
#endif
  }

  nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1");
  if (obsService) {
    obsService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    obsService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);
  }

#ifdef PLUGIN_LOGGING
  nsPluginLogging::gNPNLog = PR_NewLogModule(NPN_LOG_NAME);
  nsPluginLogging::gNPPLog = PR_NewLogModule(NPP_LOG_NAME);
  nsPluginLogging::gPluginLog = PR_NewLogModule(PLUGIN_LOG_NAME);

  PR_LOG(nsPluginLogging::gNPNLog, PLUGIN_LOG_ALWAYS,("NPN Logging Active!\n"));
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_ALWAYS,("General Plugin Logging Active! (nsPluginHost::ctor)\n"));
  PR_LOG(nsPluginLogging::gNPPLog, PLUGIN_LOG_ALWAYS,("NPP Logging Active!\n"));

  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHost::ctor\n"));
  PR_LogFlush();
#endif
}

nsPluginHost::~nsPluginHost()
{
  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHost::dtor\n"));

  Destroy();
  sInst = nsnull;
}

NS_IMPL_ISUPPORTS3(nsPluginHost,
                   nsIPluginHost,
                   nsIObserver,
                   nsISupportsWeakReference)

nsPluginHost*
nsPluginHost::GetInst()
{
  if (!sInst) {
    sInst = new nsPluginHost();
    if (!sInst)
      return nsnull;
    NS_ADDREF(sInst);
  }

  NS_ADDREF(sInst);
  return sInst;
}


const char *
nsPluginHost::GetPluginName(nsIPluginInstance *aPluginInstance)
{
  nsPluginInstanceTag *plugin =
    gActivePluginList ? gActivePluginList->find(aPluginInstance) : nsnull;

  if (plugin && plugin->mPluginTag)
    return plugin->mPluginTag->mName.get();

  return nsnull;
}

PRBool nsPluginHost::IsRunningPlugin(nsPluginTag * plugin)
{
  if (!plugin)
    return PR_FALSE;

  
  
  if (!plugin->mLibrary)
    return PR_FALSE;

  for (int i = 0; i < plugin->mVariants; i++) {
    nsPluginInstanceTag * p = mPluginInstanceTagList.find(plugin->mMimeTypeArray[i]);
    if (p && !p->mStopped)
      return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult nsPluginHost::ReloadPlugins(PRBool reloadPages)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins Begin reloadPages=%d, active_instance_count=%d\n",
  reloadPages, mPluginInstanceTagList.mCount));

  nsresult rv = NS_OK;

  
  
  if (!mPluginsLoaded)
    return LoadPlugins();

  
  
  

  
  
  
  
  PRBool pluginschanged = PR_TRUE;
  FindPlugins(PR_FALSE, &pluginschanged);

  
  if (!pluginschanged)
    return NS_ERROR_PLUGINS_PLUGINSNOTCHANGED;

  nsCOMPtr<nsISupportsArray> instsToReload;

  if (reloadPages) {
    NS_NewISupportsArray(getter_AddRefs(instsToReload));

    
    
    mPluginInstanceTagList.stopRunning(instsToReload, nsnull);
  }

  
  mPluginInstanceTagList.removeAllStopped();

  
  nsRefPtr<nsPluginTag> prev;
  nsRefPtr<nsPluginTag> next;

  for (nsRefPtr<nsPluginTag> p = mPlugins; p != nsnull;) {
    next = p->mNext;

    
    if (!IsRunningPlugin(p)) {
      if (p == mPlugins)
        mPlugins = next;
      else
        prev->mNext = next;

      p->mNext = nsnull;
      p = next;
      continue;
    }

    prev = p;
    p = next;
  }

  
  mPluginsLoaded = PR_FALSE;

  
  rv = LoadPlugins();

  
  
  
  PRUint32 c;
  if (reloadPages &&
      instsToReload &&
      NS_SUCCEEDED(instsToReload->Count(&c)) &&
      c > 0) {
    nsCOMPtr<nsIRunnable> ev = new nsPluginDocReframeEvent(instsToReload);
    if (ev)
      NS_DispatchToCurrentThread(ev);
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins End active_instance_count=%d\n",
  mPluginInstanceTagList.mCount));

  return rv;
}

#define NS_RETURN_UASTRING_SIZE 128

nsresult nsPluginHost::UserAgent(const char **retstring)
{
  static char resultString[NS_RETURN_UASTRING_SIZE];
  nsresult res;

  nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
  if (NS_FAILED(res))
    return res;

  nsCAutoString uaString;
  res = http->GetUserAgent(uaString);

  if (NS_SUCCEEDED(res)) {
    if (NS_RETURN_UASTRING_SIZE > uaString.Length()) {
      PL_strcpy(resultString, uaString.get());
    } else {
      
      PL_strncpy(resultString, uaString.get(), NS_RETURN_UASTRING_SIZE);
      for (int i = NS_RETURN_UASTRING_SIZE - 1; i >= 0; i--) {
        if (i == 0) {
          resultString[NS_RETURN_UASTRING_SIZE - 1] = '\0';
        }
        else if (resultString[i] == ' ') {
          resultString[i] = '\0';
          break;
        }
      }
    }
    *retstring = resultString;
  }
  else {
    *retstring = nsnull;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::UserAgent return=%s\n", *retstring));

  return res;
}

nsresult nsPluginHost::GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt)
{
  nsresult rv;
  nsCOMPtr<nsIPrompt> prompt;
  nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);

  if (wwatch) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    if (aOwner) {
      nsCOMPtr<nsIDocument> document;
      aOwner->GetDocument(getter_AddRefs(document));
      if (document) {
        domWindow = document->GetWindow();
      }
    }

    if (!domWindow) {
      wwatch->GetWindowByName(NS_LITERAL_STRING("_content").get(), nsnull, getter_AddRefs(domWindow));
    }
    rv = wwatch->GetNewPrompter(domWindow, getter_AddRefs(prompt));
  }

  NS_IF_ADDREF(*aPrompt = prompt);
  return rv;
}

NS_IMETHODIMP nsPluginHost::GetURL(nsISupports* pluginInst,
                                   const char* url,
                                   const char* target,
                                   nsIPluginStreamListener* streamListener,
                                   const char* altHost,
                                   const char* referrer,
                                   PRBool forceJSEnabled)
{
  return GetURLWithHeaders(pluginInst, url, target, streamListener,
                           altHost, referrer, forceJSEnabled, nsnull, nsnull);
}

nsresult nsPluginHost::GetURLWithHeaders(nsISupports* pluginInst,
                                         const char* url,
                                         const char* target,
                                         nsIPluginStreamListener* streamListener,
                                         const char* altHost,
                                         const char* referrer,
                                         PRBool forceJSEnabled,
                                         PRUint32 getHeadersLength,
                                         const char* getHeaders)
{
  nsAutoString string;
  string.AssignWithConversion(url);

  
  
  if (!target && !streamListener)
   return NS_ERROR_ILLEGAL_VALUE;

  nsresult rv;
  nsCOMPtr<nsIPluginInstance> instance = do_QueryInterface(pluginInst, &rv);
  if (NS_SUCCEEDED(rv))
    rv = DoURLLoadSecurityCheck(instance, url);

  if (NS_SUCCEEDED(rv)) {
    if (target) {
      nsCOMPtr<nsIPluginInstanceOwner> owner;
      rv = instance->GetOwner(getter_AddRefs(owner));
      if (owner) {
        if ((0 == PL_strcmp(target, "newwindow")) ||
            (0 == PL_strcmp(target, "_new")))
          target = "_blank";
        else if (0 == PL_strcmp(target, "_current"))
          target = "_self";

        rv = owner->GetURL(url, target, nsnull, 0, (void *) getHeaders, getHeadersLength);
      }
    }

    if (streamListener) {
      rv = NewPluginURLStream(string, instance, streamListener, nsnull,
                              PR_FALSE, nsnull, getHeaders, getHeadersLength);
    }
  }

  return rv;
}

NS_IMETHODIMP nsPluginHost::PostURL(nsISupports* pluginInst,
                                    const char* url,
                                    PRUint32 postDataLen,
                                    const char* postData,
                                    PRBool isFile,
                                    const char* target,
                                    nsIPluginStreamListener* streamListener,
                                    const char* altHost,
                                    const char* referrer,
                                    PRBool forceJSEnabled,
                                    PRUint32 postHeadersLength,
                                    const char* postHeaders)
{
  nsAutoString string; string.AssignWithConversion(url);
  nsresult rv;

  
  
  
  if (!target && !streamListener)
   return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIPluginInstance> instance = do_QueryInterface(pluginInst, &rv);
  if (NS_SUCCEEDED(rv))
    rv = DoURLLoadSecurityCheck(instance, url);

  if (NS_SUCCEEDED(rv)) {
    char *dataToPost;
    if (isFile) {
      rv = CreateTmpFileToPost(postData, &dataToPost);
      if (NS_FAILED(rv) || !dataToPost)
        return rv;
    } else {
      PRUint32 newDataToPostLen;
      ParsePostBufferToFixHeaders(postData, postDataLen, &dataToPost, &newDataToPostLen);
      if (!dataToPost)
        return NS_ERROR_UNEXPECTED;

      
      
      
      
      postDataLen = newDataToPostLen;
    }

    if (target) {
      nsCOMPtr<nsIPluginInstanceOwner> owner;
      rv = instance->GetOwner(getter_AddRefs(owner));
      if (owner) {
        if (!target) {
          target = "_self";
        } else {
          if ((0 == PL_strcmp(target, "newwindow")) ||
              (0 == PL_strcmp(target, "_new"))) {
            target = "_blank";
          } else if (0 == PL_strcmp(target, "_current")) {
            target = "_self";
          }
        }
        rv = owner->GetURL(url, target, (void*)dataToPost, postDataLen,
                           (void*)postHeaders, postHeadersLength, isFile);
      }
    }

    
    
    if (streamListener)
      rv = NewPluginURLStream(string, instance, streamListener,
                              (const char*)dataToPost, isFile, postDataLen,
                              postHeaders, postHeadersLength);
    if (isFile)
      NS_Free(dataToPost);
  }

  return rv;
}











NS_IMETHODIMP nsPluginHost::FindProxyForURL(const char* url, char* *result)
{
  if (!url || !result) {
    return NS_ERROR_INVALID_ARG;
  }
  nsresult res;

  nsCOMPtr<nsIURI> uriIn;
  nsCOMPtr<nsIProtocolProxyService> proxyService;
  nsCOMPtr<nsIIOService> ioService;

  proxyService = do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !proxyService)
    return res;

  ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !ioService)
    return res;

  
  res = ioService->NewURI(nsDependentCString(url), nsnull, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(res))
    return res;

  nsCOMPtr<nsIProxyInfo> pi;

  res = proxyService->Resolve(uriIn, 0, getter_AddRefs(pi));
  if (NS_FAILED(res))
    return res;

  nsCAutoString host, type;
  PRInt32 port = -1;

  
  if (pi) {
    pi->GetType(type);
    pi->GetHost(host);
    pi->GetPort(&port);
  }

  if (!pi || host.IsEmpty() || port <= 0 || host.EqualsLiteral("direct")) {
    *result = PL_strdup("DIRECT");
  } else if (type.EqualsLiteral("http")) {
    *result = PR_smprintf("PROXY %s:%d", host.get(), port);
  } else if (type.EqualsLiteral("socks4")) {
    *result = PR_smprintf("SOCKS %s:%d", host.get(), port);
  } else if (type.EqualsLiteral("socks")) {
    
    
    
    
    
    *result = PR_smprintf("SOCKS %s:%d", host.get(), port);
  } else {
    NS_ASSERTION(PR_FALSE, "Unknown proxy type!");
    *result = PL_strdup("DIRECT");
  }

  if (nsnull == *result)
    res = NS_ERROR_OUT_OF_MEMORY;

  return res;
}

NS_IMETHODIMP nsPluginHost::Init()
{
  return NS_OK;
}

NS_IMETHODIMP nsPluginHost::Destroy()
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::Destroy Called\n"));

  if (mIsDestroyed)
    return NS_OK;

  mIsDestroyed = PR_TRUE;

  
  
  mPluginInstanceTagList.stopRunning(nsnull, nsnull);

  
  mPluginInstanceTagList.shutdown();

  if (mPluginPath) {
    PR_Free(mPluginPath);
    mPluginPath = nsnull;
  }

  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);

  
  if (sPluginTempDir) {
    sPluginTempDir->Remove(PR_TRUE);
    NS_RELEASE(sPluginTempDir);
  }

#ifdef XP_WIN
  if (mPrivateDirServiceProvider) {
    nsCOMPtr<nsIDirectoryService> dirService =
      do_GetService(kDirectoryServiceContractID);
    if (dirService)
      dirService->UnregisterProvider(mPrivateDirServiceProvider);
    mPrivateDirServiceProvider = nsnull;
  }
#endif 

  mPrefService = nsnull; 

  return NS_OK;
}

void nsPluginHost::UnloadUnusedLibraries()
{
  
  for (PRUint32 i = 0; i < mUnusedLibraries.Length(); i++) {
    PRLibrary * library = mUnusedLibraries[i];
    if (library)
      PostPluginUnloadEvent(library);
  }
  mUnusedLibraries.Clear();
}

nsresult
nsPluginHost::GetPluginTempDir(nsIFile **aDir)
{
  if (!sPluginTempDir) {
    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                         getter_AddRefs(tmpDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = tmpDir->AppendNative(kPluginTmpDirName);

    
    rv = tmpDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ENSURE_SUCCESS(rv, rv);

    tmpDir.swap(sPluginTempDir);
  }

  return sPluginTempDir->Clone(aDir);
}

NS_IMETHODIMP nsPluginHost::InstantiatePluginForChannel(nsIChannel* aChannel,
                                                            nsIPluginInstanceOwner* aOwner,
                                                            nsIStreamListener** aListener)
{
  NS_PRECONDITION(aChannel && aOwner,
                  "Invalid arguments to InstantiatePluginForChannel");
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return rv;

#ifdef PLUGIN_LOGGING
  if (PR_LOG_TEST(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL)) {
    nsCAutoString urlSpec;
    uri->GetAsciiSpec(urlSpec);

    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
           ("nsPluginHost::InstantiatePluginForChannel Begin owner=%p, url=%s\n",
           aOwner, urlSpec.get()));

    PR_LogFlush();
  }
#endif

  
  

  return NewEmbeddedPluginStreamListener(uri, aOwner, nsnull, aListener);
}


NS_IMETHODIMP nsPluginHost::InstantiateEmbeddedPlugin(const char *aMimeType,
                                                          nsIURI* aURL,
                                                          nsIPluginInstanceOwner *aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (aURL)
    aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiateEmbeddedPlugin Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif

  nsresult  rv;
  nsIPluginInstance *instance = nsnull;
  nsCOMPtr<nsIPluginTagInfo> pti;
  nsPluginTagType tagType;

  rv = aOwner->QueryInterface(kIPluginTagInfoIID, getter_AddRefs(pti));

  if (rv != NS_OK)
    return rv;

  rv = pti->GetTagType(&tagType);

  if ((rv != NS_OK) || !((tagType == nsPluginTagType_Embed)
                        || (tagType == nsPluginTagType_Applet)
                        || (tagType == nsPluginTagType_Object))) {
    return rv;
  }

  
  
  
  if (aURL) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
                    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return rv; 

    nsCOMPtr<nsIDocument> doc;
    aOwner->GetDocument(getter_AddRefs(doc));
    if (!doc)
      return NS_ERROR_NULL_POINTER;

    rv = secMan->CheckLoadURIWithPrincipal(doc->NodePrincipal(), aURL, 0);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDOMElement> elem;
    pti->GetDOMElement(getter_AddRefs(elem));

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT; 
    nsresult rv =
      NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT,
                                aURL,
                                doc->NodePrincipal(),
                                elem,
                                nsDependentCString(aMimeType ? aMimeType : ""),
                                nsnull, 
                                &shouldLoad);
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad))
      return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
  }

  
  
  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_FALSE);
  if (pluginTag) {
    if (!pluginTag->IsEnabled())
      return NS_ERROR_NOT_AVAILABLE;
  }

  PRBool isJava = pluginTag && pluginTag->mIsJavaPlugin;

  
  
  
  PRBool bCanHandleInternally = PR_FALSE;
  nsCAutoString scheme;
  if (aURL && NS_SUCCEEDED(aURL->GetScheme(scheme))) {
      nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
      contractID += scheme;
      ToLowerCase(contractID);
      nsCOMPtr<nsIProtocolHandler> handler = do_GetService(contractID.get());
      if (handler)
        bCanHandleInternally = PR_TRUE;
  }

  if (FindStoppedPluginForURL(aURL, aOwner) == NS_OK) {

    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHost::InstantiateEmbeddedPlugin FoundStopped mime=%s\n", aMimeType));

    aOwner->GetInstance(instance);
    if (!isJava && bCanHandleInternally)
      rv = NewEmbeddedPluginStream(aURL, aOwner, instance);

    
    nsresult res;
    nsCOMPtr<nsIPluginInstanceOwner> javaDOM =
             do_GetService("@mozilla.org/blackwood/java-dom;1", &res);
    if (NS_SUCCEEDED(res) && javaDOM)
      javaDOM->SetInstance(instance);

    NS_IF_RELEASE(instance);
    return NS_OK;
  }

  
  
  if (!aMimeType)
    return bCanHandleInternally ? NewEmbeddedPluginStream(aURL, aOwner, nsnull) : NS_ERROR_FAILURE;

  rv = SetUpPluginInstance(aMimeType, aURL, aOwner);

  if (rv == NS_OK) {
    rv = aOwner->GetInstance(instance);
  } else {
   






    PRBool bHasPluginURL = PR_FALSE;
    nsCOMPtr<nsIPluginTagInfo> pti(do_QueryInterface(aOwner));

    if (pti) {
      const char *value;
      bHasPluginURL = NS_SUCCEEDED(pti->GetParameter("PLUGINURL", &value));
    }

    
    
    if (nsPluginTagType_Object == tagType && !bHasPluginURL)
      return rv;

    if (NS_FAILED(SetUpDefaultPluginInstance(aMimeType, aURL, aOwner)))
      return NS_ERROR_FAILURE;

    if (NS_FAILED(aOwner->GetInstance(instance)))
      return NS_ERROR_FAILURE;

    rv = NS_OK;
  }

  
  
  if (rv == NS_ERROR_FAILURE)
    return rv;

  
  

  nsPluginWindow    *window = nsnull;

  
  aOwner->GetWindow(window);

  if (instance) {
    instance->Start();
    aOwner->CreateWidget();

    
    if (window->window) {
      nsCOMPtr<nsIPluginInstance> inst = instance;
      ((nsPluginNativeWindow*)window)->CallSetWindow(inst);
    }

    
    
    PRBool havedata = PR_FALSE;

    nsCOMPtr<nsIPluginTagInfo> pti(do_QueryInterface(aOwner, &rv));

    if (pti) {
      const char *value;
      havedata = NS_SUCCEEDED(pti->GetAttribute("SRC", &value));
      
    }

    if (havedata && !isJava && bCanHandleInternally)
      rv = NewEmbeddedPluginStream(aURL, aOwner, instance);

    
    nsresult res;
    nsCOMPtr<nsIPluginInstanceOwner> javaDOM =
             do_GetService("@mozilla.org/blackwood/java-dom;1", &res);
    if (NS_SUCCEEDED(res) && javaDOM)
      javaDOM->SetInstance(instance);

    NS_RELEASE(instance);
  }

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if (aURL != nsnull) aURL->GetAsciiSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiateEmbeddedPlugin Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, rv, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return rv;
}


NS_IMETHODIMP nsPluginHost::InstantiateFullPagePlugin(const char *aMimeType,
                                                          nsIURI* aURI,
                                                          nsIStreamListener *&aStreamListener,
                                                          nsIPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  aURI->GetSpec(urlSpec);
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::InstantiateFullPagePlugin Begin mime=%s, owner=%p, url=%s\n",
  aMimeType, aOwner, urlSpec.get()));
#endif

  if (FindStoppedPluginForURL(aURI, aOwner) == NS_OK) {
    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHost::InstantiateFullPagePlugin FoundStopped mime=%s\n",aMimeType));

    nsIPluginInstance* instance;
    aOwner->GetInstance(instance);
    nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
    if (!pluginTag || !pluginTag->mIsJavaPlugin)
      NewFullPagePluginStream(aStreamListener, instance);
    NS_IF_RELEASE(instance);
    return NS_OK;
  }

  nsresult rv = SetUpPluginInstance(aMimeType, aURI, aOwner);

  if (NS_OK == rv) {
    nsCOMPtr<nsIPluginInstance> instance;
    nsPluginWindow * win = nsnull;

    aOwner->GetInstance(*getter_AddRefs(instance));
    aOwner->GetWindow(win);

    if (win && instance) {
      instance->Start();
      aOwner->CreateWidget();

      
      nsPluginNativeWindow * window = (nsPluginNativeWindow *)win;
      if (window->window)
        window->CallSetWindow(instance);

      rv = NewFullPagePluginStream(aStreamListener, instance);

      
      if (window->window)
        window->CallSetWindow(instance);
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::InstantiateFullPagePlugin End mime=%s, rv=%d, owner=%p, url=%s\n",
  aMimeType, rv, aOwner, urlSpec.get()));

  return rv;
}

nsresult nsPluginHost::FindStoppedPluginForURL(nsIURI* aURL,
                                                   nsIPluginInstanceOwner *aOwner)
{
  nsCAutoString url;
  if (!aURL)
    return NS_ERROR_FAILURE;

  aURL->GetAsciiSpec(url);

  nsPluginInstanceTag * plugin = mPluginInstanceTagList.findStopped(url.get());

  if (plugin && plugin->mStopped) {
    nsIPluginInstance* instance = plugin->mInstance;
    nsPluginWindow    *window = nsnull;
    aOwner->GetWindow(window);

    aOwner->SetInstance(instance);
    instance->SetOwner(aOwner);

    instance->Start();
    aOwner->CreateWidget();

    
    if (window->window) {
      nsCOMPtr<nsIPluginInstance> inst = instance;
      ((nsPluginNativeWindow*)window)->CallSetWindow(inst);
    }

    plugin->setStopped(PR_FALSE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsPluginHost::AddInstanceToActiveList(nsCOMPtr<nsIPlugin> aPlugin,
                                               nsIPluginInstance* aInstance,
                                               nsIURI* aURL,
                                               PRBool aDefaultPlugin)

{
  nsCAutoString url;
  
  
  if (aURL)
    aURL->GetSpec(url);

  
  
  
  nsPluginTag * pluginTag = nsnull;
  if (aPlugin) {
    for (pluginTag = mPlugins; pluginTag != nsnull; pluginTag = pluginTag->mNext) {
      if (pluginTag->mEntryPoint == aPlugin)
        break;
    }
    NS_ASSERTION(pluginTag, "Plugin tag not found");
  }

  nsPluginInstanceTag * plugin = new nsPluginInstanceTag(pluginTag, aInstance, url.get(), aDefaultPlugin);

  if (!plugin)
    return NS_ERROR_OUT_OF_MEMORY;

  mPluginInstanceTagList.add(plugin);
  return NS_OK;
}

void
nsPluginTag::RegisterWithCategoryManager(PRBool aOverrideInternalTypes,
                                         nsPluginTag::nsRegisterType aType)
{
  if (!mMimeTypeArray)
    return;

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginTag::RegisterWithCategoryManager plugin=%s, removing = %s\n",
  mFileName.get(), aType == ePluginUnregister ? "yes" : "no"));

  nsCOMPtr<nsICategoryManager> catMan = do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMan)
    return;

  const char *contractId = "@mozilla.org/content/plugin/document-loader-factory;1";

  nsCOMPtr<nsIPrefBranch> psvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!psvc)
    return; 

  
  
  
  
  
  
  
  nsXPIDLCString overrideTypes;
  psvc->GetCharPref("plugin.disable_full_page_plugin_for_types", getter_Copies(overrideTypes));
  nsCAutoString overrideTypesFormatted;
  overrideTypesFormatted.Assign(',');
  overrideTypesFormatted += overrideTypes;
  overrideTypesFormatted.Append(',');

  nsACString::const_iterator start, end;
  for (int i = 0; i < mVariants; i++) {
    if (aType == ePluginUnregister) {
      nsXPIDLCString value;
      if (NS_SUCCEEDED(catMan->GetCategoryEntry("Gecko-Content-Viewers",
                                                mMimeTypeArray[i],
                                                getter_Copies(value)))) {
        
        if (strcmp(value, contractId) == 0) {
          catMan->DeleteCategoryEntry("Gecko-Content-Viewers",
                                      mMimeTypeArray[i],
                                      PR_TRUE);
        }
      }
    } else {
      overrideTypesFormatted.BeginReading(start);
      overrideTypesFormatted.EndReading(end);
      
      nsDependentCString mimeType(mMimeTypeArray[i]);
      nsCAutoString commaSeparated; 
      commaSeparated.Assign(',');
      commaSeparated += mimeType;
      commaSeparated.Append(',');
      if (!FindInReadable(commaSeparated, start, end)) {
        catMan->AddCategoryEntry("Gecko-Content-Viewers",
                                 mMimeTypeArray[i],
                                 contractId,
                                 PR_FALSE, 
                                 aOverrideInternalTypes, 
                                 nsnull);
      }
    }

    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginTag::RegisterWithCategoryManager mime=%s, plugin=%s\n",
    mMimeTypeArray[i], mFileName.get()));
  }
}

NS_IMETHODIMP nsPluginHost::SetUpPluginInstance(const char *aMimeType,
                                                nsIURI *aURL,
                                                nsIPluginInstanceOwner *aOwner)
{
  nsresult rv = NS_OK;

  rv = TrySetUpPluginInstance(aMimeType, aURL, aOwner);

  
  
  if (NS_FAILED(rv)) {
    
    
    
    
    nsCOMPtr<nsIDocument> document;
    if (aOwner)
      aOwner->GetDocument(getter_AddRefs(document));

    nsCOMPtr<nsIDocument> currentdocument = do_QueryReferent(mCurrentDocument);
    if (document == currentdocument)
      return rv;

    mCurrentDocument = do_GetWeakReference(document);

    
    
    if (NS_ERROR_PLUGINS_PLUGINSNOTCHANGED == ReloadPlugins(PR_FALSE))
      return rv;

    
    aOwner->SetInstance(nsnull); 
    rv = TrySetUpPluginInstance(aMimeType, aURL, aOwner);
  }

  return rv;
}

nsresult
nsPluginHost::TrySetUpPluginInstance(const char *aMimeType,
                                     nsIURI *aURL,
                                     nsIPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (aURL != nsnull) aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::TrySetupPluginInstance Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif


  nsresult result = NS_ERROR_FAILURE;
  nsCOMPtr<nsIPluginInstance> instance;
  nsCOMPtr<nsIPlugin> plugin;
  const char* mimetype = nsnull;

  
  
  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
  if (!pluginTag) {
    nsCOMPtr<nsIURL> url = do_QueryInterface(aURL);
    if (!url) return NS_ERROR_FAILURE;

    nsCAutoString fileExtension;
    url->GetFileExtension(fileExtension);

    
    
    if (fileExtension.IsEmpty() ||
        !(pluginTag = FindPluginEnabledForExtension(fileExtension.get(),
                                                    mimetype))) {
      return NS_ERROR_FAILURE;
    }
  }
  else {
    mimetype = aMimeType;
  }

  NS_ASSERTION(pluginTag, "Must have plugin tag here!");

  GetPlugin(mimetype, getter_AddRefs(plugin));

  if (plugin) {
#if defined(XP_WIN) && !defined(WINCE)
    static BOOL firstJavaPlugin = FALSE;
    BOOL restoreOrigDir = FALSE;
    char origDir[_MAX_PATH];
    if (pluginTag->mIsJavaPlugin && !firstJavaPlugin) {
      DWORD dw = ::GetCurrentDirectory(_MAX_PATH, origDir);
      NS_ASSERTION(dw <= _MAX_PATH, "Falied to obtain the current directory, which may leads to incorrect class laoding");
      nsCOMPtr<nsIFile> binDirectory;
      result = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                      getter_AddRefs(binDirectory));

      if (NS_SUCCEEDED(result)) {
        nsCAutoString path;
        binDirectory->GetNativePath(path);
        restoreOrigDir = ::SetCurrentDirectory(path.get());
      }
    }
#endif
    result = plugin->CreatePluginInstance(getter_AddRefs(instance));

#if defined(XP_WIN) && !defined(WINCE)
    if (!firstJavaPlugin && restoreOrigDir) {
      BOOL bCheck = ::SetCurrentDirectory(origDir);
      NS_ASSERTION(bCheck, " Error restoring driectoy");
      firstJavaPlugin = TRUE;
    }
#endif
  }

  if (NS_FAILED(result))
    return result;

  
  aOwner->SetInstance(instance);

  result = instance->Initialize(aOwner, mimetype);  
  if (NS_FAILED(result))                
    return result;                      

  
  result = AddInstanceToActiveList(plugin, instance, aURL, PR_FALSE);

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if (aURL)
    aURL->GetSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
        ("nsPluginHost::TrySetupPluginInstance Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, result, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return result;
}

nsresult
nsPluginHost::SetUpDefaultPluginInstance(const char *aMimeType,
                                         nsIURI *aURL,
                                         nsIPluginInstanceOwner *aOwner)
{
  if (mDefaultPluginDisabled) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIPluginInstance> instance;
  nsCOMPtr<nsIPlugin> plugin = NULL;
  const char* mimetype = aMimeType;

  if (!aURL)
    return NS_ERROR_FAILURE;

  GetPlugin("*", getter_AddRefs(plugin));

  nsresult result = NS_ERROR_OUT_OF_MEMORY;
  if (plugin)
    result = plugin->CreatePluginInstance(getter_AddRefs(instance));
  if (NS_FAILED(result))
    return result;

  
  aOwner->SetInstance(instance);

  
  nsXPIDLCString mt;
  if (!mimetype || !*mimetype) {
    nsresult res = NS_OK;
    nsCOMPtr<nsIMIMEService> ms (do_GetService(NS_MIMESERVICE_CONTRACTID, &res));
    if (NS_SUCCEEDED(res)) {
      res = ms->GetTypeFromURI(aURL, mt);
      if (NS_SUCCEEDED(res))
        mimetype = mt.get();
    }
  }

  
  result = instance->Initialize(aOwner, mimetype);
  if (NS_FAILED(result))
    return result;

  
  result = AddInstanceToActiveList(plugin, instance, aURL, PR_TRUE);

  return result;
}

NS_IMETHODIMP
nsPluginHost::IsPluginEnabledForType(const char* aMimeType)
{
  
  
  nsPluginTag *plugin = FindPluginForType(aMimeType, PR_FALSE);
  if (!plugin)
    return NS_ERROR_FAILURE;

  if (!plugin->IsEnabled()) {
    if (plugin->HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED))
      return NS_ERROR_PLUGIN_BLOCKLISTED;
    else
      return NS_ERROR_PLUGIN_DISABLED;
  }

  return NS_OK;
}


static int CompareExtensions(const char *aExtensionList, const char *aExtension)
{
  if (!aExtensionList || !aExtension)
    return -1;

  const char *pExt = aExtensionList;
  const char *pComma = strchr(pExt, ',');
  if (!pComma)
    return PL_strcasecmp(pExt, aExtension);

  int extlen = strlen(aExtension);
  while (pComma) {
    int length = pComma - pExt;
    if (length == extlen && 0 == PL_strncasecmp(aExtension, pExt, length))
      return 0;
    pComma++;
    pExt = pComma;
    pComma = strchr(pExt, ',');
  }

  
  return PL_strcasecmp(pExt, aExtension);
}

NS_IMETHODIMP
nsPluginHost::IsPluginEnabledForExtension(const char* aExtension,
                                          const char* &aMimeType)
{
  nsPluginTag *plugin = FindPluginEnabledForExtension(aExtension, aMimeType);
  return plugin ? NS_OK : NS_ERROR_FAILURE;
}

class DOMMimeTypeImpl : public nsIDOMMimeType {
public:
  NS_DECL_ISUPPORTS

  DOMMimeTypeImpl(nsPluginTag* aTag, PRUint32 aMimeTypeIndex)
  {
    if (!aTag)
      return;
    CopyUTF8toUTF16(aTag->mMimeDescriptionArray[aMimeTypeIndex], mDescription);
    if (aTag->mExtensionsArray)
      CopyUTF8toUTF16(aTag->mExtensionsArray[aMimeTypeIndex], mSuffixes);
    if (aTag->mMimeTypeArray)
      CopyUTF8toUTF16(aTag->mMimeTypeArray[aMimeTypeIndex], mType);
  }

  virtual ~DOMMimeTypeImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    aDescription.Assign(mDescription);
    return NS_OK;
  }

  NS_METHOD GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
  {
    
    *aEnabledPlugin = nsnull;
    return NS_OK;
  }

  NS_METHOD GetSuffixes(nsAString& aSuffixes)
  {
    aSuffixes.Assign(mSuffixes);
    return NS_OK;
  }

  NS_METHOD GetType(nsAString& aType)
  {
    aType.Assign(mType);
    return NS_OK;
  }

private:
  nsString mDescription;
  nsString mSuffixes;
  nsString mType;
};

NS_IMPL_ISUPPORTS1(DOMMimeTypeImpl, nsIDOMMimeType)

class DOMPluginImpl : public nsIDOMPlugin {
public:
  NS_DECL_ISUPPORTS

  DOMPluginImpl(nsPluginTag* aPluginTag) : mPluginTag(aPluginTag)
  {
  }

  virtual ~DOMPluginImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    CopyUTF8toUTF16(mPluginTag.mDescription, aDescription);
    return NS_OK;
  }

  NS_METHOD GetFilename(nsAString& aFilename)
  {
    PRBool bShowPath;
    nsCOMPtr<nsIPrefBranch> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefService &&
        NS_SUCCEEDED(prefService->GetBoolPref("plugin.expose_full_path", &bShowPath)) &&
        bShowPath) {
      CopyUTF8toUTF16(mPluginTag.mFullPath, aFilename);
    } else {
      CopyUTF8toUTF16(mPluginTag.mFileName, aFilename);
    }

    return NS_OK;
  }

  NS_METHOD GetVersion(nsAString& aVersion)
  {
    CopyUTF8toUTF16(mPluginTag.mVersion, aVersion);
    return NS_OK;
  }

  NS_METHOD GetName(nsAString& aName)
  {
    CopyUTF8toUTF16(mPluginTag.mName, aName);
    return NS_OK;
  }

  NS_METHOD GetLength(PRUint32* aLength)
  {
    *aLength = mPluginTag.mVariants;
    return NS_OK;
  }

  NS_METHOD Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)
  {
    nsIDOMMimeType* mimeType = new DOMMimeTypeImpl(&mPluginTag, aIndex);
    NS_IF_ADDREF(mimeType);
    *aReturn = mimeType;
    return NS_OK;
  }

  NS_METHOD NamedItem(const nsAString& aName, nsIDOMMimeType** aReturn)
  {
    for (int i = mPluginTag.mVariants - 1; i >= 0; --i) {
      if (aName.Equals(NS_ConvertUTF8toUTF16(mPluginTag.mMimeTypeArray[i])))
        return Item(i, aReturn);
    }
    return NS_OK;
  }

private:
  nsPluginTag mPluginTag;
};

NS_IMPL_ISUPPORTS1(DOMPluginImpl, nsIDOMPlugin)

NS_IMETHODIMP
nsPluginHost::GetPluginCount(PRUint32* aPluginCount)
{
  LoadPlugins();

  PRUint32 count = 0;

  nsPluginTag* plugin = mPlugins;
  while (plugin != nsnull) {
    if (plugin->IsEnabled()) {
      ++count;
    }
    plugin = plugin->mNext;
  }

  *aPluginCount = count;

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin** aPluginArray)
{
  LoadPlugins();

  nsPluginTag* plugin = mPlugins;
  for (PRUint32 i = 0; i < aPluginCount && plugin; plugin = plugin->mNext) {
    if (plugin->IsEnabled()) {
      nsIDOMPlugin* domPlugin = new DOMPluginImpl(plugin);
      NS_IF_ADDREF(domPlugin);
      aPluginArray[i++] = domPlugin;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPluginTags(PRUint32* aPluginCount, nsIPluginTag*** aResults)
{
  LoadPlugins();

  PRUint32 count = 0;
  nsRefPtr<nsPluginTag> plugin = mPlugins;
  while (plugin != nsnull) {
    count++;
    plugin = plugin->mNext;
  }

  *aResults = static_cast<nsIPluginTag**>
                         (nsMemory::Alloc(count * sizeof(**aResults)));
  if (!*aResults)
    return NS_ERROR_OUT_OF_MEMORY;

  *aPluginCount = count;

  plugin = mPlugins;
  PRUint32 i;
  for (i = 0; i < count; i++) {
    (*aResults)[i] = plugin;
    NS_ADDREF((*aResults)[i]);
    plugin = plugin->mNext;
  }

  return NS_OK;
}

nsPluginTag*
nsPluginHost::FindPluginForType(const char* aMimeType,
                                PRBool aCheckEnabled)
{
  nsPluginTag *plugins = nsnull;
  PRInt32     variants, cnt;

  LoadPlugins();

  
  
  if (nsnull != aMimeType) {
    plugins = mPlugins;

    while (nsnull != plugins) {
      variants = plugins->mVariants;
      for (cnt = 0; cnt < variants; cnt++) {
        if ((!aCheckEnabled || plugins->IsEnabled()) &&
            plugins->mMimeTypeArray[cnt] &&
            (0 == PL_strcasecmp(plugins->mMimeTypeArray[cnt], aMimeType))) {
          return plugins;
        }
      }

      plugins = plugins->mNext;
    }
  }

  return nsnull;
}

nsPluginTag*
nsPluginHost::FindPluginEnabledForExtension(const char* aExtension,
                                            const char*& aMimeType)
{
  nsPluginTag *plugins = nsnull;
  PRInt32     variants, cnt;

  LoadPlugins();

  
  
  if (aExtension) {
    plugins = mPlugins;
    while (plugins) {
      variants = plugins->mVariants;
      if (plugins->mExtensionsArray) {
        for (cnt = 0; cnt < variants; cnt++) {
          
          
          if (plugins->IsEnabled() &&
              0 == CompareExtensions(plugins->mExtensionsArray[cnt], aExtension)) {
            aMimeType = plugins->mMimeTypeArray[cnt];
            return plugins;
          }
        }
      }

      plugins = plugins->mNext;
    }
  }

  return nsnull;
}

static nsresult ConvertToNative(nsIUnicodeEncoder *aEncoder,
                                const nsACString& aUTF8String,
                                nsACString& aNativeString)
{
  NS_ConvertUTF8toUTF16 utf16(aUTF8String);
  PRInt32 len = utf16.Length();
  PRInt32 outLen;
  nsresult rv = aEncoder->GetMaxLength(utf16.get(), len, &outLen);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!EnsureStringLength(aNativeString, outLen))
    return NS_ERROR_OUT_OF_MEMORY;
  rv = aEncoder->Convert(utf16.get(), &len,
                         aNativeString.BeginWriting(), &outLen);
  NS_ENSURE_SUCCESS(rv, rv);
  aNativeString.SetLength(outLen);
  return NS_OK;
}

static nsresult CreateNPAPIPlugin(const nsPluginTag *aPluginTag,
                                  nsIPlugin **aOutNPAPIPlugin)
{
  nsresult rv;
  nsCOMPtr <nsIPlatformCharset> pcs =
    do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString charset;
  rv = pcs->GetCharset(kPlatformCharsetSel_FileName, charset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString fullPath;
  if (!charset.LowerCaseEqualsLiteral("utf-8")) {
    nsCOMPtr<nsIUnicodeEncoder> encoder;
    nsCOMPtr<nsICharsetConverterManager> ccm =
      do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ccm->GetUnicodeEncoderRaw(charset.get(), getter_AddRefs(encoder));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ConvertToNative(encoder, aPluginTag->mFullPath, fullPath);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    fullPath = aPluginTag->mFullPath;
  }

  return nsNPAPIPlugin::CreatePlugin(fullPath.get(),
                                     aPluginTag->mLibrary,
                                     aOutNPAPIPlugin);
}

NS_IMETHODIMP nsPluginHost::GetPlugin(const char *aMimeType, nsIPlugin** aPlugin)
{
  nsresult rv = NS_ERROR_FAILURE;
  *aPlugin = NULL;

  if (!aMimeType)
    return NS_ERROR_ILLEGAL_VALUE;

  
  LoadPlugins();

  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
  if (pluginTag) {
    rv = NS_OK;
    PLUGIN_LOG(PLUGIN_LOG_BASIC,
    ("nsPluginHost::GetPlugin Begin mime=%s, plugin=%s\n",
    aMimeType, pluginTag->mFileName.get()));

#ifdef NS_DEBUG
    if (aMimeType && !pluginTag->mFileName.IsEmpty())
      printf("For %s found plugin %s\n", aMimeType, pluginTag->mFileName.get());
#endif

    if (!pluginTag->mLibrary) { 
      if (pluginTag->mFullPath.IsEmpty())
        return NS_ERROR_FAILURE;
      nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
      file->InitWithPath(NS_ConvertUTF8toUTF16(pluginTag->mFullPath));
      nsPluginFile pluginFile(file);
      PRLibrary* pluginLibrary = NULL;

      if (pluginFile.LoadPlugin(pluginLibrary) != NS_OK || pluginLibrary == NULL)
        return NS_ERROR_FAILURE;

      
      if (mUnusedLibraries.Contains(pluginLibrary))
        mUnusedLibraries.RemoveElement(pluginLibrary);

      pluginTag->mLibrary = pluginLibrary;
    }

    nsIPlugin* plugin = pluginTag->mEntryPoint;
    if (!plugin) {
      
      rv = CreateNPAPIPlugin(pluginTag, &plugin);
      if (NS_SUCCEEDED(rv))
        pluginTag->mEntryPoint = plugin;
    }

    if (plugin) {
      *aPlugin = plugin;
      plugin->AddRef();
      return NS_OK;
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::GetPlugin End mime=%s, rv=%d, plugin=%p name=%s\n",
  aMimeType, rv, *aPlugin,
  (pluginTag ? pluginTag->mFileName.get() : "(not found)")));

  return rv;
}




static PRBool isUnwantedPlugin(nsPluginTag * tag)
{
  if (tag->mFileName.IsEmpty())
    return PR_TRUE;

  for (PRInt32 i = 0; i < tag->mVariants; ++i) {
    if (nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/pdf"))
      return PR_FALSE;

    if (nsnull == PL_strcasecmp(tag->mMimeTypeArray[i], "application/x-shockwave-flash"))
      return PR_FALSE;

    if (nsnull == PL_strcasecmp(tag->mMimeTypeArray[i],"application/x-director"))
      return PR_FALSE;
  }

  
  
  if (tag->mFileName.Find("npqtplugin", PR_TRUE, 0, -1) != kNotFound)
    return PR_FALSE;

  return PR_TRUE;
}

PRBool nsPluginHost::IsJavaMIMEType(const char* aType)
{
  return aType &&
    ((0 == PL_strncasecmp(aType, "application/x-java-vm",
                          sizeof("application/x-java-vm") - 1)) ||
     (0 == PL_strncasecmp(aType, "application/x-java-applet",
                          sizeof("application/x-java-applet") - 1)) ||
     (0 == PL_strncasecmp(aType, "application/x-java-bean",
                          sizeof("application/x-java-bean") - 1)));
}

nsPluginTag * nsPluginHost::HaveSamePlugin(nsPluginTag * aPluginTag)
{
  for (nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->Equals(aPluginTag))
      return tag;
  }
  return nsnull;
}

PRBool nsPluginHost::IsDuplicatePlugin(nsPluginTag * aPluginTag)
{
  nsPluginTag * tag = HaveSamePlugin(aPluginTag);
  if (tag) {
    

    
    
    
    if (!tag->mFileName.Equals(aPluginTag->mFileName))
      return PR_TRUE;

    
    
    if (!tag->mFullPath.Equals(aPluginTag->mFullPath))
      return PR_TRUE;
  }

  
  return PR_FALSE;
}


struct pluginFileinDirectory
{
  nsString mFilePath;
  PRInt64  mModTime;

  pluginFileinDirectory()
  {
    mModTime = LL_ZERO;
  }
};




NS_SPECIALIZE_TEMPLATE
class nsDefaultComparator<pluginFileinDirectory, pluginFileinDirectory>
{
  public:
  PRBool Equals(const pluginFileinDirectory& aA,
                const pluginFileinDirectory& aB) const {
    if (aA.mModTime == aB.mModTime &&
        Compare(aA.mFilePath, aB.mFilePath,
                nsCaseInsensitiveStringComparator()) == 0)
      return PR_TRUE;
    else
      return PR_FALSE;
  }
  PRBool LessThan(const pluginFileinDirectory& aA,
                  const pluginFileinDirectory& aB) const {
    if (aA.mModTime < aB.mModTime)
      return PR_TRUE;
    else if(aA.mModTime == aB.mModTime)
      return Compare(aA.mFilePath, aB.mFilePath,
                     nsCaseInsensitiveStringComparator()) < 0;
    else
      return PR_FALSE;
  }
};

typedef NS_NPAPIPLUGIN_CALLBACK(char *, NP_GETMIMEDESCRIPTION)(void);

static nsresult FixUpPluginInfo(nsPluginInfo &aInfo, nsPluginFile &aPluginFile)
{
#ifndef XP_WIN
  return NS_OK;
#endif

  for (PRUint32 i = 0; i < aInfo.fVariantCount; i++) {
    if (PL_strcmp(aInfo.fMimeTypeArray[i], "*"))
      continue;

    
    
    
    PRLibrary *library = nsnull;
    if (NS_FAILED(aPluginFile.LoadPlugin(library)) || !library)
      return NS_ERROR_FAILURE;

    NP_GETMIMEDESCRIPTION pf = (NP_GETMIMEDESCRIPTION)PR_FindFunctionSymbol(library, "NP_GetMIMEDescription");

    if (pf) {
      
      char * mimedescription = pf();
      if (!PL_strncmp(mimedescription, NS_PLUGIN_DEFAULT_MIME_DESCRIPTION, 1))
        return NS_OK;
    }

    
    

    
    PL_strfree(aInfo.fMimeTypeArray[i]);
    aInfo.fMimeTypeArray[i] = PL_strdup("[*]");

    
  }
  return NS_OK;
}

nsresult nsPluginHost::ScanPluginsDirectory(nsIFile * pluginsDir,
                                            nsIComponentManager * compManager,
                                            PRBool aCreatePluginList,
                                            PRBool * aPluginsChanged,
                                            PRBool checkForUnwantedPlugins)
{
  NS_ENSURE_ARG_POINTER(aPluginsChanged);
  nsresult rv;

  *aPluginsChanged = PR_FALSE;

#ifdef PLUGIN_LOGGING
  nsCAutoString dirPath;
  pluginsDir->GetNativePath(dirPath);
  PLUGIN_LOG(PLUGIN_LOG_BASIC,
  ("nsPluginHost::ScanPluginsDirectory dir=%s\n", dirPath.get()));
#endif

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = pluginsDir->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv))
    return rv;

  
  nsAutoTArray<pluginFileinDirectory, 8> pluginFilesArray;

  PRBool hasMore;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;
    nsCOMPtr<nsILocalFile> dirEntry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv))
      continue;

    
    
    dirEntry->Normalize();

    nsAutoString filePath;
    rv = dirEntry->GetPath(filePath);
    if (NS_FAILED(rv))
      continue;

    if (nsPluginsDir::IsPluginFile(dirEntry)) {
      pluginFileinDirectory * item = pluginFilesArray.AppendElement();
      if (!item)
        return NS_ERROR_OUT_OF_MEMORY;

      
      PRInt64 fileModTime = LL_ZERO;
      dirEntry->GetLastModifiedTime(&fileModTime);

      item->mModTime = fileModTime;
      item->mFilePath = filePath;
    }
  } 

  
  
  pluginFilesArray.Sort();

  
  for (PRUint32 i = 0; i < pluginFilesArray.Length(); i++) {
    pluginFileinDirectory &pfd = pluginFilesArray[i];
    nsCOMPtr <nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    nsCOMPtr <nsILocalFile> localfile = do_QueryInterface(file);
    localfile->InitWithPath(pfd.mFilePath);
    PRInt64 fileModTime = pfd.mModTime;

    
    nsRefPtr<nsPluginTag> pluginTag;
    RemoveCachedPluginsInfo(NS_ConvertUTF16toUTF8(pfd.mFilePath).get(),
                            getter_AddRefs(pluginTag));

    PRBool enabled = PR_TRUE;
    PRBool seenBefore = PR_FALSE;
    if (pluginTag) {
      seenBefore = PR_TRUE;
      
      if (LL_NE(fileModTime, pluginTag->mLastModifiedTime)) {
        
        enabled = (pluginTag->Flags() & NS_PLUGIN_FLAG_ENABLED) != 0;
        pluginTag = nsnull;

        
        *aPluginsChanged = PR_TRUE;
      }
      else {
        
        
        if ((checkForUnwantedPlugins && isUnwantedPlugin(pluginTag)) ||
           IsDuplicatePlugin(pluginTag)) {
          if (!pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED)) {
            
            *aPluginsChanged = PR_TRUE;
          }
          pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
          pluginTag->mNext = mCachedPlugins;
          mCachedPlugins = pluginTag;
        } else if (pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED)) {
          pluginTag->UnMark(NS_PLUGIN_FLAG_UNWANTED);
          
          *aPluginsChanged = PR_TRUE;
        }
      }
    }
    else {
      
      *aPluginsChanged = PR_TRUE;
    }

    
    
    if (!aCreatePluginList) {
      if (*aPluginsChanged)
        return NS_OK;
      else
        continue;
    }

    
    if (!pluginTag) {
      nsPluginFile pluginFile(file);
      PRLibrary* pluginLibrary = nsnull;

      
#ifndef XP_WIN
      if (pluginFile.LoadPlugin(pluginLibrary) != NS_OK || pluginLibrary == nsnull)
        continue;
#endif

      
      nsPluginInfo info;
      memset(&info, 0, sizeof(info));
      nsresult res = pluginFile.GetPluginInfo(info);
      
      if (NS_FAILED(res) || !info.fMimeTypeArray) {
        pluginFile.FreePluginInfo(info);
        continue;
      }

      
      
      
      if (!mAllowAlienStarHandler)
        FixUpPluginInfo(info, pluginFile);

      pluginTag = new nsPluginTag(&info);
      pluginFile.FreePluginInfo(info);

      if (pluginTag == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

      pluginTag->mLibrary = pluginLibrary;
      pluginTag->mLastModifiedTime = fileModTime;

      nsCOMPtr<nsIBlocklistService> blocklist = do_GetService("@mozilla.org/extensions/blocklist;1");
      if (blocklist) {
        PRUint32 state;
        rv = blocklist->GetPluginBlocklistState(pluginTag, EmptyString(),
                                                EmptyString(), &state);

        if (NS_SUCCEEDED(rv)) {
          
          
          if (state == nsIBlocklistService::STATE_BLOCKED)
            pluginTag->Mark(NS_PLUGIN_FLAG_BLOCKLISTED);
          else if (state == nsIBlocklistService::STATE_SOFTBLOCKED && !seenBefore)
            enabled = PR_FALSE;
        }
      }

      if (!enabled)
        pluginTag->UnMark(NS_PLUGIN_FLAG_ENABLED);

      
      
      
      NS_ASSERTION(!pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED),
                   "Brand-new tags should not be unwanted");
      if ((checkForUnwantedPlugins && isUnwantedPlugin(pluginTag)) ||
         IsDuplicatePlugin(pluginTag)) {
        pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
        pluginTag->mNext = mCachedPlugins;
        mCachedPlugins = pluginTag;
      }
    }

    
    
    PRBool bAddIt = PR_TRUE;

    
    if (checkForUnwantedPlugins && isUnwantedPlugin(pluginTag))
      bAddIt = PR_FALSE;

    
    
    if (bAddIt) {
      if (HaveSamePlugin(pluginTag)) {
        
        
        
        bAddIt = PR_FALSE;
      }
    }

    
    if (bAddIt) {
      pluginTag->SetHost(this);
      pluginTag->mNext = mPlugins;
      mPlugins = pluginTag;

      if (pluginTag->IsEnabled())
        pluginTag->RegisterWithCategoryManager(mOverrideInternalTypes);
    }
  }
  return NS_OK;
}

nsresult nsPluginHost::ScanPluginsDirectoryList(nsISimpleEnumerator * dirEnum,
                                                nsIComponentManager * compManager,
                                                PRBool aCreatePluginList,
                                                PRBool * aPluginsChanged,
                                                PRBool checkForUnwantedPlugins)
{
    PRBool hasMore;
    while (NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supports;
      nsresult rv = dirEnum->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv))
        continue;
      nsCOMPtr<nsIFile> nextDir(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv))
        continue;

      
      PRBool pluginschanged = PR_FALSE;
      ScanPluginsDirectory(nextDir, compManager, aCreatePluginList, &pluginschanged, checkForUnwantedPlugins);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      if (!aCreatePluginList && *aPluginsChanged)
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsPluginHost::LoadPlugins()
{
  
  
  if (mPluginsLoaded)
    return NS_OK;

  PRBool pluginschanged;
  nsresult rv = FindPlugins(PR_TRUE, &pluginschanged);
  if (NS_FAILED(rv))
    return rv;

  
  if (pluginschanged) {
    nsCOMPtr<nsIObserverService>
      obsService(do_GetService("@mozilla.org/observer-service;1"));
    if (obsService)
      obsService->NotifyObservers(nsnull, "plugins-list-updated", nsnull);
  }

  return NS_OK;
}

#include "nsITimelineService.h"




nsresult nsPluginHost::FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged)
{
  
  if (aCreatePluginList) {
    NS_TIMELINE_START_TIMER("LoadPlugins");
  }

#ifdef CALL_SAFETY_ON
  
  NS_INIT_PLUGIN_SAFE_CALLS;
#endif

  NS_ENSURE_ARG_POINTER(aPluginsChanged);

  *aPluginsChanged = PR_FALSE;
  nsresult rv;

  
  ReadPluginInfo();

  nsCOMPtr<nsIComponentManager> compManager;
  NS_GetComponentManager(getter_AddRefs(compManager));

#ifdef XP_WIN
  
  rv = EnsurePrivateDirServiceProvider();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to register dir service provider.");
#endif 

  nsCOMPtr<nsIProperties> dirService(do_GetService(kDirectoryServiceContractID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> dirList;

  
  
  
  PRBool pluginschanged = PR_FALSE;

  
  rv = dirService->Get(NS_APP_PLUGINS_DIR_LIST, NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirList));
  if (NS_SUCCEEDED(rv)) {
    ScanPluginsDirectoryList(dirList, compManager, aCreatePluginList, &pluginschanged);

    if (pluginschanged)
      *aPluginsChanged = PR_TRUE;

    
    
    if (!aCreatePluginList && *aPluginsChanged) {
      NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
      return NS_OK;
    }
  }

  mPluginsLoaded = PR_TRUE; 
                            

#ifdef XP_WIN
  PRBool bScanPLIDs = PR_FALSE;

  if (mPrefService)
    mPrefService->GetBoolPref("plugin.scan.plid.all", &bScanPLIDs);

    
  if (bScanPLIDs && mPrivateDirServiceProvider) {
    rv = mPrivateDirServiceProvider->GetPLIDDirectories(getter_AddRefs(dirList));
    if (NS_SUCCEEDED(rv)) {
      ScanPluginsDirectoryList(dirList, compManager, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        return NS_OK;
      }
    }
  }


  

  
  const char* const prefs[] = {NS_WIN_JRE_SCAN_KEY,         nsnull,
                               NS_WIN_ACROBAT_SCAN_KEY,     nsnull,
                               NS_WIN_QUICKTIME_SCAN_KEY,   nsnull,
                               NS_WIN_WMP_SCAN_KEY,         nsnull,
                               NS_WIN_4DOTX_SCAN_KEY,       "1"   };

  PRUint32 size = sizeof(prefs) / sizeof(prefs[0]);

  for (PRUint32 i = 0; i < size; i+=2) {
    nsCOMPtr<nsIFile> dirToScan;
    PRBool bExists;
    if (NS_SUCCEEDED(dirService->Get(prefs[i], NS_GET_IID(nsIFile), getter_AddRefs(dirToScan))) &&
        dirToScan &&
        NS_SUCCEEDED(dirToScan->Exists(&bExists)) &&
        bExists) {

      PRBool bFilterUnwanted = PR_FALSE;

      
      
      
      if (prefs[i+1]) {
        PRBool bScanEverything;
        bFilterUnwanted = PR_TRUE;  
        if (mPrefService &&
            NS_SUCCEEDED(mPrefService->GetBoolPref(prefs[i], &bScanEverything)) &&
            bScanEverything)
          bFilterUnwanted = PR_FALSE;

      }
      ScanPluginsDirectory(dirToScan, compManager, aCreatePluginList, &pluginschanged, bFilterUnwanted);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        return NS_OK;
      }
    }
  }
#endif

  
  
  
  if (!*aPluginsChanged) {
    
    
    PRUint32 cachecount = 0;
    for (nsPluginTag * cachetag = mCachedPlugins; cachetag; cachetag = cachetag->mNext) {
      if (!cachetag->HasFlag(NS_PLUGIN_FLAG_UNWANTED))
        cachecount++;
    }
    
    
    
    if (cachecount > 0)
      *aPluginsChanged = PR_TRUE;
  }

  
  if (!aCreatePluginList) {
    NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
    return NS_OK;
  }

  
  
  if (*aPluginsChanged)
    WritePluginInfo();

  
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);

  
  nsRefPtr<nsPluginTag> next;
  nsRefPtr<nsPluginTag> prev;
  for (nsRefPtr<nsPluginTag> cur = mPlugins; cur; cur = next) {
    next = cur->mNext;
    cur->mNext = prev;
    prev = cur;
  }

  mPlugins = prev;

  NS_TIMELINE_STOP_TIMER("LoadPlugins");
  NS_TIMELINE_MARK_TIMER("LoadPlugins");

  return NS_OK;
}

nsresult
nsPluginHost::UpdatePluginInfo(nsPluginTag* aPluginTag)
{
  ReadPluginInfo();
  WritePluginInfo();
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);

  if (!aPluginTag || aPluginTag->IsEnabled())
    return NS_OK;

  nsCOMPtr<nsISupportsArray> instsToReload;
  NS_NewISupportsArray(getter_AddRefs(instsToReload));
  mPluginInstanceTagList.stopRunning(instsToReload, aPluginTag);
  mPluginInstanceTagList.removeAllStopped();
  
  PRUint32 c;
  if (instsToReload && NS_SUCCEEDED(instsToReload->Count(&c)) && c > 0) {
    nsCOMPtr<nsIRunnable> ev = new nsPluginDocReframeEvent(instsToReload);
    if (ev)
      NS_DispatchToCurrentThread(ev);
  }

  return NS_OK;
}

nsresult
nsPluginHost::WritePluginInfo()
{

  nsresult rv = NS_OK;
  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,&rv));
  if (NS_FAILED(rv))
    return rv;

  directoryService->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                        getter_AddRefs(mPluginRegFile));

  if (!mPluginRegFile)
    return NS_ERROR_FAILURE;

  PRFileDesc* fd = nsnull;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  rv = pluginReg->AppendNative(kPluginRegistryFilename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(pluginReg, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = localFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
  if (NS_FAILED(rv))
    return rv;

  PR_fprintf(fd, "Generated File. Do not edit.\n");

  PR_fprintf(fd, "\n[HEADER]\nVersion%c%s%c%c\n",
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             kPluginRegistryVersion,
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             PLUGIN_REGISTRY_END_OF_LINE_MARKER);

  
  PR_fprintf(fd, "\n[PLUGINS]\n");

  nsPluginTag *taglist[] = {mPlugins, mCachedPlugins};
  for (int i=0; i<(int)(sizeof(taglist)/sizeof(nsPluginTag *)); i++) {
    for (nsPluginTag *tag = taglist[i]; tag; tag=tag->mNext) {
      
      if ((taglist[i] == mCachedPlugins) && !tag->HasFlag(NS_PLUGIN_FLAG_UNWANTED))
        continue;
      
      
      
      PR_fprintf(fd, "%s%c%c\n%s%c%c\n%s%c%c\n",
        (!tag->mFileName.IsEmpty() ? tag->mFileName.get() : ""),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (!tag->mFullPath.IsEmpty() ? tag->mFullPath.get() : ""),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (!tag->mVersion.IsEmpty() ? tag->mVersion.get() : ""),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER);

      
      PR_fprintf(fd, "%lld%c%d%c%lu%c%c\n",
        tag->mLastModifiedTime,
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        tag->mCanUnloadLibrary,
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        tag->Flags(),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER);

      
      PR_fprintf(fd, "%s%c%c\n%s%c%c\n%d\n",
        (!tag->mDescription.IsEmpty() ? tag->mDescription.get() : ""),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (!tag->mName.IsEmpty() ? tag->mName.get() : ""),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        tag->mVariants + (tag->mIsNPRuntimeEnabledJavaPlugin ? 1 : 0));

      
      for (int i=0; i<tag->mVariants; i++) {
        PR_fprintf(fd, "%d%c%s%c%s%c%s%c%c\n",
          i,PLUGIN_REGISTRY_FIELD_DELIMITER,
          (tag->mMimeTypeArray && tag->mMimeTypeArray[i] ? tag->mMimeTypeArray[i] : ""),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          (!tag->mMimeDescriptionArray[i].IsEmpty() ? tag->mMimeDescriptionArray[i].get() : ""),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          (tag->mExtensionsArray && tag->mExtensionsArray[i] ? tag->mExtensionsArray[i] : ""),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          PLUGIN_REGISTRY_END_OF_LINE_MARKER);
      }

      if (tag->mIsNPRuntimeEnabledJavaPlugin) {
        PR_fprintf(fd, "%d%c%s%c%s%c%s%c%c\n",
          tag->mVariants, PLUGIN_REGISTRY_FIELD_DELIMITER,
          "application/x-java-vm-npruntime",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          "",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          "",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          PLUGIN_REGISTRY_END_OF_LINE_MARKER);
      }
    }
  }

  if (fd)
    PR_Close(fd);
  return NS_OK;
}

#define PLUGIN_REG_MIMETYPES_ARRAY_SIZE 12
nsresult
nsPluginHost::ReadPluginInfo()
{
  nsresult rv;

  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,&rv));
  if (NS_FAILED(rv))
    return rv;

  directoryService->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                        getter_AddRefs(mPluginRegFile));

  if (!mPluginRegFile)
    return NS_ERROR_FAILURE;

  PRFileDesc* fd = nsnull;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  rv = pluginReg->AppendNative(kPluginRegistryFilename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(pluginReg, &rv);
  if (NS_FAILED(rv))
    return rv;

  PRInt64 fileSize;
  rv = localFile->GetFileSize(&fileSize);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 flen = nsInt64(fileSize);
  if (flen == 0) {
    NS_WARNING("Plugins Registry Empty!");
    return NS_OK; 
  }

  nsPluginManifestLineReader reader;
  char* registry = reader.Init(flen);
  if (!registry)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd);
  if (NS_FAILED(rv))
    return rv;

  
  rv = NS_ERROR_FAILURE;

  PRInt32 bread = PR_Read(fd, registry, flen);
  PR_Close(fd);

  if (flen > bread)
    return rv;

  if (!ReadSectionHeader(reader, "HEADER"))
    return rv;;

  if (!reader.NextLine())
    return rv;

  char* values[6];

  
  if (2 != reader.ParseLine(values, 2))
    return rv;

  
  if (PL_strcmp(values[0], "Version"))
    return rv;

  
  PRInt32 vdiff = NS_CompareVersions(values[1], kPluginRegistryVersion);
  
  if (vdiff > 0)
    return rv;
  
  if (NS_CompareVersions(values[1], kMinimumRegistryVersion) < 0)
    return rv;

  
  PRBool regHasVersion = NS_CompareVersions(values[1], "0.10") >= 0;

  if (!ReadSectionHeader(reader, "PLUGINS"))
    return rv;

#if defined(XP_MACOSX)
  PRBool hasFullPathInFileNameField = PR_FALSE;
#else
  PRBool hasFullPathInFileNameField = (NS_CompareVersions(values[1], "0.11") < 0);
#endif

  while (reader.NextLine()) {
    const char *filename;
    const char *fullpath;
    nsCAutoString derivedFileName;
    
    if (hasFullPathInFileNameField) {
      fullpath = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
      
      if (fullpath) {
        nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
        file->InitWithNativePath(nsDependentCString(fullpath));
        file->GetNativeLeafName(derivedFileName);
        filename = derivedFileName.get();
      } else {
        filename = NULL;
      }

      
      if (!reader.NextLine())
        return rv;
    } else {
      filename = reader.LinePtr();
      if (!reader.NextLine())
        return rv;

      fullpath = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
    }

    const char *version;
    if (regHasVersion) {
      version = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
    } else {
      version = "0";
    }

    
    if (reader.ParseLine(values, 3) != 3)
      return rv;

    
    PRInt64 lastmod = (vdiff == 0) ? nsCRT::atoll(values[0]) : -1;
    PRBool canunload = atoi(values[1]);
    PRUint32 tagflag = atoi(values[2]);
    if (!reader.NextLine())
      return rv;

    const char *description = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

    const char *name = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

    int mimetypecount = atoi(reader.LinePtr());

    char *stackalloced[PLUGIN_REG_MIMETYPES_ARRAY_SIZE * 3];
    char **mimetypes;
    char **mimedescriptions;
    char **extensions;
    char **heapalloced = 0;
    if (mimetypecount > PLUGIN_REG_MIMETYPES_ARRAY_SIZE - 1) {
      heapalloced = new char *[mimetypecount * 3];
      mimetypes = heapalloced;
    } else {
      mimetypes = stackalloced;
    }
    mimedescriptions = mimetypes + mimetypecount;
    extensions = mimedescriptions + mimetypecount;

    int mtr = 0; 
    for (; mtr < mimetypecount; mtr++) {
      if (!reader.NextLine())
        break;

      
      if (4 != reader.ParseLine(values, 4))
        break;
      int line = atoi(values[0]);
      if (line != mtr)
        break;
      mimetypes[mtr] = values[1];
      mimedescriptions[mtr] = values[2];
      extensions[mtr] = values[3];
    }

    if (mtr != mimetypecount) {
      if (heapalloced) {
        delete [] heapalloced;
      }
      return rv;
    }

    nsRefPtr<nsPluginTag> tag = new nsPluginTag(name,
      description,
      filename,
      fullpath,
      version,
      (const char* const*)mimetypes,
      (const char* const*)mimedescriptions,
      (const char* const*)extensions,
      mimetypecount, lastmod, canunload, PR_TRUE);
    if (heapalloced)
      delete [] heapalloced;

    if (!tag)
      continue;

    
    tag->Mark(tagflag | NS_PLUGIN_FLAG_FROMCACHE);
    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
      ("LoadCachedPluginsInfo : Loading Cached plugininfo for %s\n", tag->mFileName.get()));
    tag->mNext = mCachedPlugins;
    mCachedPlugins = tag;

  }
  return NS_OK;
}

void
nsPluginHost::RemoveCachedPluginsInfo(const char *filePath, nsPluginTag **result)
{
  nsRefPtr<nsPluginTag> prev;
  nsRefPtr<nsPluginTag> tag = mCachedPlugins;
  while (tag)
  {
    if (tag->mFullPath.Equals(filePath)) {
      
      if (prev)
        prev->mNext = tag->mNext;
      else
        mCachedPlugins = tag->mNext;
      tag->mNext = nsnull;
      *result = tag;
      NS_ADDREF(*result);
      break;
    }
    prev = tag;
    tag = tag->mNext;
  }
}

#ifdef XP_WIN
nsresult
nsPluginHost::EnsurePrivateDirServiceProvider()
{
  if (!mPrivateDirServiceProvider) {
    nsresult rv;
    mPrivateDirServiceProvider = new nsPluginDirServiceProvider();
    if (!mPrivateDirServiceProvider)
      return NS_ERROR_OUT_OF_MEMORY;
    nsCOMPtr<nsIDirectoryService> dirService(do_GetService(kDirectoryServiceContractID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = dirService->RegisterProvider(mPrivateDirServiceProvider);
    if (NS_FAILED(rv))
      return rv;
  }
  return NS_OK;
}
#endif 

nsresult nsPluginHost::NewPluginURLStream(const nsString& aURL,
                                          nsIPluginInstance *aInstance,
                                          nsIPluginStreamListener* aListener,
                                          const char *aPostData,
                                          PRBool aIsFile,
                                          PRUint32 aPostDataLen,
                                          const char *aHeadersData,
                                          PRUint32 aHeadersDataLen)
{
  nsCOMPtr<nsIURI> url;
  nsAutoString absUrl;
  nsresult rv;

  if (aURL.Length() <= 0)
    return NS_OK;

  
  
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  aInstance->GetOwner(getter_AddRefs(owner));
  if (owner) {
    rv = owner->GetDocument(getter_AddRefs(doc));
    if (NS_SUCCEEDED(rv) && doc) {
      
      rv = NS_MakeAbsoluteURI(absUrl, aURL, doc->GetBaseURI());
    }
  }

  if (absUrl.IsEmpty())
    absUrl.Assign(aURL);

  rv = NS_NewURI(getter_AddRefs(url), absUrl);

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPluginTagInfo> pti = do_QueryInterface(owner);
    nsCOMPtr<nsIDOMElement> element;
    if (pti)
      pti->GetDOMElement(getter_AddRefs(element));

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                                   url,
                                   (doc ? doc->NodePrincipal() : nsnull),
                                   element,
                                   EmptyCString(), 
                                   nsnull,         
                                   &shouldLoad);
    if (NS_FAILED(rv)) return rv;
    if (NS_CP_REJECTED(shouldLoad)) {
      
      return NS_ERROR_CONTENT_BLOCKED;
    }

    nsPluginStreamListenerPeer *listenerPeer = new nsPluginStreamListenerPeer;
    if (listenerPeer == NULL)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(listenerPeer);
    rv = listenerPeer->Initialize(url, aInstance, aListener);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIInterfaceRequestor> callbacks;
      if (doc) {
        
        
        nsIScriptGlobalObject* global = doc->GetScriptGlobalObject();
        if (global) {
          nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(global);
          callbacks = do_QueryInterface(webNav);
        }
      }

      nsCOMPtr<nsIChannel> channel;

      rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull,
        nsnull, 


        callbacks);
      if (NS_FAILED(rv))
        return rv;

      if (doc) {
        
        channel->SetOwner(doc->NodePrincipal());

        
        
        nsCOMPtr<nsIScriptChannel> scriptChannel(do_QueryInterface(channel));
        if (scriptChannel) {
          scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
          
          scriptChannel->SetExecuteAsync(PR_FALSE);
        }
      }

      
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      if (httpChannel) {
        if (aPostData) {

          nsCOMPtr<nsIInputStream> postDataStream;
          rv = NS_NewPluginPostDataStream(getter_AddRefs(postDataStream), (const char*)aPostData,
                                          aPostDataLen, aIsFile);

          if (!postDataStream) {
            NS_RELEASE(aInstance);
            return NS_ERROR_UNEXPECTED;
          }

          
          
          
          nsCOMPtr<nsISeekableStream>
          postDataSeekable(do_QueryInterface(postDataStream));
          if (postDataSeekable)
            postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

          nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
          NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

          uploadChannel->SetUploadStream(postDataStream, EmptyCString(), -1);
        }

        if (aHeadersData)
          rv = AddHeadersToChannel(aHeadersData, aHeadersDataLen, httpChannel);
      }
      rv = channel->AsyncOpen(listenerPeer, nsnull);
    }
    NS_RELEASE(listenerPeer);
  }
  return rv;
}


nsresult
nsPluginHost::DoURLLoadSecurityCheck(nsIPluginInstance *aInstance,
                                         const char* aURL)
{
  if (!aURL || *aURL == '\0')
    return NS_OK;

  
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  aInstance->GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  if (!doc)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIURI> targetURL;
  NS_NewURI(getter_AddRefs(targetURL), aURL, doc->GetBaseURI());
  if (!targetURL)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> secMan(
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  return secMan->CheckLoadURIWithPrincipal(doc->NodePrincipal(), targetURL,
                                           nsIScriptSecurityManager::STANDARD);

}

nsresult
nsPluginHost::AddHeadersToChannel(const char *aHeadersData,
                                  PRUint32 aHeadersDataLen,
                                  nsIChannel *aGenericChannel)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIHttpChannel> aChannel = do_QueryInterface(aGenericChannel);
  if (!aChannel) {
    return NS_ERROR_NULL_POINTER;
  }

  
  nsCAutoString headersString;
  nsCAutoString oneHeader;
  nsCAutoString headerName;
  nsCAutoString headerValue;
  PRInt32 crlf = 0;
  PRInt32 colon = 0;

  
  headersString = aHeadersData;

  
  
  while (PR_TRUE) {
    crlf = headersString.Find("\r\n", PR_TRUE);
    if (-1 == crlf) {
      rv = NS_OK;
      return rv;
    }
    headersString.Mid(oneHeader, 0, crlf);
    headersString.Cut(0, crlf + 2);
    oneHeader.StripWhitespace();
    colon = oneHeader.Find(":");
    if (-1 == colon) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
    oneHeader.Left(headerName, colon);
    colon++;
    oneHeader.Mid(headerValue, colon, oneHeader.Length() - colon);

    

    rv = aChannel->SetRequestHeader(headerName, headerValue, PR_TRUE);
    if (NS_FAILED(rv)) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsPluginHost::StopPluginInstance(nsIPluginInstance* aInstance)
{
  if (PluginDestructionGuard::DelayDestroy(aInstance)) {
    return NS_OK;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::StopPluginInstance called instance=%p\n",aInstance));

  nsPluginInstanceTag * plugin = mPluginInstanceTagList.find(aInstance);

  if (plugin) {
    plugin->setStopped(PR_TRUE);  

    
    PRBool doCache = PR_TRUE;
    aInstance->GetValue(nsPluginInstanceVariable_DoCacheBool, (void *) &doCache);

    if (!doCache) {
      PRLibrary * library = nsnull;
      if (plugin->mPluginTag)
        library = plugin->mPluginTag->mLibrary;

      mPluginInstanceTagList.remove(plugin);
    } else {
      
      

      
      PRUint32 max_num;
      nsresult rv = NS_ERROR_FAILURE;
      if (mPrefService)
        rv = mPrefService->GetIntPref(NS_PREF_MAX_NUM_CACHED_PLUGINS, (int*)&max_num);
      if (NS_FAILED(rv))
        max_num = DEFAULT_NUMBER_OF_STOPPED_PLUGINS;

      if (mPluginInstanceTagList.getStoppedCount() >= max_num) {
        nsPluginInstanceTag * oldest = mPluginInstanceTagList.findOldestStopped();
        if (oldest != nsnull)
          mPluginInstanceTagList.remove(oldest);
      }
    }
  }
  return NS_OK;
}

nsresult nsPluginHost::NewEmbeddedPluginStreamListener(nsIURI* aURL,
                                                       nsIPluginInstanceOwner *aOwner,
                                                       nsIPluginInstance* aInstance,
                                                       nsIStreamListener** aListener)
{
  if (!aURL)
    return NS_OK;

  nsRefPtr<nsPluginStreamListenerPeer> listener =
      new nsPluginStreamListenerPeer();
  if (listener == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;

  
  
  
  
  if (aInstance != nsnull)
    rv = listener->InitializeEmbedded(aURL, aInstance);
  else if (aOwner != nsnull)
    rv = listener->InitializeEmbedded(aURL, nsnull, aOwner, this);
  else
    rv = NS_ERROR_ILLEGAL_VALUE;
  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aListener = listener);

  return rv;
}


nsresult nsPluginHost::NewEmbeddedPluginStream(nsIURI* aURL,
                                               nsIPluginInstanceOwner *aOwner,
                                               nsIPluginInstance* aInstance)
{
  nsCOMPtr<nsIStreamListener> listener;
  nsresult rv = NewEmbeddedPluginStreamListener(aURL, aOwner, aInstance,
                                                getter_AddRefs(listener));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDocument> doc;
    nsCOMPtr<nsILoadGroup> loadGroup;
    if (aOwner) {
      rv = aOwner->GetDocument(getter_AddRefs(doc));
      if (NS_SUCCEEDED(rv) && doc) {
        loadGroup = doc->GetDocumentLoadGroup();
      }
    }
    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel), aURL, nsnull, loadGroup, nsnull);
    if (NS_SUCCEEDED(rv)) {
      
      
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      if (httpChannel && doc)
        httpChannel->SetReferrer(doc->GetDocumentURI());

      rv = channel->AsyncOpen(listener, nsnull);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  return rv;
}


nsresult nsPluginHost::NewFullPagePluginStream(nsIStreamListener *&aStreamListener,
                                               nsIPluginInstance *aInstance)
{
  nsPluginStreamListenerPeer  *listener = new nsPluginStreamListenerPeer();
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;

  rv = listener->InitializeFullPage(aInstance);

  aStreamListener = listener;
  NS_ADDREF(listener);

  
  nsPluginInstanceTag * p = mPluginInstanceTagList.find(aInstance);
  if (p) {
    if (!p->mStreams && (NS_FAILED(rv = NS_NewISupportsArray(getter_AddRefs(p->mStreams)))))
      return rv;
    p->mStreams->AppendElement(aStreamListener);
  }

  return rv;
}

NS_IMETHODIMP nsPluginHost::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const PRUnichar *someData)
{
  if (!nsCRT::strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, aTopic)) {
    OnShutdown();
    Destroy();
    UnloadUnusedLibraries();
    sInst->Release();
  }
  if (!nsCRT::strcmp(NS_PRIVATE_BROWSING_SWITCH_TOPIC, aTopic)) {
    
    for (nsPluginInstanceTag* ap = mPluginInstanceTagList.mFirst; ap; ap = ap->mNext) {
      nsNPAPIPluginInstance* pi = static_cast<nsNPAPIPluginInstance*>(ap->mInstance);
      pi->PrivateModeStateChanged();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::HandleBadPlugin(PRLibrary* aLibrary, nsIPluginInstance *aInstance)
{
  
  
  

  nsresult rv = NS_OK;

  NS_ASSERTION(PR_FALSE, "Plugin performed illegal operation");

  if (mDontShowBadPluginMessage)
    return rv;

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  if (aInstance)
    aInstance->GetOwner(getter_AddRefs(owner));

  nsCOMPtr<nsIPrompt> prompt;
  GetPrompt(owner, getter_AddRefs(prompt));
  if (prompt) {
    nsCOMPtr<nsIStringBundleService> strings(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIStringBundle> bundle;
    rv = strings->CreateBundle(BRAND_PROPERTIES_URL, getter_AddRefs(bundle));
    if (NS_FAILED(rv))
      return rv;

    nsXPIDLString brandName;
    if (NS_FAILED(rv = bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(brandName))))
      return rv;

    rv = strings->CreateBundle(PLUGIN_PROPERTIES_URL, getter_AddRefs(bundle));
    if (NS_FAILED(rv))
      return rv;

    nsXPIDLString title, message, checkboxMessage;
    if (NS_FAILED(rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginTitle").get(),
                                 getter_Copies(title))))
      return rv;

    const PRUnichar *formatStrings[] = { brandName.get() };
    if (NS_FAILED(rv = bundle->FormatStringFromName(NS_LITERAL_STRING("BadPluginMessage").get(),
                                 formatStrings, 1, getter_Copies(message))))
      return rv;

    if (NS_FAILED(rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginCheckboxMessage").get(),
                                 getter_Copies(checkboxMessage))))
      return rv;

    
    nsCString pluginname;
    nsPluginInstanceTag * p = mPluginInstanceTagList.find(aInstance);
    if (p) {
      nsPluginTag * tag = p->mPluginTag;
      if (tag) {
        if (!tag->mName.IsEmpty())
          pluginname = tag->mName;
        else
          pluginname = tag->mFileName;
      }
    }

    NS_ConvertUTF8toUTF16 msg(pluginname);
    msg.AppendLiteral("\n\n");
    msg.Append(message);

    PRInt32 buttonPressed;
    PRBool checkboxState = PR_FALSE;
    rv = prompt->ConfirmEx(title, msg.get(),
                         nsIPrompt::BUTTON_TITLE_OK * nsIPrompt::BUTTON_POS_0,
                         nsnull, nsnull, nsnull,
                         checkboxMessage, &checkboxState, &buttonPressed);


    if (NS_SUCCEEDED(rv) && checkboxState)
      mDontShowBadPluginMessage = PR_TRUE;
  }

  return rv;
}

NS_IMETHODIMP
nsPluginHost::SetIsScriptableInstance(nsIPluginInstance * aPluginInstance, PRBool aScriptable)
{
  nsPluginInstanceTag * p = mPluginInstanceTagList.find(aPluginInstance);
  if (p == nsnull)
    return NS_ERROR_FAILURE;

  p->mXPConnected = aScriptable;
  if (p->mPluginTag)
    p->mPluginTag->mXPConnected = aScriptable;

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::ParsePostBufferToFixHeaders(const char *inPostData, PRUint32 inPostDataLen,
                                          char **outPostData, PRUint32 *outPostDataLen)
{
  if (!inPostData || !outPostData || !outPostDataLen)
    return NS_ERROR_NULL_POINTER;

  *outPostData = 0;
  *outPostDataLen = 0;

  const char CR = '\r';
  const char LF = '\n';
  const char CRLFCRLF[] = {CR,LF,CR,LF,'\0'}; 
  const char ContentLenHeader[] = "Content-length";

  nsAutoTArray<const char*, 8> singleLF;
  const char *pSCntlh = 0;
  const char *pSod = 0;   
  const char *pEoh = 0;   
  const char *pEod = inPostData + inPostDataLen; 
  if (*inPostData == LF) {
    
    
    
    pSod = inPostData + 1;
  } else {
    const char *s = inPostData; 
    while (s < pEod) {
      if (!pSCntlh &&
          (*s == 'C' || *s == 'c') &&
          (s + sizeof(ContentLenHeader) - 1 < pEod) &&
          (!PL_strncasecmp(s, ContentLenHeader, sizeof(ContentLenHeader) - 1)))
      {
        
        const char *p = pSCntlh = s;
        p += sizeof(ContentLenHeader) - 1;
        
        for (; p < pEod; p++) {
          if (*p == CR || *p == LF) {
            
            
            
            if (*(p-1) >= '0' && *(p-1) <= '9') {
              s = p;
            }
            break; 
          }
        }
        if (pSCntlh == s) { 
          pSCntlh = 0; 
          break; 
        }
      }

      if (*s == CR) {
        if (pSCntlh && 
            ((s + sizeof(CRLFCRLF)-1) <= pEod) &&
            !memcmp(s, CRLFCRLF, sizeof(CRLFCRLF)-1))
        {
          s += sizeof(CRLFCRLF)-1;
          pEoh = pSod = s; 
          break;
        }
      } else if (*s == LF) {
        if (*(s-1) != CR) {
          singleLF.AppendElement(s);
        }
        if (pSCntlh && (s+1 < pEod) && (*(s+1) == LF)) {
          s++;
          singleLF.AppendElement(s);
          s++;
          pEoh = pSod = s; 
          break;
        }
      }
      s++;
    }
  }

  
  if (!pSod) { 
    pSod = inPostData;
  }

  PRUint32 newBufferLen = 0;
  PRUint32 dataLen = pEod - pSod;
  PRUint32 headersLen = pEoh ? pSod - inPostData : 0;

  char *p; 
  if (headersLen) { 
    
    

    newBufferLen = dataLen + headersLen;
    
    
    int cntSingleLF = singleLF.Length();
    newBufferLen += cntSingleLF;

    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;

    
    const char *s = inPostData;
    if (cntSingleLF) {
      for (int i=0; i<cntSingleLF; i++) {
        const char *plf = singleLF.ElementAt(i); 
        int n = plf - s; 
        if (n) { 
          memcpy(p, s, n);
          p += n;
        }
        *p++ = CR;
        s = plf;
        *p++ = *s++;
      }
    }
    
    headersLen = pEoh - s;
    if (headersLen) { 
      memcpy(p, s, headersLen); 
      p += headersLen;
    }
  } else  if (dataLen) { 
    
    
    PRUint32 l = sizeof(ContentLenHeader) + sizeof(CRLFCRLF) + 32;
    newBufferLen = dataLen + l;
    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;
    headersLen = PR_snprintf(p, l,"%s: %ld%s", ContentLenHeader, dataLen, CRLFCRLF);
    if (headersLen == l) { 
      nsMemory::Free(p);
      *outPostData = 0;
      return NS_ERROR_FAILURE;
    }
    p += headersLen;
    newBufferLen = headersLen + dataLen;
  }
  
  
  
  
  if (dataLen) {
    memcpy(p, pSod, dataLen);
  }

  *outPostDataLen = newBufferLen;

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::CreateTmpFileToPost(const char *postDataURL, char **pTmpFileName)
{
  *pTmpFileName = 0;
  nsresult rv;
  PRInt64 fileSize;
  nsCAutoString filename;

  
  nsCOMPtr<nsIFile> inFile;
  rv = NS_GetFileFromURLSpec(nsDependentCString(postDataURL),
                             getter_AddRefs(inFile));
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewNativeLocalFile(nsDependentCString(postDataURL), PR_FALSE,
                               getter_AddRefs(localFile));
    if (NS_FAILED(rv)) return rv;
    inFile = localFile;
  }
  rv = inFile->GetFileSize(&fileSize);
  if (NS_FAILED(rv)) return rv;
  rv = inFile->GetNativePath(filename);
  if (NS_FAILED(rv)) return rv;

  if (!LL_IS_ZERO(fileSize)) {
    nsCOMPtr<nsIInputStream> inStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStream), inFile);
    if (NS_FAILED(rv)) return rv;

    
    

    nsCOMPtr<nsIFile> tempFile;
    rv = GetPluginTempDir(getter_AddRefs(tempFile));
    if (NS_FAILED(rv))
      return rv;

    nsCAutoString inFileName;
    inFile->GetNativeLeafName(inFileName);
    
    inFileName.Insert(NS_LITERAL_CSTRING("post-"), 0);
    rv = tempFile->AppendNative(inFileName);

    if (NS_FAILED(rv))
      return rv;

    
    rv = tempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIOutputStream> outStream;
    if (NS_SUCCEEDED(rv)) {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream),
        tempFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0600); 
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "Post data file couldn't be created!");
    if (NS_FAILED(rv))
      return rv;

    char buf[1024];
    PRUint32 br, bw;
    PRBool firstRead = PR_TRUE;
    while (1) {
      
      rv = inStream->Read(buf, 1024, &br);
      if (NS_FAILED(rv) || (PRInt32)br <= 0)
        break;
      if (firstRead) {
        
        
        
        

        char *parsedBuf;
        
        
        ParsePostBufferToFixHeaders((const char *)buf, br, &parsedBuf, &bw);
        rv = outStream->Write(parsedBuf, bw, &br);
        nsMemory::Free(parsedBuf);
        if (NS_FAILED(rv) || (bw != br))
          break;

        firstRead = PR_FALSE;
        continue;
      }
      bw = br;
      rv = outStream->Write(buf, bw, &br);
      if (NS_FAILED(rv) || (bw != br))
        break;
    }

    inStream->Close();
    outStream->Close();
    if (NS_SUCCEEDED(rv)) {
      nsCAutoString path;
      if (NS_SUCCEEDED(tempFile->GetNativePath(path)))
        *pTmpFileName = ToNewCString(path);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsPluginHost::NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  return PLUG_NewPluginNativeWindow(aPluginNativeWindow);
}

NS_IMETHODIMP
nsPluginHost::DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  return PLUG_DeletePluginNativeWindow(aPluginNativeWindow);
}

NS_IMETHODIMP
nsPluginHost::InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner)
{
  
  
  nsPluginTag *plugin = FindPluginForType("application/x-java-vm", PR_FALSE);

  if (!plugin || !plugin->mIsNPRuntimeEnabledJavaPlugin) {
    
    

    return NS_OK;
  }

  nsresult rv = SetUpPluginInstance("application/x-java-vm", nsnull, aOwner);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPluginInstance> instance;
  aOwner->GetInstance(*getter_AddRefs(instance));
  if (!instance)
    return NS_OK;

  instance->DefineJavaProperties();

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPluginName(nsIPluginInstance *aPluginInstance,
                            const char** aPluginName)
{
  *aPluginName = GetPluginName(aPluginInstance);
  return NS_OK;
}

nsresult nsPluginHost::AddUnusedLibrary(PRLibrary * aLibrary)
{
  if (!mUnusedLibraries.Contains(aLibrary)) 
    mUnusedLibraries.AppendElement(aLibrary);

  return NS_OK;
}

nsresult nsPluginStreamListenerPeer::ServeStreamAsFile(nsIRequest *request,
                                                       nsISupports* aContext)
{
  if (!mInstance)
    return NS_ERROR_FAILURE;

  
  mInstance->Stop();
  mInstance->Start();
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  mInstance->GetOwner(getter_AddRefs(owner));
  if (owner) {
    nsPluginWindow    *window = nsnull;
    owner->GetWindow(window);
#if defined (MOZ_WIDGET_GTK2)
    
    
    nsCOMPtr<nsIWidget> widget;
    ((nsPluginNativeWindow*)window)->GetPluginWidget(getter_AddRefs(widget));
    if (widget) {
      window->window = (nsPluginPort*) widget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
    }
#endif
    if (window->window) {
      nsCOMPtr<nsIPluginInstance> inst = mInstance;
      ((nsPluginNativeWindow*)window)->CallSetWindow(inst);
    }
  }

  mPluginStreamInfo->SetSeekable(0);
  mPStreamListener->OnStartBinding(mPluginStreamInfo);
  mPluginStreamInfo->SetStreamOffset(0);

  
  mStreamType = nsPluginStreamType_AsFile;

  
  nsCOMPtr<nsICachingChannel> cacheChannel = do_QueryInterface(request);
  if (!(cacheChannel && (NS_SUCCEEDED(cacheChannel->SetCacheAsFile(PR_TRUE))))) {
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
      if (channel) {
        SetupPluginCacheFile(channel);
      }
  }

  
  mPendingRequests = 0;

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsPluginByteRangeStreamListener, nsIStreamListener)
nsPluginByteRangeStreamListener::nsPluginByteRangeStreamListener(nsIWeakReference* aWeakPtr)
{
  mWeakPtrPluginStreamListenerPeer = aWeakPtr;
  mRemoveMagicNumber = PR_FALSE;
}

nsPluginByteRangeStreamListener::~nsPluginByteRangeStreamListener()
{
  mStreamConverter = 0;
  mWeakPtrPluginStreamListenerPeer = 0;
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
     return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamConverterService> serv = do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = serv->AsyncConvertData(MULTIPART_BYTERANGES,
                                "*/*",
                                finalStreamListener,
                                nsnull,
                                getter_AddRefs(mStreamConverter));
    if (NS_SUCCEEDED(rv)) {
      rv = mStreamConverter->OnStartRequest(request, ctxt);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
  }
  mStreamConverter = 0;

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  if (!httpChannel) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 responseCode = 0;
  rv = httpChannel->GetResponseStatus(&responseCode);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }
  
  
  nsPluginStreamListenerPeer *pslp =
    reinterpret_cast<nsPluginStreamListenerPeer*>(finalStreamListener.get());

  if (responseCode != 200) {
    PRBool bWantsAllNetworkStreams = PR_FALSE;
    pslp->GetPluginInstance()->
      GetValue(nsPluginInstanceVariable_WantsAllNetworkStreams,
               (void *)&bWantsAllNetworkStreams);
    if (!bWantsAllNetworkStreams){
      return NS_ERROR_FAILURE;
    }
  }

  
  
  mStreamConverter = finalStreamListener;
  mRemoveMagicNumber = PR_TRUE;

  rv = pslp->ServeStreamAsFile(request, ctxt);
  return rv;
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                              nsresult status)
{
  if (!mStreamConverter)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  if (mRemoveMagicNumber) {
    
    nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(ctxt);
    if (container) {
      PRUint32 magicNumber = 0;
      container->GetData(&magicNumber);
      if (magicNumber == MAGIC_REQUEST_CONTEXT) {
        
        
        container->SetData(0);
      }
    } else {
      NS_WARNING("Bad state of nsPluginByteRangeStreamListener");
    }
  }

  return mStreamConverter->OnStopRequest(request, ctxt, status);
}

NS_IMETHODIMP
nsPluginByteRangeStreamListener::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                                nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  if (!mStreamConverter)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamListener> finalStreamListener = do_QueryReferent(mWeakPtrPluginStreamListenerPeer);
  if (!finalStreamListener)
    return NS_ERROR_FAILURE;

  return mStreamConverter->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
}

PRBool
nsPluginStreamInfo::UseExistingPluginCacheFile(nsPluginStreamInfo* psi)
{

  NS_ENSURE_ARG_POINTER(psi);

 if ( psi->mLength == mLength &&
      psi->mModified == mModified &&
      mStreamComplete &&
      !PL_strcmp(psi->mURL, mURL))
  {
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsPluginStreamInfo::SetStreamComplete(const PRBool complete)
{
  mStreamComplete = complete;

  if (complete) {
    
    SetRequest(nsnull);
  }
}



class nsPluginDestroyRunnable : public nsRunnable,
                                public PRCList
{
public:
  nsPluginDestroyRunnable(nsIPluginInstance *aInstance)
    : mInstance(aInstance)
  {
    PR_INIT_CLIST(this);
    PR_APPEND_LINK(this, &sRunnableListHead);
  }

  virtual ~nsPluginDestroyRunnable()
  {
    PR_REMOVE_LINK(this);
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIPluginInstance> instance;

    
    
    
    instance.swap(mInstance);

    if (PluginDestructionGuard::DelayDestroy(instance)) {
      
      
      return NS_OK;
    }

    nsPluginDestroyRunnable *r =
      static_cast<nsPluginDestroyRunnable*>(PR_NEXT_LINK(&sRunnableListHead));

    while (r != &sRunnableListHead) {
      if (r != this && r->mInstance == instance) {
        
        
        return NS_OK;
      }
      r = static_cast<nsPluginDestroyRunnable*>(PR_NEXT_LINK(r));
    }

    PLUGIN_LOG(PLUGIN_LOG_NORMAL,
               ("Doing delayed destroy of instance %p\n", instance.get()));

    instance->Stop();

    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();

    if (host)
      host->StopPluginInstance(instance);

    PLUGIN_LOG(PLUGIN_LOG_NORMAL,
               ("Done with delayed destroy of instance %p\n", instance.get()));

    return NS_OK;
  }

protected:
  nsCOMPtr<nsIPluginInstance> mInstance;

  static PRCList sRunnableListHead;
};

PRCList nsPluginDestroyRunnable::sRunnableListHead =
  PR_INIT_STATIC_CLIST(&nsPluginDestroyRunnable::sRunnableListHead);

PRCList PluginDestructionGuard::sListHead =
  PR_INIT_STATIC_CLIST(&PluginDestructionGuard::sListHead);

PluginDestructionGuard::~PluginDestructionGuard()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");

  PR_REMOVE_LINK(this);

  if (mDelayedDestroy) {
    
    
    
    nsRefPtr<nsPluginDestroyRunnable> evt =
      new nsPluginDestroyRunnable(mInstance);

    NS_DispatchToMainThread(evt);
  }
}


PRBool
PluginDestructionGuard::DelayDestroy(nsIPluginInstance *aInstance)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");
  NS_ASSERTION(aInstance, "Uh, I need an instance!");

  
  

  PluginDestructionGuard *g =
    static_cast<PluginDestructionGuard*>(PR_LIST_HEAD(&sListHead));

  while (g != &sListHead) {
    if (g->mInstance == aInstance) {
      g->mDelayedDestroy = PR_TRUE;

      return PR_TRUE;
    }
    g = static_cast<PluginDestructionGuard*>(PR_NEXT_LINK(g));    
  }

  return PR_FALSE;
}
