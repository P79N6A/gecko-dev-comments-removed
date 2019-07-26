















#ifndef __LISTFORMATTER_H__
#define __LISTFORMATTER_H__

#include "unicode/unistr.h"
#include "unicode/locid.h"


U_NAMESPACE_BEGIN


class Hashtable;


struct ListFormatData : public UMemory {
    UnicodeString twoPattern;
    UnicodeString startPattern;
    UnicodeString middlePattern;
    UnicodeString endPattern;

  ListFormatData(const UnicodeString& two, const UnicodeString& start, const UnicodeString& middle, const UnicodeString& end) :
      twoPattern(two), startPattern(start), middlePattern(middle), endPattern(end) {}
};

















class U_COMMON_API ListFormatter : public UObject{

  public:
    







    static ListFormatter* createInstance(UErrorCode& errorCode);

    








    static ListFormatter* createInstance(const Locale& locale, UErrorCode& errorCode);


    




    virtual ~ListFormatter();


    









    UnicodeString& format(const UnicodeString items[], int32_t n_items,
        UnicodeString& appendTo, UErrorCode& errorCode) const;

    






    static void getFallbackLocale(const Locale& in, Locale& out, UErrorCode& errorCode);

    


    ListFormatter(const ListFormatData& listFormatterData);

  private:
    static void initializeHash(UErrorCode& errorCode);
    static void addDataToHash(const char* locale, const char* two, const char* start, const char* middle, const char* end, UErrorCode& errorCode);
    static const ListFormatData* getListFormatData(const Locale& locale, UErrorCode& errorCode);

    ListFormatter();
    ListFormatter(const ListFormatter&);

    ListFormatter& operator = (const ListFormatter&);
    void addNewString(const UnicodeString& pattern, UnicodeString& originalString,
                      const UnicodeString& newString, UErrorCode& errorCode) const;
    virtual UClassID getDynamicClassID() const;

    const ListFormatData& data;
};

U_NAMESPACE_END

#endif
