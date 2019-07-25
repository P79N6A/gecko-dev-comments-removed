





#ifndef _SYMBOL_TABLE_INCLUDED_
#define _SYMBOL_TABLE_INCLUDED_
























#include <assert.h>

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"




class TSymbol {    
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)
    TSymbol(const TString *n) :  name(n) { }
    virtual ~TSymbol() {  }
    const TString& getName() const { return *name; }
    virtual const TString& getMangledName() const { return getName(); }
    virtual bool isFunction() const { return false; }
    virtual bool isVariable() const { return false; }
    void setUniqueId(int id) { uniqueId = id; }
    int getUniqueId() const { return uniqueId; }
    virtual void dump(TInfoSink &infoSink) const = 0;	
    TSymbol(const TSymbol&);
    virtual TSymbol* clone(TStructureMap& remapper) = 0;

protected:
    const TString *name;
    unsigned int uniqueId;      
};











class TVariable : public TSymbol {
public:
    TVariable(const TString *name, const TType& t, bool uT = false ) : TSymbol(name), type(t), userType(uT), unionArray(0), arrayInformationType(0) { }
    virtual ~TVariable() { }
    virtual bool isVariable() const { return true; }    
    TType& getType() { return type; }    
    const TType& getType() const { return type; }
    bool isUserType() const { return userType; }
    void changeQualifier(TQualifier qualifier) { type.changeQualifier(qualifier); }
    void updateArrayInformationType(TType *t) { arrayInformationType = t; }
    TType* getArrayInformationType() { return arrayInformationType; }

    virtual void dump(TInfoSink &infoSink) const;

    ConstantUnion* getConstPointer()
    { 
        if (!unionArray)
            unionArray = new ConstantUnion[type.getObjectSize()];

        return unionArray;
    }

    ConstantUnion* getConstPointer() const { return unionArray; }

    void shareConstPointer( ConstantUnion *constArray)
    {
        delete unionArray;
        unionArray = constArray;  
    }
    TVariable(const TVariable&, TStructureMap& remapper); 
    virtual TVariable* clone(TStructureMap& remapper);

protected:
    TType type;
    bool userType;
    
    
    ConstantUnion *unionArray;
    TType *arrayInformationType;  
};





struct TParameter {
    TString *name;
    TType* type;
    void copyParam(const TParameter& param, TStructureMap& remapper)
    {
        name = NewPoolTString(param.name->c_str());
        type = param.type->clone(remapper);
    }
};




class TFunction : public TSymbol {
public:
    TFunction(TOperator o) :
        TSymbol(0),
        returnType(TType(EbtVoid, EbpUndefined)),
        op(o),
        defined(false) { }
    TFunction(const TString *name, TType& retType, TOperator tOp = EOpNull) : 
        TSymbol(name), 
        returnType(retType),
        mangledName(TFunction::mangleName(*name)),
        op(tOp),
        defined(false) { }
    virtual ~TFunction();
    virtual bool isFunction() const { return true; }    

    static TString mangleName(const TString& name) { return name + '('; }
    static TString unmangleName(const TString& mangledName)
    {
        return TString(mangledName.c_str(), mangledName.find_first_of('('));
    }

    void addParameter(TParameter& p) 
    { 
        parameters.push_back(p);
        mangledName = mangledName + p.type->getMangledName();
    }

    const TString& getMangledName() const { return mangledName; }
    const TType& getReturnType() const { return returnType; }
    void relateToOperator(TOperator o) { op = o; }
    TOperator getBuiltInOp() const { return op; }
    void setDefined() { defined = true; }
    bool isDefined() { return defined; }

    int getParamCount() const { return static_cast<int>(parameters.size()); }    
    TParameter& operator [](int i)       { return parameters[i]; }
    const TParameter& operator [](int i) const { return parameters[i]; }

    virtual void dump(TInfoSink &infoSink) const;
    TFunction(const TFunction&, TStructureMap& remapper);
    virtual TFunction* clone(TStructureMap& remapper);

protected:
    typedef TVector<TParameter> TParamList;
    TParamList parameters;
    TType returnType;
    TString mangledName;
    TOperator op;
    bool defined;
};


class TSymbolTableLevel {
public:
    typedef TMap<TString, TSymbol*> tLevel;
    typedef tLevel::const_iterator const_iterator;
    typedef const tLevel::value_type tLevelPair;
    typedef std::pair<tLevel::iterator, bool> tInsertResult;

    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)
    TSymbolTableLevel() { }
    ~TSymbolTableLevel();

    bool insert(TSymbol& symbol) 
    {
        
        
        
        tInsertResult result;
        result = level.insert(tLevelPair(symbol.getMangledName(), &symbol));

        return result.second;
    }

    TSymbol* find(const TString& name) const
    {
        tLevel::const_iterator it = level.find(name);
        if (it == level.end())
            return 0;
        else
            return (*it).second;
    }

    const_iterator begin() const
    {
        return level.begin();
    }

    const_iterator end() const
    {
        return level.end();
    }

    void relateToOperator(const char* name, TOperator op);
    void dump(TInfoSink &infoSink) const;
    TSymbolTableLevel* clone(TStructureMap& remapper);

protected:
    tLevel level;
};

class TSymbolTable {
public:
    TSymbolTable() : uniqueId(0)
    {
        
        
        
        
        
    }

    ~TSymbolTable()
    {
        
        while (table.size() > 1)
            pop();
    }

    
    
    
    
    
    bool isEmpty() { return table.size() == 0; }
    bool atBuiltInLevel() { return table.size() == 1; }
    bool atGlobalLevel() { return table.size() <= 2; }
    void push()
    {
        table.push_back(new TSymbolTableLevel);
        precisionStack.push_back( PrecisionStackLevel() );
    }

    void pop()
    { 
        delete table[currentLevel()]; 
        table.pop_back(); 
        precisionStack.pop_back();
    }

    bool insert(TSymbol& symbol)
    {
        symbol.setUniqueId(++uniqueId);
        return table[currentLevel()]->insert(symbol);
    }

    TSymbol* find(const TString& name, bool* builtIn = 0, bool *sameScope = 0) 
    {
        int level = currentLevel();
        TSymbol* symbol;
        do {
            symbol = table[level]->find(name);
            --level;
        } while (symbol == 0 && level >= 0);
        level++;
        if (builtIn)
            *builtIn = level == 0;
        if (sameScope)
            *sameScope = level == currentLevel();
        return symbol;
    }

    TSymbolTableLevel* getGlobalLevel() { assert(table.size() >= 2); return table[1]; }
    void relateToOperator(const char* name, TOperator op) { table[0]->relateToOperator(name, op); }
    int getMaxSymbolId() { return uniqueId; }
    void dump(TInfoSink &infoSink) const;
    void copyTable(const TSymbolTable& copyOf);

    void setDefaultPrecision( TBasicType type, TPrecision prec ){
        if( type != EbtFloat && type != EbtInt ) return; 
        int indexOfLastElement = static_cast<int>(precisionStack.size()) - 1;
        precisionStack[indexOfLastElement][type] = prec; 
    }

    
    TPrecision getDefaultPrecision( TBasicType type){
        if( type != EbtFloat && type != EbtInt ) return EbpUndefined;
        int level = static_cast<int>(precisionStack.size()) - 1;
        assert( level >= 0); 
        PrecisionStackLevel::iterator it;
        TPrecision prec = EbpUndefined; 
        while( level >= 0 ){
            it = precisionStack[level].find( type );
            if( it != precisionStack[level].end() ){
                prec = (*it).second;
                break;
            }
            level--;
        }
        return prec;
    }

protected:    
    int currentLevel() const { return static_cast<int>(table.size()) - 1; }

    std::vector<TSymbolTableLevel*> table;
    typedef std::map< TBasicType, TPrecision > PrecisionStackLevel;
    std::vector< PrecisionStackLevel > precisionStack;
    int uniqueId;     
};

#endif 
