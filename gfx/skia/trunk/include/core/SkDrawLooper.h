








#ifndef SkDrawLooper_DEFINED
#define SkDrawLooper_DEFINED

#include "SkFlattenable.h"

class SkCanvas;
class SkPaint;
struct SkRect;
class SkString;









class SK_API SkDrawLooper : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkDrawLooper)

    





    class SK_API Context : public SkNoncopyable {
    public:
        Context() {}
        virtual ~Context() {}

        













        virtual bool next(SkCanvas* canvas, SkPaint* paint) = 0;
    };

    







    virtual Context* createContext(SkCanvas*, void* storage) const = 0;

    



    virtual size_t contextSize() const = 0;


    









    virtual bool canComputeFastBounds(const SkPaint& paint) const;
    virtual void computeFastBounds(const SkPaint& paint,
                                   const SkRect& src, SkRect* dst) const;

    SK_TO_STRING_PUREVIRT()
    SK_DEFINE_FLATTENABLE_TYPE(SkDrawLooper)

protected:
    SkDrawLooper() {}
    SkDrawLooper(SkReadBuffer& buffer) : INHERITED(buffer) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif
