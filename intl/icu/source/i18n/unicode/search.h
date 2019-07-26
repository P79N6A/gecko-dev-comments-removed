








#ifndef SEARCH_H
#define SEARCH_H

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/chariter.h"
#include "unicode/brkiter.h"
#include "unicode/usearch.h"




struct USearch;



typedef struct USearch USearch;

U_NAMESPACE_BEGIN









































class U_I18N_API SearchIterator : public UObject {

public:

    

    





    SearchIterator(const SearchIterator &other);

    



    virtual ~SearchIterator();

    

    












    virtual void setOffset(int32_t position, UErrorCode &status) = 0;

    







    virtual int32_t getOffset(void) const = 0;

    








    void setAttribute(USearchAttribute       attribute,
                      USearchAttributeValue  value,
                      UErrorCode            &status);

    





    USearchAttributeValue getAttribute(USearchAttribute  attribute) const;
    
    















    int32_t getMatchedStart(void) const;

    













    int32_t getMatchedLength(void) const;
    
    













    void getMatchedText(UnicodeString &result) const;
    
    














    void setBreakIterator(BreakIterator *breakiter, UErrorCode &status);
    
    









    const BreakIterator * getBreakIterator(void) const;

    









    virtual void setText(const UnicodeString &text, UErrorCode &status);    

    














    virtual void setText(CharacterIterator &text, UErrorCode &status);
    
    




    const UnicodeString & getText(void) const;

    

    







    virtual UBool operator==(const SearchIterator &that) const;

    





    UBool operator!=(const SearchIterator &that) const;

    

    






    virtual SearchIterator* safeClone(void) const = 0;

    












    int32_t first(UErrorCode &status);

    
















    int32_t following(int32_t position, UErrorCode &status);
    
    












    int32_t last(UErrorCode &status);

    






















    int32_t preceding(int32_t position, UErrorCode &status);

    













     int32_t next(UErrorCode &status);

    












    int32_t previous(UErrorCode &status);

    







    virtual void reset();

protected:
    

    



    USearch *m_search_;

    







    BreakIterator *m_breakiterator_;
    
    



    UnicodeString  m_text_;

    

    




    SearchIterator();

    














    SearchIterator(const UnicodeString &text, 
                         BreakIterator *breakiter = NULL);

    


















    SearchIterator(CharacterIterator &text, BreakIterator *breakiter = NULL);

    

    





    SearchIterator & operator=(const SearchIterator &that);

    


















    virtual int32_t handleNext(int32_t position, UErrorCode &status) 
                                                                         = 0;

    


















     virtual int32_t handlePrev(int32_t position, UErrorCode &status) 
                                                                         = 0;

    









    virtual void setMatchLength(int32_t length);

    









    virtual void setMatchStart(int32_t position);

    



    void setMatchNotFound();
};

inline UBool SearchIterator::operator!=(const SearchIterator &that) const
{
   return !operator==(that); 
}
U_NAMESPACE_END

#endif 

#endif

