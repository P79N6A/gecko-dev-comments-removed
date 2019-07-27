







#ifndef LIBGLESV2_STATE_H_
#define LIBGLESV2_STATE_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/TransformFeedback.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/Sampler.h"

namespace gl
{
class Query;
class VertexArray;
class Context;

class State
{
  public:
    State();
    ~State();

    void setContext(Context *context) { mContext = context; }

    
    const RasterizerState &getRasterizerState() const;
    const BlendState &getBlendState() const;
    const DepthStencilState &getDepthStencilState() const;

    
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(float depth);
    void setClearStencil(int stencil);
    ClearParameters getClearParameters(GLbitfield mask) const;

    
    void setColorMask(bool red, bool green, bool blue, bool alpha);
    void setDepthMask(bool mask);

    
    bool isRasterizerDiscardEnabled() const;
    void setRasterizerDiscard(bool enabled);

    
    bool isCullFaceEnabled() const;
    void setCullFace(bool enabled);
    void setCullMode(GLenum mode);
    void setFrontFace(GLenum front);

    
    bool isDepthTestEnabled() const;
    void setDepthTest(bool enabled);
    void setDepthFunc(GLenum depthFunc);
    void setDepthRange(float zNear, float zFar);
    void getDepthRange(float *zNear, float *zFar) const;

    
    bool isBlendEnabled() const;
    void setBlend(bool enabled);
    void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
    void setBlendColor(float red, float green, float blue, float alpha);
    void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);
    const ColorF &getBlendColor() const;

    
    bool isStencilTestEnabled() const;
    void setStencilTest(bool enabled);
    void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
    void setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask);
    void setStencilWritemask(GLuint stencilWritemask);
    void setStencilBackWritemask(GLuint stencilBackWritemask);
    void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);
    void setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass);
    GLint getStencilRef() const;
    GLint getStencilBackRef() const;

    
    bool isPolygonOffsetFillEnabled() const;
    void setPolygonOffsetFill(bool enabled);
    void setPolygonOffsetParams(GLfloat factor, GLfloat units);

    
    bool isSampleAlphaToCoverageEnabled() const;
    void setSampleAlphaToCoverage(bool enabled);
    bool isSampleCoverageEnabled() const;
    void setSampleCoverage(bool enabled);
    void setSampleCoverageParams(GLclampf value, bool invert);
    void getSampleCoverageParams(GLclampf *value, bool *invert);

    
    bool isScissorTestEnabled() const;
    void setScissorTest(bool enabled);
    void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);
    const Rectangle &getScissor() const;

    
    bool isDitherEnabled() const;
    void setDither(bool enabled);

    
    void setEnableFeature(GLenum feature, bool enabled);
    bool getEnableFeature(GLenum feature);

    
    void setLineWidth(GLfloat width);

    
    void setGenerateMipmapHint(GLenum hint);
    void setFragmentShaderDerivativeHint(GLenum hint);

    
    void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);
    const Rectangle &getViewport() const;

    
    void setActiveSampler(unsigned int active);
    unsigned int getActiveSampler() const;
    void setSamplerTexture(TextureType type, Texture *texture);
    Texture *getSamplerTexture(unsigned int sampler, TextureType type) const;
    GLuint getSamplerTextureId(unsigned int sampler, TextureType type) const;
    void detachTexture(GLuint texture);

    
    void setSamplerBinding(GLuint textureUnit, Sampler *sampler);
    GLuint getSamplerId(GLuint textureUnit) const;
    Sampler *getSampler(GLuint textureUnit) const;
    void detachSampler(GLuint sampler);

    
    void setRenderbufferBinding(Renderbuffer *renderbuffer);
    GLuint getRenderbufferId() const;
    Renderbuffer *getCurrentRenderbuffer();
    void detachRenderbuffer(GLuint renderbuffer);

    
    void setReadFramebufferBinding(Framebuffer *framebuffer);
    void setDrawFramebufferBinding(Framebuffer *framebuffer);
    Framebuffer *getTargetFramebuffer(GLenum target) const;
    Framebuffer *getReadFramebuffer();
    Framebuffer *getDrawFramebuffer();
    const Framebuffer *getReadFramebuffer() const;
    const Framebuffer *getDrawFramebuffer() const;
    bool removeReadFramebufferBinding(GLuint framebuffer);
    bool removeDrawFramebufferBinding(GLuint framebuffer);

    
    void setVertexArrayBinding(VertexArray *vertexArray);
    GLuint getVertexArrayId() const;
    VertexArray *getVertexArray() const;
    bool removeVertexArrayBinding(GLuint vertexArray);

    
    void setCurrentProgram(GLuint programId, Program *newProgram);
    void setCurrentProgramBinary(ProgramBinary *binary);
    GLuint getCurrentProgramId() const;
    ProgramBinary *getCurrentProgramBinary() const;

    
    void setTransformFeedbackBinding(TransformFeedback *transformFeedback);
    TransformFeedback *getCurrentTransformFeedback() const;
    void detachTransformFeedback(GLuint transformFeedback);

    
    bool isQueryActive() const;
    void setActiveQuery(GLenum target, Query *query);
    GLuint getActiveQueryId(GLenum target) const;
    Query *getActiveQuery(GLenum target) const;

    
    
    void setArrayBufferBinding(Buffer *buffer);
    GLuint getArrayBufferId() const;
    bool removeArrayBufferBinding(GLuint buffer);

    
    void setGenericUniformBufferBinding(Buffer *buffer);
    void setIndexedUniformBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size);
    GLuint getIndexedUniformBufferId(GLuint index) const;
    Buffer *getIndexedUniformBuffer(GLuint index) const;

    
    void setGenericTransformFeedbackBufferBinding(Buffer *buffer);
    void setIndexedTransformFeedbackBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size);
    GLuint getIndexedTransformFeedbackBufferId(GLuint index) const;
    Buffer *getIndexedTransformFeedbackBuffer(GLuint index) const;
    GLuint getIndexedTransformFeedbackBufferOffset(GLuint index) const;

    
    void setCopyReadBufferBinding(Buffer *buffer);
    void setCopyWriteBufferBinding(Buffer *buffer);

    
    void setPixelPackBufferBinding(Buffer *buffer);
    void setPixelUnpackBufferBinding(Buffer *buffer);

    
    Buffer *getTargetBuffer(GLenum target) const;

    
    void setEnableVertexAttribArray(unsigned int attribNum, bool enabled);
    void setVertexAttribf(GLuint index, const GLfloat values[4]);
    void setVertexAttribu(GLuint index, const GLuint values[4]);
    void setVertexAttribi(GLuint index, const GLint values[4]);
    void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
                              bool normalized, bool pureInteger, GLsizei stride, const void *pointer);
    const VertexAttribute &getVertexAttribState(unsigned int attribNum) const;
    const VertexAttribCurrentValueData &getVertexAttribCurrentValue(unsigned int attribNum) const;
    const VertexAttribCurrentValueData *getVertexAttribCurrentValues() const;
    const void *getVertexAttribPointer(unsigned int attribNum) const;

    
    void setPackAlignment(GLint alignment);
    GLint getPackAlignment() const;
    void setPackReverseRowOrder(bool reverseRowOrder);
    bool getPackReverseRowOrder() const;
    const PixelPackState &getPackState() const;

    
    void setUnpackAlignment(GLint alignment);
    GLint getUnpackAlignment() const;
    const PixelUnpackState &getUnpackState() const;

    
    void getBooleanv(GLenum pname, GLboolean *params);
    void getFloatv(GLenum pname, GLfloat *params);
    void getIntegerv(GLenum pname, GLint *params);
    bool getIndexedIntegerv(GLenum target, GLuint index, GLint *data);
    bool getIndexedInteger64v(GLenum target, GLuint index, GLint64 *data);

    bool hasMappedBuffer(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(State);

    Context *mContext;

    ColorF mColorClearValue;
    GLclampf mDepthClearValue;
    int mStencilClearValue;

    RasterizerState mRasterizer;
    bool mScissorTest;
    Rectangle mScissor;

    BlendState mBlend;
    ColorF mBlendColor;
    bool mSampleCoverage;
    GLclampf mSampleCoverageValue;
    bool mSampleCoverageInvert;

    DepthStencilState mDepthStencil;
    GLint mStencilRef;
    GLint mStencilBackRef;

    GLfloat mLineWidth;

    GLenum mGenerateMipmapHint;
    GLenum mFragmentShaderDerivativeHint;

    Rectangle mViewport;
    float mNearZ;
    float mFarZ;

    unsigned int mActiveSampler;   
    BindingPointer<Buffer> mArrayBuffer;
    Framebuffer *mReadFramebuffer;
    Framebuffer *mDrawFramebuffer;
    BindingPointer<Renderbuffer> mRenderbuffer;
    GLuint mCurrentProgramId;
    BindingPointer<ProgramBinary> mCurrentProgramBinary;

    VertexAttribCurrentValueData mVertexAttribCurrentValues[MAX_VERTEX_ATTRIBS]; 
    VertexArray *mVertexArray;

    BindingPointer<Texture> mSamplerTexture[TEXTURE_TYPE_COUNT][IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];
    BindingPointer<Sampler> mSamplers[IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];

    typedef std::map< GLenum, BindingPointer<Query> > ActiveQueryMap;
    ActiveQueryMap mActiveQueries;

    BindingPointer<Buffer> mGenericUniformBuffer;
    OffsetBindingPointer<Buffer> mUniformBuffers[IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS];

    BindingPointer<TransformFeedback> mTransformFeedback;
    BindingPointer<Buffer> mGenericTransformFeedbackBuffer;
    OffsetBindingPointer<Buffer> mTransformFeedbackBuffers[IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS];

    BindingPointer<Buffer> mCopyReadBuffer;
    BindingPointer<Buffer> mCopyWriteBuffer;

    PixelUnpackState mUnpack;
    PixelPackState mPack;
};

}

#endif 

