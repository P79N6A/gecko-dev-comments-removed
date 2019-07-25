





































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
    enum nsHttpHeaderType {
        HTTP_REQUEST_HEADERS,
        HTTP_RESPONSE_HEADERS
    };

    nsHttpHeaderArray(nsHttpHeaderType headerType) : mType(headerType) {}
   ~nsHttpHeaderArray() { Clear(); }

    const char *PeekHeader(nsHttpAtom header);

    nsresult SetHeader(nsHttpAtom header, const nsACString &value, PRBool merge = PR_FALSE);
    nsresult GetHeader(nsHttpAtom header, nsACString &value);
    void     ClearHeader(nsHttpAtom h);

    
    const char *FindHeaderValue(nsHttpAtom header, const char *value) {
        return nsHttp::FindToken(PeekHeader(header), value,
                                 HTTP_HEADER_VALUE_SEPS);
    }

    
    PRBool HasHeaderValue(nsHttpAtom header, const char *value) {
        return FindHeaderValue(header, value) != nsnull;
    }

    nsresult VisitHeaders(nsIHttpHeaderVisitor *visitor);

    
    
    nsresult ParseHeaderLine(const char *line,
                             nsHttpAtom *header=nsnull,
                             char **value=nsnull);

    void Flatten(nsACString &, PRBool pruneProxyHeaders=PR_FALSE);

    PRUint32 Count() { return mHeaders.Length(); }

    const char *PeekHeaderAt(PRUint32 i, nsHttpAtom &header);

    void Clear();

    struct nsEntry
    {
        nsEntry() {}

        nsHttpAtom header;
        nsCString  value;

        struct MatchHeader {
          PRBool Equals(const nsEntry &entry, const nsHttpAtom &header) const {
            return entry.header == header;
          }
        };
    };

    nsTArray<nsEntry> &Headers() { return mHeaders; }

private:
    PRInt32 LookupEntry(nsHttpAtom header, nsEntry **);
    PRBool  CanAppendToHeader(nsHttpAtom header);
    PRBool  CanOverwriteHeader(nsHttpAtom header);

    nsTArray<nsEntry> mHeaders;
    nsHttpHeaderType  mType;
};

#endif
