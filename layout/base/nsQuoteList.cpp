







































#include "nsQuoteList.h"
#include "nsReadableUtils.h"

const nsString*
nsQuoteNode::Text()
{
  NS_ASSERTION(mType == eStyleContentType_OpenQuote ||
               mType == eStyleContentType_CloseQuote,
               "should only be called when mText should be non-null");
  const nsStyleQuotes* styleQuotes = mPseudoFrame->GetStyleQuotes();
  PRInt32 quotesCount = styleQuotes->QuotesCount(); 
  PRInt32 quoteDepth = Depth();

  
  
  
  if (quoteDepth >= quotesCount)
    quoteDepth = quotesCount - 1;

  const nsString *result;
  if (quoteDepth == -1) {
    
    
    result = & EmptyString();
  } else {
    result = eStyleContentType_OpenQuote == mType
               ? styleQuotes->OpenQuoteAt(quoteDepth)
               : styleQuotes->CloseQuoteAt(quoteDepth);
  }
  return result;
}

void
nsQuoteList::Calc(nsQuoteNode* aNode)
{
  if (aNode == FirstNode()) {
    aNode->mDepthBefore = 0;
  } else {
    aNode->mDepthBefore = Prev(aNode)->DepthAfter();
  }
}

void
nsQuoteList::RecalcAll()
{
  nsQuoteNode *node = FirstNode();
  if (!node)
    return;

  do {
    PRInt32 oldDepth = node->mDepthBefore;
    Calc(node);

    if (node->mDepthBefore != oldDepth && node->mText)
      node->mText->SetData(*node->Text());

    
    node = Next(node);
  } while (node != FirstNode());
}

#ifdef DEBUG
void
nsQuoteList::PrintChain()
{
  printf("Chain: \n");
  if (!FirstNode()) {
    return;
  }
  nsQuoteNode* node = FirstNode();
  do {
    printf("  %p %d - ", static_cast<void*>(node), node->mDepthBefore);
    switch(node->mType) {
        case (eStyleContentType_OpenQuote):
          printf("open");
          break;
        case (eStyleContentType_NoOpenQuote):
          printf("noOpen");
          break;
        case (eStyleContentType_CloseQuote):
          printf("close");
          break;
        case (eStyleContentType_NoCloseQuote):
          printf("noClose");
          break;
        default:
          printf("unknown!!!");
    }
    printf(" %d - %d,", node->Depth(), node->DepthAfter());
    if (node->mText) {
      nsAutoString data;
      node->mText->GetData(data);
      printf(" \"%s\",", NS_ConvertUTF16toUTF8(data).get());
    }
    printf("\n");
    node = Next(node);
  } while (node != FirstNode());
}
#endif
