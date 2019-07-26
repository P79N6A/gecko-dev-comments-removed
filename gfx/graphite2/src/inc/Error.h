

























#pragma once



namespace graphite2
{

class Error
{
public:
    Error() : _e(0) {};
    operator bool() { return (_e != 0); }
    int error() { return _e; }
    void error(int e) { _e = e; }
    bool test(bool pr, int err) { return (_e = int(pr) * err); }

private:
    int _e;
};

enum errcontext {
    EC_READGLYPHS = 1,      
    EC_READSILF = 2,        
    EC_ASILF = 3,           
    EC_APASS = 4,           
    EC_PASSCCODE = 5,       
    EC_ARULE = 6,           
    EC_ASTARTS = 7,         
    EC_ATRANS = 8,          
    EC_ARULEMAP = 9         
};

enum errors {
    E_OUTOFMEM = 1,         
    E_NOGLYPHS = 2,         
    E_BADUPEM = 3,          
    E_BADCMAP = 4,          
    E_NOSILF = 5,           
    E_TOOOLD = 6,           
    E_BADSIZE = 7,          

    E_BADMAXGLYPH = 8,      
    E_BADNUMJUSTS = 9,      
    E_BADENDJUSTS = 10,     
    E_BADCRITFEATURES = 11, 
    E_BADSCRIPTTAGS = 12,   
    E_BADAPSEUDO = 13,      
    E_BADABREAK = 14,       
    E_BADABIDI = 15,        
    E_BADAMIRROR = 16,      
    E_BADNUMPASSES = 17,    
    E_BADPASSESSTART = 18,  
    E_BADPASSBOUND = 19,    
    E_BADPPASS = 20,        
    E_BADSPASS = 21,        
    E_BADJPASSBOUND = 22,   
    E_BADJPASS = 23,        
    E_BADALIG = 24,         
    E_BADBPASS = 25,        
    E_BADNUMPSEUDO = 26,    
    E_BADCLASSSIZE = 27,    
    E_TOOMANYLINEAR = 28,   
    E_CLASSESTOOBIG = 29,   
    E_MISALIGNEDCLASSES = 30,   
    E_HIGHCLASSOFFSET = 31, 
    E_BADCLASSOFFSET = 32,  
    E_BADCLASSLOOKUPINFO = 33,  

    E_BADPASSSTART = 34,    
    E_BADPASSEND = 35,      
    E_BADPASSLENGTH = 36,   
    E_BADNUMTRANS = 37,     
    E_BADNUMSUCCESS = 38,   
    E_BADNUMSTATES = 39,    
    E_NORANGES = 40,        
    E_BADRULEMAPLEN = 41,   
    E_BADCTXTLENBOUNDS = 42,    
    E_BADCTXTLENS = 43,     
    E_BADPASSCCODEPTR = 44, 
    E_BADRULECCODEPTR = 45, 
    E_BADCCODELEN = 46,     
    E_BADACTIONCODEPTR = 47,    
    E_MUTABLECCODE = 48,    
    E_BADSTATE = 49,        
    E_BADRULEMAPPING = 50,  
    E_BADRANGE = 51,        
    E_BADRULENUM = 52,      

    E_CODEFAILURE = 60,     
        E_CODEALLOC = 61,       
        E_INVALIDOPCODE = 62,   
        E_UNIMPOPCODE = 63,     
        E_OUTOFRANGECODE = 64,  
        E_BADJUMPCODE = 65,     
        E_CODEBADARGS = 66,     
        E_CODENORETURN = 67,    
        E_CODENESTEDCTXT = 68   
};

}

