


























#include "inc/Main.h"
#include "inc/CmapCache.h"
#include "inc/TtfTypes.h"
#include "inc/TtfUtil.h"


using namespace graphite2;

CmapCache::CmapCache(const void* cmapTable, size_t length)
{
    const void * table31 = TtfUtil::FindCmapSubtable(cmapTable, 3, 1, length);
    const void * table310 = TtfUtil::FindCmapSubtable(cmapTable, 3, 10, length);
    m_isBmpOnly = (!table310);
    int rangeKey = 0;
    uint32 	codePoint = 0,
    		prevCodePoint = 0;
    if (table310 && TtfUtil::CheckCmap310Subtable(table310))
    {
        m_blocks = grzeroalloc<uint16*>(0x1100);
        if (!m_blocks) return;
        codePoint =  TtfUtil::Cmap310NextCodepoint(table310, codePoint, &rangeKey);
        while (codePoint != 0x10FFFF)
        {
            unsigned int block = (codePoint & 0xFFFF00) >> 8;
            if (!m_blocks[block])
            {
                m_blocks[block] = grzeroalloc<uint16>(0x100);
                if (!m_blocks[block])
                    return;
            }
            m_blocks[block][codePoint & 0xFF] = TtfUtil::Cmap310Lookup(table310, codePoint, rangeKey);
            
            if (codePoint <= prevCodePoint)
                codePoint = prevCodePoint + 1;
            prevCodePoint = codePoint;
            codePoint =  TtfUtil::Cmap310NextCodepoint(table310, codePoint, &rangeKey);
        }
    }
    else
    {
        m_blocks = grzeroalloc<uint16*>(0x100);
        if (!m_blocks) return;
    }
    if (table31 && TtfUtil::CheckCmap31Subtable(table31))
    {
        codePoint = 0;
        rangeKey = 0;
        codePoint =  TtfUtil::Cmap31NextCodepoint(table31, codePoint, &rangeKey);
        while (codePoint != 0xFFFF)
        {
            unsigned int block = (codePoint & 0xFFFF00) >> 8;
            if (!m_blocks[block])
            {
                m_blocks[block] = grzeroalloc<uint16>(0x100);
                if (!m_blocks[block])
                    return;
            }
            m_blocks[block][codePoint & 0xFF] = TtfUtil::Cmap31Lookup(table31, codePoint, rangeKey);
            
            if (codePoint <= prevCodePoint)
                codePoint = prevCodePoint + 1;
            prevCodePoint = codePoint;
            codePoint =  TtfUtil::Cmap31NextCodepoint(table31, codePoint, &rangeKey);
        }
    }
}

CmapCache::~CmapCache() throw()
{
    unsigned int numBlocks = (m_isBmpOnly)? 0x100 : 0x1100;
    for (unsigned int i = 0; i < numBlocks; i++)
    	free(m_blocks[i]);
    free(m_blocks);
}

uint16 CmapCache::operator [] (const uint32 usv) const throw()
{
    if ((m_isBmpOnly && usv > 0xFFFF) || (usv > 0x10FFFF))
        return 0;
    const uint32 block = 0xFFFF & (usv >> 8);
    if (m_blocks[block])
        return m_blocks[block][usv & 0xFF];
    return 0;
};

CmapCache::operator bool() const throw()
{
	return m_blocks;
}


DirectCmap::DirectCmap(const void* cmap, size_t length)
{
    _ctable = TtfUtil::FindCmapSubtable(cmap, 3, 1, length);
    if (!_ctable || !TtfUtil::CheckCmap31Subtable(_ctable))
    {
        _ctable =  0;
        return;
    }
    _stable = TtfUtil::FindCmapSubtable(cmap, 3, 10, length);
    if (_stable && !TtfUtil::CheckCmap310Subtable(_stable))
    	_stable = 0;
}

uint16 DirectCmap::operator [] (const uint32 usv) const throw()
{
    return usv > 0xFFFF ? (_stable ? TtfUtil::Cmap310Lookup(_stable, usv) : 0) : TtfUtil::Cmap31Lookup(_ctable, usv);
}

DirectCmap::operator bool () const throw()
{
	return _ctable;
}

