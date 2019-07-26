

























#pragma once

template<typename T>
inline unsigned int bit_set_count(T v)
{
	v = v - ((v >> 1) & T(~T(0)/3));                           
	v = (v & T(~T(0)/15*3)) + ((v >> 2) & T(~T(0)/15*3));      
	v = (v + (v >> 4)) & T(~T(0)/255*15);                      
	return (T)(v * T(~T(0)/255)) >> (sizeof(T)-1)*8;           
}


template<int S>
inline unsigned long _mask_over_val(unsigned long v)
{
	v = _mask_over_val<S/2>(v);
	v |= v >> S*4;
	return v;
}

template<>
inline unsigned long _mask_over_val<1>(unsigned long v)
{
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	return v;
}

template<typename T>
inline T mask_over_val(T v)
{
	return _mask_over_val<sizeof(T)>(v);
}

template<typename T>
inline unsigned long next_highest_power2(T v)
{
	return _mask_over_val<sizeof(T)>(v-1)+1;
}

template<typename T>
inline unsigned int log_binary(T v)
{
    return bit_set_count(mask_over_val(v))-1;
}

template<typename T>
inline T haszero(const T x)
{
	return (x - T(~T(0)/255)) & ~x & T(~T(0)/255*128);
}

template<typename T>
inline T zerobytes(const T x, unsigned char n)
{
	const T t = T(~T(0)/255*n);
	return T((haszero(x^t) >> 7)*n);
}


