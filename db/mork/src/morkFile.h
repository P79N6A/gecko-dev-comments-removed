




































#ifndef _MORKFILE_
#define _MORKFILE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif







#define morkDerived_kFile 0x4669 /* ascii 'Fi' */

class morkFile  : public morkObject, public nsIMdbFile { 


  

  
  
  
  
  
  
  
  
  
  
  


  
  


protected: 

  mork_u1     mFile_Frozen;   
  mork_u1     mFile_DoTrace;  
  mork_u1     mFile_IoOpen;   
  mork_u1     mFile_Active;   
  
  nsIMdbHeap* mFile_SlotHeap; 
  char*       mFile_Name; 
  

  nsIMdbFile* mFile_Thief; 
  

public: 
  NS_DECL_ISUPPORTS_INHERITED
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkFile(); 
  
public: 
  morkFile(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    nsIMdbHeap* ioSlotHeap);
  void CloseFile(morkEnv* ev); 

private: 
  morkFile(const morkFile& other);
  morkFile& operator=(const morkFile& other);

public: 
  mork_bool IsFile() const
  { return IsNode() && mNode_Derived == morkDerived_kFile; }

    

public: 

  static morkFile* OpenOldFile(morkEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath, mork_bool inFrozen);
  
  
  
  
  
  

  static morkFile* CreateNewFile(morkEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath);
  
  
  
  
  
  
  
public: 

  nsIMdbFile* AcquireFileHandle(morkEnv* ev); 
  
  mork_bool FileFrozen() const  { return mFile_Frozen == 'F'; }
  mork_bool FileDoTrace() const { return mFile_DoTrace == 'T'; }
  mork_bool FileIoOpen() const  { return mFile_IoOpen == 'O'; }
  mork_bool FileActive() const  { return mFile_Active == 'A'; }

  void SetFileFrozen(mork_bool b)  { mFile_Frozen = (mork_u1) ((b)? 'F' : 0); }
  void SetFileDoTrace(mork_bool b) { mFile_DoTrace = (mork_u1) ((b)? 'T' : 0); }
  void SetFileIoOpen(mork_bool b)  { mFile_IoOpen = (mork_u1) ((b)? 'O' : 0); }
  void SetFileActive(mork_bool b)  { mFile_Active = (mork_u1) ((b)? 'A' : 0); }

  
  mork_bool IsOpenActiveAndMutableFile() const
  { return ( IsOpenNode() && FileActive() && !FileFrozen() ); }
    
  
  mork_bool IsOpenAndActiveFile() const
  { return ( this->IsOpenNode() && this->FileActive() ); }
    
    

  nsIMdbFile* GetThief() const { return mFile_Thief; }
  void SetThief(morkEnv* ev, nsIMdbFile* ioThief); 
    
  const char* GetFileNameString() const { return mFile_Name; }
  void SetFileName(morkEnv* ev, const char* inName); 
  static void NilSlotHeapError(morkEnv* ev);
  static void NilFileNameError(morkEnv* ev);
  static void NonFileTypeError(morkEnv* ev);
    
  void NewMissingIoError(morkEnv* ev) const;
    
  void NewFileDownError(morkEnv* ev) const;
    
    
 
   void NewFileErrnoError(morkEnv* ev) const;
       

  mork_size WriteNewlines(morkEnv* ev, mork_count inNewlines);
  
         
public: 
  static void SlotWeakFile(morkFile* me,
    morkEnv* ev, morkFile** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongFile(morkFile* me,
    morkEnv* ev, morkFile** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
public:
  virtual mork_pos   Length(morkEnv* ev) const = 0; 
  
  NS_IMETHOD Tell(nsIMdbEnv* ev, mdb_pos* outPos) const = 0;
  NS_IMETHOD Seek(nsIMdbEnv* ev, mdb_pos inPos, mdb_pos *outPos) = 0;
  NS_IMETHOD Eof(nsIMdbEnv* ev, mdb_pos* outPos);
  

  
  NS_IMETHOD Read(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_size* outActualSize) = 0;
  NS_IMETHOD Get(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  
    
  
  NS_IMETHOD  Write(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_size* outActualSize) = 0;
  NS_IMETHOD  Put(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  NS_IMETHOD  Flush(nsIMdbEnv* ev) = 0;
  
    
  
  NS_IMETHOD  Path(nsIMdbEnv* ev, mdbYarn* outFilePath);
  
    
  
  NS_IMETHOD  Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief) = 0;
  NS_IMETHOD  Thief(nsIMdbEnv* ev, nsIMdbFile** acqThief);
  

  
  NS_IMETHOD BecomeTrunk(nsIMdbEnv* ev) = 0;

  NS_IMETHOD AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    nsIMdbFile** acqBud) = 0; 
  



};





#define morkDerived_kStdioFile 0x7346 /* ascii 'sF' */

class morkStdioFile  : public morkFile { 


protected: 

  void* mStdioFile_File;
  
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkStdioFile(); 
  
public: 
  morkStdioFile(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseStdioFile(morkEnv* ev); 

private: 
  morkStdioFile(const morkStdioFile& other);
  morkStdioFile& operator=(const morkStdioFile& other);

public: 
  mork_bool IsStdioFile() const
  { return IsNode() && mNode_Derived == morkDerived_kStdioFile; }


public: 
  static void NonStdioFileTypeError(morkEnv* ev);
    

public: 

  static morkStdioFile* OpenOldStdioFile(morkEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath, mork_bool inFrozen);

  static morkStdioFile* CreateNewStdioFile(morkEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath);

  virtual mork_pos   Length(morkEnv* ev) const; 

  NS_IMETHOD Tell(nsIMdbEnv* ev, mdb_pos* outPos) const;
  NS_IMETHOD Seek(nsIMdbEnv* ev, mdb_pos inPos, mdb_pos *outPos);

  

  
  NS_IMETHOD Read(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_size* outActualSize);
    
  
  NS_IMETHOD  Write(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_size* outActualSize);


  NS_IMETHOD  Flush(nsIMdbEnv* ev);
  
    
  NS_IMETHOD  Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief);
   

  
  NS_IMETHOD BecomeTrunk(nsIMdbEnv* ev);

  NS_IMETHOD AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    nsIMdbFile** acqBud); 
  






protected: 

  void new_stdio_file_fault(morkEnv* ev) const;
    

public: 
    
  morkStdioFile(morkEnv* ev, const morkUsage& inUsage, 
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap,
    const char* inName, const char* inMode);
    

  morkStdioFile(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap,
     void* ioFile, const char* inName, mork_bool inFrozen);
    
  
  void OpenStdio(morkEnv* ev, const char* inName, const char* inMode);
    
  
  void UseStdio(morkEnv* ev, void* ioFile, const char* inName,
    mork_bool inFrozen);
    
    
    
    
    
  void CloseStdio(morkEnv* ev);
    
    
    
    
    
    
public: 
  static void SlotWeakStdioFile(morkStdioFile* me,
    morkEnv* ev, morkStdioFile** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongStdioFile(morkStdioFile* me,
    morkEnv* ev, morkStdioFile** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
