





































#include "nsInternetConfigService.h"
#include "nsCOMPtr.h"
#include "nsIMIMEInfo.h"
#include "nsMIMEInfoMac.h"
#include "nsAutoPtr.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"
#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsILocalFileMac.h"
#include "nsMimeTypes.h"
#include <TextUtils.h>
#include <CodeFragments.h>
#include <Processes.h>
#include <Gestalt.h>
#include <CFURL.h>
#include <Finder.h>
#include <LaunchServices.h>



static void ConvertCharStringToStr255(const char* inString, Str255& outString)
{
  if (inString == NULL)
    return;
  
  PRInt32 len = strlen(inString);
  NS_ASSERTION(len <= 255 , " String is too big");
  if (len> 255)
  {
    len = 255;
  }
  memcpy(&outString[1], inString, len);
  outString[0] = len;
}



nsInternetConfigService::nsInternetConfigService()
{
}

nsInternetConfigService::~nsInternetConfigService()
{
}





NS_IMPL_ISUPPORTS1(nsInternetConfigService, nsIInternetConfigService)




NS_IMETHODIMP nsInternetConfigService::LaunchURL(const char *url)
{
  nsresult rv = NS_ERROR_FAILURE; 

  CFURLRef myURLRef = ::CFURLCreateWithBytes(
                                             kCFAllocatorDefault,
                                             (const UInt8*)url,
                                             strlen(url),
                                             kCFStringEncodingUTF8, NULL);
  if (myURLRef)
  {
    rv = ::LSOpenCFURLRef(myURLRef, NULL);
    ::CFRelease(myURLRef);
  }

  return rv;
}



NS_IMETHODIMP nsInternetConfigService::HasMappingForMIMEType(const char *mimetype, PRBool *_retval)
{
  ICMapEntry entry;
  nsresult rv = GetMappingForMIMEType(mimetype, nsnull, &entry);
  if (rv == noErr)
    *_retval = PR_TRUE;
  else
    *_retval = PR_FALSE;
  return rv;
}




NS_IMETHODIMP nsInternetConfigService::HasProtocolHandler(const char *protocol, PRBool *_retval)
{
  *_retval = PR_FALSE;            
  nsresult rv = NS_OK;

  
  
  
  
  nsCAutoString scheme(protocol);
  scheme += ":";
  CFURLRef myURLRef = ::CFURLCreateWithBytes(
                                              kCFAllocatorDefault,
                                              (const UInt8 *)scheme.get(),
                                              scheme.Length(),
                                              kCFStringEncodingUTF8, NULL);
  if (myURLRef)
  {
    FSRef appFSRef;
  
    if (::LSGetApplicationForURL(myURLRef, kLSRolesAll, &appFSRef, NULL) == noErr)
    { 
      ProcessSerialNumber psn;
      if (::GetCurrentProcess(&psn) == noErr)
      {
        FSRef runningAppFSRef;
        if (::GetProcessBundleLocation(&psn, &runningAppFSRef) == noErr)
        {
          if (::FSCompareFSRefs(&appFSRef, &runningAppFSRef) == noErr)
          { 
            rv = NS_ERROR_NOT_AVAILABLE;
          }
          else
          {
            *_retval = PR_TRUE;
          }
        }
      }
    }
    ::CFRelease(myURLRef);
  }

  return rv;
}



nsresult nsInternetConfigService::GetMappingForMIMEType(const char *mimetype, const char *fileextension, ICMapEntry *entry)
{
  ICInstance  inst = nsInternetConfig::GetInstance();
  OSStatus    err = noErr;
  ICAttr      attr;
  Handle      prefH;
  PRBool      domimecheck = PR_TRUE;
  PRBool      gotmatch = PR_FALSE;
  ICMapEntry  ent;
  
  
  
  if (((nsCRT::strcasecmp(mimetype, UNKNOWN_CONTENT_TYPE) == 0) ||
       (nsCRT::strcasecmp(mimetype, APPLICATION_OCTET_STREAM) == 0)) &&
       fileextension)
    domimecheck = PR_FALSE;
  
  entry->totalLength = 0;
  if (inst)
  {
    err = ::ICBegin(inst, icReadOnlyPerm);
    if (err == noErr)
    {
      prefH = ::NewHandle(2048); 
      if (prefH)
      {
        err = ::ICFindPrefHandle(inst, kICMapping, &attr, prefH);
        if (err == noErr)
        {
          long count;
          err = ::ICCountMapEntries(inst, prefH, &count);
          if (err == noErr)
          {
            long pos;
            for (long i = 1; i <= count; ++i)
            {
              err = ::ICGetIndMapEntry(inst, prefH, i, &pos, &ent);
              if (err == noErr)
              {
                
                if (domimecheck)
                {
                  nsCAutoString temp((char *)&ent.MIMEType[1], (int)ent.MIMEType[0]);
                  if (!temp.EqualsIgnoreCase(mimetype))
                  {
                    
                    
                    continue;
                  }
                }
                if (fileextension)
                {
                  
                  if (ent.extension[0]) 
                  {
                    nsCAutoString temp((char *)&ent.extension[1], (int)ent.extension[0]);
                    if (temp.EqualsIgnoreCase(fileextension))
                    {
                      
                      gotmatch = PR_TRUE;
                      
                      *entry = ent;
                      break;
                    }
                  }
                }
                else if(domimecheck)
                {
                  
                  
                  
                  gotmatch = PR_TRUE;
                  
                  *entry = ent;
                  break;
                }
              }
            }
          }
        }
        ::DisposeHandle(prefH);
      }
      else
      {
        err = memFullErr;
      }
      err = ::ICEnd(inst);
      if (err == noErr && gotmatch == PR_FALSE)
      {
        err = fnfErr; 
      }
    }
  }
  
  if (err != noErr)
    return NS_ERROR_FAILURE;
  else
    return NS_OK;
}

nsresult nsInternetConfigService::FillMIMEInfoForICEntry(ICMapEntry& entry, nsIMIMEInfo ** mimeinfo)
{
  
  nsresult  rv = NS_OK;
  nsRefPtr<nsMIMEInfoMac> info (new nsMIMEInfoMac());
  if (info)
  {
    nsCAutoString mimetype ((char *)&entry.MIMEType[1], entry.MIMEType[0]);
    
    if (entry.MIMEType[0])
      info->SetMIMEType(mimetype);
    else
    { 
      
      
      
      if (entry.fileType == 'TEXT')
        info->SetMIMEType(NS_LITERAL_CSTRING(TEXT_PLAIN));
      else
        info->SetMIMEType(NS_LITERAL_CSTRING(APPLICATION_OCTET_STREAM));
    }
    
    
    
    nsCAutoString temp((char *)&entry.extension[2], entry.extension[0] > 0 ? (int)entry.extension[0]-1 : 0);
    info->AppendExtension(temp);
    info->SetMacType(entry.fileType);
    info->SetMacCreator(entry.fileCreator);
    temp.Assign((char *) &entry.entryName[1], entry.entryName[0]);
    info->SetDescription(NS_ConvertASCIItoUTF16(temp));
    
    temp.Assign((char *) &entry.postAppName[1], entry.postAppName[0]);
    info->SetDefaultDescription(NS_ConvertASCIItoUTF16(temp));
    
    if (entry.flags & kICMapPostMask)
    {
      
      info->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
      nsCOMPtr<nsILocalFileMac> file (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));
      if (file)
      {
        rv = file->InitToAppWithCreatorCode(entry.postCreator);
        if (rv == NS_OK)
        {
          nsCOMPtr<nsIFile> nsfile = do_QueryInterface(file, &rv);
          if (rv == NS_OK)
            info->SetDefaultApplication(nsfile);
        }
      }
    }
    else
    {
      
      info->SetPreferredAction(nsIMIMEInfo::saveToDisk);
    }
    
    *mimeinfo = info;
    NS_IF_ADDREF(*mimeinfo);
  }
  else 
    rv = NS_ERROR_FAILURE;
   
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::FillInMIMEInfo(const char *mimetype, const char * aFileExtension, nsIMIMEInfo **mimeinfo)
{
  nsresult    rv;
  ICMapEntry  entry;
  
  NS_ENSURE_ARG_POINTER(mimeinfo);
  *mimeinfo = nsnull;

  if (aFileExtension)
  {
    nsCAutoString fileExtension;
    fileExtension.Assign(".");  
    fileExtension.Append(aFileExtension);
    rv = GetMappingForMIMEType(mimetype, fileExtension.get(), &entry);
  }
  else
  {
    rv = GetMappingForMIMEType(mimetype, nsnull, &entry);
  }
  
  if (rv == NS_OK)
    rv = FillMIMEInfoForICEntry(entry, mimeinfo);
  else
    rv = NS_ERROR_FAILURE;

  return rv;
}

NS_IMETHODIMP nsInternetConfigService::GetMIMEInfoFromExtension(const char *aFileExt, nsIMIMEInfo **_retval)
{
  nsresult    rv = NS_ERROR_FAILURE;
  ICInstance  instance = nsInternetConfig::GetInstance();
  if (instance)
  {
    nsCAutoString filename("foobar.");
    filename += aFileExt;
    Str255  pFileName;
    ConvertCharStringToStr255(filename.get(), pFileName);
    ICMapEntry  entry;
    OSStatus  err = ::ICMapFilename(instance, pFileName, &entry);
    if (err == noErr)
    {
      rv = FillMIMEInfoForICEntry(entry, _retval);
    }
  }   
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::GetMIMEInfoFromTypeCreator(PRUint32 aType, PRUint32 aCreator, const char *aFileExt, nsIMIMEInfo **_retval)
{
  nsresult    rv = NS_ERROR_FAILURE;
  ICInstance  instance = nsInternetConfig::GetInstance();
  if (instance)
  {
    nsCAutoString filename("foobar.");
    filename += aFileExt;
    Str255  pFileName;
    ConvertCharStringToStr255(filename.get(), pFileName);
    ICMapEntry  entry;
    OSStatus  err = ::ICMapTypeCreator(instance, aType, aCreator, pFileName, &entry);
    if (err == noErr)
      rv = FillMIMEInfoForICEntry(entry,_retval);
  }
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::GetFileMappingFlags(FSSpec* fsspec, PRBool lookupByExtensionFirst, PRInt32 *_retval)
{
  nsresult  rv = NS_ERROR_FAILURE;
  OSStatus  err = noErr;

  NS_ENSURE_ARG(_retval);
  *_retval = -1;
  
  ICInstance instance = nsInternetConfig::GetInstance();
  if (instance)
  {
    ICMapEntry  entry;
    
    if (lookupByExtensionFirst)
      err = ::ICMapFilename(instance, fsspec->name, &entry);
  
    if (!lookupByExtensionFirst || err != noErr)
    {
      FInfo info;
      err = FSpGetFInfo(fsspec, &info);
      if (err == noErr)
        err = ::ICMapTypeCreator(instance, info.fdType, info.fdCreator, fsspec->name, &entry);
    }

    if (err == noErr)
      *_retval = entry.flags;
     
     rv = NS_OK;
  }
  return rv;
}



NS_IMETHODIMP nsInternetConfigService::GetDownloadFolder(FSSpec *fsspec)
{
  ICInstance  inst = nsInternetConfig::GetInstance();
  OSStatus    err;
  Handle      prefH;
  nsresult    rv = NS_ERROR_FAILURE;
  
  NS_ENSURE_ARG_POINTER(fsspec);
  
  if (inst)
  {
    err = ::ICBegin(inst, icReadOnlyPerm);
    if (err == noErr)
    {
      prefH = ::NewHandle(256); 
      if (prefH)
      {
        ICAttr  attr;
        err = ::ICFindPrefHandle(inst, kICDownloadFolder, &attr, prefH);
        if (err == noErr)
        {
          err = ::Munger(prefH, 0, NULL, kICFileSpecHeaderSize, (Ptr)-1, 0);
          if (err == noErr)
          {
            Boolean wasChanged;
            err = ::ResolveAlias(NULL, (AliasHandle)prefH, fsspec, &wasChanged);
            if (err == noErr)
            {
              rv = NS_OK;
            }
            else
            { 
              err = ::ICFindPrefHandle(inst, kICDownloadFolder, &attr, prefH);
              if (err == noErr)
              { 
                FSSpec  tempSpec = (*(ICFileSpecHandle)prefH)->fss;
                err = ::FSMakeFSSpec(tempSpec.vRefNum, tempSpec.parID, tempSpec.name, fsspec);
                if (err == noErr)
                  rv = NS_OK;
              }
            }
          }
        }
        
        DisposeHandle(prefH);
      }
      err = ::ICEnd(inst);
    }
  }
  return rv;
}

nsresult nsInternetConfigService::GetICKeyPascalString(PRUint32 inIndex, const unsigned char*& outICKey)
{
  nsresult  rv = NS_OK;

  switch (inIndex)
  {
    case eICColor_WebBackgroundColour: outICKey = kICWebBackgroundColour; break;
    case eICColor_WebReadColor:        outICKey = kICWebReadColor;        break;
    case eICColor_WebTextColor:        outICKey = kICWebTextColor;        break;
    case eICColor_WebUnreadColor:      outICKey = kICWebUnreadColor;      break;

    case eICBoolean_WebUnderlineLinks: outICKey = kICWebUnderlineLinks;  break;
    case eICBoolean_UseFTPProxy:       outICKey = kICUseFTPProxy;        break;
    case eICBoolean_UsePassiveFTP:     outICKey = kICUsePassiveFTP;      break;
    case eICBoolean_UseHTTPProxy:      outICKey = kICUseHTTPProxy;       break;
    case eICBoolean_NewMailDialog:     outICKey = kICNewMailDialog;      break;
    case eICBoolean_NewMailFlashIcon:  outICKey = kICNewMailFlashIcon;   break;
    case eICBoolean_NewMailPlaySound:  outICKey = kICNewMailPlaySound;   break;
    case eICBoolean_UseGopherProxy:    outICKey = kICUseGopherProxy;     break;
    case eICBoolean_UseSocks:          outICKey = kICUseSocks;           break;

    case eICString_WWWHomePage:        outICKey = kICWWWHomePage;        break;
    case eICString_WebSearchPagePrefs: outICKey = kICWebSearchPagePrefs; break;
    case eICString_MacSearchHost:      outICKey = kICMacSearchHost;      break;
    case eICString_FTPHost:            outICKey = kICFTPHost;            break;
    case eICString_FTPProxyUser:       outICKey = kICFTPProxyUser;       break;
    case eICString_FTPProxyAccount:    outICKey = kICFTPProxyAccount;    break;
    case eICString_FTPProxyHost:       outICKey = kICFTPProxyHost;       break;
    case eICString_FTPProxyPassword:   outICKey = kICFTPProxyPassword;   break;
    case eICString_HTTPProxyHost:      outICKey = kICHTTPProxyHost;      break;
    case eICString_LDAPSearchbase:     outICKey = kICLDAPSearchbase;     break;
    case eICString_LDAPServer:         outICKey = kICLDAPServer;         break;
    case eICString_SMTPHost:           outICKey = kICSMTPHost;           break;
    case eICString_Email:              outICKey = kICEmail;              break;
    case eICString_MailAccount:        outICKey = kICMailAccount;        break;
    case eICString_MailPassword:       outICKey = kICMailPassword;       break;
    case eICString_NewMailSoundName:   outICKey = kICNewMailSoundName;   break;
    case eICString_NNTPHost:           outICKey = kICNNTPHost;           break;
    case eICString_NewsAuthUsername:   outICKey = kICNewsAuthUsername;   break;
    case eICString_NewsAuthPassword:   outICKey = kICNewsAuthPassword;   break;
    case eICString_InfoMacPreferred:   outICKey = kICInfoMacPreferred;   break;
    case eICString_Organization:       outICKey = kICOrganization;       break;
    case eICString_QuotingString:      outICKey = kICQuotingString;      break;
    case eICString_RealName:           outICKey = kICRealName;           break;
    case eICString_FingerHost:         outICKey = kICFingerHost;         break;
    case eICString_GopherHost:         outICKey = kICGopherHost;         break;
    case eICString_GopherProxy:        outICKey = kICGopherProxy;        break;
    case eICString_SocksHost:          outICKey = kICSocksHost;          break;
    case eICString_TelnetHost:         outICKey = kICTelnetHost;         break;
    case eICString_IRCHost:            outICKey = kICIRCHost;            break;
    case eICString_UMichPreferred:     outICKey = kICUMichPreferred;     break;
    case eICString_WAISGateway:        outICKey = kICWAISGateway;        break;
    case eICString_WhoisHost:          outICKey = kICWhoisHost;          break;
    case eICString_PhHost:             outICKey = kICPhHost;             break;
    case eICString_NTPHost:            outICKey = kICNTPHost;            break;
    case eICString_ArchiePreferred:    outICKey = kICArchiePreferred;    break;
    
    case eICText_MailHeaders:          outICKey = kICMailHeaders;        break;
    case eICText_Signature:            outICKey = kICSignature;          break;
    case eICText_NewsHeaders:          outICKey = kICNewsHeaders;        break;
    case eICText_SnailMailAddress:     outICKey = kICSnailMailAddress;   break;
    case eICText_Plan:                 outICKey = kICPlan;               break;

    default:
      rv = NS_ERROR_INVALID_ARG;
  }
  return rv;
}


nsresult nsInternetConfigService::GetICPreference(PRUint32 inKey, 
                                                  void *outData, long *ioSize)
{
  const unsigned char *icKey;
  nsresult  rv = GetICKeyPascalString(inKey, icKey);
  if (rv == NS_OK)
  {
    ICInstance  instance = nsInternetConfig::GetInstance();
    if (instance)
    {
      OSStatus  err;
      ICAttr    junk;
      err = ::ICGetPref(instance, icKey, &junk, outData, ioSize);
      if (err != noErr)
        rv = NS_ERROR_UNEXPECTED;
    }
    else
      rv = NS_ERROR_FAILURE;
  }
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::GetString(PRUint32 inKey, nsACString& value)
{
  long      size = 256;
  char      buffer[256];
  nsresult  rv = GetICPreference(inKey, (void *)&buffer, &size);
  if (rv == NS_OK)
  {
    if (size == 0)
    {
      value = "";
      rv = NS_ERROR_UNEXPECTED;
    }
    else
    { 
      value.Assign(&buffer[1], (unsigned char)buffer[0]);
    }
  }
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::GetColor(PRUint32 inKey, PRUint32 *outColor)
{




  #define MAKE_NS_RGB(_r,_g,_b) \
    ((PRUint32) ((255 << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))

  RGBColor  buffer;
  long      size = sizeof(RGBColor);
  nsresult  rv = GetICPreference(inKey, &buffer, &size);
  if (rv == NS_OK)
  {
    if (size != sizeof(RGBColor))
    { 
      *outColor = MAKE_NS_RGB(0xff, 0xff, 0xff);
    }
    else
    { 
      *outColor = MAKE_NS_RGB(buffer.red >> 8, buffer.green >> 8, buffer.blue >> 8);
    }
  }
  return rv;
}


NS_IMETHODIMP nsInternetConfigService::GetBoolean(PRUint32 inKey, PRBool *outFlag)
{
  Boolean   buffer;
  long      size = sizeof(Boolean);
  nsresult  rv = GetICPreference(inKey, (void *)&buffer, &size);
  if (rv == NS_OK)
  {
    if ((size_t)size < sizeof(Boolean))
      *outFlag = PR_FALSE;  
    else
      *outFlag = buffer;
  }
  return rv;
}
