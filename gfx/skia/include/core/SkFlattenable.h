








#ifndef SkFlattenable_DEFINED
#define SkFlattenable_DEFINED

#include "SkRefCnt.h"

class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                       flattenable::CreateProc);
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
    static SkFlattenable::Registrar g##flattenable##Reg(#flattenable, \
                                                       flattenable::CreateProc);

#define SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable)
#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END

#else

#define SK_DEFINE_FLATTENABLE_REGISTRAR(flattenable)
#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
        SkFlattenable::Registrar(#flattenable, flattenable::CreateProc);

#define SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP() static void InitializeFlattenables();

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable) \
    void flattenable::InitializeFlattenables() {

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END \
    }

#endif

#define SK_DECLARE_UNFLATTENABLE_OBJECT() \
    virtual Factory getFactory() SK_OVERRIDE { return NULL; }; \

#define SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(flattenable) \
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; } \
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) { \
        return SkNEW_ARGS(flattenable, (buffer)); \
    }







class SK_API SkFlattenable : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkFlattenable)

    typedef SkFlattenable* (*Factory)(SkFlattenableReadBuffer&);

    SkFlattenable() {}

    



    virtual Factory getFactory() = 0;

    static Factory NameToFactory(const char name[]);
    static const char* FactoryToName(Factory);
    static void Register(const char name[], Factory);

    class Registrar {
    public:
        Registrar(const char name[], Factory factory) {
            SkFlattenable::Register(name, factory);
        }
    };

protected:
    SkFlattenable(SkFlattenableReadBuffer&) {}
    



    virtual void flatten(SkFlattenableWriteBuffer&) const;

private:
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS
    static void InitializeFlattenables();
#endif

    friend class SkGraphics;
    friend class SkFlattenableWriteBuffer;

    typedef SkRefCnt INHERITED;
};

#endif
