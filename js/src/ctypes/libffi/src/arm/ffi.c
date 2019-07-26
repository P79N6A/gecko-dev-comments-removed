





























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>


static int vfp_type_p (ffi_type *);
static void layout_vfp_args (ffi_cif *);

int ffi_prep_args_SYSV(char *stack, extended_cif *ecif, float *vfp_space);
int ffi_prep_args_VFP(char *stack, extended_cif *ecif, float *vfp_space);

static char* ffi_align(ffi_type **p_arg, char *argp)
{
  
  register size_t alignment = (*p_arg)->alignment;
  if (alignment < 4)
  {
    alignment = 4;
  }
#ifdef _WIN32_WCE
  if (alignment > 4)
  {
    alignment = 4;
  }
#endif
  if ((alignment - 1) & (unsigned) argp)
  {
    argp = (char *) ALIGN(argp, alignment);
  }

  if ((*p_arg)->type == FFI_TYPE_STRUCT)
  {
    argp = (char *) ALIGN(argp, 4);
  }
  return argp;
}

static size_t ffi_put_arg(ffi_type **arg_type, void **arg, char *stack)
{
	register char* argp = stack;
	register ffi_type **p_arg = arg_type;
	register void **p_argv = arg;
	register size_t z = (*p_arg)->size;
  if (z < sizeof(int))
    {
		z = sizeof(int);
		switch ((*p_arg)->type)
      {
      case FFI_TYPE_SINT8:
        *(signed int *) argp = (signed int)*(SINT8 *)(* p_argv);
        break;
        
      case FFI_TYPE_UINT8:
        *(unsigned int *) argp = (unsigned int)*(UINT8 *)(* p_argv);
        break;
        
      case FFI_TYPE_SINT16:
        *(signed int *) argp = (signed int)*(SINT16 *)(* p_argv);
        break;
        
      case FFI_TYPE_UINT16:
        *(unsigned int *) argp = (unsigned int)*(UINT16 *)(* p_argv);
        break;
        
      case FFI_TYPE_STRUCT:
        memcpy(argp, *p_argv, (*p_arg)->size);
        break;

      default:
        FFI_ASSERT(0);
      }
    }
  else if (z == sizeof(int))
    {
		if ((*p_arg)->type == FFI_TYPE_FLOAT)
			*(float *) argp = *(float *)(* p_argv);
		else
			*(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
    }
	else if (z == sizeof(double) && (*p_arg)->type == FFI_TYPE_DOUBLE)
		{
			*(double *) argp = *(double *)(* p_argv);
		}
  else
    {
      memcpy(argp, *p_argv, z);
    }
  return z;
}







int ffi_prep_args_SYSV(char *stack, extended_cif *ecif, float *vfp_space)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;
  argp = stack;
  

  if ( ecif->cif->flags == FFI_TYPE_STRUCT ) {
    *(void **) argp = ecif->rvalue;
    argp += 4;
  }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++, p_argv++)
    {
    argp = ffi_align(p_arg, argp);
    argp += ffi_put_arg(p_arg, p_argv, argp);
    }

  return 0;
}

int ffi_prep_args_VFP(char *stack, extended_cif *ecif, float *vfp_space)
{
  
  FFI_ASSERT(ecif->cif->abi == FFI_VFP);

  register unsigned int i, vi = 0;
  register void **p_argv;
  register char *argp, *regp, *eo_regp;
  register ffi_type **p_arg;
  char stack_used = 0;
  char done_with_regs = 0;
  char is_vfp_type;

  

  regp = stack;
  eo_regp = argp = regp + 16;
  

  

  if ( ecif->cif->flags == FFI_TYPE_STRUCT ) {
    *(void **) regp = ecif->rvalue;
    regp += 4;
  }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++, p_argv++)
    {
      is_vfp_type = vfp_type_p (*p_arg);

      
      if(vi < ecif->cif->vfp_nargs && is_vfp_type)
        {
          char *vfp_slot = (char *)(vfp_space + ecif->cif->vfp_args[vi++]);
          ffi_put_arg(p_arg, p_argv, vfp_slot);
          continue;
        }
      
      else if (!done_with_regs && !is_vfp_type)
        {
          char *tregp = ffi_align(p_arg, regp);
          size_t size = (*p_arg)->size; 
          size = (size < 4)? 4 : size; 
          

          if(tregp + size <= eo_regp)
            {
              regp = tregp + ffi_put_arg(p_arg, p_argv, tregp);
              done_with_regs = (regp == argp);
              
              FFI_ASSERT(regp <= argp);
              continue;
            }
          


          else if (!stack_used) 
            {
              stack_used = 1;
              done_with_regs = 1;
              argp = tregp + ffi_put_arg(p_arg, p_argv, tregp);
              FFI_ASSERT(eo_regp < argp);
              continue;
            }
        }
      
      stack_used = 1;
      argp = ffi_align(p_arg, argp);
      argp += ffi_put_arg(p_arg, p_argv, argp);
    }
  
  return ecif->cif->vfp_used;
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  int type_code;
  


  cif->bytes = (cif->bytes + 7) & ~7;

  
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = (unsigned) FFI_TYPE_SINT64;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->abi == FFI_VFP
	  && (type_code = vfp_type_p (cif->rtype)) != 0)
	{
	  

	  cif->flags = (unsigned) type_code;
	}
      else if (cif->rtype->size <= 4)
	
	cif->flags = (unsigned)FFI_TYPE_INT;
      else
	


	cif->flags = (unsigned)FFI_TYPE_STRUCT;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  


  if (cif->abi == FFI_VFP)
    layout_vfp_args (cif);

  return FFI_OK;
}


ffi_status ffi_prep_cif_machdep_var(ffi_cif *cif,
				    unsigned int nfixedargs,
				    unsigned int ntotalargs)
{
  
  if (cif->abi == FFI_VFP)
	cif->abi = FFI_SYSV;

  return ffi_prep_cif_machdep(cif);
}


extern void ffi_call_SYSV (void (*fn)(void), extended_cif *, unsigned, unsigned, unsigned *);
extern void ffi_call_VFP (void (*fn)(void), extended_cif *, unsigned, unsigned, unsigned *);

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  int small_struct = (cif->flags == FFI_TYPE_INT 
		      && cif->rtype->type == FFI_TYPE_STRUCT);
  int vfp_struct = (cif->flags == FFI_TYPE_STRUCT_VFP_FLOAT
		    || cif->flags == FFI_TYPE_STRUCT_VFP_DOUBLE);

  unsigned int temp;
  
  ecif.cif = cif;
  ecif.avalue = avalue;

  
  

  if ((rvalue == NULL) && 
      (cif->flags == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else if (small_struct)
    ecif.rvalue = &temp;
  else if (vfp_struct)
    {
      
      ecif.rvalue = alloca(32);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case FFI_SYSV:
      ffi_call_SYSV (fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
      break;

    case FFI_VFP:
#ifdef __ARM_EABI__
      ffi_call_VFP (fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
      break;
#endif

    default:
      FFI_ASSERT(0);
      break;
    }
  if (small_struct)
    {
      FFI_ASSERT(rvalue != NULL);
      memcpy (rvalue, &temp, cif->rtype->size);
    }
    
  else if (vfp_struct)
    {
      FFI_ASSERT(rvalue != NULL);
      memcpy (rvalue, ecif.rvalue, cif->rtype->size);
    }
    
}



static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					 void** args, ffi_cif* cif, float *vfp_stack);

static void ffi_prep_incoming_args_VFP (char *stack, void **ret,
					 void** args, ffi_cif* cif, float *vfp_stack);

void ffi_closure_SYSV (ffi_closure *);

void ffi_closure_VFP (ffi_closure *);



unsigned int FFI_HIDDEN
ffi_closure_inner (ffi_closure *closure, 
		   void **respp, void *args, void *vfp_args)
{
  
  ffi_cif       *cif;
  void         **arg_area;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  




  if (cif->abi == FFI_VFP)
    ffi_prep_incoming_args_VFP(args, respp, arg_area, cif, vfp_args);
  else
    ffi_prep_incoming_args_SYSV(args, respp, arg_area, cif, vfp_args);

  (closure->fun) (cif, *respp, arg_area, closure->user_data);

  return cif->flags;
}


static void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif,
			    
			    float *vfp_stack)

{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( cif->flags == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      argp = ffi_align(p_arg, argp);

      z = (*p_arg)->size;

      

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}


static void 
ffi_prep_incoming_args_VFP(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif,
			    
			    float *vfp_stack)

{
  register unsigned int i, vi = 0;
  register void **p_argv;
  register char *argp, *regp, *eo_regp;
  register ffi_type **p_arg;
  char done_with_regs = 0;
  char stack_used = 0;
  char is_vfp_type;

  FFI_ASSERT(cif->abi == FFI_VFP);
  regp = stack;
  eo_regp = argp = regp + 16;

  if ( cif->flags == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) regp;
    regp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
    size_t z;
    is_vfp_type = vfp_type_p (*p_arg); 

    if(vi < cif->vfp_nargs && is_vfp_type)
      {
        *p_argv++ = (void*)(vfp_stack + cif->vfp_args[vi++]);
        continue;
      }
    else if (!done_with_regs && !is_vfp_type)
      {
        char* tregp = ffi_align(p_arg, regp);

        z = (*p_arg)->size; 
        z = (z < 4)? 4 : z; 
        
        

        if(tregp + z <= eo_regp || !stack_used) 
          {
          
          *p_argv = (void*) tregp;

          p_argv++;
          regp = tregp + z;
          
          
          if(regp > eo_regp)
            {
            if(stack_used)
              {
                abort(); 
                         
              }
            argp = regp;
            }
          if(regp >= eo_regp)
            {
            done_with_regs = 1;
            stack_used = 1;
            }
          continue;
          }
      }
    stack_used = 1;

    argp = ffi_align(p_arg, argp);

    z = (*p_arg)->size;

    

    *p_argv = (void*) argp;

    p_argv++;
    argp += z;
    }
  
  return;
}



extern unsigned int ffi_arm_trampoline[3];

#if FFI_EXEC_TRAMPOLINE_TABLE

#include <mach/mach.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern void *ffi_closure_trampoline_table_page;

typedef struct ffi_trampoline_table ffi_trampoline_table;
typedef struct ffi_trampoline_table_entry ffi_trampoline_table_entry;

struct ffi_trampoline_table {
  
  vm_address_t config_page;
  vm_address_t trampoline_page;

  
  uint16_t free_count;
  ffi_trampoline_table_entry *free_list;
  ffi_trampoline_table_entry *free_list_pool;

  ffi_trampoline_table *prev;
  ffi_trampoline_table *next;
};

struct ffi_trampoline_table_entry {
  void *(*trampoline)();
  ffi_trampoline_table_entry *next;
};



#undef FFI_TRAMPOLINE_SIZE
#define FFI_TRAMPOLINE_SIZE 12


#define FFI_TRAMPOLINE_CODELOC_CONFIG(codeloc) ((void **) (((uint8_t *) codeloc) - 4080));


#define FFI_TRAMPOLINE_CONFIG_PAGE_OFFSET 16


#define FFI_TRAMPOLINE_COUNT ((PAGE_SIZE - FFI_TRAMPOLINE_CONFIG_PAGE_OFFSET) / FFI_TRAMPOLINE_SIZE)

static pthread_mutex_t ffi_trampoline_lock = PTHREAD_MUTEX_INITIALIZER;
static ffi_trampoline_table *ffi_trampoline_tables = NULL;

static ffi_trampoline_table *
ffi_trampoline_table_alloc ()
{
  ffi_trampoline_table *table = NULL;

  
  while (table == NULL) {
    vm_address_t config_page = 0x0;
    kern_return_t kt;

    
    kt = vm_allocate (mach_task_self (), &config_page, PAGE_SIZE*2, VM_FLAGS_ANYWHERE);
    if (kt != KERN_SUCCESS) {
      fprintf(stderr, "vm_allocate() failure: %d at %s:%d\n", kt, __FILE__, __LINE__);
      break;
    }

    
    vm_address_t trampoline_page = config_page+PAGE_SIZE;
    kt = vm_deallocate (mach_task_self (), trampoline_page, PAGE_SIZE);
    if (kt != KERN_SUCCESS) {
      fprintf(stderr, "vm_deallocate() failure: %d at %s:%d\n", kt, __FILE__, __LINE__);
      break;
    }

    
    vm_prot_t cur_prot;
    vm_prot_t max_prot;

    kt = vm_remap (mach_task_self (), &trampoline_page, PAGE_SIZE, 0x0, FALSE, mach_task_self (), (vm_address_t) &ffi_closure_trampoline_table_page, FALSE, &cur_prot, &max_prot, VM_INHERIT_SHARE);

    
    if (kt != KERN_SUCCESS) {
      
      if (kt != KERN_NO_SPACE) {
        fprintf(stderr, "vm_remap() failure: %d at %s:%d\n", kt, __FILE__, __LINE__);
      }

      vm_deallocate (mach_task_self (), config_page, PAGE_SIZE);
      continue;
    }

    
    table = calloc (1, sizeof(ffi_trampoline_table));
    table->free_count = FFI_TRAMPOLINE_COUNT;
    table->config_page = config_page;
    table->trampoline_page = trampoline_page;

    
    table->free_list_pool = calloc(FFI_TRAMPOLINE_COUNT, sizeof(ffi_trampoline_table_entry));

    uint16_t i;
    for (i = 0; i < table->free_count; i++) {
      ffi_trampoline_table_entry *entry = &table->free_list_pool[i];
      entry->trampoline = (void *) (table->trampoline_page + (i * FFI_TRAMPOLINE_SIZE));

      if (i < table->free_count - 1)
        entry->next = &table->free_list_pool[i+1];
    }

    table->free_list = table->free_list_pool;
  }

  return table;
}

void *
ffi_closure_alloc (size_t size, void **code)
{
  
  ffi_closure *closure = malloc(size);
  if (closure == NULL)
    return NULL;

  pthread_mutex_lock(&ffi_trampoline_lock);

  
  ffi_trampoline_table *table = ffi_trampoline_tables;
  if (table == NULL || table->free_list == NULL) {
    table = ffi_trampoline_table_alloc ();
    if (table == NULL) {
      free(closure);
      return NULL;
    }

    
    table->next = ffi_trampoline_tables;
    if (table->next != NULL)
        table->next->prev = table;

    ffi_trampoline_tables = table;
  }

  
  ffi_trampoline_table_entry *entry = ffi_trampoline_tables->free_list;
  ffi_trampoline_tables->free_list = entry->next;
  ffi_trampoline_tables->free_count--;
  entry->next = NULL;

  pthread_mutex_unlock(&ffi_trampoline_lock);

  
  *code = entry->trampoline;
  closure->trampoline_table = table;
  closure->trampoline_table_entry = entry;

  return closure;
}

void
ffi_closure_free (void *ptr)
{
  ffi_closure *closure = ptr;

  pthread_mutex_lock(&ffi_trampoline_lock);

  
  ffi_trampoline_table *table = closure->trampoline_table;
  ffi_trampoline_table_entry *entry = closure->trampoline_table_entry;

  
  entry->next = table->free_list;
  table->free_list = entry;
  table->free_count++;

  

  if (table->free_count == FFI_TRAMPOLINE_COUNT && ffi_trampoline_tables != table) {
    
    if (table->prev != NULL)
      table->prev->next = table->next;

    if (table->next != NULL)
      table->next->prev = table->prev;

    
    kern_return_t kt;
    kt = vm_deallocate (mach_task_self (), table->config_page, PAGE_SIZE);
    if (kt != KERN_SUCCESS)
      fprintf(stderr, "vm_deallocate() failure: %d at %s:%d\n", kt, __FILE__, __LINE__);

    kt = vm_deallocate (mach_task_self (), table->trampoline_page, PAGE_SIZE);
    if (kt != KERN_SUCCESS)
      fprintf(stderr, "vm_deallocate() failure: %d at %s:%d\n", kt, __FILE__, __LINE__);

    
    free (table->free_list_pool);
    free (table);
  } else if (ffi_trampoline_tables != table) {
    
    table->prev = NULL;
    table->next = ffi_trampoline_tables;
    if (ffi_trampoline_tables != NULL)
      ffi_trampoline_tables->prev = table;

    ffi_trampoline_tables = table;
  }

  pthread_mutex_unlock (&ffi_trampoline_lock);

  
  free (closure);
}

#else

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX)				\
({ unsigned char *__tramp = (unsigned char*)(TRAMP);			\
   unsigned int  __fun = (unsigned int)(FUN);				\
   unsigned int  __ctx = (unsigned int)(CTX);				\
   unsigned char *insns = (unsigned char *)(CTX);                       \
   memcpy (__tramp, ffi_arm_trampoline, sizeof ffi_arm_trampoline);     \
   *(unsigned int*) &__tramp[12] = __ctx;				\
   *(unsigned int*) &__tramp[16] = __fun;				\
   __clear_cache((&__tramp[0]), (&__tramp[19])); /* Clear data mapping.  */ \
   __clear_cache(insns, insns + 3 * sizeof (unsigned int));             \
                                                 /* Clear instruction   \
                                                    mapping.  */        \
 })

#endif



ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  void (*closure_func)(ffi_closure*) = NULL;

  if (cif->abi == FFI_SYSV)
    closure_func = &ffi_closure_SYSV;
#ifdef __ARM_EABI__
  else if (cif->abi == FFI_VFP)
    closure_func = &ffi_closure_VFP;
#endif
  else
    return FFI_BAD_ABI;

#if FFI_EXEC_TRAMPOLINE_TABLE
  void **config = FFI_TRAMPOLINE_CODELOC_CONFIG(codeloc);
  config[0] = closure;
  config[1] = closure_func;
#else
  FFI_INIT_TRAMPOLINE (&closure->tramp[0], \
		       closure_func,  \
		       codeloc);
#endif

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}



static int rec_vfp_type_p (ffi_type *t, int *elt, int *elnum)
{
  switch (t->type)
    {
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      *elt = (int) t->type;
      *elnum = 1;
      return 1;

    case FFI_TYPE_STRUCT_VFP_FLOAT:
      *elt = FFI_TYPE_FLOAT;
      *elnum = t->size / sizeof (float);
      return 1;

    case FFI_TYPE_STRUCT_VFP_DOUBLE:
      *elt = FFI_TYPE_DOUBLE;
      *elnum = t->size / sizeof (double);
      return 1;

    case FFI_TYPE_STRUCT:;
      {
	int base_elt = 0, total_elnum = 0;
	ffi_type **el = t->elements;
	while (*el)
	  {
	    int el_elt = 0, el_elnum = 0;
	    if (! rec_vfp_type_p (*el, &el_elt, &el_elnum)
		|| (base_elt && base_elt != el_elt)
		|| total_elnum + el_elnum > 4)
	      return 0;
	    base_elt = el_elt;
	    total_elnum += el_elnum;
	    el++;
	  }
	*elnum = total_elnum;
	*elt = base_elt;
	return 1;
      }
    default: ;
    }
  return 0;
}

static int vfp_type_p (ffi_type *t)
{
  int elt, elnum;
  if (rec_vfp_type_p (t, &elt, &elnum))
    {
      if (t->type == FFI_TYPE_STRUCT)
	{
	  if (elnum == 1)
	    t->type = elt;
	  else
	    t->type = (elt == FFI_TYPE_FLOAT
		       ? FFI_TYPE_STRUCT_VFP_FLOAT
		       : FFI_TYPE_STRUCT_VFP_DOUBLE);
	}
      return (int) t->type;
    }
  return 0;
}

static int place_vfp_arg (ffi_cif *cif, ffi_type *t)
{
  short reg = cif->vfp_reg_free;
  int nregs = t->size / sizeof (float);
  int align = ((t->type == FFI_TYPE_STRUCT_VFP_FLOAT
		|| t->type == FFI_TYPE_FLOAT) ? 1 : 2);
  
  if ((reg & 1) && align == 2)
    reg++;
  while (reg + nregs <= 16)
    {
      int s, new_used = 0;
      for (s = reg; s < reg + nregs; s++)
	{
	  new_used |= (1 << s);
	  if (cif->vfp_used & (1 << s))
	    {
	      reg += align;
	      goto next_reg;
	    }
	}
      
      cif->vfp_used |= new_used;
      cif->vfp_args[cif->vfp_nargs++] = reg;

      
      if (cif->vfp_used & (1 << cif->vfp_reg_free))
	{
	  reg += nregs;
	  while (cif->vfp_used & (1 << reg))
	    reg += 1;
	  cif->vfp_reg_free = reg;
	}
      return 0;
    next_reg: ;
    }
  
  cif->vfp_reg_free = 16;
  cif->vfp_used = 0xFFFF;
  return 1;
}

static void layout_vfp_args (ffi_cif *cif)
{
  int i;
  
  cif->vfp_used = 0;
  cif->vfp_nargs = 0;
  cif->vfp_reg_free = 0;
  memset (cif->vfp_args, -1, 16); 

  for (i = 0; i < cif->nargs; i++)
    {
      ffi_type *t = cif->arg_types[i];
      if (vfp_type_p (t) && place_vfp_arg (cif, t) == 1)
        {
          break;
        }
    }
}
