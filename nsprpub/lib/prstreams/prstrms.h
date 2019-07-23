








































#ifndef _PRSTRMS_H
#define _PRSTRMS_H

#include "prtypes.h"
#include "prio.h"

#ifdef _MSC_VER
#pragma warning( disable : 4275)
#endif
#include <iostream.h>

#if defined(AIX) && defined(__64BIT__)
typedef long PRstreambuflen;
#else
typedef int PRstreambuflen;
#endif

#if defined (PRFSTREAMS_BROKEN)



#define	 PRfilebuf	streambuf
#define  PRifstream	ifstream
#define	 PRofstream	ofstream
#define	 PRfstream	fstream

#else

class PR_IMPLEMENT(PRfilebuf): public streambuf
{
public:
    PRfilebuf();
    PRfilebuf(PRFileDesc *fd);
    PRfilebuf(PRFileDesc *fd, char * buffptr, int bufflen);
    ~PRfilebuf();
    virtual	int	overflow(int=EOF);
    virtual	int	underflow();
    virtual	streambuf *setbuf(char *buff, PRstreambuflen bufflen);
    virtual	streampos seekoff(streamoff, ios::seek_dir, int);
    virtual int sync();
    PRfilebuf *open(const char *name, int mode, int flags);
   	PRfilebuf *attach(PRFileDesc *fd);
    PRfilebuf *close();
   	int	is_open() const {return (_fd != 0);}
    PRFileDesc *fd(){return _fd;}

private:
    PRFileDesc * _fd;
    PRBool _opened;
	PRBool _allocated;
};

class PR_IMPLEMENT(PRifstream): public istream {
public:
	PRifstream();
	PRifstream(const char *, int mode=ios::in, int flags = 0);
	PRifstream(PRFileDesc *);
	PRifstream(PRFileDesc *, char *, int);
	~PRifstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf(){return (PRfilebuf*) ios::rdbuf(); }

	void attach(PRFileDesc *fd);
	PRFileDesc *fd() {return rdbuf()->fd();}

	int is_open(){return rdbuf()->is_open();}
	void open(const char *, int mode=ios::in, int flags= 0);
	void close();
};

class PR_IMPLEMENT(PRofstream) : public ostream {
public:
	PRofstream();
	PRofstream(const char *, int mode=ios::out, int flags = 0);
	PRofstream(PRFileDesc *);
	PRofstream(PRFileDesc *, char *, int);
	~PRofstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf() { return (PRfilebuf*) ios::rdbuf(); }

	void attach(PRFileDesc *);
	PRFileDesc *fd() {return rdbuf()->fd();}

	int is_open(){return rdbuf()->is_open();}
	void open(const char *, int =ios::out, int = 0);
	void close();
};
	
class PR_IMPLEMENT(PRfstream) : public iostream {
public:
	PRfstream();
	PRfstream(const char *name, int mode, int flags= 0);
	PRfstream(PRFileDesc *fd);
	PRfstream(PRFileDesc *fd, char *buff, int bufflen);
	~PRfstream();

	streambuf * setbuf(char *, int);
	PRfilebuf* rdbuf(){ return (PRfilebuf*) ostream::rdbuf(); }

	void attach(PRFileDesc *);
	PRFileDesc *fd() { return rdbuf()->fd(); }

	int is_open() { return rdbuf()->is_open(); }
	void open(const char *, int, int = 0);
	void close();
};

#endif

#endif 
