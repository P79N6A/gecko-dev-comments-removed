
















































#include "nsIAtom.h"
#include "nsHTMLTokenizer.h"
#include "nsScanner.h"
#include "nsElementTable.h"
#include "CParserContext.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"








NS_IMPL_ISUPPORTS1(nsHTMLTokenizer, nsITokenizer)








nsHTMLTokenizer::nsHTMLTokenizer(PRInt32 aParseMode,
                                 eParserDocType aDocType,
                                 eParserCommands aCommand,
                                 PRUint16 aFlags) :
  nsITokenizer(), mTokenDeque(0), mFlags(aFlags)
{
  if (aParseMode == eDTDMode_full_standards ||
      aParseMode == eDTDMode_almost_standards) {
    mFlags |= NS_IPARSER_FLAG_STRICT_MODE;
  } else if (aParseMode == eDTDMode_quirks)  {
    mFlags |= NS_IPARSER_FLAG_QUIRKS_MODE;
  } else if (aParseMode == eDTDMode_autodetect) {
    mFlags |= NS_IPARSER_FLAG_AUTO_DETECT_MODE;
  } else {
    mFlags |= NS_IPARSER_FLAG_UNKNOWN_MODE;
  }

  if (aDocType == ePlainText) {
    mFlags |= NS_IPARSER_FLAG_PLAIN_TEXT;
  } else if (aDocType == eXML) {
    mFlags |= NS_IPARSER_FLAG_XML;
  } else if (aDocType == eHTML_Quirks ||
             aDocType == eHTML3_Quirks ||
             aDocType == eHTML_Strict) {
    mFlags |= NS_IPARSER_FLAG_HTML;
  }
  
  mFlags |= aCommand == eViewSource
            ? NS_IPARSER_FLAG_VIEW_SOURCE
            : NS_IPARSER_FLAG_VIEW_NORMAL;

  NS_ASSERTION(!(mFlags & NS_IPARSER_FLAG_XML) || 
                (mFlags & NS_IPARSER_FLAG_VIEW_SOURCE),
              "Why isn't this XML document going through our XML parser?");

  mTokenAllocator = nsnull;
  mTokenScanPos = 0;
}




nsHTMLTokenizer::~nsHTMLTokenizer()
{
  if (mTokenDeque.GetSize()) {
    CTokenDeallocator theDeallocator(mTokenAllocator->GetArenaPool());
    mTokenDeque.ForEach(theDeallocator);
  }
}
 

















void
nsHTMLTokenizer::AddToken(CToken*& aToken,
                          nsresult aResult,
                          nsDeque* aDeque,
                          nsTokenAllocator* aTokenAllocator)
{
  if (aToken && aDeque) {
    if (NS_SUCCEEDED(aResult)) {
      aDeque->Push(aToken);
    } else {
      IF_FREE(aToken, aTokenAllocator);
    }
  }
}






nsTokenAllocator*
nsHTMLTokenizer::GetTokenAllocator()
{
  return mTokenAllocator;
}







CToken*
nsHTMLTokenizer::PeekToken()
{
  return (CToken*)mTokenDeque.PeekFront();
}







CToken*
nsHTMLTokenizer::PopToken()
{
  return (CToken*)mTokenDeque.PopFront();
}









CToken*
nsHTMLTokenizer::PushTokenFront(CToken* theToken)
{
  mTokenDeque.PushFront(theToken);
  return theToken;
}







CToken*
nsHTMLTokenizer::PushToken(CToken* theToken)
{
  mTokenDeque.Push(theToken);
  return theToken;
}






PRInt32
nsHTMLTokenizer::GetCount()
{
  return mTokenDeque.GetSize();
}









CToken*
nsHTMLTokenizer::GetTokenAt(PRInt32 anIndex)
{
  return (CToken*)mTokenDeque.ObjectAt(anIndex);
}










nsresult
nsHTMLTokenizer::WillTokenize(PRBool aIsFinalChunk,
                              nsTokenAllocator* aTokenAllocator)
{
  mTokenAllocator = aTokenAllocator;
  mIsFinalChunk = aIsFinalChunk;

  
  mTokenScanPos = mTokenDeque.GetSize();
  return NS_OK;
}







void
nsHTMLTokenizer::PrependTokens(nsDeque& aDeque)
{
  PRInt32 aCount = aDeque.GetSize();
  
  for (PRInt32 anIndex = 0; anIndex < aCount; ++anIndex) {
    CToken* theToken = (CToken*)aDeque.Pop();
    PushTokenFront(theToken);
  }
}









nsresult
nsHTMLTokenizer::CopyState(nsITokenizer* aTokenizer)
{
  if (aTokenizer) {
    mFlags = ((nsHTMLTokenizer*)aTokenizer)->mFlags;
  }

  return NS_OK;
}










static PRInt32
FindLastIndexOfTag(eHTMLTags aTag, nsDeque &aTagStack)
{
  PRInt32 theCount = aTagStack.GetSize();
  
  while (0 < theCount) {
    CHTMLToken* theToken = (CHTMLToken*)aTagStack.ObjectAt(--theCount);  
    if (theToken) {
      eHTMLTags theTag = (eHTMLTags)theToken->GetTypeID();
      if (theTag == aTag) {
        return theCount;
      }
    }
  }

  return kNotFound;
}










nsresult nsHTMLTokenizer::ScanDocStructure(PRBool aFinalChunk)
{
  nsresult result = NS_OK;
  if (!mTokenDeque.GetSize()) {
    return result;
  }

  CHTMLToken* theToken = (CHTMLToken*)mTokenDeque.ObjectAt(mTokenScanPos);

  
  while (mTokenScanPos > 0) {
    if (theToken) {
      eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());  
      if (theType == eToken_start &&
          theToken->GetContainerInfo() == eFormUnknown) {
        break;
      }
    }
    theToken = (CHTMLToken*)mTokenDeque.ObjectAt(--mTokenScanPos);
  }

  
  
  

  nsDeque       theStack(0);
  nsDeque       tempStack(0);
  PRInt32       theStackDepth = 0;
  
  static  const PRInt32 theMaxStackDepth = 200;

  while (theToken && theStackDepth < theMaxStackDepth) {
    eHTMLTokenTypes theType = eHTMLTokenTypes(theToken->GetTokenType());
    eHTMLTags       theTag  = (eHTMLTags)theToken->GetTypeID();

    if (nsHTMLElement::IsContainer(theTag)) { 
      PRBool theTagIsBlock  = gHTMLElements[theTag].IsMemberOf(kBlockEntity);
      PRBool theTagIsInline = theTagIsBlock
                              ? PR_FALSE
                              : gHTMLElements[theTag].IsMemberOf(kInlineEntity);

      if (theTagIsBlock || theTagIsInline || eHTMLTag_table == theTag) {
        switch(theType) {
          case eToken_start:
            {
              if (gHTMLElements[theTag].ShouldVerifyHierarchy()) {
                PRInt32 earlyPos = FindLastIndexOfTag(theTag, theStack);
                if (earlyPos != kNotFound) {
                  
                  
                  
                  
                  
                  
                  
                  
                  
                  
                  
                  nsDequeIterator it(theStack, earlyPos), end(theStack.End());
                  while (it < end) {
                    CHTMLToken *theMalformedToken = 
                        NS_STATIC_CAST(CHTMLToken*, it++);
                  
                    theMalformedToken->SetContainerInfo(eMalformed);
                  }
                }
              }

              theStack.Push(theToken);
              ++theStackDepth;
            }
            break;
          case eToken_end: 
            {
              CHTMLToken *theLastToken =
                NS_STATIC_CAST(CHTMLToken*, theStack.Peek());
              if (theLastToken) {
                if (theTag == theLastToken->GetTypeID()) {
                  theStack.Pop(); 
                  theStackDepth--;
                  theLastToken->SetContainerInfo(eWellFormed);
                } else {
                  
                  
                  

                  if (FindLastIndexOfTag(theTag, theStack) != kNotFound) {
                    
                    
                    theStack.Pop(); 
                    do {
                      theLastToken->SetContainerInfo(eMalformed);
                      tempStack.Push(theLastToken);
                      theLastToken = NS_STATIC_CAST(CHTMLToken*, theStack.Pop());
                    } while (theLastToken && theTag != theLastToken->GetTypeID());
                    
                    

                    NS_ASSERTION(theLastToken,
                                 "FindLastIndexOfTag lied to us!"
                                 " We couldn't find theTag on theStack");
                    theLastToken->SetContainerInfo(eMalformed);

                    
                    
                    
                    
                    while (tempStack.GetSize() != 0) {
                      theStack.Push(tempStack.Pop());
                    }
                  }
                }
              }
            }
            break;
          default:
            break; 
        }
      }
    }

    theToken = (CHTMLToken*)mTokenDeque.ObjectAt(++mTokenScanPos);
  }

  return result;
}







nsresult
nsHTMLTokenizer::DidTokenize(PRBool aFinalChunk)
{
  return ScanDocStructure(aFinalChunk);
}













nsresult
nsHTMLTokenizer::ConsumeToken(nsScanner& aScanner, PRBool& aFlushTokens)
{
  PRUnichar theChar;
  CToken* theToken = nsnull;

  nsresult result = aScanner.Peek(theChar);

  switch(result) {
    case kEOF:
      
      return result;

    case NS_OK:
    default:
      if (!(mFlags & NS_IPARSER_FLAG_PLAIN_TEXT)) {
        if (kLessThan == theChar) {
          return ConsumeTag(theChar, theToken, aScanner, aFlushTokens);
        } else if (kAmpersand == theChar) {
          return ConsumeEntity(theChar, theToken, aScanner);
        }
      }

      if (kCR == theChar || kLF == theChar) {
        return ConsumeNewline(theChar, theToken, aScanner);
      } else {
        if (!nsCRT::IsAsciiSpace(theChar)) {
          if (theChar != '\0') {
            result = ConsumeText(theToken, aScanner);
          } else {
            
            aScanner.GetChar(theChar);
          }
          break;
        }
        result = ConsumeWhitespace(theChar, theToken, aScanner);
      }
      break;
  }

  return result;
}














nsresult
nsHTMLTokenizer::ConsumeTag(PRUnichar aChar,
                            CToken*& aToken,
                            nsScanner& aScanner,
                            PRBool& aFlushTokens)
{
  PRUnichar theNextChar, oldChar;
  nsresult result = aScanner.Peek(aChar, 1);

  if (NS_OK == result) {
    switch (aChar) {
      case kForwardSlash:
        result = aScanner.Peek(theNextChar, 2);

        if (NS_OK == result) {
          
          aScanner.GetChar(oldChar);

          
          
          PRBool isXML = mFlags & NS_IPARSER_FLAG_XML;
          if (nsCRT::IsAsciiAlpha(theNextChar) ||
              kGreaterThan == theNextChar      ||
              (isXML && !nsCRT::IsAscii(theNextChar))) {
            result = ConsumeEndTag(aChar, aToken, aScanner);
          } else {
            result = ConsumeComment(aChar, aToken, aScanner);
          }
        }

        break;

      case kExclamation:
        result = aScanner.Peek(theNextChar, 2);

        if (NS_OK == result) {
          
          aScanner.GetChar(oldChar);

          if (kMinus == theNextChar || kGreaterThan == theNextChar) {
            result = ConsumeComment(aChar, aToken, aScanner);
          } else {
            result = ConsumeSpecialMarkup(aChar, aToken, aScanner);
          }
        }
        break;

      case kQuestionMark:
        
        
        aScanner.GetChar(oldChar);
        result = ConsumeProcessingInstruction(aChar, aToken, aScanner);
        break;

      default:
        
        PRBool isXML = mFlags & NS_IPARSER_FLAG_XML;
        if (nsCRT::IsAsciiAlpha(aChar) ||
            (isXML && !nsCRT::IsAscii(aChar))) {
          
          aScanner.GetChar(oldChar);
          result = ConsumeStartTag(aChar, aToken, aScanner, aFlushTokens);
        } else {
          
          
          result = ConsumeText(aToken, aScanner);
        }
    }
  }

  
  if (kEOF == result && !aScanner.IsIncremental()) {
    
    
    result = ConsumeText(aToken, aScanner);
  }

  return result;
}










nsresult
nsHTMLTokenizer::ConsumeAttributes(PRUnichar aChar,
                                   CToken* aToken,
                                   nsScanner& aScanner)
{
  PRBool done = PR_FALSE;
  nsresult result = NS_OK;
  PRInt16 theAttrCount = 0;

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();

  while (!done && result == NS_OK) {
    CAttributeToken* theToken =
      NS_STATIC_CAST(CAttributeToken*,
                     theAllocator->CreateTokenOfType(eToken_attribute,
                                                     eHTMLTag_unknown));
    if (NS_LIKELY(theToken != nsnull)) {
      
      result = theToken->Consume(aChar, aScanner, mFlags);

      if (NS_SUCCEEDED(result)) {
        ++theAttrCount;
        AddToken((CToken*&)theToken, result, &mTokenDeque, theAllocator);
      } else {
        IF_FREE(theToken, mTokenAllocator);
        
        if (NS_ERROR_HTMLPARSER_BADATTRIBUTE == result) {
          result = NS_OK;
        }
      }
    }
    else {
      result = NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef DEBUG
    if (NS_SUCCEEDED(result)) {
      PRInt32 newline = 0;
      aScanner.SkipWhitespace(newline);
      NS_ASSERTION(newline == 0,
          "CAttribute::Consume() failed to collect all the newlines!");
    }
#endif
    if (NS_SUCCEEDED(result)) {
      result = aScanner.Peek(aChar);
      if (NS_SUCCEEDED(result)) {
        if (aChar == kGreaterThan) { 
          aScanner.GetChar(aChar); 
          done = PR_TRUE;
        } else if (aChar == kLessThan) {
          aToken->SetInError(PR_TRUE);
          done = PR_TRUE;
        }
      }
    }
  }

  if (NS_FAILED(result)) {
    aToken->SetInError(PR_TRUE);

    if (!aScanner.IsIncremental()) {
      result = NS_OK;
    }
  }

  aToken->SetAttributeCount(theAttrCount);
  return result;
}












nsresult
nsHTMLTokenizer::ConsumeStartTag(PRUnichar aChar,
                                 CToken*& aToken,
                                 nsScanner& aScanner,
                                 PRBool& aFlushTokens)
{
  
  PRInt32 theDequeSize = mTokenDeque.GetSize();
  nsresult result = NS_OK;

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_start, eHTMLTag_unknown);
  NS_ENSURE_TRUE(aToken, NS_ERROR_OUT_OF_MEMORY);

  
  result = aToken->Consume(aChar, aScanner, mFlags);

  if (NS_SUCCEEDED(result)) {
    AddToken(aToken, result, &mTokenDeque, theAllocator);

    eHTMLTags theTag = (eHTMLTags)aToken->GetTypeID();

    
    
    result = aScanner.Peek(aChar);
    if (NS_FAILED(result)) {
      aToken->SetInError(PR_TRUE);

      
      
      result = NS_OK;
    } else {
      if (kGreaterThan != aChar) { 
        result = ConsumeAttributes(aChar, aToken, aScanner);
      } else {
        aScanner.GetChar(aChar);
      }
    }

    





    if (NS_SUCCEEDED(result) && !(mFlags & NS_IPARSER_FLAG_XML)) {
      PRBool isCDATA = gHTMLElements[theTag].CanContainType(kCDATA);
      PRBool isPCDATA = eHTMLTag_textarea == theTag ||
                        eHTMLTag_title    == theTag;

      
      
      if ((eHTMLTag_iframe == theTag &&
            (mFlags & NS_IPARSER_FLAG_FRAMES_ENABLED)) ||
          (eHTMLTag_noframes == theTag &&
            (mFlags & NS_IPARSER_FLAG_FRAMES_ENABLED)) ||
          (eHTMLTag_noscript == theTag &&
            (mFlags & NS_IPARSER_FLAG_SCRIPT_ENABLED)) ||
          (eHTMLTag_noembed == theTag)) {
        isCDATA = PR_TRUE;
      }

      
      
      if (eHTMLTag_plaintext == theTag) {
        isCDATA = PR_FALSE;

        
        
        mFlags |= NS_IPARSER_FLAG_PLAIN_TEXT;
      }


      if (isCDATA || isPCDATA) {
        PRBool done = PR_FALSE;
        nsDependentString endTagName(nsHTMLTags::GetStringValue(theTag)); 

        CToken* text =
            theAllocator->CreateTokenOfType(eToken_text, eHTMLTag_text);
        NS_ENSURE_TRUE(text, NS_ERROR_OUT_OF_MEMORY);

        CTextToken* textToken = NS_STATIC_CAST(CTextToken*, text);

        if (isCDATA) {
          result = textToken->ConsumeCharacterData(theTag != eHTMLTag_script,
                                                   aScanner,
                                                   endTagName,
                                                   mFlags,
                                                   done);

          
          
          aFlushTokens = done && theTag == eHTMLTag_script;
        } else if (isPCDATA) {
          
          
          result = textToken->ConsumeParsedCharacterData(
                                                  theTag == eHTMLTag_textarea,
                                                  theTag == eHTMLTag_title,
                                                  aScanner,
                                                  endTagName,
                                                  mFlags,
                                                  done);

          
        }

        
        
        if (kEOF != result) {
          AddToken(text, NS_OK, &mTokenDeque, theAllocator);
          CToken* endToken = nsnull;

          if (NS_SUCCEEDED(result) && done) {
            PRUnichar theChar;
            
            result = aScanner.GetChar(theChar);
            NS_ASSERTION(NS_SUCCEEDED(result) && theChar == kLessThan,
                         "CTextToken::Consume*Data is broken!");
#ifdef DEBUG
            
            PRUnichar tempChar;  
            result = aScanner.Peek(tempChar);
            NS_ASSERTION(NS_SUCCEEDED(result) && tempChar == kForwardSlash,
                         "CTextToken::Consume*Data is broken!");
#endif
            result = ConsumeEndTag(PRUnichar('/'), endToken, aScanner);
            if (!(mFlags & NS_IPARSER_FLAG_VIEW_SOURCE) &&
                NS_SUCCEEDED(result)) {
              
              
              
              
              
              endToken->SetInError(PR_FALSE);
            }
          } else if (result == kFakeEndTag &&
                    !(mFlags & NS_IPARSER_FLAG_VIEW_SOURCE)) {
            result = NS_OK;
            endToken = theAllocator->CreateTokenOfType(eToken_end, theTag,
                                                       endTagName);
            AddToken(endToken, result, &mTokenDeque, theAllocator);
            if (NS_LIKELY(endToken != nsnull)) {
              endToken->SetInError(PR_TRUE);
            }
            else {
              result = NS_ERROR_OUT_OF_MEMORY;
            }
          } else if (result == kFakeEndTag) {
            
            
            result = NS_OK;
          }
        } else {
          IF_FREE(text, mTokenAllocator);
        }
      }
    }

    
    
    
    
    
    
    if (NS_FAILED(result)) {
      while (mTokenDeque.GetSize()>theDequeSize) {
        CToken* theToken = (CToken*)mTokenDeque.Pop();
        IF_FREE(theToken, mTokenAllocator);
      }
    }
  } else {
    IF_FREE(aToken, mTokenAllocator);
  }

  return result;
}









nsresult
nsHTMLTokenizer::ConsumeEndTag(PRUnichar aChar,
                               CToken*& aToken,
                               nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_end, eHTMLTag_unknown);
  NS_ENSURE_TRUE(aToken, NS_ERROR_OUT_OF_MEMORY);

  
  PRInt32 theDequeSize = mTokenDeque.GetSize();
  nsresult result = NS_OK;

  
  result = aToken->Consume(aChar, aScanner, mFlags);
  AddToken(aToken, result, &mTokenDeque, theAllocator);
  if (NS_FAILED(result)) {
    
    
    
    return result;
  }

  result = aScanner.Peek(aChar);
  if (NS_FAILED(result)) {
    aToken->SetInError(PR_TRUE);

    
    
    
    return NS_OK;
  }

  if (kGreaterThan != aChar) {
    result = ConsumeAttributes(aChar, aToken, aScanner);
  } else {
    aScanner.GetChar(aChar);
  }

  
  
  
  if (NS_FAILED(result)) {
    while (mTokenDeque.GetSize() > theDequeSize) {
      CToken* theToken = (CToken*)mTokenDeque.Pop();
      IF_FREE(theToken, mTokenAllocator);
    }
  }

  return result;
}










nsresult
nsHTMLTokenizer::ConsumeEntity(PRUnichar aChar,
                               CToken*& aToken,
                               nsScanner& aScanner)
{
  PRUnichar  theChar;
  nsresult result = aScanner.Peek(theChar, 1);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  if (NS_SUCCEEDED(result)) {
    if (nsCRT::IsAsciiAlpha(theChar) || theChar == kHashsign) {
      aToken = theAllocator->CreateTokenOfType(eToken_entity, eHTMLTag_entity);
      NS_ENSURE_TRUE(aToken, NS_ERROR_OUT_OF_MEMORY);
      result = aToken->Consume(theChar, aScanner, mFlags);

      if (result == NS_HTMLTOKENS_NOT_AN_ENTITY) {
        IF_FREE(aToken, mTokenAllocator);
      } else {
        if (result == kEOF && !aScanner.IsIncremental()) {
          result = NS_OK; 
        }

        AddToken(aToken, result, &mTokenDeque, theAllocator);
        return result;
      }
    }

    
    result = ConsumeText(aToken, aScanner);
  } else if (result == kEOF && !aScanner.IsIncremental()) {
    
    result = ConsumeText(aToken, aScanner);
    if (aToken) {
      aToken->SetInError(PR_TRUE);
    }
  }

  return result;
}











nsresult
nsHTMLTokenizer::ConsumeWhitespace(PRUnichar aChar,
                                   CToken*& aToken,
                                   nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_whitespace,
                                           eHTMLTag_whitespace);
  nsresult result = NS_OK;
  if (aToken) {
    result = aToken->Consume(aChar, aScanner, mFlags);
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  return result;
}










nsresult
nsHTMLTokenizer::ConsumeComment(PRUnichar aChar,
                                CToken*& aToken,
                                nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_comment, eHTMLTag_comment);
  nsresult result = NS_OK;
  if (aToken) {
    result = aToken->Consume(aChar, aScanner, mFlags);
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  if (kNotAComment == result) {
    
    result = ConsumeText(aToken, aScanner);
  }

  return result;
}










 
nsresult
nsHTMLTokenizer::ConsumeText(CToken*& aToken, nsScanner& aScanner)
{
  nsresult result = NS_OK;
  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  CTextToken* theToken =
    (CTextToken*)theAllocator->CreateTokenOfType(eToken_text, eHTMLTag_text);
  if (theToken) {
    PRUnichar ch = '\0';
    result = theToken->Consume(ch, aScanner, mFlags);
    if (NS_FAILED(result)) {
      if (0 == theToken->GetTextLength()) {
        IF_FREE(aToken, mTokenAllocator);
        aToken = nsnull;
      } else {
        result = NS_OK;
      }
    }

    aToken = theToken;
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  return result;
}










nsresult
nsHTMLTokenizer::ConsumeSpecialMarkup(PRUnichar aChar,
                                      CToken*& aToken,
                                      nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsresult result = NS_OK;
  nsAutoString theBufCopy;
  aScanner.Peek(theBufCopy, 20);
  ToUpperCase(theBufCopy);
  PRInt32 theIndex = theBufCopy.Find("DOCTYPE", PR_FALSE, 0, 0);
  nsTokenAllocator* theAllocator = this->GetTokenAllocator();

  if (theIndex == kNotFound) {
    if ('[' == theBufCopy.CharAt(0)) {
      aToken = theAllocator->CreateTokenOfType(eToken_cdatasection,
                                               eHTMLTag_comment);
    } else if (StringBeginsWith(theBufCopy, NS_LITERAL_STRING("ELEMENT")) ||
               StringBeginsWith(theBufCopy, NS_LITERAL_STRING("ATTLIST")) ||
               StringBeginsWith(theBufCopy, NS_LITERAL_STRING("ENTITY"))  ||
               StringBeginsWith(theBufCopy, NS_LITERAL_STRING("NOTATION"))) {
      aToken = theAllocator->CreateTokenOfType(eToken_markupDecl,
                                               eHTMLTag_markupDecl);
    } else {
      aToken = theAllocator->CreateTokenOfType(eToken_comment,
                                               eHTMLTag_comment);
    }
  } else {
    aToken = theAllocator->CreateTokenOfType(eToken_doctypeDecl,
                                             eHTMLTag_doctypeDecl);
  }

  if (aToken) {
    result = aToken->Consume(aChar, aScanner, mFlags);
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  if (result == kNotAComment) {
    result = ConsumeText(aToken, aScanner);
  }

  return result;
}









nsresult
nsHTMLTokenizer::ConsumeNewline(PRUnichar aChar,
                                CToken*& aToken,
                                nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_newline, eHTMLTag_newline);
  nsresult result = NS_OK;
  if (aToken) {
    result = aToken->Consume(aChar, aScanner, mFlags);
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  return result;
}










nsresult
nsHTMLTokenizer::ConsumeProcessingInstruction(PRUnichar aChar,
                                              CToken*& aToken,
                                              nsScanner& aScanner)
{
  
  aScanner.GetChar(aChar);

  nsTokenAllocator* theAllocator = this->GetTokenAllocator();
  aToken = theAllocator->CreateTokenOfType(eToken_instruction,
                                           eHTMLTag_unknown);
  nsresult result = NS_OK;
  if (aToken) {
    result = aToken->Consume(aChar, aScanner, mFlags);
    AddToken(aToken, result, &mTokenDeque, theAllocator);
  }

  return result;
}
