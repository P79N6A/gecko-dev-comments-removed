





#ifndef _SYMBOL_TABLE_INCLUDED_
#define _SYMBOL_TABLE_INCLUDED_
























#include <assert.h>

#include "common/angleutils.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/intermediate.h"


class TSymbol
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TSymbol(const TString *n)
        : uniqueId(0),
          name(n)
    {
    }
    virtual ~TSymbol()
    {
        
    }

    const TString &getName() const
    {
        return *name;
    }
    virtual const TString &getMangledName() const
    {
        return getName();
    }
    virtual bool isFunction() const
    {
        return false;
    }
    virtual bool isVariable() const
    {
        return false;
    }
    void setUniqueId(int id)
    {
        uniqueId = id;
    }
    int getUniqueId() const
    {
        return uniqueId;
    }
    void relateToExtension(const TString &ext)
    {
        extension = ext;
    }
    const TString &getExtension() const
    {
        return extension;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TSymbol);

    int uniqueId; 
    const TString *name;
    TString extension;
};









class TVariable : public TSymbol
{
  public:
    TVariable(const TString *name, const TType &t, bool uT = false)
        : TSymbol(name),
          type(t),
          userType(uT),
          unionArray(0)
    {
    }
    virtual ~TVariable()
    {
    }
    virtual bool isVariable() const
    {
        return true;
    }
    TType &getType()
    {
        return type;
    }
    const TType &getType() const
    {
        return type;
    }
    bool isUserType() const
    {
        return userType;
    }
    void setQualifier(TQualifier qualifier)
    {
        type.setQualifier(qualifier);
    }

    ConstantUnion *getConstPointer()
    { 
        if (!unionArray)
            unionArray = new ConstantUnion[type.getObjectSize()];

        return unionArray;
    }

    ConstantUnion *getConstPointer() const
    {
        return unionArray;
    }

    void shareConstPointer(ConstantUnion *constArray)
    {
        if (unionArray == constArray)
            return;

        delete[] unionArray;
        unionArray = constArray;  
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TVariable);

    TType type;
    bool userType;
    
    
    ConstantUnion *unionArray;
};



struct TParameter
{
    TString *name;
    TType *type;
};


class TFunction : public TSymbol
{
  public:
    TFunction(TOperator o)
        : TSymbol(0),
          returnType(TType(EbtVoid, EbpUndefined)),
          op(o),
          defined(false)
    {
    }
    TFunction(const TString *name, const TType &retType, TOperator tOp = EOpNull)
        : TSymbol(name),
          returnType(retType),
          mangledName(TFunction::mangleName(*name)),
          op(tOp),
          defined(false)
    {
    }
    virtual ~TFunction();
    virtual bool isFunction() const
    {
        return true;
    }

    static TString mangleName(const TString &name)
    {
        return name + '(';
    }
    static TString unmangleName(const TString &mangledName)
    {
        return TString(mangledName.c_str(), mangledName.find_first_of('('));
    }

    void addParameter(TParameter &p)
    { 
        parameters.push_back(p);
        mangledName = mangledName + p.type->getMangledName();
    }

    const TString &getMangledName() const
    {
        return mangledName;
    }
    const TType &getReturnType() const
    {
        return returnType;
    }

    void relateToOperator(TOperator o)
    {
        op = o;
    }
    TOperator getBuiltInOp() const
    {
        return op;
    }

    void setDefined()
    {
        defined = true;
    }
    bool isDefined()
    {
        return defined;
    }

    size_t getParamCount() const
    {
        return parameters.size();
    }
    const TParameter &getParam(size_t i) const
    {
        return parameters[i];
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(TFunction);

    typedef TVector<TParameter> TParamList;
    TParamList parameters;
    TType returnType;
    TString mangledName;
    TOperator op;
    bool defined;
};


class TInterfaceBlockName : public TSymbol
{
  public:
    TInterfaceBlockName(const TString *name)
        : TSymbol(name)
    {
    }

    virtual ~TInterfaceBlockName()
    {
    }
};

class TSymbolTableLevel
{
  public:
    typedef TMap<TString, TSymbol *> tLevel;
    typedef tLevel::const_iterator const_iterator;
    typedef const tLevel::value_type tLevelPair;
    typedef std::pair<tLevel::iterator, bool> tInsertResult;

    TSymbolTableLevel()
    {
    }
    ~TSymbolTableLevel();

    bool insert(TSymbol *symbol);

    TSymbol *find(const TString &name) const;

    void relateToOperator(const char *name, TOperator op);
    void relateToExtension(const char *name, const TString &ext);

  protected:
    tLevel level;
};

enum ESymbolLevel
{
    COMMON_BUILTINS = 0,
    ESSL1_BUILTINS = 1,
    ESSL3_BUILTINS = 2,
    LAST_BUILTIN_LEVEL = ESSL3_BUILTINS,
    GLOBAL_LEVEL = 3
};

class TSymbolTable
{
  public:
    TSymbolTable()
    {
        
        
        
    }

    ~TSymbolTable();

    
    
    
    bool isEmpty()
    {
        return table.empty();
    }
    bool atBuiltInLevel()
    {
        return currentLevel() <= LAST_BUILTIN_LEVEL;
    }
    bool atGlobalLevel()
    {
        return currentLevel() <= GLOBAL_LEVEL;
    }
    void push()
    {
        table.push_back(new TSymbolTableLevel);
        precisionStack.push_back(new PrecisionStackLevel);
    }

    void pop()
    {
        delete table.back();
        table.pop_back();

        delete precisionStack.back();
        precisionStack.pop_back();
    }

    bool declare(TSymbol *symbol)
    {
        return insert(currentLevel(), symbol);
    }

    bool insert(ESymbolLevel level, TSymbol *symbol)
    {
        return table[level]->insert(symbol);
    }

    bool insertConstInt(ESymbolLevel level, const char *name, int value)
    {
        TVariable *constant = new TVariable(
            NewPoolTString(name), TType(EbtInt, EbpUndefined, EvqConst, 1));
        constant->getConstPointer()->setIConst(value);
        return insert(level, constant);
    }

    void insertBuiltIn(ESymbolLevel level, TType *rvalue, const char *name,
                       TType *ptype1, TType *ptype2 = 0, TType *ptype3 = 0,
                       TType *ptype4 = 0, TType *ptype5 = 0);

    TSymbol *find(const TString &name, int shaderVersion,
                  bool *builtIn = NULL, bool *sameScope = NULL);
    TSymbol *findBuiltIn(const TString &name, int shaderVersion);
    
    TSymbolTableLevel *getOuterLevel()
    {
        assert(currentLevel() >= 1);
        return table[currentLevel() - 1];
    }

    void relateToOperator(ESymbolLevel level, const char *name, TOperator op)
    {
        table[level]->relateToOperator(name, op);
    }
    void relateToExtension(ESymbolLevel level, const char *name, const TString &ext)
    {
        table[level]->relateToExtension(name, ext);
    }
    void dump(TInfoSink &infoSink) const;

    bool setDefaultPrecision(const TPublicType &type, TPrecision prec)
    {
        if (!SupportsPrecision(type.type))
            return false;
        if (type.isAggregate())
            return false; 
        int indexOfLastElement = static_cast<int>(precisionStack.size()) - 1;
        
        (*precisionStack[indexOfLastElement])[type.type] = prec;
        return true;
    }

    
    
    TPrecision getDefaultPrecision(TBasicType type);

    static int nextUniqueId()
    {
        return ++uniqueIdCounter;
    }

  private:
    ESymbolLevel currentLevel() const
    {
        return static_cast<ESymbolLevel>(table.size() - 1);
    }

    std::vector<TSymbolTableLevel *> table;
    typedef TMap<TBasicType, TPrecision> PrecisionStackLevel;
    std::vector< PrecisionStackLevel *> precisionStack;

    static int uniqueIdCounter;
};

#endif 
