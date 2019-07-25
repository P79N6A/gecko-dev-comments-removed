
























#include "offsets.h"
#include "regs.h"
#include "unwind_i.h"

enum ia64_script_insn_opcode
  {
    IA64_INSN_INC_PSP,		
    IA64_INSN_LOAD_PSP,		
    IA64_INSN_ADD_PSP,		
    IA64_INSN_ADD_PSP_NAT,	
    IA64_INSN_ADD_SP,		
    IA64_INSN_ADD_SP_NAT,	
    IA64_INSN_MOVE,		
    IA64_INSN_MOVE_NAT,		
    IA64_INSN_MOVE_NO_NAT,	
    IA64_INSN_MOVE_STACKED,	
    IA64_INSN_MOVE_STACKED_NAT,	
    IA64_INSN_MOVE_SCRATCH,	
    IA64_INSN_MOVE_SCRATCH_NAT,	
    IA64_INSN_MOVE_SCRATCH_NO_NAT 
  };

#ifdef HAVE___THREAD
static __thread struct ia64_script_cache ia64_per_thread_cache =
  {
#ifdef HAVE_ATOMIC_OPS_H
    .busy = AO_TS_INITIALIZER
#else
    .lock = PTHREAD_MUTEX_INITIALIZER
#endif
  };
#endif

static inline unw_hash_index_t
hash (unw_word_t ip)
{
  
# define magic	((unw_word_t) 0x9e3779b97f4a7c16ULL)

  return (ip >> 4) * magic >> (64 - IA64_LOG_UNW_HASH_SIZE);
}

static inline long
cache_match (struct ia64_script *script, unw_word_t ip, unw_word_t pr)
{
  if (ip == script->ip && ((pr ^ script->pr_val) & script->pr_mask) == 0)
    return 1;
  return 0;
}

static inline void
flush_script_cache (struct ia64_script_cache *cache)
{
  int i;

  cache->lru_head = IA64_UNW_CACHE_SIZE - 1;
  cache->lru_tail = 0;

  for (i = 0; i < IA64_UNW_CACHE_SIZE; ++i)
    {
      if (i > 0)
	cache->buckets[i].lru_chain = (i - 1);
      cache->buckets[i].coll_chain = -1;
      cache->buckets[i].ip = 0;
    }
  for (i = 0; i<IA64_UNW_HASH_SIZE; ++i)
    cache->hash[i] = -1;
}

static inline struct ia64_script_cache *
get_script_cache (unw_addr_space_t as, intrmask_t *saved_maskp)
{
  struct ia64_script_cache *cache = &as->global_cache;
  unw_caching_policy_t caching = as->caching_policy;

  if (caching == UNW_CACHE_NONE)
    return NULL;

#ifdef HAVE_ATOMIC_H
  if (!spin_trylock_irqsave (&cache->busy, *saved_maskp))
    return NULL;
#else
# ifdef HAVE___THREAD
  if (as->caching_policy == UNW_CACHE_PER_THREAD)
    cache = &ia64_per_thread_cache;
# endif
# ifdef HAVE_ATOMIC_OPS_H
  if (AO_test_and_set (&cache->busy) == AO_TS_SET)
    return NULL;
# else
  if (likely (caching == UNW_CACHE_GLOBAL))
    {
      Debug (16, "%s: acquiring lock\n", __FUNCTION__);
      lock_acquire (&cache->lock, *saved_maskp);
    }
# endif
#endif

  if (atomic_read (&as->cache_generation) != atomic_read (&cache->generation))
    {
      flush_script_cache (cache);
      cache->generation = as->cache_generation;
    }
  return cache;
}

static inline void
put_script_cache (unw_addr_space_t as, struct ia64_script_cache *cache,
		  intrmask_t *saved_maskp)
{
  assert (as->caching_policy != UNW_CACHE_NONE);

  Debug (16, "unmasking signals/interrupts and releasing lock\n");
#ifdef HAVE_ATOMIC_H
  spin_unlock_irqrestore (&cache->busy, *saved_maskp);
#else
# ifdef HAVE_ATOMIC_OPS_H
  AO_CLEAR (&cache->busy);
# else
  if (likely (as->caching_policy == UNW_CACHE_GLOBAL))
    lock_release (&cache->lock, *saved_maskp);
# endif
#endif
}

static struct ia64_script *
script_lookup (struct ia64_script_cache *cache, struct cursor *c)
{
  struct ia64_script *script = cache->buckets + c->hint;
  unsigned short index;
  unw_word_t ip, pr;

  ip = c->ip;
  pr = c->pr;

  if (cache_match (script, ip, pr))
    return script;

  index = cache->hash[hash (ip)];
  if (index >= IA64_UNW_CACHE_SIZE)
    return 0;

  script = cache->buckets + index;
  while (1)
    {
      if (cache_match (script, ip, pr))
	{
	  
	  c->hint = cache->buckets[c->prev_script].hint =
	    (script - cache->buckets);
	  return script;
	}
      if (script->coll_chain >= IA64_UNW_HASH_SIZE)
	return 0;
      script = cache->buckets + script->coll_chain;
    }
}

static inline void
script_init (struct ia64_script *script, unw_word_t ip)
{
  script->ip = ip;
  script->hint = 0;
  script->count = 0;
  script->abi_marker = 0;
}

static inline struct ia64_script *
script_new (struct ia64_script_cache *cache, unw_word_t ip)
{
  struct ia64_script *script, *prev, *tmp;
  unw_hash_index_t index;
  unsigned short head;

  head = cache->lru_head;
  script = cache->buckets + head;
  cache->lru_head = script->lru_chain;

  
  cache->buckets[cache->lru_tail].lru_chain = head;
  cache->lru_tail = head;

  
  if (script->ip)
    {
      index = hash (script->ip);
      tmp = cache->buckets + cache->hash[index];
      prev = 0;
      while (1)
	{
	  if (tmp == script)
	    {
	      if (prev)
		prev->coll_chain = tmp->coll_chain;
	      else
		cache->hash[index] = tmp->coll_chain;
	      break;
	    }
	  else
	    prev = tmp;
	  if (tmp->coll_chain >= IA64_UNW_CACHE_SIZE)
	    
	    break;
	  tmp = cache->buckets + tmp->coll_chain;
	}
    }

  
  index = hash (ip);
  script->coll_chain = cache->hash[index];
  cache->hash[index] = script - cache->buckets;

  script_init (script, ip);
  return script;
}

static inline void
script_finalize (struct ia64_script *script, struct cursor *c,
		 struct ia64_state_record *sr)
{
  script->pr_mask = sr->pr_mask;
  script->pr_val = sr->pr_val;
  script->pi = c->pi;
}

static inline void
script_emit (struct ia64_script *script, struct ia64_script_insn insn)
{
  if (script->count >= IA64_MAX_SCRIPT_LEN)
    {
      Dprintf ("%s: script exceeds maximum size of %u instructions!\n",
	       __FUNCTION__, IA64_MAX_SCRIPT_LEN);
      return;
    }
  script->insn[script->count++] = insn;
}

static void
compile_reg (struct ia64_state_record *sr, int i, struct ia64_reg_info *r,
	     struct ia64_script *script)
{
  enum ia64_script_insn_opcode opc;
  unsigned long val, rval;
  struct ia64_script_insn insn;
  long is_preserved_gr;

  if (r->where == IA64_WHERE_NONE || r->when >= sr->when_target)
    return;

  opc = IA64_INSN_MOVE;
  val = rval = r->val;
  is_preserved_gr = (i >= IA64_REG_R4 && i <= IA64_REG_R7);

  if (r->where == IA64_WHERE_GR)
    {
      
      if (rval >= 32)
	{
	  
	  if (is_preserved_gr)
	    opc = IA64_INSN_MOVE_STACKED_NAT;
	  else
	    opc = IA64_INSN_MOVE_STACKED;
	  val = rval;
	}
      else if (rval >= 4 && rval <= 7)
	{
	  
	  val = IA64_REG_R4 + (rval - 4);
	  if (is_preserved_gr)
	    opc = IA64_INSN_MOVE_NAT;
	}
      else
	{
	  
	  if (is_preserved_gr)
	    opc = IA64_INSN_MOVE_SCRATCH_NAT;
	  else
	    opc = IA64_INSN_MOVE_SCRATCH;
	  val = UNW_IA64_GR + rval;
	}
    }
  else
    {
      switch (r->where)
	{
	case IA64_WHERE_FR:
	  



	  if (rval >= 2 && rval <= 5)
	    val = IA64_REG_F2 + (rval - 2);
	  else if (rval >= 16 && rval <= 31)
	    val = IA64_REG_F16 + (rval - 16);
	  else
	    {
	      opc = IA64_INSN_MOVE_SCRATCH;
	      val = UNW_IA64_FR + rval;
	    }
	  break;

	case IA64_WHERE_BR:
	  if (rval >= 1 && rval <= 5)
	    {
	      val = IA64_REG_B1 + (rval - 1);
	      if (is_preserved_gr)
		opc = IA64_INSN_MOVE_NO_NAT;
	    }
	  else
	    {
	      opc = IA64_INSN_MOVE_SCRATCH;
	      if (is_preserved_gr)
		opc = IA64_INSN_MOVE_SCRATCH_NO_NAT;
	      val = UNW_IA64_BR + rval;
	    }
	  break;

	case IA64_WHERE_SPREL:
	  if (is_preserved_gr)
	    opc = IA64_INSN_ADD_SP_NAT;
	  else
	    {
	      opc = IA64_INSN_ADD_SP;
	      if (i >= IA64_REG_F2 && i <= IA64_REG_F31)
		val |= IA64_LOC_TYPE_FP;
	    }
	  break;

	case IA64_WHERE_PSPREL:
	  if (is_preserved_gr)
	    opc = IA64_INSN_ADD_PSP_NAT;
	  else
	    {
	      opc = IA64_INSN_ADD_PSP;
	      if (i >= IA64_REG_F2 && i <= IA64_REG_F31)
		val |= IA64_LOC_TYPE_FP;
	    }
	  break;

	default:
	  Dprintf ("%s: register %u has unexpected `where' value of %u\n",
		   __FUNCTION__, i, r->where);
	  break;
	}
    }
  insn.opc = opc;
  insn.dst = i;
  insn.val = val;
  script_emit (script, insn);

  if (i == IA64_REG_PSP)
    {
      


      insn.opc = IA64_INSN_LOAD_PSP;
      script_emit (script, insn);
    }
}







static inline int
sort_regs (struct ia64_state_record *sr, int regorder[])
{
  int r, i, j, max, max_reg, max_when, num_regs = 0;

  assert (IA64_REG_BSP == 3);

  for (r = IA64_REG_BSP; r < IA64_NUM_PREGS; ++r)
    {
      if (sr->curr.reg[r].where == IA64_WHERE_NONE
	  || sr->curr.reg[r].when >= sr->when_target)
	continue;

      regorder[num_regs++] = r;
    }

  



  for (i = max = 0; i < num_regs - 1; ++i)
    {
      max_reg = regorder[max];
      max_when = sr->curr.reg[max_reg].when;

      for (j = i + 1; j < num_regs; ++j)
	if (sr->curr.reg[regorder[j]].when > max_when)
	  {
	    max = j;
	    max_reg = regorder[j];
	    max_when = sr->curr.reg[max_reg].when;
	  }
      if (i != max)
	{
	  regorder[max] = regorder[i];
	  regorder[i] = max_reg;
	}
    }
  return num_regs;
}




static inline int
build_script (struct cursor *c, struct ia64_script *script)
{
  int num_regs, i, ret, regorder[IA64_NUM_PREGS - 3];
  struct ia64_reg_info *pri_unat;
  struct ia64_state_record sr;
  struct ia64_script_insn insn;

  ret = ia64_create_state_record (c, &sr);
  if (ret < 0)
    return ret;

  




  if (sr.when_target > sr.curr.reg[IA64_REG_PSP].when
      && (sr.curr.reg[IA64_REG_PSP].where == IA64_WHERE_NONE)
      && sr.curr.reg[IA64_REG_PSP].val != 0)
    {
      
      insn.opc = IA64_INSN_INC_PSP;
      insn.val = sr.curr.reg[IA64_REG_PSP].val;	
      script_emit (script, insn);
    }
  else
    compile_reg (&sr, IA64_REG_PSP, sr.curr.reg + IA64_REG_PSP, script);

  

  if (sr.when_target >= sr.curr.reg[IA64_REG_PRI_UNAT_GR].when
      || sr.when_target >= sr.curr.reg[IA64_REG_PRI_UNAT_MEM].when)
    {
      if (sr.when_target < sr.curr.reg[IA64_REG_PRI_UNAT_GR].when)
	
	pri_unat = sr.curr.reg + IA64_REG_PRI_UNAT_MEM;
      else if (sr.when_target < sr.curr.reg[IA64_REG_PRI_UNAT_MEM].when)
	
	pri_unat = sr.curr.reg + IA64_REG_PRI_UNAT_GR;
      else if (sr.curr.reg[IA64_REG_PRI_UNAT_MEM].when >
	       sr.curr.reg[IA64_REG_PRI_UNAT_GR].when)
	
	pri_unat = sr.curr.reg + IA64_REG_PRI_UNAT_MEM;
      else
	
	pri_unat = sr.curr.reg + IA64_REG_PRI_UNAT_GR;

      
      compile_reg (&sr, IA64_REG_PRI_UNAT_MEM, pri_unat, script);
    }

  

  num_regs = sort_regs (&sr, regorder);
  for (i = 0; i < num_regs; ++i)
    compile_reg (&sr, regorder[i], sr.curr.reg + regorder[i], script);

  script->abi_marker = sr.abi_marker;
  script_finalize (script, c, &sr);

  ia64_free_state_record (&sr);
  return 0;
}

static inline void
set_nat_info (struct cursor *c, unsigned long dst,
	      ia64_loc_t nat_loc, uint8_t bitnr)
{
  assert (dst >= IA64_REG_R4 && dst <= IA64_REG_R7);

  c->loc[dst - IA64_REG_R4 + IA64_REG_NAT4] = nat_loc;
  c->nat_bitnr[dst - IA64_REG_R4] = bitnr;
}





static inline int
run_script (struct ia64_script *script, struct cursor *c)
{
  struct ia64_script_insn *ip, *limit, next_insn;
  ia64_loc_t loc, nat_loc;
  unsigned long opc, dst;
  uint8_t nat_bitnr;
  unw_word_t val;
  int ret;

  c->pi = script->pi;
  ip = script->insn;
  limit = script->insn + script->count;
  next_insn = *ip;
  c->abi_marker = script->abi_marker;

  while (ip++ < limit)
    {
      opc = next_insn.opc;
      dst = next_insn.dst;
      val = next_insn.val;
      next_insn = *ip;

      
      if (likely (opc == IA64_INSN_MOVE_STACKED))
	{
	  if ((ret = ia64_get_stacked (c, val, &loc, NULL)) < 0)
	    return ret;
	}
      else
	switch (opc)
	  {
	  case IA64_INSN_INC_PSP:
	    c->psp += val;
	    continue;

	  case IA64_INSN_LOAD_PSP:
	    if ((ret = ia64_get (c, c->loc[IA64_REG_PSP], &c->psp)) < 0)
	      return ret;
	    continue;

	  case IA64_INSN_ADD_PSP:
	    loc = IA64_LOC_ADDR (c->psp + val, (val & IA64_LOC_TYPE_FP));
	    break;

	  case IA64_INSN_ADD_SP:
	    loc = IA64_LOC_ADDR (c->sp + val, (val & IA64_LOC_TYPE_FP));
	    break;

	  case IA64_INSN_MOVE_NO_NAT:
	    set_nat_info (c, dst, IA64_NULL_LOC, 0);
	  case IA64_INSN_MOVE:
	    loc = c->loc[val];
	    break;

	  case IA64_INSN_MOVE_SCRATCH_NO_NAT:
	    set_nat_info (c, dst, IA64_NULL_LOC, 0);
	  case IA64_INSN_MOVE_SCRATCH:
	    loc = ia64_scratch_loc (c, val, NULL);
	    break;

	  case IA64_INSN_ADD_PSP_NAT:
	    loc = IA64_LOC_ADDR (c->psp + val, 0);
	    assert (!IA64_IS_REG_LOC (loc));
	    set_nat_info (c, dst,
			  c->loc[IA64_REG_PRI_UNAT_MEM],
			  ia64_unat_slot_num (IA64_GET_ADDR (loc)));
	    break;

	  case IA64_INSN_ADD_SP_NAT:
	    loc = IA64_LOC_ADDR (c->sp + val, 0);
	    assert (!IA64_IS_REG_LOC (loc));
	    set_nat_info (c, dst,
			  c->loc[IA64_REG_PRI_UNAT_MEM],
			  ia64_unat_slot_num (IA64_GET_ADDR (loc)));
	    break;

	  case IA64_INSN_MOVE_NAT:
	    loc = c->loc[val];
	    set_nat_info (c, dst,
			  c->loc[val - IA64_REG_R4 + IA64_REG_NAT4],
			  c->nat_bitnr[val - IA64_REG_R4]);
	    break;

	  case IA64_INSN_MOVE_STACKED_NAT:
	    if ((ret = ia64_get_stacked (c, val, &loc, &nat_loc)) < 0)
	      return ret;
	    assert (!IA64_IS_REG_LOC (loc));
	    set_nat_info (c, dst, nat_loc, rse_slot_num (IA64_GET_ADDR (loc)));
	    break;

	  case IA64_INSN_MOVE_SCRATCH_NAT:
	    loc = ia64_scratch_loc (c, val, NULL);
	    nat_loc = ia64_scratch_loc (c, val + (UNW_IA64_NAT - UNW_IA64_GR),
					&nat_bitnr);
	    set_nat_info (c, dst, nat_loc, nat_bitnr);
	    break;
	  }
      c->loc[dst] = loc;
    }
  return 0;
}

static int
uncached_find_save_locs (struct cursor *c)
{
  struct ia64_script script;
  int ret = 0;

  if ((ret = ia64_fetch_proc_info (c, c->ip, 1)) < 0)
    return ret;

  script_init (&script, c->ip);
  if ((ret = build_script (c, &script)) < 0)
    {
      if (ret != -UNW_ESTOPUNWIND)
	Dprintf ("%s: failed to build unwind script for ip %lx\n",
		 __FUNCTION__, (long) c->ip);
      return ret;
    }
  return run_script (&script, c);
}

HIDDEN int
ia64_find_save_locs (struct cursor *c)
{
  struct ia64_script_cache *cache = NULL;
  struct ia64_script *script = NULL;
  intrmask_t saved_mask;
  int ret = 0;

  if (c->as->caching_policy == UNW_CACHE_NONE)
    return uncached_find_save_locs (c);

  cache = get_script_cache (c->as, &saved_mask);
  if (!cache)
    {
      Debug (1, "contention on script-cache; doing uncached lookup\n");
      return uncached_find_save_locs (c);
    }
  {
    script = script_lookup (cache, c);
    Debug (8, "ip %lx %s in script cache\n", (long) c->ip,
	   script ? "hit" : "missed");

    if (!script || (script->count == 0 && !script->pi.unwind_info))
      {
	if ((ret = ia64_fetch_proc_info (c, c->ip, 1)) < 0)
	  goto out;
      }

    if (!script)
      {
	script = script_new (cache, c->ip);
	if (!script)
	  {
	    Dprintf ("%s: failed to create unwind script\n", __FUNCTION__);
	    ret = -UNW_EUNSPEC;
	    goto out;
	  }
      }
    cache->buckets[c->prev_script].hint = script - cache->buckets;

    if (script->count == 0)
      ret = build_script (c, script);

    assert (script->count > 0);

    c->hint = script->hint;
    c->prev_script = script - cache->buckets;

    if (ret < 0)
      {
	if (ret != -UNW_ESTOPUNWIND)
	  Dprintf ("%s: failed to locate/build unwind script for ip %lx\n",
		   __FUNCTION__, (long) c->ip);
	goto out;
      }

    ret = run_script (script, c);
  }
 out:
  put_script_cache (c->as, cache, &saved_mask);
  return ret;
}

HIDDEN void
ia64_validate_cache (unw_addr_space_t as, void *arg)
{
#ifndef UNW_REMOTE_ONLY
  if (as == unw_local_addr_space && ia64_local_validate_cache (as, arg) == 1)
    return;
#endif

#ifndef UNW_LOCAL_ONLY
  
  unwi_dyn_validate_cache (as, arg);
#endif
}

HIDDEN int
ia64_cache_proc_info (struct cursor *c)
{
  struct ia64_script_cache *cache;
  struct ia64_script *script;
  intrmask_t saved_mask;
  int ret = 0;

  cache = get_script_cache (c->as, &saved_mask);
  if (!cache)
    return ret;	

  
  script = script_lookup (cache, c);
  if (script)
    goto out;

  script = script_new (cache, c->ip);
  if (!script)
    {
      Dprintf ("%s: failed to create unwind script\n", __FUNCTION__);
      ret = -UNW_EUNSPEC;
      goto out;
    }

  script->pi = c->pi;

 out:
  put_script_cache (c->as, cache, &saved_mask);
  return ret;
}

HIDDEN int
ia64_get_cached_proc_info (struct cursor *c)
{
  struct ia64_script_cache *cache;
  struct ia64_script *script;
  intrmask_t saved_mask;

  cache = get_script_cache (c->as, &saved_mask);
  if (!cache)
    return -UNW_ENOINFO;	
  {
    script = script_lookup (cache, c);
    if (script)
      c->pi = script->pi;
  }
  put_script_cache (c->as, cache, &saved_mask);
  return script ? 0 : -UNW_ENOINFO;
}
