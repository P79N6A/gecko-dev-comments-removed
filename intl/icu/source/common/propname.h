










#ifndef PROPNAME_H
#define PROPNAME_H

#include "unicode/utypes.h"
#include "unicode/bytestrie.h"
#include "unicode/uchar.h"
#include "udataswp.h"
#include "uprops.h"











U_CDECL_BEGIN



















U_CAPI int32_t U_EXPORT2
uprv_compareASCIIPropertyNames(const char *name1, const char *name2);

U_CAPI int32_t U_EXPORT2
uprv_compareEBCDICPropertyNames(const char *name1, const char *name2);

#if U_CHARSET_FAMILY==U_ASCII_FAMILY
#   define uprv_comparePropertyNames uprv_compareASCIIPropertyNames
#elif U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   define uprv_comparePropertyNames uprv_compareEBCDICPropertyNames
#else
#   error U_CHARSET_FAMILY is not valid
#endif

U_CDECL_END



#define PNAME_DATA_NAME "pnames"
#define PNAME_DATA_TYPE "icu"




#define PNAME_SIG_0 ((uint8_t)0x70) /* p */
#define PNAME_SIG_1 ((uint8_t)0x6E) /* n */
#define PNAME_SIG_2 ((uint8_t)0x61) /* a */
#define PNAME_SIG_3 ((uint8_t)0x6D) /* m */

U_NAMESPACE_BEGIN

class PropNameData {
public:
    enum {
        
        IX_VALUE_MAPS_OFFSET,
        IX_BYTE_TRIES_OFFSET,
        IX_NAME_GROUPS_OFFSET,
        IX_RESERVED3_OFFSET,
        IX_RESERVED4_OFFSET,
        IX_TOTAL_SIZE,

        
        IX_MAX_NAME_LENGTH,
        IX_RESERVED7,
        IX_COUNT
    };

    static const char *getPropertyName(int32_t property, int32_t nameChoice);
    static const char *getPropertyValueName(int32_t property, int32_t value, int32_t nameChoice);

    static int32_t getPropertyEnum(const char *alias);
    static int32_t getPropertyValueEnum(int32_t property, const char *alias);

private:
    static int32_t findProperty(int32_t property);
    static int32_t findPropertyValueNameGroup(int32_t valueMapIndex, int32_t value);
    static const char *getName(const char *nameGroup, int32_t nameIndex);
    static UBool containsName(BytesTrie &trie, const char *name);

    static int32_t getPropertyOrValueEnum(int32_t bytesTrieOffset, const char *alias);

    static const int32_t indexes[];
    static const int32_t valueMaps[];
    static const uint8_t bytesTries[];
    static const char nameGroups[];
};


























































































U_NAMESPACE_END

#endif
