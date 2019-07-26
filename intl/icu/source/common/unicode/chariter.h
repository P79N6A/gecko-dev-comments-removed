








#ifndef CHARITER_H
#define CHARITER_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/unistr.h"




 
U_NAMESPACE_BEGIN



































































class U_COMMON_API ForwardCharacterIterator : public UObject {
public:
    




    enum { DONE = 0xffff };
    
    



    virtual ~ForwardCharacterIterator();
    
    







    virtual UBool operator==(const ForwardCharacterIterator& that) const = 0;
    
    









    inline UBool operator!=(const ForwardCharacterIterator& that) const;
    
    




    virtual int32_t hashCode(void) const = 0;
    
    






    virtual UClassID getDynamicClassID(void) const = 0;
    
    







    virtual UChar         nextPostInc(void) = 0;
    
    







    virtual UChar32       next32PostInc(void) = 0;
    
    








    virtual UBool        hasNext() = 0;
    
protected:
    
    ForwardCharacterIterator();
    
    
    ForwardCharacterIterator(const ForwardCharacterIterator &other);
    
    



    ForwardCharacterIterator &operator=(const ForwardCharacterIterator&) { return *this; }
};










































































































































































class U_COMMON_API CharacterIterator : public ForwardCharacterIterator {
public:
    



    enum EOrigin { kStart, kCurrent, kEnd };

    



    virtual ~CharacterIterator();

    







    virtual CharacterIterator* clone(void) const = 0;

    






    virtual UChar         first(void) = 0;

    







    virtual UChar         firstPostInc(void);

    








    virtual UChar32       first32(void) = 0;

    







    virtual UChar32       first32PostInc(void);

    






    inline int32_t    setToStart();

    






    virtual UChar         last(void) = 0;
        
    






    virtual UChar32       last32(void) = 0;

    






    inline int32_t    setToEnd();

    







    virtual UChar         setIndex(int32_t position) = 0;

    










    virtual UChar32       setIndex32(int32_t position) = 0;

    




    virtual UChar         current(void) const = 0;
        
    




    virtual UChar32       current32(void) const = 0;
        
    






    virtual UChar         next(void) = 0;
        
    









    virtual UChar32       next32(void) = 0;
        
    






    virtual UChar         previous(void) = 0;

    






    virtual UChar32       previous32(void) = 0;

    








    virtual UBool        hasPrevious() = 0;

    









    inline int32_t       startIndex(void) const;
        
    








    inline int32_t       endIndex(void) const;
        
    







    inline int32_t       getIndex(void) const;

    





    inline int32_t           getLength() const;

    










    virtual int32_t      move(int32_t delta, EOrigin origin) = 0;

    










    virtual int32_t      move32(int32_t delta, EOrigin origin) = 0;

    





    virtual void            getText(UnicodeString&  result) = 0;

protected:
    



    CharacterIterator();

    



    CharacterIterator(int32_t length);

    



    CharacterIterator(int32_t length, int32_t position);

    



    CharacterIterator(int32_t length, int32_t textBegin, int32_t textEnd, int32_t position);
  
    





    CharacterIterator(const CharacterIterator &that);

    






    CharacterIterator &operator=(const CharacterIterator &that);

    




    int32_t textLength;

    



    int32_t  pos;

    



    int32_t  begin;

    



    int32_t  end;
};

inline UBool
ForwardCharacterIterator::operator!=(const ForwardCharacterIterator& that) const {
    return !operator==(that);
}

inline int32_t
CharacterIterator::setToStart() {
    return move(0, kStart);
}

inline int32_t
CharacterIterator::setToEnd() {
    return move(0, kEnd);
}

inline int32_t
CharacterIterator::startIndex(void) const {
    return begin;
}

inline int32_t
CharacterIterator::endIndex(void) const {
    return end;
}

inline int32_t
CharacterIterator::getIndex(void) const {
    return pos;
}

inline int32_t
CharacterIterator::getLength(void) const {
    return textLength;
}

U_NAMESPACE_END
#endif
