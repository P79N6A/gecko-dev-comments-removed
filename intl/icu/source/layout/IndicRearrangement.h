





#ifndef __INDICREARRANGEMENT_H
#define __INDICREARRANGEMENT_H






#include "LETypes.h"
#include "LayoutTables.h"
#include "StateTables.h"
#include "MorphTables.h"
#include "MorphStateTables.h"

U_NAMESPACE_BEGIN

struct IndicRearrangementSubtableHeader : MorphStateTableHeader
{
};

struct IndicRearrangementSubtableHeader2 : MorphStateTableHeader2
{
};

enum IndicRearrangementFlags
{
    irfMarkFirst    = 0x8000,
    irfDontAdvance  = 0x4000,
    irfMarkLast     = 0x2000,
    irfReserved     = 0x1FF0,
    irfVerbMask     = 0x000F
};

enum IndicRearrangementVerb
{
    irvNoAction = 0x0000,               
    irvxA       = 0x0001,               
    irvDx       = 0x0002,               
    irvDxA      = 0x0003,               
        
    irvxAB      = 0x0004,               
    irvxBA      = 0x0005,               
    irvCDx      = 0x0006,               
    irvDCx      = 0x0007,               

    irvCDxA     = 0x0008,               
    irvDCxA     = 0x0009,               
    irvDxAB     = 0x000A,               
    irvDxBA     = 0x000B,               

    irvCDxAB    = 0x000C,               
    irvCDxBA    = 0x000D,               
    irvDCxAB    = 0x000E,               
    irvDCxBA    = 0x000F                
};

struct IndicRearrangementStateEntry : StateEntry
{
};

struct IndicRearrangementStateEntry2 : StateEntry2
{
};

U_NAMESPACE_END
#endif

