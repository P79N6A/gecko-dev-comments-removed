


















#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>
#include <stdlib.h>








#ifdef SQLITE_ASCII
# define charMap(X) sqlite3UpperToLower[(unsigned char)X]
#endif
#ifdef SQLITE_EBCDIC
# define charMap(X) ebcdicToAscii[(unsigned char)X]
const unsigned char ebcdicToAscii[] = {

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 95,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
};
#endif












#include "keywordhash.h"


















#ifdef SQLITE_ASCII
const char sqlite3IsIdChar[] = {

    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  
};
#define IdChar(C)  (((c=C)&0x80)!=0 || (c>0x1f && sqlite3IsIdChar[c-0x20]))
#endif
#ifdef SQLITE_EBCDIC
const char sqlite3IsIdChar[] = {

    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,  
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0,  
    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0,  
};
#define IdChar(C)  (((c=C)>=0x42 && sqlite3IsIdChar[c-0x40]))
#endif






static int getToken(const unsigned char *z, int *tokenType){
  int i, c;
  switch( *z ){
    case ' ': case '\t': case '\n': case '\f': case '\r': {
      for(i=1; isspace(z[i]); i++){}
      *tokenType = TK_SPACE;
      return i;
    }
    case '-': {
      if( z[1]=='-' ){
        for(i=2; (c=z[i])!=0 && c!='\n'; i++){}
        *tokenType = TK_COMMENT;
        return i;
      }
      *tokenType = TK_MINUS;
      return 1;
    }
    case '(': {
      *tokenType = TK_LP;
      return 1;
    }
    case ')': {
      *tokenType = TK_RP;
      return 1;
    }
    case ';': {
      *tokenType = TK_SEMI;
      return 1;
    }
    case '+': {
      *tokenType = TK_PLUS;
      return 1;
    }
    case '*': {
      *tokenType = TK_STAR;
      return 1;
    }
    case '/': {
      if( z[1]!='*' || z[2]==0 ){
        *tokenType = TK_SLASH;
        return 1;
      }
      for(i=3, c=z[2]; (c!='*' || z[i]!='/') && (c=z[i])!=0; i++){}
      if( c ) i++;
      *tokenType = TK_COMMENT;
      return i;
    }
    case '%': {
      *tokenType = TK_REM;
      return 1;
    }
    case '=': {
      *tokenType = TK_EQ;
      return 1 + (z[1]=='=');
    }
    case '<': {
      if( (c=z[1])=='=' ){
        *tokenType = TK_LE;
        return 2;
      }else if( c=='>' ){
        *tokenType = TK_NE;
        return 2;
      }else if( c=='<' ){
        *tokenType = TK_LSHIFT;
        return 2;
      }else{
        *tokenType = TK_LT;
        return 1;
      }
    }
    case '>': {
      if( (c=z[1])=='=' ){
        *tokenType = TK_GE;
        return 2;
      }else if( c=='>' ){
        *tokenType = TK_RSHIFT;
        return 2;
      }else{
        *tokenType = TK_GT;
        return 1;
      }
    }
    case '!': {
      if( z[1]!='=' ){
        *tokenType = TK_ILLEGAL;
        return 2;
      }else{
        *tokenType = TK_NE;
        return 2;
      }
    }
    case '|': {
      if( z[1]!='|' ){
        *tokenType = TK_BITOR;
        return 1;
      }else{
        *tokenType = TK_CONCAT;
        return 2;
      }
    }
    case ',': {
      *tokenType = TK_COMMA;
      return 1;
    }
    case '&': {
      *tokenType = TK_BITAND;
      return 1;
    }
    case '~': {
      *tokenType = TK_BITNOT;
      return 1;
    }
    case '`':
    case '\'':
    case '"': {
      int delim = z[0];
      for(i=1; (c=z[i])!=0; i++){
        if( c==delim ){
          if( z[i+1]==delim ){
            i++;
          }else{
            break;
          }
        }
      }
      if( c ){
        *tokenType = TK_STRING;
        return i+1;
      }else{
        *tokenType = TK_ILLEGAL;
        return i;
      }
    }
    case '.': {
#ifndef SQLITE_OMIT_FLOATING_POINT
      if( !isdigit(z[1]) )
#endif
      {
        *tokenType = TK_DOT;
        return 1;
      }
      

    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      *tokenType = TK_INTEGER;
      for(i=0; isdigit(z[i]); i++){}
#ifndef SQLITE_OMIT_FLOATING_POINT
      if( z[i]=='.' ){
        i++;
        while( isdigit(z[i]) ){ i++; }
        *tokenType = TK_FLOAT;
      }
      if( (z[i]=='e' || z[i]=='E') &&
           ( isdigit(z[i+1]) 
            || ((z[i+1]=='+' || z[i+1]=='-') && isdigit(z[i+2]))
           )
      ){
        i += 2;
        while( isdigit(z[i]) ){ i++; }
        *tokenType = TK_FLOAT;
      }
#endif
      return i;
    }
    case '[': {
      for(i=1, c=z[0]; c!=']' && (c=z[i])!=0; i++){}
      *tokenType = TK_ID;
      return i;
    }
    case '?': {
      *tokenType = TK_VARIABLE;
      for(i=1; isdigit(z[i]); i++){}
      return i;
    }
    case '#': {
      for(i=1; isdigit(z[i]); i++){}
      if( i>1 ){
        

        *tokenType = TK_REGISTER;
        return i;
      }
      

    }
#ifndef SQLITE_OMIT_TCL_VARIABLE
    case '$':
#endif
    case '@':  
    case ':': {
      int n = 0;
      *tokenType = TK_VARIABLE;
      for(i=1; (c=z[i])!=0; i++){
        if( IdChar(c) ){
          n++;
#ifndef SQLITE_OMIT_TCL_VARIABLE
        }else if( c=='(' && n>0 ){
          do{
            i++;
          }while( (c=z[i])!=0 && !isspace(c) && c!=')' );
          if( c==')' ){
            i++;
          }else{
            *tokenType = TK_ILLEGAL;
          }
          break;
        }else if( c==':' && z[i+1]==':' ){
          i++;
#endif
        }else{
          break;
        }
      }
      if( n==0 ) *tokenType = TK_ILLEGAL;
      return i;
    }
#ifndef SQLITE_OMIT_BLOB_LITERAL
    case 'x': case 'X': {
      if( (c=z[1])=='\'' || c=='"' ){
        int delim = c;
        *tokenType = TK_BLOB;
        for(i=2; (c=z[i])!=0; i++){
          if( c==delim ){
            if( i%2 ) *tokenType = TK_ILLEGAL;
            break;
          }
          if( !isxdigit(c) ){
            *tokenType = TK_ILLEGAL;
            return i;
          }
        }
        if( c ) i++;
        return i;
      }
      
    }
#endif
    default: {
      if( !IdChar(*z) ){
        break;
      }
      for(i=1; IdChar(z[i]); i++){}
      *tokenType = keywordCode((char*)z, i);
      return i;
    }
  }
  *tokenType = TK_ILLEGAL;
  return 1;
}
int sqlite3GetToken(const unsigned char *z, int *tokenType){
  return getToken(z, tokenType);
}








int sqlite3RunParser(Parse *pParse, const char *zSql, char **pzErrMsg){
  int nErr = 0;
  int i;
  void *pEngine;
  int tokenType;
  int lastTokenParsed = -1;
  sqlite3 *db = pParse->db;
  extern void *sqlite3ParserAlloc(void*(*)(int));
  extern void sqlite3ParserFree(void*, void(*)(void*));
  extern int sqlite3Parser(void*, int, Token, Parse*);

  db->flags &= ~SQLITE_Interrupt;
  pParse->rc = SQLITE_OK;
  i = 0;
  pEngine = sqlite3ParserAlloc((void*(*)(int))sqlite3MallocX);
  if( pEngine==0 ){
    return SQLITE_NOMEM;
  }
  assert( pParse->sLastToken.dyn==0 );
  assert( pParse->pNewTable==0 );
  assert( pParse->pNewTrigger==0 );
  assert( pParse->nVar==0 );
  assert( pParse->nVarExpr==0 );
  assert( pParse->nVarExprAlloc==0 );
  assert( pParse->apVarExpr==0 );
  pParse->zTail = pParse->zSql = zSql;
  while( !sqlite3MallocFailed() && zSql[i]!=0 ){
    assert( i>=0 );
    pParse->sLastToken.z = (u8*)&zSql[i];
    assert( pParse->sLastToken.dyn==0 );
    pParse->sLastToken.n = getToken((unsigned char*)&zSql[i],&tokenType);
    i += pParse->sLastToken.n;
    switch( tokenType ){
      case TK_SPACE:
      case TK_COMMENT: {
        if( (db->flags & SQLITE_Interrupt)!=0 ){
          pParse->rc = SQLITE_INTERRUPT;
          sqlite3SetString(pzErrMsg, "interrupt", (char*)0);
          goto abort_parse;
        }
        break;
      }
      case TK_ILLEGAL: {
        if( pzErrMsg ){
          sqliteFree(*pzErrMsg);
          *pzErrMsg = sqlite3MPrintf("unrecognized token: \"%T\"",
                          &pParse->sLastToken);
        }
        nErr++;
        goto abort_parse;
      }
      case TK_SEMI: {
        pParse->zTail = &zSql[i];
        
      }
      default: {
        sqlite3Parser(pEngine, tokenType, pParse->sLastToken, pParse);
        lastTokenParsed = tokenType;
        if( pParse->rc!=SQLITE_OK ){
          goto abort_parse;
        }
        break;
      }
    }
  }
abort_parse:
  if( zSql[i]==0 && nErr==0 && pParse->rc==SQLITE_OK ){
    if( lastTokenParsed!=TK_SEMI ){
      sqlite3Parser(pEngine, TK_SEMI, pParse->sLastToken, pParse);
      pParse->zTail = &zSql[i];
    }
    sqlite3Parser(pEngine, 0, pParse->sLastToken, pParse);
  }
  sqlite3ParserFree(pEngine, sqlite3FreeX);
  if( sqlite3MallocFailed() ){
    pParse->rc = SQLITE_NOMEM;
  }
  if( pParse->rc!=SQLITE_OK && pParse->rc!=SQLITE_DONE && pParse->zErrMsg==0 ){
    sqlite3SetString(&pParse->zErrMsg, sqlite3ErrStr(pParse->rc), (char*)0);
  }
  if( pParse->zErrMsg ){
    if( pzErrMsg && *pzErrMsg==0 ){
      *pzErrMsg = pParse->zErrMsg;
    }else{
      sqliteFree(pParse->zErrMsg);
    }
    pParse->zErrMsg = 0;
    if( !nErr ) nErr++;
  }
  if( pParse->pVdbe && pParse->nErr>0 && pParse->nested==0 ){
    sqlite3VdbeDelete(pParse->pVdbe);
    pParse->pVdbe = 0;
  }
#ifndef SQLITE_OMIT_SHARED_CACHE
  if( pParse->nested==0 ){
    sqliteFree(pParse->aTableLock);
    pParse->aTableLock = 0;
    pParse->nTableLock = 0;
  }
#endif
  sqlite3DeleteTable(pParse->db, pParse->pNewTable);
  sqlite3DeleteTrigger(pParse->pNewTrigger);
  sqliteFree(pParse->apVarExpr);
  if( nErr>0 && (pParse->rc==SQLITE_OK || pParse->rc==SQLITE_DONE) ){
    pParse->rc = SQLITE_ERROR;
  }
  return nErr;
}
