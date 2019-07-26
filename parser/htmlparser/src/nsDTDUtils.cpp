




       
  
#include "nsIAtom.h"
#include "nsDTDUtils.h" 
#include "CNavDTD.h" 
#include "nsIParserNode.h"
#include "nsParserNode.h" 
#include "nsIChannel.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtils.h"




















nsEntryStack::nsEntryStack()  {

  MOZ_COUNT_CTOR(nsEntryStack);
  
  mCapacity=0;
  mCount=0;
  mEntries=0;
}






nsEntryStack::~nsEntryStack() {

  MOZ_COUNT_DTOR(nsEntryStack);

  if(mEntries) {
    
    delete [] mEntries;
    mEntries=0;
  }

  mCount=mCapacity=0;
}




void 
nsEntryStack::ReleaseAll(nsNodeAllocator* aNodeAllocator)
{
  NS_ASSERTION(aNodeAllocator,"no allocator? - potential leak!");

  if(aNodeAllocator) {
    NS_ASSERTION(mCount >= 0,"count should not be negative");
    while(mCount > 0) {
      nsCParserNode* node=this->Pop();
      IF_FREE(node,aNodeAllocator);
    }
  }
}





void nsEntryStack::Empty(void) {
  mCount=0;
}






void nsEntryStack::EnsureCapacityFor(int32_t aNewMax,int32_t aShiftOffset) {
  if(mCapacity<aNewMax){ 

    const int kDelta=16;

    int32_t theSize = kDelta * ((aNewMax / kDelta) + 1);
    nsTagEntry* temp=new nsTagEntry[theSize]; 
    mCapacity=theSize;

    if(temp){ 
      int32_t index=0; 
      for(index=0;index<mCount;++index) {
        temp[aShiftOffset+index]=mEntries[index];
      }
      if(mEntries) delete [] mEntries;
      mEntries=temp;
    }
    else{
      
    }
  } 
}





void nsEntryStack::Push(nsCParserNode* aNode,
                        nsEntryStack* aStyleStack, 
                        bool aRefCntNode) 
{
  if(aNode) {
    EnsureCapacityFor(mCount+1);
    mEntries[mCount].mTag = (eHTMLTags)aNode->GetNodeType();
    if (aRefCntNode) {
      aNode->mUseCount++;
      mEntries[mCount].mNode = const_cast<nsCParserNode*>(aNode);
      IF_HOLD(mEntries[mCount].mNode);
    }
    mEntries[mCount].mParent=aStyleStack;
    mEntries[mCount++].mStyles=0;
  }
}

void nsEntryStack::PushTag(eHTMLTags aTag)
{
  EnsureCapacityFor(mCount + 1);
  mEntries[mCount].mTag = aTag;
  mEntries[mCount].mParent = nullptr;
  mEntries[mCount].mStyles = nullptr;
  ++mCount;
}







void nsEntryStack::PushFront(nsCParserNode* aNode,
                             nsEntryStack* aStyleStack, 
                             bool aRefCntNode) 
{
  if(aNode) {
    if(mCount<mCapacity) {
      int32_t index=0; 
      for(index=mCount;index>0;index--) {
        mEntries[index]=mEntries[index-1];
      }
    }
    else {
      EnsureCapacityFor(mCount+1,1);
    }
    mEntries[0].mTag = (eHTMLTags)aNode->GetNodeType();
    if (aRefCntNode) {
      aNode->mUseCount++;
      mEntries[0].mNode = const_cast<nsCParserNode*>(aNode);
      IF_HOLD(mEntries[0].mNode);
    }
    mEntries[0].mParent=aStyleStack;
    mEntries[0].mStyles=0;
    ++mCount;
  }
}





void nsEntryStack::Append(nsEntryStack *aStack) {
  if(aStack) {

    int32_t theCount=aStack->mCount;

    EnsureCapacityFor(mCount+aStack->mCount,0);

    int32_t theIndex=0;
    for(theIndex=0;theIndex<theCount;++theIndex){
      mEntries[mCount]=aStack->mEntries[theIndex];
      mEntries[mCount++].mParent=0;
    }
  }
} 
 













nsCParserNode* nsEntryStack::Remove(int32_t anIndex,
                                    eHTMLTags aTag) 
{
  nsCParserNode* result = 0;
  if (0 < mCount && anIndex < mCount){
    result = mEntries[anIndex].mNode;
    if (result)
      result->mUseCount--;
    int32_t theIndex = 0;
    mCount -= 1;
    for( theIndex = anIndex; theIndex < mCount; ++theIndex){
      mEntries[theIndex] = mEntries[theIndex+1];
    }
    mEntries[mCount].mNode = 0;
    mEntries[mCount].mStyles = 0;
    nsEntryStack* theStyleStack = mEntries[anIndex].mParent;
    if (theStyleStack) {
      
      
      uint32_t scount = theStyleStack->mCount;
#ifdef DEBUG_mrbkap
      NS_ASSERTION(scount != 0, "RemoveStyles has a bad style stack");
#endif
      nsTagEntry *theStyleEntry = theStyleStack->mEntries;
      for (uint32_t sindex = scount-1;; --sindex) {            
        if (theStyleEntry->mTag == aTag) {
          
          theStyleEntry->mParent = nullptr;
          break;
        }
        if (sindex == 0) {
#ifdef DEBUG_mrbkap
          NS_ERROR("Couldn't find the removed style on its parent stack");
#endif
          break;
        }
        ++theStyleEntry;
      }
    }
  }
  return result;
}






nsCParserNode* nsEntryStack::Pop(void)
{
  nsCParserNode* result = 0;
  if (0 < mCount) {
    result = mEntries[--mCount].mNode;
    if (result)
      result->mUseCount--;
    mEntries[mCount].mNode = 0;
    mEntries[mCount].mStyles = 0;
    nsEntryStack* theStyleStack = mEntries[mCount].mParent;
    if (theStyleStack) {
      
      
      uint32_t scount = theStyleStack->mCount;

      
      
#ifdef DEBUG_mrbkap
      NS_ASSERTION(scount != 0, "preventing a potential crash.");
#endif
      NS_ENSURE_TRUE(scount != 0, result);

      nsTagEntry *theStyleEntry = theStyleStack->mEntries;
      for (uint32_t sindex = scount - 1;; --sindex) {
        if (theStyleEntry->mTag == mEntries[mCount].mTag) {
          
          theStyleEntry->mParent = nullptr;
          break;
        }
        if (sindex == 0) {
#ifdef DEBUG_mrbkap
          NS_ERROR("Couldn't find the removed style on its parent stack");
#endif
          break;
        }
        ++theStyleEntry;
      }
    }
  }
  return result;
} 






eHTMLTags nsEntryStack::First() const 
{
  eHTMLTags result=eHTMLTag_unknown;
  if(0<mCount){
    result=mEntries[0].mTag;
  }
  return result;
}






nsCParserNode* nsEntryStack::NodeAt(int32_t anIndex) const 
{
  nsCParserNode* result=0;
  if((0<mCount) && (anIndex<mCount)) {
    result=mEntries[anIndex].mNode;
  }
  return result;
}






eHTMLTags nsEntryStack::TagAt(int32_t anIndex) const 
{
  eHTMLTags result=eHTMLTag_unknown;
  if((0<mCount) && (anIndex<mCount)) {
    result=mEntries[anIndex].mTag;
  }
  return result;
}





nsTagEntry* nsEntryStack::EntryAt(int32_t anIndex) const 
{
  nsTagEntry *result=0;
  if((0<mCount) && (anIndex<mCount)) {
    result=&mEntries[anIndex];
  }
  return result;
}







eHTMLTags nsEntryStack::operator[](int32_t anIndex) const 
{
  eHTMLTags result=eHTMLTag_unknown;
  if((0<mCount) && (anIndex<mCount)) {
    result=mEntries[anIndex].mTag;
  }
  return result;
}







eHTMLTags nsEntryStack::Last(void) const 
{
  eHTMLTags result=eHTMLTag_unknown;
  if(0<mCount) {
    result=mEntries[mCount-1].mTag;
  }
  return result;
}

nsTagEntry*
nsEntryStack::PopEntry() 
{
  nsTagEntry* entry = EntryAt(mCount-1);
  this->Pop();
  return entry;
}

void nsEntryStack::PushEntry(nsTagEntry* aEntry, 
                             bool aRefCntNode) 
{
  if (aEntry) {
    EnsureCapacityFor(mCount+1);
    mEntries[mCount].mNode   = aEntry->mNode;
    mEntries[mCount].mTag    = aEntry->mTag;
    mEntries[mCount].mParent = aEntry->mParent;
    mEntries[mCount].mStyles = aEntry->mStyles;
    if (aRefCntNode && mEntries[mCount].mNode) {
      mEntries[mCount].mNode->mUseCount++;
      IF_HOLD(mEntries[mCount].mNode);
    }
    mCount++;
  }
}










nsDTDContext::nsDTDContext() : mStack()
{
  MOZ_COUNT_CTOR(nsDTDContext);
  mResidualStyleCount=0;
  mContextTopIndex=-1;
  mTokenAllocator=0;
  mNodeAllocator=0;

#ifdef DEBUG
  memset(mXTags,0,sizeof(mXTags));
#endif
} 
 




nsDTDContext::~nsDTDContext()
{
  MOZ_COUNT_DTOR(nsDTDContext);
}






bool nsDTDContext::HasOpenContainer(eHTMLTags aTag) const {
  int32_t theIndex=mStack.LastOf(aTag);
  return bool(-1<theIndex);
}





void nsDTDContext::Push(nsCParserNode* aNode,
                        nsEntryStack* aStyleStack, 
                        bool aRefCntNode) {
  if(aNode) {
#ifdef  DEBUG
    eHTMLTags theTag = (eHTMLTags)aNode->GetNodeType();
    int size = mStack.mCount;
    if (size < eMaxTags)
      mXTags[size] = theTag;
#endif
    mStack.Push(aNode, aStyleStack, aRefCntNode);
  }
}

void nsDTDContext::PushTag(eHTMLTags aTag)
{
#ifdef DEBUG
  if (mStack.mCount < eMaxTags) {
    mXTags[mStack.mCount] = aTag;
  }
#endif

  mStack.PushTag(aTag);
}

nsTagEntry*
nsDTDContext::PopEntry()
{
  int32_t theSize = mStack.mCount;
  if(0<theSize) {
#ifdef  DEBUG
    if (theSize <= eMaxTags)
      mXTags[theSize-1]=eHTMLTag_unknown;
#endif
    return mStack.PopEntry();
  }
  return 0;
}

void nsDTDContext::PushEntry(nsTagEntry* aEntry, 
                             bool aRefCntNode)
{
#ifdef  DEBUG
    int size=mStack.mCount;
    if(size< eMaxTags && aEntry)
      mXTags[size]=aEntry->mTag;
#endif
    mStack.PushEntry(aEntry, aRefCntNode);
}





void 
nsDTDContext::MoveEntries(nsDTDContext& aDest,
                          int32_t aCount)
{
  NS_ASSERTION(aCount > 0 && mStack.mCount >= aCount, "cannot move entries");
  if (aCount > 0 && mStack.mCount >= aCount) {
    while (aCount) {
      aDest.PushEntry(&mStack.mEntries[--mStack.mCount], false);
#ifdef  DEBUG
      if (mStack.mCount < eMaxTags) {
        mXTags[mStack.mCount] = eHTMLTag_unknown;
      }
#endif
      --aCount;
    }
  }
}





nsCParserNode* nsDTDContext::Pop(nsEntryStack *&aChildStyleStack) {

  int32_t         theSize=mStack.mCount;
  nsCParserNode*  result=0;

  if(0<theSize) {

#ifdef  DEBUG
    if ((theSize>0) && (theSize <= eMaxTags))
      mXTags[theSize-1]=eHTMLTag_unknown;
#endif


    nsTagEntry* theEntry=mStack.EntryAt(mStack.mCount-1);
    aChildStyleStack=theEntry->mStyles;

    result=mStack.Pop();
    theEntry->mParent=0;
  }

  return result;
}
 





nsCParserNode* nsDTDContext::Pop() {
  nsEntryStack   *theTempStyleStack=0; 
  return Pop(theTempStyleStack);
}





eHTMLTags nsDTDContext::First(void) const {
  return mStack.First();
}





eHTMLTags nsDTDContext::TagAt(int32_t anIndex) const {
  return mStack.TagAt(anIndex);
}





nsTagEntry* nsDTDContext::LastEntry(void) const {
  return mStack.EntryAt(mStack.mCount-1);
}





eHTMLTags nsDTDContext::Last() const {
  return mStack.Last();
}






nsEntryStack* nsDTDContext::GetStylesAt(int32_t anIndex) const {
  nsEntryStack* result=0;

  if(anIndex<mStack.mCount){
    nsTagEntry* theEntry=mStack.EntryAt(anIndex);
    if(theEntry) {
      result=theEntry->mStyles;
    }
  }
  return result;
}






void nsDTDContext::PushStyle(nsCParserNode* aNode){

  nsTagEntry* theEntry=mStack.EntryAt(mStack.mCount-1);
  if(theEntry ) {
    nsEntryStack* theStack=theEntry->mStyles;
    if(!theStack) {
      theStack=theEntry->mStyles=new nsEntryStack();
    }
    if(theStack) {
      theStack->Push(aNode);
      ++mResidualStyleCount;
    }
  } 
}








void nsDTDContext::PushStyles(nsEntryStack *aStyles){

  if(aStyles) {
    nsTagEntry* theEntry=mStack.EntryAt(mStack.mCount-1);
    if(theEntry ) {
      nsEntryStack* theStyles=theEntry->mStyles;
      if(!theStyles) {
        theEntry->mStyles=aStyles;

        uint32_t scount=aStyles->mCount;
        uint32_t sindex=0;

        theEntry=aStyles->mEntries;
        for(sindex=0;sindex<scount;++sindex){            
          theEntry->mParent=0;  
          ++theEntry;
          ++mResidualStyleCount;
        } 

      }
      else {
        theStyles->Append(aStyles);
        
        delete aStyles;
        aStyles=0;
      }
    } 
    else if(mStack.mCount==0) {
      
      
      
      IF_DELETE(aStyles,mNodeAllocator);
    }
  }
}






nsCParserNode* nsDTDContext::PopStyle(void){
  nsCParserNode *result=0;

  nsTagEntry *theEntry=mStack.EntryAt(mStack.mCount-1);
  if(theEntry && (theEntry->mNode)) {
    nsEntryStack* theStyleStack=theEntry->mParent;
    if(theStyleStack){
      result=theStyleStack->Pop();
      mResidualStyleCount--;
    }
  } 
  return result;
}





nsCParserNode* nsDTDContext::PopStyle(eHTMLTags aTag){

  int32_t theLevel=0;
  nsCParserNode* result=0;

  for(theLevel=mStack.mCount-1;theLevel>0;theLevel--) {
    nsEntryStack *theStack=mStack.mEntries[theLevel].mStyles;
    if(theStack) {
      if(aTag==theStack->Last()) {
        result=theStack->Pop();
        mResidualStyleCount--;
        break; 
      } else {
        
      }
    } 
  }

  return result;
}









void nsDTDContext::RemoveStyle(eHTMLTags aTag){
  
  int32_t theLevel=mStack.mCount;
  
  while (theLevel) {
    nsEntryStack *theStack=GetStylesAt(--theLevel);
    if (theStack) {
      int32_t index=theStack->mCount;
      while (index){
        nsTagEntry *theEntry=theStack->EntryAt(--index);
        if (aTag==(eHTMLTags)theEntry->mNode->GetNodeType()) {
          mResidualStyleCount--;
          nsCParserNode* result=theStack->Remove(index,aTag);
          IF_FREE(result, mNodeAllocator);  
          return;
        } 
      }
    } 
  }
}






void nsDTDContext::ReleaseGlobalObjects(void){
}











nsTokenAllocator::nsTokenAllocator() {

  MOZ_COUNT_CTOR(nsTokenAllocator);

#ifdef DEBUG
  int i=0;
  for(i=0;i<eToken_last-1;++i) {
    mTotals[i]=0;
  }
#endif

}





nsTokenAllocator::~nsTokenAllocator() {

  MOZ_COUNT_DTOR(nsTokenAllocator);

}

class CTokenFinder: public nsDequeFunctor{
public:
  CTokenFinder(CToken* aToken) {mToken=aToken;}
  virtual void* operator()(void* anObject) {
    if(anObject==mToken) {
      return anObject;
    }
    return 0;
  }
  CToken* mToken;
};











CToken* nsTokenAllocator::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsAString& aString) {

  CToken* result=0;

#ifdef  DEBUG
    mTotals[aType-1]++;
#endif
  switch(aType){
    case eToken_start:            result=new(mArenaPool) CStartToken(aString, aTag); break;
    case eToken_end:              result=new(mArenaPool) CEndToken(aString, aTag); break;
    case eToken_comment:          result=new(mArenaPool) CCommentToken(aString); break;
    case eToken_entity:           result=new(mArenaPool) CEntityToken(aString); break;
    case eToken_whitespace:       result=new(mArenaPool) CWhitespaceToken(aString); break;
    case eToken_newline:          result=new(mArenaPool) CNewlineToken(); break;
    case eToken_text:             result=new(mArenaPool) CTextToken(aString); break;
    case eToken_attribute:        result=new(mArenaPool) CAttributeToken(aString); break;
    case eToken_instruction:      result=new(mArenaPool) CInstructionToken(aString); break;
    case eToken_cdatasection:     result=new(mArenaPool) CCDATASectionToken(aString); break;
    case eToken_doctypeDecl:      result=new(mArenaPool) CDoctypeDeclToken(aString); break;
    case eToken_markupDecl:       result=new(mArenaPool) CMarkupDeclToken(aString); break;
      default:
        NS_ASSERTION(false, "nsDTDUtils::CreateTokenOfType: illegal token type"); 
        break;
  }

  return result;
}










CToken* nsTokenAllocator::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag) {

  CToken* result=0;

#ifdef  DEBUG
    mTotals[aType-1]++;
#endif
  switch(aType){
    case eToken_start:            result=new(mArenaPool) CStartToken(aTag); break;
    case eToken_end:              result=new(mArenaPool) CEndToken(aTag); break;
    case eToken_comment:          result=new(mArenaPool) CCommentToken(); break;
    case eToken_attribute:        result=new(mArenaPool) CAttributeToken(); break;
    case eToken_entity:           result=new(mArenaPool) CEntityToken(); break;
    case eToken_whitespace:       result=new(mArenaPool) CWhitespaceToken(); break;
    case eToken_newline:          result=new(mArenaPool) CNewlineToken(); break;
    case eToken_text:             result=new(mArenaPool) CTextToken(); break;
    case eToken_instruction:      result=new(mArenaPool) CInstructionToken(); break;
    case eToken_cdatasection:     result=new(mArenaPool) CCDATASectionToken(aTag); break;
    case eToken_doctypeDecl:      result=new(mArenaPool) CDoctypeDeclToken(aTag); break;
    case eToken_markupDecl:       result=new(mArenaPool) CMarkupDeclToken(); break;
    default:
      NS_ASSERTION(false, "nsDTDUtils::CreateTokenOfType: illegal token type"); 
      break;
   }

  return result;
}

#ifdef DEBUG_TRACK_NODES 

static nsCParserNode* gAllNodes[100];
static int gAllNodeCount=0;

int FindNode(nsCParserNode *aNode) {
  int theIndex=0;
  for(theIndex=0;theIndex<gAllNodeCount;++theIndex) {
    if(gAllNodes[theIndex]==aNode) {
      return theIndex;
    }
  }
  return -1;
}

void AddNode(nsCParserNode *aNode) {
  if(-1==FindNode(aNode)) {
    gAllNodes[gAllNodeCount++]=aNode;
  }
  else {
    
  }
}

void RemoveNode(nsCParserNode *aNode) {
  int theIndex=FindNode(aNode);
  if(-1<theIndex) {
    gAllNodes[theIndex]=gAllNodes[--gAllNodeCount];
  }
}

#endif 


#ifdef HEAP_ALLOCATED_NODES
nsNodeAllocator::nsNodeAllocator():mSharedNodes(0){
#ifdef DEBUG_TRACK_NODES
  mCount=0;
#endif
#else 
nsNodeAllocator::nsNodeAllocator() {
#endif
  MOZ_COUNT_CTOR(nsNodeAllocator);
}

nsNodeAllocator::~nsNodeAllocator() {
  MOZ_COUNT_DTOR(nsNodeAllocator);

#ifdef HEAP_ALLOCATED_NODES
  nsCParserNode* theNode = 0;

  while((theNode=(nsCParserNode*)mSharedNodes.Pop())){
#ifdef DEBUG_TRACK_NODES
    RemoveNode(theNode);
#endif
    ::operator delete(theNode); 
    theNode=nullptr;
  }
#ifdef DEBUG_TRACK_NODES
  if(mCount) {
    printf("**************************\n");
    printf("%i out of %i nodes leaked!\n",gAllNodeCount,mCount);
    printf("**************************\n");
  }
#endif
#endif
}
  
nsCParserNode* nsNodeAllocator::CreateNode(CToken* aToken,  
                                           nsTokenAllocator* aTokenAllocator) 
{
  nsCParserNode* result = 0;
#ifdef HEAP_ALLOCATED_NODES
#if 0
  if(gAllNodeCount!=mSharedNodes.GetSize()) {
    int x=10; 
  }
#endif
  result = static_cast<nsCParserNode*>(mSharedNodes.Pop());
  if (result) {
    result->Init(aToken, aTokenAllocator,this);
  }
  else{
    result = nsCParserNode::Create(aToken, aTokenAllocator,this);
#ifdef DEBUG_TRACK_NODES
    ++mCount;
    AddNode(static_cast<nsCParserNode*>(result));
#endif
    IF_HOLD(result);
  }
#else
  eHTMLTokenTypes type = aToken ? eHTMLTokenTypes(aToken->GetTokenType()) : eToken_unknown;
  switch (type) {
    case eToken_start:
      result = nsCParserStartNode::Create(aToken, aTokenAllocator,this); 
      break;
    default :
      result = nsCParserNode::Create(aToken, aTokenAllocator,this); 
      break;
  }
  IF_HOLD(result);
#endif
  return result;
}

#ifdef DEBUG
void DebugDumpContainmentRules(nsIDTD& theDTD,const char* aFilename,const char* aTitle) {
}
#endif
