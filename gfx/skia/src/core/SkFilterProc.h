








#ifndef SkFilter_DEFINED
#define SkFilter_DEFINED

#include "SkMath.h"
#include "SkFixed.h"

typedef unsigned (*SkFilterProc)(unsigned x00, unsigned x01,
                                 unsigned x10, unsigned x11);

const SkFilterProc* SkGetBilinearFilterProcTable();

inline SkFilterProc SkGetBilinearFilterProc(const SkFilterProc* table,
                                            SkFixed x, SkFixed y)
{
    SkASSERT(table);
    
    
    x = (unsigned)(x << 16) >> 30;
    y = (unsigned)(y << 16) >> 30;
    return table[(y << 2) | x];
}

inline SkFilterProc SkGetBilinearFilterProc22(const SkFilterProc* table,
                                              unsigned x, unsigned y)
{
    SkASSERT(table);
    
    
    x = x << 30 >> 30;
    y = y << 30 >> 30;
    return table[(y << 2) | x];
}

inline const SkFilterProc* SkGetBilinearFilterProc22Row(const SkFilterProc* table,
                                                        unsigned y)
{
    SkASSERT(table);
    
    return &table[y << 30 >> 28];
}

inline SkFilterProc SkGetBilinearFilterProc22RowProc(const SkFilterProc* row,
                                                     unsigned x)
{
    SkASSERT(row);    
    
    return row[x << 30 >> 30];
}



typedef unsigned (*SkFilter32Proc)(uint32_t x00, uint32_t x01,
                                   uint32_t x10, uint32_t x11);

const SkFilter32Proc* SkGetFilter32ProcTable();

inline SkFilter32Proc SkGetFilter32Proc22(const SkFilter32Proc* table,
                                          unsigned x, unsigned y)
{
    SkASSERT(table);
    
    
    x = x << 30 >> 30;
    y = y << 30 >> 30;
    return table[(y << 2) | x];
}

inline const SkFilter32Proc* SkGetFilter32Proc22Row(const SkFilter32Proc* table,
                                                    unsigned y)
{
    SkASSERT(table);
    
    return &table[y << 30 >> 28];
}

inline SkFilter32Proc SkGetFilter32Proc22RowProc(const SkFilter32Proc* row,
                                                 unsigned x)
{
    SkASSERT(row);
    
    return row[x << 30 >> 30];
}






typedef uint32_t (*SkFilterPtrProc)(const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*);

const SkFilterPtrProc* SkGetBilinearFilterPtrProcTable();
inline SkFilterPtrProc SkGetBilinearFilterPtrProc(const SkFilterPtrProc* table, SkFixed x, SkFixed y)
{
    SkASSERT(table);

    
    x = (unsigned)(x << 16) >> 30;
    y = (unsigned)(y << 16) >> 30;
    return table[(y << 2) | x];
}





inline const SkFilterPtrProc* SkGetBilinearFilterPtrProcYTable(const SkFilterPtrProc* table, SkFixed y)
{
    SkASSERT(table);

    y = (unsigned)(y << 16) >> 30;
    return table + (y << 2);
}




inline SkFilterPtrProc SkGetBilinearFilterPtrXProc(const SkFilterPtrProc* table, SkFixed x)
{
    SkASSERT(table);

    
    x = (unsigned)(x << 16) >> 30;
    return table[x];
}

#endif


