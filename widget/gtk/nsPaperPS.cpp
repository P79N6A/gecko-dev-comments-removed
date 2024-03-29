





#include "mozilla/ArrayUtils.h"
 
#include "nsPaperPS.h"
#include "plstr.h"
#include "nsCoord.h"
#include "nsMemory.h"

using namespace mozilla;

const nsPaperSizePS_ nsPaperSizePS::mList[] =
{
#define SIZE_MM(x)      (x)
#define SIZE_INCH(x)    ((x) * MM_PER_INCH_FLOAT)
    { "A5",             SIZE_MM(148),   SIZE_MM(210),   true },
    { "A4",             SIZE_MM(210),   SIZE_MM(297),   true },
    { "A3",             SIZE_MM(297),   SIZE_MM(420),   true },
    { "Letter",         SIZE_INCH(8.5), SIZE_INCH(11),  false },
    { "Legal",          SIZE_INCH(8.5), SIZE_INCH(14),  false },
    { "Tabloid",        SIZE_INCH(11),  SIZE_INCH(17),  false },
    { "Executive",      SIZE_INCH(7.5), SIZE_INCH(10),  false },
#undef SIZE_INCH
#undef SIZE_MM
};

const unsigned int nsPaperSizePS::mCount = ArrayLength(mList);

bool
nsPaperSizePS::Find(const char *aName)
{
    for (int i = mCount; i--; ) {
        if (!PL_strcasecmp(aName, mList[i].name)) {
            mCurrent = i;
            return true;
        }
    }
    return false;
}
