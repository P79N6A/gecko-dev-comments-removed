














#ifndef DTDUTILS_
#define DTDUTILS_

#include "nsHTMLTags.h"
#include "nsHTMLTokens.h"
#include "nsIParser.h"
#include "nsCRT.h"
#include "nsDeque.h"
#include "nsIDTD.h"
#include "nsITokenizer.h"
#include "nsString.h"
#include "nsIParserNode.h"
#include "nsCOMArray.h"
#include "nsIParserService.h"
#include "nsReadableUtils.h"
#include "nsIHTMLContentSink.h"
#include "nsIFrame.h"

#define IF_HOLD(_ptr) \
 PR_BEGIN_MACRO       \
 if(_ptr) {           \
   _ptr->AddRef();    \
 }                    \
 PR_END_MACRO


#define IF_FREE(_ptr, _allocator)                \
  PR_BEGIN_MACRO                                 \
  if(_ptr && _allocator) {                       \
    _ptr->Release((_allocator)->GetArenaPool()); \
    _ptr=0;                                      \
  }                                              \
  PR_END_MACRO   


#define IF_DELETE(_ptr, _allocator) \
  PR_BEGIN_MACRO                    \
  if(_ptr) {                        \
    _ptr->ReleaseAll(_allocator);   \
    delete(_ptr);                   \
    _ptr=0;                         \
  }                                 \
  PR_END_MACRO

class nsIParserNode;
class nsCParserNode;
class nsNodeAllocator;


#ifdef DEBUG
void DebugDumpContainmentRules(nsIDTD& theDTD,const char* aFilename,const char* aTitle);
void DebugDumpContainmentRules2(nsIDTD& theDTD,const char* aFilename,const char* aTitle);
#endif





class nsEntryStack;  

struct nsTagEntry {
  nsTagEntry ()
    : mTag(eHTMLTag_unknown), mNode(0), mParent(0), mStyles(0){}
  eHTMLTags       mTag;  
  nsCParserNode*  mNode;
  nsEntryStack*   mParent;
  nsEntryStack*   mStyles;
};

class nsEntryStack {

public:
                  nsEntryStack();
                  ~nsEntryStack();

  nsTagEntry*     PopEntry();
  void            PushEntry(nsTagEntry* aEntry, bool aRefCntNode = true);
  void            EnsureCapacityFor(int32_t aNewMax, int32_t aShiftOffset=0);
  void            Push(nsCParserNode* aNode,nsEntryStack* aStyleStack=0, bool aRefCntNode = true);
  void            PushTag(eHTMLTags aTag);
  void            PushFront(nsCParserNode* aNode,nsEntryStack* aStyleStack=0, bool aRefCntNode = true);
  void            Append(nsEntryStack *aStack);
  nsCParserNode*  Pop(void);
  nsCParserNode*  Remove(int32_t anIndex,eHTMLTags aTag);
  nsCParserNode*  NodeAt(int32_t anIndex) const;
  eHTMLTags       First() const;
  eHTMLTags       TagAt(int32_t anIndex) const;
  nsTagEntry*     EntryAt(int32_t anIndex) const;
  eHTMLTags       operator[](int32_t anIndex) const;
  eHTMLTags       Last() const;
  void            Empty(void); 

  


  void ReleaseAll(nsNodeAllocator* aNodeAllocator);
  
  





  inline int32_t FirstOf(eHTMLTags aTag) const {
    int32_t index=-1;
    
    if(0<mCount) {
      while(++index<mCount) {
        if(aTag==mEntries[index].mTag) {
          return index;
        }
      } 
    }
    return kNotFound;
  }


  





  inline int32_t LastOf(eHTMLTags aTag) const {
    int32_t index=mCount;
    while(--index>=0) {
        if(aTag==mEntries[index].mTag) {
          return index; 
        }
    }
    return kNotFound;
  }

  nsTagEntry* mEntries;
  int32_t    mCount;
  int32_t    mCapacity;
};








class CTableState {
public:
  CTableState(CTableState *aPreviousState=0) {
    mHasCaption=false;
    mHasCols=false;
    mHasTHead=false;
    mHasTFoot=false;
    mHasTBody=false;    
    mPrevious=aPreviousState;
  }

  bool    CanOpenCaption() {
    bool result=!(mHasCaption || mHasCols || mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  bool    CanOpenCols() {
    bool result=!(mHasCols || mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  bool    CanOpenTBody() {
    bool result=!(mHasTBody);
    return result;
  }

  bool    CanOpenTHead() {
    bool result=!(mHasTHead || mHasTFoot || mHasTBody);
    return result;
  }

  bool    CanOpenTFoot() {
    bool result=!(mHasTFoot || mHasTBody);
    return result;
  }

  bool          mHasCaption;
  bool          mHasCols;
  bool          mHasTHead;
  bool          mHasTFoot;
  bool          mHasTBody;
  CTableState   *mPrevious;
};











class nsTokenAllocator
{
public: 

  nsTokenAllocator();
  ~nsTokenAllocator();
  CToken* CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsAString& aString);
  CToken* CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag);

  nsDummyAllocator& GetArenaPool() { return mArenaPool; }

protected:
  nsDummyAllocator mArenaPool;
#ifdef  DEBUG
  int mTotals[eToken_last-1];
#endif
};








#ifndef HEAP_ALLOCATED_NODES
class nsCParserNode;
#endif

class nsNodeAllocator
{
public:
  
  nsNodeAllocator();
  ~nsNodeAllocator();
  nsCParserNode* CreateNode(CToken* aToken=nullptr, nsTokenAllocator* aTokenAllocator=0);

  nsDummyAllocator&  GetArenaPool() { return mNodePool; }

#ifdef HEAP_ALLOCATED_NODES
  void Recycle(nsCParserNode* aNode) { mSharedNodes.Push(static_cast<void*>(aNode)); }
protected:
  nsDeque mSharedNodes;
#ifdef DEBUG_TRACK_NODES
  int32_t mCount;
#endif
#endif

protected:
  nsDummyAllocator mNodePool;
};





class nsDTDContext
{
public:
                  nsDTDContext();
                  ~nsDTDContext();

  nsTagEntry*     PopEntry();
  void            PushEntry(nsTagEntry* aEntry, bool aRefCntNode = true);
  void            MoveEntries(nsDTDContext& aDest, int32_t aCount);
  void            Push(nsCParserNode* aNode,nsEntryStack* aStyleStack=0, bool aRefCntNode = true);
  void            PushTag(eHTMLTags aTag);
  nsCParserNode*  Pop(nsEntryStack*& aChildStack);
  nsCParserNode*  Pop();
  nsCParserNode*  PeekNode() { return mStack.NodeAt(mStack.mCount-1); }
  eHTMLTags       First(void) const;
  eHTMLTags       Last(void) const;
  nsTagEntry*     LastEntry(void) const;
  eHTMLTags       TagAt(int32_t anIndex) const;
  eHTMLTags       operator[](int32_t anIndex) const {return TagAt(anIndex);}
  bool            HasOpenContainer(eHTMLTags aTag) const;
  int32_t         FirstOf(eHTMLTags aTag) const {return mStack.FirstOf(aTag);}
  int32_t         LastOf(eHTMLTags aTag) const {return mStack.LastOf(aTag);}

  void            Empty(void); 
  int32_t         GetCount(void) const {return mStack.mCount;}
  int32_t         GetResidualStyleCount(void) {return mResidualStyleCount;}
  nsEntryStack*   GetStylesAt(int32_t anIndex) const;
  void            PushStyle(nsCParserNode* aNode);
  void            PushStyles(nsEntryStack *aStyles);
  nsCParserNode*  PopStyle(void);
  nsCParserNode*  PopStyle(eHTMLTags aTag);
  void            RemoveStyle(eHTMLTags aTag);

  static  void    ReleaseGlobalObjects(void);

  void            SetTokenAllocator(nsTokenAllocator* aTokenAllocator) { mTokenAllocator=aTokenAllocator; }
  void            SetNodeAllocator(nsNodeAllocator* aNodeAllocator) { mNodeAllocator=aNodeAllocator; }

  nsEntryStack    mStack; 
  int32_t         mResidualStyleCount;
  int32_t         mContextTopIndex;

  nsTokenAllocator  *mTokenAllocator;
  nsNodeAllocator   *mNodeAllocator;

#ifdef  DEBUG
  enum { eMaxTags = MAX_REFLOW_DEPTH };
  eHTMLTags       mXTags[eMaxTags];
#endif
};




class CTokenDeallocator: public nsDequeFunctor{
protected:
  nsDummyAllocator& mArenaPool;

public:
  CTokenDeallocator(nsDummyAllocator& aArenaPool)
    : mArenaPool(aArenaPool) {}

  virtual void* operator()(void* anObject) {
    CToken* aToken = (CToken*)anObject;
    aToken->Release(mArenaPool);
    return 0;
  }
};





class nsITagHandler {
public:
  
  virtual void          SetString(const nsString &aTheString)=0;
  virtual nsString*     GetString()=0;
  virtual bool          HandleToken(CToken* aToken,nsIDTD* aDTD)=0;
  virtual bool          HandleCapturedTokens(CToken* aToken,nsIDTD* aDTD)=0;
};













inline int32_t IndexOfTagInSet(int32_t aTag,const eHTMLTags* aTagSet,int32_t aCount)  {

  const eHTMLTags* theEnd=aTagSet+aCount;
  const eHTMLTags* theTag=aTagSet;

  while(theTag<theEnd) {
    if(aTag==*theTag) {
      return theTag-aTagSet;
    }
    ++theTag;
  }

  return kNotFound;
}









inline bool FindTagInSet(int32_t aTag,const eHTMLTags *aTagSet,int32_t aCount)  {
  return bool(-1<IndexOfTagInSet(aTag,aTagSet,aCount));
}



struct TagList {
  size_t mCount;
  const eHTMLTags *mTags;
};








inline int32_t LastOf(nsDTDContext& aContext, const TagList& aTagList){
  int max = aContext.GetCount();
  int index;
  for(index=max-1;index>=0;index--){
    bool result=FindTagInSet(aContext[index],aTagList.mTags,aTagList.mCount);
    if(result) {
      return index;
    }
  }
  return kNotFound;
}
 








inline int32_t FirstOf(nsDTDContext& aContext,int32_t aStartOffset,TagList& aTagList){
  int max = aContext.GetCount();
  int index;
  for(index=aStartOffset;index<max;++index){
    bool result=FindTagInSet(aContext[index],aTagList.mTags,aTagList.mCount);
    if(result) {
      return index;
    }
  }
  return kNotFound;
}








inline bool HasOptionalEndTag(eHTMLTags aTag) {
  static eHTMLTags gHasOptionalEndTags[]={eHTMLTag_body,eHTMLTag_colgroup,eHTMLTag_dd,eHTMLTag_dt,
                                                    eHTMLTag_head,eHTMLTag_li,eHTMLTag_option,
                                                    eHTMLTag_p,eHTMLTag_tbody,eHTMLTag_td,eHTMLTag_tfoot,
                                                    eHTMLTag_th,eHTMLTag_thead,eHTMLTag_tr,
                                                    eHTMLTag_userdefined,eHTMLTag_unknown};
  return FindTagInSet(aTag,gHasOptionalEndTags,sizeof(gHasOptionalEndTags)/sizeof(eHTMLTag_body));
}
#endif
