







































#include "nsPluginTags.h"

#include "prlink.h"
#include "plstr.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIDocument.h"
#include "nsServiceManagerUtils.h"
#include "nsPluginsDir.h"
#include "nsPluginHost.h"
#include "nsIUnicodeDecoder.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsPluginLogging.h"
#include "nsICategoryManager.h"
#include "nsNPAPIPlugin.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using mozilla::TimeStamp;

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
mMimeTypes(aPluginTag->mMimeTypes),
mMimeDescriptions(aPluginTag->mMimeDescriptions),
mExtensions(aPluginTag->mExtensions),
mLibrary(nsnull),
mIsJavaPlugin(aPluginTag->mIsJavaPlugin),
mIsFlashPlugin(aPluginTag->mIsFlashPlugin),
mFileName(aPluginTag->mFileName),
mFullPath(aPluginTag->mFullPath),
mVersion(aPluginTag->mVersion),
mLastModifiedTime(0),
mFlags(NS_PLUGIN_FLAG_ENABLED)
{
}

nsPluginTag::nsPluginTag(nsPluginInfo* aPluginInfo)
: mPluginHost(nsnull),
mName(aPluginInfo->fName),
mDescription(aPluginInfo->fDescription),
mLibrary(nsnull),
mIsJavaPlugin(false),
mIsFlashPlugin(false),
mFileName(aPluginInfo->fFileName),
mFullPath(aPluginInfo->fFullPath),
mVersion(aPluginInfo->fVersion),
mLastModifiedTime(0),
mFlags(NS_PLUGIN_FLAG_ENABLED)
{
  InitMime(aPluginInfo->fMimeTypeArray,
           aPluginInfo->fMimeDescriptionArray,
           aPluginInfo->fExtensionArray,
           aPluginInfo->fVariantCount);
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
                         bool aArgsAreUTF8)
: mPluginHost(nsnull),
mName(aName),
mDescription(aDescription),
mLibrary(nsnull),
mIsJavaPlugin(false),
mIsFlashPlugin(false),
mFileName(aFileName),
mFullPath(aFullPath),
mVersion(aVersion),
mLastModifiedTime(aLastModifiedTime),
mFlags(0) 
{
  InitMime(aMimeTypes, aMimeDescriptions, aExtensions, static_cast<PRUint32>(aVariants));
  if (!aArgsAreUTF8)
    EnsureMembersAreUTF8();
}

nsPluginTag::~nsPluginTag()
{
  NS_ASSERTION(!mNext, "Risk of exhausting the stack space, bug 486349");
}

NS_IMPL_ISUPPORTS1(nsPluginTag, nsIPluginTag)

void nsPluginTag::InitMime(const char* const* aMimeTypes,
                           const char* const* aMimeDescriptions,
                           const char* const* aExtensions,
                           PRUint32 aVariantCount)
{
  if (!aMimeTypes) {
    return;
  }

  for (PRUint32 i = 0; i < aVariantCount; i++) {
    if (!aMimeTypes[i]) {
      continue;
    }

    
    
    
    
    if (mIsJavaPlugin) {
      if (strcmp(aMimeTypes[i], "application/x-java-vm-npruntime") == 0) {
        
        
        
        continue;
      }
    }

    
    if (nsPluginHost::IsJavaMIMEType(aMimeTypes[i])) {
      mIsJavaPlugin = true;
    } else if (strcmp(aMimeTypes[i], "application/x-shockwave-flash") == 0) {
      mIsFlashPlugin = true;
    }

    
    mMimeTypes.AppendElement(nsCString(aMimeTypes[i]));

    
    if (aMimeDescriptions && aMimeDescriptions[i]) {
      
      
      
      
      char cur = '\0';
      char pre = '\0';
      char * p = PL_strrchr(aMimeDescriptions[i], '(');
      if (p && (p != aMimeDescriptions[i])) {
        if ((p - 1) && *(p - 1) == ' ') {
          pre = *(p - 1);
          *(p - 1) = '\0';
        } else {
          cur = *p;
          *p = '\0';
        }
      }
      mMimeDescriptions.AppendElement(nsCString(aMimeDescriptions[i]));
      
      if (cur != '\0') {
        *p = cur;
      }
      if (pre != '\0') {
        *(p - 1) = pre;
      }
    } else {
      mMimeDescriptions.AppendElement(nsCString());
    }

    
    if (aExtensions && aExtensions[i]) {
      mExtensions.AppendElement(nsCString(aExtensions[i]));
    } else {
      mExtensions.AppendElement(nsCString());
    }
  }
}

#if !defined(XP_WIN) && !defined(XP_MACOSX)
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
#endif

nsresult nsPluginTag::EnsureMembersAreUTF8()
{
#if defined(XP_WIN) || defined(XP_MACOSX)
  return NS_OK;
#else
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
    for (PRUint32 i = 0; i < mMimeDescriptions.Length(); ++i) {
      ConvertToUTF8(decoder, mMimeDescriptions[i]);
    }
  }
  return NS_OK;
#endif
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
nsPluginTag::GetFullpath(nsACString& aFullPath)
{
  aFullPath = mFullPath;
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
nsPluginTag::GetDisabled(bool* aDisabled)
{
  *aDisabled = !HasFlag(NS_PLUGIN_FLAG_ENABLED);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::SetDisabled(bool aDisabled)
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
nsPluginTag::GetBlocklisted(bool* aBlocklisted)
{
  *aBlocklisted = HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginTag::SetBlocklisted(bool aBlocklisted)
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

void
nsPluginTag::RegisterWithCategoryManager(bool aOverrideInternalTypes,
                                         nsPluginTag::nsRegisterType aType)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
             ("nsPluginTag::RegisterWithCategoryManager plugin=%s, removing = %s\n",
              mFileName.get(), aType == ePluginUnregister ? "yes" : "no"));
  
  nsCOMPtr<nsICategoryManager> catMan = do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMan)
    return;
  
  const char *contractId = "@mozilla.org/content/plugin/document-loader-factory;1";
  
  
  
  
  
  
  
  
  nsCAutoString overrideTypesFormatted;
  if (aType != ePluginUnregister) {
    overrideTypesFormatted.Assign(',');
    nsAdoptingCString overrideTypes =
      Preferences::GetCString("plugin.disable_full_page_plugin_for_types");
    overrideTypesFormatted += overrideTypes;
    overrideTypesFormatted.Append(',');
  }
  
  nsACString::const_iterator start, end;
  for (PRUint32 i = 0; i < mMimeTypes.Length(); i++) {
    if (aType == ePluginUnregister) {
      nsXPIDLCString value;
      if (NS_SUCCEEDED(catMan->GetCategoryEntry("Gecko-Content-Viewers",
                                                mMimeTypes[i].get(),
                                                getter_Copies(value)))) {
        
        if (strcmp(value, contractId) == 0) {
          catMan->DeleteCategoryEntry("Gecko-Content-Viewers",
                                      mMimeTypes[i].get(),
                                      true);
        }
      }
    } else {
      overrideTypesFormatted.BeginReading(start);
      overrideTypesFormatted.EndReading(end);
      
      nsCAutoString commaSeparated; 
      commaSeparated.Assign(',');
      commaSeparated += mMimeTypes[i];
      commaSeparated.Append(',');
      if (!FindInReadable(commaSeparated, start, end)) {
        catMan->AddCategoryEntry("Gecko-Content-Viewers",
                                 mMimeTypes[i].get(),
                                 contractId,
                                 false, 
                                 aOverrideInternalTypes, 
                                 nsnull);
      }
    }
    
    PLUGIN_LOG(PLUGIN_LOG_NOISY,
               ("nsPluginTag::RegisterWithCategoryManager mime=%s, plugin=%s\n",
                mMimeTypes[i].get(), mFileName.get()));
  }
}

void nsPluginTag::Mark(PRUint32 mask)
{
  bool wasEnabled = IsEnabled();
  mFlags |= mask;
  
  if (mPluginHost && wasEnabled != IsEnabled()) {
    if (wasEnabled)
      RegisterWithCategoryManager(false, nsPluginTag::ePluginUnregister);
    else
      RegisterWithCategoryManager(false, nsPluginTag::ePluginRegister);
  }
}

void nsPluginTag::UnMark(PRUint32 mask)
{
  bool wasEnabled = IsEnabled();
  mFlags &= ~mask;
  
  if (mPluginHost && wasEnabled != IsEnabled()) {
    if (wasEnabled)
      RegisterWithCategoryManager(false, nsPluginTag::ePluginUnregister);
    else
      RegisterWithCategoryManager(false, nsPluginTag::ePluginRegister);
  }
}

bool nsPluginTag::HasFlag(PRUint32 flag)
{
  return (mFlags & flag) != 0;
}

PRUint32 nsPluginTag::Flags()
{
  return mFlags;
}

bool nsPluginTag::IsEnabled()
{
  return HasFlag(NS_PLUGIN_FLAG_ENABLED) && !HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED);
}

bool nsPluginTag::Equals(nsPluginTag *aPluginTag)
{
  NS_ENSURE_TRUE(aPluginTag, false);
  
  if ((!mName.Equals(aPluginTag->mName)) ||
      (!mDescription.Equals(aPluginTag->mDescription)) ||
      (mMimeTypes.Length() != aPluginTag->mMimeTypes.Length())) {
    return false;
  }

  for (PRUint32 i = 0; i < mMimeTypes.Length(); i++) {
    if (!mMimeTypes[i].Equals(aPluginTag->mMimeTypes[i])) {
      return false;
    }
  }

  return true;
}

void nsPluginTag::TryUnloadPlugin(bool inShutdown)
{
  
  
  if (mLibrary && !inShutdown) {
    return;
  }

  if (mPlugin) {
    mPlugin->Shutdown();
    mPlugin = nsnull;
  }
}
