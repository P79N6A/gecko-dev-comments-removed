








#ifndef TRANSLATOR_STRUCTUREHLSL_H_
#define TRANSLATOR_STRUCTUREHLSL_H_

#include "compiler/translator/Common.h"
#include "compiler/translator/IntermNode.h"

#include <set>

class TInfoSinkBase;
class TScopeBracket;

namespace sh
{



class Std140PaddingHelper
{
  public:
    explicit Std140PaddingHelper(const std::map<TString, int> &structElementIndexes,
                                 unsigned *uniqueCounter);

    int elementIndex() const { return mElementIndex; }
    int prePadding(const TType &type);
    TString prePaddingString(const TType &type);
    TString postPaddingString(const TType &type, bool useHLSLRowMajorPacking);

  private:
    TString next();

    unsigned *mPaddingCounter;
    int mElementIndex;
    const std::map<TString, int> &mStructElementIndexes;
};

class StructureHLSL
{
  public:
    StructureHLSL();

    void addConstructor(const TType &type, const TString &name, const TIntermSequence *parameters);
    std::string structsHeader() const;

    TString defineQualified(const TStructure &structure, bool useHLSLRowMajorPacking, bool useStd140Packing);
    static TString defineNameless(const TStructure &structure);

    Std140PaddingHelper getPaddingHelper();

  private:
    unsigned mUniquePaddingCounter;

    std::map<TString, int> mStd140StructElementIndexes;

    typedef std::set<TString> StructNames;
    StructNames mStructNames;

    typedef std::set<TString> Constructors;
    Constructors mConstructors;

    typedef std::vector<TString> StructDeclarations;
    StructDeclarations mStructDeclarations;

    void storeStd140ElementIndex(const TStructure &structure, bool useHLSLRowMajorPacking);
    static TString define(const TStructure &structure, bool useHLSLRowMajorPacking,
                         bool useStd140Packing, Std140PaddingHelper *padHelper);
};

}

#endif 
