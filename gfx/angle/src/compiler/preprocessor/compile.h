














































#if !defined(__COMPILE_H)
#define __COMPILE_H 1

int InitCPPStruct(void);

typedef struct Options_Rec{
    const char *profileString;
    int ErrorMode;
    int Quiet;
	
    
    int DumpAtomTable;
} Options;

struct CPPStruct_Rec {
    
    SourceLoc *pLastSourceLoc;  
    Options options;            

    
    SourceLoc lastSourceLoc;

    

    SourceLoc *tokenLoc;        
    int mostRecentToken;        
    InputSrc *currentInput;
    int previous_token;
    int pastFirstStatement;     
    
	void *pC;                   
     
    
    SourceLoc ltokenLoc;
	int ifdepth;                
    int elsedepth[64];          
    int elsetracker;            
    const char *ErrMsg;
    int CompileError;           

    
    
    
    
    int PaWhichStr;             
    const int* PaStrLen;        
    int PaArgc;                 
    const char* const* PaArgv;  
    unsigned int tokensBeforeEOF : 1;
};

#endif 
