







































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#define ERROR_REPORT(num, val, msg)   fprintf(stderr, "error(%d):\t\"%s\"\t%s\n", (num), (val), (msg));
#define CLEANUP(ptr)    do { if(NULL != ptr) { free(ptr); ptr = NULL; } } while(0)


typedef struct __struct_Options






















{
    const char* mProgramName;
    FILE* mInput;
    char* mInputName;
    FILE* mOutput;
    char* mOutputName;
    int mHelp;
    int mModules;
    int mTotalOnly;
    unsigned long mMinSize;
    unsigned long mMaxSize;
    char** mMatchScopes;
    unsigned mMatchScopeCount;
    char** mMatchClasses;
    unsigned mMatchClassCount;
    char** mMatchModules;
    unsigned mMatchModuleCount;
    char** mMatchSections;
    unsigned mMatchSectionCount;
    char** mMatchObjects;
    unsigned mMatchObjectCount;
    char** mMatchSymbols;
    unsigned mMatchSymbolCount;
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
static Switch gModuleSwitch = {"--modules", "-m", 0, NULL, "Output individual module numbers as well."};
static Switch gTotalSwitch = {"--totalonly", "-t", 0, NULL, "Output only one number." DESC_NEWLINE "The total overall size." DESC_NEWLINE "Overrides other output options."};
static Switch gMinSize = {"--min-size", "-min", 1, NULL, "Only consider symbols equal to or greater than this size." DESC_NEWLINE "The default is 0x00000000."};
static Switch gMaxSize = {"--max-size", "-max", 1, NULL, "Only consider symbols equal to or smaller than this size." DESC_NEWLINE "The default is 0xFFFFFFFF."};
static Switch gMatchScope = {"--match-scope", "-msco", 1, NULL, "Only consider scopes that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify a range of scopes," DESC_NEWLINE "though PUBLIC, STATIC, and UNDEF are your only choices."};
static Switch gMatchClass = {"--match-class", "-mcla", 1, NULL, "Only consider classes that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify a range of classes," DESC_NEWLINE "though CODE and DATA are your only choices."};
static Switch gMatchModule = {"--match-module", "-mmod", 1, NULL, "Only consider modules that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify an array of modules."};
static Switch gMatchSection = {"--match-section", "-msec", 1, NULL, "Only consider sections that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify an array of sections."  DESC_NEWLINE "Section is considered symbol type."};
static Switch gMatchObject = {"--match-object", "-mobj", 1, NULL, "Only consider objects that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify an array of objects."};
static Switch gMatchSymbol = {"--match-symbol", "-msym", 1, NULL, "Only consider symbols that have a substring match." DESC_NEWLINE "Multiple uses allowed to specify an array of symbols."};

static Switch* gSwitches[] = {
        &gInputSwitch,
        &gOutputSwitch,
        &gModuleSwitch,
        &gTotalSwitch,
        &gMinSize,
        &gMaxSize,
        &gMatchClass,
        &gMatchScope,
        &gMatchModule,
        &gMatchSection,
        &gMatchObject,
        &gMatchSymbol,
        &gHelpSwitch
};


typedef struct __struct_SizeStats






{
    unsigned long mData;
    unsigned long mCode;
}
SizeStats;


typedef struct __struct_ModuleStats






{
    char* mModule;
    SizeStats mSize;
}
ModuleStats;

typedef enum __enum_SegmentClass
{
        CODE,
        DATA
}
SegmentClass;


static int moduleCompare(const void* in1, const void* in2)



{
    int retval = 0;

    const ModuleStats* one = (const ModuleStats*)in1;
    const ModuleStats* two = (const ModuleStats*)in2;
    unsigned long oneSize = one->mSize.mCode + one->mSize.mData;
    unsigned long twoSize = two->mSize.mCode + two->mSize.mData;

    if(oneSize < twoSize)
    {
        retval = 1;
    }
    else if(oneSize > twoSize)
    {
        retval = -1;
    }

    return retval;
}


void trimWhite(char* inString)



{
    int len = strlen(inString);

    while(len)
    {
        len--;

        if(isspace(*(inString + len)))
        {
            *(inString + len) = '\0';
        }
        else
        {
            break;
        }
    }
}


int codesighs(Options* inOptions)



{
    int retval = 0;
    char lineBuffer[0x1000];
    int scanRes = 0;
    unsigned long size;
    char segClass[0x10];
    char scope[0x10];
    char module[0x100];
    char segment[0x40];
    char object[0x100];
    char* symbol;
    SizeStats overall;
    ModuleStats* modules = NULL;
    unsigned moduleCount = 0;

    memset(&overall, 0, sizeof(overall));

    




    while(0 == retval && NULL != fgets(lineBuffer, sizeof(lineBuffer), inOptions->mInput))
    {
        trimWhite(lineBuffer);

        scanRes = sscanf(lineBuffer,
            "%x\t%s\t%s\t%s\t%s\t%s\t",
            (unsigned*)&size,
            segClass,
            scope,
            module,
            segment,
            object);

        if(6 == scanRes)
        {
            SegmentClass segmentClass = CODE;

            symbol = strchr(lineBuffer, '\t') + 1;

            


            if(0 == strcmp(segClass, "DATA"))
            {
                segmentClass = DATA;
            }
            else if(0 == strcmp(segClass, "CODE"))
            {
                segmentClass = CODE;
            }
            else
            {
                retval = __LINE__;
                ERROR_REPORT(retval, segClass, "Unable to determine segment class.");
            }

            if(0 == retval)
            {
                




                


                if(size < inOptions->mMinSize)
                {
                    continue;
                }
                if(size > inOptions->mMaxSize)
                {
                    continue;
                }

                


                if(0 != inOptions->mMatchClassCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchClassCount; loop++)
                    {
                        if(NULL != strstr(segClass, inOptions->mMatchClasses[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchClassCount)
                    {
                        continue;
                    }
                }

                


                if(0 != inOptions->mMatchScopeCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchScopeCount; loop++)
                    {
                        if(NULL != strstr(scope, inOptions->mMatchScopes[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchScopeCount)
                    {
                        continue;
                    }
                }

                


                if(0 != inOptions->mMatchModuleCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchModuleCount; loop++)
                    {
                        if(NULL != strstr(module, inOptions->mMatchModules[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchModuleCount)
                    {
                        continue;
                    }
                }

                


                if(0 != inOptions->mMatchSectionCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchSectionCount; loop++)
                    {
                        if(NULL != strstr(segment, inOptions->mMatchSections[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchSectionCount)
                    {
                        continue;
                    }
                }

                


                if(0 != inOptions->mMatchObjectCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchObjectCount; loop++)
                    {
                        if(NULL != strstr(object, inOptions->mMatchObjects[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchObjectCount)
                    {
                        continue;
                    }
                }

                


                if(0 != inOptions->mMatchSymbolCount)
                {
                    unsigned loop = 0;

                    for(loop = 0; loop < inOptions->mMatchSymbolCount; loop++)
                    {
                        if(NULL != strstr(symbol, inOptions->mMatchSymbols[loop]))
                        {
                            break;
                        }
                    }

                    


                    if(loop == inOptions->mMatchSymbolCount)
                    {
                        continue;
                    }
                }

                


                if(CODE == segmentClass)
                {
                    overall.mCode += size;
                }
                else if(DATA == segmentClass)
                {
                    overall.mData += size;
                }

                


                if(0 == inOptions->mTotalOnly)
                {
                    if(inOptions->mModules)
                    {
                        unsigned index = 0;
                        
                        


                        for(index = 0; index < moduleCount; index++)
                        {
                            if(0 == strcmp(modules[index].mModule, module))
                            {
                                break;
                            }
                        }
                        
                        



                        if(index == moduleCount)
                        {
                            void* moved = NULL;
                            
                            moved = realloc(modules, sizeof(ModuleStats) * (moduleCount + 1));
                            if(NULL != moved)
                            {
                                modules = (ModuleStats*)moved;
                                moduleCount++;
                                
                                memset(modules + index, 0, sizeof(ModuleStats));
                                modules[index].mModule = strdup(module);
                                if(NULL == modules[index].mModule)
                                {
                                    retval = __LINE__;
                                    ERROR_REPORT(retval, module, "Unable to duplicate string.");
                                }
                            }
                            else
                            {
                                retval = __LINE__;
                                ERROR_REPORT(retval, inOptions->mProgramName, "Unable to allocate module memory.");
                            }
                        }
                        
                        if(0 == retval)
                        {
                            if(CODE == segmentClass)
                            {
                                modules[index].mSize.mCode += size;
                            }
                            else if(DATA == segmentClass)
                            {
                                modules[index].mSize.mData += size;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            retval = __LINE__;
            ERROR_REPORT(retval, inOptions->mInputName, "Problem extracting values from file.");
        }
    }

    if(0 == retval && 0 != ferror(inOptions->mInput))
    {
        retval = __LINE__;
        ERROR_REPORT(retval, inOptions->mInputName, "Unable to read file.");
    }

    


    if(0 == retval)
    {
        if(inOptions->mTotalOnly)
        {
            fprintf(inOptions->mOutput, "%u\n", (unsigned)(overall.mCode + overall.mData));
        }
        else
        {
            fprintf(inOptions->mOutput, "Overall Size\n");
            fprintf(inOptions->mOutput, "\tTotal:\t%10u\n", (unsigned)(overall.mCode + overall.mData));
            fprintf(inOptions->mOutput, "\tCode:\t%10u\n", (unsigned)overall.mCode);
            fprintf(inOptions->mOutput, "\tData:\t%10u\n", (unsigned)overall.mData);
        }

        


        if(inOptions->mModules && moduleCount)
        {
            unsigned loop = 0;

            


            qsort(modules, (size_t)moduleCount, sizeof(ModuleStats), moduleCompare);

            



            for(loop = 0; loop < moduleCount; loop++)
            {
                fprintf(inOptions->mOutput, "\n");
                fprintf(inOptions->mOutput, "%s\n", modules[loop].mModule);
                fprintf(inOptions->mOutput, "\tTotal:\t%10u\n", (unsigned)(modules[loop].mSize.mCode + modules[loop].mSize.mData));
                fprintf(inOptions->mOutput, "\tCode:\t%10u\n", (unsigned)modules[loop].mSize.mCode);
                fprintf(inOptions->mOutput, "\tData:\t%10u\n", (unsigned)modules[loop].mSize.mData);

                CLEANUP(modules[loop].mModule);
            }

            


            CLEANUP(modules);
            moduleCount = 0;
        }
    }

    return retval;
}


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
    outOptions->mInput = stdin;
    outOptions->mInputName = strdup("stdin");
    outOptions->mOutput = stdout;
    outOptions->mOutputName = strdup("stdout");
    outOptions->mMaxSize = 0xFFFFFFFFU;

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
                if(NULL != outOptions->mInput && stdin != outOptions->mInput)
                {
                    fclose(outOptions->mInput);
                    outOptions->mInput = NULL;
                }

                outOptions->mInput = fopen(current->mValue, "r");
                if(NULL == outOptions->mInput)
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to open input file.");
                }
                else
                {
                    outOptions->mInputName = strdup(current->mValue);
                    if(NULL == outOptions->mInputName)
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mValue, "Unable to strdup.");
                    }
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
            else if(current == &gModuleSwitch)
            {
                outOptions->mModules = __LINE__;
            }
            else if(current == &gTotalSwitch)
            {
                outOptions->mTotalOnly = __LINE__;
            }
            else if(current == &gMinSize)
            {
                unsigned long arg = 0;
                char* endScan = NULL;

                errno = 0;
                arg = strtoul(current->mValue, &endScan, 0);
                if(0 == errno && endScan != current->mValue)
                {
                    outOptions->mMinSize = arg;
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to convert to a number.");
                }
            }
            else if(current == &gMaxSize)
            {
                unsigned long arg = 0;
                char* endScan = NULL;

                errno = 0;
                arg = strtoul(current->mValue, &endScan, 0);
                if(0 == errno && endScan != current->mValue)
                {
                    outOptions->mMaxSize = arg;
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to convert to a number.");
                }
            }
            else if(current == &gMatchClass)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchClasses, sizeof(char*) * (outOptions->mMatchClassCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchClasses = (char**)moved;
                        outOptions->mMatchClasses[outOptions->mMatchClassCount] = dupMatch;
                        outOptions->mMatchClassCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
                }
            }
            else if(current == &gMatchScope)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchScopes, sizeof(char*) * (outOptions->mMatchScopeCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchScopes = (char**)moved;
                        outOptions->mMatchScopes[outOptions->mMatchScopeCount] = dupMatch;
                        outOptions->mMatchScopeCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
                }
            }
            else if(current == &gMatchModule)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchModules, sizeof(char*) * (outOptions->mMatchModuleCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchModules = (char**)moved;
                        outOptions->mMatchModules[outOptions->mMatchModuleCount] = dupMatch;
                        outOptions->mMatchModuleCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
                }
            }
            else if(current == &gMatchSection)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchSections, sizeof(char*) * (outOptions->mMatchSectionCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchSections = (char**)moved;
                        outOptions->mMatchSections[outOptions->mMatchSectionCount] = dupMatch;
                        outOptions->mMatchSectionCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
                }
            }
            else if(current == &gMatchObject)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchObjects, sizeof(char*) * (outOptions->mMatchObjectCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchObjects = (char**)moved;
                        outOptions->mMatchObjects[outOptions->mMatchObjectCount] = dupMatch;
                        outOptions->mMatchObjectCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
                }
            }
            else if(current == &gMatchSymbol)
            {
                char* dupMatch = NULL;
                
                dupMatch = strdup(current->mValue);
                if(NULL != dupMatch)
                {
                    void* moved = NULL;

                    moved = realloc(outOptions->mMatchSymbols, sizeof(char*) * (outOptions->mMatchSymbolCount + 1));
                    if(NULL != moved)
                    {
                        outOptions->mMatchSymbols = (char**)moved;
                        outOptions->mMatchSymbols[outOptions->mMatchSymbolCount] = dupMatch;
                        outOptions->mMatchSymbolCount++;
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, current->mLongName, "Unable to expand array.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, current->mValue, "Unable to duplicate string.");
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


void cleanOptions(Options* inOptions)



{
    unsigned loop = 0;

    CLEANUP(inOptions->mInputName);
    if(NULL != inOptions->mInput && stdin != inOptions->mInput)
    {
        fclose(inOptions->mInput);
    }
    CLEANUP(inOptions->mOutputName);
    if(NULL != inOptions->mOutput && stdout != inOptions->mOutput)
    {
        fclose(inOptions->mOutput);
    }

    for(loop = 0; loop < inOptions->mMatchClassCount; loop++)
    {
        CLEANUP(inOptions->mMatchClasses[loop]);
    }
    CLEANUP(inOptions->mMatchClasses);

    for(loop = 0; loop < inOptions->mMatchScopeCount; loop++)
    {
        CLEANUP(inOptions->mMatchScopes[loop]);
    }
    CLEANUP(inOptions->mMatchScopes);

    for(loop = 0; loop < inOptions->mMatchModuleCount; loop++)
    {
        CLEANUP(inOptions->mMatchModules[loop]);
    }
    CLEANUP(inOptions->mMatchModules);

    for(loop = 0; loop < inOptions->mMatchSectionCount; loop++)
    {
        CLEANUP(inOptions->mMatchSections[loop]);
    }
    CLEANUP(inOptions->mMatchSections);

    for(loop = 0; loop < inOptions->mMatchObjectCount; loop++)
    {
        CLEANUP(inOptions->mMatchObjects[loop]);
    }
    CLEANUP(inOptions->mMatchObjects);

    for(loop = 0; loop < inOptions->mMatchSymbolCount; loop++)
    {
        CLEANUP(inOptions->mMatchSymbols[loop]);
    }
    CLEANUP(inOptions->mMatchSymbols);

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

    printf("This tool takes a tsv file and reports composite code and data sizes.\n");
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
        retval = codesighs(&options);
    }

    cleanOptions(&options);
    return retval;
}

