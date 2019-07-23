




































#include "nsTXTToHTMLConv.h"
#include "nsNetUtil.h"
#include "nsStringStream.h"
#include "nsAutoPtr.h"

#define TOKEN_DELIMITERS NS_LITERAL_STRING("\t\r\n ").get()


NS_IMPL_THREADSAFE_ISUPPORTS4(nsTXTToHTMLConv,
                              nsIStreamConverter,
                              nsITXTToHTMLConv,
                              nsIRequestObserver,
                              nsIStreamListener)



NS_IMETHODIMP
nsTXTToHTMLConv::Convert(nsIInputStream *aFromStream,
                         const char *aFromType, const char *aToType,
                         nsISupports *aCtxt, nsIInputStream * *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTXTToHTMLConv::AsyncConvertData(const char *aFromType,
                                  const char *aToType,
                                  nsIStreamListener *aListener,
                                  nsISupports *aCtxt)
{
    NS_ASSERTION(aListener, "null pointer");
    mListener = aListener;
    return NS_OK;
}



NS_IMETHODIMP
nsTXTToHTMLConv::OnStartRequest(nsIRequest* request, nsISupports *aContext)
{
    mBuffer.AssignLiteral("<html>\n<head><title>");
    mBuffer.Append(mPageTitle);
    mBuffer.AppendLiteral("</title></head>\n<body>\n");
    if (mPreFormatHTML) {     
        mBuffer.AppendLiteral("<pre>\n");
    }

    
    

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    if (channel)
        channel->SetContentType(NS_LITERAL_CSTRING("text/html"));
    

    nsresult rv = mListener->OnStartRequest(request, aContext);
    if (NS_FAILED(rv)) return rv;

    
    
    request->GetStatus(&rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIInputStream> inputData;
    rv = NS_NewStringInputStream(getter_AddRefs(inputData), mBuffer);
    if (NS_FAILED(rv)) return rv;

    rv = mListener->OnDataAvailable(request, aContext,
                                    inputData, 0, mBuffer.Length());
    if (NS_FAILED(rv)) return rv;
    mBuffer.Truncate();
    return rv;
}

NS_IMETHODIMP
nsTXTToHTMLConv::OnStopRequest(nsIRequest* request, nsISupports *aContext,
                               nsresult aStatus)
{
    nsresult rv = NS_OK;
    if (mToken) {
        
        NS_ASSERTION(mToken->prepend,
                     "Non prepending tokens should be handled in "
                     "OnDataAvailable. There should only be a single "
                     "prepending token left to be processed.");
        (void)CatHTML(0, mBuffer.Length());
    }
    if (mPreFormatHTML) {
        mBuffer.AppendLiteral("</pre>\n");
    }
    mBuffer.AppendLiteral("\n</body></html>");

    nsCOMPtr<nsIInputStream> inputData;

    rv = NS_NewStringInputStream(getter_AddRefs(inputData), mBuffer);
    if (NS_FAILED(rv)) return rv;

    rv = mListener->OnDataAvailable(request, aContext,
                                    inputData, 0, mBuffer.Length());
    if (NS_FAILED(rv)) return rv;

    return mListener->OnStopRequest(request, aContext, aStatus);
}


NS_IMETHODIMP
nsTXTToHTMLConv::SetTitle(const PRUnichar *aTitle)
{
    mPageTitle.Assign(aTitle);
    return NS_OK;
}

NS_IMETHODIMP
nsTXTToHTMLConv::PreFormatHTML(PRBool value)
{
    mPreFormatHTML = value;
    return NS_OK;
}


NS_IMETHODIMP
nsTXTToHTMLConv::OnDataAvailable(nsIRequest* request, nsISupports *aContext,
                                 nsIInputStream *aInStream,
                                 PRUint32 aOffset, PRUint32 aCount)
{
    nsresult rv = NS_OK;
    nsString pushBuffer;
    PRUint32 amtRead = 0;
    nsAutoArrayPtr<char> buffer(new char[aCount+1]);
    if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

    do {
        PRUint32 read = 0;
        
        rv = aInStream->Read(buffer, aCount-amtRead, &read);
        if (NS_FAILED(rv)) return rv;

        buffer[read] = '\0';
        
        AppendASCIItoUTF16(buffer, mBuffer);
        amtRead += read;

        PRInt32 front = -1, back = -1, tokenLoc = -1, cursor = 0;

        while ( (tokenLoc = FindToken(cursor, &mToken)) > -1) {
            if (mToken->prepend) {
                front = mBuffer.RFindCharInSet(TOKEN_DELIMITERS, tokenLoc);
                front++;
                back = mBuffer.FindCharInSet(TOKEN_DELIMITERS, tokenLoc);
            } else {
                front = tokenLoc;
                back = front + mToken->token.Length();
            }
            if (back == -1) {
                
                mBuffer.Left(pushBuffer, front);
                cursor = front;
                break;
            }
            
            cursor = CatHTML(front, back);
        }

        PRInt32 end = mBuffer.RFind(TOKEN_DELIMITERS, mBuffer.Length());
        mBuffer.Left(pushBuffer, PR_MAX(cursor, end));
        mBuffer.Cut(0, PR_MAX(cursor, end));
        cursor = 0;

        if (!pushBuffer.IsEmpty()) {
            nsCOMPtr<nsIInputStream> inputData;

            rv = NS_NewStringInputStream(getter_AddRefs(inputData), pushBuffer);
            if (NS_FAILED(rv))
                return rv;

            rv = mListener->OnDataAvailable(request, aContext,
                                            inputData, 0, pushBuffer.Length());
            if (NS_FAILED(rv))
                return rv;
        }
    } while (amtRead < aCount);

    return rv;
}


nsTXTToHTMLConv::nsTXTToHTMLConv()
{
    mToken = nsnull;
    mPreFormatHTML = PR_FALSE;
}

static PRBool CleanupTokens(void *aElement, void *aData)
{
    if (aElement) delete (convToken*)aElement;
    return PR_TRUE;
}

nsTXTToHTMLConv::~nsTXTToHTMLConv()
{
    mTokens.EnumerateForwards((nsVoidArrayEnumFunc)CleanupTokens, nsnull);
}

nsresult
nsTXTToHTMLConv::Init()
{
    nsresult rv = NS_OK;

    
    convToken *token = new convToken;
    if (!token) return NS_ERROR_OUT_OF_MEMORY;
    token->prepend = PR_FALSE;
    token->token.Assign(PRUnichar('<'));
    token->modText.AssignLiteral("&lt;");
    mTokens.AppendElement(token);

    token = new convToken;
    if (!token) return NS_ERROR_OUT_OF_MEMORY;
    token->prepend = PR_FALSE;
    token->token.Assign(PRUnichar('>'));
    token->modText.AssignLiteral("&gt;");
    mTokens.AppendElement(token);

    token = new convToken;
    if (!token) return NS_ERROR_OUT_OF_MEMORY;
    token->prepend = PR_FALSE;
    token->token.Assign(PRUnichar('&'));
    token->modText.AssignLiteral("&amp;");
    mTokens.AppendElement(token);

    token = new convToken;
    if (!token) return NS_ERROR_OUT_OF_MEMORY;
    token->prepend = PR_TRUE;
    token->token.AssignLiteral("http://"); 
    mTokens.AppendElement(token);

    token = new convToken;
    if (!token) return NS_ERROR_OUT_OF_MEMORY;
    token->prepend = PR_TRUE;
    token->token.Assign(PRUnichar('@'));
    token->modText.AssignLiteral("mailto:");
    mTokens.AppendElement(token);

    return rv;
}

PRInt32
nsTXTToHTMLConv::FindToken(PRInt32 cursor, convToken* *_retval)
{
    PRInt32 loc = -1, firstToken = mBuffer.Length();
    PRInt8 token = -1;
    for (PRInt8 i=0; i < mTokens.Count(); i++) {
        loc = mBuffer.Find(((convToken*)mTokens[i])->token, cursor);
        if (loc != -1)
            if (loc < firstToken) {
                firstToken = loc;
                token = i;
            }
    }
    if (token == -1)
        return -1;

    *_retval = (convToken*)mTokens[token];
    return firstToken;
}

PRInt32
nsTXTToHTMLConv::CatHTML(PRInt32 front, PRInt32 back)
{
    PRInt32 cursor = 0;
    PRInt32 modLen = mToken->modText.Length();
    if (!mToken->prepend) {
        
        mBuffer.Cut(front, back - front);
        mBuffer.Insert(mToken->modText, front);
        cursor = front+modLen;
    } else {
        nsString linkText;
        
        mBuffer.Mid(linkText, front, back-front);
        mBuffer.Insert(NS_LITERAL_STRING("<a href=\""), front);
        cursor += front+9;
        if (modLen)
            mBuffer.Insert(mToken->modText, cursor);
        cursor += modLen-front+back;
        mBuffer.Insert(NS_LITERAL_STRING("\">"), cursor);
        cursor += 2;
        mBuffer.Insert(linkText, cursor);
        cursor += linkText.Length();
        mBuffer.Insert(NS_LITERAL_STRING("</a>"), cursor);
        cursor += 4;
    }
    mToken = nsnull; 
    return cursor;
}
