








#ifndef STSEARCH_H
#define STSEARCH_H

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/search.h"

U_NAMESPACE_BEGIN















































































































class U_I18N_API StringSearch : public SearchIterator
{
public:

    

    




















    StringSearch(const UnicodeString &pattern, const UnicodeString &text,
                 const Locale        &locale,       
                       BreakIterator *breakiter,
                       UErrorCode    &status);

    




















    StringSearch(const UnicodeString     &pattern, 
                 const UnicodeString     &text,
                       RuleBasedCollator *coll,       
                       BreakIterator     *breakiter,
                       UErrorCode        &status);

    
























    StringSearch(const UnicodeString &pattern, CharacterIterator &text,
                 const Locale        &locale, 
                       BreakIterator *breakiter,
                       UErrorCode    &status);

    
























    StringSearch(const UnicodeString     &pattern, CharacterIterator &text,
                       RuleBasedCollator *coll, 
                       BreakIterator     *breakiter,
                       UErrorCode        &status);

    





    StringSearch(const StringSearch &that);

    




    virtual ~StringSearch(void);

    










    StringSearch *clone() const;

    

    





    StringSearch & operator=(const StringSearch &that);

    







    virtual UBool operator==(const SearchIterator &that) const;

    

    












    virtual void setOffset(int32_t position, UErrorCode &status);

    







    virtual int32_t getOffset(void) const;

    










    virtual void setText(const UnicodeString &text, UErrorCode &status);
    
    













    virtual void setText(CharacterIterator &text, UErrorCode &status);

    








    RuleBasedCollator * getCollator() const;
    
    









    void setCollator(RuleBasedCollator *coll, UErrorCode &status);
    
    








    void setPattern(const UnicodeString &pattern, UErrorCode &status);
    
    




    const UnicodeString & getPattern() const;

    

    







    virtual void reset();

    







    virtual SearchIterator * safeClone(void) const;
    
    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

protected:

    

    





















    virtual int32_t handleNext(int32_t position, UErrorCode &status);

    





















    virtual int32_t handlePrev(int32_t position, UErrorCode &status);
    
private :
    StringSearch(); 

    

    



    RuleBasedCollator  m_collator_;
    



    UnicodeString      m_pattern_;
    



    UStringSearch     *m_strsrch_;

};

U_NAMESPACE_END

#endif 

#endif

