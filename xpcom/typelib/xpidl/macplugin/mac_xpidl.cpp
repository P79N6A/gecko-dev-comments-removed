












































#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <new.h>
#include <setjmp.h>


#include <Files.h>
#include <Errors.h>
#include <Strings.h>

#include "FullPath.h"
#include "MoreFilesExtras.h"


#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"


#include "mac_xpidl.h"
#include "mac_console.h"
#include "mac_strings.h"
#include "mac_xpidl_panel.h"


static CWResult	Compile(CWPluginContext context);
static CWResult	Disassemble(CWPluginContext context);
static CWResult	LocateFile(CWPluginContext context, const char* filename, FSSpec& file);


extern jmp_buf exit_jump;
extern int exit_status;


CWPluginContext gPluginContext;


static CWFileSpec gSourceFile;
static char* gSourcePath = NULL;
static CWFileSpec gOutputFile;

extern "C" {
pascal short xpidl_compiler(CWPluginContext context);
int xpidl_main(int argc, char* argv[]);
int xptdump_main(int argc, char* argv[]);

FILE * FSp_fopen(ConstFSSpecPtr spec, const char * open_mode);
}

pascal short xpidl_compiler(CWPluginContext context)
{
	long request;
	if (CWGetPluginRequest(context, &request) != cwNoErr)
		return cwErrRequestFailed;
	
	gPluginContext = context;
	short result = cwNoErr;
	
	
	switch (request) {
	case reqInitCompiler:
		
		break;
		
	case reqTermCompiler:
		
		break;
		
	case reqCompile:
		
		result = Compile(context);
		break;
	
	case reqCompDisassemble:
		
		result = Disassemble(context);
		break;
	
	default:
		result = cwErrRequestFailed;
		break;
	}
	
	
	CWDonePluginRequest(context, result);
	
	
	return (result);
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

static CWResult GetSettings(CWPluginContext context, XPIDLSettings& settings)
{
	CWMemHandle	settingsHand;
	CWResult err = CWGetNamedPreferences(context, kXPIDLPanelName, &settingsHand);
	if (!CWSUCCESS(err))
		return (err);
	
	XPIDLSettings* settingsPtr = NULL;
	err = CWLockMemHandle(context, settingsHand, false, (void**)&settingsPtr);
	if (!CWSUCCESS(err))
		return (err);
	
	settings = *settingsPtr;
	
	err = CWUnlockMemHandle(context, settingsHand);
	if (!CWSUCCESS(err))
		return (err);

	return noErr;
}

static CWResult	Compile(CWPluginContext context)
{
	CWResult err = CWGetMainFileSpec(context, &gSourceFile);
	if (!CWSUCCESS(err))
		return (err);

	long fileNum;
	err = CWGetMainFileNumber(context, &fileNum);
	if (!CWSUCCESS(err))
		return (err);

	
	gSourcePath = p2c_strdup(gSourceFile.name);
	if (gSourcePath == NULL)
		return cwErrOutOfMemory;
	
	
	XPIDLSettings settings = { kXPIDLSettingsVersion, kXPIDLModeHeader, false, false };
	GetSettings(context, settings);

#if 0	
	
	
	if (settings.mode == kXPIDLModeTypelib)
		err = CWGetSuggestedObjectFileSpec(context, fileNum, &gOutputFile);
	else
		err = CWGetOutputFileDirectory(gPluginContext, &gOutputFile);
#else
	
	err = CWGetSuggestedObjectFileSpec(context, fileNum, &gOutputFile);
#endif
	if (!CWSUCCESS(err))
		return (err);
	
	int argc = 3;
	char* modes[] = { "header", "java", "typelib", "doc" };
	char* argv[] = { "xpidl", "-m", modes[settings.mode - 1], NULL, NULL, NULL, NULL, };
	if (settings.warnings) argv[argc++] = "-w";
	if (settings.verbose) argv[argc++] = "-v";
	argv[argc++] = gSourcePath;
	
	if (setjmp(exit_jump) == 0) {
		if (xpidl_main(argc, argv) != 0)
			err = cwErrRequestFailed;
	} else {
		
		if (exit_status != 0)
			err = cwErrRequestFailed;
	}

	
	
	
	
	
	
	if (err == cwNoErr) {
		CWObjectData objectData;
		BlockZero(&objectData, sizeof(objectData));
		
		
		long dataSize, rsrcSize;
		if (FSpGetFileSize(&gOutputFile, &dataSize, &rsrcSize) == noErr)
			objectData.idatasize = dataSize;
		
		
		objectData.objectfile = &gOutputFile;
		
		err = CWStoreObjectData(context, fileNum, &objectData);
	} else {
		
		if (gOutputFile.name[0] != 0) {
			::FSpDelete(&gOutputFile);
		}
	}

	delete[] gSourcePath;
	gSourcePath = NULL;

	return (err);
}

static CWResult	Disassemble(CWPluginContext context)
{
	
	return noErr;
}

static CWResult	LocateFile(CWPluginContext context, const char* filename, FSSpec& file)
{
	
	CWFileInfo fileinfo;
	BlockZero(&fileinfo, sizeof(fileinfo));
	
	fileinfo.fullsearch = true;
	fileinfo.suppressload = true;
	fileinfo.dependencyType = cwNormalDependency;
	fileinfo.isdependentoffile = kCurrentCompiledFile;

	
	CWResult err = CWFindAndLoadFile(context, filename, &fileinfo);
	if (err == cwNoErr) {
		file = fileinfo.filespec;
	} else if (err == cwErrFileNotFound) {
		char errmsg[200];
		sprintf(errmsg, "Can't locate file \"%s\".", filename);
		CWResult callbackResult = CWReportMessage(context, 0, errmsg, 0, messagetypeError, 0);
	}
	
	return (err);
}








FILE* std::fopen(const char* filename, const char *mode)
{
	FSSpec filespec;
	CWResult err = noErr;
	do {
		if (filename == gSourcePath || strcmp(filename, gSourcePath) == 0) {
			
			filespec = gSourceFile;
		} else if (mode[0] == 'w') {
			
			c2p_strcpy(filespec.name, filename);
			filespec.vRefNum = gOutputFile.vRefNum;
			filespec.parID = gOutputFile.parID;
			c2p_strcpy(gOutputFile.name, filename);
		} else {
			
			err = LocateFile(gPluginContext, filename, filespec);
		}
	} while (0);
	
	return (err == noErr ? FSp_fopen(&filespec, mode) : NULL);
}





size_t mac_get_file_length(const char* filename)
{
	long dataSize= 0, rsrcSize = 0;
	FSSpec filespec;
	if (CWGetOutputFileDirectory(gPluginContext, &filespec) != noErr)
		return 0;
	c2p_strcpy(filespec.name, filename);
	if (FSpGetFileSize(&filespec, &dataSize, &rsrcSize) != noErr)
		return 0;
	return dataSize;
}

void mac_warning(const char* warning_message)
{
	CWReportMessage(gPluginContext, 0, warning_message, 0, messagetypeWarning, 0);
}

void mac_error(const char* error_message)
{
	CWReportMessage(gPluginContext, 0, error_message, 0, messagetypeError, 0);
}



#if CW_USE_PRAGMA_EXPORT
#pragma export on
#endif

CWPLUGIN_ENTRY(CWPlugin_GetDropInFlags)(const DropInFlags** flags, long* flagsSize)
{
	static const DropInFlags sFlags = {
		kCurrentDropInFlagsVersion,
		CWDROPINCOMPILERTYPE,
		DROPINCOMPILERLINKERAPIVERSION,
		(kGeneratescode |  kCompMultiTargAware | kCompAlwaysReload),
		Lang_MISC,
		DROPINCOMPILERLINKERAPIVERSION
	};
	
	*flags = &sFlags;
	*flagsSize = sizeof(sFlags);
	
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDropInName)(const char** dropinName)
{
	static const char* sDropInName = "xpidl";
	
	*dropinName = sDropInName;
	
	return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDisplayName)(const char** displayName)
{
	static const char* sDisplayName = "xpidl";
	
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
	static CWExtensionMapping sExtension = { 'TEXT', ".idl", 0 };
	static CWExtMapList sExtensionMapList = { kCurrentCWExtMapListVersion, 1, &sExtension };
	
	*defaultMappingList = &sExtensionMapList;
	
	return cwNoErr;
}

#if CW_USE_PRAGMA_EXPORT
#pragma export off
#endif
