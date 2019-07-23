






































#include "txURIUtils.h"

#ifndef TX_EXE
#include "nsNetUtil.h"
#include "nsIAttribute.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"
#include "nsINodeInfo.h"
#endif






#ifdef TX_EXE


const char   URIUtils::HREF_PATH_SEP  = '/';





void
txParsedURL::init(const nsAFlatString& aSpec)
{
    mPath.Truncate();
    mName.Truncate();
    mRef.Truncate();
    PRUint32 specLength = aSpec.Length();
    if (!specLength) {
        return;
    }
    const PRUnichar* start = aSpec.get();
    const PRUnichar* end = start + specLength;
    const PRUnichar* c = end - 1;

    
    for (; c >= start; --c) {
        if (*c == '#') {
            
            mRef = Substring(c + 1, end);
            end = c;
            --c;
            if (c == start) {
                
                return;
            }
            break;
        }
    }
    for (c = end - 1; c >= start; --c) {
        if (*c == '/') {
            mName = Substring(c + 1, end);
            mPath = Substring(start, c + 1);
            return;
        }
    }
    mName = Substring(start, end);
}

void
txParsedURL::resolve(const txParsedURL& aRef, txParsedURL& aDest)
{
    



    aDest.mPath = mPath + aRef.mPath;

    if (aRef.mName.IsEmpty() && aRef.mPath.IsEmpty()) {
        
        aDest.mName = mName;
        if (aRef.mRef.IsEmpty()) {
            
            aDest.mRef = mRef;
            return;
        }
        aDest.mRef = aRef.mRef;
        return;
    }
    aDest.mName = aRef.mName;
    aDest.mRef = aRef.mRef;
}









istream* URIUtils::getInputStream(const nsAString& href, nsAString& errMsg)
{
    return new ifstream(NS_LossyConvertUTF16toASCII(href).get(), ios::in);
} 





void URIUtils::getDocumentBase(const nsAFlatString& href, nsAString& dest)
{
    if (href.IsEmpty()) {
        return;
    }

    nsAFlatString::const_char_iterator temp;
    href.BeginReading(temp);
    PRUint32 iter = href.Length();
    while (iter > 0) {
        if (temp[--iter] == HREF_PATH_SEP) {
            dest.Append(StringHead(href, iter));
            break;
        }
    }
}
#endif






void URIUtils::resolveHref(const nsAString& href, const nsAString& base,
                           nsAString& dest) {
    if (base.IsEmpty()) {
        dest.Append(href);
        return;
    }
    if (href.IsEmpty()) {
        dest.Append(base);
        return;
    }

#ifndef TX_EXE
    nsCOMPtr<nsIURI> pURL;
    nsAutoString resultHref;
    nsresult result = NS_NewURI(getter_AddRefs(pURL), base);
    if (NS_SUCCEEDED(result)) {
        NS_MakeAbsoluteURI(resultHref, href, pURL);
        dest.Append(resultHref);
    }
#else
    nsAutoString documentBase;
    getDocumentBase(PromiseFlatString(base), documentBase);

    
    if (!documentBase.IsEmpty()) {
        dest.Append(documentBase);
        if (documentBase.CharAt(documentBase.Length()-1) != HREF_PATH_SEP)
            dest.Append(PRUnichar(HREF_PATH_SEP));
    }
    dest.Append(href);

#endif
} 

#ifndef TX_EXE


void
URIUtils::ResetWithSource(nsIDocument *aNewDoc, nsIDOMNode *aSourceNode)
{
    nsCOMPtr<nsINode> node = do_QueryInterface(aSourceNode);
    if (!node) {
        
        aNewDoc->Reset(nsnull, nsnull);
        return;
    }

    nsCOMPtr<nsIDocument> sourceDoc = node->GetOwnerDoc();
    if (!sourceDoc) {
        NS_ERROR("no source document found");
        
        aNewDoc->Reset(nsnull, nsnull);
        return;
    }

    nsIPrincipal* sourcePrincipal = sourceDoc->NodePrincipal();

    
    nsCOMPtr<nsILoadGroup> loadGroup = sourceDoc->GetDocumentLoadGroup();
    nsCOMPtr<nsIChannel> channel = sourceDoc->GetChannel();
    if (!channel) {
        
        if (NS_FAILED(NS_NewChannel(getter_AddRefs(channel),
                                    sourceDoc->GetDocumentURI(),
                                    nsnull,
                                    loadGroup))) {
            return;
        }
        channel->SetOwner(sourcePrincipal);
    }
    aNewDoc->Reset(channel, loadGroup);
    aNewDoc->SetPrincipal(sourcePrincipal);
    aNewDoc->SetBaseURI(sourceDoc->GetBaseURI());

    
    aNewDoc->SetDocumentCharacterSetSource(
          sourceDoc->GetDocumentCharacterSetSource());
    aNewDoc->SetDocumentCharacterSet(sourceDoc->GetDocumentCharacterSet());
}

#endif 
