






#include "ffitest.h"

static	long dummyVar;

long dummy_func(
	long double a1, char b1,
	long double a2, char b2,
	long double a3, char b3,
	long double a4, char b4)
{
	return a1 + b1 + a2 + b2 + a3 + b3 + a4 + b4;
}

void* cls_pointer_fn2(void* a1, void* a2)
{
	long double	trample1	= (intptr_t)a1 + (intptr_t)a2;
	char		trample2	= ((char*)&a1)[0] + ((char*)&a2)[0];
	long double	trample3	= (intptr_t)trample1 + (intptr_t)a1;
	char		trample4	= trample2 + ((char*)&a1)[1];
	long double	trample5	= (intptr_t)trample3 + (intptr_t)a2;
	char		trample6	= trample4 + ((char*)&a2)[1];
	long double	trample7	= (intptr_t)trample5 + (intptr_t)trample1;
	char		trample8	= trample6 + trample2;
	void*		result;

	dummyVar	= dummy_func(trample1, trample2, trample3, trample4,
		trample5, trample6, trample7, trample8);

	result	= (void*)((intptr_t)a1 + (intptr_t)a2);

	printf("0x%08x 0x%08x: 0x%08x\n", 
	       (unsigned int)(uintptr_t) a1,
               (unsigned int)(uintptr_t) a2,
               (unsigned int)(uintptr_t) result);

	return result;
}

void* cls_pointer_fn1(void* a1, void* a2)
{
	long double	trample1	= (intptr_t)a1 + (intptr_t)a2;
	char		trample2	= ((char*)&a1)[0] + ((char*)&a2)[0];
	long double	trample3	= (intptr_t)trample1 + (intptr_t)a1;
	char		trample4	= trample2 + ((char*)&a1)[1];
	long double	trample5	= (intptr_t)trample3 + (intptr_t)a2;
	char		trample6	= trample4 + ((char*)&a2)[1];
	long double	trample7	= (intptr_t)trample5 + (intptr_t)trample1;
	char		trample8	= trample6 + trample2;
	void*		result;

	dummyVar	= dummy_func(trample1, trample2, trample3, trample4,
		trample5, trample6, trample7, trample8);

	result	= (void*)((intptr_t)a1 + (intptr_t)a2);

	printf("0x%08x 0x%08x: 0x%08x\n",
               (unsigned int)(intptr_t) a1,
               (unsigned int)(intptr_t) a2,
               (unsigned int)(intptr_t) result);

	result	= cls_pointer_fn2(result, a1);

	return result;
}

static void
cls_pointer_gn(ffi_cif* cif __UNUSED__, void* resp, 
	       void** args, void* userdata __UNUSED__)
{
	void*	a1	= *(void**)(args[0]);
	void*	a2	= *(void**)(args[1]);

	long double	trample1	= (intptr_t)a1 + (intptr_t)a2;
	char		trample2	= ((char*)&a1)[0] + ((char*)&a2)[0];
	long double	trample3	= (intptr_t)trample1 + (intptr_t)a1;
	char		trample4	= trample2 + ((char*)&a1)[1];
	long double	trample5	= (intptr_t)trample3 + (intptr_t)a2;
	char		trample6	= trample4 + ((char*)&a2)[1];
	long double	trample7	= (intptr_t)trample5 + (intptr_t)trample1;
	char		trample8	= trample6 + trample2;

	dummyVar	= dummy_func(trample1, trample2, trample3, trample4,
		trample5, trample6, trample7, trample8);

	*(void**)resp = cls_pointer_fn1(a1, a2);
}

int main (void)
{
	ffi_cif	cif;
        void *code;
	ffi_closure*	pcl = ffi_closure_alloc(sizeof(ffi_closure), &code);
	void*			args[3];
	
	ffi_type*		arg_types[3];






	void*	arg1	= (void*)0x01234567;
	void*	arg2	= (void*)0x89abcdef;
	ffi_arg	res		= 0;

	arg_types[0] = &ffi_type_pointer;
	arg_types[1] = &ffi_type_pointer;
	arg_types[2] = NULL;

	CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 2, &ffi_type_pointer,
		arg_types) == FFI_OK);

	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = NULL;

	printf("\n");
	ffi_call(&cif, FFI_FN(cls_pointer_fn1), &res, args);

	printf("res: 0x%08x\n", (unsigned int) res);
	
	
	

	CHECK(ffi_prep_closure_loc(pcl, &cif, cls_pointer_gn, NULL, code) == FFI_OK);

	res = (ffi_arg)(uintptr_t)((void*(*)(void*, void*))(code))(arg1, arg2);

	printf("res: 0x%08x\n", (unsigned int) res);
	
	
	

	exit(0);
}
