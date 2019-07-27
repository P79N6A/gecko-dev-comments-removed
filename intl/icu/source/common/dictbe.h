






#ifndef DICTBE_H
#define DICTBE_H

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/utext.h"

#include "brkeng.h"

U_NAMESPACE_BEGIN

class DictionaryMatcher;
class Normalizer2;












class DictionaryBreakEngine : public LanguageBreakEngine {
 private:
    




  UnicodeSet    fSet;

    




  uint32_t      fTypes;

  



  DictionaryBreakEngine();

 public:

  




  DictionaryBreakEngine( uint32_t breakTypes );

  


  virtual ~DictionaryBreakEngine();

  








  virtual UBool handles( UChar32 c, int32_t breakType ) const;

  













  virtual int32_t findBreaks( UText *text,
                              int32_t startPos,
                              int32_t endPos,
                              UBool reverse,
                              int32_t breakType,
                              UStack &foundBreaks ) const;

 protected:

 




  virtual void setCharacters( const UnicodeSet &set );

 






 








  virtual int32_t divideUpDictionaryRange( UText *text,
                                           int32_t rangeStart,
                                           int32_t rangeEnd,
                                           UStack &foundBreaks ) const = 0;

};












class ThaiBreakEngine : public DictionaryBreakEngine {
 private:
    




  UnicodeSet                fThaiWordSet;
  UnicodeSet                fEndWordSet;
  UnicodeSet                fBeginWordSet;
  UnicodeSet                fSuffixSet;
  UnicodeSet                fMarkSet;
  DictionaryMatcher  *fDictionary;

 public:

  





  ThaiBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status);

  


  virtual ~ThaiBreakEngine();

 protected:
 








  virtual int32_t divideUpDictionaryRange( UText *text,
                                           int32_t rangeStart,
                                           int32_t rangeEnd,
                                           UStack &foundBreaks ) const;

};












class LaoBreakEngine : public DictionaryBreakEngine {
 private:
    




  UnicodeSet                fLaoWordSet;
  UnicodeSet                fEndWordSet;
  UnicodeSet                fBeginWordSet;
  UnicodeSet                fMarkSet;
  DictionaryMatcher  *fDictionary;

 public:

  





  LaoBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status);

  


  virtual ~LaoBreakEngine();

 protected:
 








  virtual int32_t divideUpDictionaryRange( UText *text,
                                           int32_t rangeStart,
                                           int32_t rangeEnd,
                                           UStack &foundBreaks ) const;

};



 
 






 
class BurmeseBreakEngine : public DictionaryBreakEngine { 
 private: 
    


 
 
  UnicodeSet                fBurmeseWordSet; 
  UnicodeSet                fEndWordSet; 
  UnicodeSet                fBeginWordSet; 
  UnicodeSet                fMarkSet; 
  DictionaryMatcher  *fDictionary; 
 
 public: 
 
  




 
  BurmeseBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status); 
 
  

 
  virtual ~BurmeseBreakEngine(); 
 
 protected: 
 







 
  virtual int32_t divideUpDictionaryRange( UText *text, 
                                           int32_t rangeStart, 
                                           int32_t rangeEnd, 
                                           UStack &foundBreaks ) const; 
 
}; 
 


 
 






 
class KhmerBreakEngine : public DictionaryBreakEngine { 
 private: 
    


 
 
  UnicodeSet                fKhmerWordSet; 
  UnicodeSet                fEndWordSet; 
  UnicodeSet                fBeginWordSet; 
  UnicodeSet                fMarkSet; 
  DictionaryMatcher  *fDictionary; 
 
 public: 
 
  




 
  KhmerBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status); 
 
  

 
  virtual ~KhmerBreakEngine(); 
 
 protected: 
 







 
  virtual int32_t divideUpDictionaryRange( UText *text, 
                                           int32_t rangeStart, 
                                           int32_t rangeEnd, 
                                           UStack &foundBreaks ) const; 
 
}; 
 
#if !UCONFIG_NO_NORMALIZATION






enum LanguageType {
    kKorean,
    kChineseJapanese
};






class CjkBreakEngine : public DictionaryBreakEngine {
 protected:
    



  UnicodeSet                fHangulWordSet;
  UnicodeSet                fHanWordSet;
  UnicodeSet                fKatakanaWordSet;
  UnicodeSet                fHiraganaWordSet;

  DictionaryMatcher        *fDictionary;
  const Normalizer2        *nfkcNorm2;

 public:

    






  CjkBreakEngine(DictionaryMatcher *adoptDictionary, LanguageType type, UErrorCode &status);

    


  virtual ~CjkBreakEngine();

 protected:
    








  virtual int32_t divideUpDictionaryRange( UText *text,
          int32_t rangeStart,
          int32_t rangeEnd,
          UStack &foundBreaks ) const;

};

#endif

U_NAMESPACE_END

    
#endif
