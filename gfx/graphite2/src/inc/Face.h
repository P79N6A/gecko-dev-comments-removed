

























#pragma once

#include <stdio.h>

#include "graphite2/Font.h"

#include "inc/Main.h"
#include "inc/FeatureMap.h"
#include "inc/TtfUtil.h"
#include "inc/Silf.h"

namespace graphite2 {

class Cmap;
class FileFace;
class GlyphCache;
class NameTable;
class json;

using TtfUtil::Tag;



class Face
{
    
    Face(const Face&);
    Face& operator=(const Face&);

public:
	class Table;
    static float default_glyph_advance(const void* face_ptr, gr_uint16 glyphid);

    Face(const void* appFaceHandle, const gr_face_ops & ops);
    virtual ~Face();

    virtual bool        runGraphite(Segment *seg, const Silf *silf) const;

public:
    bool                readGlyphs(uint32 faceOptions);
    bool                readGraphite(const Table & silf);
    bool                readFeatures();
    void                takeFileFace(FileFace* pFileFace);

    const SillMap     & theSill() const;
    const GlyphCache  & glyphs() const;
    Cmap              & cmap() const;
    NameTable         * nameTable() const;
    void                setLogger(FILE *log_file);
    json              * logger() const throw();

    const Silf        * chooseSilf(uint32 script) const;
    uint16              languageForLocale(const char * locale) const;

    
    uint16              numFeatures() const;
    const FeatureRef  * featureById(uint32 id) const;
    const FeatureRef  * feature(uint16 index) const;

    
    uint16 getGlyphMetric(uint16 gid, uint8 metric) const;
    uint16 findPseudo(uint32 uid) const;

    CLASS_NEW_DELETE;
private:
    SillMap                 m_Sill;
    gr_face_ops             m_ops;
    const void            * m_appFaceHandle;    
    FileFace              * m_pFileFace;        
    mutable GlyphCache    * m_pGlyphFaceCache;  
    mutable Cmap          * m_cmap;             
    mutable NameTable     * m_pNames;
    mutable json          * m_logger;
protected:
    Silf                  * m_silfs;    
    uint16                  m_numSilf;  
private:
    uint16 m_ascent,
           m_descent;
};



inline
const SillMap & Face::theSill() const
{
    return m_Sill;
}

inline
uint16 Face::numFeatures() const
{
    return m_Sill.theFeatureMap().numFeats();
}

inline
const FeatureRef * Face::featureById(uint32 id) const
{
    return m_Sill.theFeatureMap().findFeatureRef(id);
}

inline
const FeatureRef *Face::feature(uint16 index) const
{
    return m_Sill.theFeatureMap().feature(index);
}

inline
const GlyphCache & Face::glyphs() const
{
	return *m_pGlyphFaceCache;
}

inline
Cmap & Face::cmap() const
{
    return *m_cmap;
};

inline
json * Face::logger() const throw()
{
    return m_logger;
}



class Face::Table
{
    const Face *            _f;
    mutable const byte *    _p;
    uint32                  _sz;

public:
    Table() throw();
    Table(const Face & face, const Tag n) throw();
    Table(const Table & rhs) throw();
    ~Table() throw();

    operator const byte * () const throw();

    Table & operator = (const Table & rhs) throw();
    size_t  size() const throw();
};

inline
Face::Table::Table() throw()
: _f(0), _p(0), _sz(0)
{
}

inline
Face::Table::Table(const Table & rhs) throw()
: _f(rhs._f), _p(rhs._p), _sz(rhs._sz)
{
	rhs._p = 0;
}

inline
Face::Table::~Table() throw()
{
	if (_p && _f->m_ops.release_table)
		(*_f->m_ops.release_table)(_f->m_appFaceHandle, _p);
}

inline
Face::Table::operator const byte * () const throw()
{
	return _p;
}

inline
size_t 	Face::Table::size() const throw()
{
	return _sz;
}

} 

struct gr_face : public graphite2::Face {};
