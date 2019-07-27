








#ifndef MITRE_XMLUTILS_H
#define MITRE_XMLUTILS_H

#include "txCore.h"
#include "nsDependentSubstring.h"
#include "txXPathNode.h"

#define kExpatSeparatorChar 0xFFFF

extern "C" int MOZ_XMLIsLetter(const char* ptr);
extern "C" int MOZ_XMLIsNCNameChar(const char* ptr);

class nsIAtom;

class XMLUtils {

public:
    static nsresult splitExpatName(const char16_t *aExpatName,
                                   nsIAtom **aPrefix, nsIAtom **aLocalName,
                                   int32_t* aNameSpaceID);
    static nsresult splitQName(const nsAString& aName, nsIAtom** aPrefix,
                               nsIAtom** aLocalName);

    


    static bool isWhitespace(const char16_t& aChar)
    {
        return (aChar <= ' ' &&
                (aChar == ' ' || aChar == '\r' ||
                 aChar == '\n'|| aChar == '\t'));
    }

    


    static bool isWhitespace(const nsAFlatString& aText);

    


    static void normalizePIValue(nsAString& attValue);

    


    static bool isValidQName(const nsAFlatString& aQName,
                             const char16_t** aColon);

    


    static bool isLetter(char16_t aChar)
    {
        return !!MOZ_XMLIsLetter(reinterpret_cast<const char*>(&aChar));
    }   

    


    static bool isNCNameChar(char16_t aChar)
    {
        return !!MOZ_XMLIsNCNameChar(reinterpret_cast<const char*>(&aChar));
    }

    



    static bool getXMLSpacePreserve(const txXPathNode& aNode);
};

#endif
