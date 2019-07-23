







































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

static Switch* gSwitches[] = {
        &gInputSwitch,
        &gOutputSwitch,
        &gHelpSwitch
};


typedef struct __struct_MSDump_Symbol







{
    unsigned    mSize;
    unsigned    mOffset;
    char*       mName;
}
MSDump_Symbol;


typedef struct __struct_MSDump_Section










{
    unsigned            mLength;
    unsigned            mUsed;
    char*               mType;

    MSDump_Symbol*      mSymbols;
    unsigned            mSymbolCount;
}
MSDump_Section;


typedef struct __struct_MSDump_Object



{
    char*   mObject;

    MSDump_Section*     mSections;
    unsigned            mSectionCount;
}
MSDump_Object;


typedef struct __struct_MSDump_ReadState







{
    unsigned            mSkipLines;
    unsigned            mSectionDetails;
    MSDump_Object*      mCurrentObject;
}
MSDump_ReadState;


typedef struct __struct_MSDump_Container



{
    MSDump_ReadState    mReadState;

    MSDump_Object*      mObjects;
    unsigned            mObjectCount;
}
MSDump_Container;


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


const char* skipWhite(const char* inString)



{
    const char* retval = inString;

    while('\0' != *retval && isspace(*retval))
    {
        retval++;
    }

    return retval;
}


const char* skipNonWhite(const char* inString)



{
    const char* retval = inString;

    while('\0' != *retval && !isspace(*retval))
    {
        retval++;
    }

    return retval;
}


void slash2bs(char* inString)



{
    char* slash = inString;

    while(NULL != (slash = strchr(slash, '/')))
    {
        *slash = '\\';
        slash++;
    }
}


const char* skipToArg(const char* inString, unsigned inArgIndex)




{
    const char* retval = NULL;

    while(0 != inArgIndex && '\0' != *inString)
    {
        inArgIndex--;

        inString = skipWhite(inString);
        if(0 != inArgIndex)
        {
            inString = skipNonWhite(inString);
        }
    }

    if('\0' != *inString)
    {
        retval = inString;
    }

    return retval;
}


const char* getLastArg(const char* inString)



{
    const char* retval = NULL;
    int length = 0;
    int sawString = 0;

    length = strlen(inString);
    while(0 != length)
    {
        length--;

        if(0 == sawString)
        {
            if(0 == isspace(inString[length]))
            {
                sawString = __LINE__;
            }
        }
        else
        {
            if(0 != isspace(inString[length]))
            {
                retval = inString + length + 1;
            }
        }
    }

    return retval;
}


int processLine(Options* inOptions, MSDump_Container* inContainer, const char* inLine)







{
    int retval = 0;

    


    if(0 != inContainer->mReadState.mSectionDetails)
    {
        const char* length = NULL;
        unsigned sectionIndex = 0;

        



        sectionIndex = inContainer->mReadState.mSectionDetails - 1;
        inContainer->mReadState.mSectionDetails = 0;

        if(0 == strncmp("    Section length", inLine, 18))
        {
            const char* sectionLength = NULL;
            unsigned numericLength = 0;
            char* endScan = NULL;

            sectionLength = skipWhite(inLine + 18);

            errno = 0;
            numericLength = strtoul(sectionLength, &endScan, 16);
            if(0 == errno && endScan != sectionLength)
            {
                inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mLength = numericLength;
            }
            else
            {
                retval = __LINE__;
                ERROR_REPORT(retval, inLine, "Cannot scan for section length.");
            }
        }
        else
        {
            retval = __LINE__;
            ERROR_REPORT(retval, inLine, "Cannot parse section line.");
        }
    }
    


    else if(0 == strncmp("Dump of file ", inLine, 13))
    {
        const char* dupMe = inLine + 13;
        char* dup = NULL;
        
        dup = strdup(dupMe);
        if(NULL != dup)
        {
            void* growth = NULL;
            
            trimWhite(dup);
            slash2bs(dup);
            
            
            growth = realloc(inContainer->mObjects, (inContainer->mObjectCount + 1) * sizeof(MSDump_Object));
            if(NULL != growth)
            {
                unsigned int index = inContainer->mObjectCount;
                
                inContainer->mObjectCount++;
                inContainer->mObjects = growth;
                memset(inContainer->mObjects + index, 0, sizeof(MSDump_Object));
                
                inContainer->mObjects[index].mObject = dup;

                


                memset(&inContainer->mReadState, 0, sizeof(MSDump_ReadState));

                


                inContainer->mReadState.mCurrentObject = inContainer->mObjects + index;

                


                inContainer->mReadState.mSkipLines = 4;
            }
            else
            {
                retval = __LINE__;
                ERROR_REPORT(retval, dup, "Unable to grow object array.");
                free(dup);
            }
        }
        else
        {
            retval = __LINE__;
            ERROR_REPORT(retval, dupMe, "Unable to copy string.");
            
        }
    }
    


    else if(isxdigit(*inLine) && isxdigit(*(inLine + 1)) && isxdigit(*(inLine + 2)))
    {
        const char* sectionString = NULL;

        



        sectionString = skipToArg(inLine, 3);
        if(NULL != sectionString)
        {
            if(0 != strncmp(sectionString, "DEBUG", 5) && 0 != strncmp(sectionString, "ABS", 3) && 0 != strncmp(sectionString, "UNDEF", 5))
            {
                


                if(0 == strncmp(sectionString, "SECT", 4))
                {
                    unsigned sectionIndex1 = 0;

                    char *endScan = NULL;

                    sectionString += 4;

                    



                    errno = 0;
                    sectionIndex1 = strtoul(sectionString, &endScan, 16);
                    if(0 == errno && endScan != sectionString && 0 != sectionIndex1)
                    {
                        unsigned sectionIndex = sectionIndex1 - 1;

                        



                        if(sectionIndex1 > inContainer->mReadState.mCurrentObject->mSectionCount)
                        {
                            const char* typeArg = NULL;

                            


                            typeArg = skipToArg(sectionString, 5);
                            if(NULL != typeArg)
                            {
                                char* typeDup = NULL;

                                


                                if('.' == *typeArg)
                                {
                                    typeArg++;
                                }
                                typeDup = strdup(typeArg);

                                if(NULL != typeDup)
                                {
                                    void* moved = NULL;
                                    char* nonWhite = NULL;

                                    


                                    nonWhite = (char*)skipNonWhite(typeDup);
                                    if(NULL != nonWhite)
                                    {
                                        *nonWhite = '\0';
                                    }

                                    


                                    moved = realloc(inContainer->mReadState.mCurrentObject->mSections, sizeof(MSDump_Section) * sectionIndex1);
                                    if(NULL != moved)
                                    {
                                        unsigned oldCount = inContainer->mReadState.mCurrentObject->mSectionCount;

                                        inContainer->mReadState.mCurrentObject->mSections = (MSDump_Section*)moved;
                                        inContainer->mReadState.mCurrentObject->mSectionCount = sectionIndex1;
                                        memset(&inContainer->mReadState.mCurrentObject->mSections[oldCount], 0, sizeof(MSDump_Section) * (sectionIndex1 - oldCount));
                                        
                                        


                                        inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mType = typeDup;
                                            
                                            
                                        



                                        inContainer->mReadState.mSectionDetails = sectionIndex1;
                                    }
                                    else
                                    {
                                        retval = __LINE__;
                                        ERROR_REPORT(retval, inLine, "Unable to grow for new section.");
                                        free(typeDup);
                                    }
                                }
                                else
                                {
                                    retval = __LINE__;
                                    ERROR_REPORT(retval, typeArg, "Unable to duplicate type.");
                                }
                            }
                            else
                            {
                                retval = __LINE__;
                                ERROR_REPORT(retval, inLine, "Unable to determine section type.");
                            }

                        }
                        else
                        {
                            const char* offsetArg = NULL;
                            const char* classArg = NULL;
                            unsigned classWords = 1;
                            const char* symbolArg = NULL;

                            





                            offsetArg = skipToArg(inLine, 2);

                            classArg = skipToArg(offsetArg, 4);
                            if(0 == strncmp(classArg, "()", 2))
                            {
                                classArg = skipToArg(classArg, 2);
                            }
                            if(0 == strncmp(classArg, ".bf or.ef", 9))
                            {
                                classWords = 2;
                            }

                            symbolArg = skipToArg(classArg, 3 + (classWords - 1));

                            


                            if(
                                0 != strncmp(classArg, "Label", 5) &&
                                0 != strncmp(symbolArg, ".bf", 3) &&
                                0 != strncmp(symbolArg, ".lf", 3) &&
                                0 != strncmp(symbolArg, ".ef", 3)
                                )
                            {
                                char* endOffsetArg = NULL;
                                unsigned offset = 0;
                                
                                


                                errno = 0;
                                offset = strtoul(offsetArg, &endOffsetArg, 16);
                                if(0 == errno && endOffsetArg != offsetArg)
                                {
                                    void* moved = NULL;
                                    
                                    



                                    moved = realloc(inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols, sizeof(MSDump_Symbol) * (inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbolCount + 1));
                                    if(NULL != moved)
                                    {
                                        unsigned symIndex = 0;

                                        



                                        symIndex = inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbolCount;
                                        inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbolCount++;
                                        inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols = (MSDump_Symbol*)moved;
                                        memset(&inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols[symIndex], 0, sizeof(MSDump_Symbol));
                                        
                                        inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols[symIndex].mOffset = offset;
                                        
                                        


                                        inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols[symIndex].mName = strdup(symbolArg);
                                        if(NULL != inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols[symIndex].mName)
                                        {
                                            char* trim = NULL;

                                            trim = (char*)skipNonWhite(inContainer->mReadState.mCurrentObject->mSections[sectionIndex].mSymbols[symIndex].mName);
                                            if(NULL != trim)
                                            {
                                                *trim = '\0';
                                            }
                                        }
                                        else
                                        {
                                            retval = __LINE__;
                                            ERROR_REPORT(retval, inLine, "Unable to duplicate symbol name.");
                                        }
                                    }
                                    else
                                    {
                                        retval = __LINE__;
                                        ERROR_REPORT(retval, inLine, "Unable to grow symbol array for section.");
                                    }
                                }
                                else
                                {
                                    retval = __LINE__;
                                    ERROR_REPORT(retval, inLine, "Unable to convert offset to a number.");
                                }
                            }
                        }
                    }
                    else
                    {
                        retval = __LINE__;
                        ERROR_REPORT(retval, inLine, "Unable to determine section index.");
                    }
                }
                else
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, inLine, "No match for section prefix.");
                }
            }
        }
        else
        {
            retval = __LINE__;
            ERROR_REPORT(retval, inLine, "Unable to scan for section.");
        }
    }

    return retval;
}


void dumpCleanup(MSDump_Container* inContainer)



{
    unsigned objectLoop = 0;
    unsigned sectionLoop = 0;
    unsigned symbolLoop = 0;

    for(objectLoop = 0; objectLoop < inContainer->mObjectCount; objectLoop++)
    {
        for(sectionLoop = 0; sectionLoop < inContainer->mObjects[objectLoop].mSectionCount; sectionLoop++)
        {
            for(symbolLoop = 0; symbolLoop < inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbolCount; symbolLoop++)
            {
                CLEANUP(inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mName);
            }
            inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbolCount = 0;
            CLEANUP(inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols);
            CLEANUP(inContainer->mObjects[objectLoop].mSections[sectionLoop].mType);
        }
        inContainer->mObjects[objectLoop].mSectionCount = 0;
        CLEANUP(inContainer->mObjects[objectLoop].mSections);
    }
    CLEANUP(inContainer->mObjects);
    inContainer->mObjectCount = 0;
}


int qsortSymOffset(const void* in1, const void* in2)



{
    MSDump_Symbol* sym1 = (MSDump_Symbol*)in1;
    MSDump_Symbol* sym2 = (MSDump_Symbol*)in2;
    int retval = 0;

    if(sym1->mOffset < sym2->mOffset)
    {
        retval = 1;
    }
    else if(sym1->mOffset > sym2->mOffset)
    {
        retval = -1;
    }

    return retval;
}


int calcContainer(Options* inOptions, MSDump_Container* inContainer)





{
    int retval = 0;
    unsigned objectLoop = 0;
    unsigned sectionLoop = 0;
    unsigned symbolLoop = 0;


    


    for(objectLoop = 0; 0 == retval && objectLoop < inContainer->mObjectCount; objectLoop++)
    {
        for(sectionLoop = 0; 0 == retval && sectionLoop < inContainer->mObjects[objectLoop].mSectionCount; sectionLoop++)
        {
            qsort(
                inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols,
                inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbolCount,
                sizeof(MSDump_Symbol),
                qsortSymOffset
                );
        }
    }


    


    for(objectLoop = 0; 0 == retval && objectLoop < inContainer->mObjectCount; objectLoop++)
    {
        for(sectionLoop = 0; 0 == retval && sectionLoop < inContainer->mObjects[objectLoop].mSectionCount; sectionLoop++)
        {
            for(symbolLoop = 0; 0 == retval && symbolLoop < inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbolCount; symbolLoop++)
            {
                inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mSize =
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mLength -
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mUsed -
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mOffset;

                inContainer->mObjects[objectLoop].mSections[sectionLoop].mUsed += 
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mSize;
            }
        }
    }


    return retval;
}


int reportContainer(Options* inOptions, MSDump_Container* inContainer)




{
    int retval = 0;
    unsigned objectLoop = 0;
    unsigned sectionLoop = 0;
    unsigned symbolLoop = 0;
    int printRes = 0;

    for(objectLoop = 0; 0 == retval && objectLoop < inContainer->mObjectCount; objectLoop++)
    {
        for(sectionLoop = 0; 0 == retval && sectionLoop < inContainer->mObjects[objectLoop].mSectionCount; sectionLoop++)
        {
            for(symbolLoop = 0; 0 == retval && symbolLoop < inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbolCount; symbolLoop++)
            {
                printRes = fprintf(inOptions->mOutput, "%s\t%s\t%.8X\t%s\n",
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mName,
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mType,
                    inContainer->mObjects[objectLoop].mSections[sectionLoop].mSymbols[symbolLoop].mSize,
                    inContainer->mObjects[objectLoop].mObject
                    );

                if(0 > printRes)
                {
                    retval = __LINE__;
                    ERROR_REPORT(retval, inOptions->mOutputName, "Unable to write to file.");
                }
            }
        }
    }

    return retval;
}


int dump2symdb(Options* inOptions)




{
    int retval = 0;
    char lineBuffer[0x800];
    MSDump_Container container;

    memset(&container, 0, sizeof(container));

    


    while(0 == retval && NULL != fgets(lineBuffer, sizeof(lineBuffer), inOptions->mInput))
    {
        if(0 != container.mReadState.mSkipLines)
        {
            container.mReadState.mSkipLines--;
            continue;
        }
        retval = processLine(inOptions, &container, lineBuffer);
    }

    


    if(0 == retval)
    {
        retval = calcContainer(inOptions, &container);
    }

    


    if(0 == retval)
    {
        retval = reportContainer(inOptions, &container);
    }

    


    dumpCleanup(&container);

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

    printf("This tool takes the output of \"dumpbin /symbols\" to produce a simple\n");
    printf("tsv db file of symbols and their respective attributes, like size.\n");
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
        retval = dump2symdb(&options);
    }

    cleanOptions(&options);
    return retval;
}

