



#ifndef __UMAP__
#define __UMAP__

#define NOMAPPING 0xfffd

enum {
	uFormat0Tag = 0,
	uFormat1Tag,
	uFormat2Tag,
	uNumFormatTag
};

typedef struct {
		uint16_t srcBegin;		
		uint16_t srcEnd;			
		uint16_t destBegin;		
} uFormat0;

typedef struct {
		uint16_t srcBegin;		
		uint16_t srcEnd;			
		uint16_t	mappingOffset;	
} uFormat1;

typedef struct {
		uint16_t srcBegin;		
		uint16_t srcEnd;			
		uint16_t destBegin;		
} uFormat2;

typedef struct  {
	union {
		uFormat0	format0;
		uFormat1	format1;
		uFormat2	format2;
	} fmt;
} uMapCell;

#define UMAPCELL_SIZE (3*sizeof(uint16_t))

typedef struct  {
	uint16_t 		itemOfList;
	uint16_t		offsetToFormatArray;
	uint16_t		offsetToMapCellArray;
	uint16_t		offsetToMappingTable;
	uint16_t		data[1];
} uTable;

#endif
