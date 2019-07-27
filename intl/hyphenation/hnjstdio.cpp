









#include "hnjalloc.h"
#undef FILE // Undo the damage done in hnjalloc.h
#include "nsNetUtil.h"
#include "nsContentUtils.h"

#define BUFSIZE 1024

struct hnjFile_ {
    nsCOMPtr<nsIInputStream> mStream;
    char                     mBuffer[BUFSIZE];
    uint32_t                 mCurPos;
    uint32_t                 mLimit;
};



hnjFile*
hnjFopen(const char* aURISpec, const char* aMode)
{
    
    NS_ASSERTION(!strcmp(aMode, "r"), "unsupported fopen() mode in hnjFopen");

    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), aURISpec);
    if (NS_FAILED(rv)) {
        return nullptr;
    }

    nsCOMPtr<nsIInputStream> instream;
    rv = NS_OpenURI(getter_AddRefs(instream),
                    uri,
                    nsContentUtils::GetSystemPrincipal(),
                    nsILoadInfo::SEC_NORMAL,
                    nsIContentPolicy::TYPE_OTHER);

    if (NS_FAILED(rv)) {
        return nullptr;
    }

    hnjFile *f = new hnjFile;
    f->mStream = instream;
    f->mCurPos = 0;
    f->mLimit = 0;

    return f;
}


int
hnjFclose(hnjFile* f)
{
    NS_ASSERTION(f && f->mStream, "bad argument to hnjFclose");

    int result = 0;
    nsresult rv = f->mStream->Close();
    if (NS_FAILED(rv)) {
        result = EOF;
    }
    f->mStream = nullptr;

    delete f;
    return result;
}



char*
hnjFgets(char* s, int n, hnjFile* f)
{
    NS_ASSERTION(s && f, "bad argument to hnjFgets");

    int i = 0;
    while (i < n - 1) {
        if (f->mCurPos < f->mLimit) {
            char c = f->mBuffer[f->mCurPos++];
            s[i++] = c;
            if (c == '\n' || c == '\r') {
                break;
            }
            continue;
        }

        f->mCurPos = 0;

        nsresult rv = f->mStream->Read(f->mBuffer, BUFSIZE, &f->mLimit);
        if (NS_FAILED(rv)) {
            f->mLimit = 0;
            return nullptr;
        }

        if (f->mLimit == 0) {
            break;
        }
    }

    if (i == 0) {
        return nullptr; 
    }

    s[i] = '\0'; 
    return s;
}
