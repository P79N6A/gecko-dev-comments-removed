





#ifndef _SHHANDLE_INCLUDED_
#define _SHHANDLE_INCLUDED_








#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"

class TCompiler;
class TLinker;
class TUniformMap;





class TShHandleBase {
public:
    TShHandleBase() { }
    virtual ~TShHandleBase() { }
    virtual TCompiler* getAsCompiler() { return 0; }
    virtual TLinker* getAsLinker() { return 0; }
    virtual TUniformMap* getAsUniformMap() { return 0; }
};





class TUniformMap : public TShHandleBase {
public:
    TUniformMap() { }
    virtual ~TUniformMap() { }
    virtual TUniformMap* getAsUniformMap() { return this; }
    virtual int getLocation(const char* name) = 0;    
    virtual TInfoSink& getInfoSink() { return infoSink; }
    TInfoSink infoSink;
};
class TIntermNode;





class TCompiler : public TShHandleBase {
public:
    TCompiler(EShLanguage l) : language(l), haveValidObjectCode(false) { }
    virtual ~TCompiler() { }
    EShLanguage getLanguage() { return language; }
    virtual TInfoSink& getInfoSink() { return infoSink; }

    virtual bool compile(TIntermNode* root) = 0;

    virtual TCompiler* getAsCompiler() { return this; }
    virtual bool linkable() { return haveValidObjectCode; }

    TInfoSink infoSink;
protected:
    EShLanguage language;
    bool haveValidObjectCode;
};




typedef TVector<TCompiler*> TCompilerList;
typedef TVector<TShHandleBase*> THandleList;






class TLinker : public TShHandleBase {
public:
    TLinker(EShExecutable e) : 
        executable(e), 
        haveReturnableObjectCode(false),
        appAttributeBindings(0),
        fixedAttributeBindings(0),
        excludedAttributes(0),
        excludedCount(0),
        uniformBindings(0) { }
    virtual TLinker* getAsLinker() { return this; }
    virtual ~TLinker() { }
    virtual bool link(TCompilerList&, TUniformMap*) = 0;
    virtual bool link(THandleList&) { return false; }
    virtual void setAppAttributeBindings(const ShBindingTable* t)   { appAttributeBindings = t; }
    virtual void setFixedAttributeBindings(const ShBindingTable* t) { fixedAttributeBindings = t; }
    virtual void getAttributeBindings(ShBindingTable const **t) const = 0;
    virtual void setExcludedAttributes(const int* attributes, int count) { excludedAttributes = attributes; excludedCount = count; }
    virtual ShBindingTable* getUniformBindings() const  { return uniformBindings; }
    virtual const void* getObjectCode() const { return 0; } 
    virtual TInfoSink& getInfoSink() { return infoSink; }
    TInfoSink infoSink;
protected:
    EShExecutable executable;
    bool haveReturnableObjectCode;  

    const ShBindingTable* appAttributeBindings;
    const ShBindingTable* fixedAttributeBindings;
    const int* excludedAttributes;
    int excludedCount;
    ShBindingTable* uniformBindings;                
};










TCompiler* ConstructCompiler(EShLanguage, int);

TShHandleBase* ConstructLinker(EShExecutable, int);
void DeleteLinker(TShHandleBase*);

TUniformMap* ConstructUniformMap();
void DeleteCompiler(TCompiler*);

void DeleteUniformMap(TUniformMap*);

#endif 
