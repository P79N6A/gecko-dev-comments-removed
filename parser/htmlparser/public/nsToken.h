


































































#ifndef CTOKEN__
#define CTOKEN__

#include "prtypes.h"
#include "nsString.h"
#include "nsError.h"
#include "nsFixedSizeAllocator.h"

#define NS_HTMLTOKENS_NOT_AN_ENTITY \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_HTMLPARSER,2000)

class nsScanner;
class nsTokenAllocator;

enum eContainerInfo {
  eWellFormed,
  eMalformed,
  eFormUnknown
};





#define CTOKEN_IMPL_SIZEOF                                \
protected:                                                \
  virtual size_t SizeOf() const { return sizeof(*this); } \
public:










class CToken {
  public:

    enum  eTokenOrigin {eSource,eResidualStyle};

  protected:

    
    
    friend class nsTokenAllocator;

    





    static void * operator new (size_t aSize,nsFixedSizeAllocator& anArena) CPP_THROW_NEW
    {
      return anArena.Alloc(aSize);
    }

    


    static void operator delete (void*,size_t) {}

  protected:
    



    virtual ~CToken();

  private:
    


    static void Destroy(CToken* aToken,nsFixedSizeAllocator& aArenaPool)
    {
      size_t sz = aToken->SizeOf();
      aToken->~CToken();
      aArenaPool.Free(aToken, sz);
    }

  public:
    



    void AddRef() {
      ++mUseCount;
      NS_LOG_ADDREF(this, mUseCount, "CToken", sizeof(*this));
    }
    
    



    void Release(nsFixedSizeAllocator& aArenaPool) {
      --mUseCount;
      NS_LOG_RELEASE(this, mUseCount, "CToken");
      if (mUseCount==0)
        Destroy(this, aArenaPool);
    }

    



    CToken(PRInt32 aTag=0);

    




    virtual const nsSubstring& GetStringValue(void) = 0;

    





    virtual void GetSource(nsString& anOutputString);

    


    virtual void AppendSourceTo(nsAString& anOutputString);

    




    void SetTypeID(PRInt32 aValue) {
      mTypeID = aValue;
    }
    
    




    virtual PRInt32 GetTypeID(void);

    




    virtual PRInt16 GetAttributeCount(void);

    







    virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode);

    




    virtual PRInt32 GetTokenType(void);

    






    virtual bool IsWellFormed(void) const {return false;}

    virtual bool IsEmpty(void) { return false; }
    
    


    virtual void SetEmpty(bool aValue) { return ; }

    PRInt32 GetNewlineCount() 
    { 
      return mNewlineCount; 
    }

    void SetNewlineCount(PRInt32 aCount)
    {
      mNewlineCount = aCount;
    }

    PRInt32 GetLineNumber() 
    { 
      return mLineNumber;
    }

    void SetLineNumber(PRInt32 aLineNumber) 
    { 
      mLineNumber = mLineNumber == 0 ? aLineNumber : mLineNumber;
    }

    void SetInError(bool aInError)
    {
      mInError = aInError;
    }

    bool IsInError()
    {
      return mInError;
    }

    void SetAttributeCount(PRInt16 aValue) {  mAttrCount = aValue; }

    



    virtual void SelfTest(void);

    static int GetTokenCount();

    

protected:
    


    virtual size_t SizeOf() const = 0;

    PRInt32 mTypeID;
    PRInt32 mUseCount;
    PRInt32 mNewlineCount;
    PRUint32 mLineNumber : 31;
    PRUint32 mInError : 1;
    PRInt16 mAttrCount;
};



#endif


