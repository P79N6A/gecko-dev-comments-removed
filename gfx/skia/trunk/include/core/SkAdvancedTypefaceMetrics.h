








#ifndef SkAdvancedTypefaceMetrics_DEFINED
#define SkAdvancedTypefaceMetrics_DEFINED

#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkString.h"
#include "SkTDArray.h"
#include "SkTemplates.h"








class SkAdvancedTypefaceMetrics : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkAdvancedTypefaceMetrics)

    SkString fFontName;

    enum FontType {
        kType1_Font,
        kType1CID_Font,
        kCFF_Font,
        kTrueType_Font,
        kOther_Font,
    };
    
    
    
    FontType fType;

    enum FontFlags {
        kEmpty_FontFlag          = 0x0,  
        kMultiMaster_FontFlag    = 0x1,  
        kNotEmbeddable_FontFlag  = 0x2,  
        kNotSubsettable_FontFlag = 0x4,  
    };
    
    FontFlags fFlags;

    uint16_t fLastGlyphID; 
    uint16_t fEmSize;  

    
    enum StyleFlags {
        kFixedPitch_Style  = 0x00001,
        kSerif_Style       = 0x00002,
        kScript_Style      = 0x00008,
        kItalic_Style      = 0x00040,
        kAllCaps_Style     = 0x10000,
        kSmallCaps_Style   = 0x20000,
        kForceBold_Style   = 0x40000
    };
    uint16_t fStyle;        
    int16_t fItalicAngle;   
                            
    
    int16_t fAscent;       
    int16_t fDescent;      
    int16_t fStemV;        
    int16_t fCapHeight;    

    SkIRect fBBox;  

    
    enum PerGlyphInfo {
      kNo_PerGlyphInfo         = 0x0, 
      kHAdvance_PerGlyphInfo   = 0x1, 
      kVAdvance_PerGlyphInfo   = 0x2, 
      kGlyphNames_PerGlyphInfo = 0x4, 
      kToUnicode_PerGlyphInfo  = 0x8  
                                      
    };

    template <typename Data>
    struct AdvanceMetric {
        enum MetricType {
            kDefault,  
            kRange,    
            kRun       
        };
        MetricType fType;
        uint16_t fStartId;
        uint16_t fEndId;
        SkTDArray<Data> fAdvance;
        SkAutoTDelete<AdvanceMetric<Data> > fNext;
    };

    struct VerticalMetric {
        int16_t fVerticalAdvance;
        int16_t fOriginXDisp;  
        int16_t fOriginYDisp;  
    };
    typedef AdvanceMetric<int16_t> WidthRange;
    typedef AdvanceMetric<VerticalMetric> VerticalAdvanceRange;

    
    SkAutoTDelete<WidthRange> fGlyphWidths;
    
    SkAutoTDelete<VerticalAdvanceRange> fVerticalMetrics;

    
    SkAutoTDelete<SkAutoTArray<SkString> > fGlyphNames;

    
    
    SkTDArray<SkUnichar> fGlyphToUnicode;

private:
    typedef SkRefCnt INHERITED;
};

namespace skia_advanced_typeface_metrics_utils {

template <typename Data>
void resetRange(SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range,
                       int startId);

template <typename Data>
SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* appendRange(
        SkAutoTDelete<SkAdvancedTypefaceMetrics::AdvanceMetric<Data> >* nextSlot,
        int startId);

template <typename Data>
void finishRange(
        SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range,
        int endId,
        typename SkAdvancedTypefaceMetrics::AdvanceMetric<Data>::MetricType
                type);










template <typename Data, typename FontHandle>
SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* getAdvanceData(
        FontHandle fontHandle,
        int num_glyphs,
        const uint32_t* glyphIDs,
        uint32_t glyphIDsCount,
        bool (*getAdvance)(FontHandle fontHandle, int gId, Data* data));

} 

#endif
