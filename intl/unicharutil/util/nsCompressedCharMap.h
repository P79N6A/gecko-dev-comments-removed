





































#ifndef NSCOMPRESSEDCHARMAP_H
#define NSCOMPRESSEDCHARMAP_H
#include "prtypes.h"
#include "nsICharRepresentable.h"

#define ALU_SIZE PR_BITS_PER_LONG



#if (ALU_SIZE==32)
#   define ALU_TYPE                PRUint32
#   define CCMAP_POW2(n)           (1L<<(n))
#   define CCMAP_BITS_PER_ALU_LOG2 5
#elif (ALU_SIZE==64)
#   define ALU_TYPE                PRUint64
#   define CCMAP_POW2(n)           (1L<<(n))
#   define CCMAP_BITS_PER_ALU_LOG2 6
#else
#   define ALU_TYPE                PRUint16
#   define CCMAP_POW2(n)           (1<<(n))
#   define CCMAP_BITS_PER_ALU_LOG2 4
#endif


class nsICharRepresentable;

extern PRUint16* CreateEmptyCCMap();
extern PRUint16* MapToCCMap(PRUint32* aMap);
extern PRUint16* MapperToCCMap(nsICharRepresentable *aMapper);
extern void FreeCCMap(PRUint16* &aMap);
extern PRBool NextNonEmptyCCMapPage(const PRUint16 *, PRUint32 *);
extern PRBool IsSameCCMap(PRUint16* ccmap1, PRUint16* ccmap2);
#ifdef DEBUG
void printCCMap(PRUint16* aCCMap);
#endif



extern PRUint16*
MapToCCMapExt(PRUint32* aBmpPlaneMap, PRUint32** aOtherPlaneMaps, PRUint32 aOtherPlaneNum);






















#define CCMAP_MAX_LEN (16+16+16+256+4096)


#define EXTENDED_UNICODE_PLANES    16

class nsCompressedCharMap {
public:
  nsCompressedCharMap();
  ~nsCompressedCharMap();

  PRUint16* NewCCMap();
  PRUint16* FillCCMap(PRUint16* aCCMap);
  PRUint16  GetSize() {return mUsedLen;}
  void      SetChar(PRUint32);
  void      SetChars(PRUint16*);
  void      SetChars(PRUint16, ALU_TYPE*);
  void      SetChars(PRUint32*);
  void      Extend() {mExtended = PR_TRUE;} 

protected:
  union {
    PRUint16 mCCMap[CCMAP_MAX_LEN];
    ALU_TYPE used_for_align; 
                             
  } u;
  PRUint16 mUsedLen;   
  PRUint16 mAllOnesPage;

  PRBool mExtended;

  
  PRUint32* mExtMap[EXTENDED_UNICODE_PLANES+1];
  PRUint32  mMap[UCS2_MAP_LEN];
};



























































#define CCMAP_EMPTY_SIZE_PER_INT16    16


#define CCMAP_EMPTY_MID  CCMAP_NUM_UPPER_POINTERS
#define CCMAP_EMPTY_PAGE CCMAP_EMPTY_MID+CCMAP_NUM_MID_POINTERS






















































#define CCMAP_BITS_PER_PAGE_LOG2    8
#define CCMAP_BITS_PER_PAGE         CCMAP_POW2(CCMAP_BITS_PER_PAGE_LOG2)
#define CCMAP_BIT_INDEX(c)          ((c) & PR_BITMASK(CCMAP_BITS_PER_ALU_LOG2))
#define CCMAP_ALU_INDEX(c)          (((c)>>CCMAP_BITS_PER_ALU_LOG2) \
               & PR_BITMASK(CCMAP_BITS_PER_PAGE_LOG2 - CCMAP_BITS_PER_ALU_LOG2))

#define CCMAP_PAGE_MASK             PR_BITMASK(CCMAP_BITS_PER_PAGE_LOG2)
#define CCMAP_NUM_PRUINT16S_PER_PAGE \
                         (CCMAP_BITS_PER_PAGE/CCMAP_BITS_PER_PRUINT16)

#define CCMAP_NUM_ALUS_PER_PAGE     (CCMAP_BITS_PER_PAGE/CCMAP_BITS_PER_ALU)
#define CCMAP_NUM_UCHARS_PER_PAGE   CCMAP_BITS_PER_PAGE




#define CCMAP_BITS_PER_MID_LOG2     4
#define CCMAP_MID_INDEX(c)          \
      (((c)>>CCMAP_BITS_PER_PAGE_LOG2) & PR_BITMASK(CCMAP_BITS_PER_MID_LOG2))
#define CCMAP_NUM_MID_POINTERS    CCMAP_POW2(CCMAP_BITS_PER_MID_LOG2)
#define CCMAP_NUM_UCHARS_PER_MID    \
               CCMAP_POW2(CCMAP_BITS_PER_MID_LOG2+CCMAP_BITS_PER_PAGE_LOG2)




#define CCMAP_BITS_PER_UPPER_LOG2   4
#define CCMAP_UPPER_INDEX(c)        \
      (((c)>>(CCMAP_BITS_PER_MID_LOG2+CCMAP_BITS_PER_PAGE_LOG2)) \
         & PR_BITMASK(CCMAP_BITS_PER_UPPER_LOG2))
#define CCMAP_NUM_UPPER_POINTERS    CCMAP_POW2(CCMAP_BITS_PER_UPPER_LOG2)




#define CCMAP_BITS_PER_PRUINT16_LOG2 4
#define CCMAP_BITS_PER_PRUINT32_LOG2 5

#define CCMAP_BITS_PER_PRUINT16 CCMAP_POW2(CCMAP_BITS_PER_PRUINT16_LOG2)
#define CCMAP_BITS_PER_PRUINT32 CCMAP_POW2(CCMAP_BITS_PER_PRUINT32_LOG2)
#define CCMAP_BITS_PER_ALU      CCMAP_POW2(CCMAP_BITS_PER_ALU_LOG2)

#define CCMAP_ALUS_PER_PRUINT32  (CCMAP_BITS_PER_PRUINT32/CCMAP_BITS_PER_ALU)
#define CCMAP_PRUINT32S_PER_ALU  (CCMAP_BITS_PER_ALU/CCMAP_BITS_PER_PRUINT32)
#define CCMAP_PRUINT32S_PER_PAGE (CCMAP_BITS_PER_PAGE/CCMAP_BITS_PER_PRUINT32)

#define CCMAP_ALU_MASK       PR_BITMASK(CCMAP_BITS_PER_ALU_LOG2)
#define CCMAP_ALUS_PER_PAGE  CCMAP_POW2(CCMAP_BITS_PER_PAGE_LOG2 \
                                       - CCMAP_BITS_PER_ALU_LOG2)
#define NUM_UNICODE_CHARS    CCMAP_POW2(CCMAP_BITS_PER_UPPER_LOG2 \
                                       +CCMAP_BITS_PER_MID_LOG2 \
                                       +CCMAP_BITS_PER_PAGE_LOG2)
#define CCMAP_TOTAL_PAGES    CCMAP_POW2(CCMAP_BITS_PER_UPPER_LOG2 \
                                       +CCMAP_BITS_PER_MID_LOG2)

#define CCMAP_BEGIN_AT_START_OF_MAP 0xFFFFFFFF






#define CCMAP_TO_MID(m,c) ((m)[CCMAP_UPPER_INDEX(c)])


#define CCMAP_TO_PAGE(m,c) ((m)[CCMAP_TO_MID((m),(c)) + CCMAP_MID_INDEX(c)])


#define CCMAP_TO_ALU(m,c) \
          (*((ALU_TYPE*)(&((m)[CCMAP_TO_PAGE((m),(c))])) + CCMAP_ALU_INDEX(c)))


#define CCMAP_HAS_CHAR(m,c) (((CCMAP_TO_ALU(m,c))>>CCMAP_BIT_INDEX(c)) & 1)


#define CCMAP_UNSET_CHAR(m,c) (CCMAP_TO_ALU(m,c) &= ~(CCMAP_POW2(CCMAP_BIT_INDEX(c))))

#define CCMAP_SIZE(m) (*((m)-1))
#define CCMAP_FLAG(m) (*((m)-2))
#define CCMAP_EXTRA    (sizeof(ALU_TYPE)/sizeof(PRUint16)>2? sizeof(ALU_TYPE)/sizeof(PRUint16): 2)
#define CCMAP_SURROGATE_FLAG         0x0001  
#define CCMAP_NONE_FLAG              0x0000


#define CCMAP_PLANE_FROM_SURROGATE(h)  ((((PRUint16)(h) - (PRUint16)0xd800) >> 6) + 1)


#define CCMAP_PLANE(u)  ((((PRUint32)(u))>>16))


#define CCMAP_INPLANE_OFFSET(h, l) (((((PRUint16)(h) - (PRUint16)0xd800) & 0x3f) << 10) + ((PRUint16)(l) - (PRUint16)0xdc00))


#define CCMAP_FOR_PLANE_EXT(m, i)  ((m) + ((PRUint32*)((m) + CCMAP_SIZE(m)))[(i)-1])


#define CCMAP_HAS_CHAR_EXT2(m, h, l)  (CCMAP_FLAG(m) & CCMAP_SURROGATE_FLAG && \
                                      CCMAP_HAS_CHAR(CCMAP_FOR_PLANE_EXT((m), CCMAP_PLANE_FROM_SURROGATE(h)), CCMAP_INPLANE_OFFSET(h, l)))

#define CCMAP_HAS_CHAR_EXT(m, ucs4)  (((ucs4)&0xffff0000) ?  \
                                      (CCMAP_FLAG(m) & CCMAP_SURROGATE_FLAG) && CCMAP_HAS_CHAR(CCMAP_FOR_PLANE_EXT((m), CCMAP_PLANE(ucs4)), (ucs4) & 0xffff) : \
                                      CCMAP_HAS_CHAR(m, (PRUnichar)(ucs4)) )





     
#define DEFINE_ANY_CCMAP(var, extra, typequal)              \
static typequal union {                                     \
  PRUint16 array[var ## _SIZE];                             \
  ALU_TYPE align;                                           \
} var ## Union =                                            \
{                                                           \
  { var ## _INITIALIZER }                                   \
};                                                          \
static typequal PRUint16* var = var ## Union.array + extra

#define DEFINE_CCMAP(var, typequal)   DEFINE_ANY_CCMAP(var, 0, typequal)
#define DEFINE_X_CCMAP(var, typequal) DEFINE_ANY_CCMAP(var, CCMAP_EXTRA, typequal)

#endif 
