





































#ifndef nsFastLoadFile_h___
#define nsFastLoadFile_h___





#include "prtypes.h"
#include "pldhash.h"

#include "nsBinaryStream.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsID.h"
#include "nsMemory.h"

#include "nsIFastLoadFileControl.h"
#include "nsIFastLoadService.h"
#include "nsISeekableStream.h"
#include "nsISupportsArray.h"




















typedef PRUint32 NSFastLoadID;          
typedef PRUint32 NSFastLoadOID;         













#define MFL_ID_XOR_KEY  0x9E3779B9      // key XOR'd with ID when serialized
#define MFL_OID_XOR_KEY 0x6A09E667      // key XOR'd with OID when serialized














#define MFL_OBJECT_TAG_BITS     3
#define MFL_OBJECT_TAG_MASK     PR_BITMASK(MFL_OBJECT_TAG_BITS)

#define MFL_OBJECT_DEF_TAG      1U      // object definition follows this OID
#define MFL_WEAK_REF_TAG        2U      // OID weakly refers to a prior object
                                        
#define MFL_QUERY_INTERFACE_TAG 4U      // QI object to the ID follows this OID
                                        






#define MFL_DULL_OBJECT_OID     MFL_OBJECT_DEF_TAG




#define MFL_OID_TO_SHARP_INDEX(oid)     (((oid) >> MFL_OBJECT_TAG_BITS) - 1)
#define MFL_SHARP_INDEX_TO_OID(index)   (((index) + 1) << MFL_OBJECT_TAG_BITS)







#define MFL_FILE_MAGIC          "XPCOM\nMozFASL\r\n\032"
#define MFL_FILE_MAGIC_SIZE     16

#define MFL_FILE_VERSION_0      0
#define MFL_FILE_VERSION_1      1000
#define MFL_FILE_VERSION        5       // rev'ed to defend against unversioned
                                        




































NS_COM PRUint32
NS_AccumulateFastLoadChecksum(PRUint32 *aChecksum,
                              const PRUint8* aBuffer,
                              PRUint32 aLength,
                              PRBool aLastBuffer);

NS_COM PRUint32
NS_AddFastLoadChecksums(PRUint32 sum1, PRUint32 sum2, PRUint32 sum2ByteCount);




struct nsFastLoadHeader {
    char        mMagic[MFL_FILE_MAGIC_SIZE];
    PRUint32    mChecksum;
    PRUint32    mVersion;
    PRUint32    mFooterOffset;
    PRUint32    mFileSize;
};





struct nsFastLoadFooterPrefix {
    PRUint32    mNumIDs;
    PRUint32    mNumSharpObjects;
    PRUint32    mNumMuxedDocuments;
    PRUint32    mNumDependencies;
};

struct nsFastLoadSharpObjectInfo {
    PRUint32    mCIDOffset;     
    PRUint16    mStrongRefCnt;
    PRUint16    mWeakRefCnt;    
};

#define MFL_SINGLETON_FLAG          0x8000
#define MFL_WEAK_REFCNT_MASK        0x7fff

#define MFL_GET_SINGLETON_FLAG(ip)  ((ip)->mWeakRefCnt & MFL_SINGLETON_FLAG)
#define MFL_GET_WEAK_REFCNT(ip)     ((ip)->mWeakRefCnt & MFL_WEAK_REFCNT_MASK)

#define MFL_SET_SINGLETON_FLAG(ip)                                            \
    ((ip)->mWeakRefCnt |= MFL_SINGLETON_FLAG)
#define MFL_SET_WEAK_REFCNT(ip,rc)                                            \
    ((ip)->mWeakRefCnt = (((ip)->mWeakRefCnt & MFL_SINGLETON_FLAG) | (rc)))

#define MFL_BUMP_WEAK_REFCNT(ip)    (++(ip)->mWeakRefCnt)
#define MFL_DROP_WEAK_REFCNT(ip)    (--(ip)->mWeakRefCnt)

struct nsFastLoadMuxedDocumentInfo {
    const char* mURISpec;
    PRUint32    mInitialSegmentOffset;
};


struct nsDocumentMapReadEntry;
struct nsDocumentMapWriteEntry;



#define NS_FASTLOADFILEREADER_IID \
    {0x7d37d1bb,0xcef3,0x4c5f,{0x97,0x68,0x0f,0x89,0x7f,0x1a,0xe1,0x40}}

struct nsIFastLoadFileReader : public nsISupports {
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_FASTLOADFILEREADER_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFastLoadFileReader, NS_FASTLOADFILEREADER_IID)







class nsFastLoadFileReader
    : public nsBinaryInputStream,
      public nsIFastLoadReadControl,
      public nsISeekableStream,
      public nsIFastLoadFileReader
{
  public:
    nsFastLoadFileReader(nsIFile *aFile)
        : mCurrentDocumentMapEntry(nsnull), mFile(aFile), mFd(nsnull),
          mFileLen(0), mFilePos(0), mFileMap(nsnull), mFileData(nsnull)
    {
        MOZ_COUNT_CTOR(nsFastLoadFileReader);
    }

    virtual ~nsFastLoadFileReader() {
        MOZ_COUNT_DTOR(nsFastLoadFileReader);
    }

  private:
    
    NS_DECL_ISUPPORTS_INHERITED

    
    NS_IMETHOD ReadObject(PRBool aIsStrongRef, nsISupports* *_retval);
    NS_IMETHOD ReadID(nsID *aResult);

    void SeekTo(PRInt64 aOffset) {
        mFilePos = PR_MAX(0, PR_MIN(aOffset, mFileLen));
        NS_ASSERTION(aOffset == mFilePos, "Attempt to seek out of bounds");
    }

    
    NS_DECL_NSIFASTLOADFILECONTROL

    
    NS_DECL_NSIFASTLOADREADCONTROL

    
    NS_DECL_NSISEEKABLESTREAM

    
    NS_IMETHOD Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead);
    nsresult ReadHeader(nsFastLoadHeader *aHeader);

    


    struct nsObjectMapEntry : public nsFastLoadSharpObjectInfo {
        nsCOMPtr<nsISupports>   mReadObject;
        PRInt64                 mSkipOffset;
        PRUint16                mSaveStrongRefCnt;      
        PRUint16                mSaveWeakRefCnt;        
    };

    NS_IMETHODIMP ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                               PRUint32 aCount, PRUint32 *aResult);

    


    struct nsFastLoadFooter : public nsFastLoadFooterPrefix {
        nsFastLoadFooter()
          : mIDMap(nsnull),
            mObjectMap(nsnull) {
            mDocumentMap.ops = mURIMap.ops = nsnull;
        }

        ~nsFastLoadFooter() {
            delete[] mIDMap;
            delete[] mObjectMap;
            if (mDocumentMap.ops)
                PL_DHashTableFinish(&mDocumentMap);
            if (mURIMap.ops)
                PL_DHashTableFinish(&mURIMap);
        }

        
        
        
        
        static nsID gDummyID;
        static nsObjectMapEntry gDummySharpObjectEntry;

        const nsID& GetID(NSFastLoadID aFastId) const {
            PRUint32 index = aFastId - 1;
            NS_ASSERTION(index < mNumIDs, "aFastId out of range");
            if (index >= mNumIDs)
                return gDummyID;
            return mIDMap[index];
        }

        nsObjectMapEntry&
        GetSharpObjectEntry(NSFastLoadOID aOID) const {
            PRUint32 index = MFL_OID_TO_SHARP_INDEX(aOID);
            NS_ASSERTION(index < mNumSharpObjects, "aOID out of range");
            if (index >= mNumSharpObjects)
                return gDummySharpObjectEntry;
            return mObjectMap[index];
        }

        
        nsID* mIDMap;

        
        
        nsObjectMapEntry* mObjectMap;

        
        
        
        PLDHashTable mDocumentMap;

        
        
        PLDHashTable mURIMap;

        
        
        nsCOMPtr<nsISupportsArray> mDependencies;
    };

    nsresult ReadFooter(nsFastLoadFooter *aFooter);
    nsresult ReadFooterPrefix(nsFastLoadFooterPrefix *aFooterPrefix);
    nsresult ReadSlowID(nsID *aID);
    nsresult ReadFastID(NSFastLoadID *aID);
    nsresult ReadSharpObjectInfo(nsFastLoadSharpObjectInfo *aInfo);
    nsresult ReadMuxedDocumentInfo(nsFastLoadMuxedDocumentInfo *aInfo);
    nsresult DeserializeObject(nsISupports* *aObject);

    nsresult   Open();
    NS_IMETHOD Close();

  protected:
    nsFastLoadHeader mHeader;
    nsFastLoadFooter mFooter;

    nsDocumentMapReadEntry* mCurrentDocumentMapEntry;

    friend class nsFastLoadFileUpdater;
    nsIFile *mFile;     
    PRFileDesc *mFd;    
    PRUint32 mFileLen;  
    PRUint32 mFilePos;  
    PRFileMap *mFileMap;
    PRUint8 *mFileData; 
};

NS_COM nsresult
NS_NewFastLoadFileReader(nsIObjectInputStream* *aResult NS_OUTPARAM,
                         nsIFile* aFile);







class nsFastLoadFileWriter
    : public nsBinaryOutputStream,
      public nsIFastLoadWriteControl,
      public nsISeekableStream
{
  public:
    nsFastLoadFileWriter(nsIOutputStream *aStream, nsIFastLoadFileIO* aFileIO)
      : mCurrentDocumentMapEntry(nsnull),
        mFileIO(aFileIO)
    {
        SetOutputStream(aStream);
        mHeader.mChecksum = 0;
        mIDMap.ops = mObjectMap.ops = mDocumentMap.ops = mURIMap.ops = nsnull;
        mDependencyMap.ops = nsnull;
        MOZ_COUNT_CTOR(nsFastLoadFileWriter);
    }

    virtual ~nsFastLoadFileWriter()
    {
        if (mIDMap.ops)
            PL_DHashTableFinish(&mIDMap);
        if (mObjectMap.ops)
            PL_DHashTableFinish(&mObjectMap);
        if (mDocumentMap.ops)
            PL_DHashTableFinish(&mDocumentMap);
        if (mURIMap.ops)
            PL_DHashTableFinish(&mURIMap);
        if (mDependencyMap.ops)
            PL_DHashTableFinish(&mDependencyMap);
        MOZ_COUNT_DTOR(nsFastLoadFileWriter);
    }

  private:
    
    NS_DECL_ISUPPORTS_INHERITED

    
    NS_IMETHOD WriteObject(nsISupports* aObject, PRBool aIsStrongRef);
    NS_IMETHOD WriteSingleRefObject(nsISupports* aObject);
    NS_IMETHOD WriteCompoundObject(nsISupports* aObject,
                                   const nsIID& aIID,
                                   PRBool aIsStrongRef);
    NS_IMETHOD WriteID(const nsID& aID);

    
    NS_IMETHOD SetOutputStream(nsIOutputStream* aOutputStream);

    
    NS_DECL_NSIFASTLOADFILECONTROL

    
    NS_DECL_NSIFASTLOADWRITECONTROL

    
    NS_DECL_NSISEEKABLESTREAM

    nsresult MapID(const nsID& aSlowID, NSFastLoadID *aResult);

    nsresult WriteHeader(nsFastLoadHeader *aHeader);
    nsresult WriteFooter();
    nsresult WriteFooterPrefix(const nsFastLoadFooterPrefix& aFooterPrefix);
    nsresult WriteSlowID(const nsID& aID);
    nsresult WriteFastID(NSFastLoadID aID);
    nsresult WriteSharpObjectInfo(const nsFastLoadSharpObjectInfo& aInfo);
    nsresult WriteMuxedDocumentInfo(const nsFastLoadMuxedDocumentInfo& aInfo);

    nsresult   Init();
    nsresult   Open();
    NS_IMETHOD Close();

    nsresult WriteObjectCommon(nsISupports* aObject,
                               PRBool aIsStrongRef,
                               PRUint32 aQITag);

    static PLDHashOperator
    IDMapEnumerate(PLDHashTable *aTable,
                   PLDHashEntryHdr *aHdr,
                   PRUint32 aNumber,
                   void *aData);

    static PLDHashOperator
    ObjectMapEnumerate(PLDHashTable *aTable,
                       PLDHashEntryHdr *aHdr,
                       PRUint32 aNumber,
                       void *aData);

    static PLDHashOperator
    DocumentMapEnumerate(PLDHashTable *aTable,
                         PLDHashEntryHdr *aHdr,
                         PRUint32 aNumber,
                         void *aData);

    static PLDHashOperator
    DependencyMapEnumerate(PLDHashTable *aTable,
                           PLDHashEntryHdr *aHdr,
                           PRUint32 aNumber,
                           void *aData);

  protected:
    
    nsCOMPtr<nsISeekableStream> mSeekableOutput;

    nsFastLoadHeader mHeader;

    PLDHashTable mIDMap;
    PLDHashTable mObjectMap;
    PLDHashTable mDocumentMap;
    PLDHashTable mURIMap;
    PLDHashTable mDependencyMap;

    nsDocumentMapWriteEntry* mCurrentDocumentMapEntry;
    nsCOMPtr<nsIFastLoadFileIO> mFileIO;
};

NS_COM nsresult
NS_NewFastLoadFileWriter(nsIObjectOutputStream* *aResult NS_OUTPARAM,
                         nsIOutputStream* aDestStream,
                         nsIFastLoadFileIO* aFileIO);








class nsFastLoadFileUpdater
    : public nsFastLoadFileWriter
{
  public:
    nsFastLoadFileUpdater(nsIOutputStream* aOutputStream, nsIFastLoadFileIO *aFileIO)
        : nsFastLoadFileWriter(aOutputStream, aFileIO) {
        MOZ_COUNT_CTOR(nsFastLoadFileUpdater);
    }

    virtual ~nsFastLoadFileUpdater() {
        MOZ_COUNT_DTOR(nsFastLoadFileUpdater);
    }

  private:
    
    NS_DECL_ISUPPORTS_INHERITED

    nsresult   Open(nsFastLoadFileReader* aReader);
    NS_IMETHOD Close();

    static PLDHashOperator
    CopyReadDocumentMapEntryToUpdater(PLDHashTable *aTable,
                                      PLDHashEntryHdr *aHdr,
                                      PRUint32 aNumber,
                                      void *aData);

    friend class nsFastLoadFileReader;

  protected:
    nsCOMPtr<nsIInputStream> mInputStream;

    
    nsCOMPtr<nsISeekableStream> mSeekableInput;
};

NS_COM nsresult
NS_NewFastLoadFileUpdater(nsIObjectOutputStream* *aResult NS_OUTPARAM,
                          nsIFastLoadFileIO* aFileIO,
                          nsIObjectInputStream* aReaderAsStream);

#endif
