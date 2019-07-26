






#ifndef BRKENG_H
#define BRKENG_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/utext.h"
#include "unicode/uscript.h"

U_NAMESPACE_BEGIN

class UnicodeSet;
class UStack;
class DictionaryMatcher;














class LanguageBreakEngine : public UMemory {
 public:

  



  LanguageBreakEngine();

  


  virtual ~LanguageBreakEngine();

 








  virtual UBool handles(UChar32 c, int32_t breakType) const = 0;

 













  virtual int32_t findBreaks( UText *text,
                              int32_t startPos,
                              int32_t endPos,
                              UBool reverse,
                              int32_t breakType,
                              UStack &foundBreaks ) const = 0;

};
























class LanguageBreakFactory : public UMemory {
 public:

  



  LanguageBreakFactory();

  


  virtual ~LanguageBreakFactory();

 











  virtual const LanguageBreakEngine *getEngineFor(UChar32 c, int32_t breakType) = 0;

};
















class UnhandledEngine : public LanguageBreakEngine {
 private:

    




  UnicodeSet    *fHandled[4];

 public:

  



  UnhandledEngine(UErrorCode &status);

  


  virtual ~UnhandledEngine();

 








  virtual UBool handles(UChar32 c, int32_t breakType) const;

 













  virtual int32_t findBreaks( UText *text,
                              int32_t startPos,
                              int32_t endPos,
                              UBool reverse,
                              int32_t breakType,
                              UStack &foundBreaks ) const;

 





  virtual void handleCharacter(UChar32 c, int32_t breakType);

};










class ICULanguageBreakFactory : public LanguageBreakFactory {
 private:

    




  UStack    *fEngines;

 public:

  



  ICULanguageBreakFactory(UErrorCode &status);

  


  virtual ~ICULanguageBreakFactory();

 











  virtual const LanguageBreakEngine *getEngineFor(UChar32 c, int32_t breakType);

protected:
 









  virtual const LanguageBreakEngine *loadEngineFor(UChar32 c, int32_t breakType);

  







  virtual DictionaryMatcher *loadDictionaryMatcherFor(UScriptCode script, int32_t breakType);
};

U_NAMESPACE_END

    
#endif
