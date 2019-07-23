






































#include "nsIndexedToHTML.h"
#include "nsNetUtil.h"
#include "netCore.h"
#include "nsStringStream.h"
#include "nsIFileURL.h"
#include "nsEscape.h"
#include "nsIDirIndex.h"
#include "prtime.h"
#include "nsDateTimeFormatCID.h"
#include "nsURLHelper.h"
#include "nsCRT.h"
#include "nsIPlatformCharset.h"
#include "nsInt64.h"

NS_IMPL_THREADSAFE_ISUPPORTS4(nsIndexedToHTML,
                              nsIDirIndexListener,
                              nsIStreamConverter,
                              nsIRequestObserver,
                              nsIStreamListener)

static void ConvertNonAsciiToNCR(const nsAString& in, nsAFlatString& out)
{
  nsAString::const_iterator start, end;

  in.BeginReading(start);
  in.EndReading(end);

  out.Truncate();

  while (start != end) {
    if (*start < 128) {
      out.Append(*start++);
    } else {
      out.AppendLiteral("&#x");
      nsAutoString hex;
      hex.AppendInt(*start++, 16);
      out.Append(hex);
      out.Append((PRUnichar)';');
    }
  }
}

NS_METHOD
nsIndexedToHTML::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
    nsresult rv;
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;
    
    nsIndexedToHTML* _s = new nsIndexedToHTML();
    if (_s == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    
    rv = _s->QueryInterface(aIID, aResult);
    return rv;
}

nsresult
nsIndexedToHTML::Init(nsIStreamListener* aListener) {
    nsresult rv = NS_OK;

    mListener = aListener;

    mDateTime = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv);

    nsCOMPtr<nsIStringBundleService> sbs =
        do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = sbs->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(mBundle));

    mExpectAbsLoc = PR_FALSE;

    return rv;
}

NS_IMETHODIMP
nsIndexedToHTML::Convert(nsIInputStream* aFromStream,
                         const char* aFromType,
                         const char* aToType,
                         nsISupports* aCtxt,
                         nsIInputStream** res) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIndexedToHTML::AsyncConvertData(const char *aFromType,
                                  const char *aToType,
                                  nsIStreamListener *aListener,
                                  nsISupports *aCtxt) {
    return Init(aListener);
}

NS_IMETHODIMP
nsIndexedToHTML::OnStartRequest(nsIRequest* request, nsISupports *aContext) {
    nsresult rv;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    channel->SetContentType(NS_LITERAL_CSTRING("text/html"));

    mParser = do_CreateInstance("@mozilla.org/dirIndexParser;1",&rv);
    if (NS_FAILED(rv)) return rv;

    rv = mParser->SetListener(this);
    if (NS_FAILED(rv)) return rv;
    
    rv = mParser->OnStartRequest(request, aContext);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString baseUri,titleUri;
    rv = uri->GetAsciiSpec(baseUri);
    if (NS_FAILED(rv)) return rv;
    titleUri = baseUri;

    nsCString parentStr;

    
    
    
    
    
    
    

    PRBool isScheme = PR_FALSE;
    PRBool isSchemeFile = PR_FALSE;
    if (NS_SUCCEEDED(uri->SchemeIs("ftp", &isScheme)) && isScheme) {

        
        
        
        nsCAutoString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;
        if (baseUri.Last() != '/' && !path.LowerCaseEqualsLiteral("/%2f")) {
            baseUri.Append('/');
            path.Append('/');
            uri->SetPath(path);
        }

        
        
        
        
        nsCAutoString pw;
        rv = uri->GetPassword(pw);
        if (NS_FAILED(rv)) return rv;
        if (!pw.IsEmpty()) {
             nsCOMPtr<nsIURI> newUri;
             rv = uri->Clone(getter_AddRefs(newUri));
             if (NS_FAILED(rv)) return rv;
             rv = newUri->SetPassword(EmptyCString());
             if (NS_FAILED(rv)) return rv;
             rv = newUri->GetAsciiSpec(titleUri);
             if (NS_FAILED(rv)) return rv;
             if (titleUri.Last() != '/' && !path.LowerCaseEqualsLiteral("/%2f"))
                 titleUri.Append('/');
        }

        if (!path.EqualsLiteral("//") && !path.LowerCaseEqualsLiteral("/%2f")) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."),parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    } else if (NS_SUCCEEDED(uri->SchemeIs("file", &isSchemeFile)) && isSchemeFile) {
        nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri);
        nsCOMPtr<nsIFile> file;
        rv = fileUrl->GetFile(getter_AddRefs(file));
        if (NS_FAILED(rv)) return rv;
        nsCOMPtr<nsILocalFile> lfile = do_QueryInterface(file, &rv);
        if (NS_FAILED(rv)) return rv;
        lfile->SetFollowLinks(PR_TRUE);
        
        nsCAutoString url;
        rv = net_GetURLSpecFromFile(file, url);
        if (NS_FAILED(rv)) return rv;
        baseUri.Assign(url);
        
        nsCOMPtr<nsIFile> parent;
        rv = file->GetParent(getter_AddRefs(parent));
        
        if (parent && NS_SUCCEEDED(rv)) {
            net_GetURLSpecFromFile(parent, url);
            if (NS_FAILED(rv)) return rv;
            parentStr.Assign(url);
        }

        
        rv = mParser->SetEncoding("UTF-8");
        NS_ENSURE_SUCCESS(rv, rv);

    } else if (NS_SUCCEEDED(uri->SchemeIs("gopher", &isScheme)) && isScheme) {
        mExpectAbsLoc = PR_TRUE;
    } else if (NS_SUCCEEDED(uri->SchemeIs("jar", &isScheme)) && isScheme) {
        nsCAutoString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;

        
        
        
        
        
        
        if (!StringEndsWith(path, NS_LITERAL_CSTRING("!/"))) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."), parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    }
    else {
        
        
        nsCAutoString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;
        if (baseUri.Last() != '/') {
            baseUri.Append('/');
            path.Append('/');
            uri->SetPath(path);
        }
        if (!path.EqualsLiteral("/")) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."), parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    }

    nsString buffer;
    buffer.AssignLiteral("<?xml version=\"1.0\" encoding=\"");
    
    
    
    
    
    

    nsXPIDLCString encoding;
    rv = mParser->GetEncoding(getter_Copies(encoding));
    if (NS_FAILED(rv)) return rv;

    AppendASCIItoUTF16(encoding, buffer);

    buffer.AppendLiteral("\"?>\n"
                         "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
                         "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");

    
    

    buffer.AppendLiteral("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head><title>");

    nsXPIDLString title;

    if (!mTextToSubURI) {
        mTextToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsXPIDLString unEscapeSpec;
    rv = mTextToSubURI->UnEscapeAndConvert(encoding, titleUri.get(),
                                           getter_Copies(unEscapeSpec));
    
    
    
    
    if (NS_FAILED(rv) && isSchemeFile) {
        nsCOMPtr<nsIPlatformCharset> platformCharset(do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv));
        NS_ENSURE_SUCCESS(rv, rv);
        nsCAutoString charset;
        rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, charset);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mTextToSubURI->UnEscapeAndConvert(charset.get(), titleUri.get(),
                                               getter_Copies(unEscapeSpec));
    }
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString htmlEscSpec;
    htmlEscSpec.Adopt(nsEscapeHTML2(unEscapeSpec.get(),
                                    unEscapeSpec.Length()));

    const PRUnichar* formatTitle[] = {
        htmlEscSpec.get()
    };

    rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("DirTitle").get(),
                                       formatTitle,
                                       sizeof(formatTitle)/sizeof(PRUnichar*),
                                       getter_Copies(title));
    if (NS_FAILED(rv)) return rv;

    
    
    nsAutoString strNCR;
    ConvertNonAsciiToNCR(title, strNCR);
    buffer.Append(strNCR);

    buffer.AppendLiteral("</title>");    

    
    
    
    
    
    
    
    

    if (baseUri.FindChar('"') == kNotFound)
    {
        
        
        
        buffer.AppendLiteral("<base href=\"");
        AppendASCIItoUTF16(baseUri, buffer);
        buffer.AppendLiteral("\"/>\n");
    }
    else
    {
        NS_ERROR("broken protocol handler didn't escape double-quote.");
    }

    buffer.AppendLiteral("<style type=\"text/css\">\n"
                         "img { border: 0; padding: 0 2px; vertical-align: text-bottom; }\n"
                         "td  { font-family: monospace; padding: 2px 3px; text-align: right; vertical-align: bottom; white-space: pre; }\n"
                         "td:first-child { text-align: left; padding: 2px 10px 2px 3px; }\n"
                         "table { border: 0; }\n"
                         "a.symlink { font-style: italic; }\n"
                         "</style>\n"
                         "</head>\n<body>\n<h1>");
    
    const PRUnichar* formatHeading[] = {
        htmlEscSpec.get()
    };

    rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("DirTitle").get(),
                                       formatHeading,
                                       sizeof(formatHeading)/sizeof(PRUnichar*),
                                       getter_Copies(title));
    if (NS_FAILED(rv)) return rv;
    
    ConvertNonAsciiToNCR(title, strNCR);
    buffer.Append(strNCR);
    buffer.AppendLiteral("</h1>\n<hr/><table>\n");

    

    if (!parentStr.IsEmpty()) {
        nsXPIDLString parentText;
        rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirGoUp").get(),
                                        getter_Copies(parentText));
        if (NS_FAILED(rv)) return rv;
        
        ConvertNonAsciiToNCR(parentText, strNCR);
        buffer.AppendLiteral("<tr><td colspan=\"3\"><a href=\"");
        AppendASCIItoUTF16(parentStr, buffer);
        buffer.AppendLiteral("\">");
        buffer.Append(strNCR);
        buffer.AppendLiteral("</a></td></tr>\n");
    }

    
    

    rv = mListener->OnStartRequest(request, aContext);
    if (NS_FAILED(rv)) return rv;

    
    
    request->GetStatus(&rv);
    if (NS_FAILED(rv)) return rv;

    rv = FormatInputStream(request, aContext, buffer);
    return rv;
}

NS_IMETHODIMP
nsIndexedToHTML::OnStopRequest(nsIRequest* request, nsISupports *aContext,
                               nsresult aStatus) {
    if (NS_SUCCEEDED(aStatus)) {
        nsString buffer;
        buffer.AssignLiteral("</table><hr/></body></html>\n");

        aStatus = FormatInputStream(request, aContext, buffer);
    }

    mParser->OnStopRequest(request, aContext, aStatus);
    mParser = 0;
    
    return mListener->OnStopRequest(request, aContext, aStatus);
}

nsresult
nsIndexedToHTML::FormatInputStream(nsIRequest* aRequest, nsISupports *aContext, const nsAString &aBuffer) 
{
    nsresult rv = NS_OK;

    
    if (!mUnicodeEncoder) {
      nsXPIDLCString encoding;
      rv = mParser->GetEncoding(getter_Copies(encoding));
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsICharsetConverterManager> charsetConverterManager;
        charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
        rv = charsetConverterManager->GetUnicodeEncoder(encoding.get(), 
                                                          getter_AddRefs(mUnicodeEncoder));
        if (NS_SUCCEEDED(rv))
            rv = mUnicodeEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, 
                                                       nsnull, (PRUnichar)'?');
      }
    }

    
    char *buffer = nsnull;
    PRInt32 dstLength;
    if (NS_SUCCEEDED(rv)) {
      PRInt32 unicharLength = aBuffer.Length();
      rv = mUnicodeEncoder->GetMaxLength(PromiseFlatString(aBuffer).get(), 
                                         unicharLength, &dstLength);
      if (NS_SUCCEEDED(rv)) {
        buffer = (char *) nsMemory::Alloc(dstLength);
        NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

        rv = mUnicodeEncoder->Convert(PromiseFlatString(aBuffer).get(), &unicharLength, 
                                      buffer, &dstLength);
        if (NS_SUCCEEDED(rv)) {
          PRInt32 finLen = 0;
          rv = mUnicodeEncoder->Finish(buffer + dstLength, &finLen);
          if (NS_SUCCEEDED(rv))
            dstLength += finLen;
        }
      }
    }

    
    if (NS_FAILED(rv)) {
      rv = NS_OK;
      if (buffer) {
        nsMemory::Free(buffer);
        buffer = nsnull;
      }
    }

    nsCOMPtr<nsIInputStream> inputData;
    if (buffer) {
      rv = NS_NewCStringInputStream(getter_AddRefs(inputData), Substring(buffer, buffer + dstLength));
      nsMemory::Free(buffer);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mListener->OnDataAvailable(aRequest, aContext,
                                      inputData, 0, dstLength);
    }
    else {
      NS_ConvertUTF16toUTF8 utf8Buffer(aBuffer);
      rv = NS_NewCStringInputStream(getter_AddRefs(inputData), utf8Buffer);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mListener->OnDataAvailable(aRequest, aContext,
                                      inputData, 0, utf8Buffer.Length());
    }
    return (rv);
}

NS_IMETHODIMP
nsIndexedToHTML::OnDataAvailable(nsIRequest *aRequest,
                                 nsISupports *aCtxt,
                                 nsIInputStream* aInput,
                                 PRUint32 aOffset,
                                 PRUint32 aCount) {
    return mParser->OnDataAvailable(aRequest, aCtxt, aInput, aOffset, aCount);
}

NS_IMETHODIMP
nsIndexedToHTML::OnIndexAvailable(nsIRequest *aRequest,
                                  nsISupports *aCtxt,
                                  nsIDirIndex *aIndex) {
    nsresult rv;
    if (!aIndex)
        return NS_ERROR_NULL_POINTER;

    nsString pushBuffer;
    pushBuffer.AppendLiteral("<tr>\n <td><a");

    PRUint32 type;
    aIndex->GetType(&type);
    if (type == nsIDirIndex::TYPE_SYMLINK) {
        pushBuffer.AppendLiteral(" class=\"symlink\"");
    }

    pushBuffer.AppendLiteral(" href=\"");

    nsXPIDLCString loc;
    aIndex->GetLocation(getter_Copies(loc));

    if (!mTextToSubURI) {
        mTextToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsXPIDLCString encoding;
    rv = mParser->GetEncoding(getter_Copies(encoding));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString unEscapeSpec;
    rv = mTextToSubURI->UnEscapeAndConvert(encoding, loc,
                                           getter_Copies(unEscapeSpec));
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString escapeBuf;

    NS_ConvertUTF16toUTF8 utf8UnEscapeSpec(unEscapeSpec);

    
    PRUint32 escFlags;
    
    
    
    if (mExpectAbsLoc &&
        NS_SUCCEEDED(net_ExtractURLScheme(utf8UnEscapeSpec, nsnull, nsnull, nsnull))) {
        
        escFlags = esc_Forced | esc_OnlyASCII | esc_AlwaysCopy | esc_Minimal;
    }
    else {
        
        
        
        
        
        escFlags = esc_Forced | esc_OnlyASCII | esc_AlwaysCopy | esc_FileBaseName | esc_Colon | esc_Directory;
    }
    NS_EscapeURL(utf8UnEscapeSpec.get(), utf8UnEscapeSpec.Length(), escFlags, escapeBuf);

    AppendUTF8toUTF16(escapeBuf, pushBuffer);

    pushBuffer.AppendLiteral("\"><img src=\"");

    switch (type) {
    case nsIDirIndex::TYPE_DIRECTORY:
    case nsIDirIndex::TYPE_SYMLINK:
        pushBuffer.AppendLiteral("resource://gre/res/html/gopher-menu.gif\" alt=\"Directory: ");
        break;
    case nsIDirIndex::TYPE_FILE:
    case nsIDirIndex::TYPE_UNKNOWN:
        pushBuffer.AppendLiteral("resource://gre/res/html/gopher-unknown.gif\" alt=\"File: ");
        break;
    }
    pushBuffer.AppendLiteral("\"/>");

    nsXPIDLString tmp;
    aIndex->GetDescription(getter_Copies(tmp));
    PRUnichar* escaped = nsEscapeHTML2(tmp.get(), tmp.Length());
    pushBuffer.Append(escaped);
    nsMemory::Free(escaped);

    pushBuffer.AppendLiteral("</a></td>\n <td>");

    PRInt64 size;
    aIndex->GetSize(&size);
    
    if (nsUint64(PRUint64(size)) != nsUint64(LL_MAXUINT) &&
        type != nsIDirIndex::TYPE_DIRECTORY &&
        type != nsIDirIndex::TYPE_SYMLINK) {
        nsAutoString  sizeString;
        FormatSizeString(size, sizeString);
        pushBuffer.Append(sizeString);
    }

    pushBuffer.AppendLiteral("</td>\n <td>");

    PRTime t;
    aIndex->GetLastModified(&t);

    if (t == -1) {
        pushBuffer.AppendLiteral("</td>\n <td>");
    } else {
        nsAutoString formatted;
        nsAutoString strNCR;    
        mDateTime->FormatPRTime(nsnull,
                                kDateFormatShort,
                                kTimeFormatNone,
                                t,
                                formatted);
        ConvertNonAsciiToNCR(formatted, strNCR);
        pushBuffer.Append(strNCR);
        pushBuffer.AppendLiteral("</td>\n <td>");
        mDateTime->FormatPRTime(nsnull,
                                kDateFormatNone,
                                kTimeFormatSeconds,
                                t,
                                formatted);
        ConvertNonAsciiToNCR(formatted, strNCR);
        pushBuffer.Append(strNCR);
    }

    pushBuffer.AppendLiteral("</td>\n</tr>\n");

    return FormatInputStream(aRequest, aCtxt, pushBuffer);
}

NS_IMETHODIMP
nsIndexedToHTML::OnInformationAvailable(nsIRequest *aRequest,
                                        nsISupports *aCtxt,
                                        const nsAString& aInfo) {
    nsAutoString pushBuffer;
    PRUnichar* escaped = nsEscapeHTML2(PromiseFlatString(aInfo).get());
    if (!escaped)
        return NS_ERROR_OUT_OF_MEMORY;
    pushBuffer.AppendLiteral("<tr>\n <td>");
    pushBuffer.Append(escaped);
    nsMemory::Free(escaped);
    pushBuffer.AppendLiteral("</td>\n <td></td>\n <td></td>\n <td></td>\n</tr>\n");
    
    return FormatInputStream(aRequest, aCtxt, pushBuffer);
}

void nsIndexedToHTML::FormatSizeString(PRInt64 inSize, nsString& outSizeString)
{
    nsInt64 size(inSize);
    outSizeString.Truncate();
    if (size > nsInt64(0)) {
        
        PRInt64  upperSize = (size + nsInt64(1023)) / nsInt64(1024);
        outSizeString.AppendInt(upperSize);
        outSizeString.AppendLiteral(" KB");
    }
}

nsIndexedToHTML::nsIndexedToHTML() {
}

nsIndexedToHTML::~nsIndexedToHTML() {
}
