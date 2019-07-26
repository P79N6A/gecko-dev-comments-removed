












#ifndef RBBI_H
#define RBBI_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/brkiter.h"
#include "unicode/udata.h"
#include "unicode/parseerr.h"
#include "unicode/schriter.h"
#include "unicode/uchriter.h"


struct UTrie;

U_NAMESPACE_BEGIN


struct RBBIDataHeader;
class  RuleBasedBreakIteratorTables;
class  BreakIterator;
class  RBBIDataWrapper;
class  UStack;
class  LanguageBreakEngine;
class  UnhandledEngine;
struct RBBIStateTable;



















class U_COMMON_API RuleBasedBreakIterator : public BreakIterator {

protected:
    



    UText  *fText;

    




    CharacterIterator  *fCharIter;

    




    StringCharacterIterator *fSCharIter;

    




    UCharCharacterIterator *fDCharIter;

    



    RBBIDataWrapper    *fData;

    


    int32_t             fLastRuleStatusIndex;

    





    UBool               fLastStatusIndexValid;

    




    uint32_t            fDictionaryCharCount;

    






    int32_t*            fCachedBreakPositions;

    



    int32_t             fNumCachedBreakPositions;

    




    int32_t             fPositionInCache;
    
    






    UStack              *fLanguageBreakEngines;
    
    






    UnhandledEngine     *fUnhandledBreakEngine;
    
    




    int32_t             fBreakType;
    
protected:
    
    
    

#ifndef U_HIDE_INTERNAL_API
    







    enum EDontAdopt {
        kDontAdopt
    };

    









    RuleBasedBreakIterator(RBBIDataHeader* data, UErrorCode &status);

    







    RuleBasedBreakIterator(const RBBIDataHeader* data, enum EDontAdopt dontAdopt, UErrorCode &status);
#endif  


    friend class RBBIRuleBuilder;
    
    friend class BreakIterator;



public:

    



    RuleBasedBreakIterator();

    





    RuleBasedBreakIterator(const RuleBasedBreakIterator& that);

    







    RuleBasedBreakIterator( const UnicodeString    &rules,
                             UParseError           &parseError,
                             UErrorCode            &status);

    






















    RuleBasedBreakIterator(const uint8_t *compiledRules,
                           uint32_t       ruleLength,
                           UErrorCode    &status);

    











    RuleBasedBreakIterator(UDataMemory* image, UErrorCode &status);

    



    virtual ~RuleBasedBreakIterator();

    






    RuleBasedBreakIterator& operator=(const RuleBasedBreakIterator& that);

    







    virtual UBool operator==(const BreakIterator& that) const;

    






    UBool operator!=(const BreakIterator& that) const;

    









    virtual BreakIterator* clone() const;

    




    virtual int32_t hashCode(void) const;

    




    virtual const UnicodeString& getRules(void) const;

    
    
    

    
























    virtual  CharacterIterator& getText(void) const;


    













     virtual UText *getUText(UText *fillIn, UErrorCode &status) const;

    






    virtual void adoptText(CharacterIterator* newText);

    





    virtual void setText(const UnicodeString& newText);

    












    virtual void  setText(UText *text, UErrorCode &status);

    




    virtual int32_t first(void);

    




    virtual int32_t last(void);

    









    virtual int32_t next(int32_t n);

    




    virtual int32_t next(void);

    




    virtual int32_t previous(void);

    






    virtual int32_t following(int32_t offset);

    






    virtual int32_t preceding(int32_t offset);

    







    virtual UBool isBoundary(int32_t offset);

    




    virtual int32_t current(void) const;


    































    virtual int32_t getRuleStatus() const;

   






















    virtual int32_t getRuleStatusVec(int32_t *fillInVec, int32_t capacity, UErrorCode &status);

    










    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    























    virtual BreakIterator *  createBufferClone(void *stackBuffer,
                                               int32_t &BufferSize,
                                               UErrorCode &status);


    
















    virtual const uint8_t *getBinaryRules(uint32_t &length);

    
























    virtual RuleBasedBreakIterator &refreshInputText(UText *input, UErrorCode &status);


protected:
    
    
    
    




    virtual void reset(void);

#if 0
    







    virtual UBool isDictionaryChar(UChar32);

    



    virtual int32_t getBreakType() const;
#endif

    



    virtual void setBreakType(int32_t type);

#ifndef U_HIDE_INTERNAL_API
    




    void init();
#endif  

private:

    








    int32_t handlePrevious(const RBBIStateTable *statetable);

    








    int32_t handleNext(const RBBIStateTable *statetable);

protected:

#ifndef U_HIDE_INTERNAL_API
    













    int32_t checkDictionary(int32_t startPos, int32_t endPos, UBool reverse);
#endif  

private:

    





    const LanguageBreakEngine *getLanguageBreakEngine(UChar32 c);

    


    void makeRuleStatusValid();

};







inline UBool RuleBasedBreakIterator::operator!=(const BreakIterator& that) const {
    return !operator==(that);
}

U_NAMESPACE_END

#endif 

#endif
