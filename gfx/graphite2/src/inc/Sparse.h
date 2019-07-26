

























#pragma once
#include <iterator>
#include <utility>

#include "inc/Main.h"

namespace graphite2 {



class sparse
{
public:
    typedef uint16  key_type;
    typedef uint16  mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;

private:
	typedef unsigned long	mask_t;

	static const unsigned char  SIZEOF_CHUNK = (sizeof(mask_t) - sizeof(key_type))*8;

	struct chunk
	{
		mask_t			mask:SIZEOF_CHUNK;
		key_type		offset;
	};

	sparse(const sparse &);
	sparse & operator = (const sparse &);

public:
	template<typename I>
	sparse(I first, const I last);
	sparse() throw();
	~sparse() throw();

	operator bool () const throw();
	mapped_type 	operator [] (const key_type k) const throw();

	size_t size()     const throw();

	size_t _sizeof() const throw();
	
	CLASS_NEW_DELETE;

private:
	union {
		chunk         * map;
		mapped_type   * values;
	}           m_array;
	key_type    m_nchunks;
};


inline
sparse::sparse() throw() : m_nchunks(0)
{
	m_array.map = 0;
}


template <typename I>
sparse::sparse(I attr, const I last)
: m_nchunks(0)
{
    m_array.map = 0;

	
	size_t n_values=0;
	for (I i = attr; i != last; ++i, ++n_values)
	{
		const key_type k = (*i).first / SIZEOF_CHUNK;
		if (k >= m_nchunks) m_nchunks = k+1;
	}
	if (m_nchunks == 0)     return;

	m_array.values = grzeroalloc<mapped_type>((m_nchunks*sizeof(chunk) + sizeof(mapped_type)/2)
	                                             / sizeof(mapped_type)
	                                             + n_values*sizeof(mapped_type));

	if (m_array.values == 0)
	{
		free(m_array.values); m_array.map=0;
		return;
	}

	chunk * ci = m_array.map;
	ci->offset = (m_nchunks*sizeof(chunk) + sizeof(mapped_type)-1)/sizeof(mapped_type);
	mapped_type * vi = m_array.values + ci->offset;
	for (; attr != last; ++attr, ++vi)
	{
		const typename std::iterator_traits<I>::value_type v = *attr;
		chunk * const ci_ = m_array.map + v.first/SIZEOF_CHUNK;

		if (ci != ci_)
		{
			ci = ci_;
			ci->offset = vi - m_array.values;
		}

		ci->mask |= 1UL << (SIZEOF_CHUNK - 1 - (v.first % SIZEOF_CHUNK));
		*vi = v.second;
	}
}


inline
sparse::operator bool () const throw()
{
	return m_array.map != 0;
}


inline
size_t sparse::_sizeof() const throw()
{
	return sizeof(sparse) + size()*sizeof(mapped_type) + m_nchunks*sizeof(chunk);
}

} 
