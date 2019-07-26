















#ifndef MSGFMT_H
#define MSGFMT_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/locid.h"
#include "unicode/messagepattern.h"
#include "unicode/parseerr.h"
#include "unicode/plurfmt.h"
#include "unicode/plurrule.h"

U_CDECL_BEGIN

struct UHashtable;
typedef struct UHashtable UHashtable;
U_CDECL_END

U_NAMESPACE_BEGIN

class AppendableWrapper;
class DateFormat;
class NumberFormat;




















































































































































































































































































class U_I18N_API MessageFormat : public Format {
public:
#ifndef U_HIDE_OBSOLETE_API
    




    enum EFormatNumber {
        




        kMaxFormat = 10
    };
#endif  

    








    MessageFormat(const UnicodeString& pattern,
                  UErrorCode &status);

    







    MessageFormat(const UnicodeString& pattern,
                  const Locale& newLocale,
                        UErrorCode& status);
    









    MessageFormat(const UnicodeString& pattern,
                  const Locale& newLocale,
                  UParseError& parseError,
                  UErrorCode& status);
    



    MessageFormat(const MessageFormat&);

    



    const MessageFormat& operator=(const MessageFormat&);

    



    virtual ~MessageFormat();

    




    virtual Format* clone(void) const;

    






    virtual UBool operator==(const Format& other) const;

    




    virtual void setLocale(const Locale& theLocale);

    





    virtual const Locale& getLocale(void) const;

    







    virtual void applyPattern(const UnicodeString& pattern,
                              UErrorCode& status);
    









    virtual void applyPattern(const UnicodeString& pattern,
                             UParseError& parseError,
                             UErrorCode& status);

    

















    virtual void applyPattern(const UnicodeString& pattern,
                              UMessagePatternApostropheMode aposMode,
                              UParseError* parseError,
                              UErrorCode& status);

    



    UMessagePatternApostropheMode getApostropheMode() const {
        return msgPattern.getApostropheMode();
    }

    







    virtual UnicodeString& toPattern(UnicodeString& appendTo) const;

    















    virtual void adoptFormats(Format** formatsToAdopt, int32_t count);

    













    virtual void setFormats(const Format** newFormats, int32_t cnt);


    













    virtual void adoptFormat(int32_t formatNumber, Format* formatToAdopt);

    








    virtual void setFormat(int32_t formatNumber, const Format& format);

    







    virtual StringEnumeration* getFormatNames(UErrorCode& status);

    












    virtual Format* getFormat(const UnicodeString& formatName, UErrorCode& status);

    












    virtual void setFormat(const UnicodeString& formatName, const Format& format, UErrorCode& status);

    













    virtual void adoptFormat(const UnicodeString& formatName, Format* formatToAdopt, UErrorCode& status);

    











    virtual const Format** getFormats(int32_t& count) const;


    using Format::format;

    
















    UnicodeString& format(const Formattable* source,
                          int32_t count,
                          UnicodeString& appendTo,
                          FieldPosition& ignore,
                          UErrorCode& status) const;

    
















    static UnicodeString& format(const UnicodeString& pattern,
                                 const Formattable* arguments,
                                 int32_t count,
                                 UnicodeString& appendTo,
                                 UErrorCode& status);

    



















    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    













    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;


    

















    UnicodeString& format(const UnicodeString* argumentNames,
                          const Formattable* arguments,
                          int32_t count,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;
    












    virtual Formattable* parse(const UnicodeString& source,
                               ParsePosition& pos,
                               int32_t& count) const;

    














    virtual Formattable* parse(const UnicodeString& source,
                               int32_t& count,
                               UErrorCode& status) const;

    











    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& pos) const;

    


















    static UnicodeString autoQuoteApostrophe(const UnicodeString& pattern,
        UErrorCode& status);


    






    UBool usesNamedArguments() const;


#ifndef U_HIDE_INTERNAL_API
    









    int32_t getArgTypeCount() const;
#endif  

    










    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

#ifndef U_HIDE_INTERNAL_API
    









    static UBool equalFormats(const void* left, const void* right);
#endif  

private:

    Locale              fLocale;
    MessagePattern      msgPattern;
    Format**            formatAliases; 
    int32_t             formatAliasesCapacity;

    MessageFormat(); 

     





    class U_I18N_API PluralSelectorProvider : public PluralFormat::PluralSelector {
    public:
        PluralSelectorProvider(const Locale* loc, UPluralType type);
        virtual ~PluralSelectorProvider();
        virtual UnicodeString select(double number, UErrorCode& ec) const;

        void reset(const Locale* loc);
    private:
        const Locale* locale;
        PluralRules* rules;
        UPluralType type;
    };

    







    Formattable::Type* argTypes;
    int32_t            argTypeCount;
    int32_t            argTypeCapacity;

    




    UBool hasArgTypeConflicts;

    
    UBool allocateArgTypes(int32_t capacity, UErrorCode& status);

    






    NumberFormat* defaultNumberFormat;
    DateFormat*   defaultDateFormat;

    UHashtable* cachedFormatters;
    UHashtable* customFormatArgStarts;

    PluralSelectorProvider pluralProvider;
    PluralSelectorProvider ordinalProvider;

    



    const NumberFormat* getDefaultNumberFormat(UErrorCode&) const;
    const DateFormat*   getDefaultDateFormat(UErrorCode&) const;

    





    static int32_t findKeyword( const UnicodeString& s,
                                const UChar * const *list);

    




    UnicodeString& format(const Formattable* arguments,
                          const UnicodeString *argumentNames,
                          int32_t cnt,
                          UnicodeString& appendTo,
                          FieldPosition* pos,
                          UErrorCode& status) const;

    
















    void format(int32_t msgStart,
                double pluralNumber,
                const Formattable* arguments,
                const UnicodeString *argumentNames,
                int32_t cnt,
                AppendableWrapper& appendTo,
                FieldPosition* pos,
                UErrorCode& success) const;

    UnicodeString getArgName(int32_t partIndex);

    void setArgStartFormat(int32_t argStart, Format* formatter, UErrorCode& status);

    void setCustomArgStartFormat(int32_t argStart, Format* formatter, UErrorCode& status);

    int32_t nextTopLevelArgStart(int32_t partIndex) const;

    UBool argNameMatches(int32_t partIndex, const UnicodeString& argName, int32_t argNumber);

    void cacheExplicitFormats(UErrorCode& status);

    Format* createAppropriateFormat(UnicodeString& type,
                                    UnicodeString& style,
                                    Formattable::Type& formattableType,
                                    UParseError& parseError,
                                    UErrorCode& ec);

    const Formattable* getArgFromListByName(const Formattable* arguments,
                                            const UnicodeString *argumentNames,
                                            int32_t cnt, UnicodeString& name) const;

    Formattable* parse(int32_t msgStart,
                       const UnicodeString& source,
                       ParsePosition& pos,
                       int32_t& count,
                       UErrorCode& ec) const;

    FieldPosition* updateMetaData(AppendableWrapper& dest, int32_t prevLength,
                                  FieldPosition* fp, const Formattable* argId) const;

    Format* getCachedFormatter(int32_t argumentNumber) const;

    UnicodeString getLiteralStringUntilNextArgument(int32_t from) const;

    void copyObjects(const MessageFormat& that, UErrorCode& ec);

    void formatComplexSubMessage(int32_t msgStart,
                                 double pluralNumber,
                                 const Formattable* arguments,
                                 const UnicodeString *argumentNames,
                                 int32_t cnt,
                                 AppendableWrapper& appendTo,
                                 UErrorCode& success) const;

    


    NumberFormat* createIntegerFormat(const Locale& locale, UErrorCode& status) const;

    







    const Formattable::Type* getArgTypeList(int32_t& listCount) const {
        listCount = argTypeCount;
        return argTypes;
    }

    


    void resetPattern();

    




    class U_I18N_API DummyFormat : public Format {
    public:
        virtual UBool operator==(const Format&) const;
        virtual Format* clone() const;
        virtual UnicodeString& format(const Formattable& obj,
                              UnicodeString& appendTo,
                              UErrorCode& status) const;
        virtual UnicodeString& format(const Formattable&,
                                      UnicodeString& appendTo,
                                      FieldPosition&,
                                      UErrorCode& status) const;
        virtual UnicodeString& format(const Formattable& obj,
                                      UnicodeString& appendTo,
                                      FieldPositionIterator* posIter,
                                      UErrorCode& status) const;
        virtual void parseObject(const UnicodeString&,
                                 Formattable&,
                                 ParsePosition&) const;
        virtual UClassID getDynamicClassID() const;
    };

    friend class MessageFormatAdapter; 
};

inline UnicodeString&
MessageFormat::format(const Formattable& obj,
                      UnicodeString& appendTo,
                      UErrorCode& status) const {
    return Format::format(obj, appendTo, status);
}


U_NAMESPACE_END

#endif 

#endif 

