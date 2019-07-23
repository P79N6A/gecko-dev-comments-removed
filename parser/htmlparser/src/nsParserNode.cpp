





































#include "nsIAtom.h"
#include "nsParserNode.h" 
#include <string.h>
#include "nsHTMLTokens.h"
#include "nsITokenizer.h"
#include "nsDTDUtils.h"





nsCParserNode::nsCParserNode()
  : mRefCnt(0), mGenericState(PR_FALSE), mUseCount(0), mToken(nsnull),
    mTokenAllocator(nsnull)
{
  MOZ_COUNT_CTOR(nsCParserNode);
#ifdef HEAP_ALLOCATED_NODES
  mNodeAllocator = nsnull;
#endif
}








nsCParserNode::nsCParserNode(CToken* aToken,
                             nsTokenAllocator* aTokenAllocator,
                             nsNodeAllocator* aNodeAllocator)
  : mRefCnt(0), mGenericState(PR_FALSE), mUseCount(0), mToken(aToken),
    mTokenAllocator(aTokenAllocator)
{
  MOZ_COUNT_CTOR(nsCParserNode);

  static int theNodeCount = 0;
  ++theNodeCount;
  IF_HOLD(mToken);

#ifdef HEAP_ALLOCATED_NODES
  mNodeAllocator = aNodeAllocator;
#endif
}









nsCParserNode::~nsCParserNode() {
  MOZ_COUNT_DTOR(nsCParserNode);
  ReleaseAll();
#ifdef HEAP_ALLOCATED_NODES
  if(mNodeAllocator) {
    mNodeAllocator->Recycle(this);
  }
  mNodeAllocator = nsnull;
#endif
  mTokenAllocator = 0;
}










nsresult
nsCParserNode::Init(CToken* aToken,
                    nsTokenAllocator* aTokenAllocator,
                    nsNodeAllocator* aNodeAllocator) 
{
  mTokenAllocator = aTokenAllocator;
  mToken = aToken;
  IF_HOLD(mToken);
  mGenericState = PR_FALSE;
  mUseCount=0;
#ifdef HEAP_ALLOCATED_NODES
  mNodeAllocator = aNodeAllocator;
#endif
  return NS_OK;
}

void
nsCParserNode::AddAttribute(CToken* aToken) 
{
}









const nsAString&
nsCParserNode::GetTagName() const {
  return EmptyString();
}










const nsAString& 
nsCParserNode::GetText() const 
{
  if (mToken) {
    return mToken->GetStringValue();
  }
  return EmptyString();
}









PRInt32 
nsCParserNode::GetNodeType(void) const
{
  return (mToken) ? mToken->GetTypeID() : 0;
}










PRInt32 
nsCParserNode::GetTokenType(void) const
{
  return (mToken) ? mToken->GetTokenType() : 0;
}









PRInt32 
nsCParserNode::GetAttributeCount(PRBool askToken) const
{
  return 0;
}









const nsAString&
nsCParserNode::GetKeyAt(PRUint32 anIndex) const 
{
  return EmptyString();
}









const nsAString&
nsCParserNode::GetValueAt(PRUint32 anIndex) const 
{
  return EmptyString();
}

PRInt32 
nsCParserNode::TranslateToUnicodeStr(nsString& aString) const
{
  if (eToken_entity == mToken->GetTokenType()) {
    return ((CEntityToken*)mToken)->TranslateToUnicodeStr(aString);
  }
  return -1;
}







PRInt32
nsCParserNode::GetSourceLineNumber(void) const {
  return mToken ? mToken->GetLineNumber() : 0;
}







CToken* 
nsCParserNode::PopAttributeToken() {
  return 0;
}





void 
nsCParserNode::GetSource(nsString& aString) const
{
  eHTMLTags theTag = mToken ? (eHTMLTags)mToken->GetTypeID() : eHTMLTag_unknown;
  aString.Assign(PRUnichar('<'));
  const PRUnichar* theTagName = nsHTMLTags::GetStringValue(theTag);
  if(theTagName) {
    aString.Append(theTagName);
  }
  aString.Append(PRUnichar('>'));
}





nsresult 
nsCParserNode::ReleaseAll() 
{
  if(mTokenAllocator) {
    IF_FREE(mToken,mTokenAllocator);
  }
  return NS_OK;
}

nsresult 
nsCParserStartNode::Init(CToken* aToken,
                         nsTokenAllocator* aTokenAllocator,
                         nsNodeAllocator* aNodeAllocator) 
{
  NS_ASSERTION(mAttributes.GetSize() == 0, "attributes not recycled!");
  return nsCParserNode::Init(aToken, aTokenAllocator, aNodeAllocator);
}

void nsCParserStartNode::AddAttribute(CToken* aToken) 
{
  NS_ASSERTION(0 != aToken, "Error: Token shouldn't be null!");
  mAttributes.Push(aToken);
}

PRInt32 
nsCParserStartNode::GetAttributeCount(PRBool askToken) const
{
  PRInt32 result = 0;
  if (askToken) {
    result = mToken ? mToken->GetAttributeCount() : 0;
  }
  else {
    result = mAttributes.GetSize();
  }
  return result;
}

const nsAString&
nsCParserStartNode::GetKeyAt(PRUint32 anIndex) const 
{
  if ((PRInt32)anIndex < mAttributes.GetSize()) {
    CAttributeToken* attr = 
      static_cast<CAttributeToken*>(mAttributes.ObjectAt(anIndex));
    if (attr) {
      return attr->GetKey();
    }
  }
  return EmptyString();
}

const nsAString&
nsCParserStartNode::GetValueAt(PRUint32 anIndex) const 
{
  if (PRInt32(anIndex) < mAttributes.GetSize()) {
    CAttributeToken* attr = 
      static_cast<CAttributeToken*>(mAttributes.ObjectAt(anIndex));
    if (attr) {
      return attr->GetValue();
    }
  }
  return EmptyString();
}

CToken* 
nsCParserStartNode::PopAttributeToken() 
{
  return static_cast<CToken*>(mAttributes.Pop());
}

void nsCParserStartNode::GetSource(nsString& aString) const
{
  aString.Assign(PRUnichar('<'));
  const PRUnichar* theTagName = 
    nsHTMLTags::GetStringValue(nsHTMLTag(mToken->GetTypeID()));
  if (theTagName) {
    aString.Append(theTagName);
  }
  PRInt32 index;
  PRInt32 size = mAttributes.GetSize();
  for (index = 0 ; index < size; ++index) {
    CAttributeToken *theToken = 
      static_cast<CAttributeToken*>(mAttributes.ObjectAt(index));
    if (theToken) {
      theToken->AppendSourceTo(aString);
      aString.Append(PRUnichar(' ')); 
    }
  }
  aString.Append(PRUnichar('>'));
}

nsresult nsCParserStartNode::ReleaseAll() 
{
  NS_ASSERTION(0!=mTokenAllocator, "Error: no token allocator");
  CToken* theAttrToken;
  while ((theAttrToken = static_cast<CToken*>(mAttributes.Pop()))) {
    IF_FREE(theAttrToken, mTokenAllocator);
  }
  nsCParserNode::ReleaseAll();
  return NS_OK; 
}

