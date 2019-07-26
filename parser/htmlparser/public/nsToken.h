


































#ifndef CTOKEN__
#define CTOKEN__

#include "prtypes.h"
#include "nsString.h"
#include "nsError.h"
#include "nsFixedSizeAllocator.h"

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

    



    CToken(int32_t aTag=0);

    




    virtual const nsSubstring& GetStringValue(void) = 0;

    





    virtual void GetSource(nsString& anOutputString);

    


    virtual void AppendSourceTo(nsAString& anOutputString);

    




    void SetTypeID(int32_t aValue) {
      mTypeID = aValue;
    }
    
    




    virtual int32_t GetTypeID(void);

    




    virtual int16_t GetAttributeCount(void);

    







    virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);

    




    virtual int32_t GetTokenType(void);

    






    virtual bool IsWellFormed(void) const {return false;}

    virtual bool IsEmpty(void) { return false; }
    
    


    virtual void SetEmpty(bool aValue) { return ; }

    int32_t GetNewlineCount() 
    { 
      return mNewlineCount; 
    }

    void SetNewlineCount(int32_t aCount)
    {
      mNewlineCount = aCount;
    }

    int32_t GetLineNumber() 
    { 
      return mLineNumber;
    }

    void SetLineNumber(int32_t aLineNumber) 
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

    void SetAttributeCount(int16_t aValue) {  mAttrCount = aValue; }

    



    virtual void SelfTest(void);

    static int GetTokenCount();

    

protected:
    


    virtual size_t SizeOf() const = 0;

    int32_t mTypeID;
    int32_t mUseCount;
    int32_t mNewlineCount;
    uint32_t mLineNumber : 31;
    uint32_t mInError : 1;
    int16_t mAttrCount;
};



#endif


