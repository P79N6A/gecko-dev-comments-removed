




































#include <stdio.h>

typedef unsigned nsresult;
typedef unsigned PRUint32;
typedef unsigned nsXPCVariant;


#if defined(WIN32)
#define NS_IMETHOD virtual nsresult __stdcall
#define NS_IMETHODIMP nsresult __stdcall
#else
#define NS_IMETHOD virtual nsresult
#define NS_IMETHODIMP nsresult
#endif


class base{
public:
  NS_IMETHOD ignored() = 0;
};

class foo : public base {
public:
  NS_IMETHOD callme1(int i, int j) = 0;
  NS_IMETHOD callme2(int i, int j) = 0;
  NS_IMETHOD callme3(int i, int j) = 0;
};

class bar : public foo{
public:
  NS_IMETHOD ignored();
  NS_IMETHOD callme1(int i, int j);
  NS_IMETHOD callme2(int i, int j);
  NS_IMETHOD callme3(int i, int j);
};















NS_IMETHODIMP bar::ignored(){return 0;}

NS_IMETHODIMP bar::callme1(int i, int j)
{
  printf("called bar::callme1 with: %d %d\n", i, j);
  return 5;
}

NS_IMETHODIMP bar::callme2(int i, int j)
{
  printf("called bar::callme2 with: %d %d\n", i, j);
  return 5;
}

NS_IMETHODIMP bar::callme3(int i, int j)
{
  printf("called bar::callme3 with: %d %d\n", i, j);
  return 5;
}

void docall(foo* f, int i, int j){
  f->callme1(i, j); 
}


#if defined(WIN32)

static PRUint32 __stdcall
invoke_count_words(PRUint32 paramCount, nsXPCVariant* s)
{
    return paramCount;
}    

static void __stdcall
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPCVariant* s)
{
    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        *((PRUint32*)d) = *((PRUint32*)s);
    }
}

static nsresult __stdcall
DoInvoke(void* that, PRUint32 index,
         PRUint32 paramCount, nsXPCVariant* params)
{
    __asm {
        push    params
        push    paramCount
        call    invoke_count_words  
        shl     eax,2               
        sub     esp,eax             
        mov     edx,esp
        push    params
        push    paramCount
        push    edx
        call    invoke_copy_to_stack 
        mov     ecx,that            
        push    ecx                 
        mov     edx,[ecx]           
        mov     eax,index
        shl     eax,2               
        add     edx,eax
        call    [edx]               
    }
}

#else



static PRUint32 
invoke_count_words(PRUint32 paramCount, nsXPCVariant* s)
{
    return paramCount;
}    

static void 
invoke_copy_to_stack(PRUint32* d, PRUint32 paramCount, nsXPCVariant* s)
{
    for(PRUint32 i = 0; i < paramCount; i++, d++, s++)
    {
        *((PRUint32*)d) = *((PRUint32*)s);
    }
}

static nsresult
DoInvoke(void* that, PRUint32 index,
         PRUint32 paramCount, nsXPCVariant* params)
{
    PRUint32 result;
    void* fn_count = invoke_count_words;
    void* fn_copy = invoke_copy_to_stack;

 __asm__ __volatile__(
    "pushl %4\n\t"
    "pushl %3\n\t"
    "movl  %5, %%eax\n\t"
    "call  *%%eax\n\t"       
    "addl  $0x8, %%esp\n\t"
    "shl   $2, %%eax\n\t"    
    "subl  %%eax, %%esp\n\t" 
    "movl  %%esp, %%edx\n\t"
    "pushl %4\n\t"
    "pushl %3\n\t"
    "pushl %%edx\n\t"
    "movl  %6, %%eax\n\t"
    "call  *%%eax\n\t"       
    "addl  $0xc, %%esp\n\t"
    "movl  %1, %%ecx\n\t"
    "pushl %%ecx\n\t"
    "movl  (%%ecx), %%edx\n\t"
    "movl  %2, %%eax\n\t"   
    "shl   $2, %%eax\n\t"   
    "addl  $8, %%eax\n\t"   
    "addl  %%eax, %%edx\n\t"
    "call  *(%%edx)\n\t"    
    "movl  %%eax, %0"
    : "=g" (result)         
    : "g" (that),           
      "g" (index),          
      "g" (paramCount),     
      "g" (params),         
      "g" (fn_count),       
      "g" (fn_copy)         
    : "ax", "cx", "dx", "memory" 
    );
  
  return result;
}    

#endif


int main()
{
  nsXPCVariant params1[2] = {1,2};
  nsXPCVariant params2[2] = {2,4};
  nsXPCVariant params3[2] = {3,6};

  foo* a = new bar();




  printf("calling via ASM...\n");
  DoInvoke(a, 1, 2, params1);
  DoInvoke(a, 2, 2, params2);
  DoInvoke(a, 3, 2, params3);

  return 0;
}
