























































#ifndef CIDMAP_H
#define CIDMAP_H

#include <stdio.h>
#include "nscore.h"

typedef struct {
  PRUint32 num_bytes;
  PRUint32 start;
  PRUint32 end;
} CodeSpaceRangeElement;

void WriteCidRangeMapUnicode(FILE *aFile);
void WriteCidCharMap(const PRUnichar *aCharIDs, PRUint32 *aCIDs, int aLen,
                     FILE *aFile);
void WriteCmapFooter(FILE *aFile);
void WriteCmapHeader(const char *aName, const char *aRegistry, 
                     const char *aEncoding, int aSupplement, int aType, 
                     int aWmode, FILE *aFile);
int WriteCodeSpaceRangeMap(CodeSpaceRangeElement *aElements, int aLen, 
                           FILE *aFile);
void WriteCodeSpaceRangeMapUCS2(FILE *aFile);

#endif 
