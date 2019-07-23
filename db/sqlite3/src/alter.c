















#include "sqliteInt.h"
#include <ctype.h>





#ifndef SQLITE_OMIT_ALTERTABLE















static void renameTableFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  unsigned char const *zSql = sqlite3_value_text(argv[0]);
  unsigned char const *zTableName = sqlite3_value_text(argv[1]);

  int token;
  Token tname;
  unsigned char const *zCsr = zSql;
  int len = 0;
  char *zRet;

  



  if( zSql ){
    do {
      
      tname.z = zCsr;
      tname.n = len;

      


      do {
        zCsr += len;
        len = sqlite3GetToken(zCsr, &token);
      } while( token==TK_SPACE );
      assert( len>0 );
    } while( token!=TK_LP );

    zRet = sqlite3MPrintf("%.*s%Q%s", tname.z - zSql, zSql, 
       zTableName, tname.z+tname.n);
    sqlite3_result_text(context, zRet, -1, sqlite3FreeX);
  }
}

#ifndef SQLITE_OMIT_TRIGGER







static void renameTriggerFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  unsigned char const *zSql = sqlite3_value_text(argv[0]);
  unsigned char const *zTableName = sqlite3_value_text(argv[1]);

  int token;
  Token tname;
  int dist = 3;
  unsigned char const *zCsr = zSql;
  int len = 0;
  char *zRet;

  




  if( zSql ){
    do {
      
      tname.z = zCsr;
      tname.n = len;

      


      do {
        zCsr += len;
        len = sqlite3GetToken(zCsr, &token);
      }while( token==TK_SPACE );
      assert( len>0 );

      








      dist++;
      if( token==TK_DOT || token==TK_ON ){
        dist = 0;
      }
    } while( dist!=2 || (token!=TK_WHEN && token!=TK_FOR && token!=TK_BEGIN) );

    


    zRet = sqlite3MPrintf("%.*s%Q%s", tname.z - zSql, zSql, 
       zTableName, tname.z+tname.n);
    sqlite3_result_text(context, zRet, -1, sqlite3FreeX);
  }
}
#endif   




void sqlite3AlterFunctions(sqlite3 *db){
  static const struct {
     char *zName;
     signed char nArg;
     void (*xFunc)(sqlite3_context*,int,sqlite3_value **);
  } aFuncs[] = {
    { "sqlite_rename_table",    2, renameTableFunc},
#ifndef SQLITE_OMIT_TRIGGER
    { "sqlite_rename_trigger",  2, renameTriggerFunc},
#endif
  };
  int i;

  for(i=0; i<sizeof(aFuncs)/sizeof(aFuncs[0]); i++){
    sqlite3CreateFunc(db, aFuncs[i].zName, aFuncs[i].nArg,
        SQLITE_UTF8, 0, aFuncs[i].xFunc, 0, 0);
  }
}







static char *whereTempTriggers(Parse *pParse, Table *pTab){
  Trigger *pTrig;
  char *zWhere = 0;
  char *tmp = 0;
  const Schema *pTempSchema = pParse->db->aDb[1].pSchema; 

  




  if( pTab->pSchema!=pTempSchema ){
    for( pTrig=pTab->pTrigger; pTrig; pTrig=pTrig->pNext ){
      if( pTrig->pSchema==pTempSchema ){
        if( !zWhere ){
          zWhere = sqlite3MPrintf("name=%Q", pTrig->name);
        }else{
          tmp = zWhere;
          zWhere = sqlite3MPrintf("%s OR name=%Q", zWhere, pTrig->name);
          sqliteFree(tmp);
        }
      }
    }
  }
  return zWhere;
}









static void reloadTableSchema(Parse *pParse, Table *pTab, const char *zName){
  Vdbe *v;
  char *zWhere;
  int iDb;                   
#ifndef SQLITE_OMIT_TRIGGER
  Trigger *pTrig;
#endif

  v = sqlite3GetVdbe(pParse);
  if( !v ) return;
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  assert( iDb>=0 );

#ifndef SQLITE_OMIT_TRIGGER
  
  for(pTrig=pTab->pTrigger; pTrig; pTrig=pTrig->pNext){
    int iTrigDb = sqlite3SchemaToIndex(pParse->db, pTrig->pSchema);
    assert( iTrigDb==iDb || iTrigDb==1 );
    sqlite3VdbeOp3(v, OP_DropTrigger, iTrigDb, 0, pTrig->name, 0);
  }
#endif

  
  sqlite3VdbeOp3(v, OP_DropTable, iDb, 0, pTab->zName, 0);

  
  zWhere = sqlite3MPrintf("tbl_name=%Q", zName);
  if( !zWhere ) return;
  sqlite3VdbeOp3(v, OP_ParseSchema, iDb, 0, zWhere, P3_DYNAMIC);

#ifndef SQLITE_OMIT_TRIGGER
  


  if( (zWhere=whereTempTriggers(pParse, pTab))!=0 ){
    sqlite3VdbeOp3(v, OP_ParseSchema, 1, 0, zWhere, P3_DYNAMIC);
  }
#endif
}





void sqlite3AlterRenameTable(
  Parse *pParse,            
  SrcList *pSrc,            
  Token *pName              
){
  int iDb;                  
  char *zDb;                
  Table *pTab;              
  char *zName = 0;           
  sqlite3 *db = pParse->db; 
  Vdbe *v;
#ifndef SQLITE_OMIT_TRIGGER
  char *zWhere = 0;         
#endif
  
  if( sqlite3MallocFailed() ) goto exit_rename_table;
  assert( pSrc->nSrc==1 );

  pTab = sqlite3LocateTable(pParse, pSrc->a[0].zName, pSrc->a[0].zDatabase);
  if( !pTab ) goto exit_rename_table;
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  zDb = db->aDb[iDb].zName;

  
  zName = sqlite3NameFromToken(pName);
  if( !zName ) goto exit_rename_table;

  


  if( sqlite3FindTable(db, zName, zDb) || sqlite3FindIndex(db, zName, zDb) ){
    sqlite3ErrorMsg(pParse, 
        "there is already another table or index with this name: %s", zName);
    goto exit_rename_table;
  }

  


  if( strlen(pTab->zName)>6 && 0==sqlite3StrNICmp(pTab->zName, "sqlite_", 7) ){
    sqlite3ErrorMsg(pParse, "table %s may not be altered", pTab->zName);
    goto exit_rename_table;
  }
  if( SQLITE_OK!=sqlite3CheckObjectName(pParse, zName) ){
    goto exit_rename_table;
  }

#ifndef SQLITE_OMIT_AUTHORIZATION
  
  if( sqlite3AuthCheck(pParse, SQLITE_ALTER_TABLE, zDb, pTab->zName, 0) ){
    goto exit_rename_table;
  }
#endif

  



  v = sqlite3GetVdbe(pParse);
  if( v==0 ){
    goto exit_rename_table;
  }
  sqlite3BeginWriteOperation(pParse, 0, iDb);
  sqlite3ChangeCookie(db, v, iDb);

  
  sqlite3NestedParse(pParse,
      "UPDATE %Q.%s SET "
#ifdef SQLITE_OMIT_TRIGGER
          "sql = sqlite_rename_table(sql, %Q), "
#else
          "sql = CASE "
            "WHEN type = 'trigger' THEN sqlite_rename_trigger(sql, %Q)"
            "ELSE sqlite_rename_table(sql, %Q) END, "
#endif
          "tbl_name = %Q, "
          "name = CASE "
            "WHEN type='table' THEN %Q "
            "WHEN name LIKE 'sqlite_autoindex%%' AND type='index' THEN "
              "'sqlite_autoindex_' || %Q || substr(name, %d+18,10) "
            "ELSE name END "
      "WHERE tbl_name=%Q AND "
          "(type='table' OR type='index' OR type='trigger');", 
      zDb, SCHEMA_TABLE(iDb), zName, zName, zName, 
#ifndef SQLITE_OMIT_TRIGGER
      zName,
#endif
      zName, strlen(pTab->zName), pTab->zName
  );

#ifndef SQLITE_OMIT_AUTOINCREMENT
  


  if( sqlite3FindTable(db, "sqlite_sequence", zDb) ){
    sqlite3NestedParse(pParse,
        "UPDATE %Q.sqlite_sequence set name = %Q WHERE name = %Q",
        zDb, zName, pTab->zName);
  }
#endif

#ifndef SQLITE_OMIT_TRIGGER
  



  if( (zWhere=whereTempTriggers(pParse, pTab))!=0 ){
    sqlite3NestedParse(pParse, 
        "UPDATE sqlite_temp_master SET "
            "sql = sqlite_rename_trigger(sql, %Q), "
            "tbl_name = %Q "
            "WHERE %s;", zName, zName, zWhere);
    sqliteFree(zWhere);
  }
#endif

  
  reloadTableSchema(pParse, pTab, zName);

exit_rename_table:
  sqlite3SrcListDelete(pSrc);
  sqliteFree(zName);
}










void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef){
  Table *pNew;              
  Table *pTab;              
  int iDb;                  
  const char *zDb;          
  const char *zTab;         
  char *zCol;               
  Column *pCol;             
  Expr *pDflt;              

  if( pParse->nErr ) return;
  pNew = pParse->pNewTable;
  assert( pNew );

  iDb = sqlite3SchemaToIndex(pParse->db, pNew->pSchema);
  zDb = pParse->db->aDb[iDb].zName;
  zTab = pNew->zName;
  pCol = &pNew->aCol[pNew->nCol-1];
  pDflt = pCol->pDflt;
  pTab = sqlite3FindTable(pParse->db, zTab, zDb);
  assert( pTab );

#ifndef SQLITE_OMIT_AUTHORIZATION
  
  if( sqlite3AuthCheck(pParse, SQLITE_ALTER_TABLE, zDb, pTab->zName, 0) ){
    return;
  }
#endif

  



  if( pDflt && pDflt->op==TK_NULL ){
    pDflt = 0;
  }

  



  if( pCol->isPrimKey ){
    sqlite3ErrorMsg(pParse, "Cannot add a PRIMARY KEY column");
    return;
  }
  if( pNew->pIndex ){
    sqlite3ErrorMsg(pParse, "Cannot add a UNIQUE column");
    return;
  }
  if( pCol->notNull && !pDflt ){
    sqlite3ErrorMsg(pParse, 
        "Cannot add a NOT NULL column with default value NULL");
    return;
  }

  


  if( pDflt ){
    sqlite3_value *pVal;
    if( sqlite3ValueFromExpr(pDflt, SQLITE_UTF8, SQLITE_AFF_NONE, &pVal) ){
      
      return;
    }
    if( !pVal ){
      sqlite3ErrorMsg(pParse, "Cannot add a column with non-constant default");
      return;
    }
    sqlite3ValueFree(pVal);
  }

  
  zCol = sqliteStrNDup((char*)pColDef->z, pColDef->n);
  if( zCol ){
    char *zEnd = &zCol[pColDef->n-1];
    while( (zEnd>zCol && *zEnd==';') || isspace(*(unsigned char *)zEnd) ){
      *zEnd-- = '\0';
    }
    sqlite3NestedParse(pParse, 
        "UPDATE %Q.%s SET "
          "sql = substr(sql,1,%d) || ', ' || %Q || substr(sql,%d,length(sql)) "
        "WHERE type = 'table' AND name = %Q", 
      zDb, SCHEMA_TABLE(iDb), pNew->addColOffset, zCol, pNew->addColOffset+1,
      zTab
    );
    sqliteFree(zCol);
  }

  



  sqlite3MinimumFileFormat(pParse, iDb, pDflt ? 3 : 2);

  
  reloadTableSchema(pParse, pTab, pTab->zName);
}
















void sqlite3AlterBeginAddColumn(Parse *pParse, SrcList *pSrc){
  Table *pNew;
  Table *pTab;
  Vdbe *v;
  int iDb;
  int i;
  int nAlloc;

  
  assert( pParse->pNewTable==0 );
  if( sqlite3MallocFailed() ) goto exit_begin_add_column;
  pTab = sqlite3LocateTable(pParse, pSrc->a[0].zName, pSrc->a[0].zDatabase);
  if( !pTab ) goto exit_begin_add_column;

  
  if( pTab->pSelect ){
    sqlite3ErrorMsg(pParse, "Cannot add a column to a view");
    goto exit_begin_add_column;
  }

  assert( pTab->addColOffset>0 );
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);

  


  pNew = (Table *)sqliteMalloc(sizeof(Table));
  if( !pNew ) goto exit_begin_add_column;
  pParse->pNewTable = pNew;
  pNew->nRef = 1;
  pNew->nCol = pTab->nCol;
  assert( pNew->nCol>0 );
  nAlloc = (((pNew->nCol-1)/8)*8)+8;
  assert( nAlloc>=pNew->nCol && nAlloc%8==0 && nAlloc-pNew->nCol<8 );
  pNew->aCol = (Column *)sqliteMalloc(sizeof(Column)*nAlloc);
  pNew->zName = sqliteStrDup(pTab->zName);
  if( !pNew->aCol || !pNew->zName ){
    goto exit_begin_add_column;
  }
  memcpy(pNew->aCol, pTab->aCol, sizeof(Column)*pNew->nCol);
  for(i=0; i<pNew->nCol; i++){
    Column *pCol = &pNew->aCol[i];
    pCol->zName = sqliteStrDup(pCol->zName);
    pCol->zColl = 0;
    pCol->zType = 0;
    pCol->pDflt = 0;
  }
  pNew->pSchema = pParse->db->aDb[iDb].pSchema;
  pNew->addColOffset = pTab->addColOffset;
  pNew->nRef = 1;

  
  sqlite3BeginWriteOperation(pParse, 0, iDb);
  v = sqlite3GetVdbe(pParse);
  if( !v ) goto exit_begin_add_column;
  sqlite3ChangeCookie(pParse->db, v, iDb);

exit_begin_add_column:
  sqlite3SrcListDelete(pSrc);
  return;
}
#endif  
