#ifndef SkDeviceProperties_DEFINED
#define SkDeviceProperties_DEFINED


#include "SkFontLCDConfig.h"

struct SkDeviceProperties {
    struct Geometry {
        




        enum Orientation {
            kUnknown_Orientation      = 0x0,
            kKnown_Orientation        = 0x2,

            kHorizontal_Orientation   = 0x2,  
            kVertical_Orientation     = 0x3,

            kOrientationMask          = 0x3,
        };

        




        enum Layout {
            kUnknown_Layout   = 0x0,
            kKnown_Layout     = 0x8,

            kRGB_Layout       = 0x8,  
            kBGR_Layout       = 0xC,

            kLayoutMask       = 0xC,
        };

        Orientation getOrientation() {
            return static_cast<Orientation>(fGeometry & kOrientationMask);
        }
        Layout getLayout() {
            return static_cast<Layout>(fGeometry & kLayoutMask);
        }

        bool isOrientationKnown() {
            return SkToBool(fGeometry & kKnown_Orientation);
        }
        bool isLayoutKnown() {
            return SkToBool(fGeometry & kKnown_Layout);
        }

    private:
        
        static Orientation fromOldOrientation(SkFontLCDConfig::LCDOrientation orientation) {
            switch (orientation) {
            case SkFontLCDConfig::kHorizontal_LCDOrientation: return kHorizontal_Orientation;
            case SkFontLCDConfig::kVertical_LCDOrientation: return kVertical_Orientation;
            default: return kUnknown_Orientation;
            }
        }
        static Layout fromOldLayout(SkFontLCDConfig::LCDOrder order) {
            switch (order) {
            case SkFontLCDConfig::kRGB_LCDOrder: return kRGB_Layout;
            case SkFontLCDConfig::kBGR_LCDOrder: return kBGR_Layout;
            default: return kUnknown_Layout;
            }
        }
    public:
        static Geometry MakeDefault() {
            Orientation orientation = fromOldOrientation(SkFontLCDConfig::GetSubpixelOrientation()); 
            Layout layout = fromOldLayout(SkFontLCDConfig::GetSubpixelOrder()); 
            Geometry ret = { SkToU8(orientation | layout) };
            return ret;
        }

        static Geometry Make(Orientation orientation, Layout layout) {
            Geometry ret = { SkToU8(orientation | layout) };
            return ret;
        }

        uint8_t fGeometry;
    };

    static SkDeviceProperties MakeDefault() {
        SkDeviceProperties ret = { Geometry::MakeDefault(), SK_GAMMA_EXPONENT };
        return ret;
    }

    static SkDeviceProperties Make(Geometry geometry, SkScalar gamma) {
        SkDeviceProperties ret = { geometry, gamma };
        return ret;
    }

    

    Geometry fGeometry;

    
    SkScalar fGamma;
};

#endif
