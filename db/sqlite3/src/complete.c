



















#include "sqliteInt.h"
#ifndef SQLITE_OMIT_COMPLETE




extern const char sqlite3IsIdChar[];
#define IdChar(C)  (((c=C)&0x80)!=0 || (c>0x1f && sqlite3IsIdChar[c-0x20]))






#define tkSEMI    0
#define tkWS      1
#define tkOTHER   2
#define tkEXPLAIN 3
#define tkCREATE  4
#define tkTEMP    5
#define tkTRIGGER 6
#define tkEND     7



















































int sqlite3_complete(const char *zSql){
  u8 state = 0;   
  u8 token;       

#ifndef SQLITE_OMIT_TRIGGER
  


  static const u8 trans[7][8] = {
                     
     
      {    0,  0,     1,      2,      3,    1,       1,   1,  },
      {    0,  1,     1,      1,      1,    1,       1,   1,  },
      {    0,  2,     1,      1,      3,    1,       1,   1,  },
      {    0,  3,     1,      1,      1,    3,       4,   1,  },
      {    5,  4,     4,      4,      4,    4,       4,   4,  },
      {    5,  5,     4,      4,      4,    4,       4,   6,  },
      {    0,  6,     4,      4,      4,    4,       4,   4,  },
  };
#else
  


  static const u8 trans[2][3] = {
                     
     
      {    0,  0,     1, },
      {    0,  1,     1, },
  };
#endif 

  while( *zSql ){
    switch( *zSql ){
      case ';': {  
        token = tkSEMI;
        break;
      }
      case ' ':
      case '\r':
      case '\t':
      case '\n':
      case '\f': {  
        token = tkWS;
        break;
      }
      case '/': {   
        if( zSql[1]!='*' ){
          token = tkOTHER;
          break;
        }
        zSql += 2;
        while( zSql[0] && (zSql[0]!='*' || zSql[1]!='/') ){ zSql++; }
        if( zSql[0]==0 ) return 0;
        zSql++;
        token = tkWS;
        break;
      }
      case '-': {   
        if( zSql[1]!='-' ){
          token = tkOTHER;
          break;
        }
        while( *zSql && *zSql!='\n' ){ zSql++; }
        if( *zSql==0 ) return state==0;
        token = tkWS;
        break;
      }
      case '[': {   
        zSql++;
        while( *zSql && *zSql!=']' ){ zSql++; }
        if( *zSql==0 ) return 0;
        token = tkOTHER;
        break;
      }
      case '`':     
      case '"':     
      case '\'': {
        int c = *zSql;
        zSql++;
        while( *zSql && *zSql!=c ){ zSql++; }
        if( *zSql==0 ) return 0;
        token = tkOTHER;
        break;
      }
      default: {
        int c;
        if( IdChar((u8)*zSql) ){
          
          int nId;
          for(nId=1; IdChar(zSql[nId]); nId++){}
#ifdef SQLITE_OMIT_TRIGGER
          token = tkOTHER;
#else
          switch( *zSql ){
            case 'c': case 'C': {
              if( nId==6 && sqlite3StrNICmp(zSql, "create", 6)==0 ){
                token = tkCREATE;
              }else{
                token = tkOTHER;
              }
              break;
            }
            case 't': case 'T': {
              if( nId==7 && sqlite3StrNICmp(zSql, "trigger", 7)==0 ){
                token = tkTRIGGER;
              }else if( nId==4 && sqlite3StrNICmp(zSql, "temp", 4)==0 ){
                token = tkTEMP;
              }else if( nId==9 && sqlite3StrNICmp(zSql, "temporary", 9)==0 ){
                token = tkTEMP;
              }else{
                token = tkOTHER;
              }
              break;
            }
            case 'e':  case 'E': {
              if( nId==3 && sqlite3StrNICmp(zSql, "end", 3)==0 ){
                token = tkEND;
              }else
#ifndef SQLITE_OMIT_EXPLAIN
              if( nId==7 && sqlite3StrNICmp(zSql, "explain", 7)==0 ){
                token = tkEXPLAIN;
              }else
#endif
              {
                token = tkOTHER;
              }
              break;
            }
            default: {
              token = tkOTHER;
              break;
            }
          }
#endif 
          zSql += nId-1;
        }else{
          
          token = tkOTHER;
        }
        break;
      }
    }
    state = trans[state][token];
    zSql++;
  }
  return state==0;
}

#ifndef SQLITE_OMIT_UTF16





int sqlite3_complete16(const void *zSql){
  sqlite3_value *pVal;
  char const *zSql8;
  int rc = 0;

  pVal = sqlite3ValueNew();
  sqlite3ValueSetStr(pVal, -1, zSql, SQLITE_UTF16NATIVE, SQLITE_STATIC);
  zSql8 = sqlite3ValueText(pVal, SQLITE_UTF8);
  if( zSql8 ){
    rc = sqlite3_complete(zSql8);
  }
  sqlite3ValueFree(pVal);
  return sqlite3ApiExit(0, rc);
}
#endif 
#endif 
