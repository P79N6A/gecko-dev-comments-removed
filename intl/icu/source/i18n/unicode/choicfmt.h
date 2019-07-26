




















#ifndef CHOICFMT_H
#define CHOICFMT_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING
#ifndef U_HIDE_DEPRECATED_API

#include "unicode/fieldpos.h"
#include "unicode/format.h"
#include "unicode/messagepattern.h"
#include "unicode/numfmt.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class MessageFormat;































































































































class U_I18N_API ChoiceFormat: public NumberFormat {
public:
    







    ChoiceFormat(const UnicodeString& pattern,
                 UErrorCode& status);


    











    ChoiceFormat(const double* limits,
                 const UnicodeString* formats,
                 int32_t count );

    













    ChoiceFormat(const double* limits,
                 const UBool* closures,
                 const UnicodeString* formats,
                 int32_t count);

    





    ChoiceFormat(const ChoiceFormat& that);

    





    const ChoiceFormat& operator=(const ChoiceFormat& that);

    



    virtual ~ChoiceFormat();

    






    virtual Format* clone(void) const;

    







    virtual UBool operator==(const Format& other) const;

    







    virtual void applyPattern(const UnicodeString& pattern,
                              UErrorCode& status);

    









    virtual void applyPattern(const UnicodeString& pattern,
                             UParseError& parseError,
                             UErrorCode& status);
    







    virtual UnicodeString& toPattern(UnicodeString &pattern) const;

    












    virtual void setChoices(const double* limitsToCopy,
                            const UnicodeString* formatsToCopy,
                            int32_t count );

    









    virtual void setChoices(const double* limits,
                            const UBool* closures,
                            const UnicodeString* formats,
                            int32_t count);

    







    virtual const double* getLimits(int32_t& count) const;

    







    virtual const UBool* getClosures(int32_t& count) const;

    







    virtual const UnicodeString* getFormats(int32_t& count) const;


    using NumberFormat::format;

    










    virtual UnicodeString& format(double number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;
    










    virtual UnicodeString& format(int32_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    










    virtual UnicodeString& format(int64_t number,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos) const;

    













    virtual UnicodeString& format(const Formattable* objs,
                                  int32_t cnt,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& success) const;
    













    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    










    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    










    UnicodeString& format(  double number,
                            UnicodeString& appendTo) const;

    










    UnicodeString& format(  int32_t number,
                            UnicodeString& appendTo) const;

   













    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       ParsePosition& parsePosition) const;

    











    virtual void parse(const UnicodeString& text,
                       Formattable& result,
                       UErrorCode& status) const;

    







    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

private:
    





    static UnicodeString& dtos(double value, UnicodeString& string);

    ChoiceFormat(); 

    









    ChoiceFormat(const UnicodeString& newPattern,
                 UParseError& parseError,
                 UErrorCode& status);

    friend class MessageFormat;

    virtual void setChoices(const double* limits,
                            const UBool* closures,
                            const UnicodeString* formats,
                            int32_t count,
                            UErrorCode &errorCode);

    






    static int32_t findSubMessage(const MessagePattern &pattern, int32_t partIndex, double number);

    static double parseArgument(
            const MessagePattern &pattern, int32_t partIndex,
            const UnicodeString &source, ParsePosition &pos);

    







    static int32_t matchStringUntilLimitPart(
            const MessagePattern &pattern, int32_t partIndex, int32_t limitPartIndex,
            const UnicodeString &source, int32_t sourceOffset);

    





    UErrorCode constructorErrorCode;

    






    MessagePattern msgPattern;

    






































    
    
    
    
};

inline UnicodeString&
ChoiceFormat::format(const Formattable& obj,
                     UnicodeString& appendTo,
                     UErrorCode& status) const {
    
    
    return NumberFormat::format(obj, appendTo, status);
}

inline UnicodeString&
ChoiceFormat::format(double number,
                     UnicodeString& appendTo) const {
    return NumberFormat::format(number, appendTo);
}

inline UnicodeString&
ChoiceFormat::format(int32_t number,
                     UnicodeString& appendTo) const {
    return NumberFormat::format(number, appendTo);
}
U_NAMESPACE_END

#endif  
#endif 

#endif 

