

























#include <cassert>
#include "inc/Sparse.h"
#include "inc/bits.h"

using namespace graphite2;


sparse::~sparse() throw()
{
	free(m_array.values);
}


sparse::mapped_type sparse::operator [] (const key_type k) const throw()
{
	const chunk & 		c = m_array.map[k/SIZEOF_CHUNK];
	const mask_t 		m = c.mask >> (SIZEOF_CHUNK - 1 - (k%SIZEOF_CHUNK));
	const mapped_type   g = m & 1;

	return g*m_array.values[g*(c.offset + bit_set_count(m >> 1))];
}


size_t sparse::size() const throw()
{
	size_t n = m_nchunks,
		   s = 0;

	for (const chunk *ci=m_array.map; n; --n, ++ci)
		s += bit_set_count(ci->mask);

	return s;
}
