







































#include "mpi-priv.h"

























__declspec(naked) void
s_mpv_mul_d(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
  __asm {
    push   ebp
    mov    ebp,esp
    sub    esp,28
    push   edi
    push   esi
    push   ebx
    mov    ebx,0		; carry = 0
    mov    ecx,[ebp+12]		; ecx = a_len
    mov    edi,[ebp+20]
    cmp    ecx,0
    je     L_2			; jmp if a_len == 0
    mov    esi,[ebp+8]		; esi = a
    cld
L_1:
    lodsd			; eax = [ds:esi]; esi += 4
    mov    edx,[ebp+16]		; edx = b
    mul    edx			; edx:eax = Phi:Plo = a_i * b

    add    eax,ebx		; add carry (ebx) to edx:eax
    adc    edx,0
    mov    ebx,edx		; high half of product becomes next carry

    stosd			; [es:edi] = ax; edi += 4;
    dec    ecx			; --a_len
    jnz    L_1			; jmp if a_len != 0
L_2:
    mov    [edi],ebx		; *c = carry
    pop    ebx
    pop    esi
    pop    edi
    leave  
    ret    
    nop
  }
}

























__declspec(naked) void
s_mpv_mul_d_add(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
  __asm {
    push   ebp
    mov    ebp,esp
    sub    esp,28
    push   edi
    push   esi
    push   ebx
    mov    ebx,0		; carry = 0
    mov    ecx,[ebp+12]		; ecx = a_len
    mov    edi,[ebp+20]
    cmp    ecx,0
    je     L_4			; jmp if a_len == 0
    mov    esi,[ebp+8]		; esi = a
    cld
L_3:
    lodsd			; eax = [ds:esi]; esi += 4
    mov    edx,[ebp+16]		; edx = b
    mul    edx			; edx:eax = Phi:Plo = a_i * b

    add    eax,ebx		; add carry (ebx) to edx:eax
    adc    edx,0
    mov    ebx,[edi]		; add in current word from *c
    add    eax,ebx		
    adc    edx,0
    mov    ebx,edx		; high half of product becomes next carry

    stosd			; [es:edi] = ax; edi += 4;
    dec    ecx			; --a_len
    jnz    L_3			; jmp if a_len != 0
L_4:
    mov    [edi],ebx		; *c = carry
    pop    ebx
    pop    esi
    pop    edi
    leave  
    ret    
    nop
  }
}

























__declspec(naked) void
s_mpv_mul_d_add_prop(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
  __asm {
    push   ebp
    mov    ebp,esp
    sub    esp,28
    push   edi
    push   esi
    push   ebx
    mov    ebx,0		; carry = 0
    mov    ecx,[ebp+12]		; ecx = a_len
    mov    edi,[ebp+20]
    cmp    ecx,0
    je     L_6			; jmp if a_len == 0
    cld
    mov    esi,[ebp+8]		; esi = a
L_5:
    lodsd			; eax = [ds:esi]; esi += 4
    mov    edx,[ebp+16]		; edx = b
    mul    edx			; edx:eax = Phi:Plo = a_i * b

    add    eax,ebx		; add carry (ebx) to edx:eax
    adc    edx,0
    mov    ebx,[edi]		; add in current word from *c
    add    eax,ebx		
    adc    edx,0
    mov    ebx,edx		; high half of product becomes next carry

    stosd			; [es:edi] = ax; edi += 4;
    dec    ecx			; --a_len
    jnz    L_5			; jmp if a_len != 0
L_6:
    cmp    ebx,0		; is carry zero?
    jz     L_8
    mov    eax,[edi]		; add in current word from *c
    add    eax,ebx
    stosd			; [es:edi] = ax; edi += 4;
    jnc    L_8
L_7:
    mov    eax,[edi]		; add in current word from *c
    adc    eax,0
    stosd			; [es:edi] = ax; edi += 4;
    jc     L_7
L_8:
    pop    ebx
    pop    esi
    pop    edi
    leave  
    ret    
    nop
  }
}





















__declspec(naked) void
s_mpv_sqr_add_prop(const mp_digit *a, mp_size a_len, mp_digit *sqrs)
{
  __asm {
     push   ebp
     mov    ebp,esp
     sub    esp,12
     push   edi
     push   esi
     push   ebx
     mov    ebx,0		; carry = 0
     mov    ecx,[ebp+12]	; a_len
     mov    edi,[ebp+16]	; edi = ps
     cmp    ecx,0
     je     L_11		; jump if a_len == 0
     cld
     mov    esi,[ebp+8]		; esi = pa
L_10:
     lodsd			; eax = [ds:si]; si += 4;
     mul    eax

     add    eax,ebx		; add "carry"
     adc    edx,0
     mov    ebx,[edi]
     add    eax,ebx		; add low word from result
     mov    ebx,[edi+4]
     stosd			; [es:di] = eax; di += 4;
     adc    edx,ebx		; add high word from result
     mov    ebx,0
     mov    eax,edx
     adc    ebx,0
     stosd			; [es:di] = eax; di += 4;
     dec    ecx			; --a_len
     jnz    L_10		; jmp if a_len != 0
L_11:
    cmp    ebx,0		; is carry zero?
    jz     L_14
    mov    eax,[edi]		; add in current word from *c
    add    eax,ebx
    stosd			; [es:edi] = ax; edi += 4;
    jnc    L_14
L_12:
    mov    eax,[edi]		; add in current word from *c
    adc    eax,0
    stosd			; [es:edi] = ax; edi += 4;
    jc     L_12
L_14:
    pop    ebx
    pop    esi
    pop    edi
    leave  
    ret    
    nop
  }
}





















  
__declspec(naked) mp_err
s_mpv_div_2dx1d(mp_digit Nhi, mp_digit Nlo, mp_digit divisor,
		mp_digit *qp, mp_digit *rp)
{
  __asm {
       push   ebx
       mov    edx,[esp+8]
       mov    eax,[esp+12]
       mov    ebx,[esp+16]
       div    ebx
       mov    ebx,[esp+20]
       mov    [ebx],eax
       mov    ebx,[esp+24]
       mov    [ebx],edx
       xor    eax,eax		; return zero
       pop    ebx
       ret    
       nop
  }
}
