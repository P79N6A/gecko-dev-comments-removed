




































#ifndef nsINode_h___
#define nsINode_h___

#include "nsPIDOMEventTarget.h"
#include "nsEvent.h"
#include "nsPropertyTable.h"
#include "nsTObserverArray.h"
#include "nsINodeInfo.h"
#include "nsCOMPtr.h"

class nsIContent;
class nsIDocument;
class nsIDOMEvent;
class nsPresContext;
class nsEventChainVisitor;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;
class nsIEventListenerManager;
class nsIPrincipal;
class nsVoidArray;
class nsIMutationObserver;
class nsChildContentList;
class nsNodeWeakReference;
class nsNodeSupportsWeakRefTearoff;
class nsDOMNodeAllocator;

enum {
  
  NODE_DOESNT_HAVE_SLOTS =       0x00000001U,

  
  
  NODE_HAS_LISTENERMANAGER =     0x00000002U,

  
  NODE_HAS_PROPERTIES =          0x00000004U,

  
  
  NODE_IS_ANONYMOUS =            0x00000008U,

  
  
  NODE_IS_ANONYMOUS_FOR_EVENTS = 0x00000010U,

  
  
  NODE_MAY_HAVE_FRAME =          0x00000020U,

  
  
  NODE_FORCE_XBL_BINDINGS =      0x00000040U,

  
  NODE_MAY_BE_IN_BINDING_MNGR =  0x00000080U,

  NODE_IS_EDITABLE =             0x00000100U,

  
  
  NODE_MAY_HAVE_ID =             0x00000200U,
  NODE_MAY_HAVE_CLASS =          0x00000400U,
  NODE_MAY_HAVE_STYLE =          0x00000800U,

  NODE_IS_INSERTION_PARENT =     0x00001000U,

  
  NODE_SCRIPT_TYPE_OFFSET =               13,

  NODE_SCRIPT_TYPE_SIZE =                  4,

  
  NODE_TYPE_SPECIFIC_BITS_OFFSET =
    NODE_SCRIPT_TYPE_OFFSET + NODE_SCRIPT_TYPE_SIZE
};






template<class C, class D>
inline nsINode* NODE_FROM(C& aContent, D& aDocument)
{
  if (aContent)
    return static_cast<nsINode*>(aContent);
  return static_cast<nsINode*>(aDocument);
}



#define NS_INODE_IID \
{ 0x42f3a894, 0xe5d4, 0x44b1, \
  { 0x96, 0x34, 0x73, 0x8f, 0xf8, 0xbe, 0x5d, 0x9b } }


class nsINode_base : public nsPIDOMEventTarget {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INODE_IID)
};






class nsINode : public nsINode_base {
public:
  friend class nsNodeUtils;
  friend class nsNodeWeakReference;
  friend class nsNodeSupportsWeakRefTearoff;

#ifdef MOZILLA_INTERNAL_API
  nsINode(nsINodeInfo* aNodeInfo)
    : mNodeInfo(aNodeInfo),
      mParentPtrBits(0),
      mFlagsOrSlots(NODE_DOESNT_HAVE_SLOTS)
  {
  }
#endif

  virtual ~nsINode();

  


  enum {
    
    eCONTENT             = 1 << 0,
    
    eDOCUMENT            = 1 << 1,
    
    eATTRIBUTE           = 1 << 2,
    
    eELEMENT             = 1 << 3,
    
    eTEXT                = 1 << 4,
    
    ePROCESSING_INSTRUCTION = 1 << 5,
    
    eCOMMENT             = 1 << 6,
    
    eHTML                = 1 << 7,
    
    eHTML_FORM_CONTROL   = 1 << 8,
    
    eXUL                 = 1 << 9,
    
    eSVG                 = 1 << 10,
    
    eDOCUMENT_FRAGMENT   = 1 << 11,
    

    eDATA_NODE           = 1 << 12
  };

  







  virtual PRBool IsNodeOfType(PRUint32 aFlags) const = 0;

  



  virtual PRUint32 GetChildCount() const = 0;

  




  virtual nsIContent* GetChildAt(PRUint32 aIndex) const = 0;

  







  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const = 0;

  





  nsIDocument *GetOwnerDoc() const
  {
    return mNodeInfo->GetDocument();
  }

  




  PRBool IsInDoc() const
  {
    return mParentPtrBits & PARENT_BIT_INDOCUMENT;
  }

  





  nsIDocument *GetCurrentDoc() const
  {
    return IsInDoc() ? GetOwnerDoc() : nsnull;
  }

  



















  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify) = 0;

  

















  nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify)
  {
    return InsertChildAt(aKid, GetChildCount(), aNotify);
  }
  
  










  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify) = 0;

  










  void* GetProperty(nsIAtom *aPropertyName,
                    nsresult *aStatus = nsnull) const
  {
    return GetProperty(0, aPropertyName, aStatus);
  }

  











  virtual void* GetProperty(PRUint16 aCategory,
                            nsIAtom *aPropertyName,
                            nsresult *aStatus = nsnull) const;

  
















  nsresult SetProperty(nsIAtom *aPropertyName,
                       void *aValue,
                       NSPropertyDtorFunc aDtor = nsnull,
                       PRBool aTransfer = PR_FALSE)
  {
    return SetProperty(0, aPropertyName, aValue, aDtor, aTransfer);
  }

  


















  virtual nsresult SetProperty(PRUint16 aCategory,
                               nsIAtom *aPropertyName,
                               void *aValue,
                               NSPropertyDtorFunc aDtor = nsnull,
                               PRBool aTransfer = PR_FALSE,
                               void **aOldValue = nsnull);

  







  nsresult DeleteProperty(nsIAtom *aPropertyName)
  {
    return DeleteProperty(0, aPropertyName);
  }

  








  virtual nsresult DeleteProperty(PRUint16 aCategory, nsIAtom *aPropertyName);

  












  void* UnsetProperty(nsIAtom  *aPropertyName,
                      nsresult *aStatus = nsnull)
  {
    return UnsetProperty(0, aPropertyName, aStatus);
  }

  













  virtual void* UnsetProperty(PRUint16 aCategory,
                              nsIAtom *aPropertyName,
                              nsresult *aStatus = nsnull);
  
  PRBool HasProperties() const
  {
    return HasFlag(NODE_HAS_PROPERTIES);
  }

  



  nsIPrincipal* NodePrincipal() const {
    return mNodeInfo->NodeInfoManager()->DocumentPrincipal();
  }

  



  nsIContent* GetParent() const
  {
    return NS_LIKELY(mParentPtrBits & PARENT_BIT_PARENT_IS_CONTENT) ?
           reinterpret_cast<nsIContent*>
                           (mParentPtrBits & ~kParentBitMask) :
           nsnull;
  }

  




  nsINode* GetNodeParent() const
  {
    return reinterpret_cast<nsINode*>(mParentPtrBits & ~kParentBitMask);
  }

  







  virtual void AddMutationObserver(nsIMutationObserver* aMutationObserver);

  


  virtual void RemoveMutationObserver(nsIMutationObserver* aMutationObserver);

  








  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const = 0;

  









  PRBool HasSameOwnerDoc(nsINode *aOther)
  {
    
    
    return mNodeInfo->NodeInfoManager() == aOther->mNodeInfo->NodeInfoManager();
  }

  
  
  class nsSlots
  {
  public:
    nsSlots(PtrBits aFlags)
      : mFlags(aFlags),
        mChildNodes(nsnull),
        mWeakReference(nsnull)
    {
    }

    
    
    virtual ~nsSlots();

    




    PtrBits mFlags;

    


    nsTObserverArray<nsIMutationObserver*> mMutationObservers;

    






    nsChildContentList* mChildNodes;

    


    nsNodeWeakReference* mWeakReference;

    void* operator new(size_t aSize, nsDOMNodeAllocator* aAllocator);
    void operator delete(void* aPtr, size_t aSize);
private:
    void* operator new(size_t aSize) CPP_THROW_NEW;
  };

  


#ifdef DEBUG
  nsSlots* DebugGetSlots()
  {
    return GetSlots();
  }
#endif

  PRBool HasFlag(PtrBits aFlag) const
  {
    return !!(GetFlags() & aFlag);
  }

  PtrBits GetFlags() const
  {
    return NS_UNLIKELY(HasSlots()) ? FlagsAsSlots()->mFlags : mFlagsOrSlots;
  }

  void SetFlags(PtrBits aFlagsToSet)
  {
    NS_ASSERTION(!(aFlagsToSet & (NODE_IS_ANONYMOUS | NODE_MAY_HAVE_FRAME)) ||
                 IsNodeOfType(eCONTENT),
                 "Flag only permitted on nsIContent nodes");
    PtrBits* flags = HasSlots() ? &FlagsAsSlots()->mFlags :
                                  &mFlagsOrSlots;
    *flags |= aFlagsToSet;
  }

  void UnsetFlags(PtrBits aFlagsToUnset)
  {
    PtrBits* flags = HasSlots() ? &FlagsAsSlots()->mFlags :
                                  &mFlagsOrSlots;
    *flags &= ~aFlagsToUnset;
  }

  void SetEditableFlag(PRBool aEditable)
  {
    if (aEditable) {
      SetFlags(NODE_IS_EDITABLE);
    }
    else {
      UnsetFlags(NODE_IS_EDITABLE);
    }
  }

  PRBool IsEditable() const
  {
#ifdef _IMPL_NS_LAYOUT
    return IsEditableInternal();
#else
    return IsEditableExternal();
#endif
  }

  
  virtual nsDOMNodeAllocator* GetAllocator() = 0;

  void* operator new(size_t aSize, nsINodeInfo* aNodeInfo);
  void operator delete(void* aPtr, size_t aSize);
private:
  void* operator new(size_t aSize) CPP_THROW_NEW;
protected:

  
  virtual nsINode::nsSlots* CreateSlots();

  PRBool HasSlots() const
  {
    return !(mFlagsOrSlots & NODE_DOESNT_HAVE_SLOTS);
  }

  nsSlots* FlagsAsSlots() const
  {
    NS_ASSERTION(HasSlots(), "check HasSlots first");
    return reinterpret_cast<nsSlots*>(mFlagsOrSlots);
  }

  nsSlots* GetExistingSlots() const
  {
    return HasSlots() ? FlagsAsSlots() : nsnull;
  }

  nsSlots* GetSlots()
  {
    if (HasSlots()) {
      return FlagsAsSlots();
    }

    nsSlots* slots = CreateSlots();
    if (slots) {
      mFlagsOrSlots = reinterpret_cast<PtrBits>(slots);
    }

    return slots;
  }

  PRBool IsEditableInternal() const;
  virtual PRBool IsEditableExternal() const
  {
    return IsEditableInternal();
  }

  nsCOMPtr<nsINodeInfo> mNodeInfo;

  enum { PARENT_BIT_INDOCUMENT = 1 << 0, PARENT_BIT_PARENT_IS_CONTENT = 1 << 1 };
  enum { kParentBitMask = 0x3 };

  PtrBits mParentPtrBits;

  





  PtrBits mFlagsOrSlots;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINode_base, NS_INODE_IID)

#endif 
