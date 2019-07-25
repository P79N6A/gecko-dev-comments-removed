




#include "mozTXTToHTMLConv.h"
#include "nsIServiceManager.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsIExternalProtocolHandler.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#ifdef DEBUG_BenB_Perf
#include "prtime.h"
#include "prinrval.h"
#endif

const double growthRate = 1.2;





static inline bool IsSpace(const PRUnichar aChar)
{
  return (nsCRT::IsAsciiSpace(aChar) || aChar == 0xA0 || aChar == 0x3000);
}



void
mozTXTToHTMLConv::EscapeChar(const PRUnichar ch, nsString& aStringToAppendTo,
                             bool inAttribute)
{
    switch (ch)
    {
    case '<':
      aStringToAppendTo.AppendLiteral("&lt;");
      break;
    case '>':
      aStringToAppendTo.AppendLiteral("&gt;");
      break;
    case '&':
      aStringToAppendTo.AppendLiteral("&amp;");
      break;
    case '"':
      if (inAttribute)
      {
        aStringToAppendTo.AppendLiteral("&quot;");
        break;
      }
      
    default:
      aStringToAppendTo += ch;
    }

    return;
}



void
mozTXTToHTMLConv::EscapeStr(nsString& aInString, bool inAttribute)
{
  
  
  
  
  
  
  
  for (uint32_t i = 0; i < aInString.Length();)
  {
    switch (aInString[i])
    {
    case '<':
      aInString.Cut(i, 1);
      aInString.Insert(NS_LITERAL_STRING("&lt;"), i);
      i += 4; 
      break;
    case '>':
      aInString.Cut(i, 1);
      aInString.Insert(NS_LITERAL_STRING("&gt;"), i);
      i += 4; 
      break;
    case '&':
      aInString.Cut(i, 1);
      aInString.Insert(NS_LITERAL_STRING("&amp;"), i);
      i += 5; 
      break;
    case '"':
      if (inAttribute)
      {
        aInString.Cut(i, 1);
        aInString.Insert(NS_LITERAL_STRING("&quot;"), i);
        i += 6;
        break;
      }
      
    default:
      i++;
    }
  }
}

void 
mozTXTToHTMLConv::UnescapeStr(const PRUnichar * aInString, int32_t aStartPos, int32_t aLength, nsString& aOutString)
{
  const PRUnichar * subString = nullptr;
  for (uint32_t i = aStartPos; int32_t(i) - aStartPos < aLength;)
  {
    int32_t remainingChars = i - aStartPos;
    if (aInString[i] == '&')
    {
      subString = &aInString[i];
      if (!nsCRT::strncmp(subString, NS_LITERAL_STRING("&lt;").get(), MinInt(4, aLength - remainingChars)))
      {
        aOutString.Append(PRUnichar('<'));
        i += 4;
      }
      else if (!nsCRT::strncmp(subString, NS_LITERAL_STRING("&gt;").get(), MinInt(4, aLength - remainingChars)))
      {
        aOutString.Append(PRUnichar('>'));
        i += 4;
      }
      else if (!nsCRT::strncmp(subString, NS_LITERAL_STRING("&amp;").get(), MinInt(5, aLength - remainingChars)))
      {
        aOutString.Append(PRUnichar('&'));
        i += 5;
      }
      else if (!nsCRT::strncmp(subString, NS_LITERAL_STRING("&quot;").get(), MinInt(6, aLength - remainingChars)))
      {
        aOutString.Append(PRUnichar('"'));
        i += 6;
      }
      else
      {
        aOutString += aInString[i];
        i++;
      }
    }
    else
    {
      aOutString += aInString[i];
      i++;
    }
  }
}

void
mozTXTToHTMLConv::CompleteAbbreviatedURL(const PRUnichar * aInString, int32_t aInLength, 
                                         const uint32_t pos, nsString& aOutString)
{
  NS_ASSERTION(int32_t(pos) < aInLength, "bad args to CompleteAbbreviatedURL, see bug #190851");
  if (int32_t(pos) >= aInLength)
    return;

  if (aInString[pos] == '@')
  {
    
    
    nsDependentString inString(aInString, aInLength);
    if (inString.FindChar('.', pos) != kNotFound) 
    {
      aOutString.AssignLiteral("mailto:");
      aOutString += aInString;
    }
  }
  else if (aInString[pos] == '.')
  {
    if (ItMatchesDelimited(aInString, aInLength,
                           NS_LITERAL_STRING("www.").get(), 4, LT_IGNORE, LT_IGNORE))
    {
      aOutString.AssignLiteral("http://");
      aOutString += aInString;
    }
    else if (ItMatchesDelimited(aInString,aInLength, NS_LITERAL_STRING("ftp.").get(), 4, LT_IGNORE, LT_IGNORE))
    { 
      aOutString.AssignLiteral("ftp://");
      aOutString += aInString;
    }
  }
}

bool
mozTXTToHTMLConv::FindURLStart(const PRUnichar * aInString, int32_t aInLength,
                               const uint32_t pos, const modetype check,
                               uint32_t& start)
{
  switch(check)
  { 
  case RFC1738:
  {
    if (!nsCRT::strncmp(&aInString[MaxInt(pos - 4, 0)], NS_LITERAL_STRING("<URL:").get(), 5))
    {
      start = pos + 1;
      return true;
    }
    else
      return false;
  }
  case RFC2396E:
  {
    nsString temp(aInString, aInLength);
    int32_t i = pos <= 0 ? kNotFound : temp.RFindCharInSet(NS_LITERAL_STRING("<>\"").get(), pos - 1);
    if (i != kNotFound && (temp[uint32_t(i)] == '<' ||
                           temp[uint32_t(i)] == '"'))
    {
      start = uint32_t(++i);
      return start < pos;
    }
    else
      return false;
  }
  case freetext:
  {
    int32_t i = pos - 1;
    for (; i >= 0 && (
         nsCRT::IsAsciiAlpha(aInString[uint32_t(i)]) ||
         nsCRT::IsAsciiDigit(aInString[uint32_t(i)]) ||
         aInString[uint32_t(i)] == '+' ||
         aInString[uint32_t(i)] == '-' ||
         aInString[uint32_t(i)] == '.'
         ); i--)
      ;
    if (++i >= 0 && uint32_t(i) < pos && nsCRT::IsAsciiAlpha(aInString[uint32_t(i)]))
    {
      start = uint32_t(i);
      return true;
    }
    else
      return false;
  }
  case abbreviated:
  {
    int32_t i = pos - 1;
    
    
    bool isEmail = aInString[pos] == (PRUnichar)'@';
    
    for (; i >= 0
             && aInString[uint32_t(i)] != '>' && aInString[uint32_t(i)] != '<'
             && aInString[uint32_t(i)] != '"' && aInString[uint32_t(i)] != '\''
             && aInString[uint32_t(i)] != '`' && aInString[uint32_t(i)] != ','
             && aInString[uint32_t(i)] != '{' && aInString[uint32_t(i)] != '['
             && aInString[uint32_t(i)] != '(' && aInString[uint32_t(i)] != '|'
             && aInString[uint32_t(i)] != '\\'
             && !IsSpace(aInString[uint32_t(i)])
             && (!isEmail || nsCRT::IsAscii(aInString[uint32_t(i)]))
         ; i--)
      ;
    if
      (
        ++i >= 0 && uint32_t(i) < pos
          &&
          (
            nsCRT::IsAsciiAlpha(aInString[uint32_t(i)]) ||
            nsCRT::IsAsciiDigit(aInString[uint32_t(i)])
          )
      )
    {
      start = uint32_t(i);
      return true;
    }
    else
      return false;
  }
  default:
    return false;
  } 
}

bool
mozTXTToHTMLConv::FindURLEnd(const PRUnichar * aInString, int32_t aInStringLength, const uint32_t pos,
           const modetype check, const uint32_t start, uint32_t& end)
{
  switch(check)
  { 
  case RFC1738:
  case RFC2396E:
  {
    nsString temp(aInString, aInStringLength);

    int32_t i = temp.FindCharInSet(NS_LITERAL_STRING("<>\"").get(), pos + 1);
    if (i != kNotFound && temp[uint32_t(i--)] ==
        (check == RFC1738 || temp[start - 1] == '<' ? '>' : '"'))
    {
      end = uint32_t(i);
      return end > pos;
    }
    return false;
  }
  case freetext:
  case abbreviated:
  {
    uint32_t i = pos + 1;
    bool isEmail = aInString[pos] == (PRUnichar)'@';
    bool seenOpeningParenthesis = false; 
    bool seenOpeningSquareBracket = false; 
    for (; int32_t(i) < aInStringLength; i++)
    {
      
      if (aInString[i] == '>' || aInString[i] == '<' ||
          aInString[i] == '"' || aInString[i] == '`' ||
          aInString[i] == '}' || aInString[i] == '{' ||
          aInString[i] == '|' ||
          (aInString[i] == ')' && !seenOpeningParenthesis) ||
          (aInString[i] == ']' && !seenOpeningSquareBracket) ||
          
          (aInString[i] == '[' && i > 2 &&
           (aInString[i - 1] != '/' || aInString[i - 2] != '/')) ||
          IsSpace(aInString[i]))
          break;
      
      
      if (isEmail && (
            aInString[i] == '(' || aInString[i] == '\'' ||
            !nsCRT::IsAscii(aInString[i])))
          break;
      if (aInString[i] == '(')
        seenOpeningParenthesis = true;
      if (aInString[i] == '[')
        seenOpeningSquareBracket = true;
    }
    
    
    while (--i > pos && (
             aInString[i] == '.' || aInString[i] == ',' || aInString[i] == ';' ||
             aInString[i] == '!' || aInString[i] == '?' || aInString[i] == '-' ||
             aInString[i] == ':' || aInString[i] == '\''
             ))
        ;
    if (i > pos)
    {
      end = i;
      return true;
    }
    return false;
  }
  default:
    return false;
  } 
}

void
mozTXTToHTMLConv::CalculateURLBoundaries(const PRUnichar * aInString, int32_t aInStringLength, 
     const uint32_t pos, const uint32_t whathasbeendone,
     const modetype check, const uint32_t start, const uint32_t end,
     nsString& txtURL, nsString& desc,
     int32_t& replaceBefore, int32_t& replaceAfter)
{
  uint32_t descstart = start;
  switch(check)
  {
  case RFC1738:
  {
    descstart = start - 5;
    desc.Append(&aInString[descstart], end - descstart + 2);  
    replaceAfter = end - pos + 1;
  } break;
  case RFC2396E:
  {
    descstart = start - 1;
    desc.Append(&aInString[descstart], end - descstart + 2); 
    replaceAfter = end - pos + 1;
  } break;
  case freetext:
  case abbreviated:
  {
    descstart = start;
    desc.Append(&aInString[descstart], end - start + 1); 
    replaceAfter = end - pos;
  } break;
  default: break;
  } 

  EscapeStr(desc, false);

  txtURL.Append(&aInString[start], end - start + 1);
  txtURL.StripWhitespace();

  
  nsAutoString temp2;
  ScanTXT(&aInString[descstart], pos - descstart, ~kURLs  & whathasbeendone, temp2);
  replaceBefore = temp2.Length();
  return;
}

bool mozTXTToHTMLConv::ShouldLinkify(const nsCString& aURL)
{
  if (!mIOService)
    return false;

  nsCAutoString scheme;
  nsresult rv = mIOService->ExtractScheme(aURL, scheme);
  if(NS_FAILED(rv))
    return false;

  
  nsCOMPtr<nsIProtocolHandler> handler;    
  rv = mIOService->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
  if(NS_FAILED(rv))
    return false;

  
  nsCOMPtr<nsIExternalProtocolHandler> externalHandler = do_QueryInterface(handler);
  if (!externalHandler)
   return true; 

  
  bool exists;
  rv = externalHandler->ExternalAppExistsForScheme(scheme, &exists);
  return(NS_SUCCEEDED(rv) && exists);
}

bool
mozTXTToHTMLConv::CheckURLAndCreateHTML(
     const nsString& txtURL, const nsString& desc, const modetype mode,
     nsString& outputHTML)
{
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv;
  
  if (!mIOService)
  {
    mIOService = do_GetIOService();

    if (!mIOService)
      return false;
  }

  
  NS_ConvertUTF16toUTF8 utf8URL(txtURL);
  if (!ShouldLinkify(utf8URL))
    return false;

  
  
  rv = mIOService->NewURI(utf8URL, nullptr, nullptr, getter_AddRefs(uri));

  
  if (NS_SUCCEEDED(rv) && uri)
  {
    outputHTML.AssignLiteral("<a class=\"moz-txt-link-");
    switch(mode)
    {
    case RFC1738:
      outputHTML.AppendLiteral("rfc1738");
      break;
    case RFC2396E:
      outputHTML.AppendLiteral("rfc2396E");
      break;
    case freetext:
      outputHTML.AppendLiteral("freetext");
      break;
    case abbreviated:
      outputHTML.AppendLiteral("abbreviated");
      break;
    default: break;
    }
    nsAutoString escapedURL(txtURL);
    EscapeStr(escapedURL, true);

    outputHTML.AppendLiteral("\" href=\"");
    outputHTML += escapedURL;
    outputHTML.AppendLiteral("\">");
    outputHTML += desc;
    outputHTML.AppendLiteral("</a>");
    return true;
  }
  else
    return false;
}

NS_IMETHODIMP mozTXTToHTMLConv::FindURLInPlaintext(const PRUnichar * aInString, int32_t aInLength, int32_t aPos, int32_t * aStartPos, int32_t * aEndPos)
{
  
  nsAutoString outputHTML; 

  *aStartPos = -1;
  *aEndPos = -1;

  FindURL(aInString, aInLength, aPos, kURLs, outputHTML, *aStartPos, *aEndPos);

  return NS_OK;
}

bool
mozTXTToHTMLConv::FindURL(const PRUnichar * aInString, int32_t aInLength, const uint32_t pos,
     const uint32_t whathasbeendone,
     nsString& outputHTML, int32_t& replaceBefore, int32_t& replaceAfter)
{
  enum statetype {unchecked, invalid, startok, endok, success};
  static const modetype ranking[] = {RFC1738, RFC2396E, freetext, abbreviated};

  statetype state[mozTXTToHTMLConv_lastMode + 1]; 
  


  
  

  for (modetype iState = unknown; iState <= mozTXTToHTMLConv_lastMode;
       iState = modetype(iState + 1))
    state[iState] = aInString[pos] == ':' ? unchecked : invalid;
  switch (aInString[pos])
  {
  case '@':
    state[RFC2396E] = unchecked;
    
  case '.':
    state[abbreviated] = unchecked;
    break;
  case ':':
    state[abbreviated] = invalid;
    break;
  default:
    break;
  }

  
  int32_t iCheck = 0;  
  modetype check = ranking[iCheck];
  for (; iCheck < mozTXTToHTMLConv_numberOfModes && state[check] != success;
       iCheck++)
    

  {
    check = ranking[iCheck];

    uint32_t start, end;

    if (state[check] == unchecked)
      if (FindURLStart(aInString, aInLength, pos, check, start))
        state[check] = startok;

    if (state[check] == startok)
      if (FindURLEnd(aInString, aInLength, pos, check, start, end))
        state[check] = endok;

    if (state[check] == endok)
    {
      nsAutoString txtURL, desc;
      int32_t resultReplaceBefore, resultReplaceAfter;

      CalculateURLBoundaries(aInString, aInLength, pos, whathasbeendone, check, start, end,
                             txtURL, desc,
                             resultReplaceBefore, resultReplaceAfter);

      if (aInString[pos] != ':')
      {
        nsAutoString temp = txtURL;
        txtURL.SetLength(0);
        CompleteAbbreviatedURL(temp.get(),temp.Length(), pos - start, txtURL);
      }

      if (!txtURL.IsEmpty() && CheckURLAndCreateHTML(txtURL, desc, check,
                                                     outputHTML))
      {
        replaceBefore = resultReplaceBefore;
        replaceAfter = resultReplaceAfter;
        state[check] = success;
      }
    } 
  } 
  return state[check] == success;
}

bool
mozTXTToHTMLConv::ItMatchesDelimited(const PRUnichar * aInString,
    int32_t aInLength, const PRUnichar* rep, int32_t aRepLen,
    LIMTYPE before, LIMTYPE after)
{

  
  
  
  
  int32_t textLen = aInLength;

  if
    (
      ((before == LT_IGNORE && (after == LT_IGNORE || after == LT_DELIMITER))
        && textLen < aRepLen) ||
      ((before != LT_IGNORE || (after != LT_IGNORE && after != LT_DELIMITER))
        && textLen < aRepLen + 1) ||
      (before != LT_IGNORE && after != LT_IGNORE && after != LT_DELIMITER
        && textLen < aRepLen + 2)
    )
    return false;

  PRUnichar text0 = aInString[0];
  PRUnichar textAfterPos = aInString[aRepLen + (before == LT_IGNORE ? 0 : 1)];

  if
    (
      (before == LT_ALPHA
        && !nsCRT::IsAsciiAlpha(text0)) ||
      (before == LT_DIGIT
        && !nsCRT::IsAsciiDigit(text0)) ||
      (before == LT_DELIMITER
        &&
        (
          nsCRT::IsAsciiAlpha(text0) ||
          nsCRT::IsAsciiDigit(text0) ||
          text0 == *rep
        )) ||
      (after == LT_ALPHA
        && !nsCRT::IsAsciiAlpha(textAfterPos)) ||
      (after == LT_DIGIT
        && !nsCRT::IsAsciiDigit(textAfterPos)) ||
      (after == LT_DELIMITER
        &&
        (
          nsCRT::IsAsciiAlpha(textAfterPos) ||
          nsCRT::IsAsciiDigit(textAfterPos) ||
          textAfterPos == *rep
        )) ||
        !Substring(Substring(aInString, aInString+aInLength),
                   (before == LT_IGNORE ? 0 : 1),
                   aRepLen).Equals(Substring(rep, rep+aRepLen),
                                   nsCaseInsensitiveStringComparator())
    )
    return false;

  return true;
}

uint32_t
mozTXTToHTMLConv::NumberOfMatches(const PRUnichar * aInString, int32_t aInStringLength, 
     const PRUnichar* rep, int32_t aRepLen, LIMTYPE before, LIMTYPE after)
{
  uint32_t result = 0;

  for (int32_t i = 0; i < aInStringLength; i++)
  {
    const PRUnichar * indexIntoString = &aInString[i];
    if (ItMatchesDelimited(indexIntoString, aInStringLength - i, rep, aRepLen, before, after))
      result++;
  }
  return result;
}




bool
mozTXTToHTMLConv::StructPhraseHit(const PRUnichar * aInString, int32_t aInStringLength, bool col0,
     const PRUnichar* tagTXT, int32_t aTagTXTLen, 
     const char* tagHTML, const char* attributeHTML,
     nsString& aOutString, uint32_t& openTags)
{
  





  const PRUnichar * newOffset = aInString;
  int32_t newLength = aInStringLength;
  if (!col0) 
  {
    newOffset = &aInString[1];
    newLength = aInStringLength - 1;
  }

  
  if
    (
      ItMatchesDelimited(aInString, aInStringLength, tagTXT, aTagTXTLen, 
           (col0 ? LT_IGNORE : LT_DELIMITER), LT_ALPHA) 
        && NumberOfMatches(newOffset, newLength, tagTXT, aTagTXTLen, 
              LT_ALPHA, LT_DELIMITER)  
              > openTags
    )
  {
    openTags++;
    aOutString.AppendLiteral("<");
    aOutString.AppendASCII(tagHTML);
    aOutString.Append(PRUnichar(' '));
    aOutString.AppendASCII(attributeHTML);
    aOutString.AppendLiteral("><span class=\"moz-txt-tag\">");
    aOutString.Append(tagTXT);
    aOutString.AppendLiteral("</span>");
    return true;
  }

  
  else if (openTags > 0
       && ItMatchesDelimited(aInString, aInStringLength, tagTXT, aTagTXTLen, LT_ALPHA, LT_DELIMITER))
  {
    openTags--;
    aOutString.AppendLiteral("<span class=\"moz-txt-tag\">");
    aOutString.Append(tagTXT);
    aOutString.AppendLiteral("</span></");
    aOutString.AppendASCII(tagHTML);
    aOutString.Append(PRUnichar('>'));
    return true;
  }

  return false;
}


bool
mozTXTToHTMLConv::SmilyHit(const PRUnichar * aInString, int32_t aLength, bool col0,
         const char* tagTXT, const char* imageName,
         nsString& outputHTML, int32_t& glyphTextLen)
{
  if ( !aInString || !tagTXT || !imageName )
      return false;

  int32_t tagLen = strlen(tagTXT);
 
  uint32_t delim = (col0 ? 0 : 1) + tagLen;

  if
    (
      (col0 || IsSpace(aInString[0]))
        &&
        (
          aLength <= int32_t(delim) ||
          IsSpace(aInString[delim]) ||
          (aLength > int32_t(delim + 1)
            &&
            (
              aInString[delim] == '.' ||
              aInString[delim] == ',' ||
              aInString[delim] == ';' ||
              aInString[delim] == '8' ||
              aInString[delim] == '>' ||
              aInString[delim] == '!' ||
              aInString[delim] == '?'
            )
            && IsSpace(aInString[delim + 1]))
        )
        && ItMatchesDelimited(aInString, aLength, NS_ConvertASCIItoUTF16(tagTXT).get(), tagLen, 
                              col0 ? LT_IGNORE : LT_DELIMITER, LT_IGNORE)
	        
    )
  {
    if (!col0)
    {
      outputHTML.Truncate();
      outputHTML.Append(PRUnichar(' '));
    }

    outputHTML.AppendLiteral("<span class=\""); 
    AppendASCIItoUTF16(imageName, outputHTML);  
    outputHTML.AppendLiteral("\" title=\"");    
    AppendASCIItoUTF16(tagTXT, outputHTML);     
    outputHTML.AppendLiteral("\"><span>");      
    AppendASCIItoUTF16(tagTXT, outputHTML);     
    outputHTML.AppendLiteral("</span></span>"); 
    glyphTextLen = (col0 ? 0 : 1) + tagLen;
    return true;
  }

  return false;
}


bool
mozTXTToHTMLConv::GlyphHit(const PRUnichar * aInString, int32_t aInLength, bool col0,
         nsString& aOutputString, int32_t& glyphTextLen)
{
  PRUnichar text0 = aInString[0]; 
  PRUnichar text1 = aInString[1];
  PRUnichar firstChar = (col0 ? text0 : text1);

  
  nsAutoString outputHTML;
  bool bTestSmilie;
  bool bArg = false;
  int i;

  
  
  

  i = 0;
  while ( i < 2 )
  {
    bTestSmilie = false;
    if ( !i && (firstChar == ':' || firstChar == ';' || firstChar == '=' || firstChar == '>' || firstChar == '8' || firstChar == 'O'))
    {
        

        bTestSmilie = true;
        bArg = col0;
    }
    if ( i && col0 && ( text1 == ':' || text1 == ';' || text1 == '=' || text1 == '>' || text1 == '8' || text1 == 'O' ) )
    {
        

        bTestSmilie = true;
        bArg = false;
    }
    if ( bTestSmilie && (
          SmilyHit(aInString, aInLength, bArg,
                   ":-)",
                   "moz-smiley-s1", 
                   outputHTML, glyphTextLen) ||
  
          SmilyHit(aInString, aInLength, bArg,
                   ":)",
                   "moz-smiley-s1", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":-D",
                   "moz-smiley-s5", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":-(",
                   "moz-smiley-s2", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":(",
                   "moz-smiley-s2", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":-[",
                   "moz-smiley-s6", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ";-)",
                   "moz-smiley-s3", 
                   outputHTML, glyphTextLen) ||

          SmilyHit(aInString, aInLength, col0,
                   ";)",
                   "moz-smiley-s3", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":-\\",
                   "moz-smiley-s7", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ":-P",
                   "moz-smiley-s4", 
                   outputHTML, glyphTextLen) ||
                   
          SmilyHit(aInString, aInLength, bArg,
                   ";-P",
                   "moz-smiley-s4", 
                   outputHTML, glyphTextLen) ||  
         
          SmilyHit(aInString, aInLength, bArg,
                   "=-O",
                   "moz-smiley-s8", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ":-*",
                   "moz-smiley-s9", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ">:o",
                   "moz-smiley-s10", 
                   outputHTML, glyphTextLen) ||
          
          SmilyHit(aInString, aInLength, bArg,
                   ">:-o",
                   "moz-smiley-s10", 
                   outputHTML, glyphTextLen) ||
        
          SmilyHit(aInString, aInLength, bArg,
                   "8-)",
                   "moz-smiley-s11", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ":-$",
                   "moz-smiley-s12", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ":-!",
                   "moz-smiley-s13", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   "O:-)",
                   "moz-smiley-s14", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ":'(",
                   "moz-smiley-s15", 
                   outputHTML, glyphTextLen) ||
         
          SmilyHit(aInString, aInLength, bArg,
                   ":-X",
                   "moz-smiley-s16", 
                   outputHTML, glyphTextLen) 
        )
    )
    {
        aOutputString.Append(outputHTML);
        return true;
    }
    i++;
  }
  if (text0 == '\f')
  {
      aOutputString.AppendLiteral("<span class='moz-txt-formfeed'></span>");
      glyphTextLen = 1;
      return true;
  }
  if (text0 == '+' || text1 == '+')
  {
    if (ItMatchesDelimited(aInString, aInLength,
                           NS_LITERAL_STRING(" +/-").get(), 4,
                           LT_IGNORE, LT_IGNORE))
    {
      aOutputString.AppendLiteral(" &plusmn;");
      glyphTextLen = 4;
      return true;
    }
    if (col0 && ItMatchesDelimited(aInString, aInLength,
                                   NS_LITERAL_STRING("+/-").get(), 3,
                                   LT_IGNORE, LT_IGNORE))
    {
      aOutputString.AppendLiteral("&plusmn;");
      glyphTextLen = 3;
      return true;
    }
  }

  
  
  if    
    (
      text1 == '^'
      && 
      (
        nsCRT::IsAsciiDigit(text0) || nsCRT::IsAsciiAlpha(text0) || 
        text0 == ')' || text0 == ']' || text0 == '}'
      )
      &&
      (
        (2 < aInLength && nsCRT::IsAsciiDigit(aInString[2])) ||
        (3 < aInLength && aInString[2] == '-' && nsCRT::IsAsciiDigit(aInString[3]))
      )
    )
  {
    
    int32_t delimPos = 3;  
    for (; delimPos < aInLength
           &&
           (
             nsCRT::IsAsciiDigit(aInString[delimPos]) || 
             (aInString[delimPos] == '.' && delimPos + 1 < aInLength &&
               nsCRT::IsAsciiDigit(aInString[delimPos + 1]))
           );
         delimPos++)
      ;

    if (delimPos < aInLength && nsCRT::IsAsciiAlpha(aInString[delimPos]))
    {
      return false;
    }

    outputHTML.Truncate();
    outputHTML += text0;
    outputHTML.AppendLiteral(
      "<sup class=\"moz-txt-sup\">"
      "<span style=\"display:inline-block;width:0;height:0;overflow:hidden\">"
      "^</span>");

    aOutputString.Append(outputHTML);
    aOutputString.Append(&aInString[2], delimPos - 2);
    aOutputString.AppendLiteral("</sup>");

    glyphTextLen = delimPos  ;
    return true;
  }
  












  return false;
}





mozTXTToHTMLConv::mozTXTToHTMLConv()
{
}

mozTXTToHTMLConv::~mozTXTToHTMLConv() 
{
}

NS_IMPL_ISUPPORTS4(mozTXTToHTMLConv,
                   mozITXTToHTMLConv,
                   nsIStreamConverter,
                   nsIStreamListener,
                   nsIRequestObserver)

int32_t
mozTXTToHTMLConv::CiteLevelTXT(const PRUnichar *line,
				    uint32_t& logLineStart)
{
  int32_t result = 0;
  int32_t lineLength = NS_strlen(line);

  bool moreCites = true;
  while (moreCites)
  {
    













    uint32_t i = logLineStart;

#ifdef QUOTE_RECOGNITION_AGGRESSIVE
    for (; int32_t(i) < lineLength && IsSpace(line[i]); i++)
      ;
    for (; int32_t(i) < lineLength && nsCRT::IsAsciiAlpha(line[i])
                                   && nsCRT::IsUpper(line[i])   ; i++)
      ;
    if (int32_t(i) < lineLength && (line[i] == '>' || line[i] == ']'))
#else
    if (int32_t(i) < lineLength && line[i] == '>')
#endif
    {
      i++;
      if (int32_t(i) < lineLength && line[i] == ' ')
        i++;
      
      
      const PRUnichar * indexString = &line[logLineStart];
           
      uint32_t minlength = MinInt(6, NS_strlen(indexString));
      if (Substring(indexString,
                    indexString+minlength).Equals(Substring(NS_LITERAL_STRING(">From "), 0, minlength),
                                                  nsCaseInsensitiveStringComparator()))
        
        moreCites = false;
      else
      {
        result++;
        logLineStart = i;
      }
    }
    else
      moreCites = false;
  }

  return result;
}

void
mozTXTToHTMLConv::ScanTXT(const PRUnichar * aInString, int32_t aInStringLength, uint32_t whattodo, nsString& aOutString)
{
  bool doURLs = 0 != (whattodo & kURLs);
  bool doGlyphSubstitution = 0 != (whattodo & kGlyphSubstitution);
  bool doStructPhrase = 0 != (whattodo & kStructPhrase);

  uint32_t structPhrase_strong = 0;  
  uint32_t structPhrase_underline = 0;
  uint32_t structPhrase_italic = 0;
  uint32_t structPhrase_code = 0;

  nsAutoString outputHTML;  

  for(uint32_t i = 0; int32_t(i) < aInStringLength;)
  {
    if (doGlyphSubstitution)
    {
      int32_t glyphTextLen;
      if (GlyphHit(&aInString[i], aInStringLength - i, i == 0, aOutString, glyphTextLen))
      {
        i += glyphTextLen;
        continue;
      }
    }

    if (doStructPhrase)
    {
      const PRUnichar * newOffset = aInString;
      int32_t newLength = aInStringLength;
      if (i > 0 ) 
      {
        newOffset = &aInString[i-1];
        newLength = aInStringLength - i + 1;
      }

      switch (aInString[i]) 
      {
      case '*':
        if (StructPhraseHit(newOffset, newLength, i == 0,
                            NS_LITERAL_STRING("*").get(), 1,
                            "b", "class=\"moz-txt-star\"",
                            aOutString, structPhrase_strong))
        {
          i++;
          continue;
        }
        break;
      case '/':
        if (StructPhraseHit(newOffset, newLength, i == 0,
                            NS_LITERAL_STRING("/").get(), 1,
                            "i", "class=\"moz-txt-slash\"",
                            aOutString, structPhrase_italic))
        {
          i++;
          continue;
        }
        break;
      case '_':
        if (StructPhraseHit(newOffset, newLength, i == 0,
                            NS_LITERAL_STRING("_").get(), 1,
                            "span" ,
                            "class=\"moz-txt-underscore\"",
                            aOutString, structPhrase_underline))
        {
          i++;
          continue;
        }
        break;
      case '|':
        if (StructPhraseHit(newOffset, newLength, i == 0,
                            NS_LITERAL_STRING("|").get(), 1,
                            "code", "class=\"moz-txt-verticalline\"",
                            aOutString, structPhrase_code))
        {
          i++;
          continue;
        }
        break;
      }
    }

    if (doURLs)
    {
      switch (aInString[i])
      {
      case ':':
      case '@':
      case '.':
        if ( (i == 0 || ((i > 0) && aInString[i - 1] != ' ')) && aInString[i +1] != ' ') 
        {
          int32_t replaceBefore;
          int32_t replaceAfter;
          if (FindURL(aInString, aInStringLength, i, whattodo,
                      outputHTML, replaceBefore, replaceAfter)
                  && structPhrase_strong + structPhrase_italic +
                       structPhrase_underline + structPhrase_code == 0
                        )
          {
            aOutString.Cut(aOutString.Length() - replaceBefore, replaceBefore);
            aOutString += outputHTML;
            i += replaceAfter + 1;
            continue;
          }
        }
        break;
      } 
    }

    switch (aInString[i])
    {
    
    case '<':
    case '>':
    case '&':
      EscapeChar(aInString[i], aOutString, false);
      i++;
      break;
    
    default:
      aOutString += aInString[i];
      i++;
      break;
    }
  }
}

void
mozTXTToHTMLConv::ScanHTML(nsString& aInString, uint32_t whattodo, nsString &aOutString)
{ 
  
  
  int32_t lengthOfInString = aInString.Length();
  const PRUnichar * uniBuffer = aInString.get();

#ifdef DEBUG_BenB_Perf
  PRTime parsing_start = PR_IntervalNow();
#endif

  
  


  for (int32_t i = 0; i < lengthOfInString;)
  {
    if (aInString[i] == '<')  
    {
      uint32_t start = uint32_t(i);
      if (nsCRT::ToLower((char)aInString[uint32_t(i) + 1]) == 'a')
           
      {
        i = aInString.Find("</a>", true, i);
        if (i == kNotFound)
          i = lengthOfInString;
        else
          i += 4;
      }
      else if (aInString[uint32_t(i) + 1] == '!' && aInString[uint32_t(i) + 2] == '-' &&
        aInString[uint32_t(i) + 3] == '-')
          
      {
        i = aInString.Find("-->", false, i);
        if (i == kNotFound)
          i = lengthOfInString;
        else
          i += 3;

      }
      else  
      {
        i = aInString.FindChar('>', i);
        if (i == kNotFound)
          i = lengthOfInString;
        else
          i++;
      }
      aOutString.Append(&uniBuffer[start], uint32_t(i) - start);
    }
    else
    {
      uint32_t start = uint32_t(i);
      i = aInString.FindChar('<', i);
      if (i == kNotFound)
        i = lengthOfInString;
  
      nsString tempString;     
      tempString.SetCapacity(uint32_t((uint32_t(i) - start) * growthRate));
      UnescapeStr(uniBuffer, start, uint32_t(i) - start, tempString);
      ScanTXT(tempString.get(), tempString.Length(), whattodo, aOutString);
    }
  }

#ifdef DEBUG_BenB_Perf
  printf("ScanHTML time:    %d ms\n", PR_IntervalToMilliseconds(PR_IntervalNow() - parsing_start));
#endif
}





NS_IMETHODIMP
mozTXTToHTMLConv::Convert(nsIInputStream *aFromStream,
                          const char *aFromType,
                          const char *aToType,
                          nsISupports *aCtxt, nsIInputStream **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozTXTToHTMLConv::AsyncConvertData(const char *aFromType,
                                   const char *aToType,
                                   nsIStreamListener *aListener, nsISupports *aCtxt) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozTXTToHTMLConv::OnDataAvailable(nsIRequest* request, nsISupports *ctxt,
                                 nsIInputStream *inStr, uint32_t sourceOffset,
                                 uint32_t count)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozTXTToHTMLConv::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozTXTToHTMLConv::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                                nsresult aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
mozTXTToHTMLConv::CiteLevelTXT(const PRUnichar *line, uint32_t *logLineStart,
				uint32_t *_retval)
{
   if (!logLineStart || !_retval || !line)
     return NS_ERROR_NULL_POINTER;
   *_retval = CiteLevelTXT(line, *logLineStart);
   return NS_OK;
}

NS_IMETHODIMP
mozTXTToHTMLConv::ScanTXT(const PRUnichar *text, uint32_t whattodo,
			   PRUnichar **_retval)
{
  NS_ENSURE_ARG(text);

  
  nsString outString;
  int32_t inLength = NS_strlen(text);
  
  
  
  NS_ASSERTION(inLength, "ScanTXT passed 0 length string");
  if (inLength == 0) {
    *_retval = nsCRT::strdup(text);
    return NS_OK;
  }

  outString.SetCapacity(uint32_t(inLength * growthRate));
  ScanTXT(text, inLength, whattodo, outString);

  *_retval = ToNewUnicode(outString);
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
mozTXTToHTMLConv::ScanHTML(const PRUnichar *text, uint32_t whattodo,
			    PRUnichar **_retval)
{
  NS_ENSURE_ARG(text);

  
  nsString outString;
  nsString inString (text); 
  outString.SetCapacity(uint32_t(inString.Length() * growthRate));

  ScanHTML(inString, whattodo, outString);
  *_retval = ToNewUnicode(outString);
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
MOZ_NewTXTToHTMLConv(mozTXTToHTMLConv** aConv)
{
    NS_PRECONDITION(aConv != nullptr, "null ptr");
    if (!aConv)
      return NS_ERROR_NULL_POINTER;

    *aConv = new mozTXTToHTMLConv();
    if (!*aConv)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aConv);
    
    return NS_OK;
}
