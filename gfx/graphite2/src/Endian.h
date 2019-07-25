



































class be
{
	template<int S>
	inline static unsigned long int _peek(const unsigned char * p) {
		return _peek<S/2>(p) << (S/2)*8 | _peek<S/2>(p+S/2);
	}
public:
	template<typename T>
	inline static T peek(const unsigned char * p) { 
		return T(_peek<sizeof(T)>(p));
	}

	template<typename T>
	inline static T read(const unsigned char * &p) {
		const T r = T(_peek<sizeof(T)>(p)); 
		p += sizeof r;
		return r;
	}
	
	template<typename T>
	inline static T swap(const T x) {
		return T(_peek<sizeof(T)>(reinterpret_cast<const unsigned char *>(&x)));
	}

	template<typename T>
	inline static void skip(const unsigned char * &p, size_t n=1) {
		p += sizeof(T)*n;
	}
};

template<>
inline unsigned long int be::_peek<1>(const unsigned char * p) { return *p; }


class le 
{
	template<int S>
	inline static unsigned long int _peek(const unsigned char * p) {
		return _peek<S/2>(p) | _peek<S/2>(p+S/2)  << (S/2)*8;
	}
public:
	template<typename T>
	inline static T peek(const unsigned char * p) { 
		return T(_peek<sizeof(T)>(p));
	}

	template<typename T>
	inline static T read(const unsigned char * &p) {
		const T r = T(_peek<sizeof(T)>(p)); 
		p += sizeof r;
		return r;
	}
	
	template<typename T>
	inline static T swap(const T x) {
		return T(_peek<sizeof(T)>(reinterpret_cast<const unsigned char *>(&x)));
	}

	template<typename T>
	inline static void skip(const unsigned char * &p, size_t n=1) {
		p += sizeof(T)*n;
	}
};

template<>
inline unsigned long int le::_peek<1>(const unsigned char * p) { return *p; }

