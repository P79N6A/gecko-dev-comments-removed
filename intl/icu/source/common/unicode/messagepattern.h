













#ifndef __MESSAGEPATTERN_H__
#define __MESSAGEPATTERN_H__






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/parseerr.h"
#include "unicode/unistr.h"





































enum UMessagePatternApostropheMode {
    










    UMSGPAT_APOS_DOUBLE_OPTIONAL,
    







    UMSGPAT_APOS_DOUBLE_REQUIRED
};



typedef enum UMessagePatternApostropheMode UMessagePatternApostropheMode;





enum UMessagePatternPartType {
    








    UMSGPAT_PART_TYPE_MSG_START,
    







    UMSGPAT_PART_TYPE_MSG_LIMIT,
    






    UMSGPAT_PART_TYPE_SKIP_SYNTAX,
    





    UMSGPAT_PART_TYPE_INSERT_CHAR,
    






    UMSGPAT_PART_TYPE_REPLACE_NUMBER,
    









    UMSGPAT_PART_TYPE_ARG_START,
    





    UMSGPAT_PART_TYPE_ARG_LIMIT,
    



    UMSGPAT_PART_TYPE_ARG_NUMBER,
    




    UMSGPAT_PART_TYPE_ARG_NAME,
    




    UMSGPAT_PART_TYPE_ARG_TYPE,
    




    UMSGPAT_PART_TYPE_ARG_STYLE,
    




    UMSGPAT_PART_TYPE_ARG_SELECTOR,
    





    UMSGPAT_PART_TYPE_ARG_INT,
    






    UMSGPAT_PART_TYPE_ARG_DOUBLE
};



typedef enum UMessagePatternPartType UMessagePatternPartType;









enum UMessagePatternArgType {
    



    UMSGPAT_ARG_TYPE_NONE,
    




    UMSGPAT_ARG_TYPE_SIMPLE,
    




    UMSGPAT_ARG_TYPE_CHOICE,
    








    UMSGPAT_ARG_TYPE_PLURAL,
    



    UMSGPAT_ARG_TYPE_SELECT,
    




    UMSGPAT_ARG_TYPE_SELECTORDINAL
};



typedef enum UMessagePatternArgType UMessagePatternArgType;






#define UMSGPAT_ARG_TYPE_HAS_PLURAL_STYLE(argType) \
    ((argType)==UMSGPAT_ARG_TYPE_PLURAL || (argType)==UMSGPAT_ARG_TYPE_SELECTORDINAL)

enum {
    




    UMSGPAT_ARG_NAME_NOT_NUMBER=-1,

    






    UMSGPAT_ARG_NAME_NOT_VALID=-2
};







#define UMSGPAT_NO_NUMERIC_VALUE ((double)(-123456789))

U_NAMESPACE_BEGIN

class MessagePatternDoubleList;
class MessagePatternPartsList;

























































class U_COMMON_API MessagePattern : public UObject {
public:
    







    MessagePattern(UErrorCode &errorCode);

    








    MessagePattern(UMessagePatternApostropheMode mode, UErrorCode &errorCode);

    

















    MessagePattern(const UnicodeString &pattern, UParseError *parseError, UErrorCode &errorCode);

    




    MessagePattern(const MessagePattern &other);

    





    MessagePattern &operator=(const MessagePattern &other);

    



    virtual ~MessagePattern();

    
















    MessagePattern &parse(const UnicodeString &pattern,
                          UParseError *parseError, UErrorCode &errorCode);

    
















    MessagePattern &parseChoiceStyle(const UnicodeString &pattern,
                                     UParseError *parseError, UErrorCode &errorCode);

    
















    MessagePattern &parsePluralStyle(const UnicodeString &pattern,
                                     UParseError *parseError, UErrorCode &errorCode);

    
















    MessagePattern &parseSelectStyle(const UnicodeString &pattern,
                                     UParseError *parseError, UErrorCode &errorCode);

    




    void clear();

    





    void clearPatternAndSetApostropheMode(UMessagePatternApostropheMode mode) {
        clear();
        aposMode=mode;
    }

    




    UBool operator==(const MessagePattern &other) const;

    




    inline UBool operator!=(const MessagePattern &other) const {
        return !operator==(other);
    }

    



    int32_t hashCode() const;

    



    UMessagePatternApostropheMode getApostropheMode() const {
        return aposMode;
    }

    
    

    



    const UnicodeString &getPatternString() const {
        return msg;
    }

    




    UBool hasNamedArguments() const {
        return hasArgNames;
    }

    




    UBool hasNumberedArguments() const {
        return hasArgNumbers;
    }

    










    static int32_t validateArgumentName(const UnicodeString &name);

    









    UnicodeString autoQuoteApostropheDeep() const;

    class Part;

    





    int32_t countParts() const {
        return partsLength;
    }

    





    const Part &getPart(int32_t i) const {
        return parts[i];
    }

    






    UMessagePatternPartType getPartType(int32_t i) const {
        return getPart(i).type;
    }

    






    int32_t getPatternIndex(int32_t partIndex) const {
        return getPart(partIndex).index;
    }

    






    UnicodeString getSubstring(const Part &part) const {
        return msg.tempSubString(part.index, part.length);
    }

    






    UBool partSubstringMatches(const Part &part, const UnicodeString &s) const {
        return 0==msg.compare(part.index, part.length, s);
    }

    





    double getNumericValue(const Part &part) const;

    





    double getPluralOffset(int32_t pluralStart) const;

    







    int32_t getLimitPartIndex(int32_t start) const {
        int32_t limit=getPart(start).limitPartIndex;
        if(limit<start) {
            return start;
        }
        return limit;
    }

    






    class Part : public UMemory {
    public:
        



        Part() {}

        




        UMessagePatternPartType getType() const {
            return type;
        }

        




        int32_t getIndex() const {
            return index;
        }

        





        int32_t getLength() const {
            return length;
        }

        





        int32_t getLimit() const {
            return index+length;
        }

        





        int32_t getValue() const {
            return value;
        }

        





        UMessagePatternArgType getArgType() const {
            UMessagePatternPartType type=getType();
            if(type==UMSGPAT_PART_TYPE_ARG_START || type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
                return (UMessagePatternArgType)value;
            } else {
                return UMSGPAT_ARG_TYPE_NONE;
            }
        }

        






        static UBool hasNumericValue(UMessagePatternPartType type) {
            return type==UMSGPAT_PART_TYPE_ARG_INT || type==UMSGPAT_PART_TYPE_ARG_DOUBLE;
        }

        




        UBool operator==(const Part &other) const;

        




        inline UBool operator!=(const Part &other) const {
            return !operator==(other);
        }

        



        int32_t hashCode() const {
            return ((type*37+index)*37+length)*37+value;
        }

    private:
        friend class MessagePattern;

        static const int32_t MAX_LENGTH=0xffff;
        static const int32_t MAX_VALUE=0x7fff;

        
        
        UMessagePatternPartType type;
        int32_t index;
        uint16_t length;
        int16_t value;
        int32_t limitPartIndex;
    };

private:
    void preParse(const UnicodeString &pattern, UParseError *parseError, UErrorCode &errorCode);

    void postParse();

    int32_t parseMessage(int32_t index, int32_t msgStartLength,
                         int32_t nestingLevel, UMessagePatternArgType parentType,
                         UParseError *parseError, UErrorCode &errorCode);

    int32_t parseArg(int32_t index, int32_t argStartLength, int32_t nestingLevel,
                     UParseError *parseError, UErrorCode &errorCode);

    int32_t parseSimpleStyle(int32_t index, UParseError *parseError, UErrorCode &errorCode);

    int32_t parseChoiceStyle(int32_t index, int32_t nestingLevel,
                             UParseError *parseError, UErrorCode &errorCode);

    int32_t parsePluralOrSelectStyle(UMessagePatternArgType argType, int32_t index, int32_t nestingLevel,
                                     UParseError *parseError, UErrorCode &errorCode);

    







    static int32_t parseArgNumber(const UnicodeString &s, int32_t start, int32_t limit);

    int32_t parseArgNumber(int32_t start, int32_t limit) {
        return parseArgNumber(msg, start, limit);
    }

    







    void parseDouble(int32_t start, int32_t limit, UBool allowInfinity,
                     UParseError *parseError, UErrorCode &errorCode);

    
    

    int32_t skipWhiteSpace(int32_t index);

    int32_t skipIdentifier(int32_t index);

    



    int32_t skipDouble(int32_t index);

    static UBool isArgTypeChar(UChar32 c);

    UBool isChoice(int32_t index);

    UBool isPlural(int32_t index);

    UBool isSelect(int32_t index);

    UBool isOrdinal(int32_t index);

    



    UBool inMessageFormatPattern(int32_t nestingLevel);

    



    UBool inTopLevelChoiceMessage(int32_t nestingLevel, UMessagePatternArgType parentType);

    void addPart(UMessagePatternPartType type, int32_t index, int32_t length,
                 int32_t value, UErrorCode &errorCode);

    void addLimitPart(int32_t start,
                      UMessagePatternPartType type, int32_t index, int32_t length,
                      int32_t value, UErrorCode &errorCode);

    void addArgDoublePart(double numericValue, int32_t start, int32_t length, UErrorCode &errorCode);

    void setParseError(UParseError *parseError, int32_t index);

    
    virtual UClassID getDynamicClassID() const;

    UBool init(UErrorCode &errorCode);
    UBool copyStorage(const MessagePattern &other, UErrorCode &errorCode);

    UMessagePatternApostropheMode aposMode;
    UnicodeString msg;
    
    MessagePatternPartsList *partsList;
    Part *parts;
    int32_t partsLength;
    
    MessagePatternDoubleList *numericValuesList;
    double *numericValues;
    int32_t numericValuesLength;
    UBool hasArgNames;
    UBool hasArgNumbers;
    UBool needsAutoQuoting;
};

U_NAMESPACE_END

#endif  

#endif  
