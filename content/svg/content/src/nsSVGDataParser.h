




































#ifndef __NS_SVGDATAPARSER_H__
#define __NS_SVGDATAPARSER_H__

#include "nsCOMPtr.h"



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
  virtual nsresult Match()=0;

  nsresult MatchNonNegativeNumber(float* aX);
  bool IsTokenNonNegativeNumberStarter();
  
  nsresult MatchNumber(float* x);
  bool IsTokenNumberStarter();
  
  nsresult MatchCommaWsp();
  bool IsTokenCommaWspStarter();
  
  nsresult MatchIntegerConst();
  
  nsresult MatchFloatingPointConst();
  
  nsresult MatchFractConst();
  
  nsresult MatchExponent();
  bool IsTokenExponentStarter();
  
  nsresult MatchDigitSeq();
  bool IsTokenDigitSeqStarter();
  
  nsresult MatchWsp();
  bool IsTokenWspStarter();

  nsresult MatchLeftParen();
  nsresult MatchRightParen();
};


#endif 
