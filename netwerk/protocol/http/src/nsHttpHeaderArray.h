





































#ifndef nsHttpHeaderArray_h__
#define nsHttpHeaderArray_h__

#include "nsTArray.h"
#include "nsIHttpChannel.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsHttp.h"

class nsHttpHeaderArray
{
public:
    nsHttpHeaderArray() {}
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

    
    
    void ParseHeaderLine(char *line, nsHttpAtom *header=nsnull, char **value=nsnull);

    void Flatten(nsACString &, PRBool pruneProxyHeaders=PR_FALSE);

    PRUint32 Count() { return mHeaders.Length(); }

    const char *PeekHeaderAt(PRUint32 i, nsHttpAtom &header);

    void Clear();

private:
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

    PRInt32 LookupEntry(nsHttpAtom header, nsEntry **);
    PRBool  CanAppendToHeader(nsHttpAtom header);

private:
    nsTArray<nsEntry> mHeaders;
};

#endif
