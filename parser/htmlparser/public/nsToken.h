

























#ifndef CTOKEN__
#define CTOKEN__

#include "prtypes.h"
#include "nsString.h"
#include "nsError.h"

class nsScanner;
class nsTokenAllocator;




class nsDummyAllocator {
public:
  void* Alloc(size_t aSize) { return malloc(aSize); }
  void Free(void* aPtr) { free(aPtr); }
};

enum eContainerInfo {
  eWellFormed,
  eMalformed,
  eFormUnknown
};










class CToken {
  public:

    enum  eTokenOrigin {eSource,eResidualStyle};

  protected:

    
    
    friend class nsTokenAllocator;

    





    static void * operator new (size_t aSize, nsDummyAllocator& anArena) CPP_THROW_NEW
    {
      return anArena.Alloc(aSize);
    }

    


    static void operator delete (void*,size_t) {}

  protected:
    



    virtual ~CToken();

  private:
    


    static void Destroy(CToken* aToken, nsDummyAllocator& aArenaPool)
    {
      aToken->~CToken();
      aArenaPool.Free(aToken);
    }

  public:
    



    void AddRef() {
      ++mUseCount;
      NS_LOG_ADDREF(this, mUseCount, "CToken", sizeof(*this));
    }
    
    



    void Release(nsDummyAllocator& aArenaPool) {
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
    int32_t mTypeID;
    int32_t mUseCount;
    int32_t mNewlineCount;
    uint32_t mLineNumber : 31;
    uint32_t mInError : 1;
    int16_t mAttrCount;
};



#endif


