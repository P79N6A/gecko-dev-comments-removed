








#ifndef SkBML_WXMLParser_DEFINED
#define SkBML_WXMLParser_DEFINED

#include "SkString.h"
#include "SkXMLParser.h"

class SkStream;
class SkWStream;

class BML_WXMLParser : public SkXMLParser {
public:
    BML_WXMLParser(SkWStream& writer);
    virtual ~BML_WXMLParser();
    static void Write(SkStream& s, const char filename[]);
  
  
  SkDEBUGCODE(static void UnitTest();)
    
private:
    virtual bool onAddAttribute(const char name[], const char value[]);
    virtual bool onEndElement(const char name[]);
    virtual bool onStartElement(const char name[]);
    BML_WXMLParser& operator=(const BML_WXMLParser& src);
#ifdef SK_DEBUG
    int fElemsCount, fElemsReused;
    int fAttrsCount, fNamesReused, fValuesReused;
#endif
    SkWStream&  fWriter;
    char*       fElems[256];
    char*       fAttrNames[256];
    char*       fAttrValues[256];

    
    U8  fNextElem, fNextAttrName, fNextAttrValue;
};

#endif 

