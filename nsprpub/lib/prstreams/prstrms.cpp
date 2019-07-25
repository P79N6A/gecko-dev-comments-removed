










































#include "prstrms.h"

#include <cstdio>
#include <cstring>
#include <ios>
#include <new>

using std::ios_base;
using std::iostream;
using std::istream;
using std::nothrow;
using std::ostream;
using std::streambuf;
using std::streamsize;


PRfilebuf::PRfilebuf():
    _fd(NULL),
    _opened(false),
    _allocated(false),
    _unbuffered(false),
    _user_buf(false),
    _buf_base(NULL),
    _buf_end(NULL) { }


PRfilebuf::PRfilebuf(PRFileDesc *fd):
    _fd(fd),
    _opened(false),
    _allocated(false),
    _unbuffered(false),
    _user_buf(false),
    _buf_base(NULL),
    _buf_end(NULL) { }


PRfilebuf::PRfilebuf(PRFileDesc *fd, char_type *ptr, streamsize len):
    _fd(fd),
    _opened(false),
    _allocated(false),
    _unbuffered(false),
    _user_buf(false),
    _buf_base(NULL),
    _buf_end(NULL)
{
    setbuf(ptr, len);
}


PRfilebuf::~PRfilebuf()
{
    if (_opened) {
        close();
    } else {
        sync();
    }
    if (_allocated) {
        delete _buf_base;
    }
}


PRfilebuf *PRfilebuf::open(
    const char *name, ios_base::openmode flags, PRIntn mode)
{
    if (_fd != NULL) {
        return NULL;  
    }

    
    PRIntn prflags = 0;
    bool ate = (flags & ios_base::ate) != 0;
    flags &= ~(ios_base::ate | ios_base::binary);

    
    
    
    
    
    
    
    

    if (flags == (ios_base::out)) {
        prflags = PR_WRONLY | PR_TRUNCATE;
    } else if (flags == (ios_base::out | ios_base::app)) {
        prflags = PR_RDWR | PR_APPEND;
    } else if (flags == (ios_base::out | ios_base::trunc)) {
        prflags = PR_WRONLY | PR_TRUNCATE;
    } else if (flags == (ios_base::in)) {
        prflags = PR_RDONLY;
    } else if (flags == (ios_base::in | ios_base::out)) {
        prflags = PR_RDWR;
    } else if (flags == (ios_base::in | ios_base::out | ios_base::trunc)) {
        prflags = PR_RDWR | PR_TRUNCATE;
    } else {
        return NULL;  
    }

    if ((_fd = PR_Open(name, prflags, mode)) == NULL) {
        return NULL;
    }

    _opened = true;

    if (ate &&
            seekoff(0, ios_base::end, flags) == pos_type(traits_type::eof())) {
        close();
        return NULL;
    }

    return this;
}


PRfilebuf *PRfilebuf::attach(PRFileDesc *fd)
{
    if (_fd != NULL) {
        return NULL;  
    }

    _opened = false;
    _fd = fd;
    return this;
}


PRfilebuf *PRfilebuf::close()
{
    if (_fd == NULL)
        return NULL;

    int status = sync();

    if (PR_Close(_fd) == PR_FAILURE ||
            traits_type::eq_int_type(status, traits_type::eof())) {
        return NULL;
    }

    _fd = NULL;
    return this;
}


streambuf *PRfilebuf::setbuf(char_type *ptr, streamsize len)
{
    if (is_open() && _buf_end) {
        return NULL;
    }

    if (!ptr || len <= 0) {
        _unbuffered = true;
    } else {
        setb(ptr, ptr + len, false);
    }

    return this;
}


streambuf::pos_type PRfilebuf::seekoff(
    off_type offset, ios_base::seekdir dir, ios_base::openmode )
{
    if (PR_GetDescType(_fd) != PR_DESC_FILE) {
        return traits_type::eof();
    }

    PRSeekWhence whence;
    PRInt64 pos;

    switch (dir) {
        case ios_base::beg: whence = PR_SEEK_SET; break;
        case ios_base::cur: whence = PR_SEEK_CUR; break;
        case ios_base::end: whence = PR_SEEK_END; break;
        default:
            return traits_type::eof();  
    }

    if (traits_type::eq_int_type(sync(), traits_type::eof())) {
        return traits_type::eof();
    }

    if ((pos = PR_Seek64(_fd, offset, whence)) == -1) {
        return traits_type::eof();
    }

    return pos;
}


int PRfilebuf::sync()
{
    if (_fd == NULL) {
        return traits_type::eof();
    }

    if (!_unbuffered) {
        
        PRInt32 waiting;
        if ((waiting = pptr() - pbase()) != 0) {
            PRInt32 nout;
            if ((nout = PR_Write(_fd, pbase(), waiting)) != waiting) {
                if (nout > 0) {
                    
                    pbump(-nout);
                    memmove(pbase(), pbase() + nout, waiting - nout);
                }
                return traits_type::eof();
            }
        }
        setp(NULL, NULL);  

        if (PR_GetDescType(_fd) == PR_DESC_FILE) {
            
            PROffset64 avail;
            if ((avail = in_avail()) > 0) {
                if (PR_Seek64(_fd, -avail, PR_SEEK_CUR) != -1) {
                    return traits_type::eof();
                }
            }
        }
        setg(NULL, NULL, NULL);  
    }

    return 0;
}


streambuf::int_type PRfilebuf::underflow()
{
    PRInt32 count;
    char_type byte;

    if (gptr() != NULL && gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    
    if (!_unbuffered && _buf_base == NULL && !allocate()) {
        return traits_type::eof();
    }

    
    if (traits_type::eq_int_type(sync(), traits_type::eof())) {
        return traits_type::eof();
    }

    if (_unbuffered) {
        if (PR_Read(_fd, &byte, 1) <= 0) {
            return traits_type::eof();
        }

        return traits_type::to_int_type(byte);
    }

    if ((count = PR_Read(_fd, _buf_base, _buf_end - _buf_base)) <= 0) {
        return traits_type::eof();  
    }

    setg(_buf_base, _buf_base, _buf_base + count);
    return traits_type::to_int_type(*gptr());
}


streambuf::int_type PRfilebuf::overflow(int_type c)
{
    
    if (!_unbuffered && _buf_base == NULL && !allocate()) {
        return traits_type::eof();
    }

    
    if (traits_type::eq_int_type(sync(), traits_type::eof())) {
        return traits_type::eof();
    }

    if (!_unbuffered) {
        setp(_buf_base, _buf_end);
    }

    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        
        
        char_type byte = traits_type::to_char_type(c);
        if (!_unbuffered && pptr() < epptr()) {  
            return sputc(byte);
        } else {
            if (PR_Write(_fd, &byte, 1) != 1) {
                return traits_type::eof();
            }
        }
    }

    return traits_type::not_eof(c);
}


bool PRfilebuf::allocate()
{
    char_type *buf = new(nothrow) char_type[BUFSIZ];
    if (buf == NULL) {
        return false;
    }

    setb(buf, buf + BUFSIZ, true);
    return true;
}


void PRfilebuf::setb(char_type *buf_base, char_type *buf_end, bool user_buf)
{
    if (_buf_base && !_user_buf) {
        delete[] _buf_base;
    }

    _buf_base = buf_base;
    _buf_end = buf_end;
    _user_buf = user_buf;
}


PRifstream::PRifstream():
    istream(NULL),
    _filebuf()
{
    init(&_filebuf);
}


PRifstream::PRifstream(PRFileDesc *fd):
    istream(NULL),
    _filebuf(fd)
{
    init(&_filebuf);
}


PRifstream::PRifstream(PRFileDesc *fd, char_type *ptr, streamsize len):
    istream(NULL),
    _filebuf(fd, ptr, len)
{
    init(&_filebuf);
}


PRifstream::PRifstream(const char *name, openmode flags, PRIntn mode):
    istream(NULL),
    _filebuf()
{
    init(&_filebuf);
    if (!_filebuf.open(name, flags | in, mode)) {
        setstate(failbit);
    }
}


PRifstream::~PRifstream() { }


void PRifstream::open(const char *name, openmode flags, PRIntn mode)
{
    if (is_open() || !_filebuf.open(name, flags | in, mode)) {
        setstate(failbit);
    }
}


void PRifstream::attach(PRFileDesc *fd)
{
    if (!_filebuf.attach(fd)) {
        setstate(failbit);
    }
}


void PRifstream::close()
{
    if (_filebuf.close() == NULL) {
        setstate(failbit);
    }
}


PRofstream::PRofstream():
    ostream(NULL),
    _filebuf()
{
    init(&_filebuf);
}


PRofstream::PRofstream(PRFileDesc *fd):
    ostream(NULL),
    _filebuf(fd)
{
    init(&_filebuf);
}


PRofstream::PRofstream(PRFileDesc *fd, char_type *ptr, streamsize len):
    ostream(NULL),
    _filebuf(fd, ptr, len)
{
    init(&_filebuf);
}


PRofstream::PRofstream(const char *name, openmode flags, PRIntn mode):
    ostream(NULL),
    _filebuf()
{
    init(&_filebuf);
    if (!_filebuf.open(name, flags | out, mode)) {
        setstate(failbit);
    }
}


PRofstream::~PRofstream() { }


void PRofstream::open(const char *name, openmode flags, PRIntn mode)
{
    if (is_open() || !_filebuf.open(name, flags | out, mode)) {
        setstate(failbit);
    }
}


void PRofstream::attach(PRFileDesc *fd)
{
    if (!_filebuf.attach(fd)) {
        setstate(failbit);
    }
}


void PRofstream::close()
{
    if (_filebuf.close() == NULL) {
        setstate(failbit);
    }
}


PRfstream::PRfstream():
    iostream(NULL),
    _filebuf()
{
    init(&_filebuf);
}


PRfstream::PRfstream(PRFileDesc *fd):
    iostream(NULL),
    _filebuf(fd)
{
    init(&_filebuf);
}


PRfstream::PRfstream(PRFileDesc *fd, char_type *ptr, streamsize len):
    iostream(NULL),
    _filebuf(fd, ptr, len)
{
    init(&_filebuf);
}


PRfstream::PRfstream(const char *name, openmode flags, PRIntn mode):
    iostream(NULL),
    _filebuf()
{
    init(&_filebuf);
    if (!_filebuf.open(name, flags | in | out, mode)) {
        setstate(failbit);
    }
}


PRfstream::~PRfstream() { }


void PRfstream::open(const char *name, openmode flags, PRIntn mode)
{
    if (is_open() || !_filebuf.open(name, flags | in | out, mode)) {
        setstate(failbit);
    }
}


void PRfstream::attach(PRFileDesc *fd)
{
    if (!_filebuf.attach(fd)) {
        setstate(failbit);
    }
}


void PRfstream::close()
{
    if (_filebuf.close() == NULL) {
        setstate(failbit);
    }
}
