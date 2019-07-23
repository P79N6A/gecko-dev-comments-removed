




































#ifndef _MORKATOM_
#define _MORKATOM_ 1

#ifndef _MORK_
#include "mork.h"
#endif




#define morkAtom_kMaxByteSize 255 /* max for 8-bit integer */
#define morkAtom_kForeverCellUses 0x0FF /* max for 8-bit integer */
#define morkAtom_kMaxCellUses 0x07F /* max for 7-bit integer */

#define morkAtom_kKindWeeAnon  'a'  /* means morkWeeAnonAtom subclass */
#define morkAtom_kKindBigAnon  'A'  /* means morkBigAnonAtom subclass */
#define morkAtom_kKindWeeBook  'b'  /* means morkWeeBookAtom subclass */
#define morkAtom_kKindBigBook  'B'  /* means morkBigBookAtom subclass */
#define morkAtom_kKindFarBook  'f'  /* means morkFarBookAtom subclass */
#define morkAtom_kKindRowOid   'r'  /* means morkOidAtom subclass */
#define morkAtom_kKindTableOid 't'  /* means morkOidAtom subclass */



class morkAtom { 
 
public: 

  mork_u1       mAtom_Kind;      
  mork_u1       mAtom_CellUses;  
  mork_change   mAtom_Change;    
  mork_u1       mAtom_Size;      

public: 
  morkAtom(mork_aid inAid, mork_u1 inKind);
  
  mork_bool IsWeeAnon() const { return mAtom_Kind == morkAtom_kKindWeeAnon; }
  mork_bool IsBigAnon() const { return mAtom_Kind == morkAtom_kKindBigAnon; }
  mork_bool IsWeeBook() const { return mAtom_Kind == morkAtom_kKindWeeBook; }
  mork_bool IsBigBook() const { return mAtom_Kind == morkAtom_kKindBigBook; }
  mork_bool IsFarBook() const { return mAtom_Kind == morkAtom_kKindFarBook; }
  mork_bool IsRowOid() const { return mAtom_Kind == morkAtom_kKindRowOid; }
  mork_bool IsTableOid() const { return mAtom_Kind == morkAtom_kKindTableOid; }

  mork_bool IsBook() const { return this->IsWeeBook() || this->IsBigBook(); }

public: 

  void SetAtomClean() { mAtom_Change = morkChange_kNil; }
  void SetAtomDirty() { mAtom_Change = morkChange_kAdd; }
  
  mork_bool IsAtomClean() const { return mAtom_Change == morkChange_kNil; }
  mork_bool IsAtomDirty() const { return mAtom_Change == morkChange_kAdd; }

public: 

  mork_scope GetBookAtomSpaceScope(morkEnv* ev) const;
  

  mork_aid   GetBookAtomAid() const;
  
 
public: 
  morkAtom() { }

public: 
  void       MakeCellUseForever(morkEnv* ev);
  mork_u1    AddCellUse(morkEnv* ev);
  mork_u1    CutCellUse(morkEnv* ev);
  
  mork_bool  IsCellUseForever() const 
  { return mAtom_CellUses == morkAtom_kForeverCellUses; }
  
private: 

  static void CellUsesUnderflowWarning(morkEnv* ev);

public: 

  static void BadAtomKindError(morkEnv* ev);
  static void ZeroAidError(morkEnv* ev);
  static void AtomSizeOverflowError(morkEnv* ev);

public: 
  
  mork_bool   AsBuf(morkBuf& outBuf) const;
  mork_bool   AliasYarn(mdbYarn* outYarn) const;
  mork_bool   GetYarn(mdbYarn* outYarn) const;

private: 
  morkAtom(const morkAtom& other);
  morkAtom& operator=(const morkAtom& other);
};



class morkOidAtom : public morkAtom { 

  
  
  
  
 
public:
  mdbOid           mOidAtom_Oid;       

public: 
  morkOidAtom() { }
  void InitRowOidAtom(morkEnv* ev, const mdbOid& inOid);
  void InitTableOidAtom(morkEnv* ev, const mdbOid& inOid);

private: 
  morkOidAtom(const morkOidAtom& other);
  morkOidAtom& operator=(const morkOidAtom& other);
};
















class morkWeeAnonAtom : public morkAtom { 

  
  
  
  
  
public:
  mork_u1 mWeeAnonAtom_Body[ 1 ]; 

public: 
  morkWeeAnonAtom() { }
  void InitWeeAnonAtom(morkEnv* ev, const morkBuf& inBuf);
  
  
  static mork_size SizeForFill(mork_fill inFill)
  { return sizeof(morkWeeAnonAtom) + inFill; }

private: 
  morkWeeAnonAtom(const morkWeeAnonAtom& other);
  morkWeeAnonAtom& operator=(const morkWeeAnonAtom& other);
};








class morkBigAnonAtom : public morkAtom { 

  
  
  
  
 
public:
  mork_cscode   mBigAnonAtom_Form;      
  mork_size     mBigAnonAtom_Size;      
  mork_u1       mBigAnonAtom_Body[ 1 ]; 

public: 
  morkBigAnonAtom() { }
  void InitBigAnonAtom(morkEnv* ev, const morkBuf& inBuf, mork_cscode inForm);
  
  
  static mork_size SizeForFill(mork_fill inFill)
  { return sizeof(morkBigAnonAtom) + inFill; }

private: 
  morkBigAnonAtom(const morkBigAnonAtom& other);
  morkBigAnonAtom& operator=(const morkBigAnonAtom& other);
};

#define morkBookAtom_kMaxBodySize 1024 /* if larger, cannot be shared */





class morkBookAtom : public morkAtom { 
  
  
  
  
  
public:
  morkAtomSpace* mBookAtom_Space; 
  mork_aid       mBookAtom_Id;    

public: 
  morkBookAtom() { }

  static void NonBookAtomTypeError(morkEnv* ev);

public: 

  mork_u4 HashAid() const { return mBookAtom_Id; }
  mork_bool EqualAid(const morkBookAtom* inAtom) const
  { return ( mBookAtom_Id == inAtom->mBookAtom_Id); }

public: 
  
  

  mork_u4 HashFormAndBody(morkEnv* ev) const;
  mork_bool EqualFormAndBody(morkEnv* ev, const morkBookAtom* inAtom) const;
  
public: 

  void CutBookAtomFromSpace(morkEnv* ev);

private: 
  morkBookAtom(const morkBookAtom& other);
  morkBookAtom& operator=(const morkBookAtom& other);
};













class morkFarBookAtom : public morkBookAtom { 
  
  
  
  
  

  
  
  
public:
  mork_cscode   mFarBookAtom_Form;      
  mork_size     mFarBookAtom_Size;      
  mork_u1*      mFarBookAtom_Body;      

public: 
  morkFarBookAtom() { }
  void InitFarBookAtom(morkEnv* ev, const morkBuf& inBuf,
    mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid);
  
private: 
  morkFarBookAtom(const morkFarBookAtom& other);
  morkFarBookAtom& operator=(const morkFarBookAtom& other);
};



class morkWeeBookAtom : public morkBookAtom { 
  
  
  
  

  
  
  
public:
  mork_u1     mWeeBookAtom_Body[ 1 ]; 

public: 
  morkWeeBookAtom() { }
  morkWeeBookAtom(mork_aid inAid);
  
  void InitWeeBookAtom(morkEnv* ev, const morkBuf& inBuf,
    morkAtomSpace* ioSpace, mork_aid inAid);
  
  
  static mork_size SizeForFill(mork_fill inFill)
  { return sizeof(morkWeeBookAtom) + inFill; }

private: 
  morkWeeBookAtom(const morkWeeBookAtom& other);
  morkWeeBookAtom& operator=(const morkWeeBookAtom& other);
};



class morkBigBookAtom : public morkBookAtom { 
  
  
  
  
  

  
  
  
public:
  mork_cscode   mBigBookAtom_Form;      
  mork_size     mBigBookAtom_Size;      
  mork_u1       mBigBookAtom_Body[ 1 ]; 

public: 
  morkBigBookAtom() { }
  void InitBigBookAtom(morkEnv* ev, const morkBuf& inBuf,
    mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid);
  
  
  static mork_size SizeForFill(mork_fill inFill)
  { return sizeof(morkBigBookAtom) + inFill; }

private: 
  morkBigBookAtom(const morkBigBookAtom& other);
  morkBigBookAtom& operator=(const morkBigBookAtom& other);
};



class morkMaxBookAtom : public morkBigBookAtom { 
  
  
  
  
  

  
  

  
  
  
  
public:
  mork_u1 mMaxBookAtom_Body[ morkBookAtom_kMaxBodySize + 3 ]; 

public: 
  morkMaxBookAtom() { }
  void InitMaxBookAtom(morkEnv* ev, const morkBuf& inBuf,
    mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid)
  { this->InitBigBookAtom(ev, inBuf, inForm, ioSpace, inAid); }

private: 
  morkMaxBookAtom(const morkMaxBookAtom& other);
  morkMaxBookAtom& operator=(const morkMaxBookAtom& other);
};



#endif 

