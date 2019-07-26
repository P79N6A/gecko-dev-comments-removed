





























#pragma once
#include "inc/Main.h"
#include <cassert>
#include <cstdio>

namespace graphite2 {

class json
{
    
    json(const json &);
    json & operator = (const json &);

	typedef void (*_context_t)(json &);
	class _null_t {};

	FILE * const 	_stream;
	char 			_contexts[128],	
				  * _context,		
				  * _flatten;		
									

	void context(const char current) throw();
	void indent(const int d=0) throw();
	void push_context(const char, const char) throw();
	void pop_context() throw();

public:
	class closer;

	typedef const char *	string;
	typedef double			number;
	typedef long signed int	integer;
	typedef bool			boolean;
	static const _null_t	null;

	static void flat(json &) throw();
	static void close(json &) throw();
	static void object(json &) throw();
	static void array(json &) throw();
	static void item(json &) throw();

	json(FILE * stream) throw();
	~json() throw ();

	FILE * stream() const throw();

	json & operator << (string) throw();
	json & operator << (number) throw();
	json & operator << (integer) throw();
	json & operator << (long unsigned int d) throw();
	json & operator << (boolean) throw();
	json & operator << (_null_t) throw();
	json & operator << (_context_t) throw();

	operator bool() const throw();
	bool good() const throw();
	bool eof() const throw();

	CLASS_NEW_DELETE;
};

class json::closer
{
    
    closer(const closer &);
    closer & operator = (const closer &);

	json * const	_j;
public:
	closer(json * const j) : _j(j) {}
	~closer() throw() { if (_j)  *_j << close; }
};

inline
json::json(FILE * s) throw()
: _stream(s), _context(_contexts), _flatten(0)
{
	if (good())
		fflush(s);
}


inline
json::~json() throw ()
{
	while (_context > _contexts)	pop_context();
}

inline
FILE * json::stream() const throw()		{ return _stream; }


inline
json & json::operator << (json::_context_t ctxt) throw()
{
	ctxt(*this);
	return *this;
}

inline
json & operator << (json & j, signed char d) throw() 		{ return j << json::integer(d); }

inline
json & operator << (json & j, short signed int d) throw() 	{ return j << json::integer(d); }

inline
json & operator << (json & j, signed int d) throw() 		{ return j << json::integer(d); }

inline
json & operator << (json & j, unsigned char d) throw() 		{ return j << json::integer(d); }

inline
json & operator << (json & j, short unsigned int d) throw() { return j << json::integer(d); }

inline
json & operator << (json & j, unsigned int d) throw() 		{ return j << json::integer(d); }

inline
json & operator << (json & j, char c) throw ()
{
	const char str[2] = {c,0};
	return j << str;
}

inline
json::operator bool() const throw()		{ return good(); }

inline
bool json::good() const throw()			{ return _stream && ferror(_stream) == 0; }

inline
bool json::eof() const throw()			{ return feof(_stream) != 0; }

} 
