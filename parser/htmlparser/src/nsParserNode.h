





















#ifndef NS_PARSERNODE__
#define NS_PARSERNODE__

#include "nsIParserNode.h"
#include "nsToken.h"
#include "nsString.h"
#include "nsParserCIID.h"
#include "nsDeque.h"
#include "nsDTDUtils.h"

class nsTokenAllocator;

class nsCParserNode :  public nsIParserNode {

  protected:

    int32_t mRefCnt;

  public:

    void AddRef()
    {
      ++mRefCnt;
    }

    void Release(nsDummyAllocator& aPool)
    {
      if (--mRefCnt == 0)
        Destroy(this, aPool);
    }

#ifndef HEAP_ALLOCATED_NODES
  protected:

    


    static void* operator new(size_t) CPP_THROW_NEW { return 0; }

    


    static void operator delete(void*,size_t) {}

#endif

  public:
    static nsCParserNode* Create(CToken* aToken,
                                 nsTokenAllocator* aTokenAllocator,
                                 nsNodeAllocator* aNodeAllocator)
    {
#ifdef HEAP_ALLOCATED_NODES
      return new
#else
      nsDummyAllocator& pool = aNodeAllocator->GetArenaPool();
      void* place = pool.Alloc(sizeof(nsCParserNode));
      NS_ENSURE_TRUE(place, nullptr);
      return ::new (place)
#endif
        nsCParserNode(aToken, aTokenAllocator, aNodeAllocator);
    }

    static void Destroy(nsCParserNode* aNode, nsDummyAllocator& aPool)
    {
#ifdef HEAP_ALLOCATED_NODES
      delete aNode;
#else
      aNode->~nsCParserNode();
      aPool.Free(aNode);
#endif
    }

    


    nsCParserNode();

    




    nsCParserNode(CToken* aToken,
                  nsTokenAllocator* aTokenAllocator,
                  nsNodeAllocator* aNodeAllocator=0);

    



    virtual ~nsCParserNode();

    



    virtual nsresult Init(CToken* aToken,
                          nsTokenAllocator* aTokenAllocator,
                          nsNodeAllocator* aNodeAllocator=0);

    




    virtual const nsAString& GetTagName() const;

    




    virtual const nsAString& GetText() const;

    




    virtual int32_t GetNodeType()  const;

    




    virtual int32_t GetTokenType()  const;


    
    
    

    




    virtual int32_t GetAttributeCount(bool askToken=false) const;

    





    virtual const nsAString& GetKeyAt(uint32_t anIndex) const;

    





    virtual const nsAString& GetValueAt(uint32_t anIndex) const;

    






    virtual int32_t TranslateToUnicodeStr(nsString& aString) const;

    





    virtual void AddAttribute(CToken* aToken);

    





    virtual int32_t GetSourceLineNumber(void) const;

    



    virtual CToken* PopAttributeToken();

    
    virtual CToken* PopAttributeTokenFront();

    



    virtual void GetSource(nsString& aString) const;

    




    virtual bool    GetGenericState(void) const {return mGenericState;}
    virtual void    SetGenericState(bool aState) {mGenericState=aState;}

    



    virtual nsresult ReleaseAll();

    bool mGenericState;  
    int32_t      mUseCount;
    CToken*      mToken;
   
    nsTokenAllocator* mTokenAllocator;
#ifdef HEAP_ALLOCATED_NODES
   nsNodeAllocator*  mNodeAllocator; 
#endif
};


class nsCParserStartNode :  public nsCParserNode 
{
public:
    static nsCParserNode* Create(CToken* aToken,
                                 nsTokenAllocator* aTokenAllocator,
                                 nsNodeAllocator* aNodeAllocator)
    {
#ifdef HEAP_ALLOCATED_NODES
      return new
#else
      nsDummyAllocator& pool = aNodeAllocator->GetArenaPool();
      void* place = pool.Alloc(sizeof(nsCParserStartNode));
      NS_ENSURE_TRUE(place, nullptr);
      return ::new (place)
#endif
        nsCParserStartNode(aToken, aTokenAllocator, aNodeAllocator);
    }

    nsCParserStartNode() 
      : nsCParserNode(), mAttributes(0) { }

    nsCParserStartNode(CToken* aToken, 
                       nsTokenAllocator* aTokenAllocator, 
                       nsNodeAllocator* aNodeAllocator = 0) 
      : nsCParserNode(aToken, aTokenAllocator, aNodeAllocator), mAttributes(0) { }

    virtual ~nsCParserStartNode() 
    {
      NS_ASSERTION(mTokenAllocator || mAttributes.GetSize() == 0,
                   "Error: no token allocator");
      CToken* theAttrToken = 0;
      while ((theAttrToken = static_cast<CToken*>(mAttributes.Pop()))) {
        IF_FREE(theAttrToken, mTokenAllocator);
      }
    }

    virtual nsresult Init(CToken* aToken,
                          nsTokenAllocator* aTokenAllocator,
                          nsNodeAllocator* aNodeAllocator = 0);
    virtual void     AddAttribute(CToken* aToken);
    virtual int32_t  GetAttributeCount(bool askToken = false) const;
    virtual const    nsAString& GetKeyAt(uint32_t anIndex) const;
    virtual const    nsAString& GetValueAt(uint32_t anIndex) const;
    virtual CToken*  PopAttributeToken();
    virtual CToken*  PopAttributeTokenFront();
    virtual void     GetSource(nsString& aString) const;
    virtual nsresult ReleaseAll();
protected:
    nsDeque  mAttributes;
};

#endif

