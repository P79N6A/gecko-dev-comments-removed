




































#include "nsIServiceManager.h"

#include "nsLocalFile.h" 
#include "nsLocalFileUnicode.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsPrintfCString.h"
#include "nsCRT.h"

#ifdef XP_WIN
#include <string.h>
#endif


void NS_StartupLocalFile()
{
    nsLocalFile::GlobalInit();
}

void NS_ShutdownLocalFile()
{
    nsLocalFile::GlobalShutdown();
}

#if !defined(XP_MACOSX) && !defined(XP_WIN)
NS_IMETHODIMP
nsLocalFile::InitWithFile(nsILocalFile *aFile)
{
    NS_ENSURE_ARG(aFile);
    
    nsCAutoString path;
    aFile->GetNativePath(path);
    if (path.IsEmpty())
        return NS_ERROR_INVALID_ARG;
    return InitWithNativePath(path); 
}
#endif

#define kMaxFilenameLength 255
#define kMaxExtensionLength 100


NS_IMETHODIMP
nsLocalFile::CreateUnique(PRUint32 type, PRUint32 attributes)
{
    nsresult rv = Create(type, attributes);
    if (rv != NS_ERROR_FILE_ALREADY_EXISTS)
        return rv;

    nsCAutoString leafName; 
    rv = GetNativeLeafName(leafName);
    if (NS_FAILED(rv))
        return rv;

    const char* lastDot = strrchr(leafName.get(), '.');
    char suffix[kMaxExtensionLength] = "";
    if (lastDot)
    {
        PL_strncpyz(suffix, lastDot, sizeof(suffix)); 
        leafName.SetLength(lastDot - leafName.get()); 
    }

    PRUint32 maxRootLength = (kMaxFilenameLength - 4) - strlen(suffix) - 1;

    if (leafName.Length() > maxRootLength)
        leafName.SetLength(maxRootLength);

    for (int indx = 1; indx < 10000; indx++)
    {
        
        SetNativeLeafName(leafName +
                          nsPrintfCString("-%d", indx) +
                          nsDependentCString(suffix));

        rv = Create(type, attributes);
        if (NS_SUCCEEDED(rv) || rv != NS_ERROR_FILE_ALREADY_EXISTS) 
            return rv;
    }
 
    
    return NS_ERROR_FILE_TOO_BIG;
}

#if defined(XP_WIN) || defined(XP_OS2)
static const PRUnichar kPathSeparatorChar       = '\\';
#elif defined(XP_UNIX) || defined(XP_BEOS)
static const PRUnichar kPathSeparatorChar       = '/';
#else
#error Need to define file path separator for your platform
#endif

static PRInt32 SplitPath(PRUnichar *path, PRUnichar **nodeArray, PRInt32 arrayLen)
{
    if (*path == 0)
      return 0;

    PRUnichar **nodePtr = nodeArray;
    if (*path == kPathSeparatorChar)
      path++;    
    *nodePtr++ = path;
    
    for (PRUnichar *cp = path; *cp != 0; cp++) {
      if (*cp == kPathSeparatorChar) {
        *cp++ = 0;
        if (*cp == 0)
          break;
        if (nodePtr - nodeArray >= arrayLen)
          return -1;
        *nodePtr++ = cp;
      }
    }
    return nodePtr - nodeArray;
}

 
NS_IMETHODIMP
nsLocalFile::GetRelativeDescriptor(nsILocalFile *fromFile, nsACString& _retval)
{
    NS_ENSURE_ARG_POINTER(fromFile);
    const PRInt32 kMaxNodesInPath = 32;

    
    
    
        
    nsresult rv;
    _retval.Truncate(0);

    nsAutoString thisPath, fromPath;
    PRUnichar *thisNodes[kMaxNodesInPath], *fromNodes[kMaxNodesInPath];
    PRInt32  thisNodeCnt, fromNodeCnt, nodeIndex;
    
    rv = GetPath(thisPath);
    if (NS_FAILED(rv))
        return rv;
    rv = fromFile->GetPath(fromPath);
    if (NS_FAILED(rv))
        return rv;

    
    PRUnichar *thisPathPtr; thisPath.BeginWriting(thisPathPtr);
    PRUnichar *fromPathPtr; fromPath.BeginWriting(fromPathPtr);
    
    thisNodeCnt = SplitPath(thisPathPtr, thisNodes, kMaxNodesInPath);
    fromNodeCnt = SplitPath(fromPathPtr, fromNodes, kMaxNodesInPath);
    if (thisNodeCnt < 0 || fromNodeCnt < 0)
      return NS_ERROR_FAILURE;
    
    for (nodeIndex = 0; nodeIndex < thisNodeCnt && nodeIndex < fromNodeCnt; ++nodeIndex) {
#ifdef XP_WIN
      if (_wcsicmp(thisNodes[nodeIndex], fromNodes[nodeIndex]))
        break;
#else
      if (nsCRT::strcmp(thisNodes[nodeIndex], fromNodes[nodeIndex]))
        break;
#endif
    }
    
    PRInt32 branchIndex = nodeIndex;
    for (nodeIndex = branchIndex; nodeIndex < fromNodeCnt; nodeIndex++) 
      _retval.AppendLiteral("../");
    for (nodeIndex = branchIndex; nodeIndex < thisNodeCnt; nodeIndex++) {
      NS_ConvertUTF16toUTF8 nodeStr(thisNodes[nodeIndex]);
      _retval.Append(nodeStr);
      if (nodeIndex + 1 < thisNodeCnt)
        _retval.Append('/');
    }
        
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetRelativeDescriptor(nsILocalFile *fromFile, const nsACString& relativeDesc)
{
    NS_NAMED_LITERAL_CSTRING(kParentDirStr, "../");
 
    nsCOMPtr<nsIFile> targetFile;
    nsresult rv = fromFile->Clone(getter_AddRefs(targetFile));
    if (NS_FAILED(rv))
        return rv;

    
    
    

    nsCString::const_iterator strBegin, strEnd;
    relativeDesc.BeginReading(strBegin);
    relativeDesc.EndReading(strEnd);
    
    nsCString::const_iterator nodeBegin(strBegin), nodeEnd(strEnd);
    nsCString::const_iterator pos(strBegin);
    
    nsCOMPtr<nsIFile> parentDir;
    while (FindInReadable(kParentDirStr, nodeBegin, nodeEnd)) {
        rv = targetFile->GetParent(getter_AddRefs(parentDir));
        if (NS_FAILED(rv))
            return rv;
        if (!parentDir)
            return NS_ERROR_FILE_UNRECOGNIZED_PATH;
        targetFile = parentDir;

        nodeBegin = nodeEnd;
        pos = nodeEnd;
        nodeEnd = strEnd;
    }

    nodeBegin = nodeEnd = pos;
    while (nodeEnd != strEnd) {
      FindCharInReadable('/', nodeEnd, strEnd);
      targetFile->Append(NS_ConvertUTF8toUTF16(Substring(nodeBegin, nodeEnd)));
      if (nodeEnd != strEnd) 
        ++nodeEnd;
      nodeBegin = nodeEnd;
    }

    nsCOMPtr<nsILocalFile> targetLocalFile(do_QueryInterface(targetFile));
    return InitWithFile(targetLocalFile);
}
