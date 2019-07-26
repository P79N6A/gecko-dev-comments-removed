














#ifndef PLURFMT
#define PLURFMT

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/messagepattern.h"
#include "unicode/numfmt.h"
#include "unicode/plurrule.h"

U_NAMESPACE_BEGIN

class Hashtable;
















































































































class U_I18N_API PluralFormat : public Format {
public:

    







    PluralFormat(UErrorCode& status);

    








    PluralFormat(const Locale& locale, UErrorCode& status);

    








    PluralFormat(const PluralRules& rules, UErrorCode& status);

    










    PluralFormat(const Locale& locale, const PluralRules& rules, UErrorCode& status);

    









    PluralFormat(const Locale& locale, UPluralType type, UErrorCode& status);

    









    PluralFormat(const UnicodeString& pattern, UErrorCode& status);

    













    PluralFormat(const Locale& locale, const UnicodeString& pattern, UErrorCode& status);

    










    PluralFormat(const PluralRules& rules,
                 const UnicodeString& pattern,
                 UErrorCode& status);

    













    PluralFormat(const Locale& locale,
                 const PluralRules& rules,
                 const UnicodeString& pattern,
                 UErrorCode& status);

    












    PluralFormat(const Locale& locale,
                 UPluralType type,
                 const UnicodeString& pattern,
                 UErrorCode& status);

    



    PluralFormat(const PluralFormat& other);

    



    virtual ~PluralFormat();

    











    void applyPattern(const UnicodeString& pattern, UErrorCode& status);


    using Format::format;

    











    UnicodeString format(int32_t number, UErrorCode& status) const;

    











    UnicodeString format(double number, UErrorCode& status) const;

    















    UnicodeString& format(int32_t number,
                          UnicodeString& appendTo,
                          FieldPosition& pos,
                          UErrorCode& status) const;

    















    UnicodeString& format(double number,
                          UnicodeString& appendTo,
                          FieldPosition& pos,
                          UErrorCode& status) const;

    














    void setLocale(const Locale& locale, UErrorCode& status);

    








    void setNumberFormat(const NumberFormat* format, UErrorCode& status);

    





    PluralFormat& operator=(const PluralFormat& other);

    






    virtual UBool operator==(const Format& other) const;

    






    virtual UBool operator!=(const Format& other) const;

    




    virtual Format* clone(void) const;

    











   UnicodeString& format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPosition& pos,
                         UErrorCode& status) const;

   







   UnicodeString& toPattern(UnicodeString& appendTo);

   





















   virtual void parseObject(const UnicodeString& source,
                            Formattable& result,
                            ParsePosition& parse_pos) const;

    





    static UClassID U_EXPORT2 getStaticClassID(void);

    




     virtual UClassID getDynamicClassID() const;

#if (defined(__xlC__) && (__xlC__ < 0x0C00)) || (U_PLATFORM == U_PF_OS390) || (U_PLATFORM ==U_PF_OS400)




public:
#else
private:
#endif
     


    class U_I18N_API PluralSelector : public UMemory {
      public:
        virtual ~PluralSelector();
        






        virtual UnicodeString select(double number, UErrorCode& ec) const = 0;
    };

    


    class U_I18N_API PluralSelectorAdapter : public PluralSelector {
      public:
        PluralSelectorAdapter() : pluralRules(NULL) {
        }

        virtual ~PluralSelectorAdapter();

        virtual UnicodeString select(double number, UErrorCode& ) const;

        void reset();

        PluralRules* pluralRules;
    };

#if defined(__xlC__)

private:
#endif
    Locale  locale;
    MessagePattern msgPattern;
    NumberFormat*  numberFormat;
    double offset;
    PluralSelectorAdapter pluralRulesWrapper;

    PluralFormat();   
    void init(const PluralRules* rules, UPluralType type, UErrorCode& status);
    



    void copyObjects(const PluralFormat& other);

    









    static int32_t findSubMessage(
         const MessagePattern& pattern, int32_t partIndex,
         const PluralSelector& selector, double number, UErrorCode& ec);

    friend class MessageFormat;
};

U_NAMESPACE_END

#endif 

#endif 

