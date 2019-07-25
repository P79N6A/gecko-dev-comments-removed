

























#pragma once

#include "graphite2/Types.h"
#include "graphite2/Font.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum gr_break_weight {
    gr_breakNone = 0,
    
    gr_breakWhitespace = 10,
    gr_breakWord = 15,
    gr_breakIntra = 20,
    gr_breakLetter = 30,
    gr_breakClip = 40,
    
    gr_breakBeforeWhitespace = -10,
    gr_breakBeforeWord = -15,
    gr_breakBeforeIntra = -20,
    gr_breakBeforeLetter = -30,
    gr_breakBeforeClip = -40
};

enum gr_justFlags {
    
    gr_justStartInline = 1,
    
    gr_justEndInline = 2
};


enum gr_attrCode {
    
    gr_slatAdvX = 0,        
    
    gr_slatAdvY,            
    
    gr_slatAttTo,           
    
    gr_slatAttX,            
    
    gr_slatAttY,            
    
    gr_slatAttGpt,          
    
    gr_slatAttXOff,         
    
    gr_slatAttYOff,         
    
    gr_slatAttWithX,        
    
    gr_slatAttWithY,        
    
    gr_slatWithGpt,         
    
    gr_slatAttWithXOff,     
    
    gr_slatAttWithYOff,     
    
    gr_slatAttLevel,        
    
    gr_slatBreak,           
    
    gr_slatCompRef,         
    
    gr_slatDir,             
    
    gr_slatInsert,          
    
    gr_slatPosX,            
    
    gr_slatPosY,            
    
    gr_slatShiftX,          
    
    gr_slatShiftY,          
    
    gr_slatUserDefnV1,      
    
    gr_slatMeasureSol,      
    
    gr_slatMeasureEol,      
    
    gr_slatJStretch,        
    
    gr_slatJShrink,         
    
    gr_slatJStep,           
    
    gr_slatJWeight,         
    
    gr_slatJWidth,          
    
    gr_slatUserDefn = gr_slatJStretch + 30,
                            
    
    gr_slatMax,             
    
    gr_slatNoEffect = gr_slatMax + 1    
};

enum gr_bidirtl {
    
    gr_rtl = 1,
    
    
    gr_nobidi = 2,
    
    gr_nomirror = 4
};

typedef struct gr_char_info     gr_char_info;
typedef struct gr_segment       gr_segment;
typedef struct gr_slot          gr_slot;





GR2_API unsigned int gr_cinfo_unicode_char(const gr_char_info* p);







GR2_API int gr_cinfo_break_weight(const gr_char_info* p);






GR2_API int gr_cinfo_after(const gr_char_info* p);






GR2_API int gr_cinfo_before(const gr_char_info* p);






GR2_API size_t gr_cinfo_base(const gr_char_info* p);












GR2_API size_t gr_count_unicode_characters(enum gr_encform enc, const void* buffer_begin, const void* buffer_end, const void** pError);


























GR2_API gr_segment* gr_make_seg(const gr_font* font, const gr_face* face, gr_uint32 script, const gr_feature_val* pFeats, enum gr_encform enc, const void* pStart, size_t nChars, int dir);





GR2_API void gr_seg_destroy(gr_segment* p);





GR2_API float gr_seg_advance_X(const gr_segment* pSeg);


GR2_API float gr_seg_advance_Y(const gr_segment* pSeg);


GR2_API unsigned int gr_seg_n_cinfo(const gr_segment* pSeg);


GR2_API const gr_char_info* gr_seg_cinfo(const gr_segment* pSeg, unsigned int index);


GR2_API unsigned int gr_seg_n_slots(const gr_segment* pSeg);      






GR2_API const gr_slot* gr_seg_first_slot(gr_segment* pSeg);    





GR2_API const gr_slot* gr_seg_last_slot(gr_segment* pSeg);    

















GR2_API void gr_seg_justify(gr_segment* pSeg, gr_slot* pStart, const gr_font *pFont, double width, enum gr_justFlags flags, gr_slot* pFirst, gr_slot* pLast);






GR2_API const gr_slot* gr_slot_next_in_segment(const gr_slot* p);







GR2_API const gr_slot* gr_slot_prev_in_segment(const gr_slot* p);






GR2_API const gr_slot* gr_slot_attached_to(const gr_slot* p);









GR2_API const gr_slot* gr_slot_first_attachment(const gr_slot* p);










GR2_API const gr_slot* gr_slot_next_sibling_attachment(const gr_slot* p);







GR2_API unsigned short gr_slot_gid(const gr_slot* p);


GR2_API float gr_slot_origin_X(const gr_slot* p);


GR2_API float gr_slot_origin_Y(const gr_slot* p);







GR2_API float gr_slot_advance_X(const gr_slot* p, const gr_face* face, const gr_font *font);





GR2_API float gr_slot_advance_Y(const gr_slot* p, const gr_face* face, const gr_font *font);






GR2_API int gr_slot_before(const gr_slot* p);






GR2_API int gr_slot_after(const gr_slot* p);






GR2_API unsigned int gr_slot_index(const gr_slot* p);






GR2_API int gr_slot_attr(const gr_slot* p, const gr_segment* pSeg, enum gr_attrCode index, gr_uint8 subindex); 


GR2_API int gr_slot_can_insert_before(const gr_slot* p);






GR2_API int gr_slot_original(const gr_slot* p);






GR2_API void gr_slot_linebreak_before(gr_slot *p);

#ifdef __cplusplus
}
#endif
