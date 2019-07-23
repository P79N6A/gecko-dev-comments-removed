




#include "zutil.h"
#include "inftrees.h"

#if !defined(BUILDFIXED) && !defined(STDC)
#  define BUILDFIXED
#endif


#if 0
local const char inflate_copyright[] =
   " inflate 1.1.4 Copyright 1995-2002 Mark Adler ";
#endif








#define exop word.what.Exop
#define bits word.what.Bits


local int huft_build OF((
    uIntf *,            
    uInt,               
    uInt,               
    const uIntf *,      
    const uIntf *,      
    inflate_huft * FAR*,
    uIntf *,            
    inflate_huft *,     
    uInt *,             
    uIntf * ));         


local const uInt cplens[31] = { 
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        
local const uInt cplext[31] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112}; 
local const uInt cpdist[30] = { 
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
local const uInt cpdext[30] = { 
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};



































#define BMAX 15         /* maximum bit length of any code */

local int huft_build( 
uIntf *b,               
uInt n,                 
uInt s,                 
const uIntf *d,         
const uIntf *e,         
inflate_huft * FAR *t,  
uIntf *m,               
inflate_huft *hp,       
uInt *hn,               
uIntf *v                




)
{

  uInt a;                       
  uInt c[BMAX+1];               
  uInt f;                       
  int g;                        
  int h;                        
  register uInt i;              
  register uInt j;              
  register int k;               
  int l;                        
  uInt mask;                    
  register uIntf *p;            
  inflate_huft *q;              
  struct inflate_huft_s r;      
  inflate_huft *u[BMAX];        
  register int w;               
  uInt x[BMAX+1];               
  uIntf *xp;                    
  int y;                        
  uInt z;                       


  
  r.base = 0;

  
  p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
  C4                            
  p = b;  i = n;
  do {
    c[*p++]++;                  
  } while (--i);
  if (c[0] == n)                
  {
    *t = (inflate_huft *)Z_NULL;
    *m = 0;
    return Z_OK;
  }


  
  l = *m;
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  k = j;                        
  if ((uInt)l < j)
    l = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;
  g = i;                        
  if ((uInt)l > i)
    l = i;
  *m = l;


  
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return Z_DATA_ERROR;
  if ((y -= c[i]) < 0)
    return Z_DATA_ERROR;
  c[i] += y;


  
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 
    *xp++ = (j += *p++);
  }


  
  p = b;  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);
  n = x[g];                     


  
  x[0] = i = 0;                 
  p = v;                        
  h = -1;                       
  w = -l;                       
  u[0] = (inflate_huft *)Z_NULL;        
  q = (inflate_huft *)Z_NULL;   
  z = 0;                        

  
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      
      
      while (k > w + l)
      {
        h++;
        w += l;                 

        
        z = g - w;
        z = z > (uInt)l ? (uInt)l : z;        
        if ((f = 1 << (j = k - w)) > a + 1)     
        {                       
          f -= a + 1;           
          xp = c + k;
          if (j < z)
            while (++j < z)     
            {
              if ((f <<= 1) <= *++xp)
                break;          
              f -= *xp;         
            }
        }
        z = 1 << j;             

        
        if (*hn + z > MANY)     
          return Z_DATA_ERROR;  
        u[h] = q = hp + *hn;
        *hn += z;

        
        if (h)
        {
          x[h] = i;             
          r.bits = (Byte)l;     
          r.exop = (Byte)j;     
          j = i >> (w - l);
          r.base = (uInt)(q - u[h-1] - j);   
          u[h-1][j] = r;        
        }
        else
          *t = q;               
      }

      
      r.bits = (Byte)(k - w);
      if (p >= v + n)
        r.exop = 128 + 64;      
      else if (*p < s)
      {
        r.exop = (Byte)(*p < 256 ? 0 : 32 + 64);     
        r.base = *p++;          
      }
      else
      {
        r.exop = (Byte)(e[*p - s] + 16 + 64);
        r.base = d[*p++ - s];
      }

      
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      
      mask = (1 << w) - 1;      
      while ((i & mask) != x[h])
      {
        h--;                    
        w -= l;
        mask = (1 << w) - 1;
      }
    }
  }


  
  return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}


local int inflate_trees_bits( 
uIntf *c,               
uIntf *bb,              
inflate_huft * FAR *tb, 
inflate_huft *hp,       
z_streamp z             
)
{
  int r;
  uInt hn = 0;          
  uIntf *v;             

  if ((v = (uIntf*)ZALLOC(z, 19, sizeof(uInt))) == Z_NULL)
    return Z_MEM_ERROR;
  r = huft_build(c, 19, 19, (uIntf*)Z_NULL, (uIntf*)Z_NULL,
                 tb, bb, hp, &hn, v);
  if (r == Z_DATA_ERROR)
    z->msg = (char*)"oversubscribed dynamic bit lengths tree";
  else if (r == Z_BUF_ERROR || *bb == 0)
  {
    z->msg = (char*)"incomplete dynamic bit lengths tree";
    r = Z_DATA_ERROR;
  }
  ZFREE(z, v);
  return r;
}


local int inflate_trees_dynamic( 
uInt nl,                
uInt nd,                
uIntf *c,               
uIntf *bl,              
uIntf *bd,              
inflate_huft * FAR *tl, 
inflate_huft * FAR *td, 
inflate_huft *hp,       
z_streamp z             
)
{
  int r;
  uInt hn = 0;          
  uIntf *v;             

  
  if ((v = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
    return Z_MEM_ERROR;

  
  r = huft_build(c, nl, 257, cplens, cplext, tl, bl, hp, &hn, v);
  if (r != Z_OK || *bl == 0)
  {
    if (r == Z_DATA_ERROR)
      z->msg = (char*)"oversubscribed literal/length tree";
    else if (r != Z_MEM_ERROR)
    {
      z->msg = (char*)"incomplete literal/length tree";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
  }

  
  r = huft_build(c + nl, nd, 0, cpdist, cpdext, td, bd, hp, &hn, v);
  if (r != Z_OK || (*bd == 0 && nl > 257))
  {
    if (r == Z_DATA_ERROR)
      z->msg = (char*)"oversubscribed distance tree";
    else if (r == Z_BUF_ERROR) {
#ifdef PKZIP_BUG_WORKAROUND
      r = Z_OK;
    }
#else
      z->msg = (char*)"incomplete distance tree";
      r = Z_DATA_ERROR;
    }
    else if (r != Z_MEM_ERROR)
    {
      z->msg = (char*)"empty distance tree with lengths";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
#endif
  }

  
  ZFREE(z, v);
  return Z_OK;
}



#ifdef BUILDFIXED
local int fixed_built = 0;
#define FIXEDH 544      /* number of hufts used by fixed tables */
local inflate_huft fixed_mem[FIXEDH];
local uInt fixed_bl;
local uInt fixed_bd;
local inflate_huft *fixed_tl;
local inflate_huft *fixed_td;
#else
#include "inffixed.h"
#endif


local int inflate_trees_fixed( 
uIntf *bl,                      
uIntf *bd,                      
const inflate_huft * FAR *tl,   
const inflate_huft * FAR *td,   
z_streamp z                     
)
{
#ifdef BUILDFIXED
  
  if (!fixed_built)
  {
    int k;              
    uInt f = 0;         
    uIntf *c;           
    uIntf *v;           

    
    if ((c = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
      return Z_MEM_ERROR;
    if ((v = (uIntf*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
    {
      ZFREE(z, c);
      return Z_MEM_ERROR;
    }

    
    for (k = 0; k < 144; k++)
      c[k] = 8;
    for (; k < 256; k++)
      c[k] = 9;
    for (; k < 280; k++)
      c[k] = 7;
    for (; k < 288; k++)
      c[k] = 8;
    fixed_bl = 9;
    huft_build(c, 288, 257, cplens, cplext, &fixed_tl, &fixed_bl,
               fixed_mem, &f, v);

    
    for (k = 0; k < 30; k++)
      c[k] = 5;
    fixed_bd = 5;
    huft_build(c, 30, 0, cpdist, cpdext, &fixed_td, &fixed_bd,
               fixed_mem, &f, v);

    
    ZFREE(z, v);
    ZFREE(z, c);
    fixed_built = 1;
  }
#else
  FT_UNUSED(z);
#endif
  *bl = fixed_bl;
  *bd = fixed_bd;
  *tl = fixed_tl;
  *td = fixed_td;
  return Z_OK;
}
