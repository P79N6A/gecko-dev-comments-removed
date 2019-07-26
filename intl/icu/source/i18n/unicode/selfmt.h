














#ifndef SELFMT
#define SELFMT

#include "unicode/messagepattern.h"
#include "unicode/numfmt.h"
#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

class MessageFormat;






















































































































































class U_I18N_API SelectFormat : public Format {
public:

    







    SelectFormat(const UnicodeString& pattern, UErrorCode& status);

    



    SelectFormat(const SelectFormat& other);

    



    virtual ~SelectFormat();

    










    void applyPattern(const UnicodeString& pattern, UErrorCode& status);


    using Format::format;

    












    UnicodeString& format(const UnicodeString& keyword,
                            UnicodeString& appendTo,
                            FieldPosition& pos,
                            UErrorCode& status) const;

    





    SelectFormat& operator=(const SelectFormat& other);

    






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

private:
    friend class MessageFormat;

    SelectFormat();   

    







    static int32_t findSubMessage(const MessagePattern& pattern, int32_t partIndex,
                                  const UnicodeString& keyword, UErrorCode& ec);

    MessagePattern msgPattern;
};

U_NAMESPACE_END

#endif 

#endif 

