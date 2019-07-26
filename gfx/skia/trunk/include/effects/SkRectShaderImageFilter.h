






#ifndef SkRectShaderImageFilter_DEFINED
#define SkRectShaderImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkRect.h"

class SkShader;

class SK_API SkRectShaderImageFilter : public SkImageFilter {
public:
    









    SK_ATTR_DEPRECATED("use Create(SkShader*, const CropRect*)")
    static SkRectShaderImageFilter* Create(SkShader* s, const SkRect& rect);

    static SkRectShaderImageFilter* Create(SkShader* s, const CropRect* rect = NULL);
    virtual ~SkRectShaderImageFilter();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkRectShaderImageFilter)

protected:
    SkRectShaderImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;

private:
    SkRectShaderImageFilter(SkShader* s, const CropRect* rect);
    SkShader*  fShader;

    typedef SkImageFilter INHERITED;
};

#endif
