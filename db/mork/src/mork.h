




































#ifndef _MORK_
#define _MORK_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#include "nscore.h"




#define MORK_USED_1(x) (void)(&x)
#define MORK_USED_2(x,y) (void)(&x);(void)(&y);
#define MORK_USED_3(x,y,z) (void)(&x);(void)(&y);(void)(&z);
#define MORK_USED_4(w,x,y,z) (void)(&w);(void)(&x);(void)(&y);(void)(&z);






















#define mork_OffsetOf(obj,slot) ((unsigned int)&((obj*) 0)->slot)




typedef unsigned char  mork_u1;  
typedef unsigned short mork_u2;  
typedef short          mork_i2;  
typedef PRUint32       mork_u4;  
typedef PRInt32        mork_i4;  
typedef PRWord         mork_ip;  

typedef mork_u1 mork_ch;    
typedef mork_u1 mork_flags;  

typedef mork_u2 mork_base;    
typedef mork_u2 mork_derived; 
typedef mork_u2 mork_uses;    
typedef mork_u2 mork_refs;    

typedef mork_u4 mork_token;      
typedef mork_token mork_scope;   
typedef mork_token mork_kind;    
typedef mork_token mork_cscode;  
typedef mork_token mork_aid;     

typedef mork_token mork_column;  
typedef mork_column mork_delta;  

typedef mork_token mork_color;   
#define morkColor_kNone ((mork_color) 0)

typedef mork_u4 mork_magic;      

typedef mork_u4 mork_seed;       
typedef mork_u4 mork_count;      
typedef mork_count mork_num;     
typedef mork_u4 mork_size;       
typedef mork_u4 mork_fill;       
typedef mork_u4 mork_more;       

typedef mdb_u4 mork_percent; 

typedef mork_i4 mork_pos; 
typedef mork_i4 mork_line; 

typedef mork_u1 mork_usage;   
typedef mork_u1 mork_access;  

typedef mork_u1 mork_change; 
typedef mork_u1 mork_priority; 

typedef mork_u1 mork_able; 
typedef mork_u1 mork_load; 



typedef mork_i2 mork_test; 

#define morkTest_kVoid ((mork_test) -1) /* -1: nil key slot, no key order */
#define morkTest_kHit  ((mork_test) 0)  /*  0: keys are equal, a map hit */
#define morkTest_kMiss ((mork_test) 1)  /*  1: keys not equal, a map miss */


#define morkPriority_kHi  ((mork_priority) 0) /* best priority */
#define morkPriority_kMin ((mork_priority) 0) /* best priority is smallest */

#define morkPriority_kLo  ((mork_priority) 9) /* worst priority */
#define morkPriority_kMax ((mork_priority) 9) /* worst priority is biggest */

#define morkPriority_kCount 10 /* number of distinct priority values */

#define morkAble_kEnabled  ((mork_able) 0x55) /* same as IronDoc constant */
#define morkAble_kDisabled ((mork_able) 0xAA) /* same as IronDoc constant */
#define morkAble_kAsleep   ((mork_able) 0x5A) /* same as IronDoc constant */

#define morkChange_kAdd 'a' /* add member */
#define morkChange_kCut 'c' /* cut member */
#define morkChange_kPut 'p' /* put member */
#define morkChange_kSet 's' /* set all members */
#define morkChange_kNil 0   /* no change in this member */
#define morkChange_kDup 'd' /* duplicate changes have no effect */



#define morkLoad_kDirty ((mork_load) 0xDD) /* same as IronDoc constant */
#define morkLoad_kClean ((mork_load) 0x22) /* same as IronDoc constant */

#define morkAccess_kOpen    'o'
#define morkAccess_kClosing 'c'
#define morkAccess_kShut    's'
#define morkAccess_kDead    'd'



typedef int mork_char; 
#define morkChar_IsWhite(c) \
  ((c) == 0xA || (c) == 0x9 || (c) == 0xD || (c) == ' ')




typedef mdb_bool mork_bool; 


#define morkBool_kTrue  ((mork_bool) 1) /* actually any nonzero means true */
#define morkBool_kFalse ((mork_bool) 0) /* only zero means false */


typedef mdb_id mork_id;    
typedef mork_id mork_rid;  
typedef mork_id mork_tid;  
typedef mork_id mork_gid;  


typedef mdb_order mork_order; 


#define morkId_kMinusOne ((mdb_id) -1)



class morkMid;
class morkAtom;
class morkAtomSpace;
class morkBookAtom;
class morkBuf;
class morkBuilder;
class morkCell;
class morkCellObject;
class morkCursor;
class morkEnv;
class morkFactory;
class morkFile;
class morkHandle;
class morkHandleFace; 
class morkHandleFrame;
class morkHashArrays;
class morkMap;
class morkNode;
class morkObject;
class morkOidAtom;
class morkParser;
class morkPool;
class morkPlace;
class morkPort;
class morkPortTableCursor;
class morkProbeMap;
class morkRow;
class morkRowCellCursor;
class morkRowObject;
class morkRowSpace;
class morkSorting;
class morkSortingRowCursor;
class morkSpace;
class morkSpan;
class morkStore;
class morkStream;
class morkTable;
class morkTableChange;
class morkTableRowCursor;
class morkThumb;
class morkWriter;
class morkZone;



#ifndef _MORKCONFIG_
#include "morkConfig.h"
#endif



#endif 
