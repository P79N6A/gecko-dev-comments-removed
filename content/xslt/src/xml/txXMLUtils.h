









































#ifndef MITRE_XMLUTILS_H
#define MITRE_XMLUTILS_H

#include "txCore.h"
#include "nsCOMPtr.h"
#include "nsDependentSubstring.h"
#include "nsIAtom.h"
#include "txXPathNode.h"
#include "nsIParserService.h"
#include "nsContentUtils.h"

#define kExpatSeparatorChar 0xFFFF

class nsIAtom;
class txNamespaceMap;

class txExpandedName {
public:
    txExpandedName() : mNamespaceID(kNameSpaceID_None)
    {
    }

    txExpandedName(PRInt32 aNsID,
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
        mLocalName = nsnull;
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

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
};

class XMLUtils {

public:
    static nsresult splitExpatName(const PRUnichar *aExpatName,
                                   nsIAtom **aPrefix, nsIAtom **aLocalName,
                                   PRInt32* aNameSpaceID);
    static nsresult splitQName(const nsAString& aName, nsIAtom** aPrefix,
                               nsIAtom** aLocalName);
    static const nsDependentSubstring getLocalPart(const nsAString& src);

    


    static bool isWhitespace(const PRUnichar& aChar)
    {
        return (aChar <= ' ' &&
                (aChar == ' ' || aChar == '\r' ||
                 aChar == '\n'|| aChar == '\t'));
    }

    


    static bool isWhitespace(const nsAFlatString& aText);

    


    static void normalizePIValue(nsAString& attValue);

    


    static bool isValidQName(const nsAFlatString& aQName,
                               const PRUnichar** aColon)
    {
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && NS_SUCCEEDED(ps->CheckQName(aQName, true, aColon));
    }

    


    static bool isLetter(PRUnichar aChar)
    {
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && ps->IsXMLLetter(aChar);
    }

    


    static bool isNCNameChar(PRUnichar aChar)
    {
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && ps->IsXMLNCNameChar(aChar);
    }

    



    static bool getXMLSpacePreserve(const txXPathNode& aNode);
};

#endif
