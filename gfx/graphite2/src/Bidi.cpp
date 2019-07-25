

























#include "Main.h"
#include "Slot.h"
#include "Segment.h"

using namespace graphite2;

enum DirCode {  
        Unk        = -1,
        N          =  0,   
        L          =  1,   
        R          =  2,   
        AL         =  3,   
        EN         =  4,   
        ES         =  5,   
        ET         =  6,   
        AN         =  7,   
        CS         =  8,   
        WS         =  9,   
        BN         = 10,   

        LRO        = 11,   
        RLO        = 12,   
        LRE        = 13,   
        RLE        = 14,   
        PDF        = 15,   
        NSM        = 16,   

        ON = N
};

enum DirMask {
    Nmask = 1,
    Lmask = 2,
    Rmask = 4,
    ALmask = 8,
    ENmask = 0x10,
    ESmask = 0x20,
    ETmask = 0x40,
    ANmask = 0x80,
    CSmask = 0x100,
    WSmask = 0x200,
    BNmask = 0x400,
    LROmask = 0x800,
    RLOmask = 0x1000,
    LREmask = 0x2000,
    RLEmask = 0x4000,
    PDFmask = 0x8000,
    NSMmask = 0x10000
};

unsigned int bidi_class_map[] = { 0, 1, 2, 5, 4, 8, 9, 3, 7, 0, 0, 0, 0, 0, 0, 0, 6 };

#define MAX_LEVEL 61

Slot *resolveExplicit(int level, int dir, Slot *s, int nNest = 0)
{
    int nLastValid = nNest;
    Slot *res = NULL;
    for ( ; s && !res; s = s->next())
    {
        int cls = s->getBidiClass();
        switch(cls)
        {
        case LRO:
        case LRE:
            nNest++;
            if (level & 1)
                s->setBidiLevel(level + 1);
            else
                s->setBidiLevel(level + 2);
            if (s->getBidiLevel() > MAX_LEVEL)
                s->setBidiLevel(level);
            else
            {
                s = resolveExplicit(s->getBidiLevel(), (cls == LRE ? N : L), s->next(), nNest);
                nNest--;
                if (s) continue; else break;
            }
            cls = BN;
            s->setBidiClass(cls);
            break;

        case RLO:
        case RLE:
            nNest++;
            if (level & 1)
                s->setBidiLevel(level + 2);
            else
                s->setBidiLevel(level + 1);
            if (s->getBidiLevel() > MAX_LEVEL)
                s->setBidiLevel(level);
            else
            {
                s = resolveExplicit(s->getBidiLevel(), (cls == RLE ? N : R), s->next(), nNest);
                nNest--;
                if (s) continue; else break;
            }
            cls = BN;
            s->setBidiClass(cls);
            break;

        case PDF:
            cls = BN;
            s->setBidiClass(cls);
            if (nNest)
            {
                if (nLastValid < nNest)
                    --nNest;
                else
                    res = s;
            }
            break;
        }

        if (dir != N)
            cls = dir;
        if (s)
        {
            s->setBidiLevel(level);
            if (s->getBidiClass() != BN)
                s->setBidiClass(cls);
        }
        else
            break;
    }
    return res;
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

enum bidi_state_mask
{
    xamask = 1,
    xrmask = 2,
    xlmask = 4,
    aomask = 8,
    romask = 0x10,
    lomask = 0x20,
    rtmask = 0x40,
    ltmask = 0x80,
    cnmask = 0x100,
    ramask = 0x200,
    remask = 0x400,
    lamask = 0x800,
    lemask = 0x1000,
    acmask = 0x2000,
    rcmask = 0x4000,
    rsmask = 0x8000,
    lcmask = 0x10000,
    lsmask = 0x20000,
    retmask = 0x40000,
    letmask = 0x80000
};

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

{  ro, xl, xr, ra, re, xa,ret, ro, ro,ret,  },
{  lo, xl, xr, la, le, xa,let, lo, lo,let,  },


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
inline bool     IsDeferredState(bidi_state a)           { return (1 << a) & (rtmask | ltmask | acmask | rcmask | rsmask | lcmask | lsmask); }
inline bool     IsModifiedClass(DirCode a)              { return (1 << a) & (ALmask | NSMmask | ESmask | CSmask | ETmask | ENmask); }

void SetDeferredRunClass(Slot *s, Slot *sRun, int nval)
{
    if (!sRun || s == sRun) return;
    for (Slot *p = s->prev(); p != sRun; p = p->prev())
        p->setBidiClass(nval);
}

void resolveWeak(int baseLevel, Slot *s)
{
    int state = (baseLevel & 1) ? xr : xl;
    int cls;
    int level = baseLevel;
    Slot *sRun = NULL;
    Slot *sLast = s;

    for ( ; s; s = s->next())
    {
        sLast = s;
        cls = s->getBidiClass();
        if (cls == BN)
        {
            s->setBidiLevel(level);
            if (!s->next() && level != baseLevel)
                s->setBidiClass(EmbeddingDirection(level));
            else if (s->next() && level != s->next()->getBidiLevel() && s->next()->getBidiClass() != BN)
            {
                int newLevel = s->next()->getBidiLevel();
                if (level > newLevel)
                    newLevel = level;
                s->setBidiLevel(newLevel);
                s->setBidiClass(EmbeddingDirection(newLevel));
                level  = s->next()->getBidiLevel();
            }
            else
                continue;
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
            sRun = s->prev();
        state = stateWeak[state][bidi_class_map[cls]];
    }

    cls = EmbeddingDirection(level);
    int clsRun = GetDeferredType(actionWeak[state][bidi_class_map[cls]]);
    if (clsRun != XX)
        SetDeferredRunClass(sLast, sRun, clsRun);
}


enum neutral_action
{
        
        nL = L,         
        En = 3 << 4,    
        Rn = R << 4,    
        Ln = L << 4,    
        In = (1<<8),    
        LnL = (1<<4)+L, 
};

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
        action = action & 0xF;
        if (action == In)
            return 0;
        else
            return action;
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

const uint8 neutral_class_map[] = { 0, 1, 2, 0, 4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0 };

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

void resolveNeutrals(int baseLevel, Slot *s)
{
    int state = baseLevel ? r : l;
    int cls;
    Slot *sRun = NULL;
    Slot *sLast = s;
    int level = baseLevel;

    for ( ; s; s = s->next())
    {
        sLast = s;
        cls = s->getBidiClass();
        if (cls == BN)
        {
            if (sRun)
                sRun = sRun->prev();
            continue;
        }

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
        state = stateNeutrals[state][neutral_class_map[cls]];
        level = s->getBidiLevel();
    }
    cls = EmbeddingDirection(level);
    int clsRun = GetDeferredNeutrals(actionNeutrals[state][neutral_class_map[cls]], level);
    if (clsRun != N)
        SetDeferredRunClass(sLast, sRun, clsRun);
}

const int addLevel[][4] =
{
                
                                                
      { 0,    1,      2,      2, },   
      { 1,    0,      1,      1, },   

};

void resolveImplicit(Slot *s, Segment *seg, uint8 aMirror)
{
    bool rtl = seg->dir() & 1;
    for ( ; s; s = s->next())
    {
        int cls = s->getBidiClass();
        if (cls == BN)
            continue;
        else if (cls == AN)
            cls = AL;
        if (cls < 5 && cls > 0)
        {
            int level = s->getBidiLevel();
            level += addLevel[level & 1][cls - 1];
            s->setBidiLevel(level);
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
}

void resolveWhitespace(int baseLevel, Segment *seg, uint8 aBidi, Slot *s)
{
    for ( ; s; s = s->prev())
    {
        int cls = seg->glyphAttr(s->gid(), aBidi);
        if (cls == WS)
            s->setBidiLevel(baseLevel);
        else
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
        for (int l = r->getBidiLevel(); cs && l == cs->getBidiLevel(); cs = cs->prev())
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
        for (int l = r->getBidiLevel(); cs && l == cs->getBidiLevel(); cs = cs->next())
            re = cs;
        r->prev(re);
        re->next(r);
    }
    if (cs) cs->prev(0);
    return r;
}


Slot *resolveOrder(Slot * & cs, const bool reordered, const int level)
{
    Slot * r = 0;
    int ls;
    while (cs && level <= (ls = cs->getBidiLevel() - reordered))
    {
        r = join(level, r, level >= ls
                                ? span(cs, level & 1)
                                : resolveOrder(cs, reordered, level+1));
    }
    return r;
}

