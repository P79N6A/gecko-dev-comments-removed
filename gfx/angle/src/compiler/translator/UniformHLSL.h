








#ifndef TRANSLATOR_UNIFORMHLSL_H_
#define TRANSLATOR_UNIFORMHLSL_H_

#include "compiler/translator/Types.h"

namespace sh
{
class StructureHLSL;

class UniformHLSL
{
  public:
    UniformHLSL(StructureHLSL *structureHLSL, TranslatorHLSL *translator);

    void reserveUniformRegisters(unsigned int registerCount);
    void reserveInterfaceBlockRegisters(unsigned int registerCount);
    TString uniformsHeader(ShShaderOutput outputType, const ReferencedSymbols &referencedUniforms);
    TString interfaceBlocksHeader(const ReferencedSymbols &referencedInterfaceBlocks);

    
    static TString interfaceBlockInstanceString(const TInterfaceBlock& interfaceBlock, unsigned int arrayIndex);

    const std::map<std::string, unsigned int> &getInterfaceBlockRegisterMap() const
    {
        return mInterfaceBlockRegisterMap;
    }
    const std::map<std::string, unsigned int> &getUniformRegisterMap() const
    {
        return mUniformRegisterMap;
    }

  private:
    TString interfaceBlockString(const TInterfaceBlock &interfaceBlock, unsigned int registerIndex, unsigned int arrayIndex);
    TString interfaceBlockMembersString(const TInterfaceBlock &interfaceBlock, TLayoutBlockStorage blockStorage);
    TString interfaceBlockStructString(const TInterfaceBlock &interfaceBlock);
    const Uniform *findUniformByName(const TString &name) const;

    
    unsigned int declareUniformAndAssignRegister(const TType &type, const TString &name);

    unsigned int mUniformRegister;
    unsigned int mInterfaceBlockRegister;
    unsigned int mSamplerRegister;
    StructureHLSL *mStructureHLSL;
    ShShaderOutput mOutputType;

    const std::vector<Uniform> &mUniforms;
    std::map<std::string, unsigned int> mInterfaceBlockRegisterMap;
    std::map<std::string, unsigned int> mUniformRegisterMap;
};

}

#endif 
