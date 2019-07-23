





































#include "CHeaderSniffer.h"
#include "UMacUnicode.h"

#include "UCustomNavServicesDialogs.h"

#include "netCore.h"

#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIURL.h"
#include "nsIStringEnumerator.h"
#include "nsIPrefService.h"
#include "nsIMIMEService.h"
#include "nsIMIMEInfo.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDownload.h"
#include "nsILocalFileMac.h"

const char* const persistContractID = "@mozilla.org/embedding/browser/nsWebBrowserPersist;1";

CHeaderSniffer::CHeaderSniffer(nsIWebBrowserPersist* aPersist, nsIFile* aFile, nsIURI* aURL,
                nsIDOMDocument* aDocument, nsIInputStream* aPostData,
                const nsAString& aSuggestedFilename, PRBool aBypassCache, ESaveFormat aSaveFormat)
: mPersist(aPersist)
, mTmpFile(aFile)
, mURL(aURL)
, mDocument(aDocument)
, mPostData(aPostData)
, mDefaultFilename(aSuggestedFilename)
, mBypassCache(aBypassCache)
, mSaveFormat(aSaveFormat)
{
}

CHeaderSniffer::~CHeaderSniffer()
{
}

NS_IMPL_ISUPPORTS1(CHeaderSniffer, nsIWebProgressListener)

#pragma mark -



NS_IMETHODIMP 
CHeaderSniffer::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, 
                                PRUint32 aStatus)
{  
  if (aStateFlags & nsIWebProgressListener::STATE_START)
  {
    nsCOMPtr<nsIWebBrowserPersist> kungFuDeathGrip(mPersist);   
                                                                
    nsCOMPtr<nsIWebProgressListener> kungFuSuicideGrip(this);   
    
    nsresult rv;
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
    if (!channel) return rv;
    channel->GetContentType(mContentType);
    
    nsCOMPtr<nsIURI> origURI;
    channel->GetOriginalURI(getter_AddRefs(origURI));
    
    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    if (httpChannel)
      httpChannel->GetResponseHeader(nsCAutoString("content-disposition"), mContentDisposition);
    
    mPersist->CancelSave();
    PRBool exists;
    mTmpFile->Exists(&exists);
    if (exists)
        mTmpFile->Remove(PR_FALSE);

    rv = PerformSave(origURI, mSaveFormat);
    if (NS_FAILED(rv))
    {
      
      
    }
  }
  return NS_OK;
}


NS_IMETHODIMP 
CHeaderSniffer::OnProgressChange(nsIWebProgress *aWebProgress, 
           nsIRequest *aRequest, 
           PRInt32 aCurSelfProgress, 
           PRInt32 aMaxSelfProgress, 
           PRInt32 aCurTotalProgress, 
           PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}


NS_IMETHODIMP 
CHeaderSniffer::OnLocationChange(nsIWebProgress *aWebProgress, 
           nsIRequest *aRequest, 
           nsIURI *location)
{
  return NS_OK;
}


NS_IMETHODIMP 
CHeaderSniffer::OnStatusChange(nsIWebProgress *aWebProgress, 
               nsIRequest *aRequest, 
               nsresult aStatus, 
               const PRUnichar *aMessage)
{
  return NS_OK;
}


NS_IMETHODIMP 
CHeaderSniffer::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
  return NS_OK;
}

#pragma mark -

static ESaveFormat SaveFormatFromPrefValue(PRInt32 inPrefValue)
{
  switch (inPrefValue)
  {
    case 0:   return eSaveFormatHTMLComplete;
    default:  
    case 1:   return eSaveFormatHTML;
    case 2:   return eSaveFormatPlainText;
  }
}

static PRInt32 PrefValueFromSaveFormat(ESaveFormat inSaveFormat)
{
  switch (inSaveFormat)
  {
    case eSaveFormatPlainText:    return 2;
    default:  
    case eSaveFormatHTML:         return 1;
    case eSaveFormatHTMLComplete: return 0;
  }
}

nsresult CHeaderSniffer::PerformSave(nsIURI* inOriginalURI, const ESaveFormat inSaveFormat)
{
    nsresult rv;
    
    
    
    PRBool isHTML = (mDocument && mContentType.Equals("text/html") ||
                     mContentType.Equals("text/xml") ||
                     mContentType.Equals("application/xhtml+xml"));
    
    
    nsCOMPtr<nsIPrefService> prefs(do_GetService("@mozilla.org/preferences-service;1", &rv));
    if (!prefs)
        return rv;
    nsCOMPtr<nsIPrefBranch> dirBranch;
    prefs->GetBranch("browser.download.", getter_AddRefs(dirBranch));
    PRInt32 filterIndex = 0;
    if (inSaveFormat != eSaveFormatUnspecified) {
      filterIndex = PrefValueFromSaveFormat(inSaveFormat);
    }
    else if (dirBranch) {
        nsresult rv = dirBranch->GetIntPref("save_converter_index", &filterIndex);
        if (NS_FAILED(rv))
            filterIndex = 0;
    }

    
    nsAutoString defaultFileName;
    if (!mContentDisposition.IsEmpty()) {
        
        PRInt32 index = mContentDisposition.Find("filename=");
        if (index >= 0) {
            
            index += 9;
            AppendUTF8toUTF16(Substring(mContentDisposition, index),
                              defaultFileName);
        }
    }
    
    if (defaultFileName.IsEmpty()) {
        nsCOMPtr<nsIURL> url(do_QueryInterface(mURL));
        if (url) {
            nsCAutoString fileNameCString;
            url->GetFileName(fileNameCString); 
            AppendUTF8toUTF16(fileNameCString, defaultFileName);
        }
    }
    
    if (defaultFileName.IsEmpty() && mDocument && isHTML) {
        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
        if (htmlDoc)
            htmlDoc->GetTitle(defaultFileName); 
    }
    
    if (defaultFileName.IsEmpty()) {
        
        defaultFileName = mDefaultFilename;
    }

    if (defaultFileName.IsEmpty() && mURL) {
        
        nsCAutoString hostName;
        mURL->GetHost(hostName);
        AppendUTF8toUTF16(hostName, defaultFileName);
    }
    
    
    if (defaultFileName.IsEmpty())
        defaultFileName.AssignLiteral("untitled");
        
    
    for (PRUint32 i = 0; i < defaultFileName.Length(); i++)
        if (defaultFileName[i] == ':' || defaultFileName[i] == '/')
            defaultFileName.SetCharAt(i, PRUnichar(' '));
            
    
    nsCOMPtr<nsIURI> fileURI(do_CreateInstance("@mozilla.org/network/standard-url;1"));
    nsCOMPtr<nsIURL> fileURL(do_QueryInterface(fileURI, &rv));
    if (!fileURL)
        return rv;
     
    fileURL->SetFilePath(NS_ConvertUTF16toUTF8(defaultFileName));
    
    nsCAutoString fileExtension;
    fileURL->GetFileExtension(fileExtension);
    
    PRBool setExtension = PR_FALSE;
    if (mContentType.Equals("text/html")) {
        if (fileExtension.IsEmpty() || (!fileExtension.Equals("htm") && !fileExtension.Equals("html"))) {
            defaultFileName.AppendLiteral(".html");
            setExtension = PR_TRUE;
        }
    }
    
    if (!setExtension && fileExtension.IsEmpty()) {
        nsCOMPtr<nsIMIMEService> mimeService(do_GetService("@mozilla.org/mime;1", &rv));
        if (!mimeService)
            return rv;
        nsCOMPtr<nsIMIMEInfo> mimeInfo;
        rv = mimeService->GetFromTypeAndExtension(mContentType, EmptyCString(), getter_AddRefs(mimeInfo));
        if (!mimeInfo)
          return rv;

        nsCOMPtr<nsIUTF8StringEnumerator> extensions;
        mimeInfo->GetFileExtensions(getter_AddRefs(extensions));
        
        PRBool hasMore;
        extensions->HasMore(&hasMore);
        if (hasMore) {
            nsCAutoString ext;
            extensions->GetNext(ext);
            defaultFileName.Append(PRUnichar('.'));
            AppendUTF8toUTF16(ext, defaultFileName);
        }
    }

    
    FSSpec       destFileSpec;
    bool         isReplacing = false;

    {
        Str255          defaultName;
        bool            result;

        CPlatformUCSConversion::GetInstance()->UCSToPlatform(defaultFileName, defaultName);
#ifndef XP_MACOSX
        char            tempBuf1[256], tempBuf2[64];
        ::CopyPascalStringToC(defaultName, tempBuf1);        
        ::CopyCStringToPascal(NS_TruncNodeName(tempBuf1, tempBuf2), defaultName);
#endif
        if (isHTML) {
            ESaveFormat saveFormat = SaveFormatFromPrefValue(filterIndex);
            UNavServicesDialogs::LCustomFileDesignator customDesignator;
            result = customDesignator.AskDesignateFile(defaultName, saveFormat);
            if (!result)
                return NS_OK;       
            filterIndex = PrefValueFromSaveFormat(saveFormat);
            customDesignator.GetFileSpec(destFileSpec);
            isReplacing = customDesignator.IsReplacing();
        }
        else {
            UNavServicesDialogs::LFileDesignator stdDesignator;
            result = stdDesignator.AskDesignateFile(defaultName);
            if (!result)
                return NS_OK;       
            stdDesignator.GetFileSpec(destFileSpec);
            isReplacing = stdDesignator.IsReplacing();        
        }

        
        
        
        
        
        
        
        
        if (LEventDispatcher::GetCurrentEventDispatcher()) { 
            EventRecord theEvent;
            while (::WaitNextEvent(updateMask | activMask, &theEvent, 0, nil))
                LEventDispatcher::GetCurrentEventDispatcher()->DispatchEvent(theEvent);
        }
    }

    
    if (inSaveFormat == eSaveFormatUnspecified && isHTML)
      dirBranch->SetIntPref("save_converter_index", filterIndex);

    nsCOMPtr<nsILocalFileMac> destFile;
    
    rv = NS_NewLocalFileWithFSSpec(&destFileSpec, PR_TRUE, getter_AddRefs(destFile));
    if (NS_FAILED(rv))
      return rv;
    
    if (isReplacing) {
      PRBool exists;
      rv = destFile->Exists(&exists);
      if (NS_SUCCEEDED(rv) && exists)
        rv = destFile->Remove(PR_TRUE);
      if (NS_FAILED(rv))
        return rv;
    }

    
    if (isHTML && filterIndex == 2)
      mContentType = "text/plain";
    
    nsCOMPtr<nsISupports> sourceData;
    if (isHTML && filterIndex != 1)
        sourceData = do_QueryInterface(mDocument);    
    else
        sourceData = do_QueryInterface(mURL);         

    return InitiateDownload(sourceData, destFile, inOriginalURI);
}



nsresult CHeaderSniffer::InitiateDownload(nsISupports* inSourceData, nsILocalFile* inDestFile, nsIURI* inOriginalURI)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIWebBrowserPersist> webPersist = do_CreateInstance(persistContractID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIURI> sourceURI = do_QueryInterface(inSourceData);

  PRInt64 timeNow = PR_Now();
  
  nsAutoString fileDisplayName;
  inDestFile->GetLeafName(fileDisplayName);
  
  nsCOMPtr<nsIDownload> downloader = do_CreateInstance(NS_TRANSFER_CONTRACTID);
  
  rv = downloader->Init(inOriginalURI, inDestFile, fileDisplayName.get(), nsnull, timeNow, webPersist);
  if (NS_FAILED(rv)) return rv;
    
  PRInt32 flags = nsIWebBrowserPersist::PERSIST_FLAGS_NO_CONVERSION | 
                  nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES;
  if (mBypassCache)
    flags |= nsIWebBrowserPersist::PERSIST_FLAGS_BYPASS_CACHE;
  else
    flags |= nsIWebBrowserPersist::PERSIST_FLAGS_FROM_CACHE;

  webPersist->SetPersistFlags(flags);
    
  if (sourceURI)
  {
    rv = webPersist->SaveURI(sourceURI, nsnull, nsnull, mPostData, nsnull, inDestFile);
  }
  else
  {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(inSourceData, &rv);
    if (!domDoc) return rv;  
    
    PRInt32 encodingFlags = 0;
    nsCOMPtr<nsILocalFile> filesFolder;
    
    if (!mContentType.Equals("text/plain")) {
      
      
      filesFolder = do_CreateInstance("@mozilla.org/file/local;1");
      nsAutoString unicodePath;
      inDestFile->GetPath(unicodePath);
      filesFolder->InitWithPath(unicodePath);
      
      nsAutoString leafName;
      filesFolder->GetLeafName(leafName);
      nsAutoString nameMinusExt(leafName);
      PRInt32 index = nameMinusExt.RFind(".");
      if (index >= 0)
          nameMinusExt.Left(nameMinusExt, index);
      nameMinusExt.AppendLiteral(" Files"); 
      filesFolder->SetLeafName(nameMinusExt);
      PRBool exists = PR_FALSE;
      filesFolder->Exists(&exists);
      if (!exists) {
          rv = filesFolder->Create(nsILocalFile::DIRECTORY_TYPE, 0755);
          if (NS_FAILED(rv))
            return rv;
      }
    }
    else
    {
      encodingFlags |= nsIWebBrowserPersist::ENCODE_FLAGS_FORMATTED |
                        nsIWebBrowserPersist::ENCODE_FLAGS_ABSOLUTE_LINKS |
                        nsIWebBrowserPersist::ENCODE_FLAGS_NOFRAMES_CONTENT;
    }
    rv = webPersist->SaveDocument(domDoc, inDestFile, filesFolder, mContentType.get(), encodingFlags, 80);
  }
  
  return rv;
}
