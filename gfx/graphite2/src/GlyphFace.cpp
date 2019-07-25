

























#include <iterator>
#include "GlyphFace.h"
#include "XmlTraceLog.h"
#include "GlyphFaceCache.h"
#include "TtfUtil.h"
#include "Endian.h"


using namespace graphite2;

namespace
{
	struct glyph_attr { uint16 id, value; };

	class glat_iterator : public std::iterator<std::input_iterator_tag, glyph_attr>
	{
	public:
		glat_iterator(const void * glat=0) : _p(reinterpret_cast<const byte *>(glat)), _n(0) {}

		glat_iterator & operator ++ () 		{ ++_v.id; --_n; _p += sizeof(uint16); if (_n == -1) { _p -= 2; _v.id = *_p++; _n = *_p++; } return *this; }
		glat_iterator   operator ++ (int) 	{ glat_iterator tmp(*this); operator++(); return tmp; }

		bool operator == (const glat_iterator & rhs) { return _p >= rhs._p || _p + _n*sizeof(uint16) > rhs._p; }
		bool operator != (const glat_iterator & rhs) { return !operator==(rhs); }

		value_type 			operator * () const {
			if (_n==0) { _v.id = *_p++; _n = *_p++; }
			_v.value = be::peek<uint16>(_p);
			return _v;
		}
		const value_type *	operator ->() const { operator * (); return &_v; }

	protected:
		mutable const byte * _p;
		mutable value_type  _v;
		mutable int 		_n;
	};

	class glat2_iterator : public glat_iterator
	{
	public:
		glat2_iterator(const void * glat) : glat_iterator(glat) {}

		glat_iterator & operator ++ () 		{ ++_v.id; --_n; _p += sizeof(uint16); if (_n == -1) { _p -= sizeof(uint16)*2; _v.id = be::read<uint16>(_p); _n = be::read<uint16>(_p); } return *this; }
		glat_iterator   operator ++ (int) 	{ glat_iterator tmp(*this); operator++(); return tmp; }

		value_type 			operator * () const {
			if (_n==0) { _v.id = be::read<uint16>(_p); _n = be::read<uint16>(_p); }
			_v.value = be::peek<uint16>(_p);
			return _v;
		}
		const value_type *	operator ->() const { operator * (); return &_v; }
	};
}

GlyphFace::GlyphFace(const GlyphFaceCacheHeader& hdr, unsigned short glyphid)
{
    if (glyphid < hdr.m_nGlyphsWithGraphics)
    {
        int nLsb, xMin, yMin, xMax, yMax;
        unsigned int nAdvWid;
        size_t locidx = TtfUtil::LocaLookup(glyphid, hdr.m_pLoca, hdr.m_lLoca, hdr.m_pHead);
        void *pGlyph = TtfUtil::GlyfLookup(hdr.m_pGlyf, locidx, hdr.m_lGlyf);
        if (TtfUtil::HorMetrics(glyphid, hdr.m_pHmtx, hdr.m_lHmtx, hdr.m_pHHea, nLsb, nAdvWid))
            m_advance = Position(static_cast<float>(nAdvWid), 0);
        else
            m_advance = Position();
        if (pGlyph && TtfUtil::GlyfBox(pGlyph, xMin, yMin, xMax, yMax))
            m_bbox = Rect(Position(static_cast<float>(xMin), static_cast<float>(yMin)),

                Position(static_cast<float>(xMax), static_cast<float>(yMax)));
        else
            m_bbox = Rect();
    }
    else
    {
        m_advance = Position();
        m_bbox = Rect();
    }
#ifndef DISABLE_TRACING
    if (XmlTraceLog::get().active())
    {
        XmlTraceLog::get().openElement(ElementGlyphFace);
        XmlTraceLog::get().addAttribute(AttrGlyphId, glyphid);
        XmlTraceLog::get().addAttribute(AttrAdvanceX, m_advance.x);
        XmlTraceLog::get().addAttribute(AttrAdvanceY, m_advance.y);
    }
#endif
    if (glyphid < hdr.m_nGlyphsWithAttributes)
    {
        size_t glocs, gloce;
        if (hdr.m_locFlagsUse32Bit)
        {
            glocs = be::swap<uint32>(((uint32 *)hdr.m_pGloc)[2+glyphid]);
            gloce = be::swap<uint32>(((uint32 *)hdr.m_pGloc)[3+glyphid]);
        }
        else
        {
            glocs = be::swap<uint16>(((uint16 *)hdr.m_pGloc)[4+glyphid]);
            gloce = be::swap<uint16>(((uint16 *)hdr.m_pGloc)[5+glyphid]);
        }
        if (glocs < hdr.m_lGlat && gloce <= hdr.m_lGlat)
        {


        	if (hdr.m_fGlat < 0x00020000)
        	{
        		if (gloce - glocs < 2*sizeof(byte)+sizeof(uint16)
        		 || gloce - glocs > hdr.m_numAttrs*(2*sizeof(byte)+sizeof(uint16)))
        			return;

        		new (&m_attrs) sparse(glat_iterator(hdr.m_pGlat + glocs), glat_iterator(hdr.m_pGlat + gloce));
        	}
        	else
        	{
        		if (gloce - glocs < 3*sizeof(uint16)
        		 || gloce - glocs > hdr.m_numAttrs*3*sizeof(uint16))
        			return;

        		new (&m_attrs) sparse(glat2_iterator(hdr.m_pGlat + glocs), glat2_iterator(hdr.m_pGlat + gloce));
        	}

        	if (m_attrs.size() > hdr.m_numAttrs)
        	{
        		m_attrs.~sparse();
        		new (&m_attrs) sparse();
        	}
        }
    }
#ifndef DISABLE_TRACING
    XmlTraceLog::get().closeElement(ElementGlyphFace);
#endif
}
































































inline
void GlyphFace::logAttr(GR_MAYBE_UNUSED const uint16 attrs[], GR_MAYBE_UNUSED const uint16 * attr)
{
#ifndef DISABLE_TRACING
	if (XmlTraceLog::get().active())
	{
		XmlTraceLog::get().openElement(ElementAttr);
		XmlTraceLog::get().addAttribute(AttrAttrId, attr - attrs);
		XmlTraceLog::get().addAttribute(AttrAttrVal, *attr);
		XmlTraceLog::get().closeElement(ElementAttr);
	}
#endif
}


uint16 GlyphFace::getMetric(uint8 metric) const
{
    switch (metrics(metric))
    {
        case kgmetLsb : return static_cast<uint16>(m_bbox.bl.x);
        case kgmetRsb : return static_cast<uint16>(m_advance.x - m_bbox.tr.x);
        case kgmetBbTop : return static_cast<uint16>(m_bbox.tr.y);
        case kgmetBbBottom : return static_cast<uint16>(m_bbox.bl.y);
        case kgmetBbLeft : return static_cast<uint16>(m_bbox.bl.x);
        case kgmetBbRight : return static_cast<uint16>(m_bbox.tr.x);
        case kgmetBbHeight: return static_cast<uint16>(m_bbox.tr.y - m_bbox.bl.y);
        case kgmetBbWidth : return static_cast<uint16>(m_bbox.tr.x - m_bbox.bl.x);
        case kgmetAdvWidth : return static_cast<uint16>(m_advance.x);
        case kgmetAdvHeight : return static_cast<uint16>(m_advance.y);
        default : return 0;
    }
}
