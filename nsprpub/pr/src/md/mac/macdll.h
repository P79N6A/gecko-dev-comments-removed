




































#ifndef macdll_h__
#define macdll_h__

#include "prtypes.h"

OSErr GetNamedFragmentOffsets(const FSSpec *fileSpec, const char* fragmentName,
							UInt32 *outOffset, UInt32 *outLength);
OSErr GetIndexedFragmentOffsets(const FSSpec *fileSpec, UInt32 fragmentIndex,
							UInt32 *outOffset, UInt32 *outLength, char **outFragmentName);
							
OSErr NSLoadNamedFragment(const FSSpec *fileSpec, const char* fragmentName, CFragConnectionID *outConnectionID);
OSErr NSLoadIndexedFragment(const FSSpec *fileSpec, PRUint32 fragmentIndex,
							char** outFragName, CFragConnectionID *outConnectionID);


OSErr NSGetSharedLibrary(Str255 inLibName, CFragConnectionID* outID, Ptr* outMainAddr);
OSErr NSFindSymbol(CFragConnectionID inID, Str255 inSymName,
							Ptr*	outMainAddr, CFragSymbolClass *outSymClass);

#endif 
