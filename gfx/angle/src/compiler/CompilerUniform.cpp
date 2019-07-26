





#include "CompilerUniform.h"

namespace sh
{

Uniform::Uniform(GLenum type, GLenum precision, const char *name, int arraySize, int registerIndex)
{
    this->type = type;
    this->precision = precision;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
}

}
