







































#ifndef _NS_APPLESINGLEENCODER_H_
	#include "nsAppleSingleEncoder.h"
#endif
 
#include <Files.h>
#include <OSUtils.h>
#include "nsAppleSingleDecoder.h" 
#include "MoreFilesExtras.h"
#include "IterateDirectory.h"

nsAppleSingleEncoder::nsAppleSingleEncoder()
{
	mInFile = NULL;
	mOutFile = NULL;
}

nsAppleSingleEncoder::~nsAppleSingleEncoder()
{
}

Boolean
nsAppleSingleEncoder::HasResourceFork(FSSpecPtr aFile)
{
	OSErr	err = noErr;
	Boolean bHasResFork = false;
	short	refnum;
	long	bytes2Read = 1;
	char	buf[2];
	
	err = FSpOpenRF(aFile, fsRdPerm, &refnum);
	if (err == noErr)
	{
		err = FSRead(refnum, &bytes2Read, &buf[0]);
		if (err == noErr)	
			bHasResFork = true;
	}
	
	FSClose(refnum);
	
	return bHasResFork;
}

OSErr
nsAppleSingleEncoder::Encode(FSSpecPtr aFile)
{
	OSErr		err = noErr;
	FSSpec		transient, exists;
	
	
	if (!mOutFile || !mInFile)
	{
		mInFile = aFile;
		mOutFile = aFile;
	}
	
	
	if (!mInFile)
		return paramErr;
		
	
	mTransient = &transient;
	err = FSMakeFSSpec(	mInFile->vRefNum, mInFile->parID, kTransientName, 
						mTransient);
	if (err == noErr) 
		FSpDelete(mTransient);
		
	
	ERR_CHECK( WriteHeader() );
	ERR_CHECK( WriteEntryDescs() );
	ERR_CHECK( WriteEntries() );
	
	
	err = FSMakeFSSpec( mOutFile->vRefNum, mOutFile->parID, mOutFile->name,
						&exists );
	if (err == noErr)
		FSpDelete(mOutFile);
	FSpRename(mTransient, mOutFile->name);

	return err;
}

OSErr
nsAppleSingleEncoder::Encode(FSSpecPtr aInFile, FSSpecPtr aOutFile)
{
	OSErr err = noErr;
	
	
	if (!aInFile || !aOutFile)
		return paramErr;
		
	mInFile = aInFile;
	mOutFile = aOutFile;
	
	err = Encode(NULL);

	return err;
}

pascal void
EncodeDirIterateFilter(const CInfoPBRec * const cpbPtr, Boolean *quitFlag, void *yourDataPtr)
{	
	OSErr					err = noErr;
	FSSpec 					currFSp;
	nsAppleSingleEncoder* 	thisObj = NULL;
	Boolean					isDir = false;
	long					dummy;
	
	
	if (!yourDataPtr || !cpbPtr || !quitFlag)
		return;
	
	*quitFlag = false;
		
	
	thisObj = (nsAppleSingleEncoder*) yourDataPtr;
	thisObj->ReInit();
	
	
	err = FSMakeFSSpec(cpbPtr->hFileInfo.ioVRefNum, cpbPtr->hFileInfo.ioFlParID, 
						cpbPtr->hFileInfo.ioNamePtr, &currFSp);
	if (err == noErr)
	{
		FSpGetDirectoryID(&currFSp, &dummy, &isDir);
		
		
		if (!isDir)
		{
			
			if (nsAppleSingleEncoder::HasResourceFork(&currFSp))
			{
				
				thisObj->Encode(&currFSp);
			}
		}
		else
		{
			
			
			return;
		}
	}
}

OSErr
nsAppleSingleEncoder::EncodeFolder(FSSpecPtr aFolder)
{
	OSErr	err = noErr;
	long	dummy;
	Boolean	isDir = false;
	
	
	if (aFolder)
	{
		FSpGetDirectoryID(aFolder, &dummy, &isDir);
		if (!isDir)
			return dirNFErr;
	}
	
	
	FSpIterateDirectory(aFolder, 0, EncodeDirIterateFilter, (void*)this);
			
	return err;
}

void
nsAppleSingleEncoder::ReInit()
{
	mInFile = NULL;
	mOutFile = NULL;
}

OSErr
nsAppleSingleEncoder::WriteHeader()
{
	OSErr 		err = noErr;
	ASHeader 	hdr;
	int			i;
	long		bytes2Wr;
	
	
	hdr.magicNum = kAppleSingleMagicNum; 
	hdr.versionNum = kAppleSingleVerNum;
	for (i=0; i<16; i++)
		hdr.filler[i] = 0;
	hdr.numEntries = kNumASEntries;		
		









	
	bytes2Wr = sizeof(ASHeader);
	ERR_CHECK( FSpCreate(mTransient, 'MOZZ', '????', 0) );
	ERR_CHECK( FSpOpenDF(mTransient, fsRdWrPerm, &mTransRefNum) );
	
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &hdr) );
	if (bytes2Wr != sizeof(ASHeader))
		err = -1;
		
	return err;
}

OSErr
nsAppleSingleEncoder::WriteEntryDescs()
{
	OSErr 		err = noErr;
	long 		offset = sizeof(ASHeader), bytes2Wr = kNumASEntries*sizeof(ASEntry);
	ASEntry 	entries[kNumASEntries];
	int 		i;
	CInfoPBRec	pb;
	
	ERR_CHECK( FSpGetCatInfo(&pb, mInFile) );
	
	
	entries[0].entryOffset = sizeof(ASHeader) + (sizeof(ASEntry) * kNumASEntries);
	entries[0].entryID = AS_REALNAME;
	entries[0].entryLength = pb.hFileInfo.ioNamePtr[0];
		
	
	entries[1].entryID = AS_FINDERINFO;
	entries[1].entryLength = sizeof(FInfo) + sizeof(FXInfo);
		
	
	entries[2].entryID = AS_FILEDATES;
	entries[2].entryLength = sizeof(ASFileDates);
		
	
	entries[3].entryID = AS_RESOURCE;
	entries[3].entryLength = pb.hFileInfo.ioFlRLgLen;

	
	entries[4].entryID = AS_DATA;
	entries[4].entryLength = pb.hFileInfo.ioFlLgLen;
	
	
	entries[5].entryID = AS_MACINFO;
	entries[5].entryLength = sizeof(ASMacInfo);
	
	for (i=1; i<kNumASEntries; i++) 
	{
		entries[i].entryOffset = entries[i-1].entryOffset + entries[i-1].entryLength;
	}
	
	
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &entries[0]) );
	if (bytes2Wr != (kNumASEntries * sizeof(ASEntry)))
		err = -1;
		
	return err;
}

OSErr
nsAppleSingleEncoder::WriteEntries()
{
	OSErr 			err = noErr;
	long			bytes2Wr;
	DateTimeRec		currTime;
	ASFileDates		asDates;
	unsigned long	currSecs;
	ASMacInfo 		asMacInfo;
	int				i;
	CInfoPBRec		pb;
	char 			name[32];
	
	FSpGetCatInfo(&pb, mInFile);
	
	
	bytes2Wr = pb.hFileInfo.ioNamePtr[0];
	strncpy(name, (char*)(pb.hFileInfo.ioNamePtr+1), bytes2Wr);
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, name) );
	FSpGetCatInfo(&pb, mInFile); 
	if (bytes2Wr != pb.hFileInfo.ioNamePtr[0])
	{
		err = -1;
		goto cleanup;
	}
		
	
	bytes2Wr = sizeof(FInfo);
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &pb.hFileInfo.ioFlFndrInfo) );
	FSpGetCatInfo(&pb, mInFile); 
	if (bytes2Wr != sizeof(FInfo))
	{
		err = -1;
		goto cleanup;
	}
	
	bytes2Wr = sizeof(FXInfo);
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &pb.hFileInfo.ioFlXFndrInfo) );
	FSpGetCatInfo(&pb, mInFile); 
	if (bytes2Wr != sizeof(FXInfo))
	{
		err = -1;
		goto cleanup;
	}
		
	
	GetTime(&currTime);
	DateToSeconds(&currTime, &currSecs);
	FSpGetCatInfo(&pb, mInFile); 
	asDates.create = pb.hFileInfo.ioFlCrDat + kConvertTime;
	asDates.modify = pb.hFileInfo.ioFlMdDat + kConvertTime;
	asDates.backup = pb.hFileInfo.ioFlBkDat + kConvertTime;
	asDates.access = currSecs + kConvertTime;
	bytes2Wr = sizeof(ASFileDates);
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &asDates) );
	FSpGetCatInfo(&pb, mInFile); 
	if (bytes2Wr != sizeof(ASFileDates))
	{
		err = -1;
		goto cleanup;
	}
		
	
	ERR_CHECK( WriteResourceFork() );
	
	
	ERR_CHECK( WriteDataFork() );
	
	
	for (i=0; i<3; i++)
		asMacInfo.filler[i];
	FSpGetCatInfo(&pb, mInFile); 
	asMacInfo.ioFlAttrib = pb.hFileInfo.ioFlAttrib;
	bytes2Wr = sizeof(ASMacInfo);
	ERR_CHECK( FSWrite(mTransRefNum, &bytes2Wr, &asMacInfo) );
	if (bytes2Wr != sizeof(ASMacInfo))
	{
		err = -1;
		goto cleanup;
	}
	
cleanup:
	FSClose(mTransRefNum);
	
	return err;
}

#define BUFSIZE 8192

OSErr
nsAppleSingleEncoder::WriteResourceFork()
{
	OSErr		err = noErr;
	short		refnum;
	long		bytes2Rd, bytes2Wr;
	char 		buf[BUFSIZE];
	
	
	ERR_CHECK( FSpOpenRF(mInFile, fsRdPerm, &refnum) );
	
	while (err != eofErr)
	{
		
		bytes2Rd = BUFSIZE;
		err = FSRead(refnum, &bytes2Rd, buf);
		if (bytes2Rd <= 0)
			goto cleanup;
		if (err == noErr || err == eofErr)
		{		
			
			bytes2Wr = bytes2Rd;
			err = FSWrite(mTransRefNum, &bytes2Wr, buf);
			if (err != noErr || bytes2Wr != bytes2Rd)
				goto cleanup;
		}	
	}
		
cleanup:
	
	FSClose(refnum);
		
	
	if (err == eofErr)
		err = noErr;

	return err;
}

OSErr
nsAppleSingleEncoder::WriteDataFork()
{
	OSErr		err = noErr;
	short		refnum;
	long		bytes2Rd, bytes2Wr;
	char 		buf[BUFSIZE];
	
	
	ERR_CHECK( FSpOpenDF(mInFile, fsRdPerm, &refnum) );
	
	while (err != eofErr)
	{
		
		bytes2Rd = BUFSIZE;
		err = FSRead(refnum, &bytes2Rd, buf);
		if (bytes2Rd <= 0)
			goto cleanup;
		if (err == noErr || err == eofErr)
		{		
			
			bytes2Wr = bytes2Rd;
			err = FSWrite(mTransRefNum, &bytes2Wr, buf);
			if (err != noErr || bytes2Wr != bytes2Rd)
				goto cleanup;
		}	
	}
		
cleanup:
	
	FSClose(refnum);
	
	
	if (err == eofErr)
		err = noErr;
	
	return err;
}

OSErr
nsAppleSingleEncoder::FSpGetCatInfo(CInfoPBRec *pb, FSSpecPtr aFile)
{
	OSErr err = noErr;
	Str31 name;
	
	nsAppleSingleDecoder::PLstrncpy(name, aFile->name, aFile->name[0]);
	name[0] = aFile->name[0]; 

	pb->hFileInfo.ioNamePtr = name;
	pb->hFileInfo.ioVRefNum = aFile->vRefNum;
	pb->hFileInfo.ioDirID = aFile->parID;
	pb->hFileInfo.ioFDirIndex = 0; 
	ERR_CHECK( PBGetCatInfoSync(pb) );

	return err;
}
