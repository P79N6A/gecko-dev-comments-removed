













#ifndef FMTABLE_H
#define FMTABLE_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/stringpiece.h"






#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

class CharString;
class DigitList;





#if U_PLATFORM == U_PF_OS400
#define UNUM_INTERNAL_STACKARRAY_SIZE 144
#else
#define UNUM_INTERNAL_STACKARRAY_SIZE 128
#endif



















class U_I18N_API Formattable : public UObject {
public:
    








    enum ISDATE { kIsDate };

    



    Formattable(); 

    





    Formattable(UDate d, ISDATE flag);

    




    Formattable(double d);

    




    Formattable(int32_t l);

    




    Formattable(int64_t ll);

#if !UCONFIG_NO_CONVERSION
    





    Formattable(const char* strToCopy);
#endif

    












    Formattable(const StringPiece &number, UErrorCode &status);

    




    Formattable(const UnicodeString& strToCopy);

    




    Formattable(UnicodeString* strToAdopt);

    





    Formattable(const Formattable* arrayToCopy, int32_t count);

    




    Formattable(UObject* objectToAdopt);

    



    Formattable(const Formattable&);

    




    Formattable&    operator=(const Formattable &rhs);

    





    UBool          operator==(const Formattable &other) const;
    
    





    UBool          operator!=(const Formattable& other) const
      { return !operator==(other); }

    



    virtual         ~Formattable();

    










    Formattable *clone() const;

    





    enum Type {
        




        kDate,

        




        kDouble,

        




        kLong,

        




        kString,

        




        kArray,

        




        kInt64,

        




        kObject
   };

    




    Type            getType(void) const;
    
    





    UBool           isNumeric() const;
    
    




 
    double          getDouble(void) const { return fValue.fDouble; }

    










 
    double          getDouble(UErrorCode& status) const;

    




 
    int32_t         getLong(void) const { return (int32_t)fValue.fInt64; }

    














 
    int32_t         getLong(UErrorCode& status) const;

    




 
    int64_t         getInt64(void) const { return fValue.fInt64; }

    













 
    int64_t         getInt64(UErrorCode& status) const;

    




 
    UDate           getDate() const { return fValue.fDate; }

    






 
     UDate          getDate(UErrorCode& status) const;

    





 
    UnicodeString&  getString(UnicodeString& result) const
      { result=*fValue.fString; return result; }

    







 
    UnicodeString&  getString(UnicodeString& result, UErrorCode& status) const;

    






    inline const UnicodeString& getString(void) const;

    







    const UnicodeString& getString(UErrorCode& status) const;

    





    inline UnicodeString& getString(void);

    







    UnicodeString& getString(UErrorCode& status);

    





 
    const Formattable* getArray(int32_t& count) const
      { count=fValue.fArrayAndCount.fCount; return fValue.fArrayAndCount.fArray; }

    







 
    const Formattable* getArray(int32_t& count, UErrorCode& status) const;

    







    Formattable&    operator[](int32_t index) { return fValue.fArrayAndCount.fArray[index]; }
       
    





    const UObject*  getObject() const;

    

















    StringPiece getDecimalNumber(UErrorCode &status);

     




 
    void            setDouble(double d);

    




 
    void            setLong(int32_t l);

    




 
    void            setInt64(int64_t ll);

    




 
    void            setDate(UDate d);

    




 
    void            setString(const UnicodeString& stringToCopy);

    





 
    void            setArray(const Formattable* array, int32_t count);

    




 
    void            adoptString(UnicodeString* stringToAdopt);

    



 
    void            adoptArray(Formattable* array, int32_t count);
       
    






    void            adoptObject(UObject* objectToAdopt);

    













    void             setDecimalNumber(const StringPiece &numberString,
                                      UErrorCode &status);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

#ifndef U_HIDE_DEPRECATED_API
    




 
    inline int32_t getLong(UErrorCode* status) const;
#endif  

#ifndef U_HIDE_INTERNAL_API
    







    DigitList *getDigitList() const { return fDecimalNum;}

    


    DigitList *getInternalDigitList();

    





    void adoptDigitList(DigitList *dl);
#endif  

private:
    



    void            dispose(void);

    


    void            init();

    UnicodeString* getBogus() const;

    union {
        UObject*        fObject;
        UnicodeString*  fString;
        double          fDouble;
        int64_t         fInt64;
        UDate           fDate;
        struct {
          Formattable*  fArray;
          int32_t       fCount;
        }               fArrayAndCount;
    } fValue;

    CharString           *fDecimalStr;

    DigitList            *fDecimalNum;

    char                fStackData[UNUM_INTERNAL_STACKARRAY_SIZE]; 

    Type                fType;
    UnicodeString       fBogus; 
};

inline UDate Formattable::getDate(UErrorCode& status) const {
    if (fType != kDate) {
        if (U_SUCCESS(status)) {
            status = U_INVALID_FORMAT_ERROR;
        }
        return 0;
    }
    return fValue.fDate;
}

inline const UnicodeString& Formattable::getString(void) const {
    return *fValue.fString;
}

inline UnicodeString& Formattable::getString(void) {
    return *fValue.fString;
}

#ifndef U_HIDE_DEPRECATED_API
inline int32_t Formattable::getLong(UErrorCode* status) const {
    return getLong(*status);
}
#endif


U_NAMESPACE_END

#endif

#endif

