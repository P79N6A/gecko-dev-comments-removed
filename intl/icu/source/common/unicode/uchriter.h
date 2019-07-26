






#ifndef UCHRITER_H
#define UCHRITER_H

#include "unicode/utypes.h"
#include "unicode/chariter.h"





 
U_NAMESPACE_BEGIN













class U_COMMON_API UCharCharacterIterator : public CharacterIterator {
public:
  








  UCharCharacterIterator(const UChar* textPtr, int32_t length);

  












  UCharCharacterIterator(const UChar* textPtr, int32_t length,
                         int32_t position);

  















  UCharCharacterIterator(const UChar* textPtr, int32_t length,
                         int32_t textBegin,
                         int32_t textEnd,
                         int32_t position);

  






  UCharCharacterIterator(const UCharCharacterIterator&  that);

  



  virtual ~UCharCharacterIterator();

  







  UCharCharacterIterator&
  operator=(const UCharCharacterIterator&    that);

  







  virtual UBool          operator==(const ForwardCharacterIterator& that) const;

  




  virtual int32_t         hashCode(void) const;

  






  virtual CharacterIterator* clone(void) const;

  






  virtual UChar         first(void);

  







  virtual UChar         firstPostInc(void);

  








  virtual UChar32       first32(void);

  







  virtual UChar32       first32PostInc(void);

  






  virtual UChar         last(void);

  






  virtual UChar32       last32(void);

  







  virtual UChar         setIndex(int32_t position);

  










  virtual UChar32       setIndex32(int32_t position);

  




  virtual UChar         current(void) const;

  




  virtual UChar32       current32(void) const;

  






  virtual UChar         next(void);

  







  virtual UChar         nextPostInc(void);

  









  virtual UChar32       next32(void);

  







  virtual UChar32       next32PostInc(void);

  








  virtual UBool        hasNext();

  






  virtual UChar         previous(void);

  






  virtual UChar32       previous32(void);

  








  virtual UBool        hasPrevious();

  










  virtual int32_t      move(int32_t delta, EOrigin origin);

  










  virtual int32_t      move32(int32_t delta, EOrigin origin);

  



  void setText(const UChar* newText, int32_t newTextLength);

  






  virtual void            getText(UnicodeString& result);

  




  static UClassID         U_EXPORT2 getStaticClassID(void);

  




  virtual UClassID        getDynamicClassID(void) const;

protected:
  



  UCharCharacterIterator();
  



  const UChar*            text;

};

U_NAMESPACE_END
#endif
