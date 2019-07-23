










































#include "prprf.h"

#include "nsDirIndexParser.h"
#include "nsReadableUtils.h"
#include "nsDirIndex.h"
#include "nsEscape.h"
#include "nsIServiceManager.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsCRT.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"

NS_IMPL_ISUPPORTS3(nsDirIndexParser,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIDirIndexParser)

nsDirIndexParser::nsDirIndexParser() {
}

nsresult
nsDirIndexParser::Init() {
  mLineStart = 0;
  mHasDescription = PR_FALSE;
  mFormat = nsnull;

  
  
  NS_NAMED_LITERAL_CSTRING(kFallbackEncoding, "ISO-8859-1");
  nsXPIDLString defCharset;
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs) {
    nsCOMPtr<nsIPrefLocalizedString> prefVal;
    prefs->GetComplexValue("intl.charset.default",
                           NS_GET_IID(nsIPrefLocalizedString),
                           getter_AddRefs(prefVal));
    if (prefVal)
      prefVal->ToString(getter_Copies(defCharset));
  }
  if (!defCharset.IsEmpty())
    LossyCopyUTF16toASCII(defCharset, mEncoding); 
  else
    mEncoding.Assign(kFallbackEncoding);
 
  nsresult rv;
  
  if (gRefCntParser++ == 0)
    rv = CallGetService(NS_ITEXTTOSUBURI_CONTRACTID, &gTextToSubURI);
  else
    rv = NS_OK;

  return rv;
}

nsDirIndexParser::~nsDirIndexParser() {
  delete[] mFormat;
  
  if (--gRefCntParser == 0) {
    NS_IF_RELEASE(gTextToSubURI);
  }
}

NS_IMETHODIMP
nsDirIndexParser::SetListener(nsIDirIndexListener* aListener) {
  mListener = aListener;
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::GetListener(nsIDirIndexListener** aListener) {
  NS_IF_ADDREF(*aListener = mListener.get());
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::GetComment(char** aComment) {
  *aComment = ToNewCString(mComment);

  if (!*aComment)
    return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::SetEncoding(const char* aEncoding) {
  mEncoding.Assign(aEncoding);
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::GetEncoding(char** aEncoding) {
  *aEncoding = ToNewCString(mEncoding);

  if (!*aEncoding)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::OnStartRequest(nsIRequest* aRequest, nsISupports* aCtxt) {
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::OnStopRequest(nsIRequest *aRequest, nsISupports *aCtxt,
                                nsresult aStatusCode) {
  
  if (mBuf.Length() > (PRUint32) mLineStart) {
    ProcessData(aRequest, aCtxt);
  }

  return NS_OK;
}

nsDirIndexParser::Field
nsDirIndexParser::gFieldTable[] = {
  { "Filename", FIELD_FILENAME },
  { "Description", FIELD_DESCRIPTION },
  { "Content-Length", FIELD_CONTENTLENGTH },
  { "Last-Modified", FIELD_LASTMODIFIED },
  { "Content-Type", FIELD_CONTENTTYPE },
  { "File-Type", FIELD_FILETYPE },
  { nsnull, FIELD_UNKNOWN }
};

nsrefcnt nsDirIndexParser::gRefCntParser = 0;
nsITextToSubURI *nsDirIndexParser::gTextToSubURI;

nsresult
nsDirIndexParser::ParseFormat(const char* aFormatStr) {
  
  

  delete[] mFormat;

  
  
  const char* pos = aFormatStr;
  int num = 0;
  do {
    while (*pos && nsCRT::IsAsciiSpace(PRUnichar(*pos)))
      ++pos;
    
    ++num;

    if (! *pos)
      break;

    while (*pos && !nsCRT::IsAsciiSpace(PRUnichar(*pos)))
      ++pos;

  } while (*pos);

  mFormat = new int[num+1];
  mFormat[num] = -1;
  
  int formatNum=0;
  do {
    while (*aFormatStr && nsCRT::IsAsciiSpace(PRUnichar(*aFormatStr)))
      ++aFormatStr;
    
    if (! *aFormatStr)
      break;

    nsCAutoString name;
    PRInt32     len = 0;
    while (aFormatStr[len] && !nsCRT::IsAsciiSpace(PRUnichar(aFormatStr[len])))
      ++len;
    name.SetCapacity(len + 1);
    name.Append(aFormatStr, len);
    aFormatStr += len;
    
    
    name.SetLength(nsUnescapeCount(name.BeginWriting()));

    
    if (name.LowerCaseEqualsLiteral("description"))
      mHasDescription = PR_TRUE;
    
    for (Field* i = gFieldTable; i->mName; ++i) {
      if (name.EqualsIgnoreCase(i->mName)) {
        mFormat[formatNum] = i->mType;
        ++formatNum;
        break;
      }
    }

  } while (*aFormatStr);
  
  return NS_OK;
}

nsresult
nsDirIndexParser::ParseData(nsIDirIndex *aIdx, char* aDataStr) {
  
  

  if (!mFormat) {
    
    return NS_OK;
  }

  nsresult rv = NS_OK;

  nsCAutoString filename;

  for (PRInt32 i = 0; mFormat[i] != -1; ++i) {
    
    
    if (! *aDataStr)
      break;

    while (*aDataStr && nsCRT::IsAsciiSpace(*aDataStr))
      ++aDataStr;

    char    *value = aDataStr;

    if (*aDataStr == '"' || *aDataStr == '\'') {
      
      const char quotechar = *(aDataStr++);
      ++value;
      while (*aDataStr && *aDataStr != quotechar)
        ++aDataStr;
      *aDataStr++ = '\0';

      if (! aDataStr) {
        NS_WARNING("quoted value not terminated");
      }
    } else {
      
      value = aDataStr;
      while (*aDataStr && (!nsCRT::IsAsciiSpace(*aDataStr)))
        ++aDataStr;
      *aDataStr++ = '\0';
    }

    fieldType t = fieldType(mFormat[i]);
    switch (t) {
    case FIELD_FILENAME: {
      
      filename = value;
      
      PRBool  success = PR_FALSE;
      
      nsAutoString entryuri;
      
      if (gTextToSubURI) {
        PRUnichar   *result = nsnull;
        if (NS_SUCCEEDED(rv = gTextToSubURI->UnEscapeAndConvert(mEncoding.get(), filename.get(),
                                                                &result)) && (result)) {
          if (*result) {
            aIdx->SetLocation(filename.get());
            if (!mHasDescription)
              aIdx->SetDescription(result);
            success = PR_TRUE;
          }
          NS_Free(result);
        } else {
          NS_WARNING("UnEscapeAndConvert error");
        }
      }
      
      if (success == PR_FALSE) {
        
        
        
        
        aIdx->SetLocation(filename.get());
        if (!mHasDescription) {
          aIdx->SetDescription(NS_ConvertUTF8toUTF16(value).get());
        }
      }
    }
      break;
    case FIELD_DESCRIPTION:
      nsUnescape(value);
      aIdx->SetDescription(NS_ConvertUTF8toUTF16(value).get());
      break;
    case FIELD_CONTENTLENGTH:
      {
        PRInt64 len;
        PRInt32 status = PR_sscanf(value, "%lld", &len);
        if (status == 1)
          aIdx->SetSize(len);
        else
          aIdx->SetSize(LL_MAXUINT); 
      }
      break;
    case FIELD_LASTMODIFIED:
      {
        PRTime tm;
        nsUnescape(value);
        if (PR_ParseTimeString(value, PR_FALSE, &tm) == PR_SUCCESS) {
          aIdx->SetLastModified(tm);
        }
      }
      break;
    case FIELD_CONTENTTYPE:
      aIdx->SetContentType(value);
      break;
    case FIELD_FILETYPE:
      
      nsUnescape(value);
      if (!nsCRT::strcasecmp(value, "directory")) {
        aIdx->SetType(nsIDirIndex::TYPE_DIRECTORY);
      } else if (!nsCRT::strcasecmp(value, "file")) {
        aIdx->SetType(nsIDirIndex::TYPE_FILE);
      } else if (!nsCRT::strcasecmp(value, "symbolic-link")) {
        aIdx->SetType(nsIDirIndex::TYPE_SYMLINK);
      } else {
        aIdx->SetType(nsIDirIndex::TYPE_UNKNOWN);
      }
      break;
    case FIELD_UNKNOWN:
      
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDirIndexParser::OnDataAvailable(nsIRequest *aRequest, nsISupports *aCtxt,
                                  nsIInputStream *aStream,
                                  PRUint32 aSourceOffset,
                                  PRUint32 aCount) {
  if (aCount < 1)
    return NS_OK;
  
  PRInt32 len = mBuf.Length();
  
  
  
  if (!EnsureStringLength(mBuf, len + aCount))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsresult rv;
  PRUint32 count;
  rv = aStream->Read(mBuf.BeginWriting() + len, aCount, &count);
  if (NS_FAILED(rv)) return rv;

  
  
  
  mBuf.SetLength(len + count);

  return ProcessData(aRequest, aCtxt);
}

nsresult
nsDirIndexParser::ProcessData(nsIRequest *aRequest, nsISupports *aCtxt) {
  if (!mListener)
    return NS_ERROR_FAILURE;
  
  PRInt32     numItems = 0;
  
  while(PR_TRUE) {
    ++numItems;
    
    PRInt32             eol = mBuf.FindCharInSet("\n\r", mLineStart);
    if (eol < 0)        break;
    mBuf.SetCharAt(PRUnichar('\0'), eol);
    
    const char  *line = mBuf.get() + mLineStart;
    
    PRInt32 lineLen = eol - mLineStart;
    mLineStart = eol + 1;
    
    if (lineLen >= 4) {
      nsresult  rv;
      const char        *buf = line;
      
      if (buf[0] == '1') {
        if (buf[1] == '0') {
          if (buf[2] == '0' && buf[3] == ':') {
            
          } else if (buf[2] == '1' && buf[3] == ':') {
            
            mComment.Append(buf + 4);

            char    *value = ((char *)buf) + 4;
            nsUnescape(value);
            mListener->OnInformationAvailable(aRequest, aCtxt, NS_ConvertUTF8toUTF16(value));

          } else if (buf[2] == '2' && buf[3] == ':') {
            
            mComment.Append(buf + 4);
          }
        }
      } else if (buf[0] == '2') {
        if (buf[1] == '0') {
          if (buf[2] == '0' && buf[3] == ':') {
            
            rv = ParseFormat(buf + 4);
            if (NS_FAILED(rv)) {
              return rv;
            }
          } else if (buf[2] == '1' && buf[3] == ':') {
            
            nsCOMPtr<nsIDirIndex> idx = do_CreateInstance("@mozilla.org/dirIndex;1",&rv);
            if (NS_FAILED(rv))
              return rv;
            
            rv = ParseData(idx, ((char *)buf) + 4);
            if (NS_FAILED(rv)) {
              return rv;
            }

            mListener->OnIndexAvailable(aRequest, aCtxt, idx);
          }
        }
      } else if (buf[0] == '3') {
        if (buf[1] == '0') {
          if (buf[2] == '0' && buf[3] == ':') {
            
          } else if (buf[2] == '1' && buf[3] == ':') {
            
            int i = 4;
            while (buf[i] && nsCRT::IsAsciiSpace(buf[i]))
              ++i;
            
            if (buf[i])
              SetEncoding(buf+i);
          }
        }
      }
    }
  }
  
  return NS_OK;
}
