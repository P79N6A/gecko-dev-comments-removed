




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
    




















    static already_AddRefed<GLContext>
    CreateForWindow(nsIWidget *aWidget);

    


















    static already_AddRefed<GLContext>
    CreateOffscreen(const gfxIntSize& aSize,
                    const ContextFormat& aFormat = ContextFormat::BasicRGBA32Format,
                    const ContextFlags aFlags = GLContext::ContextFlagsNone);

    


    static GLContext *
    GetGlobalContext( const ContextFlags aFlags = GLContext::ContextFlagsNone);

    


    static void
    Shutdown();
};
