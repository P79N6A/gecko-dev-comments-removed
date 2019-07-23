







































#if !defined __msmap_H
#define __msmap_H


#if defined(__cplusplus)
extern "C" {
#endif
#if 0
}
#endif





typedef unsigned long address;


typedef enum __enum_MSMap_SymbolScope



{
    PUBLIC,
    STATIC,
    UNDEFINED
}
MSMap_SymbolScope;


typedef enum __enum_MSMap_SegmentClass



{
    CODE,
    DATA
}
MSMap_SegmentClass;


typedef struct __struct_MSMap_Segment



{
    address mPrefix;
    address mOffset;
    address mLength;
    address mUsed;
    char* mSegment;
    MSMap_SegmentClass mClass;
}
MSMap_Segment;


typedef struct __struct_MSMap_Symbol



{
    address mPrefix;
    address mOffset;
    char* mSymbol;
    address mRVABase;
    char* mObject;
    MSMap_SymbolScope mScope;
    unsigned mSymDBSize;
    MSMap_Segment* mSection;
}
MSMap_Symbol;


typedef struct __struct_MSMap_Module



{
    char* mModule;
    time_t mTimestamp;
    address mPreferredLoadAddress;
    MSMap_Segment* mSegments;
    unsigned mSegmentCount;
    unsigned mSegmentCapacity;
    address mEntryPrefix;
    address mEntryOffset;
    MSMap_Symbol* mSymbols;
    unsigned mSymbolCount;
    unsigned mSymbolCapacity;
}
MSMap_Module;





#define MSMAP_SEGMENT_GROWBY 0x10
#define MSMAP_SYMBOL_GROWBY  0x100


#if 0
{
#endif
#if defined(__cplusplus)
} 
#endif


#endif
