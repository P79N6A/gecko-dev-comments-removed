











#include "sqliteInt.h"

#ifndef SQLITE_OMIT_TRIGGER



void sqlite3DeleteTriggerStep(TriggerStep *pTriggerStep){
  while( pTriggerStep ){
    TriggerStep * pTmp = pTriggerStep;
    pTriggerStep = pTriggerStep->pNext;

    if( pTmp->target.dyn ) sqliteFree((char*)pTmp->target.z);
    sqlite3ExprDelete(pTmp->pWhere);
    sqlite3ExprListDelete(pTmp->pExprList);
    sqlite3SelectDelete(pTmp->pSelect);
    sqlite3IdListDelete(pTmp->pIdList);

    sqliteFree(pTmp);
  }
}









void sqlite3BeginTrigger(
  Parse *pParse,      
  Token *pName1,      
  Token *pName2,      
  int tr_tm,          
  int op,             
  IdList *pColumns,   
  SrcList *pTableName,
  int foreach,        
  Expr *pWhen,        
  int isTemp          
){
  Trigger *pTrigger = 0;
  Table *pTab;
  char *zName = 0;        
  sqlite3 *db = pParse->db;
  int iDb;                
  Token *pName;           
  DbFixer sFix;
  int iTabDb;

  assert( pName1!=0 );   
  assert( pName2!=0 );
  if( isTemp ){
    
    if( pName2->n>0 ){
      sqlite3ErrorMsg(pParse, "temporary trigger may not have qualified name");
      goto trigger_cleanup;
    }
    iDb = 1;
    pName = pName1;
  }else{
    
    iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pName);
    if( iDb<0 ){
      goto trigger_cleanup;
    }
  }

  




  if( !pTableName || sqlite3MallocFailed() ){
    goto trigger_cleanup;
  }
  pTab = sqlite3SrcListLookup(pParse, pTableName);
  if( pName2->n==0 && pTab && pTab->pSchema==db->aDb[1].pSchema ){
    iDb = 1;
  }

  
  if( sqlite3MallocFailed() ) goto trigger_cleanup;
  assert( pTableName->nSrc==1 );
  if( sqlite3FixInit(&sFix, pParse, iDb, "trigger", pName) && 
      sqlite3FixSrcList(&sFix, pTableName) ){
    goto trigger_cleanup;
  }
  pTab = sqlite3SrcListLookup(pParse, pTableName);
  if( !pTab ){
    
    goto trigger_cleanup;
  }

  

  zName = sqlite3NameFromToken(pName);
  if( !zName || SQLITE_OK!=sqlite3CheckObjectName(pParse, zName) ){
    goto trigger_cleanup;
  }
  if( sqlite3HashFind(&(db->aDb[iDb].pSchema->trigHash), zName,strlen(zName)) ){
    sqlite3ErrorMsg(pParse, "trigger %T already exists", pName);
    goto trigger_cleanup;
  }

  
  if( sqlite3StrNICmp(pTab->zName, "sqlite_", 7)==0 ){
    sqlite3ErrorMsg(pParse, "cannot create trigger on system table");
    pParse->nErr++;
    goto trigger_cleanup;
  }

  


  if( pTab->pSelect && tr_tm!=TK_INSTEAD ){
    sqlite3ErrorMsg(pParse, "cannot create %s trigger on view: %S", 
        (tr_tm == TK_BEFORE)?"BEFORE":"AFTER", pTableName, 0);
    goto trigger_cleanup;
  }
  if( !pTab->pSelect && tr_tm==TK_INSTEAD ){
    sqlite3ErrorMsg(pParse, "cannot create INSTEAD OF"
        " trigger on table: %S", pTableName, 0);
    goto trigger_cleanup;
  }
  iTabDb = sqlite3SchemaToIndex(db, pTab->pSchema);

#ifndef SQLITE_OMIT_AUTHORIZATION
  {
    int code = SQLITE_CREATE_TRIGGER;
    const char *zDb = db->aDb[iTabDb].zName;
    const char *zDbTrig = isTemp ? db->aDb[1].zName : zDb;
    if( iTabDb==1 || isTemp ) code = SQLITE_CREATE_TEMP_TRIGGER;
    if( sqlite3AuthCheck(pParse, code, zName, pTab->zName, zDbTrig) ){
      goto trigger_cleanup;
    }
    if( sqlite3AuthCheck(pParse, SQLITE_INSERT, SCHEMA_TABLE(iTabDb),0,zDb)){
      goto trigger_cleanup;
    }
  }
#endif

  




  if (tr_tm == TK_INSTEAD){
    tr_tm = TK_BEFORE;
  }

  
  pTrigger = (Trigger*)sqliteMalloc(sizeof(Trigger));
  if( pTrigger==0 ) goto trigger_cleanup;
  pTrigger->name = zName;
  zName = 0;
  pTrigger->table = sqliteStrDup(pTableName->a[0].zName);
  pTrigger->pSchema = db->aDb[iDb].pSchema;
  pTrigger->pTabSchema = pTab->pSchema;
  pTrigger->op = op;
  pTrigger->tr_tm = tr_tm==TK_BEFORE ? TRIGGER_BEFORE : TRIGGER_AFTER;
  pTrigger->pWhen = sqlite3ExprDup(pWhen);
  pTrigger->pColumns = sqlite3IdListDup(pColumns);
  pTrigger->foreach = foreach;
  sqlite3TokenCopy(&pTrigger->nameToken,pName);
  assert( pParse->pNewTrigger==0 );
  pParse->pNewTrigger = pTrigger;

trigger_cleanup:
  sqliteFree(zName);
  sqlite3SrcListDelete(pTableName);
  sqlite3IdListDelete(pColumns);
  sqlite3ExprDelete(pWhen);
  if( !pParse->pNewTrigger ){
    sqlite3DeleteTrigger(pTrigger);
  }else{
    assert( pParse->pNewTrigger==pTrigger );
  }
}





void sqlite3FinishTrigger(
  Parse *pParse,          
  TriggerStep *pStepList, 
  Token *pAll             
){
  Trigger *pTrig = 0;     
  sqlite3 *db = pParse->db;  
  DbFixer sFix;
  int iDb;                   

  pTrig = pParse->pNewTrigger;
  pParse->pNewTrigger = 0;
  if( pParse->nErr || !pTrig ) goto triggerfinish_cleanup;
  iDb = sqlite3SchemaToIndex(pParse->db, pTrig->pSchema);
  pTrig->step_list = pStepList;
  while( pStepList ){
    pStepList->pTrig = pTrig;
    pStepList = pStepList->pNext;
  }
  if( sqlite3FixInit(&sFix, pParse, iDb, "trigger", &pTrig->nameToken) 
          && sqlite3FixTriggerStep(&sFix, pTrig->step_list) ){
    goto triggerfinish_cleanup;
  }

  


  if( !db->init.busy ){
    static const VdbeOpList insertTrig[] = {
      { OP_NewRowid,   0, 0,  0          },
      { OP_String8,    0, 0,  "trigger"  },
      { OP_String8,    0, 0,  0          },  
      { OP_String8,    0, 0,  0          },  
      { OP_Integer,    0, 0,  0          },
      { OP_String8,    0, 0,  "CREATE TRIGGER "},
      { OP_String8,    0, 0,  0          },  
      { OP_Concat,     0, 0,  0          }, 
      { OP_MakeRecord, 5, 0,  "aaada"    },
      { OP_Insert,     0, 0,  0          },
    };
    int addr;
    Vdbe *v;

    
    v = sqlite3GetVdbe(pParse);
    if( v==0 ) goto triggerfinish_cleanup;
    sqlite3BeginWriteOperation(pParse, 0, iDb);
    sqlite3OpenMasterTable(pParse, iDb);
    addr = sqlite3VdbeAddOpList(v, ArraySize(insertTrig), insertTrig);
    sqlite3VdbeChangeP3(v, addr+2, pTrig->name, 0); 
    sqlite3VdbeChangeP3(v, addr+3, pTrig->table, 0); 
    sqlite3VdbeChangeP3(v, addr+6, (char*)pAll->z, pAll->n);
    sqlite3ChangeCookie(db, v, iDb);
    sqlite3VdbeAddOp(v, OP_Close, 0, 0);
    sqlite3VdbeOp3(v, OP_ParseSchema, iDb, 0, 
       sqlite3MPrintf("type='trigger' AND name='%q'", pTrig->name), P3_DYNAMIC);
  }

  if( db->init.busy ){
    int n;
    Table *pTab;
    Trigger *pDel;
    pDel = sqlite3HashInsert(&db->aDb[iDb].pSchema->trigHash, 
                     pTrig->name, strlen(pTrig->name), pTrig);
    if( pDel ){
      assert( sqlite3MallocFailed() && pDel==pTrig );
      goto triggerfinish_cleanup;
    }
    n = strlen(pTrig->table) + 1;
    pTab = sqlite3HashFind(&pTrig->pTabSchema->tblHash, pTrig->table, n);
    assert( pTab!=0 );
    pTrig->pNext = pTab->pTrigger;
    pTab->pTrigger = pTrig;
    pTrig = 0;
  }

triggerfinish_cleanup:
  sqlite3DeleteTrigger(pTrig);
  assert( !pParse->pNewTrigger );
  sqlite3DeleteTriggerStep(pStepList);
}











static void sqlitePersistTriggerStep(TriggerStep *p){
  if( p->target.z ){
    p->target.z = (u8*)sqliteStrNDup((char*)p->target.z, p->target.n);
    p->target.dyn = 1;
  }
  if( p->pSelect ){
    Select *pNew = sqlite3SelectDup(p->pSelect);
    sqlite3SelectDelete(p->pSelect);
    p->pSelect = pNew;
  }
  if( p->pWhere ){
    Expr *pNew = sqlite3ExprDup(p->pWhere);
    sqlite3ExprDelete(p->pWhere);
    p->pWhere = pNew;
  }
  if( p->pExprList ){
    ExprList *pNew = sqlite3ExprListDup(p->pExprList);
    sqlite3ExprListDelete(p->pExprList);
    p->pExprList = pNew;
  }
  if( p->pIdList ){
    IdList *pNew = sqlite3IdListDup(p->pIdList);
    sqlite3IdListDelete(p->pIdList);
    p->pIdList = pNew;
  }
}








TriggerStep *sqlite3TriggerSelectStep(Select *pSelect){
  TriggerStep *pTriggerStep = sqliteMalloc(sizeof(TriggerStep));
  if( pTriggerStep==0 ) {
    sqlite3SelectDelete(pSelect);
    return 0;
  }

  pTriggerStep->op = TK_SELECT;
  pTriggerStep->pSelect = pSelect;
  pTriggerStep->orconf = OE_Default;
  sqlitePersistTriggerStep(pTriggerStep);

  return pTriggerStep;
}








TriggerStep *sqlite3TriggerInsertStep(
  Token *pTableName,  
  IdList *pColumn,    
  ExprList *pEList,   
  Select *pSelect,    
  int orconf          
){
  TriggerStep *pTriggerStep = sqliteMalloc(sizeof(TriggerStep));

  assert(pEList == 0 || pSelect == 0);
  assert(pEList != 0 || pSelect != 0);

  if( pTriggerStep ){
    pTriggerStep->op = TK_INSERT;
    pTriggerStep->pSelect = pSelect;
    pTriggerStep->target  = *pTableName;
    pTriggerStep->pIdList = pColumn;
    pTriggerStep->pExprList = pEList;
    pTriggerStep->orconf = orconf;
    sqlitePersistTriggerStep(pTriggerStep);
  }else{
    sqlite3IdListDelete(pColumn);
    sqlite3ExprListDelete(pEList);
    sqlite3SelectDup(pSelect);
  }

  return pTriggerStep;
}






TriggerStep *sqlite3TriggerUpdateStep(
  Token *pTableName,   
  ExprList *pEList,    
  Expr *pWhere,        
  int orconf           
){
  TriggerStep *pTriggerStep = sqliteMalloc(sizeof(TriggerStep));
  if( pTriggerStep==0 ) return 0;

  pTriggerStep->op = TK_UPDATE;
  pTriggerStep->target  = *pTableName;
  pTriggerStep->pExprList = pEList;
  pTriggerStep->pWhere = pWhere;
  pTriggerStep->orconf = orconf;
  sqlitePersistTriggerStep(pTriggerStep);

  return pTriggerStep;
}






TriggerStep *sqlite3TriggerDeleteStep(Token *pTableName, Expr *pWhere){
  TriggerStep *pTriggerStep = sqliteMalloc(sizeof(TriggerStep));
  if( pTriggerStep==0 ) return 0;

  pTriggerStep->op = TK_DELETE;
  pTriggerStep->target  = *pTableName;
  pTriggerStep->pWhere = pWhere;
  pTriggerStep->orconf = OE_Default;
  sqlitePersistTriggerStep(pTriggerStep);

  return pTriggerStep;
}




void sqlite3DeleteTrigger(Trigger *pTrigger){
  if( pTrigger==0 ) return;
  sqlite3DeleteTriggerStep(pTrigger->step_list);
  sqliteFree(pTrigger->name);
  sqliteFree(pTrigger->table);
  sqlite3ExprDelete(pTrigger->pWhen);
  sqlite3IdListDelete(pTrigger->pColumns);
  if( pTrigger->nameToken.dyn ) sqliteFree((char*)pTrigger->nameToken.z);
  sqliteFree(pTrigger);
}









void sqlite3DropTrigger(Parse *pParse, SrcList *pName){
  Trigger *pTrigger = 0;
  int i;
  const char *zDb;
  const char *zName;
  int nName;
  sqlite3 *db = pParse->db;

  if( sqlite3MallocFailed() ) goto drop_trigger_cleanup;
  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    goto drop_trigger_cleanup;
  }

  assert( pName->nSrc==1 );
  zDb = pName->a[0].zDatabase;
  zName = pName->a[0].zName;
  nName = strlen(zName);
  for(i=OMIT_TEMPDB; i<db->nDb; i++){
    int j = (i<2) ? i^1 : i;  
    if( zDb && sqlite3StrICmp(db->aDb[j].zName, zDb) ) continue;
    pTrigger = sqlite3HashFind(&(db->aDb[j].pSchema->trigHash), zName, nName);
    if( pTrigger ) break;
  }
  if( !pTrigger ){
    sqlite3ErrorMsg(pParse, "no such trigger: %S", pName, 0);
    goto drop_trigger_cleanup;
  }
  sqlite3DropTriggerPtr(pParse, pTrigger);

drop_trigger_cleanup:
  sqlite3SrcListDelete(pName);
}





static Table *tableOfTrigger(Trigger *pTrigger){
  int n = strlen(pTrigger->table) + 1;
  return sqlite3HashFind(&pTrigger->pTabSchema->tblHash, pTrigger->table, n);
}





void sqlite3DropTriggerPtr(Parse *pParse, Trigger *pTrigger){
  Table   *pTable;
  Vdbe *v;
  sqlite3 *db = pParse->db;
  int iDb;

  iDb = sqlite3SchemaToIndex(pParse->db, pTrigger->pSchema);
  assert( iDb>=0 && iDb<db->nDb );
  pTable = tableOfTrigger(pTrigger);
  assert( pTable );
  assert( pTable->pSchema==pTrigger->pSchema || iDb==1 );
#ifndef SQLITE_OMIT_AUTHORIZATION
  {
    int code = SQLITE_DROP_TRIGGER;
    const char *zDb = db->aDb[iDb].zName;
    const char *zTab = SCHEMA_TABLE(iDb);
    if( iDb==1 ) code = SQLITE_DROP_TEMP_TRIGGER;
    if( sqlite3AuthCheck(pParse, code, pTrigger->name, pTable->zName, zDb) ||
      sqlite3AuthCheck(pParse, SQLITE_DELETE, zTab, 0, zDb) ){
      return;
    }
  }
#endif

  

  assert( pTable!=0 );
  if( (v = sqlite3GetVdbe(pParse))!=0 ){
    int base;
    static const VdbeOpList dropTrigger[] = {
      { OP_Rewind,     0, ADDR(9),  0},
      { OP_String8,    0, 0,        0}, 
      { OP_Column,     0, 1,        0},
      { OP_Ne,         0, ADDR(8),  0},
      { OP_String8,    0, 0,        "trigger"},
      { OP_Column,     0, 0,        0},
      { OP_Ne,         0, ADDR(8),  0},
      { OP_Delete,     0, 0,        0},
      { OP_Next,       0, ADDR(1),  0}, 
    };

    sqlite3BeginWriteOperation(pParse, 0, iDb);
    sqlite3OpenMasterTable(pParse, iDb);
    base = sqlite3VdbeAddOpList(v,  ArraySize(dropTrigger), dropTrigger);
    sqlite3VdbeChangeP3(v, base+1, pTrigger->name, 0);
    sqlite3ChangeCookie(db, v, iDb);
    sqlite3VdbeAddOp(v, OP_Close, 0, 0);
    sqlite3VdbeOp3(v, OP_DropTrigger, iDb, 0, pTrigger->name, 0);
  }
}




void sqlite3UnlinkAndDeleteTrigger(sqlite3 *db, int iDb, const char *zName){
  Trigger *pTrigger;
  int nName = strlen(zName);
  pTrigger = sqlite3HashInsert(&(db->aDb[iDb].pSchema->trigHash),
                               zName, nName, 0);
  if( pTrigger ){
    Table *pTable = tableOfTrigger(pTrigger);
    assert( pTable!=0 );
    if( pTable->pTrigger == pTrigger ){
      pTable->pTrigger = pTrigger->pNext;
    }else{
      Trigger *cc = pTable->pTrigger;
      while( cc ){ 
        if( cc->pNext == pTrigger ){
          cc->pNext = cc->pNext->pNext;
          break;
        }
        cc = cc->pNext;
      }
      assert(cc);
    }
    sqlite3DeleteTrigger(pTrigger);
    db->flags |= SQLITE_InternChanges;
  }
}










static int checkColumnOverLap(IdList *pIdList, ExprList *pEList){
  int e;
  if( !pIdList || !pEList ) return 1;
  for(e=0; e<pEList->nExpr; e++){
    if( sqlite3IdListIndex(pIdList, pEList->a[e].zName)>=0 ) return 1;
  }
  return 0; 
}










int sqlite3TriggersExist(
  Parse *pParse,          
  Table *pTab,            
  int op,                 
  ExprList *pChanges      
){
  Trigger *pTrigger = pTab->pTrigger;
  int mask = 0;

  while( pTrigger ){
    if( pTrigger->op==op && checkColumnOverLap(pTrigger->pColumns, pChanges) ){
      mask |= pTrigger->tr_tm;
    }
    pTrigger = pTrigger->pNext;
  }
  return mask;
}











static SrcList *targetSrcList(
  Parse *pParse,       
  TriggerStep *pStep   
){
  Token sDb;           
  int iDb;             
  SrcList *pSrc;       

  iDb = sqlite3SchemaToIndex(pParse->db, pStep->pTrig->pSchema);
  if( iDb==0 || iDb>=2 ){
    assert( iDb<pParse->db->nDb );
    sDb.z = (u8*)pParse->db->aDb[iDb].zName;
    sDb.n = strlen((char*)sDb.z);
    pSrc = sqlite3SrcListAppend(0, &sDb, &pStep->target);
  } else {
    pSrc = sqlite3SrcListAppend(0, &pStep->target, 0);
  }
  return pSrc;
}





static int codeTriggerProgram(
  Parse *pParse,            
  TriggerStep *pStepList,   
  int orconfin                
){
  TriggerStep * pTriggerStep = pStepList;
  int orconf;
  Vdbe *v = pParse->pVdbe;

  assert( pTriggerStep!=0 );
  assert( v!=0 );
  sqlite3VdbeAddOp(v, OP_ContextPush, 0, 0);
  VdbeComment((v, "# begin trigger %s", pStepList->pTrig->name));
  while( pTriggerStep ){
    orconf = (orconfin == OE_Default)?pTriggerStep->orconf:orconfin;
    pParse->trigStack->orconf = orconf;
    switch( pTriggerStep->op ){
      case TK_SELECT: {
	Select * ss = sqlite3SelectDup(pTriggerStep->pSelect);		  
	assert(ss);
	assert(ss->pSrc);
        sqlite3SelectResolve(pParse, ss, 0);
	sqlite3Select(pParse, ss, SRT_Discard, 0, 0, 0, 0, 0);
	sqlite3SelectDelete(ss);
	break;
      }
      case TK_UPDATE: {
        SrcList *pSrc;
        pSrc = targetSrcList(pParse, pTriggerStep);
        sqlite3VdbeAddOp(v, OP_ResetCount, 0, 0);
        sqlite3Update(pParse, pSrc,
		sqlite3ExprListDup(pTriggerStep->pExprList), 
		sqlite3ExprDup(pTriggerStep->pWhere), orconf);
        sqlite3VdbeAddOp(v, OP_ResetCount, 1, 0);
        break;
      }
      case TK_INSERT: {
        SrcList *pSrc;
        pSrc = targetSrcList(pParse, pTriggerStep);
        sqlite3VdbeAddOp(v, OP_ResetCount, 0, 0);
        sqlite3Insert(pParse, pSrc,
          sqlite3ExprListDup(pTriggerStep->pExprList), 
          sqlite3SelectDup(pTriggerStep->pSelect), 
          sqlite3IdListDup(pTriggerStep->pIdList), orconf);
        sqlite3VdbeAddOp(v, OP_ResetCount, 1, 0);
        break;
      }
      case TK_DELETE: {
        SrcList *pSrc;
        sqlite3VdbeAddOp(v, OP_ResetCount, 0, 0);
        pSrc = targetSrcList(pParse, pTriggerStep);
        sqlite3DeleteFrom(pParse, pSrc, sqlite3ExprDup(pTriggerStep->pWhere));
        sqlite3VdbeAddOp(v, OP_ResetCount, 1, 0);
        break;
      }
      default:
        assert(0);
    } 
    pTriggerStep = pTriggerStep->pNext;
  }
  sqlite3VdbeAddOp(v, OP_ContextPop, 0, 0);
  VdbeComment((v, "# end trigger %s", pStepList->pTrig->name));

  return 0;
}





















int sqlite3CodeRowTrigger(
  Parse *pParse,       
  int op,              
  ExprList *pChanges,  
  int tr_tm,           
  Table *pTab,         
  int newIdx,          
  int oldIdx,          
  int orconf,          
  int ignoreJump       
){
  Trigger *p;
  TriggerStack trigStackEntry;

  assert(op == TK_UPDATE || op == TK_INSERT || op == TK_DELETE);
  assert(tr_tm == TRIGGER_BEFORE || tr_tm == TRIGGER_AFTER );

  assert(newIdx != -1 || oldIdx != -1);

  for(p=pTab->pTrigger; p; p=p->pNext){
    int fire_this = 0;

    
    if( 
      p->op==op && 
      p->tr_tm==tr_tm && 
      (p->pSchema==p->pTabSchema || p->pSchema==pParse->db->aDb[1].pSchema) &&
      (op!=TK_UPDATE||!p->pColumns||checkColumnOverLap(p->pColumns,pChanges))
    ){
      TriggerStack *pS;      
      for(pS=pParse->trigStack; pS && p!=pS->pTrigger; pS=pS->pNext){}
      if( !pS ){
        fire_this = 1;
      }
#if 0    
      else{
        sqlite3ErrorMsg(pParse, "recursive triggers not supported (%s)",
            p->name);
        return SQLITE_ERROR;
      }
#endif
    }
 
    if( fire_this ){
      int endTrigger;
      Expr * whenExpr;
      AuthContext sContext;
      NameContext sNC;

      memset(&sNC, 0, sizeof(sNC));
      sNC.pParse = pParse;

      
      trigStackEntry.pTrigger = p;
      trigStackEntry.newIdx = newIdx;
      trigStackEntry.oldIdx = oldIdx;
      trigStackEntry.pTab = pTab;
      trigStackEntry.pNext = pParse->trigStack;
      trigStackEntry.ignoreJump = ignoreJump;
      pParse->trigStack = &trigStackEntry;
      sqlite3AuthContextPush(pParse, &sContext, p->name);

      
      endTrigger = sqlite3VdbeMakeLabel(pParse->pVdbe);
      whenExpr = sqlite3ExprDup(p->pWhen);
      if( sqlite3ExprResolveNames(&sNC, whenExpr) ){
        pParse->trigStack = trigStackEntry.pNext;
        sqlite3ExprDelete(whenExpr);
        return 1;
      }
      sqlite3ExprIfFalse(pParse, whenExpr, endTrigger, 1);
      sqlite3ExprDelete(whenExpr);

      codeTriggerProgram(pParse, p->step_list, orconf); 

      
      pParse->trigStack = trigStackEntry.pNext;
      sqlite3AuthContextPop(&sContext);

      sqlite3VdbeResolveLabel(pParse->pVdbe, endTrigger);
    }
  }
  return 0;
}
#endif 
