

























#include "inc/Main.h"
#include "inc/Slot.h"
#include "inc/Segment.h"
#include "inc/Bidi.h"

using namespace graphite2;

enum DirCode {  
        Unk        = -1,
        N          =  0,   
        L          =  1,   
        R          =  2,   
        AL         =  3,   
        EN         =  4,   
        EUS        =  5,   
        ET         =  6,   
        AN         =  7,   
        CUS        =  8,   
        WS         =  9,   
        BN         = 10,   

        LRO        = 11,   
        RLO        = 12,   
        LRE        = 13,   
        RLE        = 14,   
        PDF        = 15,   
        NSM        = 16,   
        LRI        = 17,   
        RLI        = 18,   
        FSI        = 19,   
        PDI        = 20,   
        OPP        = 21,   
        CPP        = 22,   

        ON = N
};

enum DirMask {
        WSflag = (1 << 7),     
        WSMask = ~(1 << 7)
};

inline uint8    BaseClass(Slot *s)   { return s->getBidiClass() & WSMask; }

unsigned int bidi_class_map[] = { 0, 1, 2, 5, 4, 8, 9, 3, 7, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0 };


void resolveWeak(Slot *start, int sos, int eos);
void resolveNeutrals(Slot *s, int baseLevel, int sos, int eos);
void processParens(Slot *s, Segment *seg, uint8 aMirror, int level, BracketPairStack &stack);

inline int calc_base_level(Slot *s)
{
    int count = 0;
    for ( ; s; s = s->next())
    {
        int cls = s->getBidiClass();
        if (count)
        {
            switch(cls)
            {
            case LRI :
            case RLI :
            case FSI :
                ++count;
                break;
            case PDI :
                --count;
            }
        }
        else
        {
            switch(cls)
            {
            case L :
                return 0;
            case R :
            case AL :
                return 1;
            case LRI :
            case RLI :
            case FSI :
                ++count;
            }
        }
    }
    return 0;
}


void do_resolves(Slot *start, int level, int sos, int eos, int &bmask, Segment *seg, uint8 aMirror, BracketPairStack &stack)
{
    if (bmask & 0x1F1178)
        resolveWeak(start, sos, eos);
    if (bmask & 0x200000)
        processParens(start, seg, aMirror, level, stack);
    if (bmask & 0x7E0361)
        resolveNeutrals(start, level, sos, eos);
    bmask = 0;
}

enum maxs
{
    MAX_LEVEL = 125,
};


Slot *process_bidi(Slot *start, int level, int prelevel, int &nextLevel, int dirover, int isol, int &cisol, int &isolerr, int &embederr, int init, Segment *seg, uint8 aMirror, BracketPairStack &bstack)
{
    int bmask = 0;
    Slot *s = start;
    Slot *slast = start;
    Slot *scurr = 0;
    Slot *stemp;
    int lnextLevel = nextLevel;
    int newLevel;
    int empty = 1;
    for ( ; s; s = s ? s->next() : s)
    {
        int cls = s->getBidiClass();
        bmask |= (1 << cls);
        s->setBidiLevel(level);
        
        
        switch (cls)
        {
        case BN :
            if (slast == s) slast = s->next();      
            continue;
        case LRE :
        case LRO :
        case RLE :
        case RLO :
            switch (cls)
            {
            case LRE :
            case LRO :
                newLevel = level + (level & 1 ? 1 : 2);
                break;
            case RLE :
            case RLO :
                newLevel = level + (level & 1 ? 2 : 1);
                break;
            }
            s->setBidiClass(BN);
            if (isolerr || newLevel > MAX_LEVEL || embederr)
            {
                if (!isolerr) ++embederr;
                break;
            }
            stemp = scurr;
            if (scurr)
                scurr->prev(0);         
            lnextLevel = newLevel;
            scurr = s;
            s->setBidiLevel(newLevel); 
            
            s = process_bidi(s->next(), newLevel, level, lnextLevel, cls < LRE, 0, cisol, isolerr, embederr, 0, seg, aMirror, bstack);
            
            
            if (lnextLevel != level || !s)      
            {
                if (slast != scurr)             
                {
                    
                    do_resolves(slast, level, (prelevel > level ? prelevel : level) & 1, lnextLevel & 1, bmask, seg, aMirror, bstack);
                    empty = 0;
                    nextLevel = level;
                }
                else if (lnextLevel != level)   
                {
                    empty = 0;                  
                    nextLevel = lnextLevel;     
                }
                if (s)                          
                {
                    prelevel = lnextLevel;      
                    lnextLevel = level;         
                }
                slast = s ? s->next() : s;
            }
            else if (stemp)
                stemp->prev(s);
            break;

        case PDF :
            s->setBidiClass(BN);
            s->prev(0);                         
            if (isol || isolerr || init)        
                break;
            if (embederr)
            {
                --embederr;
                break;
            }
            if (slast != s)
            {
                scurr->prev(0);     
                do_resolves(slast, level, level & 1, level & 1, bmask, seg, aMirror, bstack);
                empty = 0;
            }
            if (empty)
            {
                nextLevel = prelevel;       
                s->setBidiLevel(prelevel);
            }
            return s;

        case FSI :
        case LRI :
        case RLI :
            switch (cls)
            {
            case FSI :
                if (calc_base_level(s->next()))
                    newLevel = level + (level & 1 ? 2 : 1);
                else
                    newLevel = level + (level & 1 ? 1 : 2);
                break;
            case LRI :
                newLevel = level + (level & 1 ? 1 : 2);
                break;
            case RLI :
                newLevel = level + (level & 1 ? 2 : 1);
                break;
            }
            if (newLevel > MAX_LEVEL || isolerr)
            {
                ++isolerr;
                s->setBidiClass(ON | WSflag);
                break;
            }
            ++cisol;
            if (scurr) scurr->prev(s);
            scurr = s;  
            lnextLevel = newLevel;
            
            s = process_bidi(s->next(), newLevel, newLevel, lnextLevel, 0, 1, cisol, isolerr, embederr, 0, seg, aMirror, bstack);
            
            if (s)
            {
                bmask |= 1 << BaseClass(s);     
                s->setBidiLevel(level);         
            }
            lnextLevel = level;
            break;

        case PDI :
            if (isolerr)
            {
                --isolerr;
                s->setBidiClass(ON | WSflag);
                break;
            }
            if (init || !cisol)
            {
                s->setBidiClass(ON | WSflag);
                break;
            }
            embederr = 0;
            if (!isol)                  
            {
                if (empty)              
                    nextLevel = prelevel;
                return s->prev();       
            }
            else                        
            {
                if (slast != s)         
                {
                    scurr->prev(0);
                    do_resolves(slast, level, prelevel & 1, level & 1, bmask, seg, aMirror, bstack);
                }
                --cisol;                
                return s;
            }

        default :
            if (dirover)
                s->setBidiClass((level & 1 ? R : L) | (WSflag * (cls == WS)));
        }
        if (s) s->prev(0);      
        if (scurr)              
            scurr->prev(s);
        scurr = s;              
    }
    if (slast != s)
    {
        do_resolves(slast, level, (level > prelevel ? level : prelevel) & 1, lnextLevel & 1, bmask, seg, aMirror, bstack);
        empty = 0;
    }
    if (empty || isol)
        nextLevel = prelevel;
    return s;
}



enum bidi_state 
{
        xa,             
        xr,             
        xl,             

        ao,             
        ro,             
        lo,             

        rt,             
        lt,             

        cn,             
        ra,             
        re,             
        la,             
        le,             

        ac,             
        rc,             
        rs,             
        lc,             
        ls,             

        ret,            
        let,            
} ;

const bidi_state stateWeak[][10] =
{
        
{         ao, xl, xr, cn, cn, xa, xa, ao, ao, ao,  },
{         ro, xl, xr, ra, re, xa, xr, ro, ro, rt,  },
{         lo, xl, xr, la, le, xa, xl, lo, lo, lt,  },

{         ao, xl, xr, cn, cn, xa, ao, ao, ao, ao,  },
{         ro, xl, xr, ra, re, xa, ro, ro, ro, rt,  },
{         lo, xl, xr, la, le, xa, lo, lo, lo, lt,  },

{         ro, xl, xr, ra, re, xa, rt, ro, ro, rt,  },
{         lo, xl, xr, la, le, xa, lt, lo, lo, lt,  },

{         ao, xl, xr, cn, cn, xa, cn, ac, ao, ao,  },
{         ro, xl, xr, ra, re, xa, ra, rc, ro, rt,  },
{         ro, xl, xr, ra, re, xa, re, rs, rs,ret,  },
{         lo, xl, xr, la, le, xa, la, lc, lo, lt,  },
{         lo, xl, xr, la, le, xa, le, ls, ls,let,  },

{         ao, xl, xr, cn, cn, xa, ao, ao, ao, ao,  },
{         ro, xl, xr, ra, re, xa, ro, ro, ro, rt,  },
{         ro, xl, xr, ra, re, xa, ro, ro, ro, rt,  },
{         lo, xl, xr, la, le, xa, lo, lo, lo, lt,  },
{         lo, xl, xr, la, le, xa, lo, lo, lo, lt,  },

{        ro, xl, xr, ra, re, xa,ret, ro, ro,ret,  },
{        lo, xl, xr, la, le, xa,let, lo, lo,let,  },


};

enum bidi_action 
{
        
        IX = 0x100,                     
        XX = 0xF,                       

        
        xxx = (XX << 4) + XX,           
        xIx = IX + xxx,                         
        xxN = (XX << 4) + ON,           
        xxE = (XX << 4) + EN,           
        xxA = (XX << 4) + AN,           
        xxR = (XX << 4) + R,            
        xxL = (XX << 4) + L,            
        Nxx = (ON << 4) + 0xF,          
        Axx = (AN << 4) + 0xF,          
        ExE = (EN << 4) + EN,           
        NIx = (ON << 4) + 0xF + IX,     
        NxN = (ON << 4) + ON,           
        NxR = (ON << 4) + R,            
        NxE = (ON << 4) + EN,           

        AxA = (AN << 4) + AN,           
        NxL = (ON << 4) + L,            
        LxL = (L << 4) + L,             
};


const bidi_action actionWeak[][10] =
{
    
{  xxx, xxx, xxx, xxx, xxA, xxR, xxR, xxN, xxN, xxN,  },
{  xxx, xxx, xxx, xxx, xxE, xxR, xxR, xxN, xxN, xIx,  },
{  xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xIx,  },

{  xxx, xxx, xxx, xxx, xxA, xxR, xxN, xxN, xxN, xxN,  },
{  xxx, xxx, xxx, xxx, xxE, xxR, xxN, xxN, xxN, xIx,  },
{  xxx, xxx, xxx, xxx, xxL, xxR, xxN, xxN, xxN, xIx,  },

{  Nxx, Nxx, Nxx, Nxx, ExE, NxR, xIx, NxN, NxN, xIx,  },
{  Nxx, Nxx, Nxx, Nxx, LxL, NxR, xIx, NxN, NxN, xIx,  },

{  xxx, xxx, xxx, xxx, xxA, xxR, xxA, xIx, xxN, xxN,  },
{  xxx, xxx, xxx, xxx, xxE, xxR, xxA, xIx, xxN, xIx,  },
{  xxx, xxx, xxx, xxx, xxE, xxR, xxE, xIx, xIx, xxE,  },
{  xxx, xxx, xxx, xxx, xxL, xxR, xxA, xIx, xxN, xIx,  },
{  xxx, xxx, xxx, xxx, xxL, xxR, xxL, xIx, xIx, xxL,  },

{  Nxx, Nxx, Nxx, Axx, AxA, NxR, NxN, NxN, NxN, NxN,  },
{  Nxx, Nxx, Nxx, Axx, NxE, NxR, NxN, NxN, NxN, NIx,  },
{  Nxx, Nxx, Nxx, Nxx, ExE, NxR, NxN, NxN, NxN, NIx,  },
{  Nxx, Nxx, Nxx, Axx, NxL, NxR, NxN, NxN, NxN, NIx,  },
{  Nxx, Nxx, Nxx, Nxx, LxL, NxR, NxN, NxN, NxN, NIx,  },

{ xxx, xxx, xxx, xxx, xxE, xxR, xxE, xxN, xxN, xxE,  },
{ xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xxL,  },
};

inline uint8    GetDeferredType(bidi_action a)          { return (a >> 4) & 0xF; }
inline uint8    GetResolvedType(bidi_action a)          { return a & 0xF; }
inline DirCode  EmbeddingDirection(int l)               { return l & 1 ? R : L; }


enum neutral_action
{
        
        nL = L,         
        En = 3 << 4,    
        Rn = R << 4,    
        Ln = L << 4,    
        In = (1<<8),    
        LnL = (1<<4)+L, 
};


void SetDeferredRunClass(Slot *s, Slot *sRun, int nval)
{
    if (!sRun || s == sRun) return;
    for (Slot *p = sRun; p != s; p = p->prev())
        if (p->getBidiClass() == WS) p->setBidiClass(nval | WSflag);
        else if (BaseClass(p) != BN) p->setBidiClass(nval | (p->getBidiClass() & WSflag));
}

void SetThisDeferredRunClass(Slot *s, Slot *sRun, int nval)
{
    if (!sRun) return;
    for (Slot *p = sRun, *e = s->prev(); p != e; p = p->prev())
        if (p->getBidiClass() == WS) p->setBidiClass(nval | WSflag);
        else if (BaseClass(p) != BN) p->setBidiClass(nval | (p->getBidiClass() & WSflag));
}

void resolveWeak(Slot *start, int sos, int eos)
{
    int state = (sos & 1) ? xr : xl;
    int cls;
    Slot *s = start;
    Slot *sRun = NULL;
    Slot *sLast = s;

    for ( ; s; s = s->prev())
    {
        sLast = s;
        cls = BaseClass(s);
        switch (cls)
        {
        case BN :
            if (s == start) start = s->prev();  
            continue;
        case LRI :
        case RLI :
        case FSI :
        case PDI :
            {
                Slot *snext = s->prev();
                if (snext && snext->getBidiClass() == NSM)
                    snext->setBidiClass(ON);
                s->setBidiClass(ON | WSflag);
            }
            break;

        case NSM :
            if (s == start)
            {
                cls = EmbeddingDirection(sos);
                s->setBidiClass(cls);
            }
            break;
        }
        
        bidi_action action = actionWeak[state][bidi_class_map[cls]];
        int clsRun = GetDeferredType(action);
        if (clsRun != XX)
        {
            SetDeferredRunClass(s, sRun, clsRun);
            sRun = NULL;
        }
        int clsNew = GetResolvedType(action);
        if (clsNew != XX)
            s->setBidiClass(clsNew);
        if (!sRun && (IX & action))
            sRun = s;
        state = stateWeak[state][bidi_class_map[cls]];
    }

    cls = EmbeddingDirection(eos);
    int clsRun = GetDeferredType(actionWeak[state][bidi_class_map[cls]]);
    if (clsRun != XX)
        SetThisDeferredRunClass(sLast, sRun, clsRun);
}

void processParens(Slot *s, Segment *seg, uint8 aMirror, int level, BracketPairStack &stack)
{
    uint8 mask = 0;
    int8 lastDir = -1;
    BracketPair *p;
    for ( ; s; s = s->prev())       
    {
        uint16 ogid = seg->glyphAttr(s->gid(), aMirror);
        int cls = BaseClass(s);
        
        switch(cls)
        {
        case OPP :
            stack.orin(mask);
            stack.push(ogid, s, lastDir, lastDir != CPP);
            mask = 0;
            lastDir = OPP;
            break;
        case CPP :
            stack.orin(mask);
            p = stack.scan(s->gid());
            if (!p) break;
            mask = 0;
            stack.close(p, s);
            lastDir = CPP;
            break;
        case L :
            lastDir = L;
            mask |= 1;
            break;
        case R :
        case AL :
        case AN :
        case EN :
            lastDir = R;
            mask |= 2;
        }
    }
    for (p = stack.start(); p; p =p->next())      
    {
        if (p->close() && p->mask())
        {
            int dir = (level & 1) + 1;
            if (p->mask() & dir)
            { }
            else if (p->mask() & (1 << (~level & 1)))  
            {
                int ldir = p->before();
                if ((p->before() == OPP || p->before() == CPP) && p->prev())
                {
                    for (BracketPair *q = p->prev(); q; q = q->prev())
                    {
                        ldir = q->open()->getBidiClass();
                        if (ldir < 3) break;
                        ldir = q->before();
                        if (ldir < 3) break;
                    }
                    if (ldir > 2) ldir = 0;
                }
                if (ldir > 0 && (ldir - 1) != (level & 1))     
                    dir = (~level & 1) + 1;
            }
            p->open()->setBidiClass(dir);
            p->close()->setBidiClass(dir);
        }
    }
    stack.clear();
}

int GetDeferredNeutrals(int action, int level)
{
        action = (action >> 4) & 0xF;
        if (action == (En >> 4))
            return EmbeddingDirection(level);
        else
            return action;
}

int GetResolvedNeutrals(int action)
{
        return action & 0xF;
}


enum neutral_state
{
        
        r,  
        l,  
        rn, 
        ln, 
        a,  
        na, 
} ;

const uint8 neutral_class_map[] = { 0, 1, 2, 0, 4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int actionNeutrals[][5] =
{

{       In,  0,  0,  0,  0, },  
{       In,  0,  0,  0,  L, },  

{       In, En, Rn, Rn, Rn, },  
{       In, Ln, En, En, LnL, }, 

{       In,  0,  0,  0,  L, },  
{       In, En, Rn, Rn, En, },  
} ;

const int stateNeutrals[][5] =
{

{       rn, l,  r,      r,      r, },           
{       ln, l,  r,      a,      l, },           

{       rn, l,  r,      r,      r, },           
{       ln, l,  r,      a,      l, },           

{       na, l,  r,      a,      l, },           
{       na, l,  r,      a,      l, },           
} ;

void resolveNeutrals(Slot *s, int baseLevel, int sos, int eos)
{
    int state = (sos & 1) ? r : l;
    int cls;
    Slot *sRun = NULL;
    Slot *sLast = s;
    int level = baseLevel;

    for ( ; s; s = s->prev())
    {
        sLast = s;
        cls = BaseClass(s);
        switch (cls)
        {
        case BN :
            continue;
        case LRI :
        case RLI :
        case FSI :
            s->setBidiClass(BN | WSflag);
            continue;

        default :
            int action = actionNeutrals[state][neutral_class_map[cls]];
            int clsRun = GetDeferredNeutrals(action, level);
            if (clsRun != N)
            {
                SetDeferredRunClass(s, sRun, clsRun);
                sRun = NULL;
            }
            int clsNew = GetResolvedNeutrals(action);
            if (clsNew != N)
                s->setBidiClass(clsNew);
            if (!sRun && (action & In))
                sRun = s;
            state = stateNeutrals[state][neutral_class_map[cls]];
        }
    }
    cls = EmbeddingDirection(eos);
    int clsRun = GetDeferredNeutrals(actionNeutrals[state][neutral_class_map[cls]], level);
    if (clsRun != N)
        SetThisDeferredRunClass(sLast, sRun, clsRun);
}

const int addLevel[][4] =
{
        
      { 0,    1,      2,      2, },   
      { 1,    0,      1,      1, },   

};

void resolveImplicit(Slot *s, Segment *seg, uint8 aMirror)
{
    bool rtl = seg->dir() & 1;
    int level = rtl;
    Slot *slast = 0;
    for ( ; s; s = s->next())
    {
        int cls = BaseClass(s);
        s->prev(slast);         
        slast = s;
        if (cls == AN)
            cls = AL;   
        if (cls < 5 && cls > 0)
        {
            level = s->getBidiLevel();
            level += addLevel[level & 1][cls - 1];
            s->setBidiLevel(level);
        }
        if (aMirror)
        {
            int hasChar = seg->glyphAttr(s->gid(), aMirror + 1);
            if ( ((level & 1) && (!(seg->dir() & 4) || !hasChar)) 
              || ((rtl ^ (level & 1)) && (seg->dir() & 4) && hasChar) )
            {
                unsigned short g = seg->glyphAttr(s->gid(), aMirror);
                if (g) s->setGlyph(seg, g);
            }
        }
    }
}

void resolveWhitespace(int baseLevel, Slot *s)
{
    for ( ; s; s = s->prev())
    {
        int8 cls = s->getBidiClass();
        if (cls == WS || cls & WSflag)
            s->setBidiLevel(baseLevel);
        else if (cls != BN)
            break;
    }
}






inline
Slot * join(int level, Slot * a, Slot * b)
{
    if (!a) return b;
    if (level & 1)  { Slot * const t = a; a = b; b = t; }
    Slot * const t = b->prev();
    a->prev()->next(b); b->prev(a->prev()); 
    t->next(a); a->prev(t);                 
    return a;
}







Slot * span(Slot * & cs, const bool rtl)
{
    Slot * r = cs, * re = cs; cs = cs->next();
    if (rtl)
    {
        Slot * t = r->next(); r->next(r->prev()); r->prev(t);
        for (int l = r->getBidiLevel(); cs && (l == cs->getBidiLevel() || cs->getBidiClass() == BN); cs = cs->prev())
        {
            re = cs;
            t = cs->next(); cs->next(cs->prev()); cs->prev(t);
        }
        r->next(re);
        re->prev(r);
        r = re;
    }
    else
    {
        for (int l = r->getBidiLevel(); cs && (l == cs->getBidiLevel() || cs->getBidiClass() == BN); cs = cs->next())
            re = cs;
        r->prev(re);
        re->next(r);
    }
    if (cs) cs->prev(0);
    return r;
}

inline int getlevel(const Slot *cs, const int level)
{
    while (cs && cs->getBidiClass() == BN)
    { cs = cs->next(); }
    if (cs)
        return cs->getBidiLevel();
    else
        return level;
}

Slot *resolveOrder(Slot * & cs, const bool reordered, const int level)
{
    Slot * r = 0;
    int ls;
    while (cs && level <= (ls = getlevel(cs, level) - reordered))
    {
        r = join(level, r, level < ls
                                ? resolveOrder(cs, reordered, level+1) 
                                : span(cs, level & 1));
    }
    return r;
}
