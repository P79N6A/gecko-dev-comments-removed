

























#pragma once

#include <Main.h>

namespace graphite2 {

class Face;

class Cmap
{
public:
	virtual ~Cmap() throw() {}

	virtual uint16 operator [] (const uint32) const throw() { return 0; }

	virtual operator bool () const throw() { return false; }

	CLASS_NEW_DELETE;
};

class DirectCmap : public Cmap
{
public:
	DirectCmap(const void* cmap, size_t length);
	virtual uint16 operator [] (const uint32 usv) const throw();
	virtual operator bool () const throw();

    CLASS_NEW_DELETE;
private:
    const void *_stable,
    		   *_ctable;
};

class CmapCache : public Cmap
{
public:
	CmapCache(const void * cmapTable, size_t length);
	virtual ~CmapCache() throw();
	virtual uint16 operator [] (const uint32 usv) const throw();
	virtual operator bool () const throw();
    CLASS_NEW_DELETE;
private:
    bool m_isBmpOnly;
    uint16 ** m_blocks;
};

} 
