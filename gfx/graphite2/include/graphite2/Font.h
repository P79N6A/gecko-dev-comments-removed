

























#pragma once

#include "graphite2/Types.h"

#define GR2_VERSION_MAJOR   2
#define GR2_VERSION_MINOR   0
#define GR2_VERSION_BUGFIX  1

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct gr_face          gr_face;
typedef struct gr_font          gr_font;
typedef struct gr_feature_ref   gr_feature_ref;
typedef struct gr_feature_val   gr_feature_val;




GR2_API void gr_engine_version(int *nMajor, int *nMinor, int *nBugFix);







enum gr_face_options {
    
    gr_face_default = 0,
    
    gr_face_dumbRendering = 1,
    
    gr_face_preloadGlyphs = 2,
    
    gr_face_cacheCmap = 4,
    
    gr_face_preloadAll = 6
};









typedef const void *(*gr_get_table_fn)(const void* appFaceHandle, unsigned int name, size_t *len);










GR2_API gr_face* gr_make_face(const void* appFaceHandle, gr_get_table_fn getTable, unsigned int faceOptions);











GR2_API gr_face* gr_make_face_with_seg_cache(const void* appFaceHandle, gr_get_table_fn getTable, unsigned int segCacheMaxSize, unsigned int faceOptions);







GR2_API gr_uint32 gr_str_to_tag(const char *str);







GR2_API void gr_tag_to_str(gr_uint32 tag, char *str);











GR2_API gr_feature_val* gr_face_featureval_for_lang(const gr_face* pFace, gr_uint32 langname);








GR2_API const gr_feature_ref* gr_face_find_fref(const gr_face* pFace, gr_uint32 featId);


GR2_API gr_uint16 gr_face_n_fref(const gr_face* pFace);


GR2_API const gr_feature_ref* gr_face_fref(const gr_face* pFace, gr_uint16 i);


GR2_API unsigned short gr_face_n_languages(const gr_face* pFace);


GR2_API gr_uint32 gr_face_lang_by_index(const gr_face* pFace, gr_uint16 i);


GR2_API void gr_face_destroy(gr_face *face);


GR2_API unsigned short gr_face_n_glyphs(const gr_face* pFace);

#ifndef GRAPHITE2_NFILEFACE






GR2_API gr_face* gr_make_file_face(const char *filename, unsigned int faceOptions);









GR2_API gr_face* gr_make_file_face_with_seg_cache(const char *filename, unsigned int segCacheMaxSize, unsigned int faceOptions);

#endif      







GR2_API gr_font* gr_make_font(float ppm, const gr_face *face);


typedef float (*gr_advance_fn)(const void* appFontHandle, gr_uint16 glyphid);









GR2_API gr_font* gr_make_font_with_advance_fn(float ppm, const void* appFontHandle, gr_advance_fn advance, const gr_face *face);


GR2_API void gr_font_destroy(gr_font *font);







GR2_API gr_uint16 gr_fref_feature_value(const gr_feature_ref* pfeatureref, const gr_feature_val* feats);








GR2_API int gr_fref_set_feature_value(const gr_feature_ref* pfeatureref, gr_uint16 val, gr_feature_val* pDest);


GR2_API gr_uint32 gr_fref_id(const gr_feature_ref* pfeatureref);


GR2_API gr_uint16 gr_fref_n_values(const gr_feature_ref* pfeatureref);







GR2_API gr_int16 gr_fref_value(const gr_feature_ref* pfeatureref, gr_uint16 settingno);   










GR2_API void* gr_fref_label(const gr_feature_ref* pfeatureref, gr_uint16 *langId, enum gr_encform utf, gr_uint32 *length);












GR2_API void* gr_fref_value_label(const gr_feature_ref* pfeatureref, gr_uint16 settingno, gr_uint16 *langId, enum gr_encform utf, gr_uint32 *length);


GR2_API void gr_label_destroy(void * label);


GR2_API gr_feature_val* gr_featureval_clone(const gr_feature_val* pfeatures);


GR2_API void gr_featureval_destroy(gr_feature_val *pfeatures);

#ifdef __cplusplus
}
#endif

