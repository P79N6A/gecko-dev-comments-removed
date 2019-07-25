






































 
#include "nsPaperPS.h"
#include "plstr.h"
#include "nsCoord.h"
#include "nsMemory.h"

const nsPaperSizePS_ nsPaperSizePS::mList[] =
{
#define SIZE_MM(x)      (x)
#define SIZE_INCH(x)    ((x) * MM_PER_INCH_FLOAT)
    { "A5",             SIZE_MM(148),   SIZE_MM(210),   PR_TRUE },
    { "A4",             SIZE_MM(210),   SIZE_MM(297),   PR_TRUE },
    { "A3",             SIZE_MM(297),   SIZE_MM(420),   PR_TRUE },
    { "Letter",         SIZE_INCH(8.5), SIZE_INCH(11),  PR_FALSE },
    { "Legal",          SIZE_INCH(8.5), SIZE_INCH(14),  PR_FALSE },
    { "Tabloid",        SIZE_INCH(11),  SIZE_INCH(17),  PR_FALSE },
    { "Executive",      SIZE_INCH(7.5), SIZE_INCH(10),  PR_FALSE },
#undef SIZE_INCH
#undef SIZE_MM
};

const unsigned int nsPaperSizePS::mCount = NS_ARRAY_LENGTH(mList);

PRBool
nsPaperSizePS::Find(const char *aName)
{
    for (int i = mCount; i--; ) {
        if (!PL_strcasecmp(aName, mList[i].name)) {
            mCurrent = i;
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}
