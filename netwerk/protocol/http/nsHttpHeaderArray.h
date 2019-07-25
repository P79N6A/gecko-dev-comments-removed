






































#ifndef nsHttpHeaderArray_h__
#define nsHttpHeaderArray_h__

#include "nsHttp.h"
#include "nsTArray.h"
#include "nsIHttpChannel.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsHttpHeaderArray
{
public:
    nsHttpHeaderArray() {}
   ~nsHttpHeaderArray() { Clear(); }

    const char *PeekHeader(nsHttpAtom header);

    
    nsresult SetHeader(nsHttpAtom header, const nsACString &value,
                       bool merge = false);

    
    
    nsresult SetHeaderFromNet(nsHttpAtom header, const nsACString &value);

    nsresult GetHeader(nsHttpAtom header, nsACString &value);
    void     ClearHeader(nsHttpAtom h);

    
    const char *FindHeaderValue(nsHttpAtom header, const char *value) {
        return nsHttp::FindToken(PeekHeader(header), value,
                                 HTTP_HEADER_VALUE_SEPS);
    }

    
    bool HasHeaderValue(nsHttpAtom header, const char *value) {
        return FindHeaderValue(header, value) != nsnull;
    }

    nsresult VisitHeaders(nsIHttpHeaderVisitor *visitor);

    
    
    nsresult ParseHeaderLine(const char *line,
                             nsHttpAtom *header=nsnull,
                             char **value=nsnull);

    void Flatten(nsACString &, bool pruneProxyHeaders=false);

    PRUint32 Count() { return mHeaders.Length(); }

    const char *PeekHeaderAt(PRUint32 i, nsHttpAtom &header);

    void Clear();

    struct nsEntry
    {
        nsEntry() {}

        nsHttpAtom header;
        nsCString  value;

        struct MatchHeader {
          bool Equals(const nsEntry &entry, const nsHttpAtom &header) const {
            return entry.header == header;
          }
        };
    };

private:
    PRInt32 LookupEntry(nsHttpAtom header, nsEntry **);
    void MergeHeader(nsHttpAtom header, nsEntry *entry, const nsACString &value);

    
    bool    IsSingletonHeader(nsHttpAtom header);

    
    
    
    bool    IsSuspectDuplicateHeader(nsHttpAtom header);

    nsTArray<nsEntry> mHeaders;

    friend struct IPC::ParamTraits<nsHttpHeaderArray>;
};






inline PRInt32
nsHttpHeaderArray::LookupEntry(nsHttpAtom header, nsEntry **entry)
{
    PRUint32 index = mHeaders.IndexOf(header, 0, nsEntry::MatchHeader());
    if (index != PR_UINT32_MAX)
        *entry = &mHeaders[index];
    return index;
}

inline bool
nsHttpHeaderArray::IsSingletonHeader(nsHttpAtom header)
{
    return header == nsHttp::Content_Type        ||
           header == nsHttp::Content_Disposition ||
           header == nsHttp::Content_Length      ||
           header == nsHttp::User_Agent          ||
           header == nsHttp::Referer             ||
           header == nsHttp::Host                ||
           header == nsHttp::Authorization       ||
           header == nsHttp::Proxy_Authorization ||
           header == nsHttp::If_Modified_Since   ||
           header == nsHttp::If_Unmodified_Since ||
           header == nsHttp::From                ||
           header == nsHttp::Location            ||
           header == nsHttp::Max_Forwards;
}

inline void
nsHttpHeaderArray::MergeHeader(nsHttpAtom header,
                               nsEntry *entry,
                               const nsACString &value)
{
    if (value.IsEmpty())
        return;   

    
    if (header == nsHttp::Set_Cookie ||
        header == nsHttp::WWW_Authenticate ||
        header == nsHttp::Proxy_Authenticate)
    {
        
        
        
        entry->value.Append('\n');
    } else {
        
        entry->value.AppendLiteral(", ");
    }
    entry->value.Append(value);
}

inline bool
nsHttpHeaderArray::IsSuspectDuplicateHeader(nsHttpAtom header)
{
    bool retval =  header == nsHttp::Content_Length         ||
                     header == nsHttp::Content_Disposition    ||
                     header == nsHttp::Location;

    NS_ASSERTION(!retval || IsSingletonHeader(header),
                 "Only non-mergeable headers should be in this list\n");

    return retval;
}

#endif
