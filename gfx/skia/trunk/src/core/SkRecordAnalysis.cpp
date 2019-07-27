#include "SkRecordAnalysis.h"

#include "SkShader.h"
#include "SkTLogic.h"








struct BitmapTester {
    
    SK_CREATE_MEMBER_DETECTOR(bitmap);
    SK_CREATE_MEMBER_DETECTOR(paint);

    
    static const SkPaint* AsPtr(const SkPaint& p) { return &p; }
    static const SkPaint* AsPtr(const SkRecords::Optional<SkPaint>& p) { return p; }


    
    
    
    
    template <typename T>
    bool operator()(const T& r) { return CheckBitmap(r); }


    
    template <typename T>
    static SK_WHEN(HasMember_bitmap<T>, bool) CheckBitmap(const T&) { return true; }

    
    template <typename T>
    static SK_WHEN(!HasMember_bitmap<T>, bool) CheckBitmap(const T& r) { return CheckPaint(r); }

    
    template <typename T>
    static SK_WHEN(HasMember_paint<T>, bool) CheckPaint(const T& r) {
        const SkPaint* paint = AsPtr(r.paint);
        if (paint) {
            const SkShader* shader = paint->getShader();
            if (shader &&
                shader->asABitmap(NULL, NULL, NULL) == SkShader::kDefault_BitmapType) {
                return true;
            }
        }
        return false;
    }

    
    template <typename T>
    static SK_WHEN(!HasMember_paint<T>, bool) CheckPaint(const T&) { return false; }
};

bool SkRecordWillPlaybackBitmaps(const SkRecord& record) {
    BitmapTester tester;
    for (unsigned i = 0; i < record.count(); i++) {
        if (record.visit<bool>(i, tester)) {
            return true;
        }
    }
    return false;
}
