





















































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

    PRInt32 mRefCnt;

  public:

    void AddRef()
    {
      ++mRefCnt;
    }

    void Release(nsFixedSizeAllocator& aPool)
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
      nsFixedSizeAllocator& pool = aNodeAllocator->GetArenaPool();
      void* place = pool.Alloc(sizeof(nsCParserNode));
      NS_ENSURE_TRUE(place, nsnull);
      return ::new (place)
#endif
        nsCParserNode(aToken, aTokenAllocator, aNodeAllocator);
    }

    static void Destroy(nsCParserNode* aNode, nsFixedSizeAllocator& aPool)
    {
#ifdef HEAP_ALLOCATED_NODES
      delete aNode;
#else
      aNode->~nsCParserNode();
      aPool.Free(aNode, sizeof(*aNode));
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

    




    virtual PRInt32 GetNodeType()  const;

    




    virtual PRInt32 GetTokenType()  const;


    
    
    

    




    virtual PRInt32 GetAttributeCount(PRBool askToken=PR_FALSE) const;

    





    virtual const nsAString& GetKeyAt(PRUint32 anIndex) const;

    





    virtual const nsAString& GetValueAt(PRUint32 anIndex) const;

    






    virtual PRInt32 TranslateToUnicodeStr(nsString& aString) const;

    





    virtual void AddAttribute(CToken* aToken);

    





    virtual PRInt32 GetSourceLineNumber(void) const;

    



    virtual CToken* PopAttributeToken();

    



    virtual void GetSource(nsString& aString) const;

    




    virtual PRBool  GetGenericState(void) const {return mGenericState;}
    virtual void    SetGenericState(PRBool aState) {mGenericState=aState;}

    



    virtual nsresult ReleaseAll();

    PRPackedBool mGenericState;  
    PRInt32      mUseCount;
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
      nsFixedSizeAllocator& pool = aNodeAllocator->GetArenaPool();
      void* place = pool.Alloc(sizeof(nsCParserStartNode));
      NS_ENSURE_TRUE(place, nsnull);
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
    virtual PRInt32  GetAttributeCount(PRBool askToken = PR_FALSE) const;
    virtual const    nsAString& GetKeyAt(PRUint32 anIndex) const;
    virtual const    nsAString& GetValueAt(PRUint32 anIndex) const;
    virtual CToken*  PopAttributeToken();
    virtual void     GetSource(nsString& aString) const;
    virtual nsresult ReleaseAll();
protected:
    nsDeque  mAttributes;
};

#endif

