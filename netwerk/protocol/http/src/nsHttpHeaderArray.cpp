






































#include "nsHttpHeaderArray.h"
#include "nsHttp.h"





nsresult
nsHttpHeaderArray::SetHeader(nsHttpAtom header,
                             const nsACString &value,
                             PRBool merge)
{
    nsEntry *entry = nsnull;
    PRInt32 index;

    index = LookupEntry(header, &entry);

    
    
    if (value.IsEmpty()) {
        if (!merge && entry)
            mHeaders.RemoveElementAt(index);
        return NS_OK;
    }

    
    if (!entry) {
        entry = mHeaders.AppendElement(); 
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        entry->header = header;
        entry->value = value;
    }
    
    else if (merge && CanAppendToHeader(header)) {
        if (header == nsHttp::Set_Cookie ||
            header == nsHttp::WWW_Authenticate ||
            header == nsHttp::Proxy_Authenticate)
            
            
            
            entry->value.Append('\n');
        else
            
            entry->value.AppendLiteral(", ");
        entry->value.Append(value);
    }
    
    else
        entry->value = value;
    return NS_OK;
}

void
nsHttpHeaderArray::ClearHeader(nsHttpAtom header)
{
    mHeaders.RemoveElement(header, nsEntry::MatchHeader());
}

const char *
nsHttpHeaderArray::PeekHeader(nsHttpAtom header)
{
    nsEntry *entry = nsnull;
    LookupEntry(header, &entry);
    return entry ? entry->value.get() : nsnull;
}

nsresult
nsHttpHeaderArray::GetHeader(nsHttpAtom header, nsACString &result)
{
    nsEntry *entry = nsnull;
    LookupEntry(header, &entry);
    if (!entry)
        return NS_ERROR_NOT_AVAILABLE;
    result = entry->value;
    return NS_OK;
}

nsresult
nsHttpHeaderArray::VisitHeaders(nsIHttpHeaderVisitor *visitor)
{
    NS_ENSURE_ARG_POINTER(visitor);
    PRUint32 i, count = mHeaders.Length();
    for (i = 0; i < count; ++i) {
        const nsEntry &entry = mHeaders[i];
        if (NS_FAILED(visitor->VisitHeader(nsDependentCString(entry.header),
                                           entry.value)))
            break;
    }
    return NS_OK;
}

void
nsHttpHeaderArray::ParseHeaderLine(char *line, nsHttpAtom *hdr, char **val)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    char *p = (char *) strchr(line, ':');
    if (!p) {
        LOG(("malformed header [%s]: no colon\n", line));
        return;
    }

    
    if (!nsHttp::IsValidToken(line, p)) {
        LOG(("malformed header [%s]: field-name not a token\n", line));
        return;
    }
    
    *p = 0; 

    nsHttpAtom atom = nsHttp::ResolveAtom(line);
    if (!atom) {
        LOG(("failed to resolve atom [%s]\n", line));
        return;
    }

    
    p = net_FindCharNotInSet(++p, HTTP_LWS);

    
    char *p2 = net_RFindCharNotInSet(p, HTTP_LWS);

    *++p2 = 0; 
               
               

    
    if (hdr) *hdr = atom;
    if (val) *val = p;

    
    SetHeader(atom, nsDependentCString(p, p2 - p), PR_TRUE);
}

void
nsHttpHeaderArray::Flatten(nsACString &buf, PRBool pruneProxyHeaders)
{
    PRUint32 i, count = mHeaders.Length();
    for (i = 0; i < count; ++i) {
        const nsEntry &entry = mHeaders[i];
        
        if (pruneProxyHeaders && ((entry.header == nsHttp::Proxy_Authorization) || 
                                  (entry.header == nsHttp::Proxy_Connection)))
            continue;
        buf.Append(entry.header);
        buf.AppendLiteral(": ");
        buf.Append(entry.value);
        buf.AppendLiteral("\r\n");
    }
}

const char *
nsHttpHeaderArray::PeekHeaderAt(PRUint32 index, nsHttpAtom &header)
{
    const nsEntry &entry = mHeaders[index];

    header = entry.header;
    return entry.value.get();
}

void
nsHttpHeaderArray::Clear()
{
    mHeaders.Clear();
}





PRInt32
nsHttpHeaderArray::LookupEntry(nsHttpAtom header, nsEntry **entry)
{
    PRUint32 index = mHeaders.IndexOf(header, 0, nsEntry::MatchHeader());
    if (index != PR_UINT32_MAX)
      *entry = &mHeaders[index];
    return index;
}

PRBool
nsHttpHeaderArray::CanAppendToHeader(nsHttpAtom header)
{
    return header != nsHttp::Content_Type        &&
           header != nsHttp::Content_Length      &&
           header != nsHttp::User_Agent          &&
           header != nsHttp::Referer             &&
           header != nsHttp::Host                &&
           header != nsHttp::Authorization       &&
           header != nsHttp::Proxy_Authorization &&
           header != nsHttp::If_Modified_Since   &&
           header != nsHttp::If_Unmodified_Since &&
           header != nsHttp::From                &&
           header != nsHttp::Location            &&
           header != nsHttp::Max_Forwards;
}
