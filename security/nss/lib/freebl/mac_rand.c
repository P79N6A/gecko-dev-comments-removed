



































#ifdef notdef
#include "xp_core.h"
#include "xp_file.h"
#endif
#include "secrng.h"
#include "mcom_db.h"
#ifdef XP_MAC
#include <Events.h>
#include <OSUtils.h>
#include <QDOffscreen.h>
#include <PPCToolbox.h>
#include <Processes.h>
#include <LowMem.h>
#include <Scrap.h>


static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen);
void FE_ReadScreen();

static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen)
{
    union endianness {
        int32 i;
        char c[4];
    } u;

    if (srclen <= dstlen) {
        memcpy(dst, src, srclen);
        return srclen;
    }
    u.i = 0x01020304;
    if (u.c[0] == 0x01) {
        
        memcpy(dst, (char*)src + (srclen - dstlen), dstlen);
    } else {
        
        memcpy(dst, src, dstlen);
    }
    return dstlen;
}

size_t RNG_GetNoise(void *buf, size_t maxbytes)
{
    UnsignedWide microTickCount;
    Microseconds(&microTickCount);
    return CopyLowBits(buf, maxbytes,  &microTickCount, sizeof(microTickCount));
}

void RNG_FileForRNG(const char *filename)
{   
    unsigned char buffer[BUFSIZ];
    size_t bytes;
#ifdef notdef 
    XP_File file;
	unsigned long totalFileBytes = 0;

	if (filename == NULL)	
		file = XP_FileOpen(NULL, xpGlobalHistory,XP_FILE_READ_BIN);
	else
		file = XP_FileOpen(NULL, xpURL,XP_FILE_READ_BIN);
    if (file != NULL) {
        for (;;) {
            bytes = XP_FileRead(buffer, sizeof(buffer), file);
            if (bytes == 0) break;
            RNG_RandomUpdate( buffer, bytes);
            totalFileBytes += bytes;
            if (totalFileBytes > 100*1024) break;	
        }
		XP_FileClose(file);
    }
#endif
    



    bytes = RNG_GetNoise(buffer, sizeof(buffer));
    RNG_RandomUpdate(buffer, sizeof(buffer));
}

void RNG_SystemInfoForRNG()
{

	{
		unsigned long sec;
		size_t bytes;
		GetDateTime(&sec);	
		RNG_RandomUpdate( &sec, sizeof(sec));
	    bytes = RNG_GetNoise(&sec, sizeof(sec));
	    RNG_RandomUpdate(&sec, bytes);
    }

	{
		MachineLocation loc;
		ReadLocation(&loc);
		RNG_RandomUpdate( &loc, sizeof(loc));
	}
#if !TARGET_CARBON

	{
		unsigned long userRef;
		Str32 userName;
		GetDefaultUser(&userRef, userName);
		RNG_RandomUpdate( &userRef, sizeof(userRef));
		RNG_RandomUpdate( userName, sizeof(userName));
	}
#endif

	{
		Point mouseLoc;
		GetMouse(&mouseLoc);
		RNG_RandomUpdate( &mouseLoc, sizeof(mouseLoc));
	}

	{
		SInt16 keyTresh = LMGetKeyThresh();
		RNG_RandomUpdate( &keyTresh, sizeof(keyTresh));
	}

	{
		SInt8 keyLast;
		keyLast = LMGetKbdLast();
		RNG_RandomUpdate( &keyLast, sizeof(keyLast));
	}

	{
		UInt8 volume = LMGetSdVolume();
		RNG_RandomUpdate( &volume, sizeof(volume));
	}
#if !TARGET_CARBON

	{
		SInt32 dir = LMGetCurDirStore();
		RNG_RandomUpdate( &dir, sizeof(dir));
	}
#endif

	{
		ProcessSerialNumber 	process;
		ProcessInfoRec pi;
	
		process.highLongOfPSN = process.lowLongOfPSN  = kNoProcess;
		
		while (GetNextProcess(&process) == noErr)
		{
			FSSpec fileSpec;
			pi.processInfoLength = sizeof(ProcessInfoRec);
			pi.processName = NULL;
			pi.processAppSpec = &fileSpec;
			GetProcessInformation(&process, &pi);
			RNG_RandomUpdate( &pi, sizeof(pi));
			RNG_RandomUpdate( &fileSpec, sizeof(fileSpec));
		}
	}
	
#if !TARGET_CARBON

	{
		THz zone = LMGetTheZone();
		RNG_RandomUpdate( &zone, sizeof(zone));
	}
#endif
	

	{
		GDHandle h = GetMainDevice();		
		RNG_RandomUpdate( *h, sizeof(GDevice));
	}

#if !TARGET_CARBON

	{
		SInt32 scrapSize = LMGetScrapSize();
		RNG_RandomUpdate( &scrapSize, sizeof(scrapSize));
	}

	{
		SInt16 scrapCount = LMGetScrapCount();
		RNG_RandomUpdate( &scrapCount, sizeof(scrapCount));
	}
#else
	{
	    ScrapRef scrap;
        if (GetCurrentScrap(&scrap) == noErr) {
            UInt32 flavorCount;
            if (GetScrapFlavorCount(scrap, &flavorCount) == noErr) {
                ScrapFlavorInfo* flavorInfo = (ScrapFlavorInfo*) malloc(flavorCount * sizeof(ScrapFlavorInfo));
                if (flavorInfo != NULL) {
                    if (GetScrapFlavorInfoList(scrap, &flavorCount, flavorInfo) == noErr) {
                        UInt32 i;
                        RNG_RandomUpdate(&flavorCount, sizeof(flavorCount));
                        for (i = 0; i < flavorCount; ++i) {
                            Size flavorSize;
                            if (GetScrapFlavorSize(scrap, flavorInfo[i].flavorType, &flavorSize) == noErr)
                                RNG_RandomUpdate(&flavorSize, sizeof(flavorSize));
                        }
                    }
                    free(flavorInfo);
                }
            }
        }
    }
#endif

	{
		HParamBlockRec			pb;
		GetVolParmsInfoBuffer	volInfo;
		pb.ioParam.ioVRefNum = 0;
		pb.ioParam.ioNamePtr = nil;
		pb.ioParam.ioBuffer = (Ptr) &volInfo;
		pb.ioParam.ioReqCount = sizeof(volInfo);
		PBHGetVolParmsSync(&pb);
		RNG_RandomUpdate( &volInfo, sizeof(volInfo));
	}
#if !TARGET_CARBON

	{
		EvQElPtr		eventQ;
		for (eventQ = (EvQElPtr) LMGetEventQueue()->qHead; 
				eventQ; 
				eventQ = (EvQElPtr)eventQ->qLink)
			RNG_RandomUpdate( &eventQ->evtQWhat, sizeof(EventRecord));
	}
#endif
	FE_ReadScreen();
	RNG_FileForRNG(NULL);
}

void FE_ReadScreen()
{
	UInt16				coords[4];
	PixMapHandle 		pmap;
	GDHandle			gh;	
	UInt16				screenHeight;
	UInt16				screenWidth;			
	UInt32				bytesToRead;			
	UInt32				offset;					
	UInt16				rowBytes;
	UInt32				rowsToRead;
	float				bytesPerPixel;			
	Ptr					p;						
	UInt16				x, y, w, h;

	gh = LMGetMainDevice();
	if ( !gh )
		return;
	pmap = (**gh).gdPMap;
	if ( !pmap )
		return;
		
	RNG_GenerateGlobalRandomBytes( coords, sizeof( coords ) );
	
		
	screenHeight = (**pmap).bounds.bottom - (**pmap).bounds.top;
	screenWidth = (**pmap).bounds.right - (**pmap).bounds.left;
	x = coords[0] % screenWidth;
	y = coords[1] % screenHeight;
	w = ( coords[2] & 0x7F ) | 0x40;		
	h = ( coords[3] & 0x7F ) | 0x40;		

	bytesPerPixel = (**pmap).pixelSize / 8;
	rowBytes = (**pmap).rowBytes & 0x7FFF;

	
	offset = ( rowBytes * y ) + (UInt32)( (float)x * bytesPerPixel );
	
	
	bytesToRead = PR_MIN(	(UInt32)( w * bytesPerPixel ),
						(UInt32)( rowBytes - ( x * bytesPerPixel ) ) );

	
	rowsToRead = PR_MIN(	h, 
						( screenHeight - y ) );
	
	p = GetPixBaseAddr( pmap ) + offset;
	
	while ( rowsToRead-- )
	{
		RNG_RandomUpdate( p, bytesToRead );
		p += rowBytes;
	}
}
#endif
