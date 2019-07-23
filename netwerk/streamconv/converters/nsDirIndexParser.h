








































#ifndef __NSDIRINDEX_H_
#define __NSDIRINDEX_H_

#include "nsIDirIndex.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIDirIndexListener.h"
#include "nsITextToSubURI.h"



class nsDirIndexParser : public nsIDirIndexParser {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIDIRINDEXPARSER
    
    nsDirIndexParser();
    virtual ~nsDirIndexParser();
    nsresult Init();

    enum fieldType {
        FIELD_UNKNOWN = 0, 
        FIELD_FILENAME,
        FIELD_DESCRIPTION,
        FIELD_CONTENTLENGTH,
        FIELD_LASTMODIFIED,
        FIELD_CONTENTTYPE,
        FIELD_FILETYPE
    };

protected:
    nsCOMPtr<nsIDirIndexListener> mListener;

    nsCString    mEncoding;
    nsCString    mComment;
    nsCString    mBuf;
    PRInt32      mLineStart;
    PRBool       mHasDescription;
    int*         mFormat;

    nsresult ProcessData(nsIRequest *aRequest, nsISupports *aCtxt);
    nsresult ParseFormat(const char* buf);
    nsresult ParseData(nsIDirIndex* aIdx, char* aDataStr);

    struct Field {
        const char *mName;
        fieldType mType;
    };

    static Field gFieldTable[];

    static nsrefcnt gRefCntParser;
    static nsITextToSubURI* gTextToSubURI;
};

#endif
