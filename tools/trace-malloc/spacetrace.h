








































#ifndef spacetrace_h__
#define spacetrace_h__











#include "nspr.h"
#include "prlock.h"
#include "prrwlock.h"
#include "nsTraceMalloc.h"
#include "tmreader.h"
#include "formdata.h"




#if defined(HAVE_BOUTELL_GD)
#define ST_WANT_GRAPHS 1
#endif 
#if !defined(ST_WANT_GRAPHS)
#define ST_WANT_GRAPHS 0
#endif







#define REPORT_ERROR(code, function) \
        PR_fprintf(PR_STDERR, "error(%d):\t%s\n", code, #function)
#define REPORT_ERROR_MSG(code, msg) \
        PR_fprintf(PR_STDERR, "error(%d):\t%s\n", code, msg)
#define REPORT_INFO(msg) \
        PR_fprintf(PR_STDOUT, "%s: %s\n", globals.mProgramName, (msg))

#if defined(DEBUG_blythe) && 1
#define REPORT_blythe(code, msg) \
        PR_fprintf(PR_STDOUT, "gab(%d):\t%s\n", code, msg)
#else
#define REPORT_blythe(code, msg)
#endif 







#define CALLSITE_RUN(callsite) \
        ((STRun*)((callsite)->data))








#define ST_PERMS (PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IROTH)
#define ST_FLAGS (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE) 




#define ST_WEIGHT   0 /* size * timeval */
#define ST_SIZE     1
#define ST_TIMEVAL  2
#define ST_COUNT    3
#define ST_HEAPCOST 4




#define ST_FOLLOW_SIBLINGS   0
#define ST_FOLLOW_PARENTS    1




#define STGD_WIDTH           640
#define STGD_HEIGHT          480
#define STGD_MARGIN          75
#define STGD_SPACE_X         (STGD_WIDTH - (2 * STGD_MARGIN))
#define STGD_SPACE_Y         (STGD_HEIGHT - (2 * STGD_MARGIN))




#define ST_DEFAULT_LIFETIME_MIN 10








#define ST_DEFAULT_ALIGNMENT_SIZE 16
#define ST_DEFAULT_OVERHEAD_SIZE 8




#define ST_SUBSTRING_MATCH_MAX 5




#define ST_MAX_PATTERNS_PER_RULE 16




#define ST_ALLOC_STEP 16




#define ST_ROOT_CATEGORY_NAME "All"




#define ST_OPTION_STRING_MAX 256






#define ST_TIMEVAL_RESOLUTION 1000
#define ST_TIMEVAL_FORMAT "%.3f"
#define ST_TIMEVAL_PRINTABLE(timeval) ((PRFloat64)(timeval) / (PRFloat64)ST_TIMEVAL_RESOLUTION)
#define ST_TIMEVAL_PRINTABLE64(timeval) ((PRFloat64)((PRInt64)(timeval)) / (PRFloat64)ST_TIMEVAL_RESOLUTION)
#define ST_TIMEVAL_MAX ((PRUint32)-1 - ((PRUint32)-1 % ST_TIMEVAL_RESOLUTION))

#define ST_MICROVAL_RESOLUTION 1000000
#define ST_MICROVAL_FORMAT "%.6f"
#define ST_MICROVAL_PRINTABLE(timeval) ((PRFloat64)(timeval) / (PRFloat64)ST_MICROVAL_RESOLUTION)
#define ST_MICROVAL_PRINTABLE64(timeval) ((PRFloat64)((PRInt64)(timeval)) / (PRFloat64)ST_MICROVAL_RESOLUTION)
#define ST_MICROVAL_MAX ((PRUint32)-1 - ((PRUint32)-1 % ST_MICROVAL_RESOLUTION))




typedef struct __struct_STCategoryNode STCategoryNode;
typedef struct __struct_STCategoryRule STCategoryRule;







typedef struct __struct_STAllocEvent
{
        



        char mEventType;
        
        




        PRUint32 mTimeval;
        
        




        PRUint32 mHeapID;
        
        




        PRUint32 mHeapSize;

        




        tmcallsite* mCallsite;
} STAllocEvent;










typedef struct __struct_STAllocation
{
        


        PRUint32 mEventCount;
        STAllocEvent* mEvents;
        
        


        PRUint32 mMinTimeval;
        PRUint32 mMaxTimeval;

        


        PRUint32 mRunIndex;

        





        PRUint32 mHeapRuntimeCost;
} STAllocation;






typedef struct __struct_STCallsiteStats
{
        



        PRUint64 mTimeval64;

        



        PRUint64 mWeight64;

        



        PRUint32 mSize;

        




        PRUint32 mStamp;

        




        PRUint32 mCompositeCount;

        




        PRUint32 mHeapRuntimeCost;
} STCallsiteStats;











typedef struct __struct_STRun
{
        


        PRUint32 mAllocationCount;
        STAllocation** mAllocations;

        




        STCallsiteStats *mStats;

} STRun;














struct __struct_STCategoryNode
{
        


        const char *categoryName;

        


        STCategoryNode *parent;

        



        STCategoryNode** children;
        PRUint32 nchildren;

        





        STRun **runs;
};


struct __struct_STCategoryRule
{
        



        char* pats[ST_MAX_PATTERNS_PER_RULE];
        PRUint32 patlen[ST_MAX_PATTERNS_PER_RULE];
        PRUint32 npats;

        


        const char* categoryName;

        


        STCategoryNode* node;
};





typedef struct __struct_STCategoryMapEntry {
    STCategoryNode* node;
    const char * categoryName;
} STCategoryMapEntry;











typedef enum __enum_STOptionGenre
{
    CategoryGenre = 0,
    DataSortGenre,
    DataSetGenre,
    DataSizeGenre,
    UIGenre,
    ServerGenre,
    BatchModeGenre,

    


    MaxGenres
}
STOptionGenre;








#define ST_CMD_OPTION_BOOL(option_name, option_genre, option_help) PRBool m##option_name;
#define ST_CMD_OPTION_STRING(option_name, option_genre, default_value, option_help) char m##option_name[ST_OPTION_STRING_MAX];
#define ST_CMD_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help) char m##option_name[array_size][ST_OPTION_STRING_MAX];
#define ST_CMD_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help) const char** m##option_name; PRUint32 m##option_name##Count;
#define ST_CMD_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help) PRUint32 m##option_name;
#define ST_CMD_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help) PRUint64 m##option_name##64;

typedef struct __struct_STOptions
{
#include "stoptions.h"
}
STOptions;

typedef struct __struct_STContext





































{
    PRRWLock* mRWLock;
    PRUint32 mIndex;
    STRun* mSortedRun;
#if ST_WANT_GRAPHS
    PRLock* mImageLock;
    PRBool mFootprintCached;
    PRBool mTimevalCached;
    PRBool mLifespanCached;
    PRBool mWeightCached;
    PRUint32 mFootprintYData[STGD_SPACE_X];
    PRUint32 mTimevalYData[STGD_SPACE_X];
    PRUint32 mLifespanYData[STGD_SPACE_X];
    PRUint64 mWeightYData64[STGD_SPACE_X];
#endif
}
STContext;


typedef struct __struct_STContextCacheItem





















{
    STOptions mOptions;
    STContext mContext;
    PRInt32 mReferenceCount;
    PRIntervalTime mLastAccessed;
    PRBool mInUse;
}
STContextCacheItem;


typedef struct __struct_STContextCache













{
    PRLock* mLock;
    PRCondVar* mCacheMiss;
    STContextCacheItem* mItems;
    PRUint32 mItemCount;
}
STContextCache;







typedef struct __struct_STRequest
{
        


        PRFileDesc* mFD;

        


        const char* mGetFileName;

        


        const FormData* mGetData;

        


        STOptions mOptions;

        


        STContext* mContext;
} STRequest;







typedef struct __struct_STGlobals
{
        


        const char* mProgramName;

        




        STOptions mCommandLineOptions;

        




        STContextCache mContextCache;

        


        PRUint32 mMallocCount;
        PRUint32 mCallocCount;
        PRUint32 mReallocCount;
        PRUint32 mFreeCount;

        


        PRUint32 mOperationCount;

        


        STRun mRun;

        




        PRUint32 mMinTimeval;
        PRUint32 mMaxTimeval;

        


        PRUint32 mPeakMemoryUsed;
        PRUint32 mMemoryUsed;

        


       STCategoryRule** mCategoryRules;
       PRUint32 mNRules;

       


       STCategoryMapEntry** mCategoryMap;
       PRUint32 mNCategoryMap;

       


       STCategoryNode mCategoryRoot;

       




       tmreader* mTMR;
} STGlobals;





extern STRun* createRun(STContext* inContext, PRUint32 aStamp);
extern void freeRun(STRun* aRun);
extern int initCategories(STGlobals* g);
extern int categorizeRun(STOptions* inOptions, STContext* inContext, const STRun* aRun, STGlobals* g);
extern STCategoryNode* findCategoryNode(const char *catName, STGlobals *g);
extern int freeCategories(STGlobals* g);
extern int displayCategoryReport(STRequest* inRequest, STCategoryNode *root, int depth);

extern int recalculateAllocationCost(STOptions* inOptions, STContext* inContext, STRun* aRun, STAllocation* aAllocation, PRBool updateParent);
extern void htmlHeader(STRequest* inRequest, const char* aTitle);
extern void htmlFooter(STRequest* inRequest);
extern void htmlAnchor(STRequest* inRequest,
                       const char* aHref,
                       const char* aText,
                       const char* aTarget,
                       const char* aClass,
                       STOptions* inOptions);
extern char *FormatNumber(PRInt32 num);




extern STGlobals globals;

#endif 
