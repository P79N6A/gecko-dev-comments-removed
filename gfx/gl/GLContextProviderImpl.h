




#ifndef IN_GL_CONTEXT_PROVIDER_H
#error GLContextProviderImpl.h must only be included from GLContextProvider.h
#endif

#ifndef GL_CONTEXT_PROVIDER_NAME
#error GL_CONTEXT_PROVIDER_NAME not defined
#endif
#if defined(ANDROID)
typedef void* EGLSurface;
#endif 

class GL_CONTEXT_PROVIDER_NAME
{
public:
    




















    static already_AddRefed<GLContext>
    CreateForWindow(nsIWidget* widget);

    

















    static already_AddRefed<GLContext>
    CreateOffscreen(const gfxIntSize& size,
                    const SurfaceCaps& caps,
                    bool requireCompatProfile);

    
    static already_AddRefed<GLContext>
    CreateHeadless(bool requireCompatProfile);

    







    static already_AddRefed<GLContext>
    CreateWrappingExisting(void* aContext, void* aSurface);

#if defined(ANDROID)
    static EGLSurface CreateEGLSurface(void* aWindow);
    static void DestroyEGLSurface(EGLSurface surface);
#endif 

    


    static GLContext*
    GetGlobalContext();

    


    static void
    Shutdown();
};
