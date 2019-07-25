

























#pragma once

#include "Main.h"
#include "GlyphFace.h"
#include "Silf.h"
#include "TtfUtil.h"
#include "Main.h"
#include "graphite2/Font.h"
#include "FeatureMap.h"
#include "GlyphFaceCache.h"

#ifndef DISABLE_FILE_FACE
#include <cstdio>
#include <cassert>
#include "TtfTypes.h"
#endif      

namespace graphite2 {

class Segment;
class FeatureVal;
class NameTable;
class Cmap;

using TtfUtil::Tag;



#ifndef DISABLE_FILE_FACE
class TableCacheItem
{
public:
    TableCacheItem(char * theData, size_t theSize) : m_data(theData), m_size(theSize) {}
    TableCacheItem() : m_data(0), m_size(0) {}
    ~TableCacheItem()
    {
        if (m_size) free(m_data);
    }
    void set(char * theData, size_t theSize) { m_data = theData; m_size = theSize; }
    const void * data() const { return m_data; }
    size_t size() const { return m_size; }
private:
    char * m_data;
    size_t m_size;
};
#endif      




class FileFace
{
#ifndef DISABLE_FILE_FACE
public:
    static const void *table_fn(const void* appFaceHandle, unsigned int name, size_t *len);
  
    FileFace(const char *filename);
    ~FileFace();

    bool isValid() const { return m_pfile && m_pHeader && m_pTableDir; }

    CLASS_NEW_DELETE
public:     
    FILE* m_pfile;
    unsigned int m_lfile;
    mutable TableCacheItem m_tables[18];
    TtfUtil::Sfnt::OffsetSubTable* m_pHeader;
    TtfUtil::Sfnt::OffsetSubTable::Entry* m_pTableDir;       
#endif      
   
private:        
    FileFace(const FileFace&);
    FileFace& operator=(const FileFace&);
};

class Face
{
public:
    const byte *getTable(const Tag name, size_t  * len = 0) const {
    	size_t tbl_len=0;
    	const byte * const tbl = reinterpret_cast<const byte *>((*m_getTable)(m_appFaceHandle, name, &tbl_len));
    	if (len) *len = tbl_len;
    	return TtfUtil::CheckTable(name, tbl, tbl_len) ? tbl : 0;
    }
    float advance(unsigned short id) const { return m_pGlyphFaceCache->glyph(id)->theAdvance().x; }
    const Silf *silf(int i) const { return ((i < m_numSilf) ? m_silfs + i : (const Silf *)NULL); }
    virtual bool runGraphite(Segment *seg, const Silf *silf) const;
    uint16 findPseudo(uint32 uid) const { return (m_numSilf) ? m_silfs[0].findPseudo(uid) : 0; }

public:
    Face(const void* appFaceHandle, gr_get_table_fn getTable2) : 
        m_appFaceHandle(appFaceHandle), m_getTable(getTable2), m_pGlyphFaceCache(NULL),
        m_cmap(NULL), m_numSilf(0), m_silfs(NULL), m_pFileFace(NULL),
        m_pNames(NULL) {}
    virtual ~Face();
public:
    float getAdvance(unsigned short glyphid, float scale) const { return advance(glyphid) * scale; }
    const Rect &theBBoxTemporary(uint16 gid) const { return m_pGlyphFaceCache->glyph(gid)->theBBox(); }   
    unsigned short upem() const { return m_upem; }
    uint16 glyphAttr(uint16 gid, uint8 gattr) const { return m_pGlyphFaceCache->glyphAttr(gid, gattr); }

private:
    friend class Font;
    unsigned short numGlyphs() const { return m_pGlyphFaceCache->m_nGlyphs; }

public:
    bool readGlyphs(uint32 faceOptions);
    bool readGraphite();
    bool readFeatures() { return m_Sill.readFace(*this); }
    const Silf *chooseSilf(uint32 script) const;
    const SillMap& theSill() const { return m_Sill; }
    uint16 numFeatures() const { return m_Sill.m_FeatureMap.numFeats(); }
    const FeatureRef *featureById(uint32 id) const { return m_Sill.m_FeatureMap.findFeatureRef(id); }
    const FeatureRef *feature(uint16 index) const { return m_Sill.m_FeatureMap.feature(index); }
    uint16 getGlyphMetric(uint16 gid, uint8 metric) const;

    const GlyphFaceCache* getGlyphFaceCache() const { return m_pGlyphFaceCache; }      
    void takeFileFace(FileFace* pFileFace);
    Cmap & cmap() const { return *m_cmap; };
    NameTable * nameTable() const;
    uint16 languageForLocale(const char * locale) const;

    CLASS_NEW_DELETE
private:
    const void* m_appFaceHandle;
    gr_get_table_fn m_getTable;
    uint16 m_ascent;
    uint16 m_descent;
    
    
    
    mutable GlyphFaceCache* m_pGlyphFaceCache;      
    mutable Cmap * m_cmap; 
    unsigned short m_upem;          
protected:
    unsigned short m_numSilf;       
    Silf *m_silfs;                   
private:
    SillMap m_Sill;
    FileFace* m_pFileFace;      
    mutable NameTable* m_pNames;
    
private:        
    Face(const Face&);
    Face& operator=(const Face&);
};

} 

struct gr_face : public graphite2::Face {};
