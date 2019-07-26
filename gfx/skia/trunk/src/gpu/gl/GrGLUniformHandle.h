






#ifndef GrUniformHandle_DEFINED
#define GrUniformHandle_DEFINED

inline GrGLUniformManager::UniformHandle GrGLUniformManager::UniformHandle::CreateFromUniformIndex(int index) {
    return GrGLUniformManager::UniformHandle(index);
}

#endif
