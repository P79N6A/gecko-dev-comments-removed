







		  







#ifndef BOOST_SPREAD_SORT_H
#define BOOST_SPREAD_SORT_H
#include <algorithm>
#include <cstring>
#include <vector>
#include "webrtc/system_wrappers/source/spreadsortlib/constants.hpp"

#ifdef getchar
#undef getchar
#endif

namespace boost {
  namespace detail {
  	
  	template <typename T>
  	inline unsigned 
  	rough_log_2_size(const T& input) 
  	{
  		unsigned result = 0;
  		
  		while((input >> result) && (result < (8*sizeof(T)))) ++result;
  		return result;
  	}

  	
  	
  	
  	inline size_t
  	get_max_count(unsigned log_range, size_t count)
  	{
  		unsigned divisor = rough_log_2_size(count);
  		
  		if(divisor > LOG_MEAN_BIN_SIZE)
  			divisor -= LOG_MEAN_BIN_SIZE;
  		else
  			divisor = 1;
  		unsigned relative_width = (LOG_CONST * log_range)/((divisor > MAX_SPLITS) ? MAX_SPLITS : divisor);
  		
  		if((8*sizeof(size_t)) <= relative_width)
  			relative_width = (8*sizeof(size_t)) - 1;
  		return (size_t)1 << ((relative_width < (LOG_MEAN_BIN_SIZE + LOG_MIN_SPLIT_COUNT)) ? 
  			(LOG_MEAN_BIN_SIZE + LOG_MIN_SPLIT_COUNT) :  relative_width);
  	}

  	
  	template <class RandomAccessIter>
  	inline void 
  	find_extremes(RandomAccessIter current, RandomAccessIter last, RandomAccessIter & max, RandomAccessIter & min)
  	{
  		min = max = current;
  		
  		while(++current < last) {
  			if(*max < *current)
  				max = current;
  			else if(*current < *min)
  				min = current;
  		}
  	}

  	
  	template <class RandomAccessIter, class compare>
  	inline void 
  	find_extremes(RandomAccessIter current, RandomAccessIter last, RandomAccessIter & max, RandomAccessIter & min, compare comp)
  	{
  		min = max = current;
  		while(++current < last) {
  			if(comp(*max, *current))
  				max = current;
  			else if(comp(*current, *min))
  				min = current;
  		}
  	}

  	
  	inline int
  	get_log_divisor(size_t count, unsigned log_range)
  	{
  		int log_divisor;
  		
  		if((log_divisor = log_range - rough_log_2_size(count)) <= 0 && log_range < MAX_SPLITS)
  			log_divisor = 0;
  		else {
  			
  			log_divisor += LOG_MEAN_BIN_SIZE;
  			if(log_divisor < 0)
  				log_divisor = 0;
  			
  			if((log_range - log_divisor) > MAX_SPLITS)
  				log_divisor = log_range - MAX_SPLITS;
  		}
  		return log_divisor;
  	}

  	template <class RandomAccessIter>
  	inline RandomAccessIter * 
  	size_bins(std::vector<size_t> &bin_sizes, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset, unsigned &cache_end, unsigned bin_count)
  	{
  		
  		if(bin_count > bin_sizes.size())
  			bin_sizes.resize(bin_count);
  		for(size_t u = 0; u < bin_count; u++)
  			bin_sizes[u] = 0;
  		
  		cache_end = cache_offset + bin_count;
  		if(cache_end > bin_cache.size())
  			bin_cache.resize(cache_end);
  		return &(bin_cache[cache_offset]);
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void 
  	spread_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  				  , std::vector<size_t> &bin_sizes)
  	{
  		
  		
  		RandomAccessIter max, min;
  		find_extremes(first, last, max, min);
  		
  		if(max == min)
  			return;
  		RandomAccessIter * target_bin;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(*max >> 0) - (*min >> 0)));
  		div_type div_min = *min >> log_divisor;
  		div_type div_max = *max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  	
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[(*(current++) >> log_divisor) - div_min]++;
  		
  		bins[0] = first;
  		for(unsigned u = 0; u < bin_count - 1; u++)
  			bins[u + 1] = bins[u] + bin_sizes[u];
  
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count - 1; ++u) {
  			RandomAccessIter * local_bin = bins + u;
  			nextbinstart += bin_sizes[u];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = (bins + ((*current >> log_divisor) - div_min));  target_bin != local_bin; 
  					target_bin = bins + ((*current >> log_divisor) - div_min)) {
  					
  					
  					data_type tmp;
  					RandomAccessIter b = (*target_bin)++;
  					RandomAccessIter * b_bin = bins + ((*b >> log_divisor) - div_min);
  					if (b_bin != local_bin) {
  						RandomAccessIter c = (*b_bin)++;
  						tmp = *c;
  						*c = *b;
  					} 
  					else
  						tmp = *b;
  					*b = *current;
  					*current = tmp;
  				}
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[bin_count - 1] = last;
  
  		
  		if(!log_divisor)
  			return;
  
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			else
  				spread_sort_rec<RandomAccessIter, div_type, data_type>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void inner_swap_loop(RandomAccessIter * bins, const RandomAccessIter & nextbinstart, unsigned ii, right_shift &shift
  		, const unsigned log_divisor, const div_type div_min) 
  	{
  		RandomAccessIter * local_bin = bins + ii;
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			for(RandomAccessIter * target_bin = (bins + (shift(*current, log_divisor) - div_min));  target_bin != local_bin; 
  				target_bin = bins + (shift(*current, log_divisor) - div_min)) {
  				data_type tmp;
  				RandomAccessIter b = (*target_bin)++;
  				RandomAccessIter * b_bin = bins + (shift(*b, log_divisor) - div_min);
  				
  				if (b_bin != local_bin) {
  					RandomAccessIter c = (*b_bin)++;
  					tmp = *c;
  					*c = *b;
  				} 
  				
  				else
  					tmp = *b;
  				*b = *current;
  				*current = tmp;
  			}
  		}
  		*local_bin = nextbinstart;
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void swap_loop(RandomAccessIter * bins, RandomAccessIter & nextbinstart, unsigned ii, right_shift &shift
  		, const std::vector<size_t> &bin_sizes, const unsigned log_divisor, const div_type div_min) 
  	{
  		nextbinstart += bin_sizes[ii];
  		inner_swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, ii, shift, log_divisor, div_min);
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift, class compare>
  	inline void 
  	spread_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift, compare comp)
  	{
  		RandomAccessIter max, min;
  		find_extremes(first, last, max, min, comp);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(shift(*max, 0)) - (shift(*min, 0))));
  		div_type div_min = shift(*min, log_divisor);
  		div_type div_max = shift(*max, log_divisor);
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		bins[0] = first;
  		for(unsigned u = 0; u < bin_count - 1; u++)
  			bins[u + 1] = bins[u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count - 1; ++u)
  			swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, u, shift, bin_sizes, log_divisor, div_min);
  		bins[bin_count - 1] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u], comp);
  			else
  				spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift, compare>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, shift, comp);
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void 
  	spread_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift)
  	{
  		RandomAccessIter max, min;
  		find_extremes(first, last, max, min);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(shift(*max, 0)) - (shift(*min, 0))));
  		div_type div_min = shift(*min, log_divisor);
  		div_type div_max = shift(*max, log_divisor);
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		bins[0] = first;
  		for(unsigned u = 0; u < bin_count - 1; u++)
  			bins[u + 1] = bins[u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned ii = 0; ii < bin_count - 1; ++ii)
  			swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, ii, shift, bin_sizes, log_divisor, div_min);
  		bins[bin_count - 1] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			else
  				spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, shift);
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void 
  	spread_sort(RandomAccessIter first, RandomAccessIter last, div_type, data_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		spread_sort_rec<RandomAccessIter, div_type, data_type>(first, last, bin_cache, 0, bin_sizes);
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift, class compare>
  	inline void 
  	spread_sort(RandomAccessIter first, RandomAccessIter last, div_type, data_type, right_shift shift, compare comp)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift, compare>(first, last, bin_cache, 0, bin_sizes, shift, comp);
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void 
  	spread_sort(RandomAccessIter first, RandomAccessIter last, div_type, data_type, right_shift shift)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(first, last, bin_cache, 0, bin_sizes, shift);
  	}
  }

  
  template <class RandomAccessIter>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else
  		detail::spread_sort(first, last, *first >> 0, *first);
  }

  
  template <class RandomAccessIter, class right_shift, class compare>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last,
  						right_shift shift, compare comp) {
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last, comp);
  	else
  		detail::spread_sort(first, last, shift(*first, 0), *first, shift, comp);
  }

  
  template <class RandomAccessIter, class right_shift>
  inline void integer_sort(RandomAccessIter first, RandomAccessIter last,
  						right_shift shift) {
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else
  		detail::spread_sort(first, last, shift(*first, 0), *first, shift);
  }

  
  
  template<class cast_type, class RandomAccessIter>
  inline cast_type
  cast_float_iter(const RandomAccessIter & floatiter)
  {
  	cast_type result;
  	std::memcpy(&result, &(*floatiter), sizeof(cast_type));
  	return result;
  }

  
  template<class data_type, class cast_type>
  inline cast_type
  mem_cast(const data_type & data)
  {
  	cast_type result;
  	std::memcpy(&result, &data, sizeof(cast_type));
  	return result;
  }

  namespace detail {
  	template <class RandomAccessIter, class div_type, class right_shift>
  	inline void 
  	find_extremes(RandomAccessIter current, RandomAccessIter last, div_type & max, div_type & min, right_shift shift)
  	{
  		min = max = shift(*current, 0);
  		while(++current < last) {
  			div_type value = shift(*current, 0);
  			if(max < value)
  				max = value;
  			else if(value < min)
  				min = value;
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void inner_float_swap_loop(RandomAccessIter * bins, const RandomAccessIter & nextbinstart, unsigned ii
  		, const unsigned log_divisor, const div_type div_min) 
  	{
  		RandomAccessIter * local_bin = bins + ii;
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			for(RandomAccessIter * target_bin = (bins + ((cast_float_iter<div_type, RandomAccessIter>(current) >> log_divisor) - div_min));  target_bin != local_bin; 
  				target_bin = bins + ((cast_float_iter<div_type, RandomAccessIter>(current) >> log_divisor) - div_min)) {
  				data_type tmp;
  				RandomAccessIter b = (*target_bin)++;
  				RandomAccessIter * b_bin = bins + ((cast_float_iter<div_type, RandomAccessIter>(b) >> log_divisor) - div_min);
  				
  				if (b_bin != local_bin) {
  					RandomAccessIter c = (*b_bin)++;
  					tmp = *c;
  					*c = *b;
  				} 
  				else
  					tmp = *b;
  				*b = *current;
  				*current = tmp;
  			}
  		}
  		*local_bin = nextbinstart;
  	}

  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void float_swap_loop(RandomAccessIter * bins, RandomAccessIter & nextbinstart, unsigned ii
  		, const std::vector<size_t> &bin_sizes, const unsigned log_divisor, const div_type div_min) 
  	{
  		nextbinstart += bin_sizes[ii];
  		inner_float_swap_loop<RandomAccessIter, div_type, data_type>(bins, nextbinstart, ii, log_divisor, div_min);
  	}

  	template <class RandomAccessIter, class cast_type>
  	inline void 
  	find_extremes(RandomAccessIter current, RandomAccessIter last, cast_type & max, cast_type & min)
  	{
  		min = max = cast_float_iter<cast_type, RandomAccessIter>(current);
  		while(++current < last) {
  			cast_type value = cast_float_iter<cast_type, RandomAccessIter>(current);
  			if(max < value)
  				max = value;
  			else if(value < min)
  				min = value;
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void 
  	positive_float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[(cast_float_iter<div_type, RandomAccessIter>(current++) >> log_divisor) - div_min]++;
  		bins[0] = first;
  		for(unsigned u = 0; u < bin_count - 1; u++)
  			bins[u + 1] = bins[u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count - 1; ++u)
  			float_swap_loop<RandomAccessIter, div_type, data_type>(bins, nextbinstart, u, bin_sizes, log_divisor, div_min);
  		bins[bin_count - 1] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			else
  				positive_float_sort_rec<RandomAccessIter, div_type, data_type>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void 
  	negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[(cast_float_iter<div_type, RandomAccessIter>(current++) >> log_divisor) - div_min]++;
  		bins[bin_count - 1] = first;
  		for(int ii = bin_count - 2; ii >= 0; --ii)
  			bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		for(int ii = bin_count - 1; ii > 0; --ii)
  			float_swap_loop<RandomAccessIter, div_type, data_type>(bins, nextbinstart, ii, bin_sizes, log_divisor, div_min);
  		
  		bin_cache[cache_offset] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_end - 1; ii >= (int)cache_offset; lastPos = bin_cache[ii], --ii) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii]);
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void 
  	negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min, shift);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		bins[bin_count - 1] = first;
  		for(int ii = bin_count - 2; ii >= 0; --ii)
  			bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		for(int ii = bin_count - 1; ii > 0; --ii)
  			swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, ii, shift, bin_sizes, log_divisor, div_min);
  		
  		bin_cache[cache_offset] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_end - 1; ii >= (int)cache_offset; lastPos = bin_cache[ii], --ii) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii]);
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes, shift);
  		}
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift, class compare>
  	inline void 
  	negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift, compare comp)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min, shift);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		bins[bin_count - 1] = first;
  		for(int ii = bin_count - 2; ii >= 0; --ii)
  			bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		for(int ii = bin_count - 1; ii > 0; --ii)
  			swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, ii, shift, bin_sizes, log_divisor, div_min);
  		
  		bin_cache[cache_offset] = last;
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_end - 1; ii >= (int)cache_offset; lastPos = bin_cache[ii], --ii) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii], comp);
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type, right_shift, compare>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes, shift, comp);
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type>
  	inline void 
  	float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[(cast_float_iter<div_type, RandomAccessIter>(current++) >> log_divisor) - div_min]++;
  		
  		div_type first_positive = (div_min < 0) ? -div_min : 0;
  		
  		if(cache_offset + first_positive > cache_end)
  			first_positive = cache_end - cache_offset;
  		
  		
  		
  		
  		if(first_positive > 0) {
  			bins[first_positive - 1] = first;
  			for(int ii = first_positive - 2; ii >= 0; --ii) {
  				bins[ii] = first + bin_sizes[ii + 1];
  				bin_sizes[ii] += bin_sizes[ii + 1];
  			}
  			
  			if((unsigned)first_positive < bin_count) {
  				bins[first_positive] = first + bin_sizes[0];
  				bin_sizes[first_positive] += bin_sizes[0];
  			}
  		}
  		else
  			bins[0] = first;
  		for(unsigned u = first_positive; u < bin_count - 1; u++) {
  			bins[u + 1] = first + bin_sizes[u];
  			bin_sizes[u + 1] += bin_sizes[u];
  		}
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count; ++u) {
  			nextbinstart = first + bin_sizes[u];
  			inner_float_swap_loop<RandomAccessIter, div_type, data_type>(bins, nextbinstart, u, log_divisor, div_min);
  		}
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_offset + first_positive - 1; ii >= (int)cache_offset ; lastPos = bin_cache[ii--]) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii]);
  			
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes);
  		}
  		
  		for(unsigned u = cache_offset + first_positive; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			
  			else
  				positive_float_sort_rec<RandomAccessIter, div_type, data_type>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void 
  	float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min, shift);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		
  		div_type first_positive = (div_min < 0) ? -div_min : 0;
  		
  		if(cache_offset + first_positive > cache_end)
  			first_positive = cache_end - cache_offset;
  		
  		
  		
  		
  		if(first_positive > 0) {
  			bins[first_positive - 1] = first;
  			for(int ii = first_positive - 2; ii >= 0; --ii) {
  				bins[ii] = first + bin_sizes[ii + 1];
  				bin_sizes[ii] += bin_sizes[ii + 1];
  			}
  			
  			if((unsigned)first_positive < bin_count) {
  				bins[first_positive] = first + bin_sizes[0];
  				bin_sizes[first_positive] += bin_sizes[0];
  			}
  		}
  		else
  			bins[0] = first;
  		for(unsigned u = first_positive; u < bin_count - 1; u++) {
  			bins[u + 1] = first + bin_sizes[u];
  			bin_sizes[u + 1] += bin_sizes[u];
  		}
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count; ++u) {
  			nextbinstart = first + bin_sizes[u];
  			inner_swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, u, shift, log_divisor, div_min);
  		}
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_offset + first_positive - 1; ii >= (int)cache_offset ; lastPos = bin_cache[ii--]) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii]);
  			
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes, shift);
  		}
  		
  		for(unsigned u = cache_offset + first_positive; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			
  			else
  				spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, shift);
  		}
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift, class compare>
  	inline void 
  	float_sort_rec(RandomAccessIter first, RandomAccessIter last, std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
  					, std::vector<size_t> &bin_sizes, right_shift shift, compare comp)
  	{
  		div_type max, min;
  		find_extremes(first, last, max, min, shift);
  		if(max == min)
  			return;
  		unsigned log_divisor = get_log_divisor(last - first, rough_log_2_size((size_t)(max) - min));
  		div_type div_min = min >> log_divisor;
  		div_type div_max = max >> log_divisor;
  		unsigned bin_count = div_max - div_min + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);
  			
  		
  		for (RandomAccessIter current = first; current != last;)
  			bin_sizes[shift(*(current++), log_divisor) - div_min]++;
  		
  		div_type first_positive = (div_min < 0) ? -div_min : 0;
  		
  		if(cache_offset + first_positive > cache_end)
  			first_positive = cache_end - cache_offset;
  		
  		
  		
  		
  		if(first_positive > 0) {
  			bins[first_positive - 1] = first;
  			for(int ii = first_positive - 2; ii >= 0; --ii) {
  				bins[ii] = first + bin_sizes[ii + 1];
  				bin_sizes[ii] += bin_sizes[ii + 1];
  			}
  			
  			if((unsigned)first_positive < bin_count) {
  				bins[first_positive] = first + bin_sizes[0];
  				bin_sizes[first_positive] += bin_sizes[0];
  			}
  		}
  		else
  			bins[0] = first;
  		for(unsigned u = first_positive; u < bin_count - 1; u++) {
  			bins[u + 1] = first + bin_sizes[u];
  			bin_sizes[u + 1] += bin_sizes[u];
  		}
  		
  		
  		RandomAccessIter nextbinstart = first;
  		for(unsigned u = 0; u < bin_count; ++u) {
  			nextbinstart = first + bin_sizes[u];
  			inner_swap_loop<RandomAccessIter, div_type, data_type, right_shift>(bins, nextbinstart, u, shift, log_divisor, div_min);
  		}
  		
  		
  		if(!log_divisor)
  			return;
  		
  		
  		size_t max_count = get_max_count(log_divisor, last - first);
  		RandomAccessIter lastPos = first;
  		for(int ii = cache_offset + first_positive - 1; ii >= (int)cache_offset ; lastPos = bin_cache[ii--]) {
  			size_t count = bin_cache[ii] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[ii]);
  			
  			else
  				negative_float_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes, shift, comp);
  		}
  		
  		for(unsigned u = cache_offset + first_positive; u < cache_end; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			if(count < 2)
  				continue;
  			if(count < max_count)
  				std::sort(lastPos, bin_cache[u]);
  			
  			else
  				spread_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, shift, comp);
  		}
  	}

  	template <class RandomAccessIter, class cast_type, class data_type>
  	inline void 
  	float_Sort(RandomAccessIter first, RandomAccessIter last, cast_type, data_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		float_sort_rec<RandomAccessIter, cast_type, data_type>(first, last, bin_cache, 0, bin_sizes);
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift>
  	inline void 
  	float_Sort(RandomAccessIter first, RandomAccessIter last, div_type, data_type, right_shift shift)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		float_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(first, last, bin_cache, 0, bin_sizes, shift);
  	}

  	template <class RandomAccessIter, class div_type, class data_type, class right_shift, class compare>
  	inline void 
  	float_Sort(RandomAccessIter first, RandomAccessIter last, div_type, data_type, right_shift shift, compare comp)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		float_sort_rec<RandomAccessIter, div_type, data_type, right_shift>(first, last, bin_cache, 0, bin_sizes, shift, comp);
  	}
  }

  
  
  template <class RandomAccessIter, class cast_type>
  inline void float_sort_cast(RandomAccessIter first, RandomAccessIter last, cast_type cVal) 
  {
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else
  		detail::float_Sort(first, last, cVal, *first);
  }

  
  
  template <class RandomAccessIter>
  inline void float_sort_cast_to_int(RandomAccessIter first, RandomAccessIter last) 
  {
  	int cVal = 0;
  	float_sort_cast(first, last, cVal);
  }

  
  template <class RandomAccessIter, class right_shift>
  inline void float_sort(RandomAccessIter first, RandomAccessIter last, right_shift shift) 
  {
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else
  		detail::float_Sort(first, last, shift(*first, 0), *first, shift);
  }

  template <class RandomAccessIter, class right_shift, class compare>
  inline void float_sort(RandomAccessIter first, RandomAccessIter last, right_shift shift, compare comp) 
  {
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last, comp);
  	else
  		detail::float_Sort(first, last, shift(*first, 0), *first, shift, comp);
  }

  
  namespace detail {
  	
  	template<class RandomAccessIter>
  	inline void
  	update_offset(RandomAccessIter first, RandomAccessIter finish, unsigned &char_offset)
  	{
  		unsigned nextOffset = char_offset;
  		bool done = false;
  		while(!done) {
  			RandomAccessIter curr = first;
  			do {
  				
  				if((*curr).size() > char_offset && ((*curr).size() <= (nextOffset + 1) || (*curr)[nextOffset] != (*first)[nextOffset])) {
  					done = true;
  					break;
  				}
  			} while(++curr != finish);
  			if(!done)
  				++nextOffset;
  		} 
  		char_offset = nextOffset;
  	}

  	
  	template<class RandomAccessIter, class get_char, class get_length>
  	inline void
  	update_offset(RandomAccessIter first, RandomAccessIter finish, unsigned &char_offset, get_char getchar, get_length length)
  	{
  		unsigned nextOffset = char_offset;
  		bool done = false;
  		while(!done) {
  			RandomAccessIter curr = first;
  			do {
  				
  				if(length(*curr) > char_offset && (length(*curr) <= (nextOffset + 1) || getchar((*curr), nextOffset) != getchar((*first), nextOffset))) {
  					done = true;
  					break;
  				}
  			} while(++curr != finish);
  			if(!done)
  				++nextOffset;
  		} 
  		char_offset = nextOffset;
  	}

  	
  	template<class data_type, class unsignedchar_type>
  	struct offset_lessthan {
  		offset_lessthan(unsigned char_offset) : fchar_offset(char_offset){}
  		inline bool operator()(const data_type &x, const data_type &y) const 
  		{
  			unsigned minSize = std::min(x.size(), y.size());
  			for(unsigned u = fchar_offset; u < minSize; ++u) {
  				if(static_cast<unsignedchar_type>(x[u]) < static_cast<unsignedchar_type>(y[u]))
  					return true;
  				else if(static_cast<unsignedchar_type>(y[u]) < static_cast<unsignedchar_type>(x[u]))
  					return false;
  			}
  			return x.size() < y.size();
  		}
  		unsigned fchar_offset;
  	};

  	
  	template<class data_type, class unsignedchar_type>
  	struct offset_greaterthan {
  		offset_greaterthan(unsigned char_offset) : fchar_offset(char_offset){}
  		inline bool operator()(const data_type &x, const data_type &y) const 
  		{
  			unsigned minSize = std::min(x.size(), y.size());
  			for(unsigned u = fchar_offset; u < minSize; ++u) {
  				if(static_cast<unsignedchar_type>(x[u]) > static_cast<unsignedchar_type>(y[u]))
  					return true;
  				else if(static_cast<unsignedchar_type>(y[u]) > static_cast<unsignedchar_type>(x[u]))
  					return false;
  			}
  			return x.size() > y.size();
  		}
  		unsigned fchar_offset;
  	};

  	
  	template<class data_type, class get_char, class get_length>
  	struct offset_char_lessthan {
  		offset_char_lessthan(unsigned char_offset) : fchar_offset(char_offset){}
  		inline bool operator()(const data_type &x, const data_type &y) const 
  		{
  			unsigned minSize = std::min(length(x), length(y));
  			for(unsigned u = fchar_offset; u < minSize; ++u) {
  				if(getchar(x, u) < getchar(y, u))
  					return true;
  				else if(getchar(y, u) < getchar(x, u))
  					return false;
  			}
  			return length(x) < length(y);
  		}
  		unsigned fchar_offset;
  		get_char getchar;
  		get_length length;
  	};

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type>
  	inline void 
  	string_sort_rec(RandomAccessIter first, RandomAccessIter last, unsigned char_offset, std::vector<RandomAccessIter> &bin_cache
  		, unsigned cache_offset, std::vector<size_t> &bin_sizes)
  	{
  		
  		
  		while((*first).size() <= char_offset) {
  			if(++first == last)
  				return;
  		}
  		RandomAccessIter finish = last - 1;
  		
  		for(;(*finish).size() <= char_offset; --finish) { }
  		++finish;
  		
  		update_offset(first, finish, char_offset);
  		
  		const unsigned bin_count = (1 << (sizeof(unsignedchar_type)*8));
  		
  		const unsigned max_size = bin_count;
  		const unsigned membin_count = bin_count + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, membin_count) + 1;
  			
  		
  		for (RandomAccessIter current = first; current != last; ++current) {
  			if((*current).size() <= char_offset) {
  				bin_sizes[0]++;
  			}
  			else
  				bin_sizes[static_cast<unsignedchar_type>((*current)[char_offset]) + 1]++;
  		}
  		
  		bin_cache[cache_offset] = first;
  		for(unsigned u = 0; u < membin_count - 1; u++)
  			bin_cache[cache_offset + u + 1] = bin_cache[cache_offset + u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
  		nextbinstart +=	bin_sizes[0];
  		RandomAccessIter * target_bin;
  		
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			
  			while((*current).size() > char_offset) {
  				target_bin = bins + static_cast<unsignedchar_type>((*current)[char_offset]);
  				iter_swap(current, (*target_bin)++);
  			}
  		}
  		*local_bin = nextbinstart;
  		
  		unsigned last_bin = bin_count - 1;
  		for(; last_bin && !bin_sizes[last_bin + 1]; --last_bin) { }
  		
  		for(unsigned u = 0; u < last_bin; ++u) {
  			local_bin = bins + u;
  			nextbinstart += bin_sizes[u + 1];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = bins + static_cast<unsignedchar_type>((*current)[char_offset]);  target_bin != local_bin; 
  					target_bin = bins + static_cast<unsignedchar_type>((*current)[char_offset]))
  					iter_swap(current, (*target_bin)++);
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[last_bin] = last;
  		
  		RandomAccessIter lastPos = bin_cache[cache_offset];
  		
  		for(unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_size)
  				std::sort(lastPos, bin_cache[u], offset_lessthan<data_type, unsignedchar_type>(char_offset + 1));
  			else
  				string_sort_rec<RandomAccessIter, data_type, unsignedchar_type>(lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type>
  	inline void 
  	reverse_string_sort_rec(RandomAccessIter first, RandomAccessIter last, unsigned char_offset, std::vector<RandomAccessIter> &bin_cache
  		, unsigned cache_offset, std::vector<size_t> &bin_sizes)
  	{
  		
  		RandomAccessIter curr = first;
  		
  		while((*curr).size() <= char_offset) {
  			if(++curr == last)
  				return;
  		}
  		
  		while((*(--last)).size() <= char_offset) { }
  		++last;
  		
  		update_offset(curr, last, char_offset);
  		RandomAccessIter * target_bin;
  		
  		const unsigned bin_count = (1 << (sizeof(unsignedchar_type)*8));
  		
  		const unsigned max_size = bin_count;
  		const unsigned membin_count = bin_count + 1;
  		const unsigned max_bin = bin_count - 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, membin_count);
  		RandomAccessIter * end_bin = &(bin_cache[cache_offset + max_bin]);
  			
  		
  		for (RandomAccessIter current = first; current != last; ++current) {
  			if((*current).size() <= char_offset) {
  				bin_sizes[bin_count]++;
  			}
  			else
  				bin_sizes[max_bin - static_cast<unsignedchar_type>((*current)[char_offset])]++;
  		}
  		
  		bin_cache[cache_offset] = first;
  		for(unsigned u = 0; u < membin_count - 1; u++)
  			bin_cache[cache_offset + u + 1] = bin_cache[cache_offset + u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = last;
  		
  		RandomAccessIter * local_bin = &(bin_cache[cache_offset + bin_count]);
  		RandomAccessIter lastFull = *local_bin;
  		
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			
  			while((*current).size() > char_offset) {
  				target_bin = end_bin - static_cast<unsignedchar_type>((*current)[char_offset]);
  				iter_swap(current, (*target_bin)++);
  			}
  		}
  		*local_bin = nextbinstart;
  		nextbinstart = first;
  		
  		unsigned last_bin = max_bin;
  		for(; last_bin && !bin_sizes[last_bin]; --last_bin) { }
  		
  		for(unsigned u = 0; u < last_bin; ++u) {
  			local_bin = bins + u;
  			nextbinstart += bin_sizes[u];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = end_bin - static_cast<unsignedchar_type>((*current)[char_offset]);  target_bin != local_bin; 
  					target_bin = end_bin - static_cast<unsignedchar_type>((*current)[char_offset]))
  					iter_swap(current, (*target_bin)++);
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[last_bin] = lastFull;
  		
  		RandomAccessIter lastPos = first;
  		
  		for(unsigned u = cache_offset; u <= cache_offset + last_bin; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_size)
  				std::sort(lastPos, bin_cache[u], offset_greaterthan<data_type, unsignedchar_type>(char_offset + 1));
  			else
  				reverse_string_sort_rec<RandomAccessIter, data_type, unsignedchar_type>(lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes);
  		}
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type, class get_char, class get_length>
  	inline void 
  	string_sort_rec(RandomAccessIter first, RandomAccessIter last, unsigned char_offset, std::vector<RandomAccessIter> &bin_cache
  		, unsigned cache_offset, std::vector<size_t> &bin_sizes, get_char getchar, get_length length)
  	{
  		
  		
  		while(length(*first) <= char_offset) {
  			if(++first == last)
  				return;
  		}
  		RandomAccessIter finish = last - 1;
  		
  		for(;length(*finish) <= char_offset; --finish) { }
  		++finish;
  		update_offset(first, finish, char_offset, getchar, length);
  		
  		const unsigned bin_count = (1 << (sizeof(unsignedchar_type)*8));
  		
  		const unsigned max_size = bin_count;
  		const unsigned membin_count = bin_count + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, membin_count) + 1;
  			
  		
  		for (RandomAccessIter current = first; current != last; ++current) {
  			if(length(*current) <= char_offset) {
  				bin_sizes[0]++;
  			}
  			else
  				bin_sizes[getchar((*current), char_offset) + 1]++;
  		}
  		
  		bin_cache[cache_offset] = first;
  		for(unsigned u = 0; u < membin_count - 1; u++)
  			bin_cache[cache_offset + u + 1] = bin_cache[cache_offset + u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
  		nextbinstart +=	bin_sizes[0];
  		RandomAccessIter * target_bin;
  		
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			
  			while(length(*current) > char_offset) {
  				target_bin = bins + getchar((*current), char_offset);
  				iter_swap(current, (*target_bin)++);
  			}
  		}
  		*local_bin = nextbinstart;
  		
  		unsigned last_bin = bin_count - 1;
  		for(; last_bin && !bin_sizes[last_bin + 1]; --last_bin) { }
  		
  		for(unsigned ii = 0; ii < last_bin; ++ii) {
  			local_bin = bins + ii;
  			nextbinstart += bin_sizes[ii + 1];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = bins + getchar((*current), char_offset);  target_bin != local_bin; 
  					target_bin = bins + getchar((*current), char_offset))
  					iter_swap(current, (*target_bin)++);
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[last_bin] = last;
  		
  		
  		RandomAccessIter lastPos = bin_cache[cache_offset];
  		
  		for(unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_size)
  				std::sort(lastPos, bin_cache[u], offset_char_lessthan<data_type, get_char, get_length>(char_offset + 1));
  			else
  				string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length>(lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes, getchar, length);
  		}
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type, class get_char, class get_length, class compare>
  	inline void 
  	string_sort_rec(RandomAccessIter first, RandomAccessIter last, unsigned char_offset, std::vector<RandomAccessIter> &bin_cache
  		, unsigned cache_offset, std::vector<size_t> &bin_sizes, get_char getchar, get_length length, compare comp)
  	{
  		
  		
  		while(length(*first) <= char_offset) {
  			if(++first == last)
  				return;
  		}
  		RandomAccessIter finish = last - 1;
  		
  		for(;length(*finish) <= char_offset; --finish) { }
  		++finish;
  		update_offset(first, finish, char_offset, getchar, length);
  		
  		const unsigned bin_count = (1 << (sizeof(unsignedchar_type)*8));
  		
  		const unsigned max_size = bin_count;
  		const unsigned membin_count = bin_count + 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, membin_count) + 1;
  			
  		
  		for (RandomAccessIter current = first; current != last; ++current) {
  			if(length(*current) <= char_offset) {
  				bin_sizes[0]++;
  			}
  			else
  				bin_sizes[getchar((*current), char_offset) + 1]++;
  		}
  		
  		bin_cache[cache_offset] = first;
  		for(unsigned u = 0; u < membin_count - 1; u++)
  			bin_cache[cache_offset + u + 1] = bin_cache[cache_offset + u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = first;
  		
  		RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
  		nextbinstart +=	bin_sizes[0];
  		RandomAccessIter * target_bin;
  		
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			
  			while(length(*current) > char_offset) {
  				target_bin = bins + getchar((*current), char_offset);
  				iter_swap(current, (*target_bin)++);
  			}
  		}
  		*local_bin = nextbinstart;
  		
  		unsigned last_bin = bin_count - 1;
  		for(; last_bin && !bin_sizes[last_bin + 1]; --last_bin) { }
  		
  		for(unsigned u = 0; u < last_bin; ++u) {
  			local_bin = bins + u;
  			nextbinstart += bin_sizes[u + 1];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = bins + getchar((*current), char_offset);  target_bin != local_bin; 
  					target_bin = bins + getchar((*current), char_offset))
  					iter_swap(current, (*target_bin)++);
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[last_bin] = last;
  		
  		
  		RandomAccessIter lastPos = bin_cache[cache_offset];
  		
  		for(unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_size)
  				std::sort(lastPos, bin_cache[u], comp);
  			else
  				string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length, compare>(lastPos
  					, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes, getchar, length, comp);
  		}
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type, class get_char, class get_length, class compare>
  	inline void 
  	reverse_string_sort_rec(RandomAccessIter first, RandomAccessIter last, unsigned char_offset, std::vector<RandomAccessIter> &bin_cache
  		, unsigned cache_offset, std::vector<size_t> &bin_sizes, get_char getchar, get_length length, compare comp)
  	{
  		
  		RandomAccessIter curr = first;
  		
  		while(length(*curr) <= char_offset) {
  			if(++curr == last)
  				return;
  		}
  		
  		while(length(*(--last)) <= char_offset) { }
  		++last;
  		
  		update_offset(first, last, char_offset, getchar, length);
  		
  		const unsigned bin_count = (1 << (sizeof(unsignedchar_type)*8));
  		
  		const unsigned max_size = bin_count;
  		const unsigned membin_count = bin_count + 1;
  		const unsigned max_bin = bin_count - 1;
  		unsigned cache_end;
  		RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset, cache_end, membin_count);
  		RandomAccessIter *end_bin = &(bin_cache[cache_offset + max_bin]);
  			
  		
  		for (RandomAccessIter current = first; current != last; ++current) {
  			if(length(*current) <= char_offset) {
  				bin_sizes[bin_count]++;
  			}
  			else
  				bin_sizes[max_bin - getchar((*current), char_offset)]++;
  		}
  		
  		bin_cache[cache_offset] = first;
  		for(unsigned u = 0; u < membin_count - 1; u++)
  			bin_cache[cache_offset + u + 1] = bin_cache[cache_offset + u] + bin_sizes[u];
  		
  		
  		RandomAccessIter nextbinstart = last;
  		
  		RandomAccessIter * local_bin = &(bin_cache[cache_offset + bin_count]);
  		RandomAccessIter lastFull = *local_bin;
  		RandomAccessIter * target_bin;
  		
  		for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  			
  			while(length(*current) > char_offset) {
  				target_bin = end_bin - getchar((*current), char_offset);
  				iter_swap(current, (*target_bin)++);
  			}
  		}
  		*local_bin = nextbinstart;
  		nextbinstart = first;
  		
  		unsigned last_bin = max_bin;
  		for(; last_bin && !bin_sizes[last_bin]; --last_bin) { }
  		
  		for(unsigned u = 0; u < last_bin; ++u) {
  			local_bin = bins + u;
  			nextbinstart += bin_sizes[u];
  			
  			for(RandomAccessIter current = *local_bin; current < nextbinstart; ++current) {
  				
  				for(target_bin = end_bin - getchar((*current), char_offset);  target_bin != local_bin; 
  					target_bin = end_bin - getchar((*current), char_offset))
  					iter_swap(current, (*target_bin)++);
  			}
  			*local_bin = nextbinstart;
  		}
  		bins[last_bin] = lastFull;
  		
  		RandomAccessIter lastPos = first;
  		
  		for(unsigned u = cache_offset; u <= cache_offset + last_bin; lastPos = bin_cache[u], ++u) {
  			size_t count = bin_cache[u] - lastPos;
  			
  			if(count < 2)
  				continue;
  			
  			if(count < max_size)
  				std::sort(lastPos, bin_cache[u], comp);
  			else
  				reverse_string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length, compare>(lastPos
  					, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes, getchar, length, comp);
  		}
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type>
  	inline void 
  	string_sort(RandomAccessIter first, RandomAccessIter last, data_type, unsignedchar_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		string_sort_rec<RandomAccessIter, data_type, unsignedchar_type>(first, last, 0, bin_cache, 0, bin_sizes);
  	}

  	
  	template <class RandomAccessIter, class data_type, class unsignedchar_type>
  	inline void 
  	reverse_string_sort(RandomAccessIter first, RandomAccessIter last, data_type, unsignedchar_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		reverse_string_sort_rec<RandomAccessIter, data_type, unsignedchar_type>(first, last, 0, bin_cache, 0, bin_sizes);
  	}

  	
  	template <class RandomAccessIter, class get_char, class get_length, class data_type, class unsignedchar_type>
  	inline void 
  	string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length, data_type, unsignedchar_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length>(first, last, 0, bin_cache, 0, bin_sizes, getchar, length);
  	}

  	
  	template <class RandomAccessIter, class get_char, class get_length, class compare, class data_type, class unsignedchar_type>
  	inline void 
  	string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length, compare comp, data_type, unsignedchar_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length, compare>(first, last, 0, bin_cache, 0, bin_sizes, getchar, length, comp);
  	}

  	
  	template <class RandomAccessIter, class get_char, class get_length, class compare, class data_type, class unsignedchar_type>
  	inline void 
  	reverse_string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length, compare comp, data_type, unsignedchar_type)
  	{
  		std::vector<size_t> bin_sizes;
  		std::vector<RandomAccessIter> bin_cache;
  		reverse_string_sort_rec<RandomAccessIter, data_type, unsignedchar_type, get_char, get_length, compare>(first, last, 0, bin_cache, 0, bin_sizes, getchar, length, comp);
  	}
  }

  
  template <class RandomAccessIter, class unsignedchar_type>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last, unsignedchar_type unused) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else
  		detail::string_sort(first, last, *first, unused);
  }

  
  template <class RandomAccessIter>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last) 
  {
  	unsigned char unused = '\0';
  	string_sort(first, last, unused);
  }

  
  template <class RandomAccessIter, class compare, class unsignedchar_type>
  inline void reverse_string_sort(RandomAccessIter first, RandomAccessIter last, compare comp, unsignedchar_type unused) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last, comp);
  	else
  		detail::reverse_string_sort(first, last, *first, unused);
  }

  
  template <class RandomAccessIter, class compare>
  inline void reverse_string_sort(RandomAccessIter first, RandomAccessIter last, compare comp) 
  {
  	unsigned char unused = '\0';
  	reverse_string_sort(first, last, comp, unused);
  }

  template <class RandomAccessIter, class get_char, class get_length>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last);
  	else {
  		
  		
  		while(!length(*first)) {
  			if(++first == last)
  				return;
  		}
  		detail::string_sort(first, last, getchar, length, *first, getchar((*first), 0));
  	}
  }

  template <class RandomAccessIter, class get_char, class get_length, class compare>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length, compare comp) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last, comp);
  	else {
  		
  		
  		while(!length(*first)) {
  			if(++first == last)
  				return;
  		}
  		detail::string_sort(first, last, getchar, length, comp, *first, getchar((*first), 0));
  	}
  }

  template <class RandomAccessIter, class get_char, class get_length, class compare>
  inline void reverse_string_sort(RandomAccessIter first, RandomAccessIter last, get_char getchar, get_length length, compare comp) 
  {
  	
  	if(last - first < detail::MIN_SORT_SIZE)
  		std::sort(first, last, comp);
  	else {
  		
  		
  		while(!length(*(--last))) {
  			
  			if(first == last)
  				return;
  		}
  		
  		++last;
  		detail::reverse_string_sort(first, last, getchar, length, comp, *first, getchar((*first), 0));
  	}
  }
}

#endif
