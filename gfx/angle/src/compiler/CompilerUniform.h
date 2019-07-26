





#ifndef COMPILER_UNIFORM_H_
#define COMPILER_UNIFORM_H_

#include <string>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

namespace sh
{

struct Uniform
{
    Uniform(GLenum type, GLenum precision, const char *name, int arraySize, int registerIndex);

    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;
    
    int registerIndex;
};

typedef std::vector<Uniform> ActiveUniforms;

}

#endif   
