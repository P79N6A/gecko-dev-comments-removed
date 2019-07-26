










#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "compiler/SymbolTable.h"

#include <stdio.h>
#include <algorithm>
#include <climits>

TType::TType(const TPublicType &p) :
            type(p.type), precision(p.precision), qualifier(p.qualifier), size(p.size), matrix(p.matrix), array(p.array), arraySize(p.arraySize), structure(0)
{
    if (p.userDef)
        structure = p.userDef->getStruct();
}




TString TType::buildMangledName() const
{
    TString mangledName;
    if (isMatrix())
        mangledName += 'm';
    else if (isVector())
        mangledName += 'v';

    switch (type) {
    case EbtFloat:       mangledName += 'f';      break;
    case EbtInt:         mangledName += 'i';      break;
    case EbtBool:        mangledName += 'b';      break;
    case EbtSampler2D:   mangledName += "s2";     break;
    case EbtSamplerCube: mangledName += "sC";     break;
    case EbtStruct:      mangledName += structure->mangledName(); break;
    default:             break;
    }

    mangledName += static_cast<char>('0' + getNominalSize());
    if (isArray()) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", arraySize);
        mangledName += '[';
        mangledName += buf;
        mangledName += ']';
    }
    return mangledName;
}

size_t TType::getObjectSize() const
{
    size_t totalSize = 0;

    if (getBasicType() == EbtStruct)
        totalSize = structure->objectSize();
    else if (matrix)
        totalSize = size * size;
    else
        totalSize = size;

    if (isArray()) {
        size_t arraySize = getArraySize();
        if (arraySize > INT_MAX / totalSize)
            totalSize = INT_MAX;
        else
            totalSize *= arraySize;
    }

    return totalSize;
}

bool TStructure::containsArrays() const
{
    for (size_t i = 0; i < mFields->size(); ++i) {
        const TType* fieldType = (*mFields)[i]->type();
        if (fieldType->isArray() || fieldType->isStructureContainingArrays())
            return true;
    }
    return false;
}

TString TStructure::buildMangledName() const
{
    TString mangledName("struct-");
    mangledName += *mName;
    for (size_t i = 0; i < mFields->size(); ++i) {
        mangledName += '-';
        mangledName += (*mFields)[i]->type()->getMangledName();
    }
    return mangledName;
}

size_t TStructure::calculateObjectSize() const
{
    size_t size = 0;
    for (size_t i = 0; i < mFields->size(); ++i) {
        size_t fieldSize = (*mFields)[i]->type()->getObjectSize();
        if (fieldSize > INT_MAX - size)
            size = INT_MAX;
        else
            size += fieldSize;
    }
    return size;
}

int TStructure::calculateDeepestNesting() const
{
    int maxNesting = 0;
    for (size_t i = 0; i < mFields->size(); ++i) {
        maxNesting = std::max(maxNesting, (*mFields)[i]->type()->getDeepestStructNesting());
    }
    return 1 + maxNesting;
}





void TVariable::dump(TInfoSink& infoSink) const
{
    infoSink.debug << getName().c_str() << ": " << type.getQualifierString() << " " << type.getPrecisionString() << " " << type.getBasicString();
    if (type.isArray()) {
        infoSink.debug << "[0]";
    }
    infoSink.debug << "\n";
}

void TFunction::dump(TInfoSink &infoSink) const
{
    infoSink.debug << getName().c_str() << ": " <<  returnType.getBasicString() << " " << getMangledName().c_str() << "\n";
}

void TSymbolTableLevel::dump(TInfoSink &infoSink) const
{
    tLevel::const_iterator it;
    for (it = level.begin(); it != level.end(); ++it)
        (*it).second->dump(infoSink);
}

void TSymbolTable::dump(TInfoSink &infoSink) const
{
    for (int level = currentLevel(); level >= 0; --level) {
        infoSink.debug << "LEVEL " << level << "\n";
        table[level]->dump(infoSink);
    }
}




TFunction::~TFunction()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
}




TSymbolTableLevel::~TSymbolTableLevel()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        if ((*it).first == (*it).second->getMangledName())
            delete (*it).second;
}







void TSymbolTableLevel::relateToOperator(const char* name, TOperator op)
{
    tLevel::iterator it;
    for (it = level.begin(); it != level.end(); ++it) {
        if ((*it).second->isFunction()) {
            TFunction* function = static_cast<TFunction*>((*it).second);
            if (function->getName() == name)
                function->relateToOperator(op);
        }
    }
}







void TSymbolTableLevel::relateToExtension(const char* name, const TString& ext)
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it) {
        TSymbol* symbol = it->second;
        if (symbol->getName() == name)
            symbol->relateToExtension(ext);
    }
}
