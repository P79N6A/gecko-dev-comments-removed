







#ifndef LIBGLESV2_UTILITIES_H
#define LIBGLESV2_UTILITIES_H

#include "angle_gl.h"
#include <string>
#include <math.h>

namespace gl
{

int VariableComponentCount(GLenum type);
GLenum VariableComponentType(GLenum type);
size_t VariableComponentSize(GLenum type);
size_t VariableInternalSize(GLenum type);
size_t VariableExternalSize(GLenum type);
GLenum VariableBoolVectorType(GLenum type);
int VariableRowCount(GLenum type);
int VariableColumnCount(GLenum type);
bool IsSampler(GLenum type);
bool IsMatrixType(GLenum type);
GLenum TransposeMatrixType(GLenum type);
int VariableRegisterCount(GLenum type);
int MatrixRegisterCount(GLenum type, bool isRowMajorMatrix);
int MatrixComponentCount(GLenum type, bool isRowMajorMatrix);
int VariableSortOrder(GLenum type);

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

bool IsCubemapTextureTarget(GLenum target);

bool IsTriangleMode(GLenum drawMode);




template <typename outT> outT iround(GLfloat value) { return static_cast<outT>(value > 0.0f ? floor(value + 0.5f) : ceil(value - 0.5f)); }
template <typename outT> outT uiround(GLfloat value) { return static_cast<outT>(value + 0.5f); }

}

std::string getTempPath();
void writeFile(const char* path, const void* data, size_t size);

#endif  
