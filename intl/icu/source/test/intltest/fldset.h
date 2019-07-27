





#ifndef FLDSET_H_
#define FLDSET_H_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/calendar.h"
#include "unicode/ucal.h"
#include "unicode/udat.h"
#include "udbgutil.h"
#include "dbgutil.h"
#include "unicode/unistr.h"

#define U_FIELDS_SET_MAX  64








class FieldsSet {
    protected:
        






        FieldsSet(UDebugEnumType whichEnum);
        
        



        FieldsSet(int32_t fieldsCount);

    public:
    
      








      U_NAMESPACE_QUALIFIER UnicodeString diffFrom(const FieldsSet& other, UErrorCode &status) const;

    public:
      





      int32_t parseFrom(const U_NAMESPACE_QUALIFIER UnicodeString& str, UErrorCode& status) {
          return parseFrom(str,NULL,status);
      }

      







      int32_t parseFrom(const U_NAMESPACE_QUALIFIER UnicodeString& str,
                        const FieldsSet& inheritFrom,
                        UErrorCode& status) {
          return parseFrom(str, &inheritFrom, status);
      }

      







      int32_t parseFrom(const U_NAMESPACE_QUALIFIER UnicodeString& str,
                        const FieldsSet* inheritFrom,
                        UErrorCode& status);

    protected:
      













      virtual int32_t handleParseName(const FieldsSet* inheritFrom,
                                      const U_NAMESPACE_QUALIFIER UnicodeString& name,
                                      const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                                      UErrorCode& status);

      








      virtual void handleParseValue(const FieldsSet* inheritFrom,
                                    int32_t field,
                                    const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                                    UErrorCode& status);

      





      void parseValueDefault(const FieldsSet* inheritFrom,
                             int32_t field,
                             const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                             UErrorCode& status);      


      





      void parseValueEnum(UDebugEnumType type,
                          const FieldsSet* inheritFrom,
                          int32_t field,
                          const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                          UErrorCode& status);

    private:
      



      FieldsSet();
      
      



      void construct(UDebugEnumType whichEnum, int32_t fieldCount);

    public:
    


     virtual ~FieldsSet();

    


    void clear();
    
    



    void clear(int32_t field);
    
    




    void set(int32_t field, int32_t value);
    
    UBool isSet(int32_t field) const;

    




    int32_t get(int32_t field) const;
    
    


    UBool isSameType(const FieldsSet& other) const;

    


    int32_t fieldCount() const;
    
    protected:
       int32_t fValue[U_FIELDS_SET_MAX];
       UBool fIsSet[U_FIELDS_SET_MAX];
    protected:
       int32_t fFieldCount;
       UDebugEnumType fEnum;
};





class CalendarFieldsSet : public FieldsSet {
public:
    CalendarFieldsSet();
    virtual ~CalendarFieldsSet();







    



    UBool matches(U_NAMESPACE_QUALIFIER Calendar *cal,
                  CalendarFieldsSet &diffSet,
                  UErrorCode& status) const;

    





    void setOnCalendar(U_NAMESPACE_QUALIFIER Calendar *cal, UErrorCode& status) const;
        
protected:
    


    void handleParseValue(const FieldsSet* inheritFrom,
                          int32_t field,
                          const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                          UErrorCode& status);
};







class DateTimeStyleSet : public FieldsSet {
    public:
        DateTimeStyleSet();
        virtual ~DateTimeStyleSet();

        


        UDateFormatStyle getDateStyle() const;
        
        


        UDateFormatStyle getTimeStyle() const;
    protected:
        void handleParseValue(const FieldsSet* inheritFrom,
                              int32_t field,
                              const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                              UErrorCode& status);
        int32_t handleParseName(const FieldsSet* inheritFrom,
                                const U_NAMESPACE_QUALIFIER UnicodeString& name,
                                const U_NAMESPACE_QUALIFIER UnicodeString& substr,
                                UErrorCode& status);
};


#endif 
#endif 
