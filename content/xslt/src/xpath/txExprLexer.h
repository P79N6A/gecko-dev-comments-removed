







































#ifndef MITREXSL_EXPRLEXER_H
#define MITREXSL_EXPRLEXER_H

#include "txCore.h"
#include "nsString.h"







class Token
{
public:

    


    enum Type {
        
        NULL_TOKEN = 1,
        LITERAL,
        NUMBER,
        CNAME,
        VAR_REFERENCE,
        PARENT_NODE,
        SELF_NODE,
        R_PAREN,
        R_BRACKET, 
        




        COMMA,
        AT_SIGN,
        L_PAREN,
        L_BRACKET,
        AXIS_IDENTIFIER,

        
        FUNCTION_NAME_AND_PAREN, 
        COMMENT_AND_PAREN,
        NODE_AND_PAREN,
        PROC_INST_AND_PAREN,
        TEXT_AND_PAREN,

        


        
        AND_OP, 
        OR_OP,

        
        EQUAL_OP, 
        NOT_EQUAL_OP,
        LESS_THAN_OP,
        GREATER_THAN_OP,
        LESS_OR_EQUAL_OP,
        GREATER_OR_EQUAL_OP,
        
        ADDITION_OP, 
        SUBTRACTION_OP,
        
        DIVIDE_OP, 
        MULTIPLY_OP,
        MODULUS_OP,
        
        PARENT_OP, 
        ANCESTOR_OP,
        UNION_OP,
        


        
        END 
    };


    


    typedef nsASingleFragmentString::const_char_iterator iterator;

    Token(iterator aStart, iterator aEnd, Type aType)
        : mStart(aStart),
          mEnd(aEnd),
          mType(aType),
          mNext(nsnull),
          mPrevious(nsnull)
    {
    }
    Token(iterator aChar, Type aType)
        : mStart(aChar),
          mEnd(aChar + 1),
          mType(aType),
          mNext(nsnull),
          mPrevious(nsnull)
    {
    }

    const nsDependentSubstring Value()
    {
        return Substring(mStart, mEnd);
    }

    iterator mStart, mEnd;
    Type mType;
    Token* mNext;
    
    Token* mPrevious;
};








class txExprLexer
{
public:

    txExprLexer();
    ~txExprLexer();

    






    nsresult parse(const nsASingleFragmentString& aPattern);

    typedef nsASingleFragmentString::const_char_iterator iterator;
    iterator mPosition;

    



    Token* nextToken();
    Token* peek()
    {
        return mCurrentItem;
    }
    void pushBack();
    PRBool hasMoreTokens()
    {
        return (mCurrentItem->mType != Token::END);
    }

    


    
    enum _TrivialTokens {
        D_QUOTE        = '\"',
        S_QUOTE        = '\'',
        L_PAREN        = '(',
        R_PAREN        = ')',
        L_BRACKET      = '[',
        R_BRACKET      = ']',
        L_ANGLE        = '<',
        R_ANGLE        = '>',
        COMMA          = ',',
        PERIOD         = '.',
        ASTERIX        = '*',
        FORWARD_SLASH  = '/',
        EQUAL          = '=',
        BANG           = '!',
        VERT_BAR       = '|',
        AT_SIGN        = '@',
        DOLLAR_SIGN    = '$',
        PLUS           = '+',
        HYPHEN         = '-',
        COLON          = ':',
        
        SPACE          = ' ',
        TX_TAB            = '\t',
        TX_CR             = '\n',
        TX_LF             = '\r'
    };

private:

    Token* mCurrentItem;
    Token* mFirstItem;
    Token* mLastItem;

    int mTokenCount;

    void addToken(Token* aToken);

    




    PRBool nextIsOperatorToken(Token* aToken);

    



    static PRBool isXPathDigit(PRUnichar ch)
    {
        return (ch >= '0' && ch <= '9');
    }
};

#endif

