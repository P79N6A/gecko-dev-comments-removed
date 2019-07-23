







































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


#define COST_RESOLUTION 1000
#define COST_PRINTABLE(cost) ((PRFloat64)(cost) / (PRFloat64)COST_RESOLUTION)


typedef struct __struct_Options















{
    const char* mProgramName;
    char* mInputName;
    FILE* mOutput;
    char* mOutputName;
    int mHelp;
    unsigned mOverhead;
    unsigned mAlignment;
    int mAverages;
    int mDeviances;
    int mRunLength;
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
static Switch gAveragesSwitch = {"--averages", "-avg", 0, NULL, "Display averages."};
static Switch gDeviationsSwitch = {"--deviations", "-dev", 0, NULL, "Display standard deviations from the average."  DESC_NEWLINE "Implies --averages."};
static Switch gRunLengthSwitch = {"--run-length", "-rl", 0, NULL, "Display the run length in seconds."};

static Switch* gSwitches[] = {
        &gInputSwitch,
        &gOutputSwitch,
        &gAlignmentSwitch,
        &gOverheadSwitch,
        &gAveragesSwitch,
        &gDeviationsSwitch,
        &gRunLengthSwitch,
        &gHelpSwitch
};


typedef struct _struct_VarianceState



{
    unsigned mCount;
    PRUint64 mSum;
    PRUint64 mSquaredSum;
}
VarianceState;


typedef struct __struct_TMStats































{
    Options* mOptions;
    unsigned uMemoryInUse;
    unsigned uPeakMemory;
    unsigned uObjectsInUse;
    unsigned uPeakObjects;
    unsigned uMallocs;
    unsigned uCallocs;
    unsigned uReallocs;
    unsigned uFrees;

    unsigned uMallocSize;
    unsigned uCallocSize;
    unsigned uReallocSize;
    unsigned uFreeSize;
    VarianceState mMallocSizeVar;
    VarianceState mCallocSizeVar;
    VarianceState mReallocSizeVar;
    VarianceState mFreeSizeVar;

    unsigned uMallocCost;
    unsigned uCallocCost;
    unsigned uReallocCost;
    unsigned uFreeCost;
    VarianceState mMallocCostVar;
    VarianceState mCallocCostVar;
    VarianceState mReallocCostVar;
    VarianceState mFreeCostVar;

    unsigned uMinTicks;
    unsigned uMaxTicks;
}
TMStats;


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
            else if(current == &gAveragesSwitch)
            {
                outOptions->mAverages = __LINE__;
            }
            else if(current == &gDeviationsSwitch)
            {
                outOptions->mAverages = __LINE__;
                outOptions->mDeviances = __LINE__;
            }
            else if(current == &gRunLengthSwitch)
            {
                outOptions->mRunLength = __LINE__;
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

    printf("This tool reports simple heap usage and allocation call counts.\n");
    printf("Useful for eyeballing trace-malloc numbers quickly.\n");
}


void addVariance(VarianceState* inVariance, unsigned inValue)



{
    PRUint64 squared;
    PRUint64 bigValue;
    
    LL_UI2L(bigValue, inValue);

    LL_ADD(inVariance->mSum, inVariance->mSum, bigValue);

    LL_MUL(squared, bigValue, bigValue);
    LL_ADD(inVariance->mSquaredSum, inVariance->mSquaredSum, squared);

    inVariance->mCount++;
}


PRFloat64 getAverage(VarianceState* inVariance)



{
    PRFloat64 retval = 0.0;

    if(NULL != inVariance && 0 < inVariance->mCount)
    {
        PRFloat64 count;
        PRFloat64 sum;
        PRInt64 isum;

        


        isum = inVariance->mSum;

        count = (PRFloat64)inVariance->mCount;
        LL_L2F(sum, isum);

        retval = sum / count;
    }

    return retval;
}


PRFloat64 getVariance(VarianceState* inVariance)



{
    PRFloat64 retval = 0.0;

    if(NULL != inVariance && 1 < inVariance->mCount)
    {
        PRFloat64 count;
        PRFloat64 squaredSum;
        PRFloat64 avg;
        PRFloat64 squaredAvg;
        PRInt64 isquaredSum;

        


        isquaredSum = inVariance->mSquaredSum;

        count = (PRFloat64)inVariance->mCount;
        LL_L2F(squaredSum, isquaredSum);

        avg = getAverage(inVariance);
        squaredAvg = avg * avg;

        retval = (squaredSum - (count * squaredAvg)) / (count - 1.0);
    }

    return retval;
}


PRFloat64 getStdDev(VarianceState* inVariance)



{
    PRFloat64 retval = 0.0;
    PRFloat64 variance;

    variance = getVariance(inVariance);
    retval = sqrt(variance);

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
#define ticks2msec(reader, ticks) ticks2xsec((reader), (ticks), 1000)


void tmEventHandler(tmreader* inReader, tmevent* inEvent)





{
    TMStats* stats = (TMStats*)inReader->data;
    Options* options = (Options*)stats->mOptions;
    char type = inEvent->type;
    unsigned size = inEvent->u.alloc.size;
    unsigned actualSize = 0;
    unsigned actualOldSize = 0;
    PRUint32 interval = 0;

    



    if(TM_EVENT_REALLOC == type && 0 == size)
    {
        type = TM_EVENT_FREE;
        if(0 != inEvent->u.alloc.oldserial)
        {
            size = inEvent->u.alloc.oldsize;
        }
    }

    


    actualSize = actualByteSize(options, size);
    if(TM_EVENT_REALLOC == type && 0 != inEvent->u.alloc.oldserial)
    {
        actualOldSize = actualByteSize(options, inEvent->u.alloc.oldsize);
    }

    


    switch(type)
    {
    case TM_EVENT_MALLOC:
        stats->uMallocs++;
        stats->uMallocSize += actualSize;
        stats->uMallocCost += ticks2msec(inReader, inEvent->u.alloc.cost);
        stats->uMemoryInUse += actualSize;
        stats->uObjectsInUse++;

        addVariance(&stats->mMallocSizeVar, actualSize);
        addVariance(&stats->mMallocCostVar,  inEvent->u.alloc.cost);
        break;

    case TM_EVENT_CALLOC:
        stats->uCallocs++;
        stats->uCallocSize += actualSize;
        stats->uCallocCost += ticks2msec(inReader, inEvent->u.alloc.cost);
        stats->uMemoryInUse += actualSize;
        stats->uObjectsInUse++;

        addVariance(&stats->mCallocSizeVar, actualSize);
        addVariance(&stats->mCallocCostVar,  inEvent->u.alloc.cost);
        break;

    case TM_EVENT_REALLOC:
        stats->uReallocs++;
        stats->uReallocSize -= actualOldSize;
        stats->uReallocSize += actualSize;
        stats->uReallocCost += ticks2msec(inReader, inEvent->u.alloc.cost);
        stats->uMemoryInUse -= actualOldSize;
        stats->uMemoryInUse += actualSize;
        if(0 == inEvent->u.alloc.oldserial)
        {
            stats->uObjectsInUse++;
        }

        if(actualSize > actualOldSize)
        {
            addVariance(&stats->mReallocSizeVar, actualSize - actualOldSize);
        }
        else
        {
            addVariance(&stats->mReallocSizeVar, actualOldSize - actualSize);
        }
        addVariance(&stats->mReallocCostVar,  inEvent->u.alloc.cost);
        break;

    case TM_EVENT_FREE:
        stats->uFrees++;
        stats->uFreeSize += actualSize;
        stats->uFreeCost += ticks2msec(inReader, inEvent->u.alloc.cost);
        stats->uMemoryInUse -= actualSize;
        stats->uObjectsInUse--;

        addVariance(&stats->mFreeSizeVar, actualSize);
        addVariance(&stats->mFreeCostVar,  inEvent->u.alloc.cost);
        break;

    default:
        


        break;
    }

    switch(type)
    {
    case TM_EVENT_MALLOC:
    case TM_EVENT_CALLOC:
    case TM_EVENT_REALLOC:
        


        if(stats->uMemoryInUse > stats->uPeakMemory)
        {
            stats->uPeakMemory = stats->uMemoryInUse;
        }
        if(stats->uObjectsInUse > stats->uPeakObjects)
        {
            stats->uPeakObjects = stats->uObjectsInUse;
        }

        



    case TM_EVENT_FREE:
        


        interval = ticks2msec(inReader, inEvent->u.alloc.interval);
        if(stats->uMinTicks > interval)
        {
            stats->uMinTicks = interval;
        }
        if(stats->uMaxTicks < interval)
        {
            stats->uMaxTicks = interval;
        }
        break;

    default:
        


        break;
    }

}

int report_stats(Options* inOptions, TMStats* inStats)
{
    int retval = 0;

    fprintf(inOptions->mOutput, "Peak Memory Usage:                   %11d\n", inStats->uPeakMemory);
    fprintf(inOptions->mOutput, "Memory Leaked:                       %11d\n", inStats->uMemoryInUse);
    fprintf(inOptions->mOutput, "\n");

    fprintf(inOptions->mOutput, "Peak Object Count:                   %11d\n", inStats->uPeakObjects);
    fprintf(inOptions->mOutput, "Objects Leaked:                      %11d\n", inStats->uObjectsInUse);
    if(0 != inOptions->mAverages && 0 != inStats->uObjectsInUse)
    {
        fprintf(inOptions->mOutput, "Average Leaked Object Size:          %11.4f\n", (PRFloat64)inStats->uMemoryInUse / (PRFloat64)inStats->uObjectsInUse);
    }
    fprintf(inOptions->mOutput, "\n");

    fprintf(inOptions->mOutput, "Call Total:                          %11d\n", inStats->uMallocs + inStats->uCallocs + inStats->uReallocs + inStats->uFrees);
    fprintf(inOptions->mOutput, "        malloc:                      %11d\n", inStats->uMallocs);
    fprintf(inOptions->mOutput, "        calloc:                      %11d\n", inStats->uCallocs);
    fprintf(inOptions->mOutput, "       realloc:                      %11d\n", inStats->uReallocs);
    fprintf(inOptions->mOutput, "          free:                      %11d\n", inStats->uFrees);
    fprintf(inOptions->mOutput, "\n");

    fprintf(inOptions->mOutput, "Byte Total (sans free):              %11d\n", inStats->uMallocSize + inStats->uCallocSize + inStats->uReallocSize);
    fprintf(inOptions->mOutput, "        malloc:                      %11d\n", inStats->uMallocSize);
    fprintf(inOptions->mOutput, "        calloc:                      %11d\n", inStats->uCallocSize);
    fprintf(inOptions->mOutput, "       realloc:                      %11d\n", inStats->uReallocSize);
    fprintf(inOptions->mOutput, "          free:                      %11d\n", inStats->uFreeSize);
    if(0 != inOptions->mAverages)
    {
        fprintf(inOptions->mOutput, "Byte Averages:\n");
        fprintf(inOptions->mOutput, "        malloc:                      %11.4f\n", getAverage(&inStats->mMallocSizeVar));
        fprintf(inOptions->mOutput, "        calloc:                      %11.4f\n", getAverage(&inStats->mCallocSizeVar));
        fprintf(inOptions->mOutput, "       realloc:                      %11.4f\n", getAverage(&inStats->mReallocSizeVar));
        fprintf(inOptions->mOutput, "          free:                      %11.4f\n", getAverage(&inStats->mFreeSizeVar));
    }
    if(0 != inOptions->mDeviances)
    {
        fprintf(inOptions->mOutput, "Byte Standard Deviations:\n");
        fprintf(inOptions->mOutput, "        malloc:                      %11.4f\n", getStdDev(&inStats->mMallocSizeVar));
        fprintf(inOptions->mOutput, "        calloc:                      %11.4f\n", getStdDev(&inStats->mCallocSizeVar));
        fprintf(inOptions->mOutput, "       realloc:                      %11.4f\n", getStdDev(&inStats->mReallocSizeVar));
        fprintf(inOptions->mOutput, "          free:                      %11.4f\n", getStdDev(&inStats->mFreeSizeVar));
    }
    fprintf(inOptions->mOutput, "\n");
    
    fprintf(inOptions->mOutput, "Overhead Total:                      %11.4f\n", COST_PRINTABLE(inStats->uMallocCost) + COST_PRINTABLE(inStats->uCallocCost) + COST_PRINTABLE(inStats->uReallocCost) + COST_PRINTABLE(inStats->uFreeCost));
    fprintf(inOptions->mOutput, "        malloc:                      %11.4f\n", COST_PRINTABLE(inStats->uMallocCost));
    fprintf(inOptions->mOutput, "        calloc:                      %11.4f\n", COST_PRINTABLE(inStats->uCallocCost));
    fprintf(inOptions->mOutput, "       realloc:                      %11.4f\n", COST_PRINTABLE(inStats->uReallocCost));
    fprintf(inOptions->mOutput, "          free:                      %11.4f\n", COST_PRINTABLE(inStats->uFreeCost));
    if(0 != inOptions->mAverages)
    {
        fprintf(inOptions->mOutput, "Overhead Averages:\n");
        fprintf(inOptions->mOutput, "        malloc:                      %11.4f\n", COST_PRINTABLE(getAverage(&inStats->mMallocCostVar)));
        fprintf(inOptions->mOutput, "        calloc:                      %11.4f\n", COST_PRINTABLE(getAverage(&inStats->mCallocCostVar)));
        fprintf(inOptions->mOutput, "       realloc:                      %11.4f\n", COST_PRINTABLE(getAverage(&inStats->mReallocCostVar)));
        fprintf(inOptions->mOutput, "          free:                      %11.4f\n", COST_PRINTABLE(getAverage(&inStats->mFreeCostVar)));
    }
    if(0 != inOptions->mDeviances)
    {
        fprintf(inOptions->mOutput, "Overhead Standard Deviations:\n");
        fprintf(inOptions->mOutput, "        malloc:                      %11.4f\n", COST_PRINTABLE(getStdDev(&inStats->mMallocCostVar)));
        fprintf(inOptions->mOutput, "        calloc:                      %11.4f\n", COST_PRINTABLE(getStdDev(&inStats->mCallocCostVar)));
        fprintf(inOptions->mOutput, "       realloc:                      %11.4f\n", COST_PRINTABLE(getStdDev(&inStats->mReallocCostVar)));
        fprintf(inOptions->mOutput, "          free:                      %11.4f\n", COST_PRINTABLE(getStdDev(&inStats->mFreeCostVar)));
    }
    fprintf(inOptions->mOutput, "\n");
    
    if(0 != inOptions->mRunLength)
    {
        unsigned length = inStats->uMaxTicks - inStats->uMinTicks;

        fprintf(inOptions->mOutput, "Run Length:                          %11.4f\n", COST_PRINTABLE(length));
        fprintf(inOptions->mOutput, "\n");
    }

    return retval;
}


int tmstats(Options* inOptions)



{
    int retval = 0;
    tmreader* tmr = NULL;
    TMStats stats;

    memset(&stats, 0, sizeof(stats));
    stats.mOptions = inOptions;
    stats.uMinTicks = 0xFFFFFFFFU;

    


    tmr = tmreader_new(inOptions->mProgramName, &stats);
    if(NULL != tmr)
    {
        int tmResult = 0;

        tmResult = tmreader_eventloop(tmr, inOptions->mInputName, tmEventHandler);
        if(0 == tmResult)
        {
            retval = __LINE__;
            ERROR_REPORT(retval, inOptions->mInputName, "Problem reading trace-malloc data.");
        }

        tmreader_destroy(tmr);
        tmr = NULL;

        if(0 == retval)
        {
            retval = report_stats(inOptions, &stats);
        }
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
        retval = tmstats(&options);
    }

    cleanOptions(&options);
    return retval;
}

