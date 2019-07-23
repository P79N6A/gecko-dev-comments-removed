









































#ifndef MITRE_XMLUTILS_H
#define MITRE_XMLUTILS_H

#include "txCore.h"
#include "nsCOMPtr.h"
#include "nsDependentSubstring.h"
#include "nsIAtom.h"
#include "txXPathNode.h"

#ifndef TX_EXE
#include "nsIParserService.h"
#include "nsContentUtils.h"
#endif

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

    ~txExpandedName()
    {
    }
    
    nsresult init(const nsAString& aQName, txNamespaceMap* aResolver,
                  MBool aUseDefault);

    void reset()
    {
        mNamespaceID = kNameSpaceID_None;
        mLocalName = nsnull;
    }

    PRBool isNull()
    {
        return mNamespaceID == kNameSpaceID_None && !mLocalName;
    }

    txExpandedName& operator = (const txExpandedName& rhs)
    {
        mNamespaceID = rhs.mNamespaceID;
        mLocalName = rhs.mLocalName;
        return *this;
    }

    MBool operator == (const txExpandedName& rhs) const
    {
        return ((mLocalName == rhs.mLocalName) &&
                (mNamespaceID == rhs.mNamespaceID));
    }

    MBool operator != (const txExpandedName& rhs) const
    {
        return ((mLocalName != rhs.mLocalName) ||
                (mNamespaceID != rhs.mNamespaceID));
    }

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
};

#ifdef TX_EXE
extern "C" int MOZ_XMLCheckQName(const char* ptr, const char* end,
                                 int ns_aware, const char** colon);
extern "C" int MOZ_XMLIsLetter(const char* ptr);
extern "C" int MOZ_XMLIsNCNameChar(const char* ptr);
#endif

class XMLUtils {

public:
    static nsresult splitExpatName(const PRUnichar *aExpatName,
                                   nsIAtom **aPrefix, nsIAtom **aLocalName,
                                   PRInt32* aNameSpaceID);
    static nsresult splitQName(const nsAString& aName, nsIAtom** aPrefix,
                               nsIAtom** aLocalName);
    static const nsDependentSubstring getLocalPart(const nsAString& src);

    


    static MBool isWhitespace(const PRUnichar& aChar)
    {
        return (aChar <= ' ' &&
                (aChar == ' ' || aChar == '\r' ||
                 aChar == '\n'|| aChar == '\t'));
    }

    


    static PRBool isWhitespace(const nsAFlatString& aText);

    


    static void normalizePIValue(nsAString& attValue);

    


    static PRBool isValidQName(const nsAFlatString& aQName,
                               const PRUnichar** aColon)
    {
#ifdef TX_EXE
        const PRUnichar* end;
        aQName.EndReading(end);

        const char *colonPtr;
        int result = MOZ_XMLCheckQName(NS_REINTERPRET_CAST(const char*,
                                                           aQName.get()),
                                       NS_REINTERPRET_CAST(const char*,
                                                           end),
                                       PR_TRUE, &colonPtr);
        *aColon = NS_REINTERPRET_CAST(const PRUnichar*, colonPtr);
        return result == 0;
#else
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && NS_SUCCEEDED(ps->CheckQName(aQName, PR_TRUE, aColon));
#endif
    }

    


    static PRBool isLetter(PRUnichar aChar)
    {
#ifdef TX_EXE
        return MOZ_XMLIsLetter(NS_REINTERPRET_CAST(const char*, &aChar));
#else
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && ps->IsXMLLetter(aChar);
#endif
    }

    


    static PRBool isNCNameChar(PRUnichar aChar)
    {
#ifdef TX_EXE
        return MOZ_XMLIsNCNameChar(NS_REINTERPRET_CAST(const char*, &aChar));
#else
        nsIParserService* ps = nsContentUtils::GetParserService();
        return ps && ps->IsXMLNCNameChar(aChar);
#endif
    }

    



    static MBool getXMLSpacePreserve(const txXPathNode& aNode);
};

#endif
