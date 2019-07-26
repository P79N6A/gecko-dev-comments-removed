















#ifndef __LISTFORMATTER_H__
#define __LISTFORMATTER_H__

#include "unicode/utypes.h"

#ifndef U_HIDE_DRAFT_API

#include "unicode/unistr.h"
#include "unicode/locid.h"

U_NAMESPACE_BEGIN


class Hashtable;

#ifndef U_HIDE_INTERNAL_API

struct ListFormatData : public UMemory {
    UnicodeString twoPattern;
    UnicodeString startPattern;
    UnicodeString middlePattern;
    UnicodeString endPattern;

  ListFormatData(const UnicodeString& two, const UnicodeString& start, const UnicodeString& middle, const UnicodeString& end) :
      twoPattern(two), startPattern(start), middlePattern(middle), endPattern(end) {}
};
#endif  


















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
    


    ListFormatter(const ListFormatData* listFormatterData);
#endif  

  private:
    static void initializeHash(UErrorCode& errorCode);
    static const ListFormatData* getListFormatData(const Locale& locale, const char *style, UErrorCode& errorCode);

    ListFormatter();
    void addNewString(const UnicodeString& pattern, UnicodeString& originalString,
                      const UnicodeString& newString, UErrorCode& errorCode) const;

    const ListFormatData* data;
};

U_NAMESPACE_END

#endif 
#endif
