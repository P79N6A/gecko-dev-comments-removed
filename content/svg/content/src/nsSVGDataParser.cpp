




































 












#include "nsSVGDataParser.h"
#include "nsContentUtils.h"
#include "prdtoa.h"
#include "nsSVGUtils.h"
#include <stdlib.h>
#include <math.h>




nsresult
nsSVGDataParser::Parse(const nsAString &aValue)
{
  nsresult rv = NS_OK;

  char *str = ToNewUTF8String(aValue);
  if (!str)
    return NS_ERROR_OUT_OF_MEMORY;

  mInputPos = str;

  GetNextToken();
  rv = Match();
  if (mTokenType != END)
    rv = NS_ERROR_FAILURE; 

  mInputPos = nsnull;
  nsMemory::Free(str);

  return rv;
}




void nsSVGDataParser::GetNextToken()
{
  mTokenPos  = mInputPos;
  mTokenVal  = *mInputPos;
  
  switch (mTokenVal) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      mTokenType = DIGIT;
      break;
    case '\x20': case '\x9': case '\xd': case '\xa':
      mTokenType = WSP;
      break;
    case ',':
      mTokenType = COMMA;
      break;
    case '+': case '-':
      mTokenType = SIGN;
      break;
    case '.':
      mTokenType = POINT;
      break;
    case '(':
      mTokenType = LEFT_PAREN;
      break;
    case ')':
      mTokenType = RIGHT_PAREN;
      break;
    case '\0':
      mTokenType = END;
      break;
    default:
      mTokenType = OTHER;
  }
  
  if (*mInputPos != '\0') {
    ++mInputPos;
  }
}

void nsSVGDataParser::RewindTo(const char* aPos)
{
  mInputPos = aPos;
  GetNextToken();
}


nsresult nsSVGDataParser::Match()
{
   return NS_OK;
}



nsresult nsSVGDataParser::MatchNonNegativeNumber(float* aX)
{
  
  
  
  const char* pos = mTokenPos;

  nsresult rv = MatchFloatingPointConst();

  if (NS_FAILED(rv)) {
    RewindTo(pos);
    ENSURE_MATCHED(MatchIntegerConst());
  }

  char* end;
  *aX = float(PR_strtod(pos, &end));
  if (pos != end && NS_FloatIsFinite(*aX)) {
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}

PRBool nsSVGDataParser::IsTokenNonNegativeNumberStarter()
{
  return (mTokenType == DIGIT || mTokenType == POINT);
}



nsresult nsSVGDataParser::MatchNumber(float* aX)
{
  const char* pos = mTokenPos;
  
  if (mTokenType == SIGN)
    GetNextToken();

  const char* pos2 = mTokenPos;

  nsresult rv = MatchFloatingPointConst();

  if (NS_FAILED(rv)) {
    RewindTo(pos2);
    ENSURE_MATCHED(MatchIntegerConst());
  }

  char* end;
  



  *aX = float(PR_strtod(pos, &end));
  if (pos != end && NS_FloatIsFinite(*aX)) {
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}

PRBool nsSVGDataParser::IsTokenNumberStarter()
{
  return (mTokenType == DIGIT || mTokenType == POINT || mTokenType == SIGN);
}




nsresult nsSVGDataParser::MatchCommaWsp()
{
  switch (mTokenType) {
    case WSP:
      ENSURE_MATCHED(MatchWsp());
      if (mTokenType == COMMA)
        GetNextToken();
      break;
    case COMMA:
      GetNextToken();
      break;
    default:
      return NS_ERROR_FAILURE;
  }

  while (IsTokenWspStarter()) {
    ENSURE_MATCHED(MatchWsp());
  }
  return NS_OK;
}
  
PRBool nsSVGDataParser::IsTokenCommaWspStarter()
{
  return (IsTokenWspStarter() || mTokenType == COMMA);
}



nsresult nsSVGDataParser::MatchIntegerConst()
{
  ENSURE_MATCHED(MatchDigitSeq());
  return NS_OK;
}



nsresult nsSVGDataParser::MatchFloatingPointConst()
{
  
  
  
  const char* pos = mTokenPos;

  nsresult rv = MatchFractConst();
  if (NS_SUCCEEDED(rv)) {
    if (IsTokenExponentStarter())
      ENSURE_MATCHED(MatchExponent());
  }
  else {
    RewindTo(pos);
    ENSURE_MATCHED(MatchDigitSeq());
    ENSURE_MATCHED(MatchExponent());    
  }

  return NS_OK;  
}



nsresult nsSVGDataParser::MatchFractConst()
{
  if (mTokenType == POINT) {
    GetNextToken();
    ENSURE_MATCHED(MatchDigitSeq());
  }
  else {
    ENSURE_MATCHED(MatchDigitSeq());
    if (mTokenType == POINT) {
      GetNextToken();
      if (IsTokenDigitSeqStarter()) {
        ENSURE_MATCHED(MatchDigitSeq());
      }
    }
  }
  return NS_OK;
}



nsresult nsSVGDataParser::MatchExponent()
{
  if (!(tolower(mTokenVal) == 'e')) return NS_ERROR_FAILURE;

  GetNextToken();

  if (mTokenType == SIGN)
    GetNextToken();

  ENSURE_MATCHED(MatchDigitSeq());

  return NS_OK;  
}

PRBool nsSVGDataParser::IsTokenExponentStarter()
{
  return (tolower(mTokenVal) == 'e');
}



nsresult nsSVGDataParser::MatchDigitSeq()
{
  if (!(mTokenType == DIGIT)) return NS_ERROR_FAILURE;

  do {
    GetNextToken();
  } while (mTokenType == DIGIT);

  return NS_OK;
}

PRBool nsSVGDataParser::IsTokenDigitSeqStarter()
{
  return (mTokenType == DIGIT);
}



nsresult nsSVGDataParser::MatchWsp()
{
  if (!(mTokenType == WSP)) return NS_ERROR_FAILURE;

  do {
    GetNextToken();
  } while (mTokenType == WSP);

  return NS_OK;  
}

PRBool nsSVGDataParser::IsTokenWspStarter()
{
  return (mTokenType == WSP);
}  



nsresult nsSVGDataParser::MatchLeftParen()
{
  switch (mTokenType) {
    case LEFT_PAREN:
      GetNextToken();
      break;
    default:
      return NS_ERROR_FAILURE;
  }

 
  return NS_OK;
}

nsresult nsSVGDataParser::MatchRightParen()
{
  switch (mTokenType) {
    case RIGHT_PAREN:
       GetNextToken();
      break;
    default:
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

