





































#ifndef TRANSFRMX_OUTPUTFORMAT_H
#define TRANSFRMX_OUTPUTFORMAT_H

#include "txList.h"
#include "nsString.h"

enum txOutputMethod {
    eMethodNotSet,
    eXMLOutput,
    eHTMLOutput,
    eTextOutput
};

enum txThreeState {
    eNotSet,
    eFalse,
    eTrue
};

class txOutputFormat {
public:
    txOutputFormat();
    ~txOutputFormat();

    
    void reset();

    
    
    void merge(txOutputFormat& aOutputFormat);

    
    void setFromDefaults();

    
    txOutputMethod mMethod;

    
    
    nsString mVersion;

    
    
    nsString mEncoding;

    
    txThreeState mOmitXMLDeclaration;

    
    txThreeState mStandalone;

    
    nsString mPublicId;

    
    nsString mSystemId;

    
    txList mCDATASectionElements;

    
    txThreeState mIndent;

    
    nsString mMediaType;
};

#endif
