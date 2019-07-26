













#ifndef __MESSAGEIMPL_H__
#define __MESSAGEIMPL_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messagepattern.h"

U_NAMESPACE_BEGIN






class U_COMMON_API MessageImpl {
public:
    


    static UBool jdkAposMode(const MessagePattern &msgPattern) {
        return msgPattern.getApostropheMode()==UMSGPAT_APOS_DOUBLE_REQUIRED;
    }

    



    static void appendReducedApostrophes(const UnicodeString &s, int32_t start, int32_t limit,
                                         UnicodeString &sb);

    



    static UnicodeString &appendSubMessageWithoutSkipSyntax(const MessagePattern &msgPattern,
                                                            int32_t msgStart,
                                                            UnicodeString &result);

private:
    MessageImpl();  
};

U_NAMESPACE_END

#endif  

#endif  
