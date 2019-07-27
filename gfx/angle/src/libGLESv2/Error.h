







#ifndef LIBGLESV2_ERROR_H_
#define LIBGLESV2_ERROR_H_

#include "angle_gl.h"

#include <string>

namespace gl
{

class Error
{
  public:
    explicit Error(GLenum errorCode);
    Error(GLenum errorCode, const char *msg, ...);
    Error(const Error &other);
    Error &operator=(const Error &other);

    GLenum getCode() const { return mCode; }
    bool isError() const { return (mCode != GL_NO_ERROR); }

    const std::string &getMessage() const { return mMessage; }

  private:
    GLenum mCode;
    std::string mMessage;
};

}

#endif 
