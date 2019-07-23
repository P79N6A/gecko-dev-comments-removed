




































#ifndef _MORKNODEMAP_
#define _MORKNODEMAP_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKINTMAP_
#include "morkIntMap.h"
#endif



#define morkDerived_kNodeMap 0x6E4D /* ascii 'nM' */

#define morkNodeMap_kStartSlotCount 512



class morkNodeMap : public morkIntMap { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkNodeMap(); 
  
public: 
  morkNodeMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseNodeMap(morkEnv* ev); 

public: 
  mork_bool IsNodeMap() const
  { return IsNode() && mNode_Derived == morkDerived_kNodeMap; }



  


protected: 

  mork_bool  AddNode(morkEnv* ev, mork_token inToken, morkNode* ioNode);
  

  mork_bool  CutNode(morkEnv* ev, mork_token inToken);
  
  
  morkNode*  GetNode(morkEnv* ev, mork_token inToken);
  

  mork_num CutAllNodes(morkEnv* ev);
  
};

class morkNodeMapIter: public morkMapIter{ 

public:
  morkNodeMapIter(morkEnv* ev, morkNodeMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkNodeMapIter( ) : morkMapIter()  { }
  void InitNodeMapIter(morkEnv* ev, morkNodeMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstNode(morkEnv* ev, mork_token* outToken, morkNode** outNode)
  { return this->First(ev, outToken, outNode); }
  
  mork_change*
  NextNode(morkEnv* ev, mork_token* outToken, morkNode** outNode)
  { return this->Next(ev, outToken, outNode); }
  
  mork_change*
  HereNode(morkEnv* ev, mork_token* outToken, morkNode** outNode)
  { return this->Here(ev, outToken, outNode); }
  
  mork_change*
  CutHereNode(morkEnv* ev, mork_token* outToken, morkNode** outNode)
  { return this->CutHere(ev, outToken, outNode); }
};



#endif 
