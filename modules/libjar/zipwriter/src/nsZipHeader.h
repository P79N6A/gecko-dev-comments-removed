





































#ifndef _nsZipHeader_h_
#define _nsZipHeader_h_

#include "nsString.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsIZipReader.h"
#include "nsAutoPtr.h"

#define ZIP_ATTRS_FILE 0
#define ZIP_ATTRS_DIRECTORY 16

class nsZipHeader : public nsIZipEntry
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIZIPENTRY

    nsZipHeader() :
        mCRC(0),
        mCSize(0),
        mUSize(0),
        mEAttr(0),
        mOffset(0),
        mFieldLength(0),
        mVersionMade(20),
        mVersionNeeded(20),
        mFlags(0),
        mMethod(0),
        mTime(0),
        mDate(0),
        mDisk(0),
        mIAttr(0),
        mInited(PR_FALSE),
        mExtraField(NULL)
    {
    }

    ~nsZipHeader()
    {
        mExtraField = NULL;
    }

    PRUint32 mCRC;
    PRUint32 mCSize;
    PRUint32 mUSize;
    PRUint32 mEAttr;
    PRUint32 mOffset;
    PRUint32 mFieldLength;
    PRUint16 mVersionMade;
    PRUint16 mVersionNeeded;
    PRUint16 mFlags;
    PRUint16 mMethod;
    PRUint16 mTime;
    PRUint16 mDate;
    PRUint16 mDisk;
    PRUint16 mIAttr;
    PRPackedBool mInited;
    nsCString mName;
    nsCString mComment;
    nsAutoArrayPtr<char> mExtraField;

    void Init(const nsACString & aPath, PRTime aDate, PRUint32 aAttr,
              PRUint32 aOffset);
    PRUint32 GetFileHeaderLength();
    nsresult WriteFileHeader(nsIOutputStream *aStream);
    PRUint32 GetCDSHeaderLength();
    nsresult WriteCDSHeader(nsIOutputStream *aStream);
    nsresult ReadCDSHeader(nsIInputStream *aStream);
};

#endif
