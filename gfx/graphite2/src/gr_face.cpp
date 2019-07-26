

























#include "graphite2/Font.h"
#include "inc/Face.h"
#include "inc/FileFace.h"
#include "inc/GlyphCache.h"
#include "inc/CachedFace.h"
#include "inc/CmapCache.h"
#include "inc/Silf.h"

using namespace graphite2;

namespace
{
    bool load_face(Face & face, unsigned int options)
    {
        Face::Table silf(face, Tag::Silf);
        if (silf)   options &= ~gr_face_dumbRendering;
        else if (!(options &  gr_face_dumbRendering))
            return false;

        if (!face.readGlyphs(options))
            return false;

        return silf ? face.readFeatures() && face.readGraphite(silf)
                    : options & gr_face_dumbRendering;
    }
}

extern "C" {

gr_face* gr_make_face_with_ops(const void* appFaceHandle, const gr_face_ops *ops, unsigned int faceOptions)
                  
{
	if (ops == 0)	return 0;

    Face *res = new Face(appFaceHandle, *ops);
    if (res && load_face(*res, faceOptions))
        return static_cast<gr_face *>(res);

    delete res;
    return 0;
}

gr_face* gr_make_face(const void* appFaceHandle, gr_get_table_fn tablefn, unsigned int faceOptions)
{
    const gr_face_ops ops = {sizeof(gr_face_ops), tablefn, NULL};
    return gr_make_face_with_ops(appFaceHandle, &ops, faceOptions);
}

#ifndef GRAPHITE2_NSEGCACHE
gr_face* gr_make_face_with_seg_cache_and_ops(const void* appFaceHandle, const gr_face_ops *ops, unsigned int cacheSize, unsigned int faceOptions)
                  
{
	if (ops == 0)	return 0;

    CachedFace *res = new CachedFace(appFaceHandle, *ops);
    if (res && load_face(*res, faceOptions)
            && res->setupCache(cacheSize))
        return static_cast<gr_face *>(static_cast<Face *>(res));

    delete res;
    return 0;
}

gr_face* gr_make_face_with_seg_cache(const void* appFaceHandle, gr_get_table_fn getTable, unsigned int cacheSize, unsigned int faceOptions)
{
    const gr_face_ops ops = {sizeof(gr_face_ops), getTable, NULL};
    return gr_make_face_with_seg_cache_and_ops(appFaceHandle, &ops, cacheSize, faceOptions);
}
#endif

gr_uint32 gr_str_to_tag(const char *str)
{
    uint32 res = 0;
    int i = strlen(str);
    if (i > 4) i = 4;
    while (--i >= 0)
        res = (res >> 8) + (str[i] << 24);
    return res;
}

void gr_tag_to_str(gr_uint32 tag, char *str)
{
    int i = 4;
    while (--i >= 0)
    {
        str[i] = tag & 0xFF;
        tag >>= 8;
    }
}

inline
uint32 zeropad(const uint32 x)
{
	if (x == 0x20202020) 					return 0;
	if ((x & 0x00FFFFFF) == 0x00202020)		return x & 0xFF000000;
	if ((x & 0x0000FFFF) == 0x00002020)		return x & 0xFFFF0000;
	if ((x & 0x000000FF) == 0x00000020)		return x & 0xFFFFFF00;
	return x;
}

gr_feature_val* gr_face_featureval_for_lang(const gr_face* pFace, gr_uint32 langname) 
{
    assert(pFace);
    zeropad(langname);
    return static_cast<gr_feature_val *>(pFace->theSill().cloneFeatures(langname));
}


const gr_feature_ref* gr_face_find_fref(const gr_face* pFace, gr_uint32 featId)  
{
    assert(pFace);
    zeropad(featId);
    const FeatureRef* pRef = pFace->featureById(featId);
    return static_cast<const gr_feature_ref*>(pRef);
}

unsigned short gr_face_n_fref(const gr_face* pFace)
{
    assert(pFace);
    return pFace->numFeatures();
}

const gr_feature_ref* gr_face_fref(const gr_face* pFace, gr_uint16 i) 
{
    assert(pFace);
    const FeatureRef* pRef = pFace->feature(i);
    return static_cast<const gr_feature_ref*>(pRef);
}

unsigned short gr_face_n_languages(const gr_face* pFace)
{
    assert(pFace);
    return pFace->theSill().numLanguages();
}

gr_uint32 gr_face_lang_by_index(const gr_face* pFace, gr_uint16 i)
{
    assert(pFace);
    return pFace->theSill().getLangName(i);
}


void gr_face_destroy(gr_face *face)
{
    delete face;
}


gr_uint16 gr_face_name_lang_for_locale(gr_face *face, const char * locale)
{
    if (face)
    {
        return face->languageForLocale(locale);
    }
    return 0;
}

unsigned short gr_face_n_glyphs(const gr_face* pFace)
{
    return pFace->glyphs().numGlyphs();
}

const gr_faceinfo *gr_face_info(const gr_face *pFace, gr_uint32 script)
{
    if (!pFace) return 0;
    const Silf *silf = pFace->chooseSilf(script);
    if (silf) return silf->silfInfo();
    return 0;
}

int gr_face_is_char_supported(const gr_face* pFace, gr_uint32 usv, gr_uint32 script)
{
    const Cmap & cmap = pFace->cmap();
    gr_uint16 gid = cmap[usv];
    if (!gid)
    {
        const Silf * silf = pFace->chooseSilf(script);
        gid = silf->findPseudo(usv);
    }
    return (gid != 0);
}

#ifndef GRAPHITE2_NFILEFACE
gr_face* gr_make_file_face(const char *filename, unsigned int faceOptions)
{
    FileFace* pFileFace = new FileFace(filename);
    if (*pFileFace)
    {
      gr_face* pRes = gr_make_face_with_ops(pFileFace, &FileFace::ops, faceOptions);
      if (pRes)
      {
        pRes->takeFileFace(pFileFace);        
        return pRes;
      }
    }
    
    

    delete pFileFace;
    return NULL;
}

#ifndef GRAPHITE2_NSEGCACHE
gr_face* gr_make_file_face_with_seg_cache(const char* filename, unsigned int segCacheMaxSize, unsigned int faceOptions)   
                  
{
    FileFace* pFileFace = new FileFace(filename);
    if (*pFileFace)
    {
      gr_face * pRes = gr_make_face_with_seg_cache_and_ops(pFileFace, &FileFace::ops, segCacheMaxSize, faceOptions);
      if (pRes)
      {
        pRes->takeFileFace(pFileFace);        
        return pRes;
      }
    }

    

    delete pFileFace;
    return NULL;
}
#endif
#endif      


} 


