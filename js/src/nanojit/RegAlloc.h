





































#ifndef __nanojit_RegAlloc__
#define __nanojit_RegAlloc__


namespace nanojit
{
	inline RegisterMask rmask(Register r)
	{
		return 1 << r;
	}

	class RegAlloc
	{
		public:
			RegAlloc() {}
			void	clear();
			bool	isFree(Register r); 
			void	addFree(Register r);
			void	removeFree(Register r);
			void	addActive(Register r, LIns* ins);
			void	removeActive(Register r);
			LIns*	getActive(Register r); 
			void	retire(Register r);

			debug_only( uint32_t	countFree(); )
			debug_only( uint32_t	countActive(); )
			debug_only( void		checkCount(); )
			debug_only( bool		isConsistent(Register r, LIns* v); )
			debug_only( uint32_t	count; )
			debug_only( RegisterMask managed; )    

			LIns*	active[NJ_MAX_REGISTERS];  
			RegisterMask	free;
			RegisterMask	used;

			verbose_only( static void formatRegisters(RegAlloc& regs, char* s, LirNameMap*); )

			DECLARE_PLATFORM_REGALLOC()
	};
}
#endif 
