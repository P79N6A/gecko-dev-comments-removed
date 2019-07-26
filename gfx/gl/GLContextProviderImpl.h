




#ifndef IN_GL_CONTEXT_PROVIDER_H
#error GLContextProviderImpl.h must only be included from GLContextProvider.h
#endif

#ifndef GL_CONTEXT_PROVIDER_NAME
#error GL_CONTEXT_PROVIDER_NAME not defined
#endif

class GL_CONTEXT_PROVIDER_NAME
{
public:
    typedef gfx::SurfaceCaps SurfaceCaps;
    




















    static already_AddRefed<GLContext>
    CreateForWindow(nsIWidget* widget);

    

















    static already_AddRefed<GLContext>
    CreateOffscreen(const gfxIntSize& size,
                    const SurfaceCaps& caps,
                    ContextFlags flags = ContextFlagsNone);

    


    static GLContext*
    GetGlobalContext(ContextFlags flags = ContextFlagsNone);
    
    


    static void
    Shutdown();
};
