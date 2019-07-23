





































       
  
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






void nsEntryStack::EnsureCapacityFor(PRInt32 aNewMax,PRInt32 aShiftOffset) {
  if(mCapacity<aNewMax){ 

    const int kDelta=16;

    PRInt32 theSize = kDelta * ((aNewMax / kDelta) + 1);
    nsTagEntry* temp=new nsTagEntry[theSize]; 
    mCapacity=theSize;

    if(temp){ 
      PRInt32 index=0; 
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
                        PRBool aRefCntNode) 
{
  if(aNode) {
    EnsureCapacityFor(mCount+1);
    mEntries[mCount].mTag = (eHTMLTags)aNode->GetNodeType();
    if (aRefCntNode) {
      aNode->mUseCount++;
      mEntries[mCount].mNode = NS_CONST_CAST(nsCParserNode*,aNode);
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
  mEntries[mCount].mParent = nsnull;
  mEntries[mCount].mStyles = nsnull;
  ++mCount;
}







void nsEntryStack::PushFront(nsCParserNode* aNode,
                             nsEntryStack* aStyleStack, 
                             PRBool aRefCntNode) 
{
  if(aNode) {
    if(mCount<mCapacity) {
      PRInt32 index=0; 
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
      mEntries[0].mNode = NS_CONST_CAST(nsCParserNode*,aNode);
      IF_HOLD(mEntries[0].mNode);
    }
    mEntries[0].mParent=aStyleStack;
    mEntries[0].mStyles=0;
    ++mCount;
  }
}





void nsEntryStack::Append(nsEntryStack *aStack) {
  if(aStack) {

    PRInt32 theCount=aStack->mCount;

    EnsureCapacityFor(mCount+aStack->mCount,0);

    PRInt32 theIndex=0;
    for(theIndex=0;theIndex<theCount;++theIndex){
      mEntries[mCount]=aStack->mEntries[theIndex];
      mEntries[mCount++].mParent=0;
    }
  }
} 
 













nsCParserNode* nsEntryStack::Remove(PRInt32 anIndex,
                                    eHTMLTags aTag) 
{
  nsCParserNode* result = 0;
  if (0 < mCount && anIndex < mCount){
    result = mEntries[anIndex].mNode;
    if (result)
      result->mUseCount--;
    PRInt32 theIndex = 0;
    mCount -= 1;
    for( theIndex = anIndex; theIndex < mCount; ++theIndex){
      mEntries[theIndex] = mEntries[theIndex+1];
    }
    mEntries[mCount].mNode = 0;
    mEntries[mCount].mStyles = 0;
    nsEntryStack* theStyleStack = mEntries[anIndex].mParent;
    if (theStyleStack) {
      
      
      PRUint32 scount = theStyleStack->mCount;
#ifdef DEBUG_mrbkap
      NS_ASSERTION(scount != 0, "RemoveStyles has a bad style stack");
#endif
      nsTagEntry *theStyleEntry = theStyleStack->mEntries;
      for (PRUint32 sindex = scount-1;; --sindex) {            
        if (theStyleEntry->mTag == aTag) {
          
          theStyleEntry->mParent = nsnull;
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
      
      
      PRUint32 scount = theStyleStack->mCount;

      
      
#ifdef DEBUG_mrbkap
      NS_ASSERTION(scount != 0, "preventing a potential crash.");
#endif
      NS_ENSURE_TRUE(scount != 0, result);

      nsTagEntry *theStyleEntry = theStyleStack->mEntries;
      for (PRUint32 sindex = scount - 1;; --sindex) {
        if (theStyleEntry->mTag == mEntries[mCount].mTag) {
          
          theStyleEntry->mParent = nsnull;
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






nsCParserNode* nsEntryStack::NodeAt(PRInt32 anIndex) const 
{
  nsCParserNode* result=0;
  if((0<mCount) && (anIndex<mCount)) {
    result=mEntries[anIndex].mNode;
  }
  return result;
}






eHTMLTags nsEntryStack::TagAt(PRInt32 anIndex) const 
{
  eHTMLTags result=eHTMLTag_unknown;
  if((0<mCount) && (anIndex<mCount)) {
    result=mEntries[anIndex].mTag;
  }
  return result;
}





nsTagEntry* nsEntryStack::EntryAt(PRInt32 anIndex) const 
{
  nsTagEntry *result=0;
  if((0<mCount) && (anIndex<mCount)) {
    result=&mEntries[anIndex];
  }
  return result;
}







eHTMLTags nsEntryStack::operator[](PRInt32 anIndex) const 
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
                             PRBool aRefCntNode) 
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






PRBool nsDTDContext::HasOpenContainer(eHTMLTags aTag) const {
  PRInt32 theIndex=mStack.LastOf(aTag);
  return PRBool(-1<theIndex);
}





void nsDTDContext::Push(nsCParserNode* aNode,
                        nsEntryStack* aStyleStack, 
                        PRBool aRefCntNode) {
  if(aNode) {
#ifdef  NS_DEBUG
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
#ifdef NS_DEBUG
  if (mStack.mCount < eMaxTags) {
    mXTags[mStack.mCount] = aTag;
  }
#endif

  mStack.PushTag(aTag);
}

nsTagEntry*
nsDTDContext::PopEntry()
{
  PRInt32 theSize = mStack.mCount;
  if(0<theSize) {
#ifdef  NS_DEBUG
    if (theSize <= eMaxTags)
      mXTags[theSize-1]=eHTMLTag_unknown;
#endif
    return mStack.PopEntry();
  }
  return 0;
}

void nsDTDContext::PushEntry(nsTagEntry* aEntry, 
                             PRBool aRefCntNode)
{
#ifdef  NS_DEBUG
    int size=mStack.mCount;
    if(size< eMaxTags && aEntry)
      mXTags[size]=aEntry->mTag;
#endif
    mStack.PushEntry(aEntry, aRefCntNode);
}





void 
nsDTDContext::MoveEntries(nsDTDContext& aDest,
                          PRInt32 aCount)
{
  NS_ASSERTION(aCount > 0 && mStack.mCount >= aCount, "cannot move entries");
  if (aCount > 0 && mStack.mCount >= aCount) {
    while (aCount) {
      aDest.PushEntry(&mStack.mEntries[--mStack.mCount], PR_FALSE);
#ifdef  NS_DEBUG
      if (mStack.mCount < eMaxTags) {
        mXTags[mStack.mCount] = eHTMLTag_unknown;
      }
#endif
      --aCount;
    }
  }
}





nsCParserNode* nsDTDContext::Pop(nsEntryStack *&aChildStyleStack) {

  PRInt32         theSize=mStack.mCount;
  nsCParserNode*  result=0;

  if(0<theSize) {

#ifdef  NS_DEBUG
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





eHTMLTags nsDTDContext::TagAt(PRInt32 anIndex) const {
  return mStack.TagAt(anIndex);
}





nsTagEntry* nsDTDContext::LastEntry(void) const {
  return mStack.EntryAt(mStack.mCount-1);
}





eHTMLTags nsDTDContext::Last() const {
  return mStack.Last();
}






nsEntryStack* nsDTDContext::GetStylesAt(PRInt32 anIndex) const {
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

        PRUint32 scount=aStyles->mCount;
        PRUint32 sindex=0;

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

  PRInt32 theLevel=0;
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
  
  PRInt32 theLevel=mStack.mCount;
  
  while (theLevel) {
    nsEntryStack *theStack=GetStylesAt(--theLevel);
    if (theStack) {
      PRInt32 index=theStack->mCount;
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






static const size_t  kTokenBuckets[]       ={sizeof(CStartToken),sizeof(CAttributeToken),sizeof(CCommentToken),sizeof(CEndToken)};
static const PRInt32 kNumTokenBuckets      = sizeof(kTokenBuckets) / sizeof(size_t);
static const PRInt32 kInitialTokenPoolSize = NS_SIZE_IN_HEAP(sizeof(CToken)) * 200;






nsTokenAllocator::nsTokenAllocator() {

  MOZ_COUNT_CTOR(nsTokenAllocator);

  mArenaPool.Init("TokenPool", kTokenBuckets, kNumTokenBuckets, kInitialTokenPoolSize);

#ifdef NS_DEBUG
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

#ifdef  NS_DEBUG
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
        NS_ASSERTION(PR_FALSE, "nsDTDUtils::CreateTokenOfType: illegal token type"); 
        break;
  }

  return result;
}










CToken* nsTokenAllocator::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag) {

  CToken* result=0;

#ifdef  NS_DEBUG
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
      NS_ASSERTION(PR_FALSE, "nsDTDUtils::CreateTokenOfType: illegal token type"); 
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
  static const size_t  kNodeBuckets[]       = { sizeof(nsCParserNode), sizeof(nsCParserStartNode) };
  static const PRInt32 kNumNodeBuckets      = sizeof(kNodeBuckets) / sizeof(size_t);
  static const PRInt32 kInitialNodePoolSize = NS_SIZE_IN_HEAP(sizeof(nsCParserNode)) * 35; 
nsNodeAllocator::nsNodeAllocator() {
  mNodePool.Init("NodePool", kNodeBuckets, kNumNodeBuckets, kInitialNodePoolSize);
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
    theNode=nsnull;
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
  result = NS_STATIC_CAST(nsCParserNode*,mSharedNodes.Pop());
  if (result) {
    result->Init(aToken, aTokenAllocator,this);
  }
  else{
    result = nsCParserNode::Create(aToken, aTokenAllocator,this);
#ifdef DEBUG_TRACK_NODES
    ++mCount;
    AddNode(NS_STATIC_CAST(nsCParserNode*,result));
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






NS_IMPL_ISUPPORTS1(nsObserverEntry, nsIObserverEntry)

nsObserverEntry::nsObserverEntry(const nsAString& aTopic) : mTopic(aTopic) 
{
  memset(mObservers, 0, sizeof(mObservers));
}

nsObserverEntry::~nsObserverEntry() {
  for (PRInt32 i = 0; i <= NS_HTML_TAG_MAX; ++i){
    if (mObservers[i]) {
      PRInt32 count = mObservers[i]->Count();
      for (PRInt32 j = 0; j < count; ++j) {
        nsISupports* obs = (nsISupports*)mObservers[i]->ElementAt(j);
        NS_IF_RELEASE(obs);
      }
      delete mObservers[i];
    }
  }
}

NS_IMETHODIMP
nsObserverEntry::Notify(nsIParserNode* aNode,
                        nsIParser* aParser,
                        nsISupports* aWebShell,
                        const PRUint32 aFlags) 
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_ARG_POINTER(aParser);

  nsresult result = NS_OK;
  eHTMLTags theTag = (eHTMLTags)aNode->GetNodeType();
 
  if (theTag <= NS_HTML_TAG_MAX) {
    nsVoidArray*  theObservers = mObservers[theTag];
    if (theObservers) {
      PRInt32   theCharsetSource;
      nsCAutoString      charset;
      aParser->GetDocumentCharset(charset,theCharsetSource);
      NS_ConvertASCIItoUTF16 theCharsetValue(charset);

      PRInt32 theAttrCount = aNode->GetAttributeCount(); 
      PRInt32 theObserversCount = theObservers->Count(); 
      if (0 < theObserversCount){
        nsStringArray keys(theAttrCount+4), values(theAttrCount+4);

        
        
        
        
        PRInt32 index;
        for (index = 0; index < theAttrCount; ++index) {
          keys.AppendString(aNode->GetKeyAt(index));
          values.AppendString(aNode->GetValueAt(index));
        } 

        nsAutoString intValue;

        keys.AppendString(NS_LITERAL_STRING("charset")); 
        values.AppendString(theCharsetValue);       
      
        keys.AppendString(NS_LITERAL_STRING("charsetSource")); 
        intValue.AppendInt(PRInt32(theCharsetSource),10);
        values.AppendString(intValue); 

        keys.AppendString(NS_LITERAL_STRING("X_COMMAND"));
        values.AppendString(NS_LITERAL_STRING("text/html")); 

        nsCOMPtr<nsIChannel> channel;
        aParser->GetChannel(getter_AddRefs(channel));

        for (index=0;index<theObserversCount;++index) {
          nsIElementObserver* observer = NS_STATIC_CAST(nsIElementObserver*,theObservers->ElementAt(index));
          if (observer) {
            result = observer->Notify(aWebShell, channel,
                                      nsHTMLTags::GetStringValue(theTag),
                                      &keys, &values, aFlags);
            if (NS_FAILED(result)) {
              break;
            }

            if (result == NS_HTMLPARSER_VALID_META_CHARSET) {
              
              
              aParser->SetDocumentCharset(charset, kCharsetFromMetaTag);
              result = NS_OK;
            }
          }
        } 
      } 
    }
  }
  return result;
}

PRBool 
nsObserverEntry::Matches(const nsAString& aString) {
  PRBool result = aString.Equals(mTopic);
  return result;
}

nsresult
nsObserverEntry::AddObserver(nsIElementObserver *aObserver,
                             eHTMLTags aTag) 
{
  if (aObserver) {
    if (!mObservers[aTag]) {
      mObservers[aTag] = new nsAutoVoidArray();
      if (!mObservers[aTag]) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    NS_ADDREF(aObserver);
    mObservers[aTag]->AppendElement(aObserver);
  }
  return NS_OK;
}

void 
nsObserverEntry::RemoveObserver(nsIElementObserver *aObserver)
{
  for (PRInt32 i=0; i <= NS_HTML_TAG_MAX; ++i){
    if (mObservers[i]) {
      nsISupports* obs = aObserver;
      PRBool removed = mObservers[i]->RemoveElement(obs);
      if (removed) {
        NS_RELEASE(obs);
      }
    }
  }
}
