



















































#include "sqliteInt.h"





#define etRADIX       1 /* Integer types.  %d, %x, %o, and so forth */
#define etFLOAT       2 /* Floating point.  %f */
#define etEXP         3 /* Exponentional notation. %e and %E */
#define etGENERIC     4 /* Floating or exponential, depending on exponent. %g */
#define etSIZE        5 /* Return number of characters processed so far. %n */
#define etSTRING      6 /* Strings. %s */
#define etDYNSTRING   7 /* Dynamically allocated strings. %z */
#define etPERCENT     8 /* Percent symbol. %% */
#define etCHARX       9 /* Characters. %c */

#define etCHARLIT    10 /* Literal characters.  %' */
#define etSQLESCAPE  11 /* Strings with '\'' doubled.  %q */
#define etSQLESCAPE2 12 /* Strings with '\'' doubled and enclosed in '',
                          NULL pointers replaced by SQL NULL.  %Q */
#define etTOKEN      13 /* a pointer to a Token structure */
#define etSRCLIST    14 /* a pointer to a SrcList */
#define etPOINTER    15 /* The %p conversion */





typedef unsigned char etByte;





typedef struct et_info {   
  char fmttype;            
  etByte base;             
  etByte flags;            
  etByte type;             
  etByte charset;          
  etByte prefix;           
} et_info;




#define FLAG_SIGNED  1     /* True if the value to convert is signed */
#define FLAG_INTERN  2     /* True if for internal use only */
#define FLAG_STRING  4     /* Allow infinity precision */






static const char aDigits[] = "0123456789ABCDEF0123456789abcdef";
static const char aPrefix[] = "-x0\000X0";
static const et_info fmtinfo[] = {
  {  'd', 10, 1, etRADIX,      0,  0 },
  {  's',  0, 4, etSTRING,     0,  0 },
  {  'g',  0, 1, etGENERIC,    30, 0 },
  {  'z',  0, 6, etDYNSTRING,  0,  0 },
  {  'q',  0, 4, etSQLESCAPE,  0,  0 },
  {  'Q',  0, 4, etSQLESCAPE2, 0,  0 },
  {  'c',  0, 0, etCHARX,      0,  0 },
  {  'o',  8, 0, etRADIX,      0,  2 },
  {  'u', 10, 0, etRADIX,      0,  0 },
  {  'x', 16, 0, etRADIX,      16, 1 },
  {  'X', 16, 0, etRADIX,      0,  4 },
#ifndef SQLITE_OMIT_FLOATING_POINT
  {  'f',  0, 1, etFLOAT,      0,  0 },
  {  'e',  0, 1, etEXP,        30, 0 },
  {  'E',  0, 1, etEXP,        14, 0 },
  {  'G',  0, 1, etGENERIC,    14, 0 },
#endif
  {  'i', 10, 1, etRADIX,      0,  0 },
  {  'n',  0, 0, etSIZE,       0,  0 },
  {  '%',  0, 0, etPERCENT,    0,  0 },
  {  'p', 16, 0, etPOINTER,    0,  1 },
  {  'T',  0, 2, etTOKEN,      0,  0 },
  {  'S',  0, 2, etSRCLIST,    0,  0 },
};
#define etNINFO  (sizeof(fmtinfo)/sizeof(fmtinfo[0]))





#ifndef SQLITE_OMIT_FLOATING_POINT













static int et_getdigit(LONGDOUBLE_TYPE *val, int *cnt){
  int digit;
  LONGDOUBLE_TYPE d;
  if( (*cnt)++ >= 16 ) return '0';
  digit = (int)*val;
  d = digit;
  digit += '0';
  *val = (*val - d)*10.0;
  return digit;
}
#endif 






#ifndef SQLITE_PRINT_BUF_SIZE
# define SQLITE_PRINT_BUF_SIZE 350
#endif
#define etBUFSIZE SQLITE_PRINT_BUF_SIZE  /* Size of the output buffer */




























static int vxprintf(
  void (*func)(void*,const char*,int),     
  void *arg,                         
  int useExtended,                   
  const char *fmt,                   
  va_list ap                         
){
  int c;                     
  char *bufpt;               
  int precision;             
  int length;                
  int idx;                   
  int count;                 
  int width;                 
  etByte flag_leftjustify;   
  etByte flag_plussign;      
  etByte flag_blanksign;     
  etByte flag_alternateform; 
  etByte flag_altform2;      
  etByte flag_zeropad;       
  etByte flag_long;          
  etByte flag_longlong;      
  etByte done;               
  sqlite_uint64 longvalue;   
  LONGDOUBLE_TYPE realvalue; 
  const et_info *infop;      
  char buf[etBUFSIZE];       
  char prefix;               
  etByte errorflag = 0;      
  etByte xtype;              
  char *zExtra;              
  static const char spaces[] =
   "                                                                         ";
#define etSPACESIZE (sizeof(spaces)-1)
#ifndef SQLITE_OMIT_FLOATING_POINT
  int  exp, e2;              
  double rounder;            
  etByte flag_dp;            
  etByte flag_rtz;           
  etByte flag_exp;           
  int nsd;                   
#endif

  func(arg,"",0);
  count = length = 0;
  bufpt = 0;
  for(; (c=(*fmt))!=0; ++fmt){
    if( c!='%' ){
      int amt;
      bufpt = (char *)fmt;
      amt = 1;
      while( (c=(*++fmt))!='%' && c!=0 ) amt++;
      (*func)(arg,bufpt,amt);
      count += amt;
      if( c==0 ) break;
    }
    if( (c=(*++fmt))==0 ){
      errorflag = 1;
      (*func)(arg,"%",1);
      count++;
      break;
    }
    
    flag_leftjustify = flag_plussign = flag_blanksign = 
     flag_alternateform = flag_altform2 = flag_zeropad = 0;
    done = 0;
    do{
      switch( c ){
        case '-':   flag_leftjustify = 1;     break;
        case '+':   flag_plussign = 1;        break;
        case ' ':   flag_blanksign = 1;       break;
        case '#':   flag_alternateform = 1;   break;
        case '!':   flag_altform2 = 1;        break;
        case '0':   flag_zeropad = 1;         break;
        default:    done = 1;                 break;
      }
    }while( !done && (c=(*++fmt))!=0 );
    
    width = 0;
    if( c=='*' ){
      width = va_arg(ap,int);
      if( width<0 ){
        flag_leftjustify = 1;
        width = -width;
      }
      c = *++fmt;
    }else{
      while( c>='0' && c<='9' ){
        width = width*10 + c - '0';
        c = *++fmt;
      }
    }
    if( width > etBUFSIZE-10 ){
      width = etBUFSIZE-10;
    }
    
    if( c=='.' ){
      precision = 0;
      c = *++fmt;
      if( c=='*' ){
        precision = va_arg(ap,int);
        if( precision<0 ) precision = -precision;
        c = *++fmt;
      }else{
        while( c>='0' && c<='9' ){
          precision = precision*10 + c - '0';
          c = *++fmt;
        }
      }
    }else{
      precision = -1;
    }
    
    if( c=='l' ){
      flag_long = 1;
      c = *++fmt;
      if( c=='l' ){
        flag_longlong = 1;
        c = *++fmt;
      }else{
        flag_longlong = 0;
      }
    }else{
      flag_long = flag_longlong = 0;
    }
    
    infop = 0;
    for(idx=0; idx<etNINFO; idx++){
      if( c==fmtinfo[idx].fmttype ){
        infop = &fmtinfo[idx];
        if( useExtended || (infop->flags & FLAG_INTERN)==0 ){
          xtype = infop->type;
        }
        break;
      }
    }
    zExtra = 0;
    if( infop==0 ){
      return -1;
    }


    
    if( precision>etBUFSIZE-40 && (infop->flags & FLAG_STRING)==0 ){
      precision = etBUFSIZE-40;
    }

    




















    switch( xtype ){
      case etPOINTER:
        flag_longlong = sizeof(char*)==sizeof(i64);
        flag_long = sizeof(char*)==sizeof(long int);
        
      case etRADIX:
        if( infop->flags & FLAG_SIGNED ){
          i64 v;
          if( flag_longlong )   v = va_arg(ap,i64);
          else if( flag_long )  v = va_arg(ap,long int);
          else                  v = va_arg(ap,int);
          if( v<0 ){
            longvalue = -v;
            prefix = '-';
          }else{
            longvalue = v;
            if( flag_plussign )        prefix = '+';
            else if( flag_blanksign )  prefix = ' ';
            else                       prefix = 0;
          }
        }else{
          if( flag_longlong )   longvalue = va_arg(ap,u64);
          else if( flag_long )  longvalue = va_arg(ap,unsigned long int);
          else                  longvalue = va_arg(ap,unsigned int);
          prefix = 0;
        }
        if( longvalue==0 ) flag_alternateform = 0;
        if( flag_zeropad && precision<width-(prefix!=0) ){
          precision = width-(prefix!=0);
        }
        bufpt = &buf[etBUFSIZE-1];
        {
          register const char *cset;      
          register int base;
          cset = &aDigits[infop->charset];
          base = infop->base;
          do{                                           
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
        }
        length = &buf[etBUFSIZE-1]-bufpt;
        for(idx=precision-length; idx>0; idx--){
          *(--bufpt) = '0';                             
        }
        if( prefix ) *(--bufpt) = prefix;               
        if( flag_alternateform && infop->prefix ){      
          const char *pre;
          char x;
          pre = &aPrefix[infop->prefix];
          if( *bufpt!=pre[0] ){
            for(; (x=(*pre))!=0; pre++) *(--bufpt) = x;
          }
        }
        length = &buf[etBUFSIZE-1]-bufpt;
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC:
        realvalue = va_arg(ap,double);
#ifndef SQLITE_OMIT_FLOATING_POINT
        if( precision<0 ) precision = 6;         
        if( precision>etBUFSIZE/2-10 ) precision = etBUFSIZE/2-10;
        if( realvalue<0.0 ){
          realvalue = -realvalue;
          prefix = '-';
        }else{
          if( flag_plussign )          prefix = '+';
          else if( flag_blanksign )    prefix = ' ';
          else                         prefix = 0;
        }
        if( xtype==etGENERIC && precision>0 ) precision--;
#if 0
        
        for(idx=precision, rounder=0.4999; idx>0; idx--, rounder*=0.1);
#else
        
        for(idx=precision, rounder=0.5; idx>0; idx--, rounder*=0.1){}
#endif
        if( xtype==etFLOAT ) realvalue += rounder;
        
        exp = 0;
        if( realvalue>0.0 ){
          while( realvalue>=1e32 && exp<=350 ){ realvalue *= 1e-32; exp+=32; }
          while( realvalue>=1e8 && exp<=350 ){ realvalue *= 1e-8; exp+=8; }
          while( realvalue>=10.0 && exp<=350 ){ realvalue *= 0.1; exp++; }
          while( realvalue<1e-8 && exp>=-350 ){ realvalue *= 1e8; exp-=8; }
          while( realvalue<1.0 && exp>=-350 ){ realvalue *= 10.0; exp--; }
          if( exp>350 || exp<-350 ){
            bufpt = "NaN";
            length = 3;
            break;
          }
        }
        bufpt = buf;
        



        flag_exp = xtype==etEXP;
        if( xtype!=etFLOAT ){
          realvalue += rounder;
          if( realvalue>=10.0 ){ realvalue *= 0.1; exp++; }
        }
        if( xtype==etGENERIC ){
          flag_rtz = !flag_alternateform;
          if( exp<-4 || exp>precision ){
            xtype = etEXP;
          }else{
            precision = precision - exp;
            xtype = etFLOAT;
          }
        }else{
          flag_rtz = 0;
        }
        if( xtype==etEXP ){
          e2 = 0;
        }else{
          e2 = exp;
        }
        nsd = 0;
        flag_dp = (precision>0) | flag_alternateform | flag_altform2;
        
        if( prefix ){
          *(bufpt++) = prefix;
        }
        
        if( e2<0 ){
          *(bufpt++) = '0';
        }else{
          for(; e2>=0; e2--){
            *(bufpt++) = et_getdigit(&realvalue,&nsd);
          }
        }
        
        if( flag_dp ){
          *(bufpt++) = '.';
        }
        

        for(e2++; e2<0 && precision>0; precision--, e2++){
          *(bufpt++) = '0';
        }
        
        while( (precision--)>0 ){
          *(bufpt++) = et_getdigit(&realvalue,&nsd);
        }
        
        if( flag_rtz && flag_dp ){
          while( bufpt[-1]=='0' ) *(--bufpt) = 0;
          assert( bufpt>buf );
          if( bufpt[-1]=='.' ){
            if( flag_altform2 ){
              *(bufpt++) = '0';
            }else{
              *(--bufpt) = 0;
            }
          }
        }
        
        if( flag_exp || (xtype==etEXP && exp) ){
          *(bufpt++) = aDigits[infop->charset];
          if( exp<0 ){
            *(bufpt++) = '-'; exp = -exp;
          }else{
            *(bufpt++) = '+';
          }
          if( exp>=100 ){
            *(bufpt++) = (exp/100)+'0';                
            exp %= 100;
          }
          *(bufpt++) = exp/10+'0';                     
          *(bufpt++) = exp%10+'0';                     
        }
        *bufpt = 0;

        


        length = bufpt-buf;
        bufpt = buf;

        

        if( flag_zeropad && !flag_leftjustify && length < width){
          int i;
          int nPad = width - length;
          for(i=width; i>=nPad; i--){
            bufpt[i] = bufpt[i-nPad];
          }
          i = prefix!=0;
          while( nPad-- ) bufpt[i++] = '0';
          length = width;
        }
#endif
        break;
      case etSIZE:
        *(va_arg(ap,int*)) = count;
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARLIT:
      case etCHARX:
        c = buf[0] = (xtype==etCHARX ? va_arg(ap,int) : *++fmt);
        if( precision>=0 ){
          for(idx=1; idx<precision; idx++) buf[idx] = c;
          length = precision;
        }else{
          length =1;
        }
        bufpt = buf;
        break;
      case etSTRING:
      case etDYNSTRING:
        bufpt = va_arg(ap,char*);
        if( bufpt==0 ){
          bufpt = "";
        }else if( xtype==etDYNSTRING ){
          zExtra = bufpt;
        }
        length = strlen(bufpt);
        if( precision>=0 && precision<length ) length = precision;
        break;
      case etSQLESCAPE:
      case etSQLESCAPE2: {
        int i, j, n, ch, isnull;
        int needQuote;
        char *escarg = va_arg(ap,char*);
        isnull = escarg==0;
        if( isnull ) escarg = (xtype==etSQLESCAPE2 ? "NULL" : "(NULL)");
        for(i=n=0; (ch=escarg[i])!=0; i++){
          if( ch=='\'' )  n++;
        }
        needQuote = !isnull && xtype==etSQLESCAPE2;
        n += i + 1 + needQuote*2;
        if( n>etBUFSIZE ){
          bufpt = zExtra = sqliteMalloc( n );
          if( bufpt==0 ) return -1;
        }else{
          bufpt = buf;
        }
        j = 0;
        if( needQuote ) bufpt[j++] = '\'';
        for(i=0; (ch=escarg[i])!=0; i++){
          bufpt[j++] = ch;
          if( ch=='\'' ) bufpt[j++] = ch;
        }
        if( needQuote ) bufpt[j++] = '\'';
        bufpt[j] = 0;
        length = j;
        
        
        break;
      }
      case etTOKEN: {
        Token *pToken = va_arg(ap, Token*);
        if( pToken && pToken->z ){
          (*func)(arg, (char*)pToken->z, pToken->n);
        }
        length = width = 0;
        break;
      }
      case etSRCLIST: {
        SrcList *pSrc = va_arg(ap, SrcList*);
        int k = va_arg(ap, int);
        struct SrcList_item *pItem = &pSrc->a[k];
        assert( k>=0 && k<pSrc->nSrc );
        if( pItem->zDatabase && pItem->zDatabase[0] ){
          (*func)(arg, pItem->zDatabase, strlen(pItem->zDatabase));
          (*func)(arg, ".", 1);
        }
        (*func)(arg, pItem->zName, strlen(pItem->zName));
        length = width = 0;
        break;
      }
    }
    




    if( !flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=etSPACESIZE ){
          (*func)(arg,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) (*func)(arg,spaces,nspace);
      }
    }
    if( length>0 ){
      (*func)(arg,bufpt,length);
      count += length;
    }
    if( flag_leftjustify ){
      register int nspace;
      nspace = width-length;
      if( nspace>0 ){
        count += nspace;
        while( nspace>=etSPACESIZE ){
          (*func)(arg,spaces,etSPACESIZE);
          nspace -= etSPACESIZE;
        }
        if( nspace>0 ) (*func)(arg,spaces,nspace);
      }
    }
    if( zExtra ){
      sqliteFree(zExtra);
    }
  }
  return errorflag ? -1 : count;
} 





struct sgMprintf {
  char *zBase;     
  char *zText;     
  int  nChar;      
  int  nTotal;     
  int  nAlloc;     
  void *(*xRealloc)(void*,int);  
};







static void mout(void *arg, const char *zNewText, int nNewChar){
  struct sgMprintf *pM = (struct sgMprintf*)arg;
  pM->nTotal += nNewChar;
  if( pM->nChar + nNewChar + 1 > pM->nAlloc ){
    if( pM->xRealloc==0 ){
      nNewChar =  pM->nAlloc - pM->nChar - 1;
    }else{
      pM->nAlloc = pM->nChar + nNewChar*2 + 1;
      if( pM->zText==pM->zBase ){
        pM->zText = pM->xRealloc(0, pM->nAlloc);
        if( pM->zText && pM->nChar ){
          memcpy(pM->zText, pM->zBase, pM->nChar);
        }
      }else{
        char *zNew;
        zNew = pM->xRealloc(pM->zText, pM->nAlloc);
        if( zNew ){
          pM->zText = zNew;
        }
      }
    }
  }
  if( pM->zText ){
    if( nNewChar>0 ){
      memcpy(&pM->zText[pM->nChar], zNewText, nNewChar);
      pM->nChar += nNewChar;
    }
    pM->zText[pM->nChar] = 0;
  }
}





static char *base_vprintf(
  void *(*xRealloc)(void*,int),   
  int useInternal,                
  char *zInitBuf,                 
  int nInitBuf,                   
  const char *zFormat,            
  va_list ap                      
){
  struct sgMprintf sM;
  sM.zBase = sM.zText = zInitBuf;
  sM.nChar = sM.nTotal = 0;
  sM.nAlloc = nInitBuf;
  sM.xRealloc = xRealloc;
  vxprintf(mout, &sM, useInternal, zFormat, ap);
  if( xRealloc ){
    if( sM.zText==sM.zBase ){
      sM.zText = xRealloc(0, sM.nChar+1);
      if( sM.zText ){
        memcpy(sM.zText, sM.zBase, sM.nChar+1);
      }
    }else if( sM.nAlloc>sM.nChar+10 ){
      char *zNew = xRealloc(sM.zText, sM.nChar+1);
      if( zNew ){
        sM.zText = zNew;
      }
    }
  }
  return sM.zText;
}




static void *printf_realloc(void *old, int size){
  return sqliteRealloc(old,size);
}





char *sqlite3VMPrintf(const char *zFormat, va_list ap){
  char zBase[SQLITE_PRINT_BUF_SIZE];
  return base_vprintf(printf_realloc, 1, zBase, sizeof(zBase), zFormat, ap);
}





char *sqlite3MPrintf(const char *zFormat, ...){
  va_list ap;
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  va_start(ap, zFormat);
  z = base_vprintf(printf_realloc, 1, zBase, sizeof(zBase), zFormat, ap);
  va_end(ap);
  return z;
}





char *sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
  char zBuf[200];

  va_start(ap,zFormat);
  z = base_vprintf((void*(*)(void*,int))realloc, 0, 
                   zBuf, sizeof(zBuf), zFormat, ap);
  va_end(ap);
  return z;
}



char *sqlite3_vmprintf(const char *zFormat, va_list ap){
  char zBuf[200];
  return base_vprintf((void*(*)(void*,int))realloc, 0,
                      zBuf, sizeof(zBuf), zFormat, ap);
}







char *sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  char *z;
  va_list ap;

  va_start(ap,zFormat);
  z = base_vprintf(0, 0, zBuf, n, zFormat, ap);
  va_end(ap);
  return z;
}

#if defined(SQLITE_TEST) || defined(SQLITE_DEBUG)





void sqlite3DebugPrintf(const char *zFormat, ...){
  extern int getpid(void);
  va_list ap;
  char zBuf[500];
  va_start(ap, zFormat);
  base_vprintf(0, 0, zBuf, sizeof(zBuf), zFormat, ap);
  va_end(ap);
  fprintf(stdout,"%d: %s", getpid(), zBuf);
  fflush(stdout);
}
#endif
