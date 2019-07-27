







#ifndef LIBGLESV2_RENDERER_DYNAMIC_HLSL_H_
#define LIBGLESV2_RENDERER_DYNAMIC_HLSL_H_

#include "common/angleutils.h"
#include "libGLESv2/constants.h"

#include "angle_gl.h"

#include <vector>
#include <map>

namespace rx
{
class Renderer;
}

namespace sh
{
struct Attribute;
struct ShaderVariable;
}

namespace gl
{
class InfoLog;
struct VariableLocation;
struct LinkedVarying;
struct VertexAttribute;
struct VertexFormat;
struct PackedVarying;
}

namespace rx
{
class Renderer;
class ShaderD3D;
class VertexShaderD3D;
class FragmentShaderD3D;

typedef const gl::PackedVarying *VaryingPacking[gl::IMPLEMENTATION_MAX_VARYING_VECTORS][4];

struct PixelShaderOuputVariable
{
    GLenum type;
    std::string name;
    std::string source;
    size_t outputIndex;
};

class DynamicHLSL
{
  public:
    explicit DynamicHLSL(rx::Renderer *const renderer);

    int packVaryings(gl::InfoLog &infoLog, VaryingPacking packing, rx::FragmentShaderD3D *fragmentShader,
                     rx::VertexShaderD3D *vertexShader, const std::vector<std::string>& transformFeedbackVaryings);
    std::string generateVertexShaderForInputLayout(const std::string &sourceShader, const gl::VertexFormat inputLayout[],
                                                   const sh::Attribute shaderAttributes[]) const;
    std::string generatePixelShaderForOutputSignature(const std::string &sourceShader, const std::vector<PixelShaderOuputVariable> &outputVariables,
                                                      bool usesFragDepth, const std::vector<GLenum> &outputLayout) const;
    bool generateShaderLinkHLSL(gl::InfoLog &infoLog, int registers, const VaryingPacking packing,
                                std::string& pixelHLSL, std::string& vertexHLSL,
                                rx::FragmentShaderD3D *fragmentShader, rx::VertexShaderD3D *vertexShader,
                                const std::vector<std::string>& transformFeedbackVaryings,
                                std::vector<gl::LinkedVarying> *linkedVaryings,
                                std::map<int, gl::VariableLocation> *programOutputVars,
                                std::vector<PixelShaderOuputVariable> *outPixelShaderKey,
                                bool *outUsesFragDepth) const;

    std::string generateGeometryShaderHLSL(int registers, rx::FragmentShaderD3D *fragmentShader, rx::VertexShaderD3D *vertexShader) const;
    void getInputLayoutSignature(const gl::VertexFormat inputLayout[], GLenum signature[]) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(DynamicHLSL);

    rx::Renderer *const mRenderer;

    struct SemanticInfo;

    std::string getVaryingSemantic(bool pointSize) const;
    SemanticInfo getSemanticInfo(int startRegisters, bool fragCoord, bool pointCoord, bool pointSize,
                                        bool pixelShader) const;
    std::string generateVaryingLinkHLSL(const SemanticInfo &info, const std::string &varyingHLSL) const;
    std::string generateVaryingHLSL(rx::VertexShaderD3D *shader) const;
    void storeUserLinkedVaryings(const rx::VertexShaderD3D *vertexShader, std::vector<gl::LinkedVarying> *linkedVaryings) const;
    void storeBuiltinLinkedVaryings(const SemanticInfo &info, std::vector<gl::LinkedVarying> *linkedVaryings) const;
    void defineOutputVariables(rx::FragmentShaderD3D *fragmentShader, std::map<int, gl::VariableLocation> *programOutputVars) const;
    std::string generatePointSpriteHLSL(int registers, rx::FragmentShaderD3D *fragmentShader, rx::VertexShaderD3D *vertexShader) const;

    
    static std::string decorateVariable(const std::string &name);

    std::string generateAttributeConversionHLSL(const gl::VertexFormat &vertexFormat, const sh::ShaderVariable &shaderAttrib) const;
};

}

#endif 
