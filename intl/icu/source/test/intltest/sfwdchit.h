





#ifndef SFDWCHIT_H
#define SFDWCHIT_H

#include "unicode/chariter.h"
#include "intltest.h"

class SimpleFwdCharIterator : public ForwardCharacterIterator {
public:
    
    SimpleFwdCharIterator(UChar *s, int32_t len, UBool adopt = FALSE);

    virtual ~SimpleFwdCharIterator();

  



  
        
  


  virtual int32_t hashCode(void) const;
        
  




  virtual UClassID getDynamicClassID(void) const;

  





  virtual UChar         nextPostInc(void);
        
  





  virtual UChar32       next32PostInc(void);
        
  





  virtual UBool        hasNext();

protected:
    SimpleFwdCharIterator() {}
    SimpleFwdCharIterator(const SimpleFwdCharIterator &other)
        : ForwardCharacterIterator(other) {}
    SimpleFwdCharIterator &operator=(const SimpleFwdCharIterator&) { return *this; }
private:
    static const int32_t            kInvalidHashCode;
    static const int32_t            kEmptyHashCode;

    UChar *fStart, *fEnd, *fCurrent;
    int32_t fLen;
    UBool fBogus;
    int32_t fHashCode;
};

#endif
