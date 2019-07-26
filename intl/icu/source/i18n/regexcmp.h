












#ifndef RBBISCAN_H
#define RBBISCAN_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/uobject.h"
#include "unicode/uniset.h"
#include "unicode/parseerr.h"
#include "uhash.h"
#include "uvector.h"



U_NAMESPACE_BEGIN







struct  RegexTableEl;
class   RegexPattern;


class RegexCompile : public UMemory {
public:

    enum {
        kStackSize = 100            
    };                              
                                    
                                    

    struct RegexPatternChar {
        UChar32             fChar;
        UBool               fQuoted;
    };

    RegexCompile(RegexPattern *rp, UErrorCode &e);

    void       compile(const UnicodeString &pat, UParseError &pp, UErrorCode &e);
    void       compile(UText *pat, UParseError &pp, UErrorCode &e);
    

    virtual    ~RegexCompile();

    void        nextChar(RegexPatternChar &c);      

    static void cleanup();                       



    
    
    
    enum EParenClass {
        plain        = -1,               
        capturing    = -2,
        atomic       = -3,
        lookAhead    = -4,
        negLookAhead = -5,
        flags        = -6,
        lookBehind   = -7,
        lookBehindN  = -8
    };

private:


    UBool       doParseActions(int32_t a);
    void        error(UErrorCode e);                   

    UChar32     nextCharLL();
    UChar32     peekCharLL();
    UnicodeSet  *scanProp();
    UnicodeSet  *scanPosixProp();
    void        handleCloseParen();
    int32_t     blockTopLoc(UBool reserve);          
                                                     
                                                     
                                                     
    void        compileSet(UnicodeSet *theSet);      
                                                     
    void        compileInterval(int32_t InitOp,      
                               int32_t LoopOp);
    UBool       compileInlineInterval();             
    void        literalChar(UChar32 c);              
    void        fixLiterals(UBool split=FALSE);      
    void        insertOp(int32_t where);             
                                                     
    int32_t     minMatchLength(int32_t start,
                               int32_t end);
    int32_t     maxMatchLength(int32_t start,
                               int32_t end);
    void        matchStartType();
    void        stripNOPs();

    void        setEval(int32_t op);
    void        setPushOp(int32_t op);
    UChar32     scanNamedChar();
    UnicodeSet *createSetForProperty(const UnicodeString &propName, UBool negated);


    UErrorCode                    *fStatus;
    RegexPattern                  *fRXPat;
    UParseError                   *fParseErr;

    
    
    
    int64_t                       fScanIndex;        
                                                     
    UBool                         fQuoteMode;        
    UBool                         fInBackslashQuote; 
    UBool                         fEOLComments;      
                                                     
    int64_t                       fLineNum;          
    int64_t                       fCharNum;          
    UChar32                       fLastChar;         
                                                     
    UChar32                       fPeekChar;         


    RegexPatternChar              fC;                
                                                     

    
    
    
    RegexTableEl                  **fStateTable;     
                                                     

    uint16_t                      fStack[kStackSize];  
    int32_t                       fStackPtr;           
                                                       

    
    
    
    int32_t                       fModeFlags;        
                                                     
                                                     
                                                     
    int32_t                       fNewModeFlags;     
                                                     
    UBool                         fSetModeFlag;      

    UnicodeString                 fLiteralChars;     
                                                     
                                                     
                                                     
                                                     

    int64_t                       fPatternLength;    
    
    UVector32                     fParenStack;       
                                                     
                                                     
                                                     
                                                     
                                                     
                                                     
                                                     
                                                     


    int32_t                       fMatchOpenParen;   
                                                     
                                                     
                                                     
    int32_t                       fMatchCloseParen;  
                                                     
                                                     

    int32_t                       fIntervalLow;      
    int32_t                       fIntervalUpper;    
                                                     
                                                     
                                                     
                                                     

    int64_t                       fNameStartPos;     
                                                     
                                                     

    UStack                        fSetStack;         
                                                     
                                                     
    UStack                        fSetOpStack;       

    UChar32                       fLastSetLiteral;   
                                                     
                                                     
};




enum SetOperations {
    setStart         = 0 << 16 | 1,
    setEnd           = 1 << 16 | 2,
    setNegation      = 2 << 16 | 3,
    setCaseClose     = 2 << 16 | 9,
    setDifference2   = 3 << 16 | 4,    
    setIntersection2 = 3 << 16 | 5,    
    setUnion         = 4 << 16 | 6,    
    setDifference1   = 4 << 16 | 7,    
    setIntersection1 = 4 << 16 | 8     
    };

U_NAMESPACE_END
#endif
#endif
