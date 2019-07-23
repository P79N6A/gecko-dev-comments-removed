








































#include "nanojit.h"
#undef MEMORY_INFO

namespace nanojit
{
    #ifdef FEATURE_NANOJIT

    using namespace avmplus;

    static uint32_t calcSaneCacheSize(uint32_t in)
    {
        if (in < uint32_t(NJ_LOG2_PAGE_SIZE)) return NJ_LOG2_PAGE_SIZE;    
        if (in > 32) return 32;    
        return in;
    }

    


    Fragmento::Fragmento(AvmCore* core, LogControl* logc, uint32_t cacheSizeLog2, CodeAlloc* codeAlloc)
        :
#ifdef NJ_VERBOSE
          enterCounts(NULL),
          mergeCounts(NULL),
          labels(NULL),
#endif
          _core(core),
          _codeAlloc(codeAlloc),
          _frags(core->GetGC()),
          _max_pages(1 << (calcSaneCacheSize(cacheSizeLog2) - NJ_LOG2_PAGE_SIZE)),
          _pagesGrowth(1)
    {
#ifdef _DEBUG
        {
            
            
            NanoStaticAssert((LIR_lt ^ 3) == LIR_ge);
            NanoStaticAssert((LIR_le ^ 3) == LIR_gt);
            NanoStaticAssert((LIR_ult ^ 3) == LIR_uge);
            NanoStaticAssert((LIR_ule ^ 3) == LIR_ugt);
            NanoStaticAssert((LIR_flt ^ 3) == LIR_fge);
            NanoStaticAssert((LIR_fle ^ 3) == LIR_fgt);

            
            uint32_t count = 0;
#define OPDEF(op, number, operands, repkind) \
        NanoAssertMsg(LIR_##op == count++, "misnumbered opcode");
#define OPDEF64(op, number, operands, repkind) \
        OPDEF(op, number, operands, repkind)
#include "LIRopcode.tbl"
#undef OPDEF
#undef OPDEF64
        }
#endif

#ifdef MEMORY_INFO
        _allocList.set_meminfo_name("Fragmento._allocList");
#endif
        NanoAssert(_max_pages > _pagesGrowth); 
        verbose_only( enterCounts = NJ_NEW(core->gc, BlockHist)(core->gc); )
        verbose_only( mergeCounts = NJ_NEW(core->gc, BlockHist)(core->gc); )

        memset(&_stats, 0, sizeof(_stats));
    }

    Fragmento::~Fragmento()
    {
        clearFrags();
#if defined(NJ_VERBOSE)
        NJ_DELETE(enterCounts);
        NJ_DELETE(mergeCounts);
#endif
    }


    
    
    void Fragmento::clearFragment(Fragment* f)
    {
        Fragment *peer = f->peer;
        while (peer) {
            Fragment *next = peer->peer;
            peer->releaseTreeMem(_codeAlloc);
            NJ_DELETE(peer);
            peer = next;
        }
        f->releaseTreeMem(_codeAlloc);
        NJ_DELETE(f);
    }

    void Fragmento::clearFrags()
    {
        while (!_frags.isEmpty()) {
            clearFragment(_frags.removeLast());
        }

        verbose_only( enterCounts->clear();)
        verbose_only( mergeCounts->clear();)
        verbose_only( _stats.flushes++ );
        verbose_only( _stats.compiles = 0 );
        
    }

    AvmCore* Fragmento::core()
    {
        return _core;
    }

    Fragment* Fragmento::getAnchor(const void* ip)
    {
        Fragment *f = newFrag(ip);
        Fragment *p = _frags.get(ip);
        if (p) {
            f->first = p;
            
            Fragment* next;
            while ((next = p->peer) != NULL)
                p = next;
            p->peer = f;
        } else {
            f->first = f;
            _frags.put(ip, f); 
        }
        f->anchor = f;
        f->root = f;
        f->kind = LoopTrace;
        verbose_only( addLabel(f, "T", _frags.size()); )
        return f;
    }

    Fragment* Fragmento::getLoop(const void* ip)
    {
        return _frags.get(ip);
    }

#ifdef NJ_VERBOSE
    void Fragmento::addLabel(Fragment *f, const char *prefix, int id)
    {
        char fragname[20];
        sprintf(fragname,"%s%d", prefix, id);
        labels->add(f, sizeof(Fragment), 0, fragname);
    }
#endif

    Fragment *Fragmento::createBranch(SideExit* exit, const void* ip)
    {
        Fragment *f = newBranch(exit->from, ip);
        f->kind = BranchTrace;
        f->treeBranches = f->root->treeBranches;
        f->root->treeBranches = f;
        return f;
    }

    
    
    
    Fragment::Fragment(const void* _ip)
        :
#ifdef NJ_VERBOSE
          _called(0),
          _native(0),
          _exitNative(0),
          _lir(0),
          _lirbytes(0),
          _token(NULL),
          traceTicks(0),
          interpTicks(0),
          eot_target(NULL),
          sid(0),
          compileNbr(0),
#endif
          treeBranches(NULL),
          branches(NULL),
          nextbranch(NULL),
          anchor(NULL),
          root(NULL),
          parent(NULL),
          first(NULL),
          peer(NULL),
          lirbuf(NULL),
          lastIns(NULL),
          spawnedFrom(NULL),
          kind(LoopTrace),
          ip(_ip),
          guardCount(0),
          xjumpCount(0),
          recordAttempts(0),
          blacklistLevel(0),
          fragEntry(NULL),
          loopEntry(NULL),
          vmprivate(NULL),
          codeList(0),
          _code(NULL),
          _hits(0)
    {
    }

    Fragment::~Fragment()
    {
        onDestroy();
    }

    void Fragment::blacklist()
    {
        blacklistLevel++;
        _hits = -(1<<blacklistLevel);
    }

    Fragment *Fragmento::newFrag(const void* ip)
    {
        GC *gc = _core->gc;
        Fragment *f = NJ_NEW(gc, Fragment)(ip);
        f->blacklistLevel = 5;
        return f;
    }

    Fragment *Fragmento::newBranch(Fragment *from, const void* ip)
    {
        Fragment *f = newFrag(ip);
        f->anchor = from->anchor;
        f->root = from->root;
        f->xjumpCount = from->xjumpCount;
        


        
        if (!from->branches) {
            from->branches = f;
        } else {
            Fragment *p = from->branches;
            while (p->nextbranch != 0)
                p = p->nextbranch;
            p->nextbranch = f;
        }
        return f;
    }

    void Fragment::releaseLirBuffer()
    {
        lastIns = 0;
    }

    void Fragment::releaseCode(CodeAlloc *codeAlloc)
    {
        _code = 0;
        codeAlloc->freeAll(codeList);
    }

    void Fragment::releaseTreeMem(CodeAlloc *codeAlloc)
    {
        releaseLirBuffer();
        releaseCode(codeAlloc);

        
        Fragment* branch = branches;
        while(branch)
        {
            Fragment* next = branch->nextbranch;
            branch->releaseTreeMem(codeAlloc);  
            NJ_DELETE(branch);
            branch = next;
        }
    }
    #endif 
}


