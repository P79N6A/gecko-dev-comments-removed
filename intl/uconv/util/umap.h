




































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
		PRUint16 srcBegin;		
		PRUint16 srcEnd;			
		PRUint16 destBegin;		
} uFormat0;

typedef struct {
		PRUint16 srcBegin;		
		PRUint16 srcEnd;			
		PRUint16	mappingOffset;	
} uFormat1;

typedef struct {
		PRUint16 srcBegin;		
		PRUint16 srcEnd;			
		PRUint16 destBegin;		
} uFormat2;

typedef struct  {
	union {
		uFormat0	format0;
		uFormat1	format1;
		uFormat2	format2;
	} fmt;
} uMapCell;

#define UMAPCELL_SIZE (3*sizeof(PRUint16))

typedef struct  {
	PRUint16 		itemOfList;
	PRUint16		offsetToFormatArray;
	PRUint16		offsetToMapCellArray;
	PRUint16		offsetToMappingTable;
	PRUint16		data[1];
} uTable;

#endif
