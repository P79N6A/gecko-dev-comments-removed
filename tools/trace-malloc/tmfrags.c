







































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "nspr.h"
#include "tmreader.h"


#define ERROR_REPORT(num, val, msg)   fprintf(stderr, "error(%d):\t\"%s\"\t%s\n", (num), (val), (msg));
#define CLEANUP(ptr)    do { if(NULL != ptr) { free(ptr); ptr = NULL; } } while(0)


#define ticks2msec(reader, ticks) ticks2xsec((reader), (ticks), 1000)
#define ticks2usec(reader, ticks) ticks2xsec((reader), (ticks), 1000000)
#define TICK_RESOLUTION 1000
#define TICK_PRINTABLE(timeval) ((PRFloat64)(timeval) / (PRFloat64)ST_TIMEVAL_RESOLUTION)


typedef struct __struct_Options















{
    const char* mProgramName;
    char* mInputName;
    FILE* mOutput;
    char* mOutputName;
    int mHelp;
    unsigned mOverhead;
    unsigned mAlignment;
    unsigned mPageSize;
}
Options;


typedef struct __struct_Switch



{
    const char* mLongName;
    const char* mShortName;
    int mHasValue;
    const char* mValue;
    const char* mDescription;
}
Switch;

#define DESC_NEWLINE "\n\t\t"

static Switch gInputSwitch = {"--input", "-i", 1, NULL, "Specify input file." DESC_NEWLINE "stdin is default."};
static Switch gOutputSwitch = {"--output", "-o", 1, NULL, "Specify output file." DESC_NEWLINE "Appends if file exists." DESC_NEWLINE "stdout is default."};
static Switch gHelpSwitch = {"--help", "-h", 0, NULL, "Information on usage."};
static Switch gAlignmentSwitch = {"--alignment", "-al", 1, NULL, "All allocation sizes are made to be a multiple of this number." DESC_NEWLINE "Closer to actual heap conditions; set to 1 for true sizes." DESC_NEWLINE "Default value is 16."};
static Switch gOverheadSwitch = {"--overhead", "-ov", 1, NULL, "After alignment, all allocations are made to increase by this number." DESC_NEWLINE "Closer to actual heap conditions; set to 0 for true sizes." DESC_NEWLINE "Default value is 8."};
static Switch gPageSizeSwitch = {"--page-size", "-ps", 1, NULL, "Sets the page size which aids the identification of fragmentation." DESC_NEWLINE "Closer to actual heap conditions; set to 4294967295 for true sizes."  DESC_NEWLINE "Default value is 4096."};

static Switch* gSwitches[] = {
        &gInputSwitch,
        &gOutputSwitch,
        &gAlignmentSwitch,
        &gOverheadSwitch,
        &gPageSizeSwitch,
        &gHelpSwitch
};


typedef struct __struct_AnyArray









{
    void* mItems;
    unsigned mItemSize;
    unsigned mCount;
    unsigned mCapacity;
    unsigned mGrowBy;
}
AnyArray;


typedef int (*arrayMatchFunc)(void* inContext, AnyArray* inArray, void* inItem, unsigned inItemIndex)













;


typedef enum __enum_HeapEventType



{
    FREE,
    ALLOC
}
HeapEventType;


typedef enum __enum_HeapObjectType



{
    ALLOCATION,
    FRAGMENT
}
HeapObjectType;


typedef struct __struct_HeapObject HeapObject;
typedef struct __struct_HeapHistory











{
    unsigned mTimestamp;
    unsigned mTMRSerial;
    unsigned mObjectIndex;
}
HeapHistory;


struct __struct_HeapObject
















{
    unsigned mUniqueID;

    HeapObjectType mType;
    unsigned mHeapOffset;
    unsigned mSize;

    HeapHistory mBirth;
    HeapHistory mDeath;
};


typedef struct __struct_TMState











{
    Options* mOptions;
    tmreader* mTMR;

    int mLoopExitTMR;

    unsigned uMinTicks;
    unsigned uMaxTicks;
}
TMState;


int initOptions(Options* outOptions, int inArgc, char** inArgv)



{
    int retval = 0;
    int loop = 0;
    int switchLoop = 0;
    int match = 0;
    const int switchCount = sizeof(gSwitches) / sizeof(gSwitches[0]);
    Switch* current = NULL;

    


    memset(outOptions, 0, sizeof(Options));
    outOptions->mProgramName = inArgv[0];
    outOptions->mInputName = strdup("-");
    outOptions->mOutput = stdout;
    outOptions->mOutputName = strdup("stdout");
    outOptions->mAlignment = 16;
    outOptions->mOverhead = 8;

    if(NULL == outOptions->mOutputName || NULL == outOptions->mInputName)
    {
        retval = __LINE__;
        ERROR_REPORT(retval, "stdin/stdout", "Unable to strdup.");
    }

    


    for(loop = 1; loop < inArgc && 0 == retval; loop++)
    {
        match = 0;
        current = NULL;

        for(switchLoop = 0; switchLoop < switchCount && 0 == retval; switchLoop++)
        {
            if(0 == strcmp(gSwitches[switchLoop]->mLongName, inArgv[loop]))
            {
                match = __LINE__;
            }
            else if(0 == strcmp(gSwitches[switchLoop]->mShortName, inArgv[loop]))
            {
                match = __LINE__;
            }

            if(match)
            {
                if(gSwitches[switchLoop]->mHasValue)
                {
                    


                    if(loop + 1 < inArgc)
                    {
                        loop++;

                        current = gSwitches[switchLoop];
                        current->mValue = inArgv[loop];
                    }
                }
                else
                {
                    current = gSwitches[switchLoop];
                }

                break;
            }
        }

        if(0 == match)
        {
            outOptions->mHelp = __LINE__;
            retval = __LINE__;
            ERROR_REPORT(retval, inArgv[loop], "Unknown command line switch.");
        }
        else if(NULL == current)
        {
            outOptions->mHelp = __LINE__;
            retval = __LINE__;
            ERROR_REPORT(retval, inArgv[loop], "Command line switch requires a value.");
        }
        else
        {
            


            if(current == &gInputSwitch)
            {
                CLEANUP(outOptions->mInputName);
                outOptions->mInputName = strdup(current->mValue);
                if(NULL == outOptions->mInputName)
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to strdup.");
                }
            }
            else if(current == &gOutputSwitch)
            {
                CLEANUP(outOptions->mOutputName);
                if(NULL != outOptions->mOutput && stdout != outOptions->mOutput)
                {
                    fclose(outOptions->mOutput);
                    outOptions->mOutput = NULL;
                }

                outOptions->mOutput = fopen(current->mValue, "a");
                if(NULL == outOptions->mOutput)
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to open output file.");
                }
                else
                {
                    outOptions->mOutputName = strdup(current->mValue);
                    if(NULL == outOptions->mOutputName)
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mValue, "Unable to strdup.");
                    }
                }
            }
            else if(current == &gHelpSwitch)
            {
                outOptions->mHelp = __LINE__;
            }
            else if(current == &gAlignmentSwitch)
            {
                unsigned arg = 0;
                char* endScan = NULL;

                errno = 0;
                arg = strtoul(current->mValue, &endScan, 0);
                if(0 == errno && endScan != current->mValue)
                {
                    outOptions->mAlignment = arg;
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to convert to a number.");
                }
            }
            else if(current == &gOverheadSwitch)
            {
                unsigned arg = 0;
                char* endScan = NULL;

                errno = 0;
                arg = strtoul(current->mValue, &endScan, 0);
                if(0 == errno && endScan != current->mValue)
                {
                    outOptions->mOverhead = arg;
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to convert to a number.");
                }
            }
            else if(current == &gPageSizeSwitch)
            {
                unsigned arg = 0;
                char* endScan = NULL;

                errno = 0;
                arg = strtoul(current->mValue, &endScan, 0);
                if(0 == errno && endScan != current->mValue)
                {
                    outOptions->mPageSize = arg;
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to convert to a number.");
                }
            }
            else
            {
                retval = __LINE__;
                ERROR_REPORT(retval, current->mLongName, "No handler for command line switch.");
            }
        }
    }

    return retval;
}


PRUint32 ticks2xsec(tmreader* aReader, PRUint32 aTicks, PRUint32 aResolution)



{
    PRUint32 retval = 0;
    PRUint64 bigone;
    PRUint64 tmp64;

    LL_UI2L(bigone, aResolution);
    LL_UI2L(tmp64, aTicks);
    LL_MUL(bigone, bigone, tmp64);
    LL_UI2L(tmp64, aReader->ticksPerSec);
    LL_DIV(bigone, bigone, tmp64);
    LL_L2UI(retval, bigone);
    return retval;
}


void cleanOptions(Options* inOptions)



{
    unsigned loop = 0;

    CLEANUP(inOptions->mInputName);
    CLEANUP(inOptions->mOutputName);
    if(NULL != inOptions->mOutput && stdout != inOptions->mOutput)
    {
        fclose(inOptions->mOutput);
    }

    memset(inOptions, 0, sizeof(Options));
}


void showHelp(Options* inOptions)



{
    int loop = 0;
    const int switchCount = sizeof(gSwitches) / sizeof(gSwitches[0]);
    const char* valueText = NULL;

    printf("usage:\t%s [arguments]\n", inOptions->mProgramName);
    printf("\n");
    printf("arguments:\n");

    for(loop = 0; loop < switchCount; loop++)
    {
        if(gSwitches[loop]->mHasValue)
        {
            valueText = " <value>";
        }
        else
        {
            valueText = "";
        }

        printf("\t%s%s\n", gSwitches[loop]->mLongName, valueText);
        printf("\t %s%s", gSwitches[loop]->mShortName, valueText);
        printf(DESC_NEWLINE "%s\n\n", gSwitches[loop]->mDescription);
    }

    printf("This tool reports heap fragmentation stats from a trace-malloc log.\n");
}


AnyArray* arrayCreate(unsigned inItemSize, unsigned inGrowBy)



{
    AnyArray* retval = NULL;

    if(0 != inGrowBy && 0 != inItemSize)
    {
        retval = (AnyArray*)calloc(1, sizeof(AnyArray));
        retval->mItemSize = inItemSize;
        retval->mGrowBy = inGrowBy;
    }

    return retval;
}


void arrayDestroy(AnyArray* inArray)




{
    if(NULL != inArray)
    {
        if(NULL != inArray->mItems)
        {
            free(inArray->mItems);
        }
        free(inArray);
    }
}


unsigned arrayAlloc(AnyArray* inArray, unsigned inItems)






{
    unsigned retval = (unsigned)-1;

    if(NULL != inArray)
    {
        void* moved = NULL;

        moved = realloc(inArray->mItems, inItems * inArray->mItemSize);
        if(NULL != moved)
        {
            inArray->mItems = moved;
            inArray->mCapacity = inItems;
            if(inArray->mCount > inItems)
            {
                inArray->mCount = inItems;
            }

            retval = inItems;
        }
    }

    return retval;
}


void* arrayItem(AnyArray* inArray, unsigned inIndex)






{
    void* retval = NULL;

    if(NULL != inArray && inIndex < inArray->mCount)
    {
        retval = (void*)((char*)inArray->mItems + (inArray->mItemSize * inIndex));
    }

    return retval;
}


unsigned arrayIndex(AnyArray* inArray, void* inItem, unsigned inStartIndex)








{
    unsigned retval = (unsigned)-1;

    if(NULL != inArray && NULL != inItem && inStartIndex < inArray->mCount)
    {
        void* curItem = NULL;

        for(retval = inStartIndex; retval < inArray->mCount; retval++)
        {
            curItem = arrayItem(inArray, retval);
            if(0 == memcmp(inItem, curItem, inArray->mItemSize))
            {
                break;
            }
        }
    }


    return retval;
}


unsigned arrayIndexFn(AnyArray* inArray, arrayMatchFunc inFunc, void* inFuncContext, unsigned inStartIndex)








{
    unsigned retval = (unsigned)-1;

    if(NULL != inArray && NULL != inFunc && inStartIndex < inArray->mCount)
    {
        void* curItem = NULL;

        for(retval = inStartIndex; retval < inArray->mCount; retval++)
        {
            curItem = arrayItem(inArray, retval);
            if(0 == inFunc(inFuncContext, inArray, curItem, retval))
            {
                break;
            }
        }
    }

    return retval;
}


unsigned arrayAddItem(AnyArray* inArray, void* inItem)






{
    unsigned retval = (unsigned)-1;

    if(NULL != inArray && NULL != inItem)
    {
        int noCopy = 0;

        


        if(inArray->mCount == inArray->mCapacity)
        {
            unsigned allocRes = 0;

            allocRes = arrayAlloc(inArray, inArray->mCapacity + inArray->mGrowBy);
            if(allocRes > inArray->mCapacity)
            {
                noCopy = __LINE__;
            }
        }

        if(0 == noCopy)
        {
            retval = inArray->mCount;

            inArray->mCount++;
            memcpy(arrayItem(inArray, retval), inItem, inArray->mItemSize);
        }
    }

    return retval;
}


HeapObject* initHeapObject(HeapObject* inObject)




{
    HeapObject* retval = inObject;

    if(NULL != inObject)
    {
        static unsigned uniqueGenerator = 0;

        memset(inObject, -1, sizeof(HeapObject));

        inObject->mUniqueID = uniqueGenerator;
        uniqueGenerator++;
    }

    return retval;
}


int simpleHeapEvent(TMState* inStats, HeapEventType inType, unsigned mTimestamp, unsigned inSerial, unsigned inHeapID, unsigned inSize)




{
    int retval = 0;
    HeapObject newObject;

    


    initHeapObject(&newObject);
    newObject.mHeapOffset = inHeapID;
    newObject.mSize = inSize;
    if(FREE == inType)
    {
        newObject.mType = FRAGMENT;
    }
    else if(ALLOC == inType)
    {
        newObject.mType = ALLOCATION;
    }

    



    






















    return retval;
}


int complexHeapEvent(TMState* inStats, unsigned mTimestamp, unsigned inOldSerial, unsigned inOldHeapID, unsigned inOSize, unsigned inNewSerial, unsigned inNewHeapID, unsigned inNewSize)




{
    int retval = 0;

    



    return retval;
}


unsigned actualByteSize(Options* inOptions, unsigned retval)




{
    if(0 != retval)
    {
        unsigned eval = 0;
        unsigned over = 0;

        eval = retval - 1;
        if(0 != inOptions->mAlignment)
        {
            over = eval % inOptions->mAlignment;
        }
        retval = eval + inOptions->mOverhead + inOptions->mAlignment - over;
    }

    return retval;
}


void tmEventHandler(tmreader* inReader, tmevent* inEvent)




{
    char type = inEvent->type;
    TMState* stats = (TMState*)inReader->data;

    


    switch(type)
    {
    default:
        return;

    case TM_EVENT_MALLOC:
    case TM_EVENT_CALLOC:
    case TM_EVENT_REALLOC:
    case TM_EVENT_FREE:
        break;
    }

    



    if(0 == stats->mLoopExitTMR)
    {
        Options* options = (Options*)stats->mOptions;
        unsigned timestamp = ticks2msec(stats->mTMR, inEvent->u.alloc.interval);
        unsigned actualSize = actualByteSize(options, inEvent->u.alloc.size);
        unsigned heapID = inEvent->u.alloc.ptr;
        unsigned serial = inEvent->serial;
        
        


        if(stats->uMinTicks > timestamp)
        {
            stats->uMinTicks = timestamp;
        }
        if(stats->uMaxTicks < timestamp)
        {
            stats->uMaxTicks = timestamp;
        }
        
        



        if(TM_EVENT_REALLOC == type && 0 != inEvent->u.alloc.oldserial)
        {
            unsigned oldActualSize = actualByteSize(options, inEvent->u.alloc.oldsize);
            unsigned oldHeapID = inEvent->u.alloc.oldptr;
            unsigned oldSerial = inEvent->u.alloc.oldserial;

            if(0 == actualSize)
            {
                


                stats->mLoopExitTMR = simpleHeapEvent(stats, FREE, timestamp, serial, oldHeapID, oldActualSize);
            }
            else if(heapID != oldHeapID || actualSize != oldActualSize)
            {
                








                stats->mLoopExitTMR = complexHeapEvent(stats, timestamp, oldSerial, oldHeapID, oldActualSize, serial, heapID, actualSize);
            }
            else
            {
                








            }
        }
        else if(TM_EVENT_FREE == type)
        {
            


            stats->mLoopExitTMR = simpleHeapEvent(stats, FREE, timestamp, serial, heapID, actualSize);
        }
        else
        {
            


            stats->mLoopExitTMR = simpleHeapEvent(stats, ALLOC, timestamp, serial, heapID, actualSize);
        }
    }
}


int tmfrags(Options* inOptions)



{
    int retval = 0;
    TMState stats;

    memset(&stats, 0, sizeof(stats));
    stats.mOptions = inOptions;
    stats.uMinTicks = 0xFFFFFFFFU;

    


    stats.mTMR = tmreader_new(inOptions->mProgramName, &stats);
    if(NULL != stats.mTMR)
    {
        int tmResult = 0;

        tmResult = tmreader_eventloop(stats.mTMR, inOptions->mInputName, tmEventHandler);
        if(0 == tmResult)
        {
            retval = __LINE__;
            ERROR_REPORT(retval, inOptions->mInputName, "Problem reading trace-malloc data.");
        }
        if(0 != stats.mLoopExitTMR)
        {
            retval = stats.mLoopExitTMR;
            ERROR_REPORT(retval, inOptions->mInputName, "Aborted trace-malloc input loop.");
        }

        tmreader_destroy(stats.mTMR);
        stats.mTMR = NULL;
    }
    else
    {
        retval = __LINE__;
        ERROR_REPORT(retval, inOptions->mProgramName, "Unable to obtain tmreader.");
    }

    return retval;
}


int main(int inArgc, char** inArgv)
{
    int retval = 0;
    Options options;

    retval = initOptions(&options, inArgc, inArgv);
    if(options.mHelp)
    {
        showHelp(&options);
    }
    else if(0 == retval)
    {
        retval = tmfrags(&options);
    }

    cleanOptions(&options);
    return retval;
}

