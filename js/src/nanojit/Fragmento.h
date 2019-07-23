





































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__



namespace nanojit
{
	struct GuardRecord;
	class Assembler;
	
    struct PageHeader
    {
        struct Page *next;
        verbose_only (int seq;) 
    };
    struct Page: public PageHeader
    {
        union {
            LIns lir[(NJ_PAGE_SIZE-sizeof(PageHeader))/sizeof(LIns)];
            NIns code[(NJ_PAGE_SIZE-sizeof(PageHeader))/sizeof(NIns)];
        };
    };
	typedef avmplus::List<Page*,avmplus::LIST_NonGCObjects>	AllocList;

	typedef avmplus::SortedMap<const void*, uint32_t, avmplus::LIST_NonGCObjects> BlockSortedMap;
	class BlockHist: public BlockSortedMap
	{
	public:
		BlockHist(GC*gc): BlockSortedMap(gc)
		{
		}
		uint32_t count(const void *p) {
			uint32_t c = 1+get(p);
			put(p, c);
			return c;
		}
	};

	



	class Fragmento : public GCFinalizedObject
	{
		public:
			Fragmento(AvmCore* core);
			~Fragmento();

			void		addMemory(void* firstPage, uint32_t pageCount);  
			Assembler*	assm();
			AvmCore*	core();
			Page*		pageAlloc();
			void		pageFree(Page* page);
			
            Fragment*	getLoop(const avmplus::InterpState &is);
			void		clearFrags();	
            Fragment*	getMerge(GuardRecord *lr, const avmplus::InterpState &is);
            Fragment*   createBranch(GuardRecord *lr, const avmplus::InterpState &is);
            Fragment*   newFrag(const avmplus::InterpState &is);
            Fragment*   newBranch(Fragment *from, const avmplus::InterpState &is);

            verbose_only ( uint32_t pageCount(); )
			verbose_only ( void dumpStats(); )
			verbose_only ( void dumpRatio(const char*, BlockHist*);)
			verbose_only ( void dumpFragStats(Fragment*, int level, 
				int& size, uint64_t &dur, uint64_t &interpDur); )
				verbose_only ( void countBlock(BlockHist*, avmplus::FOpcodep pc); )
			verbose_only ( void countIL(uint32_t il, uint32_t abc); )
			verbose_only( void addLabel(Fragment* f, const char *prefix, int id); )

			
			struct 
			{
				uint32_t	pages;					
				uint32_t	flushes, ilsize, abcsize, compiles, totalCompiles, freePages;
			}
			_stats;

			verbose_only( DWB(BlockHist*)		enterCounts; )
			verbose_only( DWB(BlockHist*)		mergeCounts; )
			verbose_only( DWB(LabelMap*)        labels; )

		private:
			void		pagesGrow(int32_t count);

			AvmCore*			_core;
			DWB(Assembler*)		_assm;
			DWB(FragmentMap*)	_frags;		
			Page*			_pageList;

			
			AllocList	_allocList;
			GCHeap*		_gcHeap;
	};

    struct SideExit
    {
		int32_t f_adj;
        int32_t ip_adj;
		int32_t sp_adj;
		int32_t rp_adj;
        Fragment *target;
		int32_t calldepth;
		verbose_only( uint32_t sid; )
		verbose_only(Fragment *from;)
    };

	enum TraceKind {
		LoopTrace,
		BranchTrace,
		MergeTrace
	};
	
	






	class Fragment : public GCFinalizedObject
	{
		public:
			Fragment(FragID);
			~Fragment();

			NIns*			code()							{ return _code; }
			void			setCode(NIns* codee, Page* pages) { _code = codee; _pages = pages; }
			GuardRecord*	links()							{ return _links; }
			int32_t&		hits()							{ return _hits; }
            void            blacklist();
			bool			isBlacklisted()		{ return _hits < 0; }
			void			resetLinks();
			void			addLink(GuardRecord* lnk);
			void			removeLink(GuardRecord* lnk);
			void			link(Assembler* assm);
			void			linkBranches(Assembler* assm);
			void			unlink(Assembler* assm);
			void			unlinkBranches(Assembler* assm);
			debug_only( bool hasOnlyTreeLinks(); )
			void			removeIntraLinks();
            void            removeExit(Fragment *target);
			void			releaseLirBuffer();
			void			releaseCode(Fragmento* frago);
			void			releaseTreeMem(Fragmento* frago);
			bool			isAnchor() { return anchor == this; }
			bool			isRoot() { return root == this; }
			
			verbose_only( uint32_t		_called; )
			verbose_only( uint32_t		_native; )
            verbose_only( uint32_t      _exitNative; )
			verbose_only( uint32_t		_lir; )
			verbose_only( const char*	_token; )
            verbose_only( uint64_t      traceTicks; )
            verbose_only( uint64_t      interpTicks; )
            verbose_only( int32_t line; )
            verbose_only( DRCWB(avmplus::String *)file; )
			verbose_only( DWB(Fragment*) eot_target; )
			verbose_only( uint32_t mergeid;)
			verbose_only( uint32_t		sid;)
			verbose_only( uint32_t		compileNbr;)

            DWB(Fragment*) treeBranches;
            DWB(Fragment*) branches;
            DWB(Fragment*) nextbranch;
            DWB(Fragment*) anchor;
            DWB(Fragment*) root;
			DWB(BlockHist*) mergeCounts;
            DWB(LirBuffer*) lirbuf;
			LIns*			lastIns;
			LIns*		spawnedFrom;
			GuardRecord*	outbound;
			
			TraceKind kind;
			const FragID frid;
			uint32_t guardCount;
            uint32_t xjumpCount;
            int32_t blacklistLevel;
            NIns* fragEntry;
            LInsp param0,param1,sp,rp;
			int32_t calldepth;
			
		private:
			NIns*			_code;		
			GuardRecord*	_links;		
			int32_t			_hits;
			Page*			_pages;		
	};
	
#ifdef NJ_VERBOSE
	inline int nbr(LInsp x) 
	{
        Page *p = x->page();
        return (p->seq * NJ_PAGE_SIZE + (intptr_t(x)-intptr_t(p))) / sizeof(LIns);
	}
#else
    inline int nbr(LInsp x)
    {
        return int(x) & (NJ_PAGE_SIZE-1);
    }
#endif
}
#endif 
