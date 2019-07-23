




































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

#ifndef _MORKYARN_
#include "morkYarn.h"
#endif






 void
morkYarn::CloseMorkNode(morkEnv* ev)  
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseYarn(ev);
    this->MarkShut();
  }
}


morkYarn::~morkYarn()  
{
  MORK_ASSERT(mYarn_Body.mYarn_Buf==0);
}


morkYarn::morkYarn(morkEnv* ev, 
  const morkUsage& inUsage, nsIMdbHeap* ioHeap)
  : morkNode(ev, inUsage, ioHeap)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kYarn;
}

 void
morkYarn::CloseYarn(morkEnv* ev)  
{
  if ( this )
  {
    if ( this->IsNode() )
      this->MarkShut();
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkYarn::NonYarnTypeError(morkEnv* ev)
{
  ev->NewError("non morkYarn");
}


