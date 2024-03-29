

























#include "graphite2/Font.h"

#include "inc/Main.h"
#include "inc/Face.h"     
#include "inc/GlyphCache.h"
#include "inc/GlyphFace.h"
#include "inc/Endian.h"

using namespace graphite2;

namespace
{
    
    
    
    
    

    template<typename W>
    class _glat_iterator : public std::iterator<std::input_iterator_tag, std::pair<sparse::key_type, sparse::mapped_type> >
    {
        unsigned short  key() const             { return be::peek<W>(_e) + _n; }
        unsigned int    run() const             { return be::peek<W>(_e+sizeof(W)); }
        void            advance_entry()         { _n = 0; _e = _v; be::skip<W>(_v,2); }
    public:
        _glat_iterator(const void * glat=0) : _e(reinterpret_cast<const byte *>(glat)), _v(_e+2*sizeof(W)), _n(0) {}

        _glat_iterator<W> & operator ++ () {
            ++_n; be::skip<uint16>(_v);
            if (_n == run()) advance_entry();
            return *this;
        }
        _glat_iterator<W>   operator ++ (int)   { _glat_iterator<W> tmp(*this); operator++(); return tmp; }

        
        
        
        bool operator == (const _glat_iterator<W> & rhs) { return _v >= rhs._e; }
        bool operator != (const _glat_iterator<W> & rhs) { return !operator==(rhs); }

        value_type          operator * () const {
            return value_type(key(), be::peek<uint16>(_v));
        }

    protected:
        const byte     * _e, * _v;
        ptrdiff_t        _n;
    };

    typedef _glat_iterator<uint8>   glat_iterator;
    typedef _glat_iterator<uint16>  glat2_iterator;
}


class GlyphCache::Loader
{
public:
    Loader(const Face & face, const bool dumb_font);    

    operator bool () const throw();
    unsigned short int units_per_em() const throw();
    unsigned short int num_glyphs() const throw();
    unsigned short int num_attrs() const throw();

    const GlyphFace * read_glyph(unsigned short gid, GlyphFace &) const throw();

    CLASS_NEW_DELETE;
private:
    Face::Table _head,
                _hhea,
                _hmtx,
                _glyf,
                _loca,
                m_pGlat,
                m_pGloc;

    bool            _long_fmt;
    unsigned short  _num_glyphs_graphics,        
                    _num_glyphs_attributes,
                    _num_attrs;                    
};



GlyphCache::GlyphCache(const Face & face, const uint32 face_options)
: _glyph_loader(new Loader(face, bool(face_options & gr_face_dumbRendering))),
  _glyphs(_glyph_loader && *_glyph_loader ? grzeroalloc<const GlyphFace *>(_glyph_loader->num_glyphs()) : 0),
  _num_glyphs(_glyphs ? _glyph_loader->num_glyphs() : 0),
  _num_attrs(_glyphs ? _glyph_loader->num_attrs() : 0),
  _upem(_glyphs ? _glyph_loader->units_per_em() : 0)
{
    if ((face_options & gr_face_preloadGlyphs) && _glyph_loader && _glyphs)
    {
        GlyphFace * const glyphs = new GlyphFace [_num_glyphs];
        if (!glyphs)
            return;

        
        _glyphs[0] = _glyph_loader->read_glyph(0, glyphs[0]);

        
        
        
        const GlyphFace * loaded = _glyphs[0];
        for (uint16 gid = 1; loaded && gid != _num_glyphs; ++gid)
            _glyphs[gid] = loaded = _glyph_loader->read_glyph(gid, glyphs[gid]);

        if (!loaded)
        {
            _glyphs[0] = 0;
            delete [] glyphs;
        }
        delete _glyph_loader;
        _glyph_loader = 0;
    }

    if (_glyphs && glyph(0) == 0)
    {
        free(_glyphs);
        _glyphs = 0;
        _num_glyphs = _num_attrs = _upem = 0;
    }
}


GlyphCache::~GlyphCache()
{
    if (_glyphs)
    {
        if (_glyph_loader)
        {
            const GlyphFace *  * g = _glyphs;
            for(unsigned short n = _num_glyphs; n; --n, ++g)
                delete *g;
        }
        else
            delete [] _glyphs[0];
        free(_glyphs);
    }
    delete _glyph_loader;
}

const GlyphFace *GlyphCache::glyph(unsigned short glyphid) const      
{ 
    const GlyphFace * & p = _glyphs[glyphid];
    if (p == 0 && _glyph_loader)
    {
        GlyphFace * g = new GlyphFace();
        if (g)  p = _glyph_loader->read_glyph(glyphid, *g);
        if (!p)
        {
            delete g;
            return *_glyphs;
        }
    }
    return p;
}



GlyphCache::Loader::Loader(const Face & face, const bool dumb_font)
: _head(face, Tag::head),
  _hhea(face, Tag::hhea),
  _hmtx(face, Tag::hmtx),
  _glyf(face, Tag::glyf),
  _loca(face, Tag::loca),
  _long_fmt(false),
  _num_glyphs_graphics(0),
  _num_glyphs_attributes(0),
  _num_attrs(0)
{
    if (!operator bool())
        return;

    const Face::Table maxp = Face::Table(face, Tag::maxp);
    if (!maxp) { _head = Face::Table(); return; }

    _num_glyphs_graphics = TtfUtil::GlyphCount(maxp);
    
    if (_glyf && TtfUtil::LocaLookup(_num_glyphs_graphics-1, _loca, _loca.size(), _head) == size_t(-1))
    {
        _head = Face::Table();
        return;
    }

    if (!dumb_font)
    {
        if ((m_pGlat = Face::Table(face, Tag::Glat)) == NULL
            || (m_pGloc = Face::Table(face, Tag::Gloc)) == NULL
            || m_pGloc.size() < 6)
        {
            _head = Face::Table();
            return;
        }
        const byte    * p = m_pGloc;
        const int       version = be::read<uint32>(p);
        const uint16    flags = be::read<uint16>(p);
        _num_attrs = be::read<uint16>(p);
        
        
        
        _long_fmt              = flags & 1;
        _num_glyphs_attributes = (m_pGloc.size()
                                   - (p - m_pGloc)
                                   - sizeof(uint16)*(flags & 0x2 ? _num_attrs : 0))
                                       / (_long_fmt ? sizeof(uint32) : sizeof(uint16)) - 1;

        if (version != 0x00010000
            || _num_attrs == 0 || _num_attrs > 0x3000  
            || _num_glyphs_graphics > _num_glyphs_attributes)
        {
            _head = Face::Table();
            return;
        }
    }
}

inline
GlyphCache::Loader::operator bool () const throw()
{
    return _head && _hhea && _hmtx && !(bool(_glyf) != bool(_loca));
}

inline
unsigned short int GlyphCache::Loader::units_per_em() const throw()
{
    return _head ? TtfUtil::DesignUnits(_head) : 0;
}

inline
unsigned short int GlyphCache::Loader::num_glyphs() const throw()
{
    return max(_num_glyphs_graphics, _num_glyphs_attributes);
}

inline
unsigned short int GlyphCache::Loader::num_attrs() const throw()
{
    return _num_attrs;
}

const GlyphFace * GlyphCache::Loader::read_glyph(unsigned short glyphid, GlyphFace & glyph) const throw()
{
    Rect        bbox;
    Position    advance;

    if (glyphid < _num_glyphs_graphics)
    {
        int nLsb;
        unsigned int nAdvWid;
        if (_glyf)
        {
            int xMin, yMin, xMax, yMax;
            size_t locidx = TtfUtil::LocaLookup(glyphid, _loca, _loca.size(), _head);
            void *pGlyph = TtfUtil::GlyfLookup(_glyf, locidx, _glyf.size());

            if (pGlyph && TtfUtil::GlyfBox(pGlyph, xMin, yMin, xMax, yMax))
                bbox = Rect(Position(static_cast<float>(xMin), static_cast<float>(yMin)),
                    Position(static_cast<float>(xMax), static_cast<float>(yMax)));
        }
        if (TtfUtil::HorMetrics(glyphid, _hmtx, _hmtx.size(), _hhea, nLsb, nAdvWid))
            advance = Position(static_cast<float>(nAdvWid), 0);
    }

    if (glyphid < _num_glyphs_attributes)
    {
        const byte * gloc = m_pGloc;
        size_t      glocs = 0, gloce = 0;

        be::skip<uint32>(gloc);
        be::skip<uint16>(gloc,2);
        if (_long_fmt)
        {
            be::skip<uint32>(gloc, glyphid);
            glocs = be::read<uint32>(gloc);
            gloce = be::peek<uint32>(gloc);
        }
        else
        {
            be::skip<uint16>(gloc, glyphid);
            glocs = be::read<uint16>(gloc);
            gloce = be::peek<uint16>(gloc);
        }

        if (glocs >= m_pGlat.size() || gloce > m_pGlat.size())
            return 0;

        const uint32 glat_version = be::peek<uint32>(m_pGlat);
        if (glat_version < 0x00020000)
        {
            if (gloce - glocs < 2*sizeof(byte)+sizeof(uint16)
                || gloce - glocs > _num_attrs*(2*sizeof(byte)+sizeof(uint16)))
            {
                return 0;
            }

            new (&glyph) GlyphFace(bbox, advance, glat_iterator(m_pGlat + glocs), glat_iterator(m_pGlat + gloce));
        }
        else
        {
            if (gloce - glocs < 3*sizeof(uint16)
                || gloce - glocs > _num_attrs*3*sizeof(uint16))
            {
                return 0;
            }

            new (&glyph) GlyphFace(bbox, advance, glat2_iterator(m_pGlat + glocs), glat2_iterator(m_pGlat + gloce));
        }

        if (!glyph.attrs() || glyph.attrs().capacity() > _num_attrs)
            return 0;
    }

    return &glyph;
}
