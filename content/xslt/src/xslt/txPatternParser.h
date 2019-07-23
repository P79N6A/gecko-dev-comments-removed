





































#ifndef TX_PATTERNPARSER_H
#define TX_PATTERNPARSER_H

#include "txXSLTPatterns.h"
#include "txExprParser.h"

class txPatternParser : public txExprParser
{
public:
    static txPattern* createPattern(const nsAFlatString& aPattern,
                                    txIParseContext* aContext);
protected:
    static nsresult createUnionPattern(txExprLexer& aLexer,
                                       txIParseContext* aContext,
                                       txPattern*& aPattern);
    static nsresult createLocPathPattern(txExprLexer& aLexer,
                                         txIParseContext* aContext,
                                         txPattern*& aPattern);
    static nsresult createIdPattern(txExprLexer& aLexer,
                                    txPattern*& aPattern);
    static nsresult createKeyPattern(txExprLexer& aLexer,
                                     txIParseContext* aContext,
                                     txPattern*& aPattern);
    static nsresult createStepPattern(txExprLexer& aLexer,
                                      txIParseContext* aContext,
                                      txPattern*& aPattern);
};

#endif 
