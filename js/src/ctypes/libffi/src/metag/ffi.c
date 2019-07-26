























#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))






unsigned int ffi_prep_args(char *stack, extended_cif *ecif)
{
	register unsigned int i;
	register void **p_argv;
	register char *argp;
	register ffi_type **p_arg;

	argp = stack;

	
	if ( ecif->cif->flags == FFI_TYPE_STRUCT ) {
		argp -= 4;
		*(void **) argp = ecif->rvalue;
	}

	p_argv = ecif->avalue;

	
	for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; (i != 0); i--, p_arg++, p_argv++)
	{
		size_t z;

		
		z = (*p_arg)->size;
		argp -= z;

		
		argp = (char *) ALIGN_DOWN(ALIGN_DOWN(argp, (*p_arg)->alignment), 4);

		if (z < sizeof(int)) {
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
			case FFI_TYPE_STRUCT:
				memcpy(argp, *p_argv, (*p_arg)->size);
				break;
			default:
				FFI_ASSERT(0);
			}
		} else if ( z == sizeof(int)) {
			*(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
		} else {
			memcpy(argp, *p_argv, z);
		}
	}

	

	return ALIGN(MIN(stack - argp, 6*4), 8);
}


ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
	ffi_type **ptr;
	unsigned i, bytes = 0;

	for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++) {
		if ((*ptr)->size == 0)
			return FFI_BAD_TYPEDEF;

		

		FFI_ASSERT_VALID_TYPE(*ptr);

		
		if (((*ptr)->alignment - 1) & bytes)
			bytes = ALIGN(bytes, (*ptr)->alignment);

		bytes += ALIGN((*ptr)->size, 4);
	}

	
	bytes = ALIGN(bytes, 8);

	
	if (cif->rtype->type == FFI_TYPE_STRUCT) {
		bytes += sizeof(void*);

		
		bytes = ALIGN(bytes, 8);
	}

	cif->bytes = bytes;

	
	switch (cif->rtype->type) {
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
		
		if (cif->rtype->size <= 4)
			
			cif->flags = (unsigned)FFI_TYPE_INT;
		else if ((cif->rtype->size > 4) && (cif->rtype->size <= 8))
			
			cif->flags = (unsigned)FFI_TYPE_DOUBLE;
		else
			
			cif->flags = (unsigned)FFI_TYPE_STRUCT;
		break;
	default:
		cif->flags = (unsigned)FFI_TYPE_INT;
		break;
	}
	return FFI_OK;
}

extern void ffi_call_SYSV(void (*fn)(void), extended_cif *, unsigned, unsigned, double *);









void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
	extended_cif ecif;

	int small_struct = (((cif->flags == FFI_TYPE_INT) || (cif->flags == FFI_TYPE_DOUBLE)) && (cif->rtype->type == FFI_TYPE_STRUCT));
	ecif.cif = cif;
	ecif.avalue = avalue;

	double temp;

	




	if ((rvalue == NULL ) && (cif->flags == FFI_TYPE_STRUCT))
		ecif.rvalue = alloca(cif->rtype->size);
	else if (small_struct)
		ecif.rvalue = &temp;
	else
		ecif.rvalue = rvalue;

	switch (cif->abi) {
	case FFI_SYSV:
		ffi_call_SYSV(fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
		break;
	default:
		FFI_ASSERT(0);
		break;
	}

	if (small_struct)
		memcpy (rvalue, &temp, cif->rtype->size);
}



static void ffi_prep_incoming_args_SYSV (char *, void **, void **,
	ffi_cif*, float *);

void ffi_closure_SYSV (ffi_closure *);


extern unsigned int ffi_metag_trampoline[10]; 










void ffi_init_trampoline(unsigned char *__tramp, unsigned int __fun, unsigned int __ctx) {
	memcpy (__tramp, ffi_metag_trampoline, sizeof(ffi_metag_trampoline));
	*(unsigned int*) &__tramp[40] = __ctx;
	*(unsigned int*) &__tramp[44] = __fun;
	
	__builtin_meta2_cachewd(&__tramp[0], 1);
	__builtin_meta2_cachewd(&__tramp[47], 1);
}





ffi_status
ffi_prep_closure_loc (ffi_closure *closure,
	ffi_cif* cif,
	void (*fun)(ffi_cif*,void*,void**,void*),
	void *user_data,
	void *codeloc)
{
	void (*closure_func)(ffi_closure*) = NULL;

	if (cif->abi == FFI_SYSV)
		closure_func = &ffi_closure_SYSV;
	else
		return FFI_BAD_ABI;

	ffi_init_trampoline(
		(unsigned char*)&closure->tramp[0],
		(unsigned int)closure_func,
		(unsigned int)codeloc);

	closure->cif = cif;
	closure->user_data = user_data;
	closure->fun = fun;

	return FFI_OK;
}



unsigned int ffi_closure_SYSV_inner (closure, respp, args, vfp_args)
	ffi_closure *closure;
	void **respp;
	void *args;
	void *vfp_args;
{
	ffi_cif *cif;
	void **arg_area;

	cif = closure->cif;
	arg_area = (void**) alloca (cif->nargs * sizeof (void*));

	






	ffi_prep_incoming_args_SYSV(args, respp, arg_area, cif, vfp_args);

	(closure->fun) ( cif, *respp, arg_area, closure->user_data);

	return cif->flags;
}

static void ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
	void **avalue, ffi_cif *cif,
	float *vfp_stack)
{
	register unsigned int i;
	register void **p_argv;
	register char *argp;
	register ffi_type **p_arg;

	
	argp = stack;

	
	if ( cif->flags == FFI_TYPE_STRUCT ) {
		argp -= 4;
		*rvalue = *(void **) argp;
	}

	p_argv = avalue;

	for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++) {
		size_t z;
		size_t alignment;

		alignment = (*p_arg)->alignment;
		if (alignment < 4)
			alignment = 4;
		if ((alignment - 1) & (unsigned)argp)
			argp = (char *) ALIGN(argp, alignment);

		z = (*p_arg)->size;
		*p_argv = (void*) argp;
		p_argv++;
		argp -= z;
	}
	return;
}
