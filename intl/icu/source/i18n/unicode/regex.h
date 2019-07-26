














#ifndef REGEX_H
#define REGEX_H



























#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/utext.h"
#include "unicode/parseerr.h"

#include "unicode/uregex.h"



U_NAMESPACE_BEGIN

struct Regex8BitSet;
class  RegexCImpl;
class  RegexMatcher;
class  RegexPattern;
struct REStackFrame;
class  RuleBasedBreakIterator;
class  UnicodeSet;
class  UVector;
class  UVector32;
class  UVector64;





#ifdef REGEX_DEBUG
U_INTERNAL void U_EXPORT2
    RegexPatternDump(const RegexPattern *pat);
#else
    #undef RegexPatternDump
    #define RegexPatternDump(pat)
#endif














class U_I18N_API RegexPattern: public UObject {
public:

    






    RegexPattern();

    





    RegexPattern(const RegexPattern &source);

    




    virtual ~RegexPattern();

    







    UBool           operator==(const RegexPattern& that) const;

    







    inline UBool    operator!=(const RegexPattern& that) const {return ! operator ==(that);}

    




    RegexPattern  &operator =(const RegexPattern &source);

    






    virtual RegexPattern  *clone() const;


   























    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        UParseError          &pe,
        UErrorCode           &status);

   

























    static RegexPattern * U_EXPORT2 compile( UText *regex,
        UParseError          &pe,
        UErrorCode           &status);

   























    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        uint32_t             flags,
        UParseError          &pe,
        UErrorCode           &status);

   

























    static RegexPattern * U_EXPORT2 compile( UText *regex,
        uint32_t             flags,
        UParseError          &pe,
        UErrorCode           &status);

   





















    static RegexPattern * U_EXPORT2 compile( const UnicodeString &regex,
        uint32_t             flags,
        UErrorCode           &status);

   























    static RegexPattern * U_EXPORT2 compile( UText *regex,
        uint32_t             flags,
        UErrorCode           &status);

   




    virtual uint32_t flags() const;

   
















    virtual RegexMatcher *matcher(const UnicodeString &input,
        UErrorCode          &status) const;
        
private:
    












    RegexMatcher *matcher(const UChar *input,
        UErrorCode          &status) const;
public:


   










    virtual RegexMatcher *matcher(UErrorCode  &status) const;


   













    static UBool U_EXPORT2 matches(const UnicodeString   &regex,
        const UnicodeString   &input,
              UParseError     &pe,
              UErrorCode      &status);

   













    static UBool U_EXPORT2 matches(UText *regex,
        UText           *input,
        UParseError     &pe,
        UErrorCode      &status);

   







    virtual UnicodeString pattern() const;
    
    
   









    virtual UText *patternText(UErrorCode      &status) const;


    





































    virtual int32_t  split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode       &status) const;


    





































    virtual int32_t  split(UText *input,
        UText            *dest[],
        int32_t          destCapacity,
        UErrorCode       &status) const;


    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:
    
    
    
    UText          *fPattern;      
    UnicodeString  *fPatternString; 
    uint32_t        fFlags;        
                                   
    UVector64       *fCompiledPat; 
    UnicodeString   fLiteralText;  
                                   

    UVector         *fSets;        
    Regex8BitSet    *fSets8;       


    UErrorCode      fDeferredStatus; 
                                   

    int32_t         fMinMatchLen;  
                                   
                                   
                                   
    
    int32_t         fFrameSize;    
                                   

    int32_t         fDataSize;     
                                   
                                   

    UVector32       *fGroupMap;    
                                   

    int32_t         fMaxCaptureDigits;

    UnicodeSet     **fStaticSets;  
                                   

    Regex8BitSet   *fStaticSets8;  
                                   

    int32_t         fStartType;    
    int32_t         fInitialStringIdx;     
    int32_t         fInitialStringLen;
    UnicodeSet     *fInitialChars;
    UChar32         fInitialChar;
    Regex8BitSet   *fInitialChars8;
    UBool           fNeedsAltInput;

    friend class RegexCompile;
    friend class RegexMatcher;
    friend class RegexCImpl;

    
    
    
    void        init();            
    void        zap();             
#ifdef REGEX_DEBUG
    void        dumpOp(int32_t index) const;
    friend     void U_EXPORT2 RegexPatternDump(const RegexPattern *);
#endif

};












class U_I18N_API RegexMatcher: public UObject {
public:

    













    RegexMatcher(const UnicodeString &regexp, uint32_t flags, UErrorCode &status);

    














    RegexMatcher(UText *regexp, uint32_t flags, UErrorCode &status);

    




















    RegexMatcher(const UnicodeString &regexp, const UnicodeString &input,
        uint32_t flags, UErrorCode &status);

    




















    RegexMatcher(UText *regexp, UText *input,
        uint32_t flags, UErrorCode &status);

private:
    












    RegexMatcher(const UnicodeString &regexp, const UChar *input,
        uint32_t flags, UErrorCode &status);
public:


   




    virtual ~RegexMatcher();


   





    virtual UBool matches(UErrorCode &status);


   









    virtual UBool matches(int64_t startIndex, UErrorCode &status);


   












    virtual UBool lookingAt(UErrorCode &status);


  












    virtual UBool lookingAt(int64_t startIndex, UErrorCode &status);


   











    virtual UBool find();


   








    virtual UBool find(int64_t start, UErrorCode &status);


   








    virtual UnicodeString group(UErrorCode &status) const;


   











    virtual UnicodeString group(int32_t groupNum, UErrorCode &status) const;


   




    virtual int32_t groupCount() const;


   













    virtual UText *group(UText *dest, int64_t &group_len, UErrorCode &status) const; 

   














    virtual UText *group(int32_t groupNum, UText *dest, int64_t &group_len, UErrorCode &status) const;

   














    virtual UText *group(int32_t groupNum, UText *dest, UErrorCode &status) const;


   






    virtual int32_t start(UErrorCode &status) const;

   






    virtual int64_t start64(UErrorCode &status) const;


   












    virtual int32_t start(int32_t group, UErrorCode &status) const;

   












    virtual int64_t start64(int32_t group, UErrorCode &status) const;


   












    virtual int32_t end(UErrorCode &status) const;

   












    virtual int64_t end64(UErrorCode &status) const;


   
















    virtual int32_t end(int32_t group, UErrorCode &status) const;

   
















    virtual int64_t end64(int32_t group, UErrorCode &status) const;


   







    virtual RegexMatcher &reset();


   














    virtual RegexMatcher &reset(int64_t index, UErrorCode &status);


   
















    virtual RegexMatcher &reset(const UnicodeString &input);


   












    virtual RegexMatcher &reset(UText *input);


  























    virtual RegexMatcher &refreshInputText(UText *input, UErrorCode &status);

private:
    












    RegexMatcher &reset(const UChar *input);
public:

   






    virtual const UnicodeString &input() const;
    
   







    virtual UText *inputText() const;
    
   









    virtual UText *getInput(UText *dest, UErrorCode &status) const;
    

   

















     virtual RegexMatcher &region(int64_t start, int64_t limit, UErrorCode &status);

   










     virtual RegexMatcher &region(int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode &status);

   







     virtual int32_t regionStart() const;

   







     virtual int64_t regionStart64() const;


    







      virtual int32_t regionEnd() const;

   







      virtual int64_t regionEnd64() const;

    







      virtual UBool hasTransparentBounds() const;

    

















      virtual RegexMatcher &useTransparentBounds(UBool b);

     
    





    
      virtual UBool hasAnchoringBounds() const;


    











      virtual RegexMatcher &useAnchoringBounds(UBool b);


    











      virtual UBool hitEnd() const;

    








      virtual UBool requireEnd() const;


   




    virtual const RegexPattern &pattern() const;


   















    virtual UnicodeString replaceAll(const UnicodeString &replacement, UErrorCode &status);


   



















    virtual UText *replaceAll(UText *replacement, UText *dest, UErrorCode &status);
    

   



















    virtual UnicodeString replaceFirst(const UnicodeString &replacement, UErrorCode &status);
    

   























    virtual UText *replaceFirst(UText *replacement, UText *dest, UErrorCode &status);
    
    
   


























    virtual RegexMatcher &appendReplacement(UnicodeString &dest,
        const UnicodeString &replacement, UErrorCode &status);
    
    
   


























    virtual RegexMatcher &appendReplacement(UText *dest,
        UText *replacement, UErrorCode &status);


   









    virtual UnicodeString &appendTail(UnicodeString &dest);


   












    virtual UText *appendTail(UText *dest, UErrorCode &status);


    






















    virtual int32_t  split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode       &status);


    






















    virtual int32_t  split(UText *input,
        UText           *dest[],
        int32_t          destCapacity,
        UErrorCode       &status);
    
  




















    virtual void setTimeLimit(int32_t limit, UErrorCode &status);

  





    virtual int32_t getTimeLimit() const;

  




















    virtual void setStackLimit(int32_t  limit, UErrorCode &status);
    
  






    virtual int32_t  getStackLimit() const;


  












    virtual void setMatchCallback(URegexMatchCallback     *callback,
                                  const void              *context,
                                  UErrorCode              &status);


  









    virtual void getMatchCallback(URegexMatchCallback     *&callback,
                                  const void              *&context,
                                  UErrorCode              &status);


  












    virtual void setFindProgressCallback(URegexFindProgressCallback      *callback,
                                              const void                              *context,
                                              UErrorCode                              &status);


  









    virtual void getFindProgressCallback(URegexFindProgressCallback      *&callback,
                                              const void                      *&context,
                                              UErrorCode                      &status);

#ifndef U_HIDE_INTERNAL_API
   




    void setTrace(UBool state);
#endif  

    




    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual UClassID getDynamicClassID() const;

private:
    
    
    RegexMatcher();                  
    RegexMatcher(const RegexPattern *pat);
    RegexMatcher(const RegexMatcher &other);
    RegexMatcher &operator =(const RegexMatcher &rhs);
    void init(UErrorCode &status);                      
    void init2(UText *t, UErrorCode &e);  

    friend class RegexPattern;
    friend class RegexCImpl;
public:
#ifndef U_HIDE_INTERNAL_API
    
    void resetPreserveRegion();  
#endif  
private:

    
    
    
    
    void                 MatchAt(int64_t startIdx, UBool toEnd, UErrorCode &status);
    inline void          backTrack(int64_t &inputIdx, int32_t &patIdx);
    UBool                isWordBoundary(int64_t pos);         
    UBool                isUWordBoundary(int64_t pos);        
    REStackFrame        *resetStack();
    inline REStackFrame *StateSave(REStackFrame *fp, int64_t savePatIdx, UErrorCode &status);
    void                 IncrementTime(UErrorCode &status);
    UBool                ReportFindProgress(int64_t matchIndex, UErrorCode &status);
    
    int64_t              appendGroup(int32_t groupNum, UText *dest, UErrorCode &status) const;
    
    UBool                findUsingChunk();
    void                 MatchChunkAt(int32_t startIdx, UBool toEnd, UErrorCode &status);
    UBool                isChunkWordBoundary(int32_t pos);

    const RegexPattern  *fPattern;
    RegexPattern        *fPatternOwned;    
                                           

    const UnicodeString *fInput;           
    UText               *fInputText;       
    UText               *fAltInputText;    
                                           
    int64_t              fInputLength;     
    int32_t              fFrameSize;       
    
    int64_t              fRegionStart;     
    int64_t              fRegionLimit;     
    
    int64_t              fAnchorStart;     
    int64_t              fAnchorLimit;     
    
    int64_t              fLookStart;       
    int64_t              fLookLimit;       
                                           

    int64_t              fActiveStart;     
    int64_t              fActiveLimit;     
                                           
                                           

    UBool                fTransparentBounds;  
    UBool                fAnchoringBounds; 

    UBool                fMatch;           
    int64_t              fMatchStart;      
    int64_t              fMatchEnd;        
                                           
                                           
    int64_t              fLastMatchEnd;    
                                           
    int64_t              fAppendPosition;  
                                           
                                           
                                           
    UBool                fHitEnd;          
    UBool                fRequireEnd;      
                                           

    UVector64           *fStack;
    REStackFrame        *fFrame;           
                                           
                                           

    int64_t             *fData;            
    int64_t             fSmallData[8];     

    int32_t             fTimeLimit;        
                                           
    
    int32_t             fTime;             
    int32_t             fTickCounter;      
                                           
                                           
                                           

    int32_t             fStackLimit;       
                                           

    URegexMatchCallback *fCallbackFn;       
                                           
    const void         *fCallbackContext;  

    URegexFindProgressCallback  *fFindProgressCallbackFn;  
                                                           
    const void         *fFindProgressCallbackContext;      


    UBool               fInputUniStrMaybeMutable;  

    UBool               fTraceDebug;       

    UErrorCode          fDeferredStatus;   
                                           

    RuleBasedBreakIterator  *fWordBreakItr;
};

U_NAMESPACE_END
#endif  
#endif
