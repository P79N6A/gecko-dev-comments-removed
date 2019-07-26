#ifndef _HTYPES_HXX_
#define _HTYPES_HXX_

#define ROTATE_LEN   5

#define ROTATE(v,q) \
   (v) = ((v) << (q)) | (((v) >> (32 - q)) & ((1 << (q))-1));


#define H_OPT        (1 << 0)
#define H_OPT_ALIASM (1 << 1)
#define H_OPT_PHON   (1 << 2)


#define HENTRY_WORD(h) &(h->word[0])


#define USERWORD 1000

struct hentry
{
  unsigned char blen; 
  unsigned char clen; 
  short    alen;      
  unsigned short * astr;  
  struct   hentry * next; 
  struct   hentry * next_homonym; 
  char     var;       
  char     word[1];   
};

#endif
