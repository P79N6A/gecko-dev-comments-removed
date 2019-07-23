














#include "sqliteInt.h"
#include "os.h"
#include <ctype.h>



#if !defined(SQLITE_OMIT_PRAGMA) && !defined(SQLITE_OMIT_PARSER)

#if defined(SQLITE_DEBUG) || defined(SQLITE_TEST)
# include "pager.h"
# include "btree.h"
#endif











static int getSafetyLevel(const char *z){
                             
  static const char zText[] = "onoffalseyestruefull";
  static const u8 iOffset[] = {0, 1, 2, 4, 9, 12, 16};
  static const u8 iLength[] = {2, 2, 3, 5, 3, 4, 4};
  static const u8 iValue[] =  {1, 0, 0, 0, 1, 1, 2};
  int i, n;
  if( isdigit(*z) ){
    return atoi(z);
  }
  n = strlen(z);
  for(i=0; i<sizeof(iLength); i++){
    if( iLength[i]==n && sqlite3StrNICmp(&zText[iOffset[i]],z,n)==0 ){
      return iValue[i];
    }
  }
  return 1;
}




static int getBoolean(const char *z){
  return getSafetyLevel(z)&1;
}

#ifndef SQLITE_OMIT_PAGER_PRAGMAS





static int getTempStore(const char *z){
  if( z[0]>='0' && z[0]<='2' ){
    return z[0] - '0';
  }else if( sqlite3StrICmp(z, "file")==0 ){
    return 1;
  }else if( sqlite3StrICmp(z, "memory")==0 ){
    return 2;
  }else{
    return 0;
  }
}
#endif 

#ifndef SQLITE_OMIT_PAGER_PRAGMAS




static int invalidateTempStorage(Parse *pParse){
  sqlite3 *db = pParse->db;
  if( db->aDb[1].pBt!=0 ){
    if( db->flags & SQLITE_InTrans ){
      sqlite3ErrorMsg(pParse, "temporary storage cannot be changed "
        "from within a transaction");
      return SQLITE_ERROR;
    }
    sqlite3BtreeClose(db->aDb[1].pBt);
    db->aDb[1].pBt = 0;
    sqlite3ResetInternalSchema(db, 0);
  }
  return SQLITE_OK;
}
#endif 

#ifndef SQLITE_OMIT_PAGER_PRAGMAS





static int changeTempStorage(Parse *pParse, const char *zStorageType){
  int ts = getTempStore(zStorageType);
  sqlite3 *db = pParse->db;
  if( db->temp_store==ts ) return SQLITE_OK;
  if( invalidateTempStorage( pParse ) != SQLITE_OK ){
    return SQLITE_ERROR;
  }
  db->temp_store = ts;
  return SQLITE_OK;
}
#endif 




static void returnSingleInt(Parse *pParse, const char *zLabel, int value){
  Vdbe *v = sqlite3GetVdbe(pParse);
  sqlite3VdbeAddOp(v, OP_Integer, value, 0);
  if( pParse->explain==0 ){
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, zLabel, P3_STATIC);
  }
  sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
}

#ifndef SQLITE_OMIT_FLAG_PRAGMAS





static int flagPragma(Parse *pParse, const char *zLeft, const char *zRight){
  static const struct sPragmaType {
    const char *zName;  
    int mask;           
  } aPragma[] = {
    { "vdbe_trace",               SQLITE_VdbeTrace     },
    { "sql_trace",                SQLITE_SqlTrace      },
    { "vdbe_listing",             SQLITE_VdbeListing   },
    { "full_column_names",        SQLITE_FullColNames  },
    { "short_column_names",       SQLITE_ShortColNames },
    { "count_changes",            SQLITE_CountRows     },
    { "empty_result_callbacks",   SQLITE_NullCallback  },
    { "legacy_file_format",       SQLITE_LegacyFileFmt },
    { "fullfsync",                SQLITE_FullFSync     },
#ifndef SQLITE_OMIT_CHECK
    { "ignore_check_constraints", SQLITE_IgnoreChecks  },
#endif
    
    { "writable_schema",          SQLITE_WriteSchema   },
    { "omit_readlock",            SQLITE_NoReadlock    },

    

    { "read_uncommitted",         SQLITE_ReadUncommitted },
  };
  int i;
  const struct sPragmaType *p;
  for(i=0, p=aPragma; i<sizeof(aPragma)/sizeof(aPragma[0]); i++, p++){
    if( sqlite3StrICmp(zLeft, p->zName)==0 ){
      sqlite3 *db = pParse->db;
      Vdbe *v;
      v = sqlite3GetVdbe(pParse);
      if( v ){
        if( zRight==0 ){
          returnSingleInt(pParse, p->zName, (db->flags & p->mask)!=0 );
        }else{
          if( getBoolean(zRight) ){
            db->flags |= p->mask;
          }else{
            db->flags &= ~p->mask;
          }
        }
      }
      return 1;
    }
  }
  return 0;
}
#endif 
















void sqlite3Pragma(
  Parse *pParse, 
  Token *pId1,        
  Token *pId2,        
  Token *pValue,      
  int minusFlag       
){
  char *zLeft = 0;       
  char *zRight = 0;      
  const char *zDb = 0;   
  Token *pId;            
  int iDb;               
  sqlite3 *db = pParse->db;
  Db *pDb;
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;

  

  iDb = sqlite3TwoPartName(pParse, pId1, pId2, &pId);
  if( iDb<0 ) return;
  pDb = &db->aDb[iDb];

  


  if( iDb==1 && sqlite3OpenTempDatabase(pParse) ){
    return;
  }

  zLeft = sqlite3NameFromToken(pId);
  if( !zLeft ) return;
  if( minusFlag ){
    zRight = sqlite3MPrintf("-%T", pValue);
  }else{
    zRight = sqlite3NameFromToken(pValue);
  }

  zDb = ((iDb>0)?pDb->zName:0);
  if( sqlite3AuthCheck(pParse, SQLITE_PRAGMA, zLeft, zRight, zDb) ){
    goto pragma_out;
  }
 
#ifndef SQLITE_OMIT_PAGER_PRAGMAS
  















  if( sqlite3StrICmp(zLeft,"default_cache_size")==0 ){
    static const VdbeOpList getCacheSize[] = {
      { OP_ReadCookie,  0, 2,        0},  
      { OP_AbsValue,    0, 0,        0},
      { OP_Dup,         0, 0,        0},
      { OP_Integer,     0, 0,        0},
      { OP_Ne,          0, 6,        0},
      { OP_Integer,     0, 0,        0},  
      { OP_Callback,    1, 0,        0},
    };
    int addr;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    if( !zRight ){
      sqlite3VdbeSetNumCols(v, 1);
      sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "cache_size", P3_STATIC);
      addr = sqlite3VdbeAddOpList(v, ArraySize(getCacheSize), getCacheSize);
      sqlite3VdbeChangeP1(v, addr, iDb);
      sqlite3VdbeChangeP1(v, addr+5, MAX_PAGES);
    }else{
      int size = atoi(zRight);
      if( size<0 ) size = -size;
      sqlite3BeginWriteOperation(pParse, 0, iDb);
      sqlite3VdbeAddOp(v, OP_Integer, size, 0);
      sqlite3VdbeAddOp(v, OP_ReadCookie, iDb, 2);
      addr = sqlite3VdbeAddOp(v, OP_Integer, 0, 0);
      sqlite3VdbeAddOp(v, OP_Ge, 0, addr+3);
      sqlite3VdbeAddOp(v, OP_Negative, 0, 0);
      sqlite3VdbeAddOp(v, OP_SetCookie, iDb, 2);
      pDb->pSchema->cache_size = size;
      sqlite3BtreeSetCacheSize(pDb->pBt, pDb->pSchema->cache_size);
    }
  }else

  








  if( sqlite3StrICmp(zLeft,"page_size")==0 ){
    Btree *pBt = pDb->pBt;
    if( !zRight ){
      int size = pBt ? sqlite3BtreeGetPageSize(pBt) : 0;
      returnSingleInt(pParse, "page_size", size);
    }else{
      sqlite3BtreeSetPageSize(pBt, atoi(zRight), -1);
    }
  }else
#endif 

  





#ifndef SQLITE_OMIT_AUTOVACUUM
  if( sqlite3StrICmp(zLeft,"auto_vacuum")==0 ){
    Btree *pBt = pDb->pBt;
    if( !zRight ){
      int auto_vacuum = 
          pBt ? sqlite3BtreeGetAutoVacuum(pBt) : SQLITE_DEFAULT_AUTOVACUUM;
      returnSingleInt(pParse, "auto_vacuum", auto_vacuum);
    }else{
      sqlite3BtreeSetAutoVacuum(pBt, getBoolean(zRight));
    }
  }else
#endif

#ifndef SQLITE_OMIT_PAGER_PRAGMAS
  













  if( sqlite3StrICmp(zLeft,"cache_size")==0 ){
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    if( !zRight ){
      returnSingleInt(pParse, "cache_size", pDb->pSchema->cache_size);
    }else{
      int size = atoi(zRight);
      if( size<0 ) size = -size;
      pDb->pSchema->cache_size = size;
      sqlite3BtreeSetCacheSize(pDb->pBt, pDb->pSchema->cache_size);
    }
  }else

  










  if( sqlite3StrICmp(zLeft, "temp_store")==0 ){
    if( !zRight ){
      returnSingleInt(pParse, "temp_store", db->temp_store);
    }else{
      changeTempStorage(pParse, zRight);
    }
  }else

  









  if( sqlite3StrICmp(zLeft, "temp_store_directory")==0 ){
    if( !zRight ){
      if( sqlite3_temp_directory ){
        sqlite3VdbeSetNumCols(v, 1);
        sqlite3VdbeSetColName(v, 0, COLNAME_NAME, 
            "temp_store_directory", P3_STATIC);
        sqlite3VdbeOp3(v, OP_String8, 0, 0, sqlite3_temp_directory, 0);
        sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
      }
    }else{
      if( zRight[0] && !sqlite3OsIsDirWritable(zRight) ){
        sqlite3ErrorMsg(pParse, "not a writable directory");
        goto pragma_out;
      }
      if( TEMP_STORE==0
       || (TEMP_STORE==1 && db->temp_store<=1)
       || (TEMP_STORE==2 && db->temp_store==1)
      ){
        invalidateTempStorage(pParse);
      }
      sqliteFree(sqlite3_temp_directory);
      if( zRight[0] ){
        sqlite3_temp_directory = zRight;
        zRight = 0;
      }else{
        sqlite3_temp_directory = 0;
      }
    }
  }else

  








  if( sqlite3StrICmp(zLeft,"synchronous")==0 ){
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    if( !zRight ){
      returnSingleInt(pParse, "synchronous", pDb->safety_level-1);
    }else{
      if( !db->autoCommit ){
        sqlite3ErrorMsg(pParse, 
            "Safety level may not be changed inside a transaction");
      }else{
        pDb->safety_level = getSafetyLevel(zRight)+1;
      }
    }
  }else
#endif 

#ifndef SQLITE_OMIT_FLAG_PRAGMAS
  if( flagPragma(pParse, zLeft, zRight) ){
    

  }else
#endif 

#ifndef SQLITE_OMIT_SCHEMA_PRAGMAS
  











  if( sqlite3StrICmp(zLeft, "table_info")==0 && zRight ){
    Table *pTab;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    pTab = sqlite3FindTable(db, zRight, zDb);
    if( pTab ){
      int i;
      Column *pCol;
      sqlite3VdbeSetNumCols(v, 6);
      sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "cid", P3_STATIC);
      sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "name", P3_STATIC);
      sqlite3VdbeSetColName(v, 2, COLNAME_NAME, "type", P3_STATIC);
      sqlite3VdbeSetColName(v, 3, COLNAME_NAME, "notnull", P3_STATIC);
      sqlite3VdbeSetColName(v, 4, COLNAME_NAME, "dflt_value", P3_STATIC);
      sqlite3VdbeSetColName(v, 5, COLNAME_NAME, "pk", P3_STATIC);
      sqlite3ViewGetColumnNames(pParse, pTab);
      for(i=0, pCol=pTab->aCol; i<pTab->nCol; i++, pCol++){
        sqlite3VdbeAddOp(v, OP_Integer, i, 0);
        sqlite3VdbeOp3(v, OP_String8, 0, 0, pCol->zName, 0);
        sqlite3VdbeOp3(v, OP_String8, 0, 0,
           pCol->zType ? pCol->zType : "", 0);
        sqlite3VdbeAddOp(v, OP_Integer, pCol->notNull, 0);
        sqlite3ExprCode(pParse, pCol->pDflt);
        sqlite3VdbeAddOp(v, OP_Integer, pCol->isPrimKey, 0);
        sqlite3VdbeAddOp(v, OP_Callback, 6, 0);
      }
    }
  }else

  if( sqlite3StrICmp(zLeft, "index_info")==0 && zRight ){
    Index *pIdx;
    Table *pTab;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    pIdx = sqlite3FindIndex(db, zRight, zDb);
    if( pIdx ){
      int i;
      pTab = pIdx->pTable;
      sqlite3VdbeSetNumCols(v, 3);
      sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "seqno", P3_STATIC);
      sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "cid", P3_STATIC);
      sqlite3VdbeSetColName(v, 2, COLNAME_NAME, "name", P3_STATIC);
      for(i=0; i<pIdx->nColumn; i++){
        int cnum = pIdx->aiColumn[i];
        sqlite3VdbeAddOp(v, OP_Integer, i, 0);
        sqlite3VdbeAddOp(v, OP_Integer, cnum, 0);
        assert( pTab->nCol>cnum );
        sqlite3VdbeOp3(v, OP_String8, 0, 0, pTab->aCol[cnum].zName, 0);
        sqlite3VdbeAddOp(v, OP_Callback, 3, 0);
      }
    }
  }else

  if( sqlite3StrICmp(zLeft, "index_list")==0 && zRight ){
    Index *pIdx;
    Table *pTab;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    pTab = sqlite3FindTable(db, zRight, zDb);
    if( pTab ){
      v = sqlite3GetVdbe(pParse);
      pIdx = pTab->pIndex;
      if( pIdx ){
        int i = 0; 
        sqlite3VdbeSetNumCols(v, 3);
        sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "seq", P3_STATIC);
        sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "name", P3_STATIC);
        sqlite3VdbeSetColName(v, 2, COLNAME_NAME, "unique", P3_STATIC);
        while(pIdx){
          sqlite3VdbeAddOp(v, OP_Integer, i, 0);
          sqlite3VdbeOp3(v, OP_String8, 0, 0, pIdx->zName, 0);
          sqlite3VdbeAddOp(v, OP_Integer, pIdx->onError!=OE_None, 0);
          sqlite3VdbeAddOp(v, OP_Callback, 3, 0);
          ++i;
          pIdx = pIdx->pNext;
        }
      }
    }
  }else

  if( sqlite3StrICmp(zLeft, "database_list")==0 ){
    int i;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    sqlite3VdbeSetNumCols(v, 3);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "seq", P3_STATIC);
    sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "name", P3_STATIC);
    sqlite3VdbeSetColName(v, 2, COLNAME_NAME, "file", P3_STATIC);
    for(i=0; i<db->nDb; i++){
      if( db->aDb[i].pBt==0 ) continue;
      assert( db->aDb[i].zName!=0 );
      sqlite3VdbeAddOp(v, OP_Integer, i, 0);
      sqlite3VdbeOp3(v, OP_String8, 0, 0, db->aDb[i].zName, 0);
      sqlite3VdbeOp3(v, OP_String8, 0, 0,
           sqlite3BtreeGetFilename(db->aDb[i].pBt), 0);
      sqlite3VdbeAddOp(v, OP_Callback, 3, 0);
    }
  }else

  if( sqlite3StrICmp(zLeft, "collation_list")==0 ){
    int i = 0;
    HashElem *p;
    sqlite3VdbeSetNumCols(v, 2);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "seq", P3_STATIC);
    sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "name", P3_STATIC);
    for(p=sqliteHashFirst(&db->aCollSeq); p; p=sqliteHashNext(p)){
      CollSeq *pColl = (CollSeq *)sqliteHashData(p);
      sqlite3VdbeAddOp(v, OP_Integer, i++, 0);
      sqlite3VdbeOp3(v, OP_String8, 0, 0, pColl->zName, 0);
      sqlite3VdbeAddOp(v, OP_Callback, 2, 0);
    }
  }else
#endif 

#ifndef SQLITE_OMIT_FOREIGN_KEY
  if( sqlite3StrICmp(zLeft, "foreign_key_list")==0 && zRight ){
    FKey *pFK;
    Table *pTab;
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    pTab = sqlite3FindTable(db, zRight, zDb);
    if( pTab ){
      v = sqlite3GetVdbe(pParse);
      pFK = pTab->pFKey;
      if( pFK ){
        int i = 0; 
        sqlite3VdbeSetNumCols(v, 5);
        sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "id", P3_STATIC);
        sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "seq", P3_STATIC);
        sqlite3VdbeSetColName(v, 2, COLNAME_NAME, "table", P3_STATIC);
        sqlite3VdbeSetColName(v, 3, COLNAME_NAME, "from", P3_STATIC);
        sqlite3VdbeSetColName(v, 4, COLNAME_NAME, "to", P3_STATIC);
        while(pFK){
          int j;
          for(j=0; j<pFK->nCol; j++){
            char *zCol = pFK->aCol[j].zCol;
            sqlite3VdbeAddOp(v, OP_Integer, i, 0);
            sqlite3VdbeAddOp(v, OP_Integer, j, 0);
            sqlite3VdbeOp3(v, OP_String8, 0, 0, pFK->zTo, 0);
            sqlite3VdbeOp3(v, OP_String8, 0, 0,
                             pTab->aCol[pFK->aCol[j].iFrom].zName, 0);
            sqlite3VdbeOp3(v, zCol ? OP_String8 : OP_Null, 0, 0, zCol, 0);
            sqlite3VdbeAddOp(v, OP_Callback, 5, 0);
          }
          ++i;
          pFK = pFK->pNextFrom;
        }
      }
    }
  }else
#endif 

#ifndef NDEBUG
  if( sqlite3StrICmp(zLeft, "parser_trace")==0 ){
    extern void sqlite3ParserTrace(FILE*, char *);
    if( zRight ){
      if( getBoolean(zRight) ){
        sqlite3ParserTrace(stderr, "parser: ");
      }else{
        sqlite3ParserTrace(0, 0);
      }
    }
  }else
#endif

  


  if( sqlite3StrICmp(zLeft, "case_sensitive_like")==0 ){
    if( zRight ){
      sqlite3RegisterLikeFunctions(db, getBoolean(zRight));
    }
  }else

#ifndef SQLITE_OMIT_INTEGRITY_CHECK
  if( sqlite3StrICmp(zLeft, "integrity_check")==0 ){
    int i, j, addr;

    



    static const VdbeOpList endCode[] = {
      { OP_MemLoad,     0, 0,        0},
      { OP_Integer,     0, 0,        0},
      { OP_Ne,          0, 0,        0},    
      { OP_String8,     0, 0,        "ok"},
      { OP_Callback,    1, 0,        0},
    };

    
    if( sqlite3ReadSchema(pParse) ) goto pragma_out;
    sqlite3VdbeSetNumCols(v, 1);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "integrity_check", P3_STATIC);
    sqlite3VdbeAddOp(v, OP_MemInt, 0, 0);  

    
    for(i=0; i<db->nDb; i++){
      HashElem *x;
      Hash *pTbls;
      int cnt = 0;

      if( OMIT_TEMPDB && i==1 ) continue;

      sqlite3CodeVerifySchema(pParse, i);

      

      pTbls = &db->aDb[i].pSchema->tblHash;
      for(x=sqliteHashFirst(pTbls); x; x=sqliteHashNext(x)){
        Table *pTab = sqliteHashData(x);
        Index *pIdx;
        sqlite3VdbeAddOp(v, OP_Integer, pTab->tnum, 0);
        cnt++;
        for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
          sqlite3VdbeAddOp(v, OP_Integer, pIdx->tnum, 0);
          cnt++;
        }
      }
      assert( cnt>0 );
      sqlite3VdbeAddOp(v, OP_IntegrityCk, cnt, i);
      sqlite3VdbeAddOp(v, OP_Dup, 0, 1);
      addr = sqlite3VdbeOp3(v, OP_String8, 0, 0, "ok", P3_STATIC);
      sqlite3VdbeAddOp(v, OP_Eq, 0, addr+7);
      sqlite3VdbeOp3(v, OP_String8, 0, 0,
         sqlite3MPrintf("*** in database %s ***\n", db->aDb[i].zName),
         P3_DYNAMIC);
      sqlite3VdbeAddOp(v, OP_Pull, 1, 0);
      sqlite3VdbeAddOp(v, OP_Concat, 0, 1);
      sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
      sqlite3VdbeAddOp(v, OP_MemIncr, 1, 0);

      

      sqlite3CodeVerifySchema(pParse, i);
      for(x=sqliteHashFirst(pTbls); x; x=sqliteHashNext(x)){
        Table *pTab = sqliteHashData(x);
        Index *pIdx;
        int loopTop;

        if( pTab->pIndex==0 ) continue;
        sqlite3OpenTableAndIndices(pParse, pTab, 1, OP_OpenRead);
        sqlite3VdbeAddOp(v, OP_MemInt, 0, 1);
        loopTop = sqlite3VdbeAddOp(v, OP_Rewind, 1, 0);
        sqlite3VdbeAddOp(v, OP_MemIncr, 1, 1);
        for(j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
          int jmp2;
          static const VdbeOpList idxErr[] = {
            { OP_MemIncr,     1,  0,  0},
            { OP_String8,     0,  0,  "rowid "},
            { OP_Rowid,       1,  0,  0},
            { OP_String8,     0,  0,  " missing from index "},
            { OP_String8,     0,  0,  0},    
            { OP_Concat,      2,  0,  0},
            { OP_Callback,    1,  0,  0},
          };
          sqlite3GenerateIndexKey(v, pIdx, 1);
          jmp2 = sqlite3VdbeAddOp(v, OP_Found, j+2, 0);
          addr = sqlite3VdbeAddOpList(v, ArraySize(idxErr), idxErr);
          sqlite3VdbeChangeP3(v, addr+4, pIdx->zName, P3_STATIC);
          sqlite3VdbeJumpHere(v, jmp2);
        }
        sqlite3VdbeAddOp(v, OP_Next, 1, loopTop+1);
        sqlite3VdbeJumpHere(v, loopTop);
        for(j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
          static const VdbeOpList cntIdx[] = {
             { OP_MemInt,       0,  2,  0},
             { OP_Rewind,       0,  0,  0},  
             { OP_MemIncr,      1,  2,  0},
             { OP_Next,         0,  0,  0},  
             { OP_MemLoad,      1,  0,  0},
             { OP_MemLoad,      2,  0,  0},
             { OP_Eq,           0,  0,  0},  
             { OP_MemIncr,      1,  0,  0},
             { OP_String8,      0,  0,  "wrong # of entries in index "},
             { OP_String8,      0,  0,  0},  
             { OP_Concat,       0,  0,  0},
             { OP_Callback,     1,  0,  0},
          };
          if( pIdx->tnum==0 ) continue;
          addr = sqlite3VdbeAddOpList(v, ArraySize(cntIdx), cntIdx);
          sqlite3VdbeChangeP1(v, addr+1, j+2);
          sqlite3VdbeChangeP2(v, addr+1, addr+4);
          sqlite3VdbeChangeP1(v, addr+3, j+2);
          sqlite3VdbeChangeP2(v, addr+3, addr+2);
          sqlite3VdbeJumpHere(v, addr+6);
          sqlite3VdbeChangeP3(v, addr+9, pIdx->zName, P3_STATIC);
        }
      } 
    }
    addr = sqlite3VdbeAddOpList(v, ArraySize(endCode), endCode);
    sqlite3VdbeJumpHere(v, addr+2);
  }else
#endif 

#ifndef SQLITE_OMIT_UTF16
  





















  if( sqlite3StrICmp(zLeft, "encoding")==0 ){
    static struct EncName {
      char *zName;
      u8 enc;
    } encnames[] = {
      { "UTF-8",    SQLITE_UTF8        },
      { "UTF8",     SQLITE_UTF8        },
      { "UTF-16le", SQLITE_UTF16LE     },
      { "UTF16le",  SQLITE_UTF16LE     },
      { "UTF-16be", SQLITE_UTF16BE     },
      { "UTF16be",  SQLITE_UTF16BE     },
      { "UTF-16",   0  },
      { "UTF16",    0  },
      { 0, 0 }
    };
    struct EncName *pEnc;
    encnames[6].enc = encnames[7].enc = SQLITE_UTF16NATIVE;
    if( !zRight ){    
      if( sqlite3ReadSchema(pParse) ) goto pragma_out;
      sqlite3VdbeSetNumCols(v, 1);
      sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "encoding", P3_STATIC);
      sqlite3VdbeAddOp(v, OP_String8, 0, 0);
      for(pEnc=&encnames[0]; pEnc->zName; pEnc++){
        if( pEnc->enc==ENC(pParse->db) ){
          sqlite3VdbeChangeP3(v, -1, pEnc->zName, P3_STATIC);
          break;
        }
      }
      sqlite3VdbeAddOp(v, OP_Callback, 1, 0);
    }else{                        
      




      if( 
        !(DbHasProperty(db, 0, DB_SchemaLoaded)) || 
        DbHasProperty(db, 0, DB_Empty) 
      ){
        for(pEnc=&encnames[0]; pEnc->zName; pEnc++){
          if( 0==sqlite3StrICmp(zRight, pEnc->zName) ){
            ENC(pParse->db) = pEnc->enc;
            break;
          }
        }
        if( !pEnc->zName ){
          sqlite3ErrorMsg(pParse, "unsupported encoding: %s", zRight);
        }
      }
    }
  }else
#endif 

#ifndef SQLITE_OMIT_SCHEMA_VERSION_PRAGMAS
  
























  if( sqlite3StrICmp(zLeft, "schema_version")==0 ||
      sqlite3StrICmp(zLeft, "user_version")==0 ){

    int iCookie;   
    if( zLeft[0]=='s' || zLeft[0]=='S' ){
      iCookie = 0;
    }else{
      iCookie = 5;
    }

    if( zRight ){
      
      static const VdbeOpList setCookie[] = {
        { OP_Transaction,    0,  1,  0},    
        { OP_Integer,        0,  0,  0},    
        { OP_SetCookie,      0,  0,  0},    
      };
      int addr = sqlite3VdbeAddOpList(v, ArraySize(setCookie), setCookie);
      sqlite3VdbeChangeP1(v, addr, iDb);
      sqlite3VdbeChangeP1(v, addr+1, atoi(zRight));
      sqlite3VdbeChangeP1(v, addr+2, iDb);
      sqlite3VdbeChangeP2(v, addr+2, iCookie);
    }else{
      
      static const VdbeOpList readCookie[] = {
        { OP_ReadCookie,      0,  0,  0},    
        { OP_Callback,        1,  0,  0}
      };
      int addr = sqlite3VdbeAddOpList(v, ArraySize(readCookie), readCookie);
      sqlite3VdbeChangeP1(v, addr, iDb);
      sqlite3VdbeChangeP2(v, addr, iCookie);
      sqlite3VdbeSetNumCols(v, 1);
    }
  }
#endif 

#if defined(SQLITE_DEBUG) || defined(SQLITE_TEST)
  


  if( sqlite3StrICmp(zLeft, "lock_status")==0 ){
    static const char *const azLockName[] = {
      "unlocked", "shared", "reserved", "pending", "exclusive"
    };
    int i;
    Vdbe *v = sqlite3GetVdbe(pParse);
    sqlite3VdbeSetNumCols(v, 2);
    sqlite3VdbeSetColName(v, 0, COLNAME_NAME, "database", P3_STATIC);
    sqlite3VdbeSetColName(v, 1, COLNAME_NAME, "status", P3_STATIC);
    for(i=0; i<db->nDb; i++){
      Btree *pBt;
      Pager *pPager;
      if( db->aDb[i].zName==0 ) continue;
      sqlite3VdbeOp3(v, OP_String8, 0, 0, db->aDb[i].zName, P3_STATIC);
      pBt = db->aDb[i].pBt;
      if( pBt==0 || (pPager = sqlite3BtreePager(pBt))==0 ){
        sqlite3VdbeOp3(v, OP_String8, 0, 0, "closed", P3_STATIC);
      }else{
        int j = sqlite3pager_lockstate(pPager);
        sqlite3VdbeOp3(v, OP_String8, 0, 0, 
            (j>=0 && j<=4) ? azLockName[j] : "unknown", P3_STATIC);
      }
      sqlite3VdbeAddOp(v, OP_Callback, 2, 0);
    }
  }else
#endif

#ifdef SQLITE_SSE
  



  if( sqlite3StrICmp(zLeft, "create_sqlite_statement_table")==0 ){
    extern int sqlite3CreateStatementsTable(Parse*);
    sqlite3CreateStatementsTable(pParse);
  }else
#endif

#if SQLITE_HAS_CODEC
  if( sqlite3StrICmp(zLeft, "key")==0 ){
    sqlite3_key(db, zRight, strlen(zRight));
  }else
#endif

  {}

  if( v ){
    



    sqlite3VdbeAddOp(v, OP_Expire, 1, 0);

    



#ifndef SQLITE_OMIT_PAGER_PRAGMAS
    if( db->autoCommit ){
      sqlite3BtreeSetSafetyLevel(pDb->pBt, pDb->safety_level,
                 (db->flags&SQLITE_FullFSync)!=0);
    }
#endif
  }
pragma_out:
  sqliteFree(zLeft);
  sqliteFree(zRight);
}

#endif 
