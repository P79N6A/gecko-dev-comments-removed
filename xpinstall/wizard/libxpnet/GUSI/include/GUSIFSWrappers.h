








































#ifndef _GUSIFSWrappers_
#define _GUSIFSWrappers_

#ifdef GUSI_SOURCE

#include <Files.h>

#include <sys/cdefs.h>

#include "GUSIFileSpec.h"

__BEGIN_DECLS

OSErr GUSIFSOpenDriver(StringPtr name, short * refNum);

OSErr GUSIFSGetFInfo(const FSSpec * spec, FInfo * info);
OSErr GUSIFSSetFInfo(const FSSpec * spec, const FInfo * info);

OSErr GUSIFSOpenDF(const FSSpec * spec, char permission, short * refNum);
OSErr GUSIFSOpenRF(const FSSpec * spec, char permission, short * refNum);

OSErr GUSIFSGetVolParms(short vRefNum, GetVolParmsInfoBuffer * volParms);

OSErr GUSIFSCreate(const FSSpec * spec, OSType creator, OSType type, ScriptCode script);

OSErr GUSIFSDelete(const FSSpec * spec);

OSErr GUSIFSDirCreate(const FSSpec * spec);

OSErr GUSIFSSetFLock(const FSSpec * spec);
OSErr GUSIFSRstFLock(const FSSpec * spec);

OSErr GUSIFSRename(const FSSpec * spec, ConstStr255Param newname);

OSErr GUSIFSCatMove(const FSSpec * spec, const FSSpec * dest);

OSErr GUSIFSMoveRename(const FSSpec * spec, const FSSpec * dest);
__END_DECLS
#ifdef __cplusplus

OSErr GUSIFSGetCatInfo(GUSIIOPBWrapper<GUSICatInfo> * info);
OSErr GUSIFSSetCatInfo(GUSIIOPBWrapper<GUSICatInfo> * info);

OSErr GUSIFSGetFCBInfo(GUSIIOPBWrapper<FCBPBRec> * fcb);

OSErr GUSIFSGetVInfo(GUSIIOPBWrapper<ParamBlockRec> * pb);
OSErr GUSIFSHGetVInfo(GUSIIOPBWrapper<HParamBlockRec> * pb);




OSErr GUSIFSOpen(GUSIIOPBWrapper<ParamBlockRec> * pb);

OSErr GUSIFSHGetFInfo(GUSIIOPBWrapper<HParamBlockRec> * pb);
OSErr GUSIFSHSetFInfo(GUSIIOPBWrapper<HParamBlockRec> * pb);

OSErr GUSIFSHGetVolParms(GUSIIOPBWrapper<HParamBlockRec> * pb);

OSErr GUSIFSCreate(const FSSpec * spec);

OSErr GUSIFSCatMove(const FSSpec * spec, long dest);
#endif

#endif 

#endif 
