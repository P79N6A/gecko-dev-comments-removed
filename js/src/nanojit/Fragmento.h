








































#ifndef __nanojit_Fragmento__
#define __nanojit_Fragmento__

#ifdef AVMPLUS_VERBOSE
extern void drawTraceTrees(Fragmento *frago, FragmentMap * _frags, avmplus::AvmCore *core, char *fileName);
#endif

namespace nanojit
{
	struct GuardRecord;
	class Assembler;
	
    struct PageHeader
    {
        struct Page *next;
    };
    struct Page: public PageHeader
    {
        union {
            LIns lir[(NJ_PAGE_SIZE-sizeof(PageHeader))/sizeof(LIns)];
            NIns code[(NJ_PAGE_SIZE-sizeof(PageHeader))/sizeof(NIns)];
        };
    };
    struct AllocEntry : public GCObject
    {
        Page *page;
        uint32_t allocSize;
    };
	typedef avmplus::List<AllocEntry*,avmplus::LIST_NonGCObjects>	AllocList;

	typedef avmplus::GCSortedMap<const void*, uint32_t, avmplus::LIST_NonGCObjects> BlockSortedMap;
	class BlockHist: public BlockSortedMap
	{
	public:
		BlockHist(GC*gc) : BlockSortedMap(gc)
		{
		}
		uint32_t count(const void *p) {
			uint32_t c = 1+get(p);
			put(p, c);
			return c;
		}
	};

	struct fragstats;
	



	class Fragmento : public GCFinalizedObject
	{
		public:
			Fragmento(AvmCore* core, uint32_t cacheSizeLog2);
			~Fragmento();

			void		addMemory(void* firstPage, uint32_t pageCount);  
			Assembler*	assm();
			AvmCore*	core();
			Page*		pageAlloc();
			void		pageFree(Page* page);
			
            Fragment*   getLoop(const void* ip);
            Fragment*   getAnchor(const void* ip);
			void        clearFrags();	
            Fragment*   getMerge(GuardRecord *lr, const void* ip);
            Fragment*   createBranch(GuardRecord *lr, const void* ip);
            Fragment*   newFrag(const void* ip);
            Fragment*   newBranch(Fragment *from, const void* ip);

            verbose_only ( uint32_t pageCount(); )
			verbose_only ( void dumpStats(); )
			verbose_only ( void dumpRatio(const char*, BlockHist*);)
			verbose_only ( void dumpFragStats(Fragment*, int level, fragstats&); )
			verbose_only ( void countBlock(BlockHist*, const void* pc); )
			verbose_only ( void countIL(uint32_t il, uint32_t abc); )
			verbose_only( void addLabel(Fragment* f, const char *prefix, int id); )
			
			
			struct 
			{
				uint32_t	pages;					
				uint32_t	freePages;				
				uint32_t	maxPageUse;				
				uint32_t	flushes, ilsize, abcsize, compiles, totalCompiles;
			}
			_stats;

			verbose_only( DWB(BlockHist*)		enterCounts; )
			verbose_only( DWB(BlockHist*)		mergeCounts; )
			verbose_only( DWB(LabelMap*)        labels; )
			
    		#ifdef AVMPLUS_VERBOSE
    		void	drawTrees(char *fileName);
            #endif
			
			uint32_t cacheUsed() const { return (_stats.pages-_stats.freePages)<<NJ_LOG2_PAGE_SIZE; }
			uint32_t cacheUsedMax() const { return (_stats.maxPageUse)<<NJ_LOG2_PAGE_SIZE; }
		private:
			void		pagesGrow(int32_t count);
			void		trackFree(int32_t delta);

			AvmCore*			_core;
			DWB(Assembler*)		_assm;
			DWB(FragmentMap*)	_frags;		
			Page*			_pageList;

			
			AllocList	_allocList;
			GCHeap*		_gcHeap;

			const uint32_t _max_pages;
			uint32_t _pagesGrowth;
	};

	enum TraceKind {
		LoopTrace,
		BranchTrace,
		MergeTrace
	};
	
	






	class Fragment : public GCFinalizedObject
	{
		public:
			Fragment(const void*);
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
			void			releaseLirBuffer();
			void			releaseCode(Fragmento* frago);
			void			releaseTreeMem(Fragmento* frago);
			bool			isAnchor() { return anchor == this; }
			bool			isRoot() { return root == this; }
            void            onDestroy();
			
			verbose_only( uint32_t		_called; )
			verbose_only( uint32_t		_native; )
            verbose_only( uint32_t      _exitNative; )
			verbose_only( uint32_t		_lir; )
			verbose_only( uint32_t		_lirbytes; )
			verbose_only( const char*	_token; )
            verbose_only( uint64_t      traceTicks; )
            verbose_only( uint64_t      interpTicks; )
			verbose_only( DWB(Fragment*) eot_target; )
			verbose_only( uint32_t		sid;)
			verbose_only( uint32_t		compileNbr;)

            DWB(Fragment*) treeBranches;
            DWB(Fragment*) branches;
            DWB(Fragment*) nextbranch;
            DWB(Fragment*) anchor;
            DWB(Fragment*) root;
            DWB(Fragment*) parent;
            DWB(Fragment*) first;
            DWB(Fragment*) peer;
			DWB(BlockHist*) mergeCounts;
            DWB(LirBuffer*) lirbuf;
			LIns*			lastIns;
			LIns*		spawnedFrom;
			GuardRecord*	outbound;
			
			TraceKind kind;
			const void* ip;
			uint32_t guardCount;
            uint32_t xjumpCount;
            int32_t blacklistLevel;
            NIns* fragEntry;
			int32_t calldepth;
			void* vmprivate;
			
		private:
			NIns*			_code;		
			GuardRecord*	_links;		
			int32_t			_hits;
			Page*			_pages;		
	};
}
#endif 
