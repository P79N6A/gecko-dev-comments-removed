







#ifndef NORMLZR_H
#define NORMLZR_H

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_NORMALIZATION

#include "unicode/chariter.h"
#include "unicode/normalizer2.h"
#include "unicode/unistr.h"
#include "unicode/unorm.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN






































































































class U_COMMON_API Normalizer : public UObject {
public:
  




  enum {
      DONE=0xffff
  };

  

  









  Normalizer(const UnicodeString& str, UNormalizationMode mode);

  










  Normalizer(const UChar* str, int32_t length, UNormalizationMode mode);

  









  Normalizer(const CharacterIterator& iter, UNormalizationMode mode);

  




  Normalizer(const Normalizer& copy);

  



  virtual ~Normalizer();


  
  
  

  













  static void U_EXPORT2 normalize(const UnicodeString& source,
                        UNormalizationMode mode, int32_t options,
                        UnicodeString& result,
                        UErrorCode &status);

  
















  static void U_EXPORT2 compose(const UnicodeString& source,
                      UBool compat, int32_t options,
                      UnicodeString& result,
                      UErrorCode &status);

  
















  static void U_EXPORT2 decompose(const UnicodeString& source,
                        UBool compat, int32_t options,
                        UnicodeString& result,
                        UErrorCode &status);

  



















  static inline UNormalizationCheckResult
  quickCheck(const UnicodeString &source, UNormalizationMode mode, UErrorCode &status);

  












  static UNormalizationCheckResult
  quickCheck(const UnicodeString &source, UNormalizationMode mode, int32_t options, UErrorCode &status);

  



















  static inline UBool
  isNormalized(const UnicodeString &src, UNormalizationMode mode, UErrorCode &errorCode);

  














  static UBool
  isNormalized(const UnicodeString &src, UNormalizationMode mode, int32_t options, UErrorCode &errorCode);

  




























  static UnicodeString &
  U_EXPORT2 concatenate(const UnicodeString &left, const UnicodeString &right,
              UnicodeString &result,
              UNormalizationMode mode, int32_t options,
              UErrorCode &errorCode);

  































































  static inline int32_t
  compare(const UnicodeString &s1, const UnicodeString &s2,
          uint32_t options,
          UErrorCode &errorCode);

  
  
  

  







  UChar32              current(void);

  







  UChar32              first(void);

  







  UChar32              last(void);

  













  UChar32              next(void);

  













  UChar32              previous(void);

  








  void                 setIndexOnly(int32_t index);

  




  void                reset(void);

  













  int32_t            getIndex(void) const;

  







  int32_t            startIndex(void) const;

  









  int32_t            endIndex(void) const;

  







  UBool        operator==(const Normalizer& that) const;

  







  inline UBool        operator!=(const Normalizer& that) const;

  





  Normalizer*        clone(void) const;

  





  int32_t                hashCode(void) const;

  
  
  

  














  void setMode(UNormalizationMode newMode);

  









  UNormalizationMode getUMode(void) const;

  















  void setOption(int32_t option,
         UBool value);

  









  UBool getOption(int32_t option) const;

  







  void setText(const UnicodeString& newText,
           UErrorCode &status);

  







  void setText(const CharacterIterator& newText,
           UErrorCode &status);

  








  void setText(const UChar* newText,
                    int32_t length,
            UErrorCode &status);
  





  void            getText(UnicodeString&  result);

  




  static UClassID U_EXPORT2 getStaticClassID();

  




  virtual UClassID getDynamicClassID() const;

private:
  
  
  

  Normalizer(); 
  Normalizer &operator=(const Normalizer &that); 

  
  
  UBool nextNormalize();
  UBool previousNormalize();

  void    init();
  void    clearBuffer(void);

  
  
  

  FilteredNormalizer2*fFilteredNorm2;  
  const Normalizer2  *fNorm2;  
  UNormalizationMode  fUMode;
  int32_t             fOptions;

  
  CharacterIterator  *text;

  
  
  int32_t         currentIndex, nextIndex;

  
  UnicodeString       buffer;
  int32_t         bufferPos;
};





inline UBool
Normalizer::operator!= (const Normalizer& other) const
{ return ! operator==(other); }

inline UNormalizationCheckResult
Normalizer::quickCheck(const UnicodeString& source,
                       UNormalizationMode mode,
                       UErrorCode &status) {
    return quickCheck(source, mode, 0, status);
}

inline UBool
Normalizer::isNormalized(const UnicodeString& source,
                         UNormalizationMode mode,
                         UErrorCode &status) {
    return isNormalized(source, mode, 0, status);
}

inline int32_t
Normalizer::compare(const UnicodeString &s1, const UnicodeString &s2,
                    uint32_t options,
                    UErrorCode &errorCode) {
  
  return unorm_compare(s1.getBuffer(), s1.length(),
                       s2.getBuffer(), s2.length(),
                       options,
                       &errorCode);
}

U_NAMESPACE_END

#endif

#endif
