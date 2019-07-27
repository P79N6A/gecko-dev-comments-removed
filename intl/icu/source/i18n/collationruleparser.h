










#ifndef __COLLATIONRULEPARSER_H__
#define __COLLATIONRULEPARSER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"

struct UParseError;

U_NAMESPACE_BEGIN

struct CollationData;
struct CollationTailoring;

class Locale;
class Normalizer2;

struct CollationSettings;

class U_I18N_API CollationRuleParser : public UMemory {
public:
    
    enum Position {
        FIRST_TERTIARY_IGNORABLE,
        LAST_TERTIARY_IGNORABLE,
        FIRST_SECONDARY_IGNORABLE,
        LAST_SECONDARY_IGNORABLE,
        FIRST_PRIMARY_IGNORABLE,
        LAST_PRIMARY_IGNORABLE,
        FIRST_VARIABLE,
        LAST_VARIABLE,
        FIRST_REGULAR,
        LAST_REGULAR,
        FIRST_IMPLICIT,
        LAST_IMPLICIT,
        FIRST_TRAILING,
        LAST_TRAILING
    };

    





    static const UChar POS_LEAD = 0xfffe;
    




    static const UChar POS_BASE = 0x2800;

    class U_I18N_API Sink : public UObject {
    public:
        virtual ~Sink();
        




        virtual void addReset(int32_t strength, const UnicodeString &str,
                              const char *&errorReason, UErrorCode &errorCode) = 0;
        


        virtual void addRelation(int32_t strength, const UnicodeString &prefix,
                                 const UnicodeString &str, const UnicodeString &extension,
                                 const char *&errorReason, UErrorCode &errorCode) = 0;

        virtual void suppressContractions(const UnicodeSet &set, const char *&errorReason,
                                          UErrorCode &errorCode);

        virtual void optimize(const UnicodeSet &set, const char *&errorReason,
                              UErrorCode &errorCode);
    };

    class U_I18N_API Importer : public UObject {
    public:
        virtual ~Importer();
        virtual void getRules(
                const char *localeID, const char *collationType,
                UnicodeString &rules,
                const char *&errorReason, UErrorCode &errorCode) = 0;
    };

    




    CollationRuleParser(const CollationData *base, UErrorCode &errorCode);
    ~CollationRuleParser();

    



    void setSink(Sink *sinkAlias) {
        sink = sinkAlias;
    }

    



    void setImporter(Importer *importerAlias) {
        importer = importerAlias;
    }

    void parse(const UnicodeString &ruleString,
               CollationSettings &outSettings,
               UParseError *outParseError,
               UErrorCode &errorCode);

    const char *getErrorReason() const { return errorReason; }

    




    static int32_t getReorderCode(const char *word);

private:
    
    static const int32_t STRENGTH_MASK = 0xf;
    static const int32_t STARRED_FLAG = 0x10;
    static const int32_t OFFSET_SHIFT = 8;

    void parse(const UnicodeString &ruleString, UErrorCode &errorCode);
    void parseRuleChain(UErrorCode &errorCode);
    int32_t parseResetAndPosition(UErrorCode &errorCode);
    int32_t parseRelationOperator(UErrorCode &errorCode);
    void parseRelationStrings(int32_t strength, int32_t i, UErrorCode &errorCode);
    void parseStarredCharacters(int32_t strength, int32_t i, UErrorCode &errorCode);
    int32_t parseTailoringString(int32_t i, UnicodeString &raw, UErrorCode &errorCode);
    int32_t parseString(int32_t i, UnicodeString &raw, UErrorCode &errorCode);

    



    int32_t parseSpecialPosition(int32_t i, UnicodeString &str, UErrorCode &errorCode);
    void parseSetting(UErrorCode &errorCode);
    void parseReordering(const UnicodeString &raw, UErrorCode &errorCode);
    static UColAttributeValue getOnOffValue(const UnicodeString &s);

    int32_t parseUnicodeSet(int32_t i, UnicodeSet &set, UErrorCode &errorCode);
    int32_t readWords(int32_t i, UnicodeString &raw) const;
    int32_t skipComment(int32_t i) const;

    void setParseError(const char *reason, UErrorCode &errorCode);
    void setErrorContext();

    



    static UBool isSyntaxChar(UChar32 c);
    int32_t skipWhiteSpace(int32_t i) const;

    const Normalizer2 &nfd, &nfc;

    const UnicodeString *rules;
    const CollationData *const baseData;
    CollationSettings *settings;
    UParseError *parseError;
    const char *errorReason;

    Sink *sink;
    Importer *importer;

    int32_t ruleIndex;
};

U_NAMESPACE_END

#endif
#endif
