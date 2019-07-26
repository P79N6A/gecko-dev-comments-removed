





#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"


#define MAX_SHORTENED_IDENTIFIER_SIZE 32




class LongNameMap {
public:
    static LongNameMap* GetInstance();
    void Release();

    
    
    const char* Find(const char* originalName) const;

    
    void Insert(const char* originalName, const char* mappedName);

    
    int Size() const;

private:
    LongNameMap();
    ~LongNameMap();

    size_t refCount;
    std::map<std::string, std::string> mLongNameMap;
};



class MapLongVariableNames : public TIntermTraverser {
public:
    MapLongVariableNames(LongNameMap* globalMap);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    TString mapGlobalLongName(const TString& name);

    LongNameMap* mGlobalMap;
};

#endif  
