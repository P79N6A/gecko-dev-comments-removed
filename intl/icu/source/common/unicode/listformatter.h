















#ifndef __LISTFORMATTER_H__
#define __LISTFORMATTER_H__

#include "unicode/utypes.h"

#include "unicode/unistr.h"
#include "unicode/locid.h"

U_NAMESPACE_BEGIN


class Hashtable;


struct ListFormatInternal;



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

    



    ListFormatter(const ListFormatter&);

    



    ListFormatter& operator=(const ListFormatter& other);

    







    static ListFormatter* createInstance(UErrorCode& errorCode);

    








    static ListFormatter* createInstance(const Locale& locale, UErrorCode& errorCode);

#ifndef U_HIDE_INTERNAL_API
    









    static ListFormatter* createInstance(const Locale& locale, const char* style, UErrorCode& errorCode);
#endif  

    




    virtual ~ListFormatter();


    









    UnicodeString& format(const UnicodeString items[], int32_t n_items,
        UnicodeString& appendTo, UErrorCode& errorCode) const;

#ifndef U_HIDE_INTERNAL_API
    


    UnicodeString& format(
            const UnicodeString items[],
            int32_t n_items,
            UnicodeString& appendTo,
            int32_t index,
            int32_t &offset,
            UErrorCode& errorCode) const;
    


    ListFormatter(const ListFormatData &data);
    


    ListFormatter(const ListFormatInternal* listFormatterInternal);
#endif  

  private:
    static void initializeHash(UErrorCode& errorCode);
    static const ListFormatInternal* getListFormatInternal(const Locale& locale, const char *style, UErrorCode& errorCode);

    ListFormatter();

    ListFormatInternal* owned;
    const ListFormatInternal* data;
};

U_NAMESPACE_END

#endif
