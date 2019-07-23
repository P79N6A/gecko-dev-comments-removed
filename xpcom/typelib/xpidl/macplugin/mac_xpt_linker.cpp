












































#include <stdio.h>
#include <string.h>
#include <setjmp.h>


#include <Files.h>
#include <Strings.h>
#include <Aliases.h>
#include <Resources.h>


#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"


#include "mac_xpidl_panel.h"
#include "mac_console.h"
#include "mac_strings.h"
#include "FullPath.h"
#include "MoreFilesExtras.h"


#define kDebuggerCreator	'MWDB'


static CWResult	Link(CWPluginContext context);
static CWResult	Disassemble(CWPluginContext context);
static CWResult	GetTargetInfo(CWPluginContext context);

extern "C" {
pascal short xpt_linker(CWPluginContext context);
int xptlink_main(int argc, char* argv[]);
int xptdump_main(int argc, char* argv[]);

FILE * FSp_fopen(ConstFSSpecPtr spec, const char * open_mode);
size_t mac_get_file_length(const char* filename);
}


extern jmp_buf exit_jump;
extern int exit_status;


CWPluginContext gPluginContext;


static CWFileSpec gOutputDirectory;
static CWFileSpec gObjectCodeDirectory;





pascal short xpt_linker(CWPluginContext context)
{
	long request;
	if (CWGetPluginRequest(context, &request) != cwNoErr)
		return cwErrRequestFailed;
	
	gPluginContext = context;
	short result = cwNoErr;
		
	
	switch (request) {
	case reqInitLinker:
		
		break;
		
	case reqTermLinker:
		
		break;
		
	case reqLink:
		
		result = Link(context);
		break;
		
	case reqDisassemble:
		
		result = Disassemble(context);
		break;
	
	case reqTargetInfo:
		
		result = GetTargetInfo(context);
		break;
		
	default:
		result = cwErrRequestFailed;
		break;
	}
	
	result = CWDonePluginRequest(context, result);
	
	
	return result;
}

static char* full_path_to(const FSSpec& file)
{
	short len = 0;
	Handle fullPath = NULL;
	if (FSpGetFullPath(&file, &len, &fullPath) == noErr && fullPath != NULL) {
		char* path = new char[1 + len];
		if (path != NULL) {
			BlockMoveData(*fullPath, path, len);
			path[len] = '\0';
		}
		DisposeHandle(fullPath);
		return path;
	}
	return NULL;
}




static char* full_path_to(short vRefNum, long dirID)
{
	long parID;
	if (GetParentID(vRefNum, dirID, NULL, &parID) == noErr) {
		FSSpec dirSpec = { vRefNum, parID };
		if (GetDirName(vRefNum, dirID, dirSpec.name) == noErr) {
			return full_path_to(dirSpec);
		}
	}
	return NULL;
}





size_t mac_get_file_length(const char* filename)
{
	FSSpec fileSpec = { gObjectCodeDirectory.vRefNum, gObjectCodeDirectory.parID };
	c2p_strcpy(fileSpec.name, filename);
	long dataSize, rsrcSize;
	if (FSpGetFileSize(&fileSpec, &dataSize, &rsrcSize) != noErr)
		dataSize = 0;
	return dataSize;
}





FILE* std::fopen(const char* filename, const char *mode)
{
	CWFileSpec& fileDir = (mode[0] == 'r' ? gObjectCodeDirectory : gOutputDirectory);
	FSSpec fileSpec = { fileDir.vRefNum, fileDir.parID };
	c2p_strcpy(fileSpec.name, filename);
	return FSp_fopen(&fileSpec, mode);
}

static CWResult GetSettings(CWPluginContext context, XPIDLSettings& settings)
{
	CWMemHandle	settingsHand;
	CWResult err = CWGetNamedPreferences(context, kXPIDLPanelName, &settingsHand);
	if (!CWSUCCESS(err))
		return err;
	
	XPIDLSettings* settingsPtr = NULL;
	err = CWLockMemHandle(context, settingsHand, false, (void**)&settingsPtr);
	if (!CWSUCCESS(err))
		return err;
	
	settings = *settingsPtr;
	
	err = CWUnlockMemHandle(context, settingsHand);
	if (!CWSUCCESS(err))
		return err;

	return cwNoErr;
}

static CWResult LinkHeaders(CWPluginContext context, XPIDLSettings& settings)
{
	
	long fileCount = 0;
	CWResult err = CWGetProjectFileCount(context, &fileCount);
	if (err != cwNoErr || fileCount == 0)
		return err;

	
	FSSpec outputDir;
	err = CWGetOutputFileDirectory(context, &outputDir);
	if (!CWSUCCESS(err))
		return err;
	
	
	
	for (long index = 0; (err == cwNoErr) && (index < fileCount); index++) {
		
		CWFileSpec outputFile;
		err = CWGetStoredObjectFileSpec(context, index, &outputFile);
		if (err == cwNoErr) {
			FInfo info;
			err = FSpGetFInfo(&outputFile, &info);
			
			FSSpec aliasFile = { outputDir.vRefNum, outputDir.parID };
			BlockMoveData(outputFile.name, aliasFile.name, 1 + outputFile.name[0]);
			
			AliasHandle alias = NULL;
			if (NewAliasMinimal(&outputFile, &alias) == noErr) {
				
				FSpDelete(&aliasFile);
				FSpCreateResFile(&aliasFile, info.fdCreator, info.fdType, smRoman);
				short refNum = FSpOpenResFile(&aliasFile, fsRdWrPerm);
				if (refNum != -1) {
					UseResFile(refNum);
					AddResource(Handle(alias), rAliasType, 0, aliasFile.name);
					ReleaseResource(Handle(alias));
					UpdateResFile(refNum);
					CloseResFile(refNum);
				}
				
				FSpGetFInfo(&aliasFile, &info);
				info.fdFlags |= kIsAlias;
				FSpSetFInfo(&aliasFile, &info);
			}
		}
	}
	
	
	BlockMoveData(settings.output, outputDir.name, 1 + settings.output[0]);
	FILE* outputFile = FSp_fopen(&outputDir, "w");
	if (outputFile != NULL) fclose(outputFile);

	return err;
}

static CWResult LinkTypeLib(CWPluginContext context, XPIDLSettings& settings)
{
	
	long fileCount = 0;
	CWResult err = CWGetProjectFileCount(context, &fileCount);
	if (err != cwNoErr || fileCount == 0)
		return err;

	
	
	char** argv = new char*[2 + fileCount + 1];
	int argc = 0;
	argv[argc++] = "xpt_link";

	
	err = CWGetOutputFileDirectory(context, &gOutputDirectory);
	if (!CWSUCCESS(err))
		return err;
	
	
	err = CWGetStoredObjectFileSpec(context, 0, &gObjectCodeDirectory);
	if (!CWSUCCESS(err))
		return err;

	
	if ((argv[argc++] = p2c_strdup(settings.output)) == NULL)
		return cwErrOutOfMemory;

	for (long index = 0; (err == cwNoErr) && (index < fileCount); index++) {
		
		CWFileSpec outputFile;
		err = CWGetStoredObjectFileSpec(context, index, &outputFile);
		if (err == cwNoErr) {
			if ((argv[argc++] = p2c_strdup(outputFile.name)) == NULL) {
				err = cwErrOutOfMemory;
				break;
			}
		}
	}
	
	if (err != cwNoErr)
		return err;
	
	
	if (setjmp(exit_jump) == 0) {
		if (xptlink_main(argc, argv) != 0)
			err = cwErrRequestFailed;
	} else {
		
		if (exit_status != 0)
			err = cwErrRequestFailed;
	}
	
	return err;
}

static CWResult	Link(CWPluginContext context)
{
	
	XPIDLSettings settings = { kXPIDLSettingsVersion, kXPIDLModeTypelib, false, false };
	CWResult err = GetSettings(context, settings);
	if (err != cwNoErr)
		return err;

	switch (settings.mode) {
	case kXPIDLModeHeader:
		return LinkHeaders(context, settings);
	case kXPIDLModeTypelib:
		return LinkTypeLib(context, settings);
	default:
		return cwNoErr;
	}
}

static CWResult	Disassemble(CWPluginContext context)
{
	CWResult err = noErr;

	
	err = CWGetOutputFileDirectory(gPluginContext, &gOutputDirectory);
	if (!CWSUCCESS(err))
		return err;

	long fileNum;
	err = CWGetMainFileNumber(context, &fileNum);
	if (!CWSUCCESS(err))
		return err;

	
	err = CWGetStoredObjectFileSpec(context, fileNum, &gObjectCodeDirectory);
	if (!CWSUCCESS(err))
		return err;
	
	char* outputName = p2c_strdup(gObjectCodeDirectory.name);
	if (outputName == NULL)
		return cwErrOutOfMemory;

	XPIDLSettings settings = { kXPIDLSettingsVersion, kXPIDLModeTypelib, false, false };
	GetSettings(context, settings);

	
	int argc = 1;
	char* argv[] = { "xpt_dump", NULL, NULL, NULL };
	if (settings.verbose) argv[argc++] = "-v";
	argv[argc++] = outputName;
	
	
	if (setjmp(exit_jump) == 0) {
		if (xptdump_main(argc, argv) != 0)
			err = cwErrRequestFailed;
	} else {
		
		if (exit_status != 0)
			err = cwErrRequestFailed;
	}

	delete[] outputName;

	if (err == noErr) {
		
		CWNewTextDocumentInfo info = {
			NULL,
			mac_console_handle,
			false
		};
		CWResizeMemHandle(context, mac_console_handle, mac_console_count);
		err = CWCreateNewTextDocument(context, &info);
	}

	return err;
}

static CWResult	GetTargetInfo(CWPluginContext context)
{
	CWTargetInfo targ;
	memset(&targ, 0, sizeof(targ));
	
	CWResult err = CWGetOutputFileDirectory(context, &targ.outfile);
	targ.outputType = linkOutputFile;
	targ.symfile = targ.outfile;	
	targ.linkType = exelinkageFlat;
	targ.targetCPU = '****';
	targ.targetOS = '****';
	
	
	XPIDLSettings settings = { kXPIDLSettingsVersion, kXPIDLModeTypelib, false, false };
	err = GetSettings(context, settings);
	if (err != cwNoErr)
		return err;
	
#if CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS
	
	targ.outfileCreator		= 'MMCH';
	targ.outfileType		= 'CWIE';
	targ.debuggerCreator	= kDebuggerCreator;	

	BlockMoveData(settings.output, targ.outfile.name, 1 + settings.output[0]);
	targ.symfile.name[0] = 0;
#endif

#if CWPLUGIN_HOST == CWPLUGIN_HOST_WIN32
	targ.debugHelperIsRegKey = true;
	*(long*)targ.debugHelperName = kDebuggerCreator;
	targ.debugHelperName[4] = 0;
	strcat(targ.outfile.path, "\\");
	strcat(targ.outfile.path, prefsData.outfile);
	strcpy(targ.symfile.path, targ.outfile.path);
	strcat(targ.symfile.path, ".SYM");
#endif

	targ.runfile			= targ.outfile;
	targ.linkAgainstFile	= targ.outfile;

	
	
	
	
	
	
	err = CWSetTargetInfo(context, &targ);
	
	return err;
}

#if 0

#if CW_USE_PRAGMA_EXPORT
#pragma export on
#endif

CWPLUGIN_ENTRY(CWPlugin_GetDropInFlags)(const DropInFlags** flags, long* flagsSize)
{
	static const DropInFlags sFlags = {
		kCurrentDropInFlagsVersion,
		CWDROPINLINKERTYPE,
		DROPINCOMPILERLINKERAPIVERSION_7,
		(linkMultiTargAware | linkAlwaysReload),
		0,
		DROPINCOMPILERLINKERAPIVERSION
	};
	
	*flags = &sFlags;
	*flagsSize = sizeof(sFlags);
	
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDropInName)(const char** dropinName)
{
	static const char* sDropInName = "xpt Linker";
	*dropinName = sDropInName;
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDisplayName)(const char** displayName)
{
	static const char* sDisplayName = "xpt Linker";
	*displayName = sDisplayName;
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetPanelList)(const CWPanelList** panelList)
{
	
	static const char* sPanelName = kXPIDLPanelName;
	static CWPanelList sPanelList = { kCurrentCWPanelListVersion, 1, &sPanelName };
	
	*panelList = &sPanelList;
	
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetTargetList)(const CWTargetList** targetList)
{
	static CWDataType sCPU = '****';
	static CWDataType sOS = '****';
	static CWTargetList sTargetList = { kCurrentCWTargetListVersion, 1, &sCPU, 1, &sOS };
	
	*targetList = &sTargetList;
	
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDefaultMappingList)(const CWExtMapList** defaultMappingList)
{
	static CWExtensionMapping sExtension = { 'MMCH', ".xpt", 0 };
	static CWExtMapList sExtensionMapList = { kCurrentCWExtMapListVersion, 1, &sExtension };
	
	*defaultMappingList = &sExtensionMapList;
	
	return cwNoErr;
}

CWPLUGIN_ENTRY (CWPlugin_GetFamilyList)(const CWFamilyList** familyList)
{
	static CWFamily sFamily = { 'XIDL', "xpidl Settings" };
	static CWFamilyList sFamilyList = { kCurrentCWFamilyListVersion, 0, &sFamily };
	
	*familyList = &sFamilyList;
	
	return cwNoErr;
}

#if CW_USE_PRAGMA_EXPORT
#pragma export off
#endif

#endif
