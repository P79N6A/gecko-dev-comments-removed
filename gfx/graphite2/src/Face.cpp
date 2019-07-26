

























#include <cstring>
#include "graphite2/Segment.h"
#include "inc/CmapCache.h"
#include "inc/debug.h"
#include "inc/Endian.h"
#include "inc/Face.h"
#include "inc/FileFace.h"
#include "inc/GlyphFace.h"
#include "inc/json.h"
#include "inc/SegCacheStore.h"
#include "inc/Segment.h"
#include "inc/NameTable.h"


using namespace graphite2;

Face::Face(const void* appFaceHandle, const gr_face_ops & ops)
: m_appFaceHandle(appFaceHandle),
  m_pFileFace(NULL),
  m_pGlyphFaceCache(NULL),
  m_cmap(NULL),
  m_pNames(NULL),
  m_logger(NULL),
  m_silfs(NULL),
  m_numSilf(0),
  m_ascent(0),
  m_descent(0)
{
    memset(&m_ops, 0, sizeof m_ops);
    memcpy(&m_ops, &ops, std::min(sizeof m_ops, ops.size));
}


Face::~Face()
{
    setLogger(0);
    delete m_pGlyphFaceCache;
    delete m_cmap;
    delete[] m_silfs;
#ifndef GRAPHITE2_NFILEFACE
    delete m_pFileFace;
#endif
    delete m_pNames;
}

float Face::default_glyph_advance(const void* font_ptr, gr_uint16 glyphid)
{
    const Font & font = *reinterpret_cast<const Font *>(font_ptr);

    return font.face().glyphs().glyph(glyphid)->theAdvance().x * font.scale();
}

bool Face::readGlyphs(uint32 faceOptions)
{
    if (faceOptions & gr_face_cacheCmap)
    	m_cmap = new CachedCmap(*this);
    else
    	m_cmap = new DirectCmap(*this);

    m_pGlyphFaceCache = new GlyphCache(*this, faceOptions);
    if (!m_pGlyphFaceCache
        || m_pGlyphFaceCache->numGlyphs() == 0
        || m_pGlyphFaceCache->unitsPerEm() == 0
    	|| !m_cmap || !*m_cmap)
    	return false;

    if (faceOptions & gr_face_preloadGlyphs)
        nameTable();        

    return true;
}

bool Face::readGraphite(const Table & silf)
{
    const byte * p = silf;
    if (!p) return false;

    const uint32 version = be::read<uint32>(p);
    if (version < 0x00020000) return false;
    if (version >= 0x00030000)
    	be::skip<uint32>(p);		
    m_numSilf = be::read<uint16>(p);
    be::skip<uint16>(p);			

    bool havePasses = false;
    m_silfs = new Silf[m_numSilf];
    for (int i = 0; i < m_numSilf; i++)
    {
        const uint32 offset = be::read<uint32>(p),
        			 next   = i == m_numSilf - 1 ? silf.size() : be::peek<uint32>(p);
        if (next > silf.size() || offset >= next)
            return false;

        if (!m_silfs[i].readGraphite(silf + offset, next - offset, *this, version))
            return false;

        if (m_silfs[i].numPasses())
            havePasses = true;
    }

    return havePasses;
}

bool Face::readFeatures()
{
    return m_Sill.readFace(*this);
}

bool Face::runGraphite(Segment *seg, const Silf *aSilf) const
{
#if !defined GRAPHITE2_NTRACING
    json * dbgout = logger();
    if (dbgout)
    {
    	*dbgout << json::object
    				<< "id"			<< objectid(seg)
    				<< "passes"		<< json::array;
    }
#endif

    bool res = aSilf->runGraphite(seg, 0, aSilf->justificationPass());
    res &= aSilf->runGraphite(seg, aSilf->positionPass(), aSilf->numPasses());

#if !defined GRAPHITE2_NTRACING
	if (dbgout)
{
		*dbgout 			<< json::item
							<< json::close 
				<< "output" << json::array;
		for(Slot * s = seg->first(); s; s = s->next())
			*dbgout		<< dslot(seg, s);
		seg->finalise(0);					
		*dbgout			<< json::close
				<< "advance" << seg->advance()
				<< "chars"	 << json::array;
		for(size_t i = 0, n = seg->charInfoCount(); i != n; ++i)
			*dbgout 	<< json::flat << *seg->charinfo(i);
		*dbgout			<< json::close	
					<< json::close;		
	}
#endif

    return res;
}

void Face::setLogger(FILE * log_file GR_MAYBE_UNUSED)
{
#if !defined GRAPHITE2_NTRACING
    delete m_logger;
    m_logger = log_file ? new json(log_file) : 0;
#endif
}

const Silf *Face::chooseSilf(uint32 script) const
{
    if (m_numSilf == 0)
        return NULL;
    else if (m_numSilf == 1 || script == 0)
        return m_silfs;
    else 
        return m_silfs;
}

uint16 Face::findPseudo(uint32 uid) const
{
    return (m_numSilf) ? m_silfs[0].findPseudo(uid) : 0;
}

uint16 Face::getGlyphMetric(uint16 gid, uint8 metric) const
{
    switch (metrics(metric))
    {
        case kgmetAscent : return m_ascent;
        case kgmetDescent : return m_descent;
        default: return glyphs().glyph(gid)->getMetric(metric);
    }
}

void Face::takeFileFace(FileFace* pFileFace GR_MAYBE_UNUSED)
{
#ifndef GRAPHITE2_NFILEFACE
    if (m_pFileFace==pFileFace)
      return;
    
    delete m_pFileFace;
    m_pFileFace = pFileFace;
#endif
}

NameTable * Face::nameTable() const
{
    if (m_pNames) return m_pNames;
    const Table name(*this, Tag::name);
    if (name)
        m_pNames = new NameTable(name, name.size());
    return m_pNames;
}

uint16 Face::languageForLocale(const char * locale) const
{
    nameTable();
    if (m_pNames)
        return m_pNames->getLanguageId(locale);
    return 0;
}

Face::Table::Table(const Face & face, const Tag n) throw()
: _f(&face)
{
    size_t sz = 0;
    _p = reinterpret_cast<const byte *>((*_f->m_ops.get_table)(_f->m_appFaceHandle, n, &sz));
    _sz = uint32(sz);
    if (!TtfUtil::CheckTable(n, _p, _sz))
    {
        this->~Table();     
        _p = 0; _sz = 0;
    }
}

Face::Table & Face::Table::operator = (const Table & rhs) throw()
{
    if (_p == rhs._p)   return *this;

    this->~Table();
    new (this) Table(rhs);
    return *this;
}

