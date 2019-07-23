




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKINTMAP_
#include "morkIntMap.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif






 void
morkNodeMap::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseNodeMap(ev);
    this->MarkShut();
  }
}


morkNodeMap::~morkNodeMap() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkNodeMap::morkNodeMap(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkIntMap(ev, inUsage,  sizeof(morkNode*), ioHeap, ioSlotHeap,
   morkBool_kTrue)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kNodeMap;
}

 void
morkNodeMap::CloseNodeMap(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      this->CutAllNodes(ev);
      this->CloseMap(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




mork_bool
morkNodeMap::AddNode(morkEnv* ev, mork_token inToken, morkNode* ioNode)
  
{
  if ( ioNode && ev->Good() )
  {
    morkNode* node = 0; 
    
    mork_bool put = this->Put(ev, &inToken, &ioNode,
       (void*) 0, &node, (mork_change**) 0);
      
    if ( put ) 
    {
      if ( node && node != ioNode ) 
        node->CutStrongRef(ev);
    }
    
    if ( ev->Bad() || !ioNode->AddStrongRef(ev) )
    {
      
      this->Cut(ev, &inToken,  
         (void*) 0,  (void*) 0, (mork_change**) 0);
    }
  }
  else if ( !ioNode )
    ev->NilPointerError();
    
  return ev->Good();
}

mork_bool
morkNodeMap::CutNode(morkEnv* ev, mork_token inToken)
{
  morkNode* node = 0; 
  mork_bool outCutNode = this->Cut(ev, &inToken, 
     (void*) 0, &node, (mork_change**) 0);
  if ( node )
    node->CutStrongRef(ev);
  
  return outCutNode;
}

morkNode*
morkNodeMap::GetNode(morkEnv* ev, mork_token inToken)
  
{
  morkNode* node = 0; 
  this->Get(ev, &inToken,  (void*) 0, &node, (mork_change**) 0);
  
  return node;
}

mork_num
morkNodeMap::CutAllNodes(morkEnv* ev)
  
{
  mork_num outSlots = mMap_Slots;
  mork_token key = 0; 
  morkNode* val = 0; 
  
  mork_change* c = 0;
  morkNodeMapIter i(ev, this);
  for ( c = i.FirstNode(ev, &key, &val); c ; c = i.NextNode(ev, &key, &val) )
  {
    if ( val )
      val->CutStrongRef(ev);
    i.CutHereNode(ev,  (mork_token*) 0,  (morkNode**) 0);
  }
  
  return outSlots;
}



