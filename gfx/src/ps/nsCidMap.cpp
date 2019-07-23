

























































#include "nsCidMap.h"

CodeSpaceRangeElement UCS2_CodeSpaceRange[2] = {
  { 2, 0x0000, 0xD7FF },
  { 2, 0xE000, 0xFFFF },
};

int len_UCS2_CodeSpaceRange = 
             sizeof(UCS2_CodeSpaceRange)/sizeof(UCS2_CodeSpaceRange[0]);

void
WriteCidCharMap(const PRUnichar *aCharIDs, PRUint32 *aCIDs,
                int aLen, FILE *aFile)
{
  int i, j, blk_len;
  int fix_len = 0; 

  while (aLen) {
    
    if (aLen >= 100)
      blk_len = 100;
    else
      blk_len = aLen;

    if (blk_len == 2) {
      fix_len = 1;
      fprintf(aFile, "%% add an extra dummy value to the end of this block "
              " since older versions of\n");
      fprintf(aFile, "%% Ghostscript do not like a block len of 2\n");
    }

    
    fprintf(aFile, "%d begincidchar\n", blk_len+fix_len);
    for (i=0; i<blk_len; i++)
      fprintf(aFile, "<%04X> %d\n", aCharIDs[i], aCIDs[i]);
    for (j=0; j<fix_len; j++) 
      fprintf(aFile, "<%04X> %d\n", aCharIDs[i-1], aCIDs[i-1]);
    fprintf(aFile, "endcidchar\n\n");

    
    aLen -= blk_len;
    aCharIDs += blk_len;
    aCIDs += blk_len;
  }
}

void
WriteCidRangeMapUnicode(FILE *aFile)
{
  int i;

  fprintf(aFile, "100 begincidrange\n");
  for (i=0; i<100; i++)
    fprintf(aFile, "<%04X> <%04X> %d\n", i*256, ((i+1)*256)-1, i*256);
  fprintf(aFile, "endcidrange\n\n");

  fprintf(aFile, "100 begincidrange\n");
  for (i=100; i<200; i++)
    fprintf(aFile, "<%04X> <%04X> %d\n", i*256, ((i+1)*256)-1, i*256);
  fprintf(aFile, "endcidrange\n\n");

  fprintf(aFile, "56 begincidrange\n");
  for (i=200; i<256; i++)
    fprintf(aFile, "<%04X> <%04X> %d\n", i*256, ((i+1)*256)-1, i*256);
  fprintf(aFile, "endcidrange\n\n");

}

void
WriteCmapHeader(const char *aName, const char *aRegistry, 
                const char *aEncoding, int aSupplement,
                int aType, int aWmode, FILE *aFile)
{
  fprintf(aFile, "%%%%DocumentNeededResources: procset CIDInit\n");
  fprintf(aFile, "%%%%IncludeResource: procset CIDInit\n");
  fprintf(aFile, "%%%%BeginResource: CMap %s\n", aName);
  fprintf(aFile, "%%%%Title: (%s %s %s %d)\n", aName, aRegistry, aEncoding,aSupplement);
  fprintf(aFile, "%%%%Version : 1\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "/CIDInit /ProcSet findresource begin\n");
  
  
  
  fprintf(aFile, "\n");
  fprintf(aFile, "12 dict begin\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "begincmap\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "/CIDSystemInfo 3 dict dup begin\n");
  fprintf(aFile, "  /Registry (%s) def\n", aRegistry);
  fprintf(aFile, "  /Ordering (%s) def\n", aEncoding);
  fprintf(aFile, "  /Supplement %d def\n", aSupplement);
  fprintf(aFile, "end def\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "/CMapName /%s def\n", aName);
  fprintf(aFile, "\n");
  fprintf(aFile, "/CMapVersion 1 def\n");
  fprintf(aFile, "/CMapType %d def\n", aType);
  fprintf(aFile, "\n");
  fprintf(aFile, "/WMode %d def\n", aWmode);
  fprintf(aFile, "\n");
}

void
WriteCmapFooter(FILE *aFile)
{
  fprintf(aFile, "endcmap\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "CMapName currentdict /CMap defineresource pop\n");
  fprintf(aFile, "\n");
  fprintf(aFile, "end\n");
  fprintf(aFile, "end\n");
  fprintf(aFile, "%%%%EndResource\n");
  fprintf(aFile, "\n");
}

PRBool
WriteCodeSpaceRangeMap(CodeSpaceRangeElement *aElements, int aLen, FILE *aFile)
{
  int i, blk_len;

  while (aLen) {
    
    if (aLen >= 100)
      blk_len = 100;
    else
      blk_len = aLen;

    
    fprintf(aFile, "%d begincodespacerange\n", blk_len);
    for (i=0; i<blk_len; i++, aElements++) {
      if (aElements->num_bytes == 1)
        fprintf(aFile, "<%02X>   <%02X>\n", aElements->start, aElements->end);
      else if (aElements->num_bytes == 2)
        fprintf(aFile, "<%04X> <%04X>\n", aElements->start, aElements->end);
      else {
        fprintf(aFile, "codespacerange: invalid num_bytes (%d)\nexiting...\n", 
                aElements->num_bytes);
        return PR_FALSE;
      }
    }
    fprintf(aFile, "endcodespacerange\n\n");

    
    aLen -= blk_len;
  }
  return PR_TRUE;
}

void
WriteCodeSpaceRangeMapUCS2(FILE *aFile)
{
  WriteCodeSpaceRangeMap(UCS2_CodeSpaceRange, len_UCS2_CodeSpaceRange, aFile);
}

