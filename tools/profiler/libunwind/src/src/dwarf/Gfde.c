
























#include "dwarf_i.h"

static inline int
is_cie_id (unw_word_t val, int is_debug_frame)
{
  


  if (is_debug_frame)
    return (val == - (uint32_t) 1 || val == - (uint64_t) 1);
  else
    return (val == 0);
}






static inline int
parse_cie (unw_addr_space_t as, unw_accessors_t *a, unw_word_t addr,
	   const unw_proc_info_t *pi, struct dwarf_cie_info *dci,
	   unw_word_t base, void *arg)
{
  uint8_t version, ch, augstr[5], fde_encoding, handler_encoding;
  unw_word_t len, cie_end_addr, aug_size;
  uint32_t u32val;
  uint64_t u64val;
  size_t i;
  int ret;
# define STR2(x)	#x
# define STR(x)		STR2(x)

  




  switch (dwarf_addr_size (as))
    {
    case 4:	fde_encoding = DW_EH_PE_udata4; break;
    case 8:	fde_encoding = DW_EH_PE_udata8; break;
    default:	fde_encoding = DW_EH_PE_omit; break;
    }

  dci->lsda_encoding = DW_EH_PE_omit;
  dci->handler = 0;

  if ((ret = dwarf_readu32 (as, a, &addr, &u32val, arg)) < 0)
    return ret;

  if (u32val != 0xffffffff)
    {
      
      uint32_t cie_id;
      
      const uint32_t expected_id = (base) ? 0xffffffff : 0;

      len = u32val;
      cie_end_addr = addr + len;
      if ((ret = dwarf_readu32 (as, a, &addr, &cie_id, arg)) < 0)
	return ret;
      if (cie_id != expected_id)
	{
	  Debug (1, "Unexpected CIE id %x\n", cie_id);
	  return -UNW_EINVAL;
	}
    }
  else
    {
      
      uint64_t cie_id;
      

      const uint64_t expected_id = (base) ? 0xffffffffffffffffull : 0;

      if ((ret = dwarf_readu64 (as, a, &addr, &u64val, arg)) < 0)
	return ret;
      len = u64val;
      cie_end_addr = addr + len;
      if ((ret = dwarf_readu64 (as, a, &addr, &cie_id, arg)) < 0)
	return ret;
      if (cie_id != expected_id)
	{
	  Debug (1, "Unexpected CIE id %llx\n", (long long) cie_id);
	  return -UNW_EINVAL;
	}
    }
  dci->cie_instr_end = cie_end_addr;

  if ((ret = dwarf_readu8 (as, a, &addr, &version, arg)) < 0)
    return ret;

  if (version != 1 && version != DWARF_CIE_VERSION)
    {
      Debug (1, "Got CIE version %u, expected version 1 or "
	     STR (DWARF_CIE_VERSION) "\n", version);
      return -UNW_EBADVERSION;
    }

  
  memset (augstr, 0, sizeof (augstr));
  for (i = 0;;)
    {
      if ((ret = dwarf_readu8 (as, a, &addr, &ch, arg)) < 0)
	return ret;

      if (!ch)
	break;	

      if (i < sizeof (augstr) - 1)
	augstr[i++] = ch;
    }

  if ((ret = dwarf_read_uleb128 (as, a, &addr, &dci->code_align, arg)) < 0
      || (ret = dwarf_read_sleb128 (as, a, &addr, &dci->data_align, arg)) < 0)
    return ret;

  
  if (version == 1)
    {
      if ((ret = dwarf_readu8 (as, a, &addr, &ch, arg)) < 0)
	return ret;
      dci->ret_addr_column = ch;
    }
  else if ((ret = dwarf_read_uleb128 (as, a, &addr, &dci->ret_addr_column,
				      arg)) < 0)
    return ret;

  i = 0;
  if (augstr[0] == 'z')
    {
      dci->sized_augmentation = 1;
      if ((ret = dwarf_read_uleb128 (as, a, &addr, &aug_size, arg)) < 0)
	return ret;
      i++;
    }

  for (; i < sizeof (augstr) && augstr[i]; ++i)
    switch (augstr[i])
      {
      case 'L':
	
	if ((ret = dwarf_readu8 (as, a, &addr, &ch, arg)) < 0)
	  return ret;
	dci->lsda_encoding = ch;
	break;

      case 'R':
	
	if ((ret = dwarf_readu8 (as, a, &addr, &fde_encoding, arg)) < 0)
	  return ret;
	break;

      case 'P':
	
	if ((ret = dwarf_readu8 (as, a, &addr, &handler_encoding, arg)) < 0)
	  return ret;
	if ((ret = dwarf_read_encoded_pointer (as, a, &addr, handler_encoding,
					       pi, &dci->handler, arg)) < 0)
	  return ret;
	break;

      case 'S':
	
	dci->signal_frame = 1;

	

	dci->have_abi_marker = 1;
	break;

      default:
	Debug (1, "Unexpected augmentation string `%s'\n", augstr);
	if (dci->sized_augmentation)
	  

	  goto done;
	else
	  return -UNW_EINVAL;
      }
 done:
  dci->fde_encoding = fde_encoding;
  dci->cie_instr_start = addr;
  Debug (15, "CIE parsed OK, augmentation = \"%s\", handler=0x%lx\n",
	 augstr, (long) dci->handler);
  return 0;
}






HIDDEN int
dwarf_extract_proc_info_from_fde (unw_addr_space_t as, unw_accessors_t *a,
				  unw_word_t *addrp, unw_proc_info_t *pi,
				  int need_unwind_info, unw_word_t base,
				  void *arg)
{
  unw_word_t fde_end_addr, cie_addr, cie_offset_addr, aug_end_addr = 0;
  unw_word_t start_ip, ip_range, aug_size, addr = *addrp;
  int ret, ip_range_encoding;
  struct dwarf_cie_info dci;
  uint64_t u64val;
  uint32_t u32val;

  Debug (12, "FDE @ 0x%lx\n", (long) addr);

  memset (&dci, 0, sizeof (dci));

  if ((ret = dwarf_readu32 (as, a, &addr, &u32val, arg)) < 0)
    return ret;

  if (u32val != 0xffffffff)
    {
      int32_t cie_offset;

      

      if (u32val == 0)
	return -UNW_ENOINFO;

      

      *addrp = fde_end_addr = addr + u32val;
      cie_offset_addr = addr;

      if ((ret = dwarf_reads32 (as, a, &addr, &cie_offset, arg)) < 0)
	return ret;

      if (is_cie_id (cie_offset, base != 0))
	
	return 0;

      if (base != 0)
        cie_addr = base + cie_offset;
      else
	



	cie_addr = cie_offset_addr - cie_offset;
    }
  else
    {
      int64_t cie_offset;

      

      if ((ret = dwarf_readu64 (as, a, &addr, &u64val, arg)) < 0)
	return ret;

      *addrp = fde_end_addr = addr + u64val;
      cie_offset_addr = addr;

      if ((ret = dwarf_reads64 (as, a, &addr, &cie_offset, arg)) < 0)
	return ret;

      if (is_cie_id (cie_offset, base != 0))
	
	return 0;

      if (base != 0)
	cie_addr = base + cie_offset;
      else
	



	cie_addr = (unw_word_t) ((uint64_t) cie_offset_addr - cie_offset);
    }

  Debug (15, "looking for CIE at address %lx\n", (long) cie_addr);

  if ((ret = parse_cie (as, a, cie_addr, pi, &dci, base, arg)) < 0)
    return ret;

  

  ip_range_encoding = dci.fde_encoding & DW_EH_PE_FORMAT_MASK;

  if ((ret = dwarf_read_encoded_pointer (as, a, &addr, dci.fde_encoding,
					 pi, &start_ip, arg)) < 0
      || (ret = dwarf_read_encoded_pointer (as, a, &addr, ip_range_encoding,
					    pi, &ip_range, arg)) < 0)
    return ret;
  pi->start_ip = start_ip;
  pi->end_ip = start_ip + ip_range;
  pi->handler = dci.handler;

  if (dci.sized_augmentation)
    {
      if ((ret = dwarf_read_uleb128 (as, a, &addr, &aug_size, arg)) < 0)
	return ret;
      aug_end_addr = addr + aug_size;
    }

  if ((ret = dwarf_read_encoded_pointer (as, a, &addr, dci.lsda_encoding,
					 pi, &pi->lsda, arg)) < 0)
    return ret;

  Debug (15, "FDE covers IP 0x%lx-0x%lx, LSDA=0x%lx\n",
	 (long) pi->start_ip, (long) pi->end_ip, (long) pi->lsda);

  if (need_unwind_info)
    {
      pi->format = UNW_INFO_FORMAT_TABLE;
      pi->unwind_info_size = sizeof (dci);
      pi->unwind_info = mempool_alloc (&dwarf_cie_info_pool);
      if (!pi->unwind_info)
	return -UNW_ENOMEM;

      if (dci.have_abi_marker)
	{
	  if ((ret = dwarf_readu16 (as, a, &addr, &dci.abi, arg)) < 0
	      || (ret = dwarf_readu16 (as, a, &addr, &dci.tag, arg)) < 0)
	    return ret;
	  Debug (13, "Found ABI marker = (abi=%u, tag=%u)\n",
		 dci.abi, dci.tag);
	}

      if (dci.sized_augmentation)
	dci.fde_instr_start = aug_end_addr;
      else
	dci.fde_instr_start = addr;
      dci.fde_instr_end = fde_end_addr;

      memcpy (pi->unwind_info, &dci, sizeof (dci));
    }
  return 0;
}
