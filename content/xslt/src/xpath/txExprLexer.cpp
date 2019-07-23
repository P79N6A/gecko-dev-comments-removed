









































#include "txExprLexer.h"
#include "txAtoms.h"
#include "nsString.h"
#include "txError.h"
#include "txXMLUtils.h"




txExprLexer::txExprLexer()
  : mCurrentItem(nsnull),
    mFirstItem(nsnull),
    mLastItem(nsnull),
    mTokenCount(0)
{
}




txExprLexer::~txExprLexer()
{
  
  Token* tok = mFirstItem;
  while (tok) {
    Token* temp = tok->mNext;
    delete tok;
    tok = temp;
  }
  mCurrentItem = nsnull;
}

Token*
txExprLexer::nextToken()
{
  NS_ASSERTION(mCurrentItem, "nextToken called beyoned the end");
  Token* token = mCurrentItem;
  mCurrentItem = mCurrentItem->mNext;
  return token;
}

void
txExprLexer::pushBack()
{
  mCurrentItem = mCurrentItem ? mCurrentItem->mPrevious : mLastItem;
}

void
txExprLexer::addToken(Token* aToken)
{
  if (mLastItem) {
    aToken->mPrevious = mLastItem;
    mLastItem->mNext = aToken;
  }
  if (!mFirstItem) {
    mFirstItem = aToken;
    mCurrentItem = aToken;
  }
  mLastItem = aToken;
  ++mTokenCount;
}






PRBool
txExprLexer::nextIsOperatorToken(Token* aToken)
{
  if (!aToken || aToken->mType == Token::NULL_TOKEN) {
    return PR_FALSE;
  }
  
  return aToken->mType < Token::COMMA ||
    aToken->mType > Token::UNION_OP;

}




nsresult
txExprLexer::parse(const nsASingleFragmentString& aPattern)
{
  iterator start, end;
  start = aPattern.BeginReading(mPosition);
  aPattern.EndReading(end);

  
  
  Token nullToken(nsnull, nsnull, Token::NULL_TOKEN);

  Token::Type defType;
  Token* newToken = nsnull;
  Token* prevToken = &nullToken;
  PRBool isToken;

  while (mPosition < end) {

    defType = Token::CNAME;
    isToken = PR_TRUE;

    if (*mPosition == DOLLAR_SIGN) {
      if (++mPosition == end || !XMLUtils::isLetter(*mPosition)) {
        return NS_ERROR_XPATH_INVALID_VAR_NAME;
      }
      defType = Token::VAR_REFERENCE;
    } 
    
    

    if (XMLUtils::isLetter(*mPosition)) {
      
      
      
      start = mPosition;
      while (++mPosition < end && XMLUtils::isNCNameChar(*mPosition)) {
        
      }
      if (mPosition < end && *mPosition == COLON) {
        
        if (++mPosition == end) {
          return NS_ERROR_XPATH_UNEXPECTED_END;
        }
        if (XMLUtils::isLetter(*mPosition)) {
          while (++mPosition < end && XMLUtils::isNCNameChar(*mPosition)) {
            
          }
        }
        else if (*mPosition == '*' && defType != Token::VAR_REFERENCE) {
          
          ++mPosition;
        }
        else {
          --mPosition; 
        }
      }
      if (nextIsOperatorToken(prevToken)) {
        NS_ConvertUTF16toUTF8 opUTF8(Substring(start, mPosition));
        if (txXPathAtoms::_and->EqualsUTF8(opUTF8)) {
          defType = Token::AND_OP;
        }
        else if (txXPathAtoms::_or->EqualsUTF8(opUTF8)) {
          defType = Token::OR_OP;
        }
        else if (txXPathAtoms::mod->EqualsUTF8(opUTF8)) {
          defType = Token::MODULUS_OP;
        }
        else if (txXPathAtoms::div->EqualsUTF8(opUTF8)) {
          defType = Token::DIVIDE_OP;
        }
        else {
          
          
          return NS_ERROR_XPATH_OPERATOR_EXPECTED;
        }
      }
      newToken = new Token(start, mPosition, defType);
    }
    else if (isXPathDigit(*mPosition)) {
      start = mPosition;
      while (++mPosition < end && isXPathDigit(*mPosition)) {
        
      }
      if (mPosition < end && *mPosition == '.') {
        while (++mPosition < end && isXPathDigit(*mPosition)) {
          
        }
      }
      newToken = new Token(start, mPosition, Token::NUMBER);
    }
    else {
      switch (*mPosition) {
        
      case SPACE:
      case TX_TAB:
      case TX_CR:
      case TX_LF:
        ++mPosition;
        isToken = PR_FALSE;
        break;
      case S_QUOTE :
      case D_QUOTE :
        start = mPosition;
        while (++mPosition < end && *mPosition != *start) {
          
        }
        if (mPosition == end) {
          mPosition = start;
          return NS_ERROR_XPATH_UNCLOSED_LITERAL;
        }
        newToken = new Token(start + 1, mPosition, Token::LITERAL);
        ++mPosition;
        break;
      case PERIOD:
        
        if (++mPosition == end) {
          newToken = new Token(mPosition - 1, Token::SELF_NODE);
        }
        else if (isXPathDigit(*mPosition)) {
          start = mPosition - 1;
          while (++mPosition < end && isXPathDigit(*mPosition)) {
            
          }
          newToken = new Token(start, mPosition, Token::NUMBER);
        }
        else if (*mPosition == PERIOD) {
          ++mPosition;
          newToken = new Token(mPosition - 2, mPosition, Token::PARENT_NODE);
        }
        else {
          newToken = new Token(mPosition - 1, Token::SELF_NODE);
        }
        break;
      case COLON: 
        if (++mPosition >= end || *mPosition != COLON ||
            prevToken->mType != Token::CNAME) {
          return NS_ERROR_XPATH_BAD_COLON;
        }
        prevToken->mType = Token::AXIS_IDENTIFIER;
        ++mPosition;
        isToken = PR_FALSE;
        break;
      case FORWARD_SLASH :
        if (++mPosition < end && *mPosition == FORWARD_SLASH) {
          ++mPosition;
          newToken = new Token(mPosition - 2, mPosition, Token::ANCESTOR_OP);
        }
        else {
          newToken = new Token(mPosition - 1, Token::PARENT_OP);
        }
        break;
      case BANG : 
        if (++mPosition < end && *mPosition == EQUAL) {
          ++mPosition;
          newToken = new Token(mPosition - 2, mPosition, Token::NOT_EQUAL_OP);
          break;
        }
        
        return NS_ERROR_XPATH_BAD_BANG;
      case EQUAL:
        newToken = new Token(mPosition, Token::EQUAL_OP);
        ++mPosition;
        break;
      case L_ANGLE:
        if (++mPosition == end) {
          return NS_ERROR_XPATH_UNEXPECTED_END;
        }
        if (*mPosition == EQUAL) {
          ++mPosition;
          newToken = new Token(mPosition - 2, mPosition,
                               Token::LESS_OR_EQUAL_OP);
        }
        else {
          newToken = new Token(mPosition - 1, Token::LESS_THAN_OP);
        }
        break;
      case R_ANGLE:
        if (++mPosition == end) {
          return NS_ERROR_XPATH_UNEXPECTED_END;
        }
        if (*mPosition == EQUAL) {
          ++mPosition;
          newToken = new Token(mPosition - 2, mPosition,
                               Token::GREATER_OR_EQUAL_OP);
        }
        else {
          newToken = new Token(mPosition - 1, Token::GREATER_THAN_OP);
        }
        break;
      case HYPHEN :
        newToken = new Token(mPosition, Token::SUBTRACTION_OP);
        ++mPosition;
        break;
      case ASTERIX:
        if (nextIsOperatorToken(prevToken)) {
          newToken = new Token(mPosition, Token::MULTIPLY_OP);
        }
        else {
          newToken = new Token(mPosition, Token::CNAME);
        }
        ++mPosition;
        break;
      case L_PAREN:
        if (prevToken->mType == Token::CNAME) {
          NS_ConvertUTF16toUTF8 utf8Value(prevToken->Value());
          if (txXPathAtoms::comment->EqualsUTF8(utf8Value)) {
            prevToken->mType = Token::COMMENT_AND_PAREN;
          }
          else if (txXPathAtoms::node->EqualsUTF8(utf8Value)) {
            prevToken->mType = Token::NODE_AND_PAREN;
          }
          else if (txXPathAtoms::processingInstruction->EqualsUTF8(utf8Value)) {
            prevToken->mType = Token::PROC_INST_AND_PAREN;
          }
          else if (txXPathAtoms::text->EqualsUTF8(utf8Value)) {
            prevToken->mType = Token::TEXT_AND_PAREN;
          }
          else {
            prevToken->mType = Token::FUNCTION_NAME_AND_PAREN;
          }
          isToken = PR_FALSE;
        }
        else {
          newToken = new Token(mPosition, Token::L_PAREN);
        }
        ++mPosition;
        break;
      case R_PAREN:
        newToken = new Token(mPosition, Token::R_PAREN);
        ++mPosition;
        break;
      case L_BRACKET:
        newToken = new Token(mPosition, Token::L_BRACKET);
        ++mPosition;
        break;
      case R_BRACKET:
        newToken = new Token(mPosition, Token::R_BRACKET);
        ++mPosition;
        break;
      case COMMA:
        newToken = new Token(mPosition, Token::COMMA);
        ++mPosition;
        break;
      case AT_SIGN :
        newToken = new Token(mPosition, Token::AT_SIGN);
        ++mPosition;
        break;
      case PLUS:
        newToken = new Token(mPosition, Token::ADDITION_OP);
        ++mPosition;
        break;
      case VERT_BAR:
        newToken = new Token(mPosition, Token::UNION_OP);
        ++mPosition;
        break;
      default:
        
        return NS_ERROR_XPATH_ILLEGAL_CHAR;
      }
    }
    if (isToken) {
      NS_ENSURE_TRUE(newToken, NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_TRUE(newToken != mLastItem, NS_ERROR_FAILURE);
      prevToken = newToken;
      addToken(newToken);
    }
  }

  
  newToken = new Token(end, end, Token::END);
  if (!newToken) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  addToken(newToken);

  return NS_OK;
}
