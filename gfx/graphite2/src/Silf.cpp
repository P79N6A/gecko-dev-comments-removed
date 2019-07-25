

























#include <cstdlib>
#include "graphite2/Segment.h"
#include "inc/debug.h"
#include "inc/Endian.h"
#include "inc/Silf.h"
#include "inc/Segment.h"
#include "inc/Rule.h"


using namespace graphite2;


Silf::Silf() throw()
: m_passes(0), m_pseudos(0), m_classOffsets(0), m_classData(0), m_justs(0),
  m_numPasses(0), m_sPass(0), m_pPass(0), m_jPass(0), m_bPass(0), m_flags(0),
  m_aBreak(0), m_aUser(0), m_iMaxComp(0),
  m_aLig(0), m_numPseudo(0), m_nClass(0), m_nLinear(0)
{
}

Silf::~Silf() throw()
{
    releaseBuffers();
}

void Silf::releaseBuffers() throw()
{
    delete [] m_passes;
    delete [] m_pseudos;
    free(m_classOffsets);
    free(m_classData);
    free(m_justs);
    m_passes= 0;
    m_pseudos = 0;
    m_classOffsets = 0;
    m_classData = 0;
    m_justs = 0;
}


bool Silf::readGraphite(const void* pSilf, size_t lSilf, const Face& face, uint32 version)
{
    const byte * p = (byte *)pSilf,
    		   * const eSilf = p + lSilf;

    if (version >= 0x00030000)
    {
        if (lSilf < 27)		{ releaseBuffers(); return false; }
        p += 8;
    }
    else if (lSilf < 19) 	{ releaseBuffers(); return false; }
    p += 2;     
    p += 4;     
    m_numPasses = uint8(*p++);
    if (m_numPasses > 128)
        return false;
    m_passes = new Pass[m_numPasses];
    m_sPass = uint8(*p++);
    m_pPass = uint8(*p++);
    if (m_pPass < m_sPass) {
        releaseBuffers();
        return false;
    }
    m_jPass = uint8(*p++);
    if (m_jPass < m_pPass) {
        releaseBuffers();
        return false;
    }
    m_bPass = uint8(*p++);     
    if (m_bPass != 0xFF && (m_bPass < m_jPass || m_bPass > m_numPasses)) {
        releaseBuffers();
        return false;
    }
    m_flags = uint8(*p++);
    p += 2;     
    m_aPseudo = uint8(*p++);
    m_aBreak = uint8(*p++);
    m_aBidi = uint8(*p++);
    m_aMirror = uint8(*p++);
    p += 1;     
    m_numJusts = uint8(*p++);
    m_justs = gralloc<Justinfo>(m_numJusts);
    for (uint8 i = 0; i < m_numJusts; i++)
    {
        ::new(m_justs + i) Justinfo(p[0], p[1], p[2], p[3]);
        p += 8;
    }

    if (p + 9 >= eSilf) { releaseBuffers(); return false; }
    m_aLig = be::read<uint16>(p);
    if (m_aLig > 127) {
        releaseBuffers();
        return false;
    }
    m_aUser = uint8(*p++);
    m_iMaxComp = uint8(*p++);
    p += 5;     
    p += uint8(*p) * 2 + 1;        
    p++;        
    if (p >= eSilf) 
    {
        releaseBuffers();
        return false;
    }
    p += uint8(*p) * 4 + 1;        
    p += 2;     
    
    if (p + 4 * (m_numPasses + 1) + 6 >= eSilf) 
    {
        releaseBuffers(); 
        return false;
    }
    const byte * pPasses = p;
    p += 4 * (m_numPasses + 1);
    m_numPseudo = be::read<uint16>(p);
    p += 6;
    if (p + m_numPseudo * 6 >= eSilf) 
    {
        releaseBuffers();
        return false;
    }
    m_pseudos = new Pseudo[m_numPseudo];
    for (int i = 0; i < m_numPseudo; i++)
    {
        m_pseudos[i].uid = be::read<uint32>(p);
        m_pseudos[i].gid = be::read<uint16>(p);
    }
    if (p >= eSilf) 
    {
        releaseBuffers();
        return false;
    }

    int clen = readClassMap(p, be::peek<uint32>(pPasses) - (p - (byte *)pSilf), version);
    if (clen < 0) {
        releaseBuffers();
        return false;
    }
    p += clen;

    for (size_t i = 0; i < m_numPasses; ++i)
    {
        uint32 pOffset = be::read<uint32>(pPasses);
        uint32 pEnd = be::peek<uint32>(pPasses);
        if ((uint8 *)pSilf + pEnd > eSilf || pOffset > pEnd)
        {
            releaseBuffers();
            return false;
        }
        m_passes[i].init(this);
        if (!m_passes[i].readPass((char *)pSilf + pOffset, pEnd - pOffset, pOffset, face))
        {
        	releaseBuffers();
        	return false;
        }
    }
    return true;
}

template<typename T> inline uint32 Silf::readClassOffsets(const byte *&p, size_t data_len)
{
	const T cls_off = 2*sizeof(uint16) + sizeof(T)*(m_nClass+1);
	const uint32 max_off = (be::peek<T>(p + sizeof(T)*m_nClass) - cls_off)/sizeof(uint16);
	
	if (be::peek<T>(p) != cls_off || max_off > (data_len - cls_off)/sizeof(uint16))
		return 0;

	
	m_classOffsets = gralloc<uint32>(m_nClass+1);
	for (uint32 * o = m_classOffsets, * const o_end = o + m_nClass + 1; o != o_end; ++o)
	{
		*o = (be::read<T>(p) - cls_off)/sizeof(uint16);
		if (*o > max_off)
			return 0;
	}
    return max_off;
}

size_t Silf::readClassMap(const byte *p, size_t data_len, uint32 version)
{
	if (data_len < sizeof(uint16)*2)	return -1;

	m_nClass  = be::read<uint16>(p);
	m_nLinear = be::read<uint16>(p);

	
	
	if (m_nLinear > m_nClass
	 || (m_nClass + 1) * (version >= 0x00040000 ? sizeof(uint32) : sizeof(uint16))> (data_len - 4))
		return -1;

    
    uint32 max_off;
    if (version >= 0x00040000)
        max_off = readClassOffsets<uint32>(p, data_len);
    else
        max_off = readClassOffsets<uint16>(p, data_len);

    if (max_off == 0) return -1;

	
	for (const uint32 *o = m_classOffsets, * const o_end = o + m_nLinear; o != o_end; ++o)
		if (o[0] > o[1])
			return -1;

	
    m_classData = gralloc<uint16>(max_off);
    for (uint16 *d = m_classData, * const d_end = d + max_off; d != d_end; ++d)
        *d = be::read<uint16>(p);

	
	for (const uint32 *o = m_classOffsets + m_nLinear, * const o_end = m_classOffsets + m_nClass; o != o_end; ++o)
	{
		const uint16 * lookup = m_classData + *o;
		if (lookup[0] == 0							
		 || lookup[0] > (max_off - *o - 4)/2  	    
		 || lookup[3] != lookup[0] - lookup[1])		
			return -1;
	}

	return max_off;
}

uint16 Silf::findPseudo(uint32 uid) const
{
    for (int i = 0; i < m_numPseudo; i++)
        if (m_pseudos[i].uid == uid) return m_pseudos[i].gid;
    return 0;
}

uint16 Silf::findClassIndex(uint16 cid, uint16 gid) const
{
    if (cid > m_nClass) return -1;

    const uint16 * cls = m_classData + m_classOffsets[cid];
    if (cid < m_nLinear)        
    {
        for (unsigned int i = 0, n = m_classOffsets[cid + 1]; i < n; ++i, ++cls)
            if (*cls == gid) return i;
        return -1;
    }
    else
    {
    	const uint16 *	min = cls + 4,		
    				 * 	max = min + cls[0]*2; 
    	do
        {
        	const uint16 * p = min + (-2U & ((max-min)/2));
        	if 	(p[0] > gid)	max = p;
        	else 				min = p;
        }
        while (max - min > 2);
        return min[0] == gid ? min[1] : -1;
    }
}

uint16 Silf::getClassGlyph(uint16 cid, unsigned int index) const
{
    if (cid > m_nClass) return 0;

    uint32 loc = m_classOffsets[cid];
    if (cid < m_nLinear)
    {
        if (index < m_classOffsets[cid + 1] - loc)
            return m_classData[index + loc];
    }
    else        
    {
        for (unsigned int i = loc + 4; i < m_classOffsets[cid + 1]; i += 2)
            if (m_classData[i + 1] == index) return m_classData[i];
    }
    return 0;
}


bool Silf::runGraphite(Segment *seg, uint8 firstPass, uint8 lastPass) const
{
    assert(seg != 0);
    SlotMap            map(*seg);
    FiniteStateMachine fsm(map);
    vm::Machine        m(map);
    unsigned int       initSize = seg->slotCount();

    if (lastPass == 0)
    {
        if (firstPass == lastPass)
            return true;
        lastPass = m_numPasses;
    }

#if !defined GRAPHITE2_NTRACING
    if (dbgout)
    {
    	char version[64];
    	sprintf(version, "%d.%d.%d",
    			GR2_VERSION_MAJOR, GR2_VERSION_MINOR, GR2_VERSION_BUGFIX);
    	*dbgout << json::object
    				<< "version"	<< version
    				<< "passes"		<< json::array;
    }
#endif

    for (size_t i = firstPass; i < lastPass; ++i)
    {
    	
        if (i == m_bPass)
        {
#if !defined GRAPHITE2_NTRACING
        	if (dbgout)
        	{
        		*dbgout << json::item << json::object
        					<< "id"		<< -1
        					<< "slots"	<< json::array;
        		seg->positionSlots(0);
        		for(Slot * s = seg->first(); s; s = s->next())
        			*dbgout		<< dslot(seg, s);
        		*dbgout			<< json::close
        					<< "rules"	<< json::array << json::close
        					<< json::close;
        	}
#endif

        	if (!(seg->dir() & 2))
            	seg->bidiPass(m_aBidi, seg->dir() & 1, m_aMirror);
        	else if (m_aMirror)
            {
                Slot * s;
                for (s = seg->first(); s; s = s->next())
                {
                    unsigned short g = seg->glyphAttr(s->gid(), m_aMirror);
                    if (g && (!(seg->dir() & 4) || !seg->glyphAttr(s->gid(), m_aMirror + 1)))
                        s->setGlyph(seg, g);
                }
            }
        }

#if !defined GRAPHITE2_NTRACING
    	if (dbgout)
    	{
    		*dbgout << json::item << json::object
    					<< "id"		<< i+1
    					<< "slots"	<< json::array;
    		seg->positionSlots(0);
    		for(Slot * s = seg->first(); s; s = s->next())
    			*dbgout		<< dslot(seg, s);
    		*dbgout			<< json::close;
    	}
#endif

        
        m_passes[i].runGraphite(m, fsm);
        
        if (m.status() != vm::Machine::finished
        	|| (i < m_pPass && (seg->slotCount() > initSize * MAX_SEG_GROWTH_FACTOR
            || (seg->slotCount() && seg->slotCount() * MAX_SEG_GROWTH_FACTOR < initSize))))
            return false;
    }
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
    return true;
}
