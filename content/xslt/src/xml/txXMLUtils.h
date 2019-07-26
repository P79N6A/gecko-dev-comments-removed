








#ifndef MITRE_XMLUTILS_H
#define MITRE_XMLUTILS_H

#include "txCore.h"
#include "nsCOMPtr.h"
#include "nsDependentSubstring.h"
#include "nsIAtom.h"
#include "txXPathNode.h"

#define kExpatSeparatorChar 0xFFFF

extern "C" int MOZ_XMLIsLetter(const char* ptr);
extern "C" int MOZ_XMLIsNCNameChar(const char* ptr);

class nsIAtom;
class txNamespaceMap;

class txExpandedName {
public:
    txExpandedName() : mNamespaceID(kNameSpaceID_None)
    {
    }

    txExpandedName(int32_t aNsID,
                   nsIAtom* aLocalName) : mNamespaceID(aNsID),
                                          mLocalName(aLocalName)
    {
    }

    txExpandedName(const txExpandedName& aOther) :
        mNamespaceID(aOther.mNamespaceID),
        mLocalName(aOther.mLocalName)
    {
    }

    nsresult init(const nsAString& aQName, txNamespaceMap* aResolver,
                  bool aUseDefault);

    void reset()
    {
        mNamespaceID = kNameSpaceID_None;
        mLocalName = nullptr;
    }

    bool isNull()
    {
        return mNamespaceID == kNameSpaceID_None && !mLocalName;
    }

    txExpandedName& operator = (const txExpandedName& rhs)
    {
        mNamespaceID = rhs.mNamespaceID;
        mLocalName = rhs.mLocalName;
        return *this;
    }

    bool operator == (const txExpandedName& rhs) const
    {
        return ((mLocalName == rhs.mLocalName) &&
                (mNamespaceID == rhs.mNamespaceID));
    }

    bool operator != (const txExpandedName& rhs) const
    {
        return ((mLocalName != rhs.mLocalName) ||
                (mNamespaceID != rhs.mNamespaceID));
    }

    int32_t mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
};

class XMLUtils {

public:
    static nsresult splitExpatName(const PRUnichar *aExpatName,
                                   nsIAtom **aPrefix, nsIAtom **aLocalName,
                                   int32_t* aNameSpaceID);
    static nsresult splitQName(const nsAString& aName, nsIAtom** aPrefix,
                               nsIAtom** aLocalName);

    


    static bool isWhitespace(const PRUnichar& aChar)
    {
        return (aChar <= ' ' &&
                (aChar == ' ' || aChar == '\r' ||
                 aChar == '\n'|| aChar == '\t'));
    }

    


    static bool isWhitespace(const nsAFlatString& aText);

    


    static void normalizePIValue(nsAString& attValue);

    


    static bool isValidQName(const nsAFlatString& aQName,
                             const PRUnichar** aColon);

    


    static bool isLetter(PRUnichar aChar)
    {
        return !!MOZ_XMLIsLetter(reinterpret_cast<const char*>(&aChar));
    }   

    


    static bool isNCNameChar(PRUnichar aChar)
    {
        return !!MOZ_XMLIsNCNameChar(reinterpret_cast<const char*>(&aChar));
    }

    



    static bool getXMLSpacePreserve(const txXPathNode& aNode);
};

#endif
