








#ifndef SkFlattenable_DEFINED
#define SkFlattenable_DEFINED

#include "SkRefCnt.h"

class SkReadBuffer;
class SkWriteBuffer;

#define SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(flattenable) \
        SkFlattenable::Registrar(#flattenable, flattenable::CreateProc, \
                                 flattenable::GetFlattenableType());

#define SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP() static void InitializeFlattenables();

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(flattenable) \
    void flattenable::InitializeFlattenables() {

#define SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END \
    }

#define SK_DECLARE_UNFLATTENABLE_OBJECT() \
    virtual Factory getFactory() const SK_OVERRIDE { return NULL; }

#define SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(flattenable) \
    virtual Factory getFactory() const SK_OVERRIDE { return CreateProc; } \
    static SkFlattenable* CreateProc(SkReadBuffer& buffer) { \
        return SkNEW_ARGS(flattenable, (buffer)); \
    }




#define SK_DEFINE_FLATTENABLE_TYPE(flattenable) \
    static Type GetFlattenableType() { \
        return k##flattenable##_Type; \
    }







class SK_API SkFlattenable : public SkRefCnt {
public:
    enum Type {
        kSkColorFilter_Type,
        kSkDrawLooper_Type,
        kSkImageFilter_Type,
        kSkMaskFilter_Type,
        kSkPathEffect_Type,
        kSkPixelRef_Type,
        kSkRasterizer_Type,
        kSkShader_Type,
        kSkUnused_Type,     
        kSkXfermode_Type,
    };

    SK_DECLARE_INST_COUNT(SkFlattenable)

    typedef SkFlattenable* (*Factory)(SkReadBuffer&);

    SkFlattenable() {}

    



    virtual Factory getFactory() const = 0;

    

    const char* getTypeName() const { return FactoryToName(getFactory()); }

    static Factory NameToFactory(const char name[]);
    static const char* FactoryToName(Factory);
    static bool NameToType(const char name[], Type* type);

    static void Register(const char name[], Factory, Type);

    class Registrar {
    public:
        Registrar(const char name[], Factory factory, Type type) {
            SkFlattenable::Register(name, factory, type);
        }
    };

    



    virtual void flatten(SkWriteBuffer&) const;

protected:
    SkFlattenable(SkReadBuffer&) {}

private:
    static void InitializeFlattenablesIfNeeded();

    friend class SkGraphics;

    typedef SkRefCnt INHERITED;
};

#endif
