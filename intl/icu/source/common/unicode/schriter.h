
















#ifndef SCHRITER_H
#define SCHRITER_H

#include "unicode/utypes.h"
#include "unicode/chariter.h"
#include "unicode/uchriter.h"





 
U_NAMESPACE_BEGIN












class U_COMMON_API StringCharacterIterator : public UCharCharacterIterator {
public:
  






  StringCharacterIterator(const UnicodeString& textStr);

  








  StringCharacterIterator(const UnicodeString&    textStr,
              int32_t              textPos);

  















  StringCharacterIterator(const UnicodeString&    textStr,
              int32_t              textBegin,
              int32_t              textEnd,
              int32_t              textPos);

  







  StringCharacterIterator(const StringCharacterIterator&  that);

  



  virtual ~StringCharacterIterator();

  







  StringCharacterIterator&
  operator=(const StringCharacterIterator&    that);

  







  virtual UBool          operator==(const ForwardCharacterIterator& that) const;

  






  virtual CharacterIterator* clone(void) const;

  




  void setText(const UnicodeString& newText);

  






  virtual void            getText(UnicodeString& result);

  




  virtual UClassID         getDynamicClassID(void) const;

  




  static UClassID   U_EXPORT2 getStaticClassID(void);

protected:
  



  StringCharacterIterator();

  





  void setText(const UChar* newText, int32_t newTextLength);

  



  UnicodeString            text;

};

U_NAMESPACE_END
#endif
