






#ifndef SkLumaColorFilter_DEFINED
#define SkLumaColorFilter_DEFINED

#include "SkColorFilter.h"












class SK_API SkLumaColorFilter : public SkColorFilter {
public:
    static SkColorFilter* Create();

    virtual void filterSpan(const SkPMColor src[], int count, SkPMColor[]) const SK_OVERRIDE;

#if SK_SUPPORT_GPU
    virtual GrEffect* asNewEffect(GrContext*) const SK_OVERRIDE;
#endif

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLumaColorFilter)

protected:
    SkLumaColorFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkLumaColorFilter();

    typedef SkColorFilter INHERITED;
};

#endif
