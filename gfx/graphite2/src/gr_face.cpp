

























#include "graphite2/Font.h"
#include "inc/Face.h"
#include "inc/CachedFace.h"


using namespace graphite2;

extern "C" {


gr_face* gr_make_face(const void* appFaceHandle, gr_get_table_fn getTable, unsigned int faceOptions)
                  
{
    Face *res = new Face(appFaceHandle, getTable);

    if (res->getTable(Tag::Silf) == 0)
    {
		if (!(faceOptions & gr_face_dumbRendering))
		{
			delete res;
			return 0;
		}
    }
    else
    	faceOptions &= ~gr_face_dumbRendering;

    bool valid = true;
    valid &= res->readGlyphs(faceOptions);
    if (!valid) {
        delete res;
        return 0;
    }
    valid &= res->readFeatures();
    valid &= res->readGraphite();
    
    if (!(faceOptions & gr_face_dumbRendering) && !valid) {
        delete res;
        return 0;
    }
    return static_cast<gr_face *>(res);
}

#ifndef GRAPHITE2_NSEGCACHE
gr_face* gr_make_face_with_seg_cache(const void* appFaceHandle, gr_get_table_fn getTable, unsigned int cacheSize, unsigned int faceOptions)
                  
{
    CachedFace *res = new CachedFace(appFaceHandle, getTable);

    if (res->getTable(Tag::Silf) == 0)
    {
		if (!(faceOptions & gr_face_dumbRendering))
		{
			delete res;
			return 0;
		}
    }
    else
    	faceOptions &= ~gr_face_dumbRendering;

    bool valid = true;
    valid &= res->readGlyphs(faceOptions);
    if (!valid) {
        delete res;
        return 0;
    }
    valid &= res->readFeatures();
    valid &= res->readGraphite();
    valid &= res->setupCache(cacheSize);

    if (!(faceOptions & gr_face_dumbRendering) && !valid) {
        delete res;
        return 0;
    }
    return static_cast<gr_face *>(static_cast<Face *>(res));
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
        

gr_feature_val* gr_face_featureval_for_lang(const gr_face* pFace, gr_uint32 langname) 
{
    assert(pFace);
    if (langname == 0x20202020) langname = 0;
    else if ((langname & 0x00FFFFFF) == 0x00202020) langname = langname & 0xFF000000;
    else if ((langname & 0x0000FFFF) == 0x00002020) langname = langname & 0xFFFF0000;
    else if ((langname & 0x000000FF) == 0x00000020) langname = langname & 0xFFFFFF00;
    return static_cast<gr_feature_val *>(pFace->theSill().cloneFeatures(langname));
}


const gr_feature_ref* gr_face_find_fref(const gr_face* pFace, gr_uint32 featId)  
{
    assert(pFace);
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

#if 0      

uint16 *face_name(const gr_face * pFace, uint16 nameid, uint16 lid)
{
    size_t nLen = 0, lOffset = 0, lSize = 0;
    const void *pName = pFace->getTable(tagName, &nLen);
    uint16 *res;
    if (!pName || !TtfUtil::GetNameInfo(pName, 3, 0, lid, nameid, lOffset, lSize))
        return NULL;
    lSize >>= 1;
    res = gralloc<uint16>(lSize + 1);
    for (size_t i = 0; i < lSize; ++i)
        res[i] = swap16(*(uint16 *)((char *)pName + lOffset));
    res[lSize] = 0;
    return res;
}
#endif

unsigned short gr_face_n_glyphs(const gr_face* pFace)
{
    return pFace->getGlyphFaceCache()->numGlyphs();
}


#ifndef GRAPHITE2_NFILEFACE
gr_face* gr_make_file_face(const char *filename, unsigned int faceOptions)
{
    FileFace* pFileFace = new FileFace(filename);
    if (pFileFace->m_pTableDir)
    {
      gr_face* pRes =gr_make_face(pFileFace, &FileFace::table_fn, faceOptions);
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
    if (pFileFace->m_pTableDir)
    {
      gr_face* pRes = gr_make_face_with_seg_cache(pFileFace, &FileFace::table_fn, segCacheMaxSize, faceOptions);
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


