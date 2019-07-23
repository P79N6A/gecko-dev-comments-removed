






































#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <MacTypes.h>
#include "MoreFiles.h"
#include "MoreFilesExtras.h"
#include "FullPath.h"  
#include <AppleEvents.h>
#include <Gestalt.h>
#include <TextUtils.h>
#include <Folders.h>
#include <Processes.h>
#include <Resources.h>
#include <Aliases.h>

#include "InstallCleanup.h"
#include "InstallCleanupDefines.h"

#define kSleepMax 60  // sleep 1 second

Boolean gAppleEventsFlag, gQuitFlag;
long gSleepVal;


int   strcasecmp(const char *str1, const char *str2);
OSErr GetFSSpecFromPath(char *aPath, FSSpec *aSpec);
void  my_c2pstrcpy(Str255 aDstPStr, const char *aSrcCStr);
OSErr GetCWD(short *aVRefNum, long *aDirID);
OSErr GetCleanupReg(FSSpec *aCleanupReg);

int strcasecmp(const char *str1, const char *str2)
{
    char    currentChar1, currentChar2;

    while (1) {
    
        currentChar1 = *str1;
        currentChar2 = *str2;
        
        if ((currentChar1 >= 'a') && (currentChar1 <= 'z'))
            currentChar1 += ('A' - 'a');
        
        if ((currentChar2 >= 'a') && (currentChar2 <= 'z'))
            currentChar2 += ('A' - 'a');
                
        if (currentChar1 == '\0')
            break;
    
        if (currentChar1 != currentChar2)
            return currentChar1 - currentChar2;
            
        str1++;
        str2++;
    
    }
    
    return currentChar1 - currentChar2;
}

OSErr GetFSSpecFromPath(const char *aPath, FSSpec *aSpec)
{
    if (!aPath || !aSpec)
        return paramErr;
        
    
    
    
    
    if ((!*aPath) || 
       (*(aPath + strlen(aPath) - 1) == ':') ||
       (*aPath == ':'))
    {
       return paramErr;
    }
    
    
    return FSpLocationFromFullPath(strlen(aPath), (const void *) aPath, aSpec);
}

void
my_c2pstrcpy(Str255 aDstPStr, const char *aSrcCStr)
{
    if (!aSrcCStr)
        return;
    
    memcpy(&aDstPStr[1], aSrcCStr, strlen(aSrcCStr) > 255 ? 255 : strlen(aSrcCStr));
    aDstPStr[0] = strlen(aSrcCStr);
}

OSErr
GetCWD(short *aVRefNum, long *aDirID)
{
    OSErr               err = noErr;
    ProcessSerialNumber psn;
    ProcessInfoRec      pInfo;
    FSSpec              tmp;
        
    if (!aVRefNum || !aDirID)
        return paramErr;
    
    *aVRefNum = 0;
    *aDirID = 0;
    
    
    if (!(err = GetCurrentProcess(&psn))) 
    {
        pInfo.processName = nil;
        pInfo.processAppSpec = &tmp;
        pInfo.processInfoLength = (sizeof(ProcessInfoRec));
             
        if(!(err = GetProcessInformation(&psn, &pInfo)))
        {   
            *aVRefNum = pInfo.processAppSpec->vRefNum;
            *aDirID = pInfo.processAppSpec->parID; 
        }
    }
      
    return err;
}

OSErr
GetCleanupReg(FSSpec *aCleanupReg)
{
    OSErr err = noErr;
    short efVRefNum = 0;
    long efDirID = 0;
    
    if (!aCleanupReg)
        return paramErr;
        
    err = GetCWD(&efVRefNum, &efDirID);
    if (err == noErr)
    {
        Str255 pCleanupReg;
        my_c2pstrcpy(pCleanupReg, CLEANUP_REGISTRY);
        err = FSMakeFSSpec(efVRefNum, efDirID, pCleanupReg, aCleanupReg);
    }
    
    return err;
}


#pragma mark -




int NativeDeleteFile(const char* aFileToDelete)
{
    OSErr err;
    FSSpec delSpec;
    
    if (!aFileToDelete)
        return DONE;
        
    
    err = GetFSSpecFromPath(aFileToDelete, &delSpec);
    if (err != noErr)
    {
        
        return DONE;
    }
        
    
    err = FSpDelete(&delSpec);
    if (err != noErr)
    {
        
        return TRY_LATER;
    }

    return DONE;
}




int NativeReplaceFile(const char* aReplacementFile, const char* aDoomedFile )
{
    OSErr err;
    FSSpec replSpec, doomSpec, tgtDirSpec;
    long dirID;
    Boolean isDir;
    
    if (!aReplacementFile || !aDoomedFile)
        return DONE;
        
    err = GetFSSpecFromPath(aReplacementFile, &replSpec);
    if (err != noErr)
        return DONE;
                      
    
    err = FSpGetDirectoryID(&replSpec, &dirID, &isDir);
    if (err != noErr || isDir)
    {
        
        return DONE;
    }
        
    
    if (strcasecmp(aReplacementFile, aDoomedFile) == 0)
    {
        
        return DONE;
    }
        
    
    err = GetFSSpecFromPath(aDoomedFile, &doomSpec); 
    if (err == noErr)
    { 
        
        err = FSpDelete(&doomSpec);
        
        
        if (err != noErr)
            return TRY_LATER;
    }
    
    
    err = FSMakeFSSpec(doomSpec.vRefNum, doomSpec.parID, "\p", &tgtDirSpec);
    if (err == noErr)
    {
        
        err = FSpMoveRename(&replSpec, &tgtDirSpec, doomSpec.name);
        if (err != noErr)
        {
            
            return TRY_LATER;
        }
    }
        
    return DONE;
}


#pragma mark -




OSErr
GetProgramSpec(FSSpecPtr aProgSpec)
{
	OSErr 				err = noErr;
	ProcessSerialNumber	psn;
	ProcessInfoRec		pInfo;
	
	if (!aProgSpec)
	    return paramErr;
	    
	
	if (!(err = GetCurrentProcess(&psn))) 
	{
		pInfo.processName = nil;
		pInfo.processAppSpec = aProgSpec;
		pInfo.processInfoLength = (sizeof(ProcessInfoRec));
		
		err = GetProcessInformation(&psn, &pInfo);
	}
	
	return err;
}

void
PutAliasInStartupItems(FSSpecPtr aAlias)
{
    OSErr err;
    FSSpec fsProg, fsAlias;
    long strtDirID = 0;
    short strtVRefNum = 0;
    FInfo info;
    AliasHandle aliasH;

    if (!aAlias)
        return;
        
    
    err = GetProgramSpec(&fsProg);
    if (err != noErr)
        return;  
     
    
    err = FindFolder(kOnSystemDisk, kStartupFolderType, kCreateFolder, 
                     &strtVRefNum, &strtDirID);
    if (err != noErr)
        return;
             
    
    
    err = FSMakeFSSpec(strtVRefNum, strtDirID, fsProg.name, &fsAlias);
    if (err == noErr)
    {
        
        
        err = FSpDelete(&fsAlias);
        if (err != noErr)
            return;  
    }
      
    
    err = NewAliasMinimal(&fsProg, &aliasH);
    if (err != noErr) 
        return;
        
    FSpGetFInfo(&fsProg, &info);
    FSpCreateResFile(&fsAlias, info.fdCreator, info.fdType, smRoman);
    short refNum = FSpOpenResFile(&fsAlias, fsRdWrPerm);
    if (refNum != -1)
    {
        UseResFile(refNum);
        AddResource((Handle)aliasH, rAliasType, 0, fsAlias.name);
        ReleaseResource((Handle)aliasH);
        UpdateResFile(refNum);
        CloseResFile(refNum);
    }
    else
    {
        ReleaseResource((Handle)aliasH);
        FSpDelete(&fsAlias);
        return;  
    }

    
    FSpGetFInfo(&fsAlias, &info);
    info.fdFlags |= kIsAlias;
    FSpSetFInfo(&fsAlias, &info);    
    
    *aAlias = fsAlias;
}

void
RemoveAliasFromStartupItems(FSSpecPtr aAlias)
{
    
    FSpDelete(aAlias);
}


#pragma mark -





static pascal OSErr DoAEOpenApplication(const AppleEvent * theAppleEvent, AppleEvent * replyAppleEvent, long refCon)
{
#pragma unused (theAppleEvent, replyAppleEvent, refCon)
    return noErr;
}

static pascal OSErr DoAEOpenDocuments(const AppleEvent * theAppleEvent, AppleEvent * replyAppleEvent, long refCon)
{
#pragma unused (theAppleEvent, replyAppleEvent, refCon)
    return errAEEventNotHandled;
}

static pascal OSErr DoAEPrintDocuments(const AppleEvent * theAppleEvent, AppleEvent * replyAppleEvent, long refCon)
{
#pragma unused (theAppleEvent, replyAppleEvent, refCon)
    return errAEEventNotHandled;
}

static pascal OSErr DoAEQuitApplication(const AppleEvent * theAppleEvent, AppleEvent * replyAppleEvent, long refCon)
{
#pragma unused (theAppleEvent, replyAppleEvent, refCon)
    gQuitFlag = true;
    return noErr;
}






static void InitAppleEventsStuff(void)
{
    OSErr retCode;

    if (gAppleEventsFlag) {

        retCode = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                    NewAEEventHandlerUPP(DoAEOpenApplication), 0, false);

        if (retCode == noErr)
            retCode = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                    NewAEEventHandlerUPP(DoAEOpenDocuments), 0, false);

        if (retCode == noErr)
            retCode = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
                    NewAEEventHandlerUPP(DoAEPrintDocuments), 0, false);
        if (retCode == noErr)
            retCode = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                    NewAEEventHandlerUPP(DoAEQuitApplication), 0, false);

        if (retCode != noErr) DebugStr("\pInstall event handler failed");
        
    }
}






static void DoHighLevelEvent(EventRecord * theEventRecPtr)
{
    (void) AEProcessAppleEvent(theEventRecPtr);
}


#pragma mark -

void main(void)
{
    OSErr retCode;
    long gestResponse;
    FSSpec aliasToSelf;
    FSSpec fsCleanupReg;

    EventRecord mainEventRec;
    Boolean eventFlag, bDone = false, bHaveCleanupReg = false;
    
    HREG reg;
    int rv = DONE;

#if !TARGET_CARBON
    
    InitGraf(&qd.thePort);
#endif

    
    gQuitFlag = false;
    gSleepVal = kSleepMax;

    
    retCode = Gestalt(gestaltAppleEventsAttr, &gestResponse);
    if (retCode == noErr &&
        (gestResponse & (1 << gestaltAppleEventsPresent)) != 0)
        gAppleEventsFlag = true;
    else gAppleEventsFlag = false;

    
    InitAppleEventsStuff();

    
    
    
    FSMakeFSSpec(0, 0, "\p", &aliasToSelf);  
    PutAliasInStartupItems(&aliasToSelf);
    
    if ( REGERR_OK == NR_StartupRegistry() )
    {
        char *regName = "";
        Boolean regNameAllocd = false;
        Handle pathH = 0;
        short pathLen = 0;
        
        
        retCode = GetCleanupReg(&fsCleanupReg);
        if (retCode == noErr)
        {
            bHaveCleanupReg = true;
            
            
            retCode = FSpGetFullPath(&fsCleanupReg, &pathLen, &pathH);
            if (retCode == noErr && pathH)
            {
                HLock(pathH);
                if (*pathH)
                {
                    regName = (char *) malloc(sizeof(char) * (pathLen + 1));
                    if (regName)
                        regNameAllocd = true;
                    else
                        retCode = memFullErr;
                    strncpy(regName, *pathH, pathLen);
                    *(regName + pathLen) = 0;
                }
                HUnlock(pathH);
                DisposeHandle(pathH);
            }
        }
            
        if ( (retCode == noErr) && (REGERR_OK == NR_RegOpen(regName, &reg)) )
        {
            

            while (!gQuitFlag)
            {
                eventFlag = WaitNextEvent(everyEvent, &mainEventRec, gSleepVal, nil);

                if (mainEventRec.what == kHighLevelEvent)
                    DoHighLevelEvent(&mainEventRec);

                rv = PerformScheduledTasks(reg);
                if (rv == DONE)
                {
                    bDone = true;
                    gQuitFlag = true;
                }
            }
            NR_RegClose(&reg);
            NR_ShutdownRegistry();
        }
        
        if (regNameAllocd)
            free(regName);      
    }
    
    
    
    if (bDone)
    {
        if (bHaveCleanupReg)
            FSpDelete(&fsCleanupReg);
        RemoveAliasFromStartupItems(&aliasToSelf);
    }
}

