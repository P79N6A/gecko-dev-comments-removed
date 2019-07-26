





#include "nsParserNode.h"
#include "nsToken.h"








nsParserNode::nsParserNode(eHTMLTags aTag)
  : mTag(aTag)
{
}









nsParserNode::~nsParserNode() {
}










int32_t 
nsParserNode::GetNodeType(void) const
{
   return mTag;
}










int32_t 
nsParserNode::GetTokenType(void) const
{
  return eToken_start;
}









int32_t 
nsParserNode::GetAttributeCount() const
{
  return 0;
}









const nsAString&
nsParserNode::GetKeyAt(uint32_t anIndex) const
{
  return EmptyString();
}








const nsAString&
nsParserNode::GetValueAt(uint32_t anIndex) const
{
  return EmptyString();
}
