




#ifndef IN_GL_CONTEXT_PROVIDER_H
#error GLContextProviderImpl.h must only be included from GLContextProvider.h
#endif

#ifndef GL_CONTEXT_PROVIDER_NAME
#error GL_CONTEXT_PROVIDER_NAME not defined
#endif

class THEBES_API GL_CONTEXT_PROVIDER_NAME
{
public:
    typedef GLContext::ContextFlags ContextFlags;
    typedef gfx::SurfaceCaps SurfaceCaps;
    




















    static already_AddRefed<GLContext>
    CreateForWindow(nsIWidget* widget);

    

















    static already_AddRefed<GLContext>
    CreateOffscreen(const gfxIntSize& size,
                    const SurfaceCaps& caps,
                    ContextFlags flags = GLContext::ContextFlagsNone);

    


    static GLContext*
    GetGlobalContext(ContextFlags flags = GLContext::ContextFlagsNone);

    


    static void
    Shutdown();
};
