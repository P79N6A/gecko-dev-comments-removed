




































#ifndef __NS_SVGDATAPARSER_H__
#define __NS_SVGDATAPARSER_H__

#include "nsCOMPtr.h"
#include "nsVoidArray.h"



#define ENSURE_MATCHED(exp) { nsresult rv = exp; if (NS_FAILED(rv)) return rv; }





class nsSVGDataParser
{
public:
  nsresult Parse(const nsAString &aValue);

protected:
  const char* mInputPos;
  
  const char* mTokenPos;
  enum { DIGIT, WSP, COMMA, POINT, SIGN, LEFT_PAREN, RIGHT_PAREN, OTHER, END } mTokenType;
  char mTokenVal;

  
  void GetNextToken();
  void RewindTo(const char* aPos);
  virtual nsresult Match();

  nsresult MatchNonNegativeNumber(float* aX);
  PRBool IsTokenNonNegativeNumberStarter();
  
  nsresult MatchNumber(float* x);
  PRBool IsTokenNumberStarter();
  
  nsresult MatchCommaWsp();
  PRBool IsTokenCommaWspStarter();
  
  nsresult MatchIntegerConst();
  
  nsresult MatchFloatingPointConst();
  
  nsresult MatchFractConst();
  
  nsresult MatchExponent();
  PRBool IsTokenExponentStarter();
  
  nsresult MatchDigitSeq();
  PRBool IsTokenDigitSeqStarter();
  
  nsresult MatchWsp();
  PRBool IsTokenWspStarter();

  nsresult MatchLeftParen();
  nsresult MatchRightParen();
};


#endif 
