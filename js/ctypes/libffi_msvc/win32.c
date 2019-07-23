



































#include <ffi.h>
#include <ffi_common.h>

__declspec(naked) int
ffi_call_x86(void (* prepfunc)(char *, extended_cif *), 
	     extended_cif *ecif, 
	     unsigned bytes, 
	     unsigned flags, 
	     unsigned *rvalue, 
	     void (*fn)()) 
{
	_asm {
		push ebp
		mov ebp, esp

		push esi 

		mov esi, esp		


		mov ecx, [ebp+16]
		sub esp, ecx		
		
		mov eax, esp


		push [ebp + 12] 
		push eax
		call [ebp + 8] 


		add esp, 8


		call [ebp + 28]


		mov ecx, [ebp + 12]
		mov ecx, [ecx]ecif.cif
		mov ecx, [ecx]ecif.cif.abi
		
		cmp ecx, FFI_STDCALL
		je noclean

		mov ecx, [ebp + 16]
		add esp, ecx

noclean:

		sub esi, esp


		mov ecx, [ebp + 20]






		cmp DWORD PTR [ebp + 24], 0
		jne sc_retint



		cmp ecx, FFI_TYPE_FLOAT
		jne sc_noretval

		fstp st(0)

		jmp sc_epilogue

sc_retint:
		cmp ecx, FFI_TYPE_INT
		jne sc_retfloat

		mov ecx, [ebp + 24]
		mov [ecx + 0], eax
		jmp sc_epilogue

sc_retfloat:
		cmp ecx, FFI_TYPE_FLOAT
		jne sc_retdouble

		mov ecx, [ebp+24]

		fstp DWORD PTR [ecx]
		jmp sc_epilogue

sc_retdouble:
		cmp ecx, FFI_TYPE_DOUBLE
		jne sc_retlongdouble

		mov ecx, [ebp+24]
		fstp QWORD PTR [ecx]
		jmp sc_epilogue

		jmp sc_retlongdouble 
sc_retlongdouble:
		cmp ecx, FFI_TYPE_LONGDOUBLE
		jne sc_retint64

		mov ecx, [ebp+24]

		fstp QWORD PTR [ecx] 
		jmp sc_epilogue

sc_retint64:
		cmp ecx, FFI_TYPE_SINT64
		jne sc_retstruct

		mov ecx, [ebp+24]
		mov [ecx+0], eax
		mov [ecx+4], edx

sc_retstruct:


sc_noretval:
sc_epilogue:
		mov eax, esi
		pop esi 
		mov esp, ebp
		pop ebp
		ret
	}
}
