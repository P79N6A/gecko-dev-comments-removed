






#ifndef GrUniformHandle_DEFINED
#define GrUniformHandle_DEFINED

namespace {
inline int handle_to_index(GrGLUniformManager::UniformHandle h) { return ~h; }
inline GrGLUniformManager::UniformHandle index_to_handle(int i) { return ~i; }
}

#endif
