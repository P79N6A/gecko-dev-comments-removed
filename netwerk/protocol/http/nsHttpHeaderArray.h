





#ifndef nsHttpHeaderArray_h__
#define nsHttpHeaderArray_h__

#include "nsHttp.h"
#include "nsTArray.h"
#include "nsString.h"

class nsIHttpHeaderVisitor;



namespace IPC {
    template <typename> struct ParamTraits;
}

namespace mozilla { namespace net {

class nsHttpHeaderArray
{
public:
    const char *PeekHeader(nsHttpAtom header) const;

    
    nsresult SetHeader(nsHttpAtom header, const nsACString &value,
                       bool merge = false);

    
    
    nsresult SetHeaderFromNet(nsHttpAtom header, const nsACString &value);

    nsresult GetHeader(nsHttpAtom header, nsACString &value) const;
    void     ClearHeader(nsHttpAtom h);

    
    const char *FindHeaderValue(nsHttpAtom header, const char *value) const
    {
        return nsHttp::FindToken(PeekHeader(header), value,
                                 HTTP_HEADER_VALUE_SEPS);
    }

    
    bool HasHeaderValue(nsHttpAtom header, const char *value) const
    {
        return FindHeaderValue(header, value) != nullptr;
    }

    nsresult VisitHeaders(nsIHttpHeaderVisitor *visitor);

    
    
    nsresult ParseHeaderLine(const char *line,
                             nsHttpAtom *header=nullptr,
                             char **value=nullptr);

    void Flatten(nsACString &, bool pruneProxyHeaders=false);

    void ParseHeaderSet(char *buffer);

    uint32_t Count() const { return mHeaders.Length(); }

    const char *PeekHeaderAt(uint32_t i, nsHttpAtom &header) const;

    void Clear();

    
    struct nsEntry
    {
        nsHttpAtom header;
        nsCString value;

        struct MatchHeader {
          bool Equals(const nsEntry &entry, const nsHttpAtom &header) const {
            return entry.header == header;
          }
        };

        bool operator==(const nsEntry& aOther) const
        {
            return header == aOther.header && value == aOther.value;
        }
    };

    bool operator==(const nsHttpHeaderArray& aOther) const
    {
        return mHeaders == aOther.mHeaders;
    }

private:
    int32_t LookupEntry(nsHttpAtom header, const nsEntry **) const;
    int32_t LookupEntry(nsHttpAtom header, nsEntry **);
    void MergeHeader(nsHttpAtom header, nsEntry *entry, const nsACString &value);

    
    bool    IsSingletonHeader(nsHttpAtom header);
    
    
    bool    TrackEmptyHeader(nsHttpAtom header);

    
    
    
    bool    IsSuspectDuplicateHeader(nsHttpAtom header);

    
    nsTArray<nsEntry> mHeaders;

    friend struct IPC::ParamTraits<nsHttpHeaderArray>;
};






inline int32_t
nsHttpHeaderArray::LookupEntry(nsHttpAtom header, const nsEntry **entry) const
{
    uint32_t index = mHeaders.IndexOf(header, 0, nsEntry::MatchHeader());
    if (index != UINT32_MAX)
        *entry = &mHeaders[index];
    return index;
}

inline int32_t
nsHttpHeaderArray::LookupEntry(nsHttpAtom header, nsEntry **entry)
{
    uint32_t index = mHeaders.IndexOf(header, 0, nsEntry::MatchHeader());
    if (index != UINT32_MAX)
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

inline bool
nsHttpHeaderArray::TrackEmptyHeader(nsHttpAtom header)
{
    return header == nsHttp::Content_Length ||
           header == nsHttp::Location;
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

    MOZ_ASSERT(!retval || IsSingletonHeader(header),
               "Only non-mergeable headers should be in this list\n");

    return retval;
}

}} 

#endif
