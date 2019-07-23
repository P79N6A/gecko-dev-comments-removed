
























#include "lcms.h"



static
LPcmsNAMEDCOLORLIST GrowNamedColorList(LPcmsNAMEDCOLORLIST v, int ByElements)
{           
    if (ByElements > v ->Allocated) {
        
        LPcmsNAMEDCOLORLIST TheNewList;
        int NewElements;
        size_t size;

        if (v ->Allocated == 0)
            NewElements = 64;   
        else
            NewElements = v ->Allocated;

        while (ByElements > NewElements)
                NewElements *= 2;
        
        size = sizeof(cmsNAMEDCOLORLIST) + (sizeof(cmsNAMEDCOLOR) * NewElements);
        TheNewList = (LPcmsNAMEDCOLORLIST) _cmsMalloc(size);
        

        if (TheNewList == NULL) {
            cmsSignalError(LCMS_ERRC_ABORTED, "Out of memory reallocating named color list");
            return NULL;
        }
        else {
              ZeroMemory(TheNewList, size);
              CopyMemory(TheNewList, v, sizeof(cmsNAMEDCOLORLIST) + (v ->nColors - 1) * sizeof(cmsNAMEDCOLOR));
              TheNewList -> Allocated = NewElements;

              _cmsFree(v);
              return TheNewList;
        }
    }
    
    return v;
}


LPcmsNAMEDCOLORLIST cmsAllocNamedColorList(int n)
{
    size_t size = sizeof(cmsNAMEDCOLORLIST) + (n - 1) * sizeof(cmsNAMEDCOLOR);

    LPcmsNAMEDCOLORLIST v = (LPcmsNAMEDCOLORLIST) _cmsMalloc(size);
    

    if (v == NULL) {
        cmsSignalError(LCMS_ERRC_ABORTED, "Out of memory creating named color list");
        return NULL;
    }

    ZeroMemory(v, size);

    v ->nColors   = n;
    v ->Allocated = n;  
    v ->Prefix[0] = 0;
    v ->Suffix[0] = 0;  
           
    return v;
}

void cmsFreeNamedColorList(LPcmsNAMEDCOLORLIST v)
{
    if (v == NULL) {
        cmsSignalError(LCMS_ERRC_RECOVERABLE, "Couldn't free a NULL named color list");
        return;
    }
                    
    _cmsFree(v);
}   

LCMSBOOL cmsAppendNamedColor(cmsHTRANSFORM xform, const char* Name, WORD PCS[3], WORD Colorant[MAXCHANNELS])
{
    _LPcmsTRANSFORM v = (_LPcmsTRANSFORM) xform;
    LPcmsNAMEDCOLORLIST List;
    int i;

    if (v ->NamedColorList == NULL) return FALSE;

    v ->NamedColorList = GrowNamedColorList(v ->NamedColorList, v->NamedColorList ->nColors + 1);
    
    List = v ->NamedColorList;

    for (i=0; i < MAXCHANNELS; i++)
        List ->List[List ->nColors].DeviceColorant[i] = Colorant[i];

    for (i=0; i < 3; i++)
        List ->List[List ->nColors].PCS[i] = PCS[i];

    strncpy(List ->List[List ->nColors].Name, Name, MAX_PATH-1);

    List ->nColors++;
    return TRUE;
}





int LCMSEXPORT cmsNamedColorCount(cmsHTRANSFORM xform)
{
     _LPcmsTRANSFORM v = (_LPcmsTRANSFORM) xform;

     if (v ->NamedColorList == NULL) return 0;
     return v ->NamedColorList ->nColors;
}


LCMSBOOL LCMSEXPORT cmsNamedColorInfo(cmsHTRANSFORM xform, int nColor, char* Name, char* Prefix, char* Suffix)
{
    _LPcmsTRANSFORM v = (_LPcmsTRANSFORM) xform;


     if (v ->NamedColorList == NULL) return FALSE;

     if (nColor < 0 || nColor >= cmsNamedColorCount(xform)) return FALSE;

     if (Name) strncpy(Name, v ->NamedColorList->List[nColor].Name, 31);
     if (Prefix) strncpy(Prefix, v ->NamedColorList->Prefix, 31);
     if (Suffix) strncpy(Suffix, v ->NamedColorList->Suffix, 31);

     return TRUE;
}


int  LCMSEXPORT cmsNamedColorIndex(cmsHTRANSFORM xform, const char* Name)
{
    _LPcmsTRANSFORM v = (_LPcmsTRANSFORM) xform;    
    int i, n;

         if (v ->NamedColorList == NULL) return -1;

        n = cmsNamedColorCount(xform);
        for (i=0; i < n; i++) {
            if (stricmp(Name,  v ->NamedColorList->List[i].Name) == 0)
                    return i;
        }

        return -1;
}


