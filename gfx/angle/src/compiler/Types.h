





#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#include "compiler/BaseTypes.h"
#include "compiler/Common.h"
#include "compiler/compilerdebug.h"




class TType;
struct TTypeLine {
    TType* type;
    int line;
};
typedef TVector<TTypeLine> TTypeList;

inline TTypeList* NewPoolTTypeList()
{
    void* memory = GlobalPoolAllocator.allocate(sizeof(TTypeList));
    return new(memory) TTypeList;
}










class TPublicType {
public:
    TBasicType type;
    TQualifier qualifier;
    TPrecision precision;
    int size;          
    bool matrix;
    bool array;
    int arraySize;
    TType* userDef;
    int line;

    void setBasic(TBasicType bt, TQualifier q, int ln = 0)
    {
        type = bt;
        qualifier = q;
        precision = EbpUndefined;
        size = 1;
        matrix = false;
        array = false;
        arraySize = 0;
        userDef = 0;
        line = ln;
    }

    void setAggregate(int s, bool m = false)
    {
        size = s;
        matrix = m;
    }

    void setArray(bool a, int s = 0)
    {
        array = a;
        arraySize = s;
    }
};

typedef TMap<TTypeList*, TTypeList*> TStructureMap;
typedef TMap<TTypeList*, TTypeList*>::iterator TStructureMapIterator;



class TType {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)
    TType() {}
    TType(TBasicType t, TPrecision p, TQualifier q = EvqTemporary, int s = 1, bool m = false, bool a = false) :
            type(t), precision(p), qualifier(q), size(s), matrix(m), array(a), arraySize(0),
            maxArraySize(0), arrayInformationType(0), structure(0), structureSize(0), fieldName(0), mangled(0), typeName(0)
    {
    }
    explicit TType(const TPublicType &p) :
            type(p.type), precision(p.precision), qualifier(p.qualifier), size(p.size), matrix(p.matrix), array(p.array), arraySize(p.arraySize),
            maxArraySize(0), arrayInformationType(0), structure(0), structureSize(0), fieldName(0), mangled(0), typeName(0)
    {
        if (p.userDef) {
            structure = p.userDef->getStruct();
            typeName = NewPoolTString(p.userDef->getTypeName().c_str());
        }
    }
    TType(TTypeList* userDef, const TString& n, TPrecision p = EbpUndefined) :
            type(EbtStruct), precision(p), qualifier(EvqTemporary), size(1), matrix(false), array(false), arraySize(0),
            maxArraySize(0), arrayInformationType(0), structure(userDef), structureSize(0), fieldName(0), mangled(0)
    {
        typeName = NewPoolTString(n.c_str());
    }

    void copyType(const TType& copyOf, TStructureMap& remapper)
    {
        type = copyOf.type;
        precision = copyOf.precision;
        qualifier = copyOf.qualifier;
        size = copyOf.size;
        matrix = copyOf.matrix;
        array = copyOf.array;
        arraySize = copyOf.arraySize;

        TStructureMapIterator iter;
        if (copyOf.structure) {
            if ((iter = remapper.find(structure)) == remapper.end()) {
                
                structure = NewPoolTTypeList();
                for (unsigned int i = 0; i < copyOf.structure->size(); ++i) {
                    TTypeLine typeLine;
                    typeLine.line = (*copyOf.structure)[i].line;
                    typeLine.type = (*copyOf.structure)[i].type->clone(remapper);
                    structure->push_back(typeLine);
                }
            } else {
                structure = iter->second;
            }
        } else
            structure = 0;

        fieldName = 0;
        if (copyOf.fieldName)
            fieldName = NewPoolTString(copyOf.fieldName->c_str());
        typeName = 0;
        if (copyOf.typeName)
            typeName = NewPoolTString(copyOf.typeName->c_str());

        mangled = 0;
        if (copyOf.mangled)
            mangled = NewPoolTString(copyOf.mangled->c_str());

        structureSize = copyOf.structureSize;
        maxArraySize = copyOf.maxArraySize;
        assert(copyOf.arrayInformationType == 0);
        arrayInformationType = 0; 
    }

    TType* clone(TStructureMap& remapper)
    {
        TType *newType = new TType();
        newType->copyType(*this, remapper);

        return newType;
    }

    TBasicType getBasicType() const { return type; }
    void setBasicType(TBasicType t) { type = t; }

    TPrecision getPrecision() const { return precision; }
    void setPrecision(TPrecision p) { precision = p; }

    TQualifier getQualifier() const { return qualifier; }
    void setQualifier(TQualifier q) { qualifier = q; }

    
    int getNominalSize() const { return size; }
    void setNominalSize(int s) { size = s; }
    
    int getObjectSize() const
    {
        int totalSize;

        if (getBasicType() == EbtStruct)
            totalSize = getStructSize();
        else if (matrix)
            totalSize = size * size;
        else
            totalSize = size;

        if (isArray())
            totalSize *= std::max(getArraySize(), getMaxArraySize());

        return totalSize;
    }

    bool isMatrix() const { return matrix ? true : false; }
    void setMatrix(bool m) { matrix = m; }

    bool isArray() const  { return array ? true : false; }
    int getArraySize() const { return arraySize; }
    void setArraySize(int s) { array = true; arraySize = s; }
    int getMaxArraySize () const { return maxArraySize; }
    void setMaxArraySize (int s) { maxArraySize = s; }
    void clearArrayness() { array = false; arraySize = 0; maxArraySize = 0; }
    void setArrayInformationType(TType* t) { arrayInformationType = t; }
    TType* getArrayInformationType() const { return arrayInformationType; }

    bool isVector() const { return size > 1 && !matrix; }
    bool isScalar() const { return size == 1 && !matrix && !structure; }

    TTypeList* getStruct() const { return structure; }
    void setStruct(TTypeList* s) { structure = s; }

    const TString& getTypeName() const
    {
        assert(typeName);
        return *typeName;
    }
    void setTypeName(const TString& n)
    {
        typeName = NewPoolTString(n.c_str());
    }

    bool isField() const { return fieldName != 0; }
    const TString& getFieldName() const
    {
        assert(fieldName);
        return *fieldName;
    }
    void setFieldName(const TString& n)
    {
        fieldName = NewPoolTString(n.c_str());
    }

    TString& getMangledName() {
        if (!mangled) {
            mangled = NewPoolTString("");
            buildMangledName(*mangled);
            *mangled += ';' ;
        }

        return *mangled;
    }

    bool sameElementType(const TType& right) const {
        return      type == right.type   &&
                    size == right.size   &&
                  matrix == right.matrix &&
               structure == right.structure;
    }
    bool operator==(const TType& right) const {
        return      type == right.type   &&
                    size == right.size   &&
                  matrix == right.matrix &&
                   array == right.array  && (!array || arraySize == right.arraySize) &&
               structure == right.structure;
        
    }
    bool operator!=(const TType& right) const {
        return !operator==(right);
    }
    bool operator<(const TType& right) const {
        if (type != right.type) return type < right.type;
        if (size != right.size) return size < right.size;
        if (matrix != right.matrix) return matrix < right.matrix;
        if (array != right.array) return array < right.array;
        if (arraySize != right.arraySize) return arraySize < right.arraySize;
        if (structure != right.structure) return structure < right.structure;

        return false;
    }

    const char* getBasicString() const { return ::getBasicString(type); }
    const char* getPrecisionString() const { return ::getPrecisionString(precision); }
    const char* getQualifierString() const { return ::getQualifierString(qualifier); }
    TString getCompleteString() const;

protected:
    void buildMangledName(TString&);
    int getStructSize() const;

    TBasicType type      : 6;
    TPrecision precision;
    TQualifier qualifier : 7;
    int size             : 8; 
    unsigned int matrix  : 1;
    unsigned int array   : 1;
    int arraySize;
    int maxArraySize;
    TType* arrayInformationType;

    TTypeList* structure;      
    mutable int structureSize;

    TString *fieldName;         
    TString *mangled;
    TString *typeName;          
};

#endif 
