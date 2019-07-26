




















#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/regex.h"
#include "uvector.h"
#include "hash.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_CONVERSION

enum UXMLNodeType {
    
    UXML_NODE_TYPE_STRING,
    
    UXML_NODE_TYPE_ELEMENT,
    UXML_NODE_TYPE_COUNT
};

U_NAMESPACE_BEGIN

class UXMLParser;




class U_TOOLUTIL_API UXMLElement : public UObject {
public:
    


    virtual ~UXMLElement();

    


    const UnicodeString &getTagName() const;
    






    UnicodeString getText(UBool recurse) const;
    


    int32_t countAttributes() const;
    







    const UnicodeString *getAttribute(int32_t i, UnicodeString &name, UnicodeString &value) const;
    





    const UnicodeString *getAttribute(const UnicodeString &name) const;
    


    int32_t countChildren() const;
    





    const UObject *getChild(int32_t i, UXMLNodeType &type) const;
    




    const UXMLElement *nextChildElement(int32_t &i) const;
    







    const UXMLElement *getChildElement(const UnicodeString &name) const;

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

private:
    
    UXMLElement();
    UXMLElement(const UXMLElement &other);
    UXMLElement &operator=(const UXMLElement &other);

    void appendText(UnicodeString &text, UBool recurse) const;

    friend class UXMLParser;

    UXMLElement(const UXMLParser *parser, const UnicodeString *name, UErrorCode &errorCode);

    const UXMLParser *fParser;
    const UnicodeString *fName;          
    UnicodeString       fContent;        
                                         
                                         
                                         
                                         
                                         
    UVector             fAttNames;       
                                         
    UVector             fAttValues;      
                                         
                                         

    UVector             fChildren;       

    UXMLElement        *fParent;         
};








class U_TOOLUTIL_API UXMLParser : public UObject {
public:
    


    static UXMLParser *createParser(UErrorCode &errorCode);
    


    virtual ~UXMLParser();

    




    UXMLElement *parse(const UnicodeString &src, UErrorCode &errorCode);
    




    UXMLElement *parseFile(const char *filename, UErrorCode &errorCode);

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

private:
    
    UXMLParser();
    UXMLParser(const UXMLParser &other);
    UXMLParser &operator=(const UXMLParser &other);

    
    UXMLParser(UErrorCode &status);

    void           parseMisc(UErrorCode &status);
    UXMLElement   *createElement(RegexMatcher &mEl, UErrorCode &status);
    void           error(const char *message, UErrorCode &status);
    UnicodeString  scanContent(UErrorCode &status);
    void           replaceCharRefs(UnicodeString &s, UErrorCode &status);

    const UnicodeString *intern(const UnicodeString &s, UErrorCode &errorCode);
public:
    
    const UnicodeString *findName(const UnicodeString &s) const;
private:

    
    
    RegexMatcher mXMLDecl;
    RegexMatcher mXMLComment;
    RegexMatcher mXMLSP;
    RegexMatcher mXMLDoctype;
    RegexMatcher mXMLPI;
    RegexMatcher mXMLElemStart;
    RegexMatcher mXMLElemEnd;
    RegexMatcher mXMLElemEmpty;
    RegexMatcher mXMLCharData;
    RegexMatcher mAttrValue;
    RegexMatcher mAttrNormalizer;
    RegexMatcher mNewLineNormalizer;
    RegexMatcher mAmps;

    Hashtable             fNames;           
    UStack                fElementStack;    
                                            
                                            
    int32_t               fPos;             
                                            
    UnicodeString         fOneLF;
};

U_NAMESPACE_END
#endif 

#endif
