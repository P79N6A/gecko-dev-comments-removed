




#ifndef TRANSFRMX_XSLT_FUNCTIONS_H
#define TRANSFRMX_XSLT_FUNCTIONS_H

#include "txExpr.h"
#include "txXMLUtils.h"
#include "nsAutoPtr.h"
#include "txNamespaceMap.h"

class txStylesheet;




class DocumentFunctionCall : public FunctionCall {

public:

    


    explicit DocumentFunctionCall(const nsAString& aBaseURI);

    TX_DECL_FUNCTION

private:
    nsString mBaseURI;
};




class txKeyFunctionCall : public FunctionCall {

public:

    


    explicit txKeyFunctionCall(txNamespaceMap* aMappings);

    TX_DECL_FUNCTION

private:
    nsRefPtr<txNamespaceMap> mMappings;
};




class txFormatNumberFunctionCall : public FunctionCall {

public:

    


    txFormatNumberFunctionCall(txStylesheet* aStylesheet, txNamespaceMap* aMappings);

    TX_DECL_FUNCTION

private:
    static const char16_t FORMAT_QUOTE;

    enum FormatParseState {
        Prefix,
        IntDigit,
        IntZero,
        FracZero,
        FracDigit,
        Suffix,
        Finished
    };
    
    txStylesheet* mStylesheet;
    nsRefPtr<txNamespaceMap> mMappings;
};





class txDecimalFormat {

public:
    



    txDecimalFormat();
    bool isEqual(txDecimalFormat* other);
    
    char16_t       mDecimalSeparator;
    char16_t       mGroupingSeparator;
    nsString        mInfinity;
    char16_t       mMinusSign;
    nsString        mNaN;
    char16_t       mPercent;
    char16_t       mPerMille;
    char16_t       mZeroDigit;
    char16_t       mDigit;
    char16_t       mPatternSeparator;
};




class CurrentFunctionCall : public FunctionCall {

public:

    


    CurrentFunctionCall();

    TX_DECL_FUNCTION
};




class GenerateIdFunctionCall : public FunctionCall {

public:

    


    GenerateIdFunctionCall();

    TX_DECL_FUNCTION
};





class txXSLTEnvironmentFunctionCall : public FunctionCall
{
public:
    enum eType { SYSTEM_PROPERTY, ELEMENT_AVAILABLE, FUNCTION_AVAILABLE };

    txXSLTEnvironmentFunctionCall(eType aType, txNamespaceMap* aMappings)
        : mType(aType),
          mMappings(aMappings)
    {
    }

    TX_DECL_FUNCTION

private:
    eType mType;
    nsRefPtr<txNamespaceMap> mMappings; 
};

#endif
