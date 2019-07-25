


























#include "inc/Sparse.h"

using namespace graphite2;

namespace
{
	template<typename T>
	inline unsigned int bit_set_count(T v)
	{
		v = v - ((v >> 1) & T(~T(0)/3));                           
		v = (v & T(~T(0)/15*3)) + ((v >> 2) & T(~T(0)/15*3));      
		v = (v + (v >> 4)) & T(~T(0)/255*15);                      
		return (T)(v * T(~T(0)/255)) >> (sizeof(T)-1)*8;           
	}
}


sparse::~sparse() throw()
{
	free(m_array.values);
}


sparse::value sparse::operator [] (int k) const throw()
{
	bool g = k < m_nchunks*SIZEOF_CHUNK;	
	k *= g;									
	const chunk & 		c = m_array.map[k/SIZEOF_CHUNK];
	const mask_t 		m = c.mask >> (SIZEOF_CHUNK - 1 - (k%SIZEOF_CHUNK));
	g *= m & 1;			

	return g*m_array.values[c.offset + g*bit_set_count(m >> 1)];
}


size_t sparse::size() const throw()
{
	size_t n = m_nchunks,
		   s = 0;

	for (const chunk *ci=m_array.map; n; --n, ++ci)
		s += bit_set_count(ci->mask);

	return s;
}
