




































#ifndef _MORKCELL_
#define _MORKCELL_ 1

#ifndef _MORK_
#include "mork.h"
#endif



#define morkDelta_kShift 8 /* 8 bit shift */
#define morkDelta_kChangeMask 0x0FF /* low 8 bit mask */
#define morkDelta_kColumnMask (~ (mork_column) morkDelta_kChangeMask)
#define morkDelta_Init(self,cl,ch) ((self) = ((cl)<<morkDelta_kShift) | (ch))
#define morkDelta_Change(self) ((mork_change) ((self) & morkDelta_kChangeMask))
#define morkDelta_Column(self) ((self) >> morkDelta_kShift)

class morkCell { 

public:
  mork_delta   mCell_Delta;   
  morkAtom*    mCell_Atom;    
  
public:
  morkCell() : mCell_Delta( 0 ), mCell_Atom( 0 ) { }

  morkCell(const morkCell& c)
  : mCell_Delta( c.mCell_Delta ), mCell_Atom( c.mCell_Atom ) { }
  
  
  morkCell(mork_column inCol, mork_change inChange, morkAtom* ioAtom)
  {
    morkDelta_Init(mCell_Delta, inCol,inChange);
    mCell_Atom = ioAtom;
  }

  
  void Init(mork_column inCol, mork_change inChange, morkAtom* ioAtom)
  {
    morkDelta_Init(mCell_Delta,inCol,inChange);
    mCell_Atom = ioAtom;
  }
  
  mork_column  GetColumn() const { return morkDelta_Column(mCell_Delta); }
  mork_change  GetChange() const { return morkDelta_Change(mCell_Delta); }
  
  mork_bool IsCellClean() const { return GetChange() == morkChange_kNil; }
  mork_bool IsCellDirty() const { return GetChange() != morkChange_kNil; }

  void SetCellClean(); 
  void SetCellDirty(); 
  
  void SetCellColumnDirty(mork_column inCol)
  { this->SetColumnAndChange(inCol, morkChange_kAdd); }
  
  void SetCellColumnClean(mork_column inCol)
  { this->SetColumnAndChange(inCol, morkChange_kNil); }
  
  void         SetColumnAndChange(mork_column inCol, mork_change inChange)
  { morkDelta_Init(mCell_Delta, inCol, inChange); }
    
  morkAtom*  GetAtom() { return mCell_Atom; }
  
  void       SetAtom(morkEnv* ev, morkAtom* ioAtom, morkPool* ioPool);
  
  
  
  
  
  
  
  
  
  
  
  
  void       SetYarn(morkEnv* ev, const mdbYarn* inYarn, morkStore* ioStore);
  
  void       AliasYarn(morkEnv* ev, mdbYarn* outYarn) const;
  void       GetYarn(morkEnv* ev, mdbYarn* outYarn) const;
};



#endif 
