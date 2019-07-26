


















#ifndef BRKITER_H
#define BRKITER_H

#include "unicode/utypes.h"






#if UCONFIG_NO_BREAK_ITERATION

U_NAMESPACE_BEGIN





class BreakIterator;

U_NAMESPACE_END

#else

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/chariter.h"
#include "unicode/locid.h"
#include "unicode/ubrk.h"
#include "unicode/strenum.h"
#include "unicode/utext.h"
#include "unicode/umisc.h"

U_NAMESPACE_BEGIN














































class U_COMMON_API BreakIterator : public UObject {
public:
    



    virtual ~BreakIterator();

    












    virtual UBool operator==(const BreakIterator&) const = 0;

    





    UBool operator!=(const BreakIterator& rhs) const { return !operator==(rhs); }

    




    virtual BreakIterator* clone(void) const = 0;

    




    virtual UClassID getDynamicClassID(void) const = 0;

    



    virtual CharacterIterator& getText(void) const = 0;


    













     virtual UText *getUText(UText *fillIn, UErrorCode &status) const = 0;

    





    virtual void  setText(const UnicodeString &text) = 0;

    












    virtual void  setText(UText *text, UErrorCode &status) = 0;

    







    virtual void  adoptText(CharacterIterator* it) = 0;

    enum {
        




        DONE = (int32_t)-1
    };

    




    virtual int32_t first(void) = 0;

    




    virtual int32_t last(void) = 0;

    





    virtual int32_t previous(void) = 0;

    





    virtual int32_t next(void) = 0;

    




    virtual int32_t current(void) const = 0;

    







    virtual int32_t following(int32_t offset) = 0;

    







    virtual int32_t preceding(int32_t offset) = 0;

    







    virtual UBool isBoundary(int32_t offset) = 0;

    








    virtual int32_t next(int32_t n) = 0;

    


















    static BreakIterator* U_EXPORT2
    createWordInstance(const Locale& where, UErrorCode& status);

    




















    static BreakIterator* U_EXPORT2
    createLineInstance(const Locale& where, UErrorCode& status);

    


















    static BreakIterator* U_EXPORT2
    createCharacterInstance(const Locale& where, UErrorCode& status);

    

















    static BreakIterator* U_EXPORT2
    createSentenceInstance(const Locale& where, UErrorCode& status);

    





















    static BreakIterator* U_EXPORT2
    createTitleInstance(const Locale& where, UErrorCode& status);

    








    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

    








    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         const Locale& displayLocale,
                                         UnicodeString& name);

    







    static UnicodeString& U_EXPORT2 getDisplayName(const Locale& objectLocale,
                                         UnicodeString& name);

    
















    virtual BreakIterator *  createBufferClone(void *stackBuffer,
                                               int32_t &BufferSize,
                                               UErrorCode &status) = 0;

    





    inline UBool isBufferClone(void);

#if !UCONFIG_NO_SERVICE
    











    static URegistryKey U_EXPORT2 registerInstance(BreakIterator* toAdopt,
                                        const Locale& locale,
                                        UBreakIteratorType kind,
                                        UErrorCode& status);

    








    static UBool U_EXPORT2 unregister(URegistryKey key, UErrorCode& status);

    





    static StringEnumeration* U_EXPORT2 getAvailableLocales(void);
#endif

    




    Locale getLocale(ULocDataLocaleType type, UErrorCode& status) const;

#ifndef U_HIDE_INTERNAL_API
    





    const char *getLocaleID(ULocDataLocaleType type, UErrorCode& status) const;
#endif  

    
























    virtual BreakIterator &refreshInputText(UText *input, UErrorCode &status) = 0;

 private:
    static BreakIterator* buildInstance(const Locale& loc, const char *type, int32_t kind, UErrorCode& status);
    static BreakIterator* createInstance(const Locale& loc, int32_t kind, UErrorCode& status);
    static BreakIterator* makeInstance(const Locale& loc, int32_t kind, UErrorCode& status);

    friend class ICUBreakIteratorFactory;
    friend class ICUBreakIteratorService;

protected:
    
    
    
    BreakIterator();
    
    UBool fBufferClone;
    
    BreakIterator (const BreakIterator &other) : UObject(other), fBufferClone(FALSE) {}

private:

    
    char actualLocale[ULOC_FULLNAME_CAPACITY];
    char validLocale[ULOC_FULLNAME_CAPACITY];

    



    BreakIterator& operator=(const BreakIterator&);
};

inline UBool BreakIterator::isBufferClone()
{
    return fBufferClone;
}

U_NAMESPACE_END

#endif 

#endif 


