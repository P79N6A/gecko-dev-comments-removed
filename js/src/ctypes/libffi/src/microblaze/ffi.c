

























#include <ffi.h>
#include <ffi_common.h>

extern void ffi_call_SYSV(void (*)(void*, extended_cif*), extended_cif*,
		unsigned int, unsigned int, unsigned int*, void (*fn)(void),
		unsigned int, unsigned int);

extern void ffi_closure_SYSV(void);

#define WORD_SIZE			sizeof(unsigned int)
#define ARGS_REGISTER_SIZE	(WORD_SIZE * 6)
#define WORD_ALIGN(x)		ALIGN(x, WORD_SIZE)



void ffi_prep_args(void* stack, extended_cif* ecif)
{
	unsigned int i;
	ffi_type** p_arg;
	void** p_argv;
	void* stack_args_p = stack;

	p_argv = ecif->avalue;

	if (ecif == NULL || ecif->cif == NULL) {
		return; 
	}

	if ((ecif->cif->rtype != NULL) &&
			(ecif->cif->rtype->type == FFI_TYPE_STRUCT))
	{
		


		char* addr = stack_args_p;
		memcpy(addr, &(ecif->rvalue), WORD_SIZE);
		stack_args_p += WORD_SIZE;
	}

	if (ecif->avalue == NULL) {
		return; 
	}

	for (i = 0, p_arg = ecif->cif->arg_types; i < ecif->cif->nargs;
			i++, p_arg++)
	{
		size_t size = (*p_arg)->size;
		int type = (*p_arg)->type;
		void* value = p_argv[i];
		char* addr = stack_args_p;
		int aligned_size = WORD_ALIGN(size);

		
		stack_args_p += aligned_size;
		
		switch (type)
		{
			case FFI_TYPE_UINT8:
				*(unsigned int *)addr = (unsigned int)*(UINT8*)(value);
				break;
			case FFI_TYPE_SINT8:
				*(signed int *)addr = (signed int)*(SINT8*)(value);
				break;
			case FFI_TYPE_UINT16:
				*(unsigned int *)addr = (unsigned int)*(UINT16*)(value);
				break;
			case FFI_TYPE_SINT16:
				*(signed int *)addr = (signed int)*(SINT16*)(value);
				break;
			case FFI_TYPE_STRUCT:
#if __BIG_ENDIAN__
				





















				if (size < WORD_SIZE)
				{
				  memcpy (addr + (WORD_SIZE - size), value, size);
				  break;
				}
#endif
			case FFI_TYPE_SINT32:
			case FFI_TYPE_UINT32:
			case FFI_TYPE_FLOAT:
			case FFI_TYPE_SINT64:
			case FFI_TYPE_UINT64:
			case FFI_TYPE_DOUBLE:
			default:
				memcpy(addr, value, aligned_size);
		}
	}
}

ffi_status ffi_prep_cif_machdep(ffi_cif* cif)
{
	
	switch (cif->abi)
	{
		case FFI_SYSV:
			break;
		default:
			return FFI_BAD_ABI;
	}
	return FFI_OK;
}

void ffi_call(ffi_cif* cif, void (*fn)(void), void* rvalue, void** avalue)
{
	extended_cif ecif;
	ecif.cif = cif;
	ecif.avalue = avalue;

	
	
	if ((rvalue == NULL) && (cif->rtype->type == FFI_TYPE_STRUCT)) {
		ecif.rvalue = alloca(cif->rtype->size);
	} else {
		ecif.rvalue = rvalue;
	}

	switch (cif->abi)
	{
	case FFI_SYSV:
		ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, cif->flags,
				ecif.rvalue, fn, cif->rtype->type, cif->rtype->size);
		break;
	default:
		FFI_ASSERT(0);
		break;
	}
}

void ffi_closure_call_SYSV(void* register_args, void* stack_args,
			ffi_closure* closure, void* rvalue,
			unsigned int* rtype, unsigned int* rsize)
{
	
	ffi_cif* cif = closure->cif;
	ffi_type** arg_types = cif->arg_types;

	




	char* stackclone = alloca(cif->bytes);
	void** avalue = alloca(cif->nargs * sizeof(void*));
	void* struct_rvalue = NULL;
	char* ptr = stackclone;
	int i;

	
	int registers_used = cif->bytes;
	if (registers_used > ARGS_REGISTER_SIZE) {
		registers_used = ARGS_REGISTER_SIZE;
	}
	memcpy(stackclone, register_args, registers_used);

	
	if (cif->bytes > ARGS_REGISTER_SIZE) {
		int stack_used = cif->bytes - ARGS_REGISTER_SIZE;
		memcpy(stackclone + ARGS_REGISTER_SIZE, stack_args, stack_used);
	}

	
	if ((cif->rtype != NULL) && (cif->rtype->type == FFI_TYPE_STRUCT)) {
		struct_rvalue = *((void**)ptr);
		ptr += WORD_SIZE;
	}

	
	for (i = 0; i < cif->nargs; i++)
	{
		switch (arg_types[i]->type)
		{
			case FFI_TYPE_SINT8:
			case FFI_TYPE_UINT8:
#ifdef __BIG_ENDIAN__
				avalue[i] = ptr + 3;
#else
				avalue[i] = ptr;
#endif
				break;
			case FFI_TYPE_SINT16:
			case FFI_TYPE_UINT16:
#ifdef __BIG_ENDIAN__
				avalue[i] = ptr + 2;
#else
				avalue[i] = ptr;
#endif
				break;
			case FFI_TYPE_STRUCT:
#if __BIG_ENDIAN__
				



				if (arg_types[i]->size < WORD_SIZE)
				{
				  memcpy (ptr, ptr + (WORD_SIZE - arg_types[i]->size), arg_types[i]->size);
				}
#endif
				avalue[i] = (void*)ptr;
				break;
			case FFI_TYPE_UINT64:
			case FFI_TYPE_SINT64:
			case FFI_TYPE_DOUBLE:
				avalue[i] = ptr;
				break;
			case FFI_TYPE_SINT32:
			case FFI_TYPE_UINT32:
			case FFI_TYPE_FLOAT:
			default:
				
				avalue[i] = ptr;
				break;
		}
		ptr += WORD_ALIGN(arg_types[i]->size);
	}

	
	*rsize = cif->rtype->size;
	*rtype = cif->rtype->type;
	if (struct_rvalue != NULL) {
		closure->fun(cif, struct_rvalue, avalue, closure->user_data);
		
		*((void**)rvalue) = struct_rvalue;
	} else {
		closure->fun(cif, rvalue, avalue, closure->user_data);
	}
}

ffi_status ffi_prep_closure_loc(
		ffi_closure* closure, ffi_cif* cif,
		void (*fun)(ffi_cif*, void*, void**, void*),
		void* user_data, void* codeloc)
{
	unsigned long* tramp = (unsigned long*)&(closure->tramp[0]);
	unsigned long cls = (unsigned long)codeloc;
	unsigned long fn = 0;
	unsigned long fn_closure_call_sysv = (unsigned long)ffi_closure_call_SYSV;

	closure->cif = cif;
	closure->fun = fun;
	closure->user_data = user_data;

	switch (cif->abi)
	{
	case FFI_SYSV:
		fn = (unsigned long)ffi_closure_SYSV;

		
		
		tramp[0] = 0xb0000000 | ((fn >> 16) & 0xffff);
		
		tramp[1] = 0x31600000 | (fn & 0xffff);

		
		
		tramp[2] = 0xb0000000 | ((cls >> 16) & 0xffff);
		
		tramp[3] = 0x31800000 | (cls & 0xffff);

		
		
		tramp[4] = 0xb0000000 | ((fn_closure_call_sysv >> 16) & 0xffff);
		
		tramp[5] = 0x30600000 | (fn_closure_call_sysv & 0xffff);
		
		tramp[6] = 0x98085800; 

		break;
	default:
		return FFI_BAD_ABI;
	}
	return FFI_OK;
}
