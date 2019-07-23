




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif



void
morkCell::SetYarn(morkEnv* ev, const mdbYarn* inYarn, morkStore* ioStore)
{
  morkAtom* atom = ioStore->YarnToAtom(ev, inYarn, PR_TRUE );
  if ( atom )
    this->SetAtom(ev, atom, ioStore->StorePool()); 
}

void
morkCell::GetYarn(morkEnv* ev, mdbYarn* outYarn) const
{
  MORK_USED_1(ev);
  mCell_Atom->GetYarn(outYarn);
}

void
morkCell::AliasYarn(morkEnv* ev, mdbYarn* outYarn) const
{
  MORK_USED_1(ev);
  mCell_Atom->AliasYarn(outYarn);
}
  
  
void
morkCell::SetCellClean()
{
  mork_column col = this->GetColumn();
  this->SetColumnAndChange(col, morkChange_kNil);
}
  
void
morkCell::SetCellDirty()
{
  mork_column col = this->GetColumn();
  this->SetColumnAndChange(col, morkChange_kAdd);
}

void
morkCell::SetAtom(morkEnv* ev, morkAtom* ioAtom, morkPool* ioPool)
  
  
  
  
  
  
  
  
  
  
  
{
  morkAtom* oldAtom = mCell_Atom;
  if ( oldAtom != ioAtom ) 
  {
    if ( oldAtom )
    {
      mCell_Atom = 0;
      if ( oldAtom->CutCellUse(ev) == 0 )
      {
      
      




            




      }
    }
    if ( ioAtom )
      ioAtom->AddCellUse(ev);
      
    mCell_Atom = ioAtom;
  }
}


