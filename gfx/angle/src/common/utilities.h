







#ifndef LIBGLESV2_UTILITIES_H
#define LIBGLESV2_UTILITIES_H

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <math.h>

namespace gl
{

int UniformComponentCount(GLenum type);
GLenum UniformComponentType(GLenum type);
size_t UniformComponentSize(GLenum type);
size_t UniformInternalSize(GLenum type);
size_t UniformExternalSize(GLenum type);
GLenum UniformBoolVectorType(GLenum type);
int VariableRowCount(GLenum type);
int VariableColumnCount(GLenum type);
bool IsSampler(GLenum type);
bool IsMatrixType(GLenum type);
GLenum TransposeMatrixType(GLenum type);
int AttributeRegisterCount(GLenum type);
int MatrixRegisterCount(GLenum type, bool isRowMajorMatrix);
int MatrixComponentCount(GLenum type, bool isRowMajorMatrix);

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

bool IsCubemapTextureTarget(GLenum target);
bool IsInternalTextureTarget(GLenum target, GLuint clientVersion);

bool IsTriangleMode(GLenum drawMode);




template <typename outT> outT iround(GLfloat value) { return static_cast<outT>(value > 0.0f ? floor(value + 0.5f) : ceil(value - 0.5f)); }
template <typename outT> outT uiround(GLfloat value) { return static_cast<outT>(value + 0.5f); }

}

std::string getTempPath();
void writeFile(const char* path, const void* data, size_t size);

#endif  
