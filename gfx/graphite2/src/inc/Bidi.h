

























#pragma once

namespace graphite2
{

class BracketPair
{
public:
    BracketPair(uint16 g, Slot *s, uint8 b, BracketPair *p, BracketPair *l) : _open(s), _close(0), _parent(p), _next(0), _prev(l), _gid(g), _mask(0), _before(b) {};
    uint16 gid() const { return _gid; }
    Slot *open() const { return _open; }
    Slot *close() const { return _close; }
    uint8 mask() const { return _mask; }
    int8 before() const { return _before; }
    BracketPair *parent() const { return _parent; }
    void close(Slot *s) { _close = s; }
    BracketPair *next() const { return _next; }
    BracketPair *prev() const { return _prev; }
    void next(BracketPair *n) { _next = n; }
    void orin(uint8 m) { _mask |= m; }
private:
    Slot        *   _open;          
    Slot        *   _close;         
    BracketPair *   _parent;        
    BracketPair *   _next;          
    BracketPair *   _prev;          
    uint16          _gid;           
    uint8           _mask;          
    int8            _before;        
};

class BracketPairStack
{
public:
    BracketPairStack(int s) : _stack(grzeroalloc<BracketPair>(s)), _ip(_stack - 1), _top(0), _last(0), _lastclose(0), _size(s) {}
    ~BracketPairStack() { free(_stack); }

public:
    BracketPair *scan(uint16 gid);
    void close(BracketPair *tos, Slot *s);
    BracketPair *push(uint16 gid, Slot *pos, uint8 before, int prevopen);
    void orin(uint8 mask);
    void clear() { _ip = _stack - 1; _top = 0; _last = 0; _lastclose = 0; }
    int size() const { return _size; }
    BracketPair *start() const { return _stack; }

    CLASS_NEW_DELETE

private:

    BracketPair *_stack;        
    BracketPair *_ip;           
    BracketPair *_top;          
    BracketPair *_last;         
    BracketPair *_lastclose;    
    int          _size;         
};

inline BracketPair *BracketPairStack::scan(uint16 gid)
{
    BracketPair *res = _top;
    while (res >= _stack)
    {
        if (res->gid() == gid)
            return res;
        res = res->parent();
    }
    return 0;
}

inline void BracketPairStack::close(BracketPair *tos, Slot *s) 
{
    for ( ; _last && _last != tos && !_last->close(); _last = _last->parent())
    { }
    tos->close(s);
    _last->next(NULL);
    _lastclose = tos;
    _top = tos->parent();
}

inline BracketPair *BracketPairStack::push(uint16 gid, Slot *pos, uint8 before, int prevopen)
{
    if (++_ip - _stack < _size && _stack)
    {
        ::new (_ip) BracketPair(gid, pos, before, _top, prevopen ? _last : _lastclose);
        if (_last) _last->next(_ip);
        _last = _ip;
    }
    _top = _ip;
    return _ip;
}

inline void BracketPairStack::orin(uint8 mask)
{
    BracketPair *t = _top;
    for ( ; t; t = t->parent())
        t->orin(mask);
}

}
