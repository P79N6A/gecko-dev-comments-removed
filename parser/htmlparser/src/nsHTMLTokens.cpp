






































#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include "nsScanner.h"
#include "nsToken.h"
#include "nsIAtom.h"
#include "nsHTMLTokens.h"
#include "prtypes.h"
#include "nsDebug.h"
#include "nsHTMLTags.h"
#include "nsHTMLEntities.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsScanner.h"


static const PRUnichar sUserdefined[] = {'u', 's', 'e', 'r', 'd', 'e', 'f',
                                         'i', 'n', 'e', 'd', 0};

static const PRUnichar kAttributeTerminalChars[] = {
  PRUnichar('&'), PRUnichar('\b'), PRUnichar('\t'),
  PRUnichar('\n'), PRUnichar('\r'), PRUnichar(' '),
  PRUnichar('>'),
  PRUnichar(0)
};

static void AppendNCR(nsSubstring& aString, PRInt32 aNCRValue);










static nsresult
ConsumeEntity(nsScannerSharedSubstring& aString,
              nsScanner& aScanner,
              PRBool aIECompatible,
              PRInt32 aFlag)
{
  nsresult result = NS_OK;

  PRUnichar ch;
  result = aScanner.Peek(ch, 1);

  if (NS_SUCCEEDED(result)) {
    PRUnichar amp = 0;
    PRInt32 theNCRValue = 0;
    nsAutoString entity;

    if (nsCRT::IsAsciiAlpha(ch) && !(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      result = CEntityToken::ConsumeEntity(ch, entity, aScanner);
      if (NS_SUCCEEDED(result)) {
        theNCRValue = nsHTMLEntities::EntityToUnicode(entity);
        PRUnichar theTermChar = entity.Last();
        
        
        
        

        nsSubstring &writable = aString.writable();
        if (theNCRValue < 0 ||
            (aIECompatible && theNCRValue > 255 && theTermChar != ';')) {
          
          writable.Append(kAmpersand);
          writable.Append(entity);
        } else {
          
          writable.Append(PRUnichar(theNCRValue));
        }
      }
    } else if (ch == kHashsign && !(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      result = CEntityToken::ConsumeEntity(ch, entity, aScanner);
      if (NS_SUCCEEDED(result)) {
        nsSubstring &writable = aString.writable();
        if (result == NS_HTMLTOKENS_NOT_AN_ENTITY) {
          
          aScanner.GetChar(amp);
          writable.Append(amp);
          result = NS_OK;
        } else {
          PRInt32 err;
          theNCRValue = entity.ToInteger(&err, kAutoDetect);
          AppendNCR(writable, theNCRValue);
        }
      }
    } else {
      
      aScanner.GetChar(amp);
      aString.writable().Append(amp);
    }
  }

  return result;
}



















static nsresult
ConsumeUntil(nsScannerSharedSubstring& aString,
             PRInt32& aNewlineCount,
             nsScanner& aScanner,
             const nsReadEndCondition& aEndCondition,
             PRBool aAllowNewlines,
             PRBool aIECompatEntities,
             PRInt32 aFlag)
{
  nsresult result = NS_OK;
  PRBool   done = PR_FALSE;

  do {
    result = aScanner.ReadUntil(aString, aEndCondition, PR_FALSE);
    if (NS_SUCCEEDED(result)) {
      PRUnichar ch;
      aScanner.Peek(ch);
      if (ch == kAmpersand) {
        result = ConsumeEntity(aString, aScanner, aIECompatEntities, aFlag);
      } else if (ch == kCR && aAllowNewlines) {
        aScanner.GetChar(ch);
        result = aScanner.Peek(ch);
        if (NS_SUCCEEDED(result)) {
          nsSubstring &writable = aString.writable();
          if (ch == kNewLine) {
            writable.AppendLiteral("\r\n");
            aScanner.GetChar(ch);
          } else {
            writable.Append(PRUnichar('\r'));
          }
          ++aNewlineCount;
        }
      } else if (ch == kNewLine && aAllowNewlines) {
        aScanner.GetChar(ch);
        aString.writable().Append(PRUnichar('\n'));
        ++aNewlineCount;
      } else {
        done = PR_TRUE;
      }
    }
  } while (NS_SUCCEEDED(result) && !done);

  return result;
}








CHTMLToken::CHTMLToken(eHTMLTags aTag)
  : CToken(aTag)
{
}


CHTMLToken::~CHTMLToken()
{
}




CStartToken::CStartToken(eHTMLTags aTag)
  : CHTMLToken(aTag)
{
  mEmpty = PR_FALSE;
  mContainerInfo = eFormUnknown;
#ifdef DEBUG
  mAttributed = PR_FALSE;
#endif
}

CStartToken::CStartToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_unknown)
{
  mEmpty = PR_FALSE;
  mContainerInfo = eFormUnknown;
  mTextValue.Assign(aName);
#ifdef DEBUG
  mAttributed = PR_FALSE;
#endif
}

CStartToken::CStartToken(const nsAString& aName, eHTMLTags aTag)
  : CHTMLToken(aTag)
{
  mEmpty = PR_FALSE;
  mContainerInfo = eFormUnknown;
  mTextValue.Assign(aName);
#ifdef DEBUG
  mAttributed = PR_FALSE;
#endif
}




PRInt32
CStartToken::GetTypeID()
{
  if (eHTMLTag_unknown == mTypeID) {
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
  }
  return mTypeID;
}

PRInt32
CStartToken::GetTokenType()
{
  return eToken_start;
}

void
CStartToken::SetEmpty(PRBool aValue)
{
  mEmpty = aValue;
}

PRBool
CStartToken::IsEmpty()
{
  return mEmpty;
}




nsresult
CStartToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  
  
  
  

  nsresult result = NS_OK;
  nsScannerSharedSubstring tagIdent;

  if (aFlag & NS_IPARSER_FLAG_HTML) {
    result = aScanner.ReadTagIdentifier(tagIdent);
    mTypeID = (PRInt32)nsHTMLTags::LookupTag(tagIdent.str());
    
    
    if (eHTMLTag_userdefined == mTypeID ||
        (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      mTextValue = tagIdent.str();
    }
  } else {
    result = aScanner.ReadTagIdentifier(tagIdent);
    mTextValue = tagIdent.str();
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
  }

  if (NS_SUCCEEDED(result) && !(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
    result = aScanner.SkipWhitespace(mNewlineCount);
  }

  if (kEOF == result && !aScanner.IsIncremental()) {
    
    result = NS_OK;
  }

  return result;
}

const nsSubstring&
CStartToken::GetStringValue()
{
  if (eHTMLTag_unknown < mTypeID && mTypeID < eHTMLTag_text) {
    if (!mTextValue.Length()) {
      mTextValue.Assign(nsHTMLTags::GetStringValue((nsHTMLTag) mTypeID));
    }
  }
  return mTextValue;
}

void
CStartToken::GetSource(nsString& anOutputString)
{
  anOutputString.Truncate();
  AppendSourceTo(anOutputString);
}

void
CStartToken::AppendSourceTo(nsAString& anOutputString)
{
  anOutputString.Append(PRUnichar('<'));
  


  if (!mTextValue.IsEmpty()) {
    anOutputString.Append(mTextValue);
  } else {
    anOutputString.Append(GetTagName(mTypeID));
  }

  anOutputString.Append(PRUnichar('>'));
}

CEndToken::CEndToken(eHTMLTags aTag)
  : CHTMLToken(aTag)
{
}

CEndToken::CEndToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_unknown)
{
  mTextValue.Assign(aName);
}

CEndToken::CEndToken(const nsAString& aName, eHTMLTags aTag)
  : CHTMLToken(aTag)
{
  mTextValue.Assign(aName);
}

nsresult
CEndToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  nsresult result = NS_OK;
  nsScannerSharedSubstring tagIdent;

  if (aFlag & NS_IPARSER_FLAG_HTML) {
    result = aScanner.ReadTagIdentifier(tagIdent);

    mTypeID = (PRInt32)nsHTMLTags::LookupTag(tagIdent.str());
    
    
    if (eHTMLTag_userdefined == mTypeID ||
        (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      mTextValue = tagIdent.str();
    }
  } else {
    result = aScanner.ReadTagIdentifier(tagIdent);
    mTextValue = tagIdent.str();
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
  }

  if (NS_SUCCEEDED(result) && !(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
    result = aScanner.SkipWhitespace(mNewlineCount);
  }

  if (kEOF == result && !aScanner.IsIncremental()) {
    
    result = NS_OK;
  }

  return result;
}







PRInt32
CEndToken::GetTypeID()
{
  if (eHTMLTag_unknown == mTypeID) {
    mTypeID = nsHTMLTags::LookupTag(mTextValue);
    switch (mTypeID) {
      case eHTMLTag_dir:
      case eHTMLTag_menu:
        mTypeID = eHTMLTag_ul;
        break;

      default:
        break;
    }
  }

  return mTypeID;
}

PRInt32
CEndToken::GetTokenType()
{
  return eToken_end;
}

const nsSubstring&
CEndToken::GetStringValue()
{
  if (eHTMLTag_unknown < mTypeID && mTypeID < eHTMLTag_text) {
    if (!mTextValue.Length()) {
      mTextValue.Assign(nsHTMLTags::GetStringValue((nsHTMLTag) mTypeID));
    }
  }
  return mTextValue;
}

void
CEndToken::GetSource(nsString& anOutputString)
{
  anOutputString.Truncate();
  AppendSourceTo(anOutputString);
}

void
CEndToken::AppendSourceTo(nsAString& anOutputString)
{
  anOutputString.AppendLiteral("</");
  if (!mTextValue.IsEmpty()) {
    anOutputString.Append(mTextValue);
  } else {
    anOutputString.Append(GetTagName(mTypeID));
  }

  anOutputString.Append(PRUnichar('>'));
}

CTextToken::CTextToken()
  : CHTMLToken(eHTMLTag_text)
{
}

CTextToken::CTextToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_text)
{
  mTextValue.Rebind(aName);
}

PRInt32
CTextToken::GetTokenType()
{
  return eToken_text;
}

PRInt32
CTextToken::GetTextLength()
{
  return mTextValue.Length();
}

nsresult
CTextToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  static const PRUnichar theTerminalsChars[] =
    { PRUnichar('\n'), PRUnichar('\r'), PRUnichar('&'), PRUnichar('<'),
      PRUnichar(0) };
  static const nsReadEndCondition theEndCondition(theTerminalsChars);
  nsresult  result = NS_OK;
  PRBool    done = PR_FALSE;
  nsScannerIterator origin, start, end;

  
  
  aScanner.CurrentPosition(origin);
  start = origin;
  aScanner.EndReading(end);

  NS_ASSERTION(start != end, "Calling CTextToken::Consume when already at the "
                             "end of a document is a bad idea.");

  aScanner.SetPosition(++start);

  while (NS_OK == result && !done) {
    result = aScanner.ReadUntil(start, end, theEndCondition, PR_FALSE);
    if (NS_OK == result) {
      result = aScanner.Peek(aChar);

      if (NS_OK == result && (kCR == aChar || kNewLine == aChar)) {
        switch (aChar) {
          case kCR:
          {
            
            
            
            
            PRUnichar theNextChar;
            result = aScanner.Peek(theNextChar, 1);

            if (result == kEOF && aScanner.IsIncremental()) {
              break;
            }

            if (NS_SUCCEEDED(result)) {
              
              aScanner.GetChar(aChar);
            }

            if (kLF == theNextChar) {
              
              
              end.advance(2);
              aScanner.GetChar(theNextChar);
            } else {
              
              
              aScanner.ReplaceCharacter(end, kLF);
              ++end;
            }
            ++mNewlineCount;
            break;
          }
          case kLF:
            aScanner.GetChar(aChar);
            ++end;
            ++mNewlineCount;
            break;
        }
      } else {
        done = PR_TRUE;
      }
    }
  }

  
  
  
  aScanner.BindSubstring(mTextValue, origin, end);

  return result;
}
















nsresult
CTextToken::ConsumeCharacterData(PRBool aIgnoreComments,
                                 nsScanner& aScanner,
                                 const nsAString& aEndTagName,
                                 PRInt32 aFlag,
                                 PRBool& aFlushTokens)
{
  nsresult result = NS_OK;
  nsScannerIterator theStartOffset, theCurrOffset, theTermStrPos,
                    theStartCommentPos, theAltTermStrPos, endPos;
  PRBool        done = PR_FALSE;
  PRBool        theLastIteration = PR_FALSE;

  aScanner.CurrentPosition(theStartOffset);
  theCurrOffset = theStartOffset;
  aScanner.EndReading(endPos);
  theTermStrPos = theStartCommentPos = theAltTermStrPos = endPos;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_NAMED_LITERAL_STRING(ltslash, "</");
  const nsString theTerminalString = ltslash + aEndTagName;

  PRUint32 termStrLen = theTerminalString.Length();
  while (result == NS_OK && !done) {
    PRBool found = PR_FALSE;
    nsScannerIterator gtOffset, ltOffset = theCurrOffset;
    while (FindCharInReadable(PRUnichar(kLessThan), ltOffset, endPos) &&
           ((PRUint32)ltOffset.size_forward() >= termStrLen ||
            Distance(ltOffset, endPos) >= termStrLen)) {
      
      

      nsScannerIterator start(ltOffset), end(ltOffset);
      end.advance(termStrLen);

      if (CaseInsensitiveFindInReadable(theTerminalString, start, end) &&
          (end == endPos || (*end == '>'  || *end == ' '  ||
                             *end == '\t' || *end == '\n' ||
                             *end == '\r' || *end == '\b'))) {
        gtOffset = end;
        
        
        if ((end == endPos && aIgnoreComments) ||
            FindCharInReadable(PRUnichar(kGreaterThan), gtOffset, endPos)) {
          found = PR_TRUE;
          theTermStrPos = start;
        }
        break;
      }
      ltOffset.advance(1);
    }

    if (found && theTermStrPos != endPos) {
      if (!(aFlag & NS_IPARSER_FLAG_STRICT_MODE) &&
          !theLastIteration && !aIgnoreComments) {
        nsScannerIterator endComment(ltOffset);
        endComment.advance(5);

        if ((theStartCommentPos == endPos) &&
            FindInReadable(NS_LITERAL_STRING("<!--"), theCurrOffset,
                           endComment)) {
          theStartCommentPos = theCurrOffset;
        }

        if (theStartCommentPos != endPos) {
          
          theCurrOffset = theStartCommentPos;
          nsScannerIterator terminal(theTermStrPos);
          if (!RFindInReadable(NS_LITERAL_STRING("-->"),
                               theCurrOffset, terminal)) {
            
            
            
            if (theAltTermStrPos == endPos) {
              
              theAltTermStrPos = theTermStrPos;
            }

            
            theCurrOffset = theTermStrPos;
            theCurrOffset.advance(termStrLen);
            continue;
          }
        }
      }

      aScanner.BindSubstring(mTextValue, theStartOffset, theTermStrPos);
      aScanner.SetPosition(ltOffset);

      
      aFlushTokens = PR_TRUE;
      done = PR_TRUE;
    } else {
      
      
      
      if (!aScanner.IsIncremental()) {
        if (theAltTermStrPos != endPos) {
          
          
          theCurrOffset = theAltTermStrPos;
          theLastIteration = PR_TRUE;
        } else {
          
          done = PR_TRUE; 
          result = kFakeEndTag;
          aScanner.BindSubstring(mTextValue, theStartOffset, endPos);
          aScanner.SetPosition(endPos);
        }
      } else {
        result = kEOF;
      }
    }
  }

  if (result == NS_OK) {
    mNewlineCount = mTextValue.CountChar(kNewLine);
  }

  return result;
}














nsresult
CTextToken::ConsumeParsedCharacterData(PRBool aDiscardFirstNewline,
                                       PRBool aConservativeConsume,
                                       nsScanner& aScanner,
                                       const nsAString& aEndTagName,
                                       PRInt32 aFlag,
                                       PRBool& aFound)
{
  
  
  
  
  
  
  
  
  

  static const PRUnichar terminalChars[] = {
    PRUnichar('\r'), PRUnichar('\n'), PRUnichar('&'), PRUnichar('<'),
    PRUnichar(0)
  };
  static const nsReadEndCondition theEndCondition(terminalChars);

  nsScannerIterator currPos, endPos, altEndPos;
  PRUint32 truncPos = 0;
  aScanner.CurrentPosition(currPos);
  aScanner.EndReading(endPos);

  altEndPos = endPos;

  nsScannerSharedSubstring theContent;
  PRUnichar ch = 0;

  NS_NAMED_LITERAL_STRING(commentStart, "<!--");
  NS_NAMED_LITERAL_STRING(ltslash, "</");
  const nsString theTerminalString = ltslash + aEndTagName;
  PRUint32 termStrLen = theTerminalString.Length();
  PRUint32 commentStartLen = commentStart.Length();

  nsresult result = NS_OK;

  
  
  do {
    result = ConsumeUntil(theContent, mNewlineCount, aScanner,
                          theEndCondition, PR_TRUE, PR_FALSE, aFlag);

    if (aDiscardFirstNewline &&
        (NS_SUCCEEDED(result) || !aScanner.IsIncremental()) &&
        !(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      
      
      
      
      
      
      const nsSubstring &firstChunk = theContent.str();
      if (!firstChunk.IsEmpty()) {
        PRUint32 where = 0;
        PRUnichar newline = firstChunk.First();

        if (newline == kCR || newline == kNewLine) {
          ++where;

          if (firstChunk.Length() > 1) {
            if (newline == kCR && firstChunk.CharAt(1) == kNewLine) {
              
              ++where;
            }
            
          }
        }

        if (where != 0) {
          theContent.writable() = Substring(firstChunk, where);
        }
      }
    }
    aDiscardFirstNewline = PR_FALSE;

    if (NS_FAILED(result)) {
      if (kEOF == result && !aScanner.IsIncremental()) {
        aFound = PR_TRUE; 
        result = kFakeEndTag;

        if (aConservativeConsume && altEndPos != endPos) {
          
          
          theContent.writable().Truncate(truncPos);
          aScanner.SetPosition(altEndPos, PR_FALSE, PR_TRUE);
        }
        
        mTextValue.Rebind(theContent.str());
      } else {
        aFound = PR_FALSE;
      }

      return result;
    }

    aScanner.CurrentPosition(currPos);
    aScanner.GetChar(ch); 

    if (ch == kLessThan && altEndPos == endPos) {
      
      altEndPos = currPos;
      truncPos = theContent.str().Length();
    }

    if (Distance(currPos, endPos) >= termStrLen) {
      nsScannerIterator start(currPos), end(currPos);
      end.advance(termStrLen);

      if (CaseInsensitiveFindInReadable(theTerminalString, start, end)) {
        if (end != endPos && (*end == '>'  || *end == ' '  ||
                              *end == '\t' || *end == '\n' ||
                              *end == '\r' || *end == '\b')) {
          aFound = PR_TRUE;
          mTextValue.Rebind(theContent.str());

          
          
          
          
          aScanner.SetPosition(currPos, PR_FALSE, PR_TRUE);
          break;
        }
      }
    }
    
    if (Distance(currPos, endPos) >= commentStartLen) {
      nsScannerIterator start(currPos), end(currPos);
      end.advance(commentStartLen);

      if (CaseInsensitiveFindInReadable(commentStart, start, end)) {
        CCommentToken consumer; 

        
        aScanner.SetPosition(currPos.advance(2));

        
        
        result = consumer.Consume(*currPos, aScanner,
                                  (aFlag & ~NS_IPARSER_FLAG_QUIRKS_MODE) |
                                   NS_IPARSER_FLAG_STRICT_MODE);
        if (kEOF == result) {
          
          return kEOF;
        } else if (kNotAComment == result) {
          
          aScanner.CurrentPosition(currPos);
          aScanner.SetPosition(currPos.advance(1));
        } else {
          consumer.AppendSourceTo(theContent.writable());
          mNewlineCount += consumer.GetNewlineCount();
          continue;
        }
      }
    }

    result = kEOF;
    
    
    theContent.writable().Append(ch);
  } while (currPos != endPos);

  return result;
}

void
CTextToken::CopyTo(nsAString& aStr)
{
  nsScannerIterator start, end;
  mTextValue.BeginReading(start);
  mTextValue.EndReading(end);
  CopyUnicodeTo(start, end, aStr);
}

const nsSubstring& CTextToken::GetStringValue()
{
  return mTextValue.AsString();
}

void
CTextToken::Bind(nsScanner* aScanner, nsScannerIterator& aStart,
                 nsScannerIterator& aEnd)
{
  aScanner->BindSubstring(mTextValue, aStart, aEnd);
}

void
CTextToken::Bind(const nsAString& aStr)
{
  mTextValue.Rebind(aStr);
}

CCDATASectionToken::CCDATASectionToken(eHTMLTags aTag)
  : CHTMLToken(aTag)
{
}

CCDATASectionToken::CCDATASectionToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_unknown)
{
  mTextValue.Assign(aName);
}

PRInt32
CCDATASectionToken::GetTokenType()
{
  return eToken_cdatasection;
}









nsresult
CCDATASectionToken::Consume(PRUnichar aChar, nsScanner& aScanner,
                            PRInt32 aFlag)
{
  static const PRUnichar theTerminalsChars[] =
  { PRUnichar('\r'), PRUnichar('\n'), PRUnichar(']'), PRUnichar(0) };
  static const nsReadEndCondition theEndCondition(theTerminalsChars);
  nsresult  result = NS_OK;
  PRBool    done = PR_FALSE;

  while (NS_OK == result && !done) {
    result = aScanner.ReadUntil(mTextValue, theEndCondition, PR_FALSE);
    if (NS_OK == result) {
      result = aScanner.Peek(aChar);
      if (kCR == aChar && NS_OK == result) {
        result = aScanner.GetChar(aChar); 
        result = aScanner.Peek(aChar);    
        if (NS_OK == result) {
          switch(aChar) {
            case kCR:
              result = aScanner.GetChar(aChar); 
              mTextValue.AppendLiteral("\n\n");
              mNewlineCount += 2;
              break;

            case kNewLine:
              
              result = aScanner.GetChar(aChar); 

              
            default:
              mTextValue.AppendLiteral("\n");
              mNewlineCount++;
              break;
          }
        }
      } else if (kNewLine == aChar) {
        result = aScanner.GetChar(aChar);
        mTextValue.Append(aChar);
        ++mNewlineCount;
      } else if (kRightSquareBracket == aChar) {
        PRBool canClose = PR_FALSE;
        result = aScanner.GetChar(aChar); 
        mTextValue.Append(aChar);
        result = aScanner.Peek(aChar);    
        if (NS_OK == result && kRightSquareBracket == aChar) {
          result = aScanner.GetChar(aChar); 
          mTextValue.Append(aChar);
          canClose = PR_TRUE;
        }

        
        
        
        
        
        
        
        
        
        PRBool inCDATA = (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) &&
          StringBeginsWith(mTextValue, NS_LITERAL_STRING("[CDATA["));
        if (inCDATA) {
          
          
          while (true) {
            result = aScanner.Peek(aChar);
            if (result != NS_OK || aChar != kRightSquareBracket) {
              break;
            }

            mTextValue.Append(aChar);
            aScanner.GetChar(aChar);
          }
        } else {
          nsAutoString dummy; 
          result = aScanner.ReadUntil(dummy, kGreaterThan, PR_FALSE);
        }
        if (NS_OK == result &&
            (!inCDATA || (canClose && kGreaterThan == aChar))) {
          result = aScanner.GetChar(aChar); 
          done = PR_TRUE;
        }
      } else {
        done = PR_TRUE;
      }
    }
  }

  if (kEOF == result && !aScanner.IsIncremental()) {
    
    
    
    
    mInError = PR_TRUE;
    result = NS_OK;
  }

  return result;
}

const nsSubstring&
CCDATASectionToken::GetStringValue()
{
  return mTextValue;
}


CMarkupDeclToken::CMarkupDeclToken()
  : CHTMLToken(eHTMLTag_markupDecl)
{
}

CMarkupDeclToken::CMarkupDeclToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_markupDecl)
{
  mTextValue.Rebind(aName);
}

PRInt32
CMarkupDeclToken::GetTokenType()
{
  return eToken_markupDecl;
}










nsresult
CMarkupDeclToken::Consume(PRUnichar aChar, nsScanner& aScanner,
                          PRInt32 aFlag)
{
  static const PRUnichar theTerminalsChars[] =
    { PRUnichar('\n'), PRUnichar('\r'), PRUnichar('\''), PRUnichar('"'),
      PRUnichar('>'),
      PRUnichar(0) };
  static const nsReadEndCondition theEndCondition(theTerminalsChars);
  nsresult  result = NS_OK;
  PRBool    done = PR_FALSE;
  PRUnichar quote = 0;

  nsScannerIterator origin, start, end;
  aScanner.CurrentPosition(origin);
  start = origin;

  while (NS_OK == result && !done) {
    aScanner.SetPosition(start);
    result = aScanner.ReadUntil(start, end, theEndCondition, PR_FALSE);
    if (NS_OK == result) {
      result = aScanner.Peek(aChar);

      if (NS_OK == result) {
        PRUnichar theNextChar = 0;
        if (kCR == aChar || kNewLine == aChar) {
          result = aScanner.GetChar(aChar); 
          result = aScanner.Peek(theNextChar); 
        }
        switch(aChar) {
          case kCR:
            
            if (kLF == theNextChar) {
              
              
              end.advance(2);
              result = aScanner.GetChar(theNextChar);
            } else {
              
              
              aScanner.ReplaceCharacter(end, kLF);
              ++end;
            }
            ++mNewlineCount;
            break;
          case kLF:
            ++end;
            ++mNewlineCount;
            break;
          case '\'':
          case '"':
            ++end;
            if (quote) {
              if (quote == aChar) {
                quote = 0;
              }
            } else {
              quote = aChar;
            }
            break;
          case kGreaterThan:
            if (quote) {
              ++end;
            } else {
              start = end;
              
              ++start;
              aScanner.SetPosition(start); 
              done = PR_TRUE;
            }
            break;
          default:
            NS_ABORT_IF_FALSE(0, "should not happen, switch is missing cases?");
            break;
        }
        start = end;
      } else {
        done = PR_TRUE;
      }
    }
  }
  aScanner.BindSubstring(mTextValue, origin, end);

  if (kEOF == result) {
    mInError = PR_TRUE;
    if (!aScanner.IsIncremental()) {
      
      result = NS_OK;
    }
  }

  return result;
}

const nsSubstring&
CMarkupDeclToken::GetStringValue()
{
  return mTextValue.AsString();
}


CCommentToken::CCommentToken()
  : CHTMLToken(eHTMLTag_comment)
{
}

CCommentToken::CCommentToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_comment)
{
  mComment.Rebind(aName);
}

void
CCommentToken::AppendSourceTo(nsAString& anOutputString)
{
  AppendUnicodeTo(mCommentDecl, anOutputString);
}

static PRBool
IsCommentEnd(const nsScannerIterator& aCurrent, const nsScannerIterator& aEnd,
             nsScannerIterator& aGt)
{
  nsScannerIterator current = aCurrent;
  PRInt32 dashes = 0;

  while (current != aEnd && dashes != 2) {
    if (*current == kGreaterThan) {
      aGt = current;
      return PR_TRUE;
    }
    if (*current == PRUnichar('-')) {
      ++dashes;
    } else {
      dashes = 0;
    }
    ++current;
  }

  return PR_FALSE;
}

nsresult
CCommentToken::ConsumeStrictComment(nsScanner& aScanner)
{
  
  




  nsScannerIterator end, current, gt, lt;
  aScanner.EndReading(end);
  aScanner.CurrentPosition(current);

  nsScannerIterator beginData = end;

  lt = current;
  lt.advance(-2); 

  current.advance(-1);

  
  if (*current == kExclamation &&
      ++current != end && *current == kMinus &&
      ++current != end && *current == kMinus &&
      ++current != end) {
    nsScannerIterator currentEnd = end;
    PRBool balancedComment = PR_FALSE;
    NS_NAMED_LITERAL_STRING(dashes, "--");
    beginData = current;

    while (FindInReadable(dashes, current, currentEnd)) {
      current.advance(2);

      balancedComment = !balancedComment; 

      if (balancedComment && IsCommentEnd(current, end, gt)) {
        
        current.advance(-2);
        
        
        aScanner.BindSubstring(mComment, beginData, current);
        aScanner.BindSubstring(mCommentDecl, lt, ++gt);
        aScanner.SetPosition(gt);
        return NS_OK;
      }

      
      currentEnd = end;
    }
  }

  
  if (beginData == end) {
    
    
    
    aScanner.CurrentPosition(current);
    beginData = current;
    if (FindCharInReadable('>', current, end)) {
      aScanner.BindSubstring(mComment, beginData, current);
      aScanner.BindSubstring(mCommentDecl, lt, ++current);
      aScanner.SetPosition(current);
      return NS_OK;
    }
  }

  if (aScanner.IsIncremental()) {
    
    
    
    
    
    
    return kEOF;
  }

  
  aScanner.SetPosition(lt, PR_FALSE, PR_TRUE);
  return kNotAComment;
}

nsresult
CCommentToken::ConsumeQuirksComment(nsScanner& aScanner)
{
  
  




  nsScannerIterator end, current;
  aScanner.EndReading(end);
  aScanner.CurrentPosition(current);
  nsScannerIterator beginData = current,
                    beginLastMinus = end,
                    bestAltCommentEnd = end,
                    lt = current;
  lt.advance(-2); 

  
  
  if (current != end && *current == kMinus) {
    beginLastMinus = current;
    ++current;
    ++beginData;
    if (current != end && *current == kMinus) { 
      beginLastMinus = current;
      ++current;
      ++beginData;
      

      nsScannerIterator currentEnd = end, gt = end;

      
      while (FindCharInReadable(kGreaterThan, current, currentEnd)) {
        gt = current;
        if (bestAltCommentEnd == end) {
          bestAltCommentEnd = gt;
        }
        --current;
        PRBool goodComment = PR_FALSE;
        if (current != beginLastMinus && *current == kMinus) { 
          --current;
          if (current != beginLastMinus && *current == kMinus) { 
            goodComment = PR_TRUE;
            --current;
          }
        } else if (current != beginLastMinus && *current == '!') {
          --current;
          if (current != beginLastMinus && *current == kMinus) {
            --current;
            if (current != beginLastMinus && *current == kMinus) { 
              --current;
              goodComment = PR_TRUE;
            }
          }
        } else if (current == beginLastMinus) {
          goodComment = PR_TRUE;
        }

        if (goodComment) {
          
          aScanner.BindSubstring(mComment, beginData, ++current);
          aScanner.BindSubstring(mCommentDecl, lt, ++gt);
          aScanner.SetPosition(gt);
          return NS_OK;
        } else {
          
          current = ++gt;
          currentEnd = end;
        }
      }

      if (aScanner.IsIncremental()) {
        
        
        
        
        
        
        return kEOF;
      }

      
      
      
      
      
      
      
      gt = bestAltCommentEnd;
      aScanner.BindSubstring(mComment, beginData, gt);
      if (gt != end) {
        ++gt;
      }
      aScanner.BindSubstring(mCommentDecl, lt, gt);
      aScanner.SetPosition(gt);
      return NS_OK;
    }
  }

  
  
  current = beginData;
  if (FindCharInReadable(kGreaterThan, current, end)) {
    nsScannerIterator gt = current;
    if (current != beginData) {
      --current;
      if (current != beginData && *current == kMinus) { 
        --current;
        if (current != beginData && *current == kMinus) { 
          --current;
        }
      } else if (current != beginData && *current == '!') { 
        --current;
        if (current != beginData && *current == kMinus) { 
          --current;
          if (current != beginData && *current == kMinus) { 
            --current;
          }
        }
      }
    }

    if (current != gt) {
      aScanner.BindSubstring(mComment, beginData, ++current);
    } else {
      
      
      aScanner.BindSubstring(mComment, beginData, current);
    }
    aScanner.BindSubstring(mCommentDecl, lt, ++gt);
    aScanner.SetPosition(gt);
    return NS_OK;
  }

  if (!aScanner.IsIncremental()) {
    
    aScanner.SetPosition(lt, PR_FALSE, PR_TRUE);
    return kNotAComment;
  }

  
  return kEOF;
}









nsresult
CCommentToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  nsresult result = PR_TRUE;

  if (aFlag & NS_IPARSER_FLAG_STRICT_MODE) {
    
    result = ConsumeStrictComment(aScanner);
  } else {
    result = ConsumeQuirksComment(aScanner);
  }

  if (NS_SUCCEEDED(result)) {
    mNewlineCount = mCommentDecl.CountChar(kNewLine);
  }

  return result;
}

const nsSubstring&
CCommentToken::GetStringValue()
{
  return mComment.AsString();
}

PRInt32
CCommentToken::GetTokenType()
{
  return eToken_comment;
}

CNewlineToken::CNewlineToken()
  : CHTMLToken(eHTMLTag_newline)
{
}

PRInt32
CNewlineToken::GetTokenType()
{
  return eToken_newline;
}

static nsScannerSubstring* gNewlineStr;
void
CNewlineToken::AllocNewline()
{
  gNewlineStr = new nsScannerSubstring(NS_LITERAL_STRING("\n"));
}

void
CNewlineToken::FreeNewline()
{
  if (gNewlineStr) {
    delete gNewlineStr;
    gNewlineStr = nsnull;
  }
}






const nsSubstring&
CNewlineToken::GetStringValue()
{
  return gNewlineStr->AsString();
}








nsresult
CNewlineToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  







  nsresult rv = NS_OK;
  if (aChar == kCR) {
    PRUnichar theChar;
    rv = aScanner.Peek(theChar);
    if (theChar == kNewLine) {
      rv = aScanner.GetChar(theChar);
    } else if (rv == kEOF && !aScanner.IsIncremental()) {
      
      rv = NS_OK;
    }
  }

  mNewlineCount = 1;
  return rv;
}

CAttributeToken::CAttributeToken()
  : CHTMLToken(eHTMLTag_unknown)
{
  mHasEqualWithoutValue = PR_FALSE;
}




CAttributeToken::CAttributeToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_unknown)
{
  mTextValue.writable().Assign(aName);
  mHasEqualWithoutValue = PR_FALSE;
}




CAttributeToken::CAttributeToken(const nsAString& aKey, const nsAString& aName)
  : CHTMLToken(eHTMLTag_unknown)
{
  mTextValue.writable().Assign(aName);
  mTextKey.Rebind(aKey);
  mHasEqualWithoutValue = PR_FALSE;
}

PRInt32
CAttributeToken::GetTokenType()
{
  return eToken_attribute;
}

const nsSubstring&
CAttributeToken::GetStringValue()
{
  return mTextValue.str();
}

void
CAttributeToken::GetSource(nsString& anOutputString)
{
  anOutputString.Truncate();
  AppendSourceTo(anOutputString);
}

void
CAttributeToken::AppendSourceTo(nsAString& anOutputString)
{
  AppendUnicodeTo(mTextKey, anOutputString);
  if (mTextValue.str().Length() || mHasEqualWithoutValue) {
    anOutputString.AppendLiteral("=");
  }
  anOutputString.Append(mTextValue.str());
  
}





static nsresult
ConsumeQuotedString(PRUnichar aChar,
                    nsScannerSharedSubstring& aString,
                    PRInt32& aNewlineCount,
                    nsScanner& aScanner,
                    PRInt32 aFlag)
{
  NS_ASSERTION(aChar == kQuote || aChar == kApostrophe,
               "char is neither quote nor apostrophe");
  
  PRUint32 origLen = aString.str().Length();

  static const PRUnichar theTerminalCharsQuote[] = {
    PRUnichar(kQuote), PRUnichar('&'), PRUnichar(kCR),
    PRUnichar(kNewLine), PRUnichar(0) };
  static const PRUnichar theTerminalCharsApostrophe[] = {
    PRUnichar(kApostrophe), PRUnichar('&'), PRUnichar(kCR),
    PRUnichar(kNewLine), PRUnichar(0) };
  static const nsReadEndCondition
    theTerminateConditionQuote(theTerminalCharsQuote);
  static const nsReadEndCondition
    theTerminateConditionApostrophe(theTerminalCharsApostrophe);

  
  const nsReadEndCondition *terminateCondition = &theTerminateConditionQuote;
  if (aChar == kApostrophe) {
    terminateCondition = &theTerminateConditionApostrophe;
  }

  nsresult result = NS_OK;
  nsScannerIterator theOffset;
  aScanner.CurrentPosition(theOffset);

  result = ConsumeUntil(aString, aNewlineCount, aScanner,
                      *terminateCondition, PR_TRUE, PR_TRUE, aFlag);

  if (NS_SUCCEEDED(result)) {
    result = aScanner.GetChar(aChar); 
  }

  
  
  
  if (!aString.str().IsEmpty() && aString.str().Last() != aChar &&
      !aScanner.IsIncremental() && result == kEOF) {
    static const nsReadEndCondition
      theAttributeTerminator(kAttributeTerminalChars);
    aString.writable().Truncate(origLen);
    aScanner.SetPosition(theOffset, PR_FALSE, PR_TRUE);
    result = ConsumeUntil(aString, aNewlineCount, aScanner,
                          theAttributeTerminator, PR_FALSE, PR_TRUE, aFlag);
    if (NS_SUCCEEDED(result) && (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      
      result = NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL;
    }
  }
  return result;
}













static nsresult
ConsumeInvalidAttribute(nsScanner& aScanner,
                        PRUnichar aChar,
                        nsScannerIterator& aCurrent,
                        PRInt32& aNewlineCount)
{
  NS_ASSERTION(aChar == kApostrophe || aChar == kQuote || aChar == kForwardSlash,
               "aChar must be a quote or apostrophe");
  nsScannerIterator end, wsbeg;
  aScanner.EndReading(end);

  while (aCurrent != end && *aCurrent == aChar) {
    ++aCurrent;
  }

  aScanner.SetPosition(aCurrent);
  return aScanner.ReadWhitespace(wsbeg, aCurrent, aNewlineCount);
}




nsresult
CAttributeToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  nsresult result;
  nsScannerIterator wsstart, wsend;

  if (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) {
    result = aScanner.ReadWhitespace(wsstart, wsend, mNewlineCount);
    if (kEOF == result && wsstart != wsend) {
      
      
      aScanner.BindSubstring(mTextKey, wsstart, wsend);
    }
  } else {
    result = aScanner.SkipWhitespace(mNewlineCount);
  }

  if (NS_OK == result) {
    static const PRUnichar theTerminalsChars[] =
    { PRUnichar(' '), PRUnichar('"'),
      PRUnichar('='), PRUnichar('\n'),
      PRUnichar('\r'), PRUnichar('\t'),
      PRUnichar('>'), PRUnichar('<'),
      PRUnichar('\b'), PRUnichar('\''),
      PRUnichar('/'), PRUnichar(0) };
    static const nsReadEndCondition theEndCondition(theTerminalsChars);

    nsScannerIterator start, end;
    result = aScanner.ReadUntil(start, end, theEndCondition, PR_FALSE);

    if (!(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
      aScanner.BindSubstring(mTextKey, start, end);
    } else if (kEOF == result && wsstart != end) {
      
      
      aScanner.BindSubstring(mTextKey, wsstart, end);
    }

    
    if (NS_OK == result) {
      if (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) {
        result = aScanner.ReadWhitespace(start, wsend, mNewlineCount);
        aScanner.BindSubstring(mTextKey, wsstart, wsend);
      } else {
        result = aScanner.SkipWhitespace(mNewlineCount);
      }

      if (NS_OK == result) {
        
        result = aScanner.Peek(aChar);
        if (NS_OK == result) {
          if (kEqual == aChar) {
            result = aScanner.GetChar(aChar);  
            if (NS_OK == result) {
              if (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) {
                PRBool haveCR;
                result = aScanner.ReadWhitespace(mTextValue, mNewlineCount,
                                                 haveCR);
              } else {
                result = aScanner.SkipWhitespace(mNewlineCount);
              }

              if (NS_OK == result) {
                result = aScanner.Peek(aChar);  
                if (NS_OK == result) {
                  if (kQuote == aChar || kApostrophe == aChar) {
                    aScanner.GetChar(aChar);
                    if (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) {
                      mTextValue.writable().Append(aChar);
                    }

                    result = ConsumeQuotedString(aChar, mTextValue,
                                                 mNewlineCount, aScanner,
                                                 aFlag);
                    if (NS_SUCCEEDED(result) &&
                        (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
                      mTextValue.writable().Append(aChar);
                    } else if (result ==
                                NS_ERROR_HTMLPARSER_UNTERMINATEDSTRINGLITERAL) {
                      result = NS_OK;
                      mInError = PR_TRUE;
                    }
                    
                    
                    
                    
                    
                    
                  } else if (kGreaterThan == aChar) {
                    mHasEqualWithoutValue = PR_TRUE;
                    mInError = PR_TRUE;
                  } else {
                    static const nsReadEndCondition
                      theAttributeTerminator(kAttributeTerminalChars);
                    result =
                      ConsumeUntil(mTextValue,
                                   mNewlineCount,
                                   aScanner,
                                   theAttributeTerminator,
                                   PR_FALSE,
                                   PR_TRUE,
                                   aFlag);
                  }
                }
                if (NS_OK == result) {
                  if (aFlag & NS_IPARSER_FLAG_VIEW_SOURCE) {
                    PRBool haveCR;
                    result = aScanner.ReadWhitespace(mTextValue, mNewlineCount,
                                                     haveCR);
                  } else {
                    result = aScanner.SkipWhitespace(mNewlineCount);
                  }
                }
              } else {
                
                mHasEqualWithoutValue = PR_TRUE;
                mInError = PR_TRUE;
              }
            }
          } else {
            
            
            

            
            
            
            
            
            
            if (kQuote == aChar || kApostrophe == aChar ||
                kForwardSlash == aChar) {
              
              if (kForwardSlash != aChar || !(aFlag & NS_IPARSER_FLAG_XML)) {
                mInError = PR_TRUE;
              }

              if (!(aFlag & NS_IPARSER_FLAG_VIEW_SOURCE)) {
                result = aScanner.SkipOver(aChar); 
                if (NS_SUCCEEDED(result)) {
                  result = aScanner.SkipWhitespace(mNewlineCount);
                }
              } else {
                
                
                
                result = ConsumeInvalidAttribute(aScanner, aChar,
                                                 wsend, mNewlineCount);

                aScanner.BindSubstring(mTextKey, wsstart, wsend);
                aScanner.SetPosition(wsend);
              }
            }
          }
        }
      }
    }

    if (NS_OK == result) {
      if (mTextValue.str().Length() == 0 && mTextKey.Length() == 0 &&
          mNewlineCount == 0 && !mHasEqualWithoutValue) {
        
        
        
        
        return NS_ERROR_HTMLPARSER_BADATTRIBUTE;
      }
    }
  }

  if (kEOF == result && !aScanner.IsIncremental()) {
    
    
    
    if (mTextKey.Length() == 0) {
      result = NS_ERROR_HTMLPARSER_BADATTRIBUTE;
    } else {
      result = NS_OK;
    }
  }

  return result;
}

void
CAttributeToken::SetKey(const nsAString& aKey)
{
  mTextKey.Rebind(aKey);
}

void
CAttributeToken::BindKey(nsScanner* aScanner,
                         nsScannerIterator& aStart,
                         nsScannerIterator& aEnd)
{
  aScanner->BindSubstring(mTextKey, aStart, aEnd);
}

CWhitespaceToken::CWhitespaceToken()
  : CHTMLToken(eHTMLTag_whitespace)
{
}

CWhitespaceToken::CWhitespaceToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_whitespace)
{
  mTextValue.writable().Assign(aName);
}

PRInt32 CWhitespaceToken::GetTokenType()
{
  return eToken_whitespace;
}









nsresult
CWhitespaceToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  
  
  

  nsScannerIterator start;
  aScanner.CurrentPosition(start);
  aScanner.SetPosition(--start, PR_FALSE, PR_TRUE);

  PRBool haveCR;

  nsresult result = aScanner.ReadWhitespace(mTextValue, mNewlineCount, haveCR);

  if (result == kEOF && !aScanner.IsIncremental()) {
    
    
    result = NS_OK;
  }

  if (NS_OK == result && haveCR) {
    mTextValue.writable().StripChar(kCR);
  }
  return result;
}

const nsSubstring&
CWhitespaceToken::GetStringValue()
{
  return mTextValue.str();
}

CEntityToken::CEntityToken()
  : CHTMLToken(eHTMLTag_entity)
{
}

CEntityToken::CEntityToken(const nsAString& aName)
  : CHTMLToken(eHTMLTag_entity)
{
  mTextValue.Assign(aName);
}









nsresult
CEntityToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  nsresult result = ConsumeEntity(aChar, mTextValue, aScanner);
  return result;
}

PRInt32
CEntityToken::GetTokenType()
{
  return eToken_entity;
}










nsresult
CEntityToken::ConsumeEntity(PRUnichar aChar,
                            nsString& aString,
                            nsScanner& aScanner)
{
  nsresult result = NS_OK;
  if (kLeftBrace == aChar) {
    
    aScanner.GetChar(aChar); 

    PRInt32 rightBraceCount = 0;
    PRInt32 leftBraceCount  = 0;

    do {
      result = aScanner.GetChar(aChar);

      if (NS_FAILED(result)) {
        return result;
      }

      aString.Append(aChar);
      if (aChar == kRightBrace) {
        ++rightBraceCount;
      } else if (aChar == kLeftBrace) {
        ++leftBraceCount;
      }
    } while (leftBraceCount != rightBraceCount);
  } else {
    PRUnichar theChar = 0;
    if (kHashsign == aChar) {
      result = aScanner.Peek(theChar, 2);

      if (NS_FAILED(result)) {
        if (kEOF == result && !aScanner.IsIncremental()) {
          
          
          
          return NS_HTMLTOKENS_NOT_AN_ENTITY;
        }
        return result;
      }

      if (nsCRT::IsAsciiDigit(theChar)) {
        aScanner.GetChar(aChar); 
        aScanner.GetChar(aChar); 
        aString.Assign(aChar);
        result = aScanner.ReadNumber(aString, 10);
      } else if (theChar == 'x' || theChar == 'X') {
        aScanner.GetChar(aChar);   
        aScanner.GetChar(aChar);   
        aScanner.GetChar(theChar); 
        aString.Assign(aChar);
        aString.Append(theChar);
        result = aScanner.ReadNumber(aString, 16);
      } else {
        return NS_HTMLTOKENS_NOT_AN_ENTITY;
      }
    } else {
      result = aScanner.Peek(theChar, 1);

      if (NS_FAILED(result)) {
        return result;
      }

      if (nsCRT::IsAsciiAlpha(theChar) ||
        theChar == '_' ||
        theChar == ':') {
        aScanner.GetChar(aChar); 
        result = aScanner.ReadEntityIdentifier(aString);
      } else {
        return NS_HTMLTOKENS_NOT_AN_ENTITY;
      }
    }
  }

  if (NS_FAILED(result)) {
    return result;
  }

  result = aScanner.Peek(aChar);

  if (NS_FAILED(result)) {
    return result;
  }

  if (aChar == kSemicolon) {
    
    aString.Append(aChar);
    result = aScanner.GetChar(aChar);
  }

  return result;
}





#define NOT_USED 0xfffd

static const PRUint16 PA_HackTable[] = {
	0x20ac,  
	NOT_USED,
	0x201a,  
	0x0192,  
	0x201e,  
	0x2026,  
	0x2020,  
	0x2021,  
	0x02c6,  
	0x2030,  
	0x0160,  
	0x2039,  
	0x0152,  
	NOT_USED,
	0x017D,  
	NOT_USED,
	NOT_USED,
	0x2018,  
	0x2019,  
	0x201c,  
	0x201d,  
	0x2022,  
	0x2013,  
	0x2014,  
	0x02dc,  
	0x2122,  
	0x0161,  
	0x203a,  
	0x0153,  
	NOT_USED,
	0x017E,  
	0x0178   
};

static void
AppendNCR(nsSubstring& aString, PRInt32 aNCRValue)
{
  
  if (aNCRValue >= 0x0080 && aNCRValue <= 0x009f) {
    aNCRValue = PA_HackTable[aNCRValue - 0x0080];
  }

  AppendUCS4ToUTF16(ENSURE_VALID_CHAR(aNCRValue), aString);
}








PRInt32
CEntityToken::TranslateToUnicodeStr(nsString& aString)
{
  PRInt32 value = 0;

  if (mTextValue.Length() > 1) {
    PRUnichar theChar0 = mTextValue.CharAt(0);

    if (kHashsign == theChar0) {
      PRInt32 err = 0;

      value = mTextValue.ToInteger(&err, kAutoDetect);

      if (0 == err) {
        AppendNCR(aString, value);
      }
    } else {
      value = nsHTMLEntities::EntityToUnicode(mTextValue);
      if (-1 < value) {
        
        aString.Assign(PRUnichar(value));
      }
    }
  }

  return value;
}


const
nsSubstring& CEntityToken::GetStringValue()
{
  return mTextValue;
}

void
CEntityToken::GetSource(nsString& anOutputString)
{
  anOutputString.AppendLiteral("&");
  anOutputString += mTextValue;
  
}

void
CEntityToken::AppendSourceTo(nsAString& anOutputString)
{
  anOutputString.AppendLiteral("&");
  anOutputString += mTextValue;
  
}

const PRUnichar*
GetTagName(PRInt32 aTag)
{
  const PRUnichar *result = nsHTMLTags::GetStringValue((nsHTMLTag) aTag);

  if (result) {
    return result;
  }

  if (aTag >= eHTMLTag_userdefined) {
    return sUserdefined;
  }

  return 0;
}


CInstructionToken::CInstructionToken()
  : CHTMLToken(eHTMLTag_instruction)
{
}

CInstructionToken::CInstructionToken(const nsAString& aString)
  : CHTMLToken(eHTMLTag_unknown)
{
  mTextValue.Assign(aString);
}

nsresult
CInstructionToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  mTextValue.AssignLiteral("<?");
  nsresult result = NS_OK;
  PRBool done = PR_FALSE;

  while (NS_OK == result && !done) {
    
    result = aScanner.ReadUntil(mTextValue, kGreaterThan, PR_FALSE);
    if (NS_SUCCEEDED(result)) {
      
      
      if (!(aFlag & NS_IPARSER_FLAG_XML) ||
          kQuestionMark == mTextValue.Last()) {
        
        done = PR_TRUE;
      }
      
      aScanner.GetChar(aChar);
      mTextValue.Append(aChar);
    }
  }

  if (kEOF == result && !aScanner.IsIncremental()) {
    
    mInError = PR_TRUE;
    result = NS_OK;
  }

  return result;
}

PRInt32
CInstructionToken::GetTokenType()
{
  return eToken_instruction;
}

const nsSubstring&
CInstructionToken::GetStringValue()
{
  return mTextValue;
}



CDoctypeDeclToken::CDoctypeDeclToken(eHTMLTags aTag)
  : CHTMLToken(aTag)
{
}

CDoctypeDeclToken::CDoctypeDeclToken(const nsAString& aString, eHTMLTags aTag)
  : CHTMLToken(aTag), mTextValue(aString)
{
}







nsresult
CDoctypeDeclToken::Consume(PRUnichar aChar, nsScanner& aScanner, PRInt32 aFlag)
{
  static const PRUnichar terminalChars[] =
  { PRUnichar('>'), PRUnichar('<'),
    PRUnichar(0)
  };
  static const nsReadEndCondition theEndCondition(terminalChars);

  nsScannerIterator start, end;

  aScanner.CurrentPosition(start);
  aScanner.EndReading(end);

  nsresult result = aScanner.ReadUntil(start, end, theEndCondition, PR_FALSE);

  if (NS_SUCCEEDED(result)) {
    PRUnichar ch;
    aScanner.Peek(ch);
    if (ch == kGreaterThan) {
      
      
      aScanner.GetChar(ch);
      end.advance(1);
    } else {
      NS_ASSERTION(kLessThan == ch,
                   "Make sure this doctype decl. is really in error.");
      mInError = PR_TRUE;
    }
  } else if (!aScanner.IsIncremental()) {
    
    
    
    mInError = PR_TRUE;
    result = NS_OK;
  }

  if (NS_SUCCEEDED(result)) {
    start.advance(-2); 
    CopyUnicodeTo(start, end, mTextValue);
  }

  return result;
}

PRInt32
CDoctypeDeclToken::GetTokenType()
{
  return eToken_doctypeDecl;
}

const nsSubstring&
CDoctypeDeclToken::GetStringValue()
{
  return mTextValue;
}

void
CDoctypeDeclToken::SetStringValue(const nsAString& aStr)
{
  mTextValue.Assign(aStr);
}
