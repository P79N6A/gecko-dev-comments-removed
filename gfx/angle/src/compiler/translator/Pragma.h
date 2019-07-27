





#ifndef COMPILER_PRAGMA_H_
#define COMPILER_PRAGMA_H_

struct TPragma {
    
    TPragma() : optimize(true), debug(false) { }
    TPragma(bool o, bool d) : optimize(o), debug(d) { }

    bool optimize;
    bool debug;
};

#endif 
