







































#include <sys/types.h>
#include <sys/stat.h>

#include "nsOSHelperAppService.h"
#include "nsMIMEInfoUnix.h"
#ifdef MOZ_WIDGET_GTK2
#include "nsGNOMERegistry.h"
#endif
#include "nsISupports.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsXPIDLString.h"
#include "nsIURL.h"
#include "nsIFileStreams.h"
#include "nsILineInputStream.h"
#include "nsILocalFile.h"
#include "nsIProcess.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsNetCID.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsHashtable.h"
#include "nsCRT.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "prenv.h"      
#include "nsAutoPtr.h"

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, PR_LOG_DEBUG)

static nsresult
FindSemicolon(nsAString::const_iterator& aSemicolon_iter,
              const nsAString::const_iterator& aEnd_iter);
static nsresult
ParseMIMEType(const nsAString::const_iterator& aStart_iter,
              nsAString::const_iterator& aMajorTypeStart,
              nsAString::const_iterator& aMajorTypeEnd,
              nsAString::const_iterator& aMinorTypeStart,
              nsAString::const_iterator& aMinorTypeEnd,
              const nsAString::const_iterator& aEnd_iter);

inline PRBool
IsNetscapeFormat(const nsACString& aBuffer);

nsOSHelperAppService::nsOSHelperAppService() : nsExternalHelperAppService()
{
  mode_t mask = umask(0777);
  umask(mask);
  mPermissions = 0666 & ~mask;
}

nsOSHelperAppService::~nsOSHelperAppService()
{}







nsresult
nsOSHelperAppService::UnescapeCommand(const nsAString& aEscapedCommand,
                                      const nsAString& aMajorType,
                                      const nsAString& aMinorType,
                                      nsHashtable& aTypeOptions,
                                      nsACString& aUnEscapedCommand) {
  LOG(("-- UnescapeCommand"));
  LOG(("Command to escape: '%s'\n",
       NS_LossyConvertUTF16toASCII(aEscapedCommand).get()));
  
  
  LOG(("UnescapeCommand really needs some work -- it should actually do some unescaping\n"));

  CopyUTF16toUTF8(aEscapedCommand, aUnEscapedCommand);
  LOG(("Escaped command: '%s'\n",
       PromiseFlatCString(aUnEscapedCommand).get()));
  return NS_OK;
}





static nsresult
FindSemicolon(nsAString::const_iterator& aSemicolon_iter,
              const nsAString::const_iterator& aEnd_iter) {
  PRBool semicolonFound = PR_FALSE;
  while (aSemicolon_iter != aEnd_iter && !semicolonFound) {
    switch(*aSemicolon_iter) {
    case '\\':
      aSemicolon_iter.advance(2);
      break;
    case ';':
      semicolonFound = PR_TRUE;
      break;
    default:
      ++aSemicolon_iter;
      break;
    }
  }
  return NS_OK;
}

static nsresult
ParseMIMEType(const nsAString::const_iterator& aStart_iter,
              nsAString::const_iterator& aMajorTypeStart,
              nsAString::const_iterator& aMajorTypeEnd,
              nsAString::const_iterator& aMinorTypeStart,
              nsAString::const_iterator& aMinorTypeEnd,
              const nsAString::const_iterator& aEnd_iter) {
  nsAString::const_iterator iter(aStart_iter);
  
  
  while (iter != aEnd_iter && nsCRT::IsAsciiSpace(*iter)) {
    ++iter;
  }

  if (iter == aEnd_iter) {
    return NS_ERROR_INVALID_ARG;
  }
  
  aMajorTypeStart = iter;

  
  while (iter != aEnd_iter && *iter != '/') {
    ++iter;
  }
  
  if (iter == aEnd_iter) {
    return NS_ERROR_INVALID_ARG;
  }

  aMajorTypeEnd = iter;
  
  
  ++iter;

  if (iter == aEnd_iter) {
    return NS_ERROR_INVALID_ARG;
  }

  aMinorTypeStart = iter;

  
  while (iter != aEnd_iter && !nsCRT::IsAsciiSpace(*iter) && *iter != ';') {
    ++iter;
  }

  aMinorTypeEnd = iter;

  return NS_OK;
}


nsresult
nsOSHelperAppService::GetFileLocation(const char* aPrefName,
                                      const char* aEnvVarName,
                                      PRUnichar** aFileLocation) {
  LOG(("-- GetFileLocation.  Pref: '%s'  EnvVar: '%s'\n",
       aPrefName,
       aEnvVarName));
  NS_PRECONDITION(aPrefName, "Null pref name passed; don't do that!");
  
  nsresult rv;
  *aFileLocation = nsnull;
  




  nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  



  nsCOMPtr<nsISupportsString> prefFileName;
  PRBool isUserPref = PR_FALSE;
  prefBranch->PrefHasUserValue(aPrefName, &isUserPref);
  if (isUserPref) {
    rv = prefBranch->GetComplexValue(aPrefName,
                                     NS_GET_IID(nsISupportsString),
                                     getter_AddRefs(prefFileName));
    if (NS_SUCCEEDED(rv)) {
      return prefFileName->ToString(aFileLocation);
    }
  }

  if (aEnvVarName && *aEnvVarName) {
    char* prefValue = PR_GetEnv(aEnvVarName);
    if (prefValue && *prefValue) {
      
      
      
      
      nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = file->InitWithNativePath(nsDependentCString(prefValue));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString unicodePath;
      rv = file->GetPath(unicodePath);
      NS_ENSURE_SUCCESS(rv, rv);
      
      *aFileLocation = ToNewUnicode(unicodePath);
      if (!*aFileLocation)
        return NS_ERROR_OUT_OF_MEMORY;
      return NS_OK;
    }
  }
  
  rv = prefBranch->GetComplexValue(aPrefName,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(prefFileName));
  if (NS_SUCCEEDED(rv)) {
    return prefFileName->ToString(aFileLocation);
  }
  
  return rv;
}





nsresult
nsOSHelperAppService::LookUpTypeAndDescription(const nsAString& aFileExtension,
                                               nsAString& aMajorType,
                                               nsAString& aMinorType,
                                               nsAString& aDescription,
                                               PRBool aUserData) {
  LOG(("-- LookUpTypeAndDescription for extension '%s'\n",
       NS_LossyConvertUTF16toASCII(aFileExtension).get()));
  nsresult rv = NS_OK;
  nsXPIDLString mimeFileName;

  const char* filenamePref = aUserData ?
    "helpers.private_mime_types_file" : "helpers.global_mime_types_file";
  
  rv = GetFileLocation(filenamePref,
                       nsnull,
                       getter_Copies(mimeFileName));
  if (NS_SUCCEEDED(rv) && !mimeFileName.IsEmpty()) {
    rv = GetTypeAndDescriptionFromMimetypesFile(mimeFileName,
                                                aFileExtension,
                                                aMajorType,
                                                aMinorType,
                                                aDescription);
  } else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }

  return rv;
}

inline PRBool
IsNetscapeFormat(const nsACString& aBuffer) {
  return StringBeginsWith(aBuffer, NS_LITERAL_CSTRING("#--Netscape Communications Corporation MIME Information")) ||
         StringBeginsWith(aBuffer, NS_LITERAL_CSTRING("#--MCOM MIME Information"));
}







nsresult
nsOSHelperAppService::CreateInputStream(const nsAString& aFilename,
                                        nsIFileInputStream ** aFileInputStream,
                                        nsILineInputStream ** aLineInputStream,
                                        nsACString& aBuffer,
                                        PRBool * aNetscapeFormat,
                                        PRBool * aMore) {
  LOG(("-- CreateInputStream"));
  nsresult rv = NS_OK;

  nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  rv = file->InitWithPath(aFilename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFileInputStream> fileStream(do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  rv = fileStream->Init(file, -1, -1, PR_FALSE);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILineInputStream> lineStream(do_QueryInterface(fileStream, &rv));

  if (NS_FAILED(rv)) {
    LOG(("Interface trouble in stream land!"));
    return rv;
  }

  rv = lineStream->ReadLine(aBuffer, aMore);
  if (NS_FAILED(rv)) {
    fileStream->Close();
    return rv;
  }

  *aNetscapeFormat = IsNetscapeFormat(aBuffer);

  *aFileInputStream = fileStream;
  NS_ADDREF(*aFileInputStream);
  *aLineInputStream = lineStream;
  NS_ADDREF(*aLineInputStream);

  return NS_OK;
}




nsresult
nsOSHelperAppService::GetTypeAndDescriptionFromMimetypesFile(const nsAString& aFilename,
                                                             const nsAString& aFileExtension,
                                                             nsAString& aMajorType,
                                                             nsAString& aMinorType,
                                                             nsAString& aDescription) {
  LOG(("-- GetTypeAndDescriptionFromMimetypesFile\n"));
  LOG(("Getting type and description from types file '%s'\n",
       NS_LossyConvertUTF16toASCII(aFilename).get()));
  LOG(("Using extension '%s'\n",
       NS_LossyConvertUTF16toASCII(aFileExtension).get()));
  nsresult rv = NS_OK;
  nsCOMPtr<nsIFileInputStream> mimeFile;
  nsCOMPtr<nsILineInputStream> mimeTypes;
  PRBool netscapeFormat;
  nsAutoString buf;
  nsCAutoString cBuf;
  PRBool more = PR_FALSE;
  rv = CreateInputStream(aFilename, getter_AddRefs(mimeFile), getter_AddRefs(mimeTypes),
                         cBuf, &netscapeFormat, &more);

  if (NS_FAILED(rv)) {
    return rv;
  }
  
  nsAutoString extensions;
  nsString entry;
  entry.SetCapacity(100);
  nsAString::const_iterator majorTypeStart, majorTypeEnd,
                            minorTypeStart, minorTypeEnd,
                            descriptionStart, descriptionEnd;

  do {
    CopyASCIItoUTF16(cBuf, buf);
    
    

    
    if (!buf.IsEmpty() && buf.First() != '#') {
      entry.Append(buf);
      if (entry.Last() == '\\') {
        entry.Truncate(entry.Length() - 1);
        entry.Append(PRUnichar(' '));  
      } else {  
        LOG(("Current entry: '%s'\n",
             NS_LossyConvertUTF16toASCII(entry).get()));
        if (netscapeFormat) {
          rv = ParseNetscapeMIMETypesEntry(entry,
                                           majorTypeStart, majorTypeEnd,
                                           minorTypeStart, minorTypeEnd,
                                           extensions,
                                           descriptionStart, descriptionEnd);
          if (NS_FAILED(rv)) {
            
            
            
            LOG(("Bogus entry; trying 'normal' mode\n"));
            rv = ParseNormalMIMETypesEntry(entry,
                                           majorTypeStart, majorTypeEnd,
                                           minorTypeStart, minorTypeEnd,
                                           extensions,
                                           descriptionStart, descriptionEnd);
          }
        } else {
          rv = ParseNormalMIMETypesEntry(entry,
                                         majorTypeStart, majorTypeEnd,
                                         minorTypeStart, minorTypeEnd,
                                         extensions,
                                         descriptionStart, descriptionEnd);
          if (NS_FAILED(rv)) {
            
            
            
            LOG(("Bogus entry; trying 'Netscape' mode\n"));
            rv = ParseNetscapeMIMETypesEntry(entry,
                                             majorTypeStart, majorTypeEnd,
                                             minorTypeStart, minorTypeEnd,
                                             extensions,
                                             descriptionStart, descriptionEnd);
          }
        }

        if (NS_SUCCEEDED(rv)) { 
          nsAString::const_iterator start, end;
          extensions.BeginReading(start);
          extensions.EndReading(end);
          nsAString::const_iterator iter(start);

          while (start != end) {
            FindCharInReadable(',', iter, end);
            if (Substring(start, iter).Equals(aFileExtension,
                                              nsCaseInsensitiveStringComparator())) {
              
              aMajorType.Assign(Substring(majorTypeStart, majorTypeEnd));
              aMinorType.Assign(Substring(minorTypeStart, minorTypeEnd));
              aDescription.Assign(Substring(descriptionStart, descriptionEnd));
              mimeFile->Close();
              return NS_OK;
            }
            if (iter != end) {
              ++iter;
            }
            start = iter;
          }
        } else {
          LOG(("Failed to parse entry: %s\n", NS_LossyConvertUTF16toASCII(entry).get()));
        }
        
        entry.Truncate();
      }
    }
    if (!more) {
      rv = NS_ERROR_NOT_AVAILABLE;
      break;
    }
    
    rv = mimeTypes->ReadLine(cBuf, &more);
  } while (NS_SUCCEEDED(rv));

  mimeFile->Close();
  return rv;
}




nsresult
nsOSHelperAppService::LookUpExtensionsAndDescription(const nsAString& aMajorType,
                                                     const nsAString& aMinorType,
                                                     nsAString& aFileExtensions,
                                                     nsAString& aDescription) {
  LOG(("-- LookUpExtensionsAndDescription for type '%s/%s'\n",
       NS_LossyConvertUTF16toASCII(aMajorType).get(),
       NS_LossyConvertUTF16toASCII(aMinorType).get()));
  nsresult rv = NS_OK;
  nsXPIDLString mimeFileName;

  rv = GetFileLocation("helpers.private_mime_types_file",
                       nsnull,
                       getter_Copies(mimeFileName));
  if (NS_SUCCEEDED(rv) && !mimeFileName.IsEmpty()) {
    rv = GetExtensionsAndDescriptionFromMimetypesFile(mimeFileName,
                                                      aMajorType,
                                                      aMinorType,
                                                      aFileExtensions,
                                                      aDescription);
  } else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }
  if (NS_FAILED(rv) || aFileExtensions.IsEmpty()) {
    rv = GetFileLocation("helpers.global_mime_types_file",
                         nsnull,
                         getter_Copies(mimeFileName));
    if (NS_SUCCEEDED(rv) && !mimeFileName.IsEmpty()) {
      rv = GetExtensionsAndDescriptionFromMimetypesFile(mimeFileName,
                                                        aMajorType,
                                                        aMinorType,
                                                        aFileExtensions,
                                                        aDescription);
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
  }
  return rv;
}




nsresult
nsOSHelperAppService::GetExtensionsAndDescriptionFromMimetypesFile(const nsAString& aFilename,
                                                                   const nsAString& aMajorType,
                                                                   const nsAString& aMinorType,
                                                                   nsAString& aFileExtensions,
                                                                   nsAString& aDescription) {
  LOG(("-- GetExtensionsAndDescriptionFromMimetypesFile\n"));
  LOG(("Getting extensions and description from types file '%s'\n",
       NS_LossyConvertUTF16toASCII(aFilename).get()));
  LOG(("Using type '%s/%s'\n",
       NS_LossyConvertUTF16toASCII(aMajorType).get(),
       NS_LossyConvertUTF16toASCII(aMinorType).get()));

  nsresult rv = NS_OK;
  nsCOMPtr<nsIFileInputStream> mimeFile;
  nsCOMPtr<nsILineInputStream> mimeTypes;
  PRBool netscapeFormat;
  nsCAutoString cBuf;
  nsAutoString buf;
  PRBool more = PR_FALSE;
  rv = CreateInputStream(aFilename, getter_AddRefs(mimeFile), getter_AddRefs(mimeTypes),
                         cBuf, &netscapeFormat, &more);

  if (NS_FAILED(rv)) {
    return rv;
  }
  
  nsAutoString extensions;
  nsString entry;
  entry.SetCapacity(100);
  nsAString::const_iterator majorTypeStart, majorTypeEnd,
                            minorTypeStart, minorTypeEnd,
                            descriptionStart, descriptionEnd;
  
  do {
    CopyASCIItoUTF16(cBuf, buf);
    
    

    
    if (!buf.IsEmpty() && buf.First() != '#') {
      entry.Append(buf);
      if (entry.Last() == '\\') {
        entry.Truncate(entry.Length() - 1);
        entry.Append(PRUnichar(' '));  
      } else {  
        LOG(("Current entry: '%s'\n",
             NS_LossyConvertUTF16toASCII(entry).get()));
        if (netscapeFormat) {
          rv = ParseNetscapeMIMETypesEntry(entry,
                                           majorTypeStart, majorTypeEnd,
                                           minorTypeStart, minorTypeEnd,
                                           extensions,
                                           descriptionStart, descriptionEnd);
          
          if (NS_FAILED(rv)) {
            
            
            
            LOG(("Bogus entry; trying 'normal' mode\n"));
            rv = ParseNormalMIMETypesEntry(entry,
                                           majorTypeStart, majorTypeEnd,
                                           minorTypeStart, minorTypeEnd,
                                           extensions,
                                           descriptionStart, descriptionEnd);
          }
        } else {
          rv = ParseNormalMIMETypesEntry(entry,
                                         majorTypeStart, majorTypeEnd,
                                         minorTypeStart,
                                         minorTypeEnd, extensions,
                                         descriptionStart, descriptionEnd);
          
          if (NS_FAILED(rv)) {
            
            
            
            LOG(("Bogus entry; trying 'Netscape' mode\n"));
            rv = ParseNetscapeMIMETypesEntry(entry,
                                             majorTypeStart, majorTypeEnd,
                                             minorTypeStart, minorTypeEnd,
                                             extensions,
                                             descriptionStart, descriptionEnd);
          }
        }
        
        if (NS_SUCCEEDED(rv) &&
            Substring(majorTypeStart,
                      majorTypeEnd).Equals(aMajorType,
                                           nsCaseInsensitiveStringComparator())&&
            Substring(minorTypeStart,
                      minorTypeEnd).Equals(aMinorType,
                                           nsCaseInsensitiveStringComparator())) {
          
          aFileExtensions.Assign(extensions);
          aDescription.Assign(Substring(descriptionStart, descriptionEnd));
          mimeFile->Close();
          return NS_OK;
        } else if (NS_FAILED(rv)) {
          LOG(("Failed to parse entry: %s\n", NS_LossyConvertUTF16toASCII(entry).get()));
        }
        
        entry.Truncate();
      }
    }
    if (!more) {
      rv = NS_ERROR_NOT_AVAILABLE;
      break;
    }
    
    rv = mimeTypes->ReadLine(cBuf, &more);
  } while (NS_SUCCEEDED(rv));

  mimeFile->Close();
  return rv;
}












nsresult
nsOSHelperAppService::ParseNetscapeMIMETypesEntry(const nsAString& aEntry,
                                                  nsAString::const_iterator& aMajorTypeStart,
                                                  nsAString::const_iterator& aMajorTypeEnd,
                                                  nsAString::const_iterator& aMinorTypeStart,
                                                  nsAString::const_iterator& aMinorTypeEnd,
                                                  nsAString& aExtensions,
                                                  nsAString::const_iterator& aDescriptionStart,
                                                  nsAString::const_iterator& aDescriptionEnd) {
  LOG(("-- ParseNetscapeMIMETypesEntry\n"));
  NS_ASSERTION(!aEntry.IsEmpty(), "Empty Netscape MIME types entry being parsed.");
  
  nsAString::const_iterator start_iter, end_iter, match_start, match_end;

  aEntry.BeginReading(start_iter);
  aEntry.EndReading(end_iter);
  
  
  do {
    --end_iter;
  } while (end_iter != start_iter &&
           nsCRT::IsAsciiSpace(*end_iter));
  
  
  if (*end_iter != '"')
    ++end_iter;
  match_start = start_iter;
  match_end = end_iter;
  
  
  
  if (! FindInReadable(NS_LITERAL_STRING("type="), match_start, match_end)) {
    return NS_ERROR_FAILURE;
  }
  
  match_start = match_end;
  
  while (match_end != end_iter &&
         *match_end != '/') {
    ++match_end;
  }
  if (match_end == end_iter) {
    return NS_ERROR_FAILURE;
  }
  
  aMajorTypeStart = match_start;
  aMajorTypeEnd = match_end;

  
  if (++match_end == end_iter) {
    return NS_ERROR_FAILURE;
  }
  
  match_start = match_end;
  
  while (match_end != end_iter &&
         !nsCRT::IsAsciiSpace(*match_end) &&
         *match_end != ';') {
    ++match_end;
  }
  if (match_end == end_iter) {
    return NS_ERROR_FAILURE;
  }
  
  aMinorTypeStart = match_start;
  aMinorTypeEnd = match_end;
  
  
  start_iter = match_end;
  
  
  match_start = match_end;
  match_end = end_iter;
  if (FindInReadable(NS_LITERAL_STRING("exts="), match_start, match_end)) {
    nsAString::const_iterator extStart, extEnd;

    if (match_end == end_iter ||
        (*match_end == '"' && ++match_end == end_iter)) {
      return NS_ERROR_FAILURE;
    }
  
    extStart = match_end;
    match_start = extStart;
    match_end = end_iter;
    if (FindInReadable(NS_LITERAL_STRING("desc=\""), match_start, match_end)) {
      
      extEnd = match_start;
      if (extEnd == extStart) {
        return NS_ERROR_FAILURE;
      }
    
      do {
        --extEnd;
      } while (extEnd != extStart &&
               nsCRT::IsAsciiSpace(*extEnd));
      
      if (extEnd != extStart && *extEnd == '"') {
        --extEnd;
      }
    } else {
      
      extEnd = end_iter;
    }
    aExtensions = Substring(extStart, extEnd);
  } else {
    
    aExtensions.Truncate();
  }

  
  match_start = start_iter;
  match_end = end_iter;
  if (FindInReadable(NS_LITERAL_STRING("desc=\""), match_start, match_end)) {
    aDescriptionStart = match_end;
    match_start = aDescriptionStart;
    match_end = end_iter;
    if (FindInReadable(NS_LITERAL_STRING("exts="), match_start, match_end)) {
      
      aDescriptionEnd = match_start;
      if (aDescriptionEnd == aDescriptionStart) {
        return NS_ERROR_FAILURE;
      }
      
      do {
        --aDescriptionEnd;
      } while (aDescriptionEnd != aDescriptionStart &&
               nsCRT::IsAsciiSpace(*aDescriptionEnd));
      
      if (aDescriptionStart != aDescriptionStart && *aDescriptionEnd == '"') {
        --aDescriptionEnd;
      }
    } else {
      
      aDescriptionEnd = end_iter;
    }
  } else {
    
    aDescriptionStart = start_iter;
    aDescriptionEnd = start_iter;
  }

  return NS_OK;
}







nsresult
nsOSHelperAppService::ParseNormalMIMETypesEntry(const nsAString& aEntry,
                                                nsAString::const_iterator& aMajorTypeStart,
                                                nsAString::const_iterator& aMajorTypeEnd,
                                                nsAString::const_iterator& aMinorTypeStart,
                                                nsAString::const_iterator& aMinorTypeEnd,
                                                nsAString& aExtensions,
                                                nsAString::const_iterator& aDescriptionStart,
                                                nsAString::const_iterator& aDescriptionEnd) {
  LOG(("-- ParseNormalMIMETypesEntry\n"));
  NS_ASSERTION(!aEntry.IsEmpty(), "Empty Normal MIME types entry being parsed.");

  nsAString::const_iterator start_iter, end_iter, iter;
  
  aEntry.BeginReading(start_iter);
  aEntry.EndReading(end_iter);

  
  aDescriptionStart = start_iter;
  aDescriptionEnd = start_iter;

  
  while (start_iter != end_iter && nsCRT::IsAsciiSpace(*start_iter)) {
    ++start_iter;
  }
  if (start_iter == end_iter) {
    return NS_ERROR_FAILURE;
  }
  
  do {
    --end_iter;
  } while (end_iter != start_iter && nsCRT::IsAsciiSpace(*end_iter));
           
  ++end_iter; 
  iter = start_iter;

  
  if (! FindCharInReadable('/', iter, end_iter))
    return NS_ERROR_FAILURE;

  nsAString::const_iterator equals_sign_iter(start_iter);
  if (FindCharInReadable('=', equals_sign_iter, iter))
    return NS_ERROR_FAILURE; 
  
  aMajorTypeStart = start_iter;
  aMajorTypeEnd = iter;
  
  
  if (++iter == end_iter) {
    return NS_ERROR_FAILURE;
  }
  start_iter = iter;

  while (iter != end_iter && !nsCRT::IsAsciiSpace(*iter)) { 
    ++iter;
  }
  aMinorTypeStart = start_iter;
  aMinorTypeEnd = iter;

  
  aExtensions.Truncate();
  while (iter != end_iter) {
    while (iter != end_iter && nsCRT::IsAsciiSpace(*iter)) {
      ++iter;
    }

    start_iter = iter;
    while (iter != end_iter && !nsCRT::IsAsciiSpace(*iter)) {
      ++iter;
    }
    aExtensions.Append(Substring(start_iter, iter));
    if (iter != end_iter) { 
      aExtensions.Append(PRUnichar(','));
    }
  }

  return NS_OK;
}


nsresult
nsOSHelperAppService::LookUpHandlerAndDescription(const nsAString& aMajorType,
                                                  const nsAString& aMinorType,
                                                  nsHashtable& aTypeOptions,
                                                  nsAString& aHandler,
                                                  nsAString& aDescription,
                                                  nsAString& aMozillaFlags) {
  
  
  
  
  
  
  
  
  nsresult rv = DoLookUpHandlerAndDescription(aMajorType,
                                              aMinorType,
                                              aTypeOptions,
                                              aHandler,
                                              aDescription,
                                              aMozillaFlags,
                                              PR_TRUE);
  if (NS_FAILED(rv)) {
    rv = DoLookUpHandlerAndDescription(aMajorType,
                                       aMinorType,
                                       aTypeOptions,
                                       aHandler,
                                       aDescription,
                                       aMozillaFlags,
                                       PR_FALSE);
  }

  
  if (NS_FAILED(rv)) {
    rv = DoLookUpHandlerAndDescription(aMajorType,
                                       NS_LITERAL_STRING("*"),
                                       aTypeOptions,
                                       aHandler,
                                       aDescription,
                                       aMozillaFlags,
                                       PR_TRUE);
  }

  if (NS_FAILED(rv)) {
    rv = DoLookUpHandlerAndDescription(aMajorType,
                                       NS_LITERAL_STRING("*"),
                                       aTypeOptions,
                                       aHandler,
                                       aDescription,
                                       aMozillaFlags,
                                       PR_FALSE);
  }

  return rv;
}


nsresult
nsOSHelperAppService::DoLookUpHandlerAndDescription(const nsAString& aMajorType,
                                                    const nsAString& aMinorType,
                                                    nsHashtable& aTypeOptions,
                                                    nsAString& aHandler,
                                                    nsAString& aDescription,
                                                    nsAString& aMozillaFlags,
                                                    PRBool aUserData) {
  LOG(("-- LookUpHandlerAndDescription for type '%s/%s'\n",
       NS_LossyConvertUTF16toASCII(aMajorType).get(),
       NS_LossyConvertUTF16toASCII(aMinorType).get()));
  nsresult rv = NS_OK;
  nsXPIDLString mailcapFileName;

  const char * filenamePref = aUserData ?
    "helpers.private_mailcap_file" : "helpers.global_mailcap_file";
  const char * filenameEnvVar = aUserData ?
    "PERSONAL_MAILCAP" : "MAILCAP";
  
  rv = GetFileLocation(filenamePref,
                       filenameEnvVar,
                       getter_Copies(mailcapFileName));
  if (NS_SUCCEEDED(rv) && !mailcapFileName.IsEmpty()) {
    rv = GetHandlerAndDescriptionFromMailcapFile(mailcapFileName,
                                                 aMajorType,
                                                 aMinorType,
                                                 aTypeOptions,
                                                 aHandler,
                                                 aDescription,
                                                 aMozillaFlags);
  } else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }

  return rv;
}


nsresult
nsOSHelperAppService::GetHandlerAndDescriptionFromMailcapFile(const nsAString& aFilename,
                                                              const nsAString& aMajorType,
                                                              const nsAString& aMinorType,
                                                              nsHashtable& aTypeOptions,
                                                              nsAString& aHandler,
                                                              nsAString& aDescription,
                                                              nsAString& aMozillaFlags) {

  LOG(("-- GetHandlerAndDescriptionFromMailcapFile\n"));
  LOG(("Getting handler and description from mailcap file '%s'\n",
       NS_LossyConvertUTF16toASCII(aFilename).get()));
  LOG(("Using type '%s/%s'\n",
       NS_LossyConvertUTF16toASCII(aMajorType).get(),
       NS_LossyConvertUTF16toASCII(aMinorType).get()));

  nsresult rv = NS_OK;
  PRBool more = PR_FALSE;
  
  nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  rv = file->InitWithPath(aFilename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFileInputStream> mailcapFile(do_CreateInstance(NS_LOCALFILEINPUTSTREAM_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  rv = mailcapFile->Init(file, -1, -1, PR_FALSE);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILineInputStream> mailcap (do_QueryInterface(mailcapFile, &rv));

  if (NS_FAILED(rv)) {
    LOG(("Interface trouble in stream land!"));
    return rv;
  }

  nsString entry, buffer;
  nsCAutoString cBuffer;
  entry.SetCapacity(128);
  cBuffer.SetCapacity(80);
  rv = mailcap->ReadLine(cBuffer, &more);
  if (NS_FAILED(rv)) {
    mailcapFile->Close();
    return rv;
  }

  do {  

    CopyASCIItoUTF16(cBuffer, buffer);
    if (!buffer.IsEmpty() && buffer.First() != '#') {
      entry.Append(buffer);
      if (entry.Last() == '\\') {  
        entry.Truncate(entry.Length()-1);
        entry.Append(PRUnichar(' ')); 
      } else {  
        LOG(("Current entry: '%s'\n",
             NS_LossyConvertUTF16toASCII(entry).get()));

        nsAString::const_iterator semicolon_iter,
                                  start_iter, end_iter,
                                  majorTypeStart, majorTypeEnd,
                                  minorTypeStart, minorTypeEnd;
        entry.BeginReading(start_iter);
        entry.EndReading(end_iter);
        semicolon_iter = start_iter;
        FindSemicolon(semicolon_iter, end_iter);
        if (semicolon_iter != end_iter) { 
          rv = ParseMIMEType(start_iter, majorTypeStart, majorTypeEnd,
                             minorTypeStart, minorTypeEnd, semicolon_iter);
          if (NS_SUCCEEDED(rv) &&
              Substring(majorTypeStart,
                        majorTypeEnd).Equals(aMajorType,
                                             nsCaseInsensitiveStringComparator()) &&
              Substring(minorTypeStart,
                        minorTypeEnd).Equals(aMinorType,
                                             nsCaseInsensitiveStringComparator())) { 
            PRBool match = PR_TRUE;
            ++semicolon_iter;             
            start_iter = semicolon_iter;  
            FindSemicolon(semicolon_iter, end_iter);
            while (start_iter != semicolon_iter &&
                   nsCRT::IsAsciiSpace(*start_iter)) {
              ++start_iter;
            }

            LOG(("The real handler is: '%s'\n",
                 NS_LossyConvertUTF16toASCII(Substring(start_iter,
                                                      semicolon_iter)).get()));
              
            
            nsAString::const_iterator end_handler_iter = semicolon_iter;
            nsAString::const_iterator end_executable_iter = start_iter;
            while (end_executable_iter != end_handler_iter &&
                   !nsCRT::IsAsciiSpace(*end_executable_iter)) {
              ++end_executable_iter;
            }
            
            
            aHandler = Substring(start_iter, end_executable_iter);
            
            nsAString::const_iterator start_option_iter, end_optionname_iter, equal_sign_iter;
            PRBool equalSignFound;
            while (match &&
                   semicolon_iter != end_iter &&
                   ++semicolon_iter != end_iter) { 
              start_option_iter = semicolon_iter;
              
              while (start_option_iter != end_iter &&
                     nsCRT::IsAsciiSpace(*start_option_iter)) {
                ++start_option_iter;
              }
              if (start_option_iter == end_iter) { 
                break;
              }
              semicolon_iter = start_option_iter;
              FindSemicolon(semicolon_iter, end_iter);
              equal_sign_iter = start_option_iter;
              equalSignFound = PR_FALSE;
              while (equal_sign_iter != semicolon_iter && !equalSignFound) {
                switch(*equal_sign_iter) {
                case '\\':
                  equal_sign_iter.advance(2);
                  break;
                case '=':
                  equalSignFound = PR_TRUE;
                  break;
                default:
                  ++equal_sign_iter;
                  break;
                }
              }
              end_optionname_iter = start_option_iter;
              
              while (end_optionname_iter != equal_sign_iter &&
                     !nsCRT::IsAsciiSpace(*end_optionname_iter)) {
                ++end_optionname_iter;
              }                     
              nsDependentSubstring optionName(start_option_iter, end_optionname_iter);
              if (equalSignFound) {
                
                if (optionName.EqualsLiteral("description")) {
                  aDescription = Substring(++equal_sign_iter, semicolon_iter);
                } else if (optionName.EqualsLiteral("x-mozilla-flags")) {
                  aMozillaFlags = Substring(++equal_sign_iter, semicolon_iter);
                } else if (optionName.EqualsLiteral("test")) {
                  nsCAutoString testCommand;
                  rv = UnescapeCommand(Substring(++equal_sign_iter, semicolon_iter),
                                       aMajorType,
                                       aMinorType,
                                       aTypeOptions,
                                       testCommand);
                  if (NS_FAILED(rv))
                    continue;
                  nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID, &rv);
                  if (NS_FAILED(rv))
                    continue;
                  nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
                  if (NS_FAILED(rv))
                    continue;
                  rv = file->InitWithNativePath(NS_LITERAL_CSTRING("/bin/sh"));
                  if (NS_FAILED(rv))
                    continue;
                  rv = process->Init(file);
                  if (NS_FAILED(rv))
                    continue;
                  const char *args[] = { "-c", testCommand.get() };
                  LOG(("Running Test: %s\n", testCommand.get()));
                  rv = process->Run(PR_TRUE, args, 2);
                  if (NS_FAILED(rv))
                    continue;
                  PRInt32 exitValue;
                  rv = process->GetExitValue(&exitValue);
                  if (NS_FAILED(rv))
                    continue;
                  LOG(("Exit code: %d\n", exitValue));
                  if (exitValue) {
                    match = PR_FALSE;
                  }
                }
              } else {
                
                if (optionName.EqualsLiteral("needsterminal")) {
                  match = PR_FALSE;
                }
              }
            }
            

            if (match) { 
              
              mailcapFile->Close();
              return NS_OK;
            } else { 
              aDescription.Truncate();
              aMozillaFlags.Truncate();
              aHandler.Truncate();
            }
          }
        }
        
        entry.Truncate();
      }    
    }
    if (!more) {
      rv = NS_ERROR_NOT_AVAILABLE;
      break;
    }
    rv = mailcap->ReadLine(cBuffer, &more);
  } while (NS_SUCCEEDED(rv));
  mailcapFile->Close();
  return rv;
}

nsresult nsOSHelperAppService::OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists)
{
  LOG(("-- nsOSHelperAppService::OSProtocolHandlerExists for '%s'\n",
       aProtocolScheme));
  *aHandlerExists = PR_FALSE;

#ifdef MOZ_WIDGET_GTK2
  
  *aHandlerExists = nsGNOMERegistry::HandlerExists(aProtocolScheme);
#ifdef MOZ_PLATFORM_HILDON
  *aHandlerExists = nsMIMEInfoUnix::HandlerExists(aProtocolScheme);
#endif
#endif

  return NS_OK;
}

NS_IMETHODIMP nsOSHelperAppService::GetApplicationDescription(const nsACString& aScheme, nsAString& _retval)
{
#ifdef MOZ_WIDGET_GTK2
  nsGNOMERegistry::GetAppDescForScheme(aScheme, _retval);
  return _retval.IsEmpty() ? NS_ERROR_NOT_AVAILABLE : NS_OK;
#else
  return NS_ERROR_NOT_AVAILABLE;
#endif
}

nsresult nsOSHelperAppService::GetFileTokenForPath(const PRUnichar * platformAppPath, nsIFile ** aFile)
{
  LOG(("-- nsOSHelperAppService::GetFileTokenForPath: '%s'\n",
       NS_LossyConvertUTF16toASCII(platformAppPath).get()));
  if (! *platformAppPath) { 
    NS_WARNING("Empty filename passed in.");
    return NS_ERROR_INVALID_ARG;
  }
 
  
  nsresult rv = nsExternalHelperAppService::GetFileTokenForPath(platformAppPath, aFile);
  if (NS_SUCCEEDED(rv))
    return rv;
  
  
  
  if (rv == NS_ERROR_FILE_NOT_FOUND)
    return rv;

  
  NS_ASSERTION(*platformAppPath != PRUnichar('/'), "Unexpected absolute path");

  nsCOMPtr<nsILocalFile> localFile (do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));

  if (!localFile) return NS_ERROR_NOT_INITIALIZED;
 
  PRBool exists = PR_FALSE;
  
  char* unixpath = PR_GetEnv("PATH");
  nsCAutoString path(unixpath);

  const char* start_iter = path.BeginReading(start_iter);
  const char* colon_iter = start_iter;
  const char* end_iter = path.EndReading(end_iter);

  while (start_iter != end_iter && !exists) {
    while (colon_iter != end_iter && *colon_iter != ':') {
      ++colon_iter;
    }
    localFile->InitWithNativePath(Substring(start_iter, colon_iter));
    rv = localFile->AppendRelativePath(nsDependentString(platformAppPath));
    
    
    
    NS_ENSURE_SUCCESS(rv, rv);
    localFile->Exists(&exists);
    if (!exists) {
      if (colon_iter == end_iter) {
        break;
      }
      ++colon_iter;
      start_iter = colon_iter;
    }
  }

  if (exists) {
    rv = NS_OK;
  } else {
    rv = NS_ERROR_NOT_AVAILABLE;
  }
  
  *aFile = localFile;
  NS_IF_ADDREF(*aFile);

  return rv;
}

already_AddRefed<nsMIMEInfoBase>
nsOSHelperAppService::GetFromExtension(const nsCString& aFileExt) {
  
  if (aFileExt.IsEmpty())
    return nsnull;
  
  LOG(("Here we do an extension lookup for '%s'\n", aFileExt.get()));

  nsAutoString majorType, minorType,
               mime_types_description, mailcap_description,
               handler, mozillaFlags;
  
  nsresult rv = LookUpTypeAndDescription(NS_ConvertUTF8toUTF16(aFileExt),
                                         majorType,
                                         minorType,
                                         mime_types_description,
                                         PR_TRUE);

  if (NS_FAILED(rv) || majorType.IsEmpty()) {
    
#ifdef MOZ_WIDGET_GTK2
    LOG(("Looking in GNOME registry\n"));
    nsMIMEInfoBase *gnomeInfo = nsGNOMERegistry::GetFromExtension(aFileExt).get();
    if (gnomeInfo) {
      LOG(("Got MIMEInfo from GNOME registry\n"));
      return gnomeInfo;
    }
#endif

    rv = LookUpTypeAndDescription(NS_ConvertUTF8toUTF16(aFileExt),
                                  majorType,
                                  minorType,
                                  mime_types_description,
                                  PR_FALSE);
  }
  
  if (NS_FAILED(rv))
    return nsnull;

  NS_LossyConvertUTF16toASCII asciiMajorType(majorType);
  NS_LossyConvertUTF16toASCII asciiMinorType(minorType);

  LOG(("Type/Description results:  majorType='%s', minorType='%s', description='%s'\n",
          asciiMajorType.get(),
          asciiMinorType.get(),
          NS_LossyConvertUTF16toASCII(mime_types_description).get()));

  if (majorType.IsEmpty() && minorType.IsEmpty()) {
    
    return nsnull;
  }

  nsCAutoString mimeType(asciiMajorType + NS_LITERAL_CSTRING("/") + asciiMinorType);
  nsMIMEInfoUnix* mimeInfo = new nsMIMEInfoUnix(mimeType);
  if (!mimeInfo)
    return nsnull;
  NS_ADDREF(mimeInfo);
  
  mimeInfo->AppendExtension(aFileExt);
  nsHashtable typeOptions; 
  rv = LookUpHandlerAndDescription(majorType, minorType, typeOptions,
                                   handler, mailcap_description,
                                   mozillaFlags);
  LOG(("Handler/Description results:  handler='%s', description='%s', mozillaFlags='%s'\n",
          NS_LossyConvertUTF16toASCII(handler).get(),
          NS_LossyConvertUTF16toASCII(mailcap_description).get(),
          NS_LossyConvertUTF16toASCII(mozillaFlags).get()));
  mailcap_description.Trim(" \t\"");
  mozillaFlags.Trim(" \t");
  if (!mime_types_description.IsEmpty()) {
    mimeInfo->SetDescription(mime_types_description);
  } else {
    mimeInfo->SetDescription(mailcap_description);
  }

  if (NS_SUCCEEDED(rv) && handler.IsEmpty()) {
    rv = NS_ERROR_NOT_AVAILABLE;
  }
  
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIFile> handlerFile;
    rv = GetFileTokenForPath(handler.get(), getter_AddRefs(handlerFile));
    
    if (NS_SUCCEEDED(rv)) {
      mimeInfo->SetDefaultApplication(handlerFile);
      mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
      mimeInfo->SetDefaultDescription(handler);
    }
  }

  if (NS_FAILED(rv)) {
    mimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);
  }

  return mimeInfo;
}

already_AddRefed<nsMIMEInfoBase>
nsOSHelperAppService::GetFromType(const nsCString& aMIMEType) {
  
  if (aMIMEType.IsEmpty())
    return nsnull;
  
  LOG(("Here we do a mimetype lookup for '%s'\n", aMIMEType.get()));

  
  NS_ConvertASCIItoUTF16 mimeType(aMIMEType);
  nsAString::const_iterator start_iter, end_iter,
                            majorTypeStart, majorTypeEnd,
                            minorTypeStart, minorTypeEnd;

  mimeType.BeginReading(start_iter);
  mimeType.EndReading(end_iter);

  
  nsHashtable typeOptions;  
  nsresult rv = ParseMIMEType(start_iter, majorTypeStart, majorTypeEnd,
                              minorTypeStart, minorTypeEnd, end_iter);

  if (NS_FAILED(rv)) {
    return nsnull;
  }

  nsDependentSubstring majorType(majorTypeStart, majorTypeEnd);
  nsDependentSubstring minorType(minorTypeStart, minorTypeEnd);

  
  nsAutoString mailcap_description, handler, mozillaFlags;
  DoLookUpHandlerAndDescription(majorType,
                                minorType,
                                typeOptions,
                                handler,
                                mailcap_description,
                                mozillaFlags,
                                PR_TRUE);
  
  LOG(("Private Handler/Description results:  handler='%s', description='%s'\n",
          NS_LossyConvertUTF16toASCII(handler).get(),
          NS_LossyConvertUTF16toASCII(mailcap_description).get()));

#ifdef MOZ_WIDGET_GTK2
  nsMIMEInfoBase *gnomeInfo = nsnull;
  if (handler.IsEmpty()) {
    
    
    
    
    LOG(("Looking in GNOME registry\n"));
    gnomeInfo = nsGNOMERegistry::GetFromType(aMIMEType).get();
    if (gnomeInfo && gnomeInfo->HasExtensions()) {
      LOG(("Got MIMEInfo from GNOME registry, and it has extensions set\n"));
      return gnomeInfo;
    }
  }
#endif

  
  nsAutoString extensions, mime_types_description;
  LookUpExtensionsAndDescription(majorType,
                                 minorType,
                                 extensions,
                                 mime_types_description);

#ifdef MOZ_WIDGET_GTK2
  if (gnomeInfo) {
    LOG(("Got MIMEInfo from GNOME registry without extensions; setting them "
         "to %s\n", NS_LossyConvertUTF16toASCII(extensions).get()));

    NS_ASSERTION(!gnomeInfo->HasExtensions(), "How'd that happen?");
    gnomeInfo->SetFileExtensions(NS_ConvertUTF16toUTF8(extensions));
    return gnomeInfo;
  }
#endif

  if (handler.IsEmpty()) {
    DoLookUpHandlerAndDescription(majorType,
                                  minorType,
                                  typeOptions,
                                  handler,
                                  mailcap_description,
                                  mozillaFlags,
                                  PR_FALSE);
  }

  if (handler.IsEmpty()) {
    DoLookUpHandlerAndDescription(majorType,
                                  NS_LITERAL_STRING("*"),
                                  typeOptions,
                                  handler,
                                  mailcap_description,
                                  mozillaFlags,
                                  PR_TRUE);
  }

  if (handler.IsEmpty()) {
    DoLookUpHandlerAndDescription(majorType,
                                  NS_LITERAL_STRING("*"),
                                  typeOptions,
                                  handler,
                                  mailcap_description,
                                  mozillaFlags,
                                  PR_FALSE);
  }  
  
  LOG(("Handler/Description results:  handler='%s', description='%s', mozillaFlags='%s'\n",
          NS_LossyConvertUTF16toASCII(handler).get(),
          NS_LossyConvertUTF16toASCII(mailcap_description).get(),
          NS_LossyConvertUTF16toASCII(mozillaFlags).get()));
  
  mailcap_description.Trim(" \t\"");
  mozillaFlags.Trim(" \t");

  if (handler.IsEmpty() && extensions.IsEmpty() &&
      mailcap_description.IsEmpty() && mime_types_description.IsEmpty()) {
    
    return nsnull;
  }
  
  nsMIMEInfoUnix* mimeInfo = new nsMIMEInfoUnix(aMIMEType);
  if (!mimeInfo)
    return nsnull;
  NS_ADDREF(mimeInfo);

  mimeInfo->SetFileExtensions(NS_ConvertUTF16toUTF8(extensions));
  if (! mime_types_description.IsEmpty()) {
    mimeInfo->SetDescription(mime_types_description);
  } else {
    mimeInfo->SetDescription(mailcap_description);
  }

  rv = NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIFile> handlerFile;
  if (!handler.IsEmpty()) {
    rv = GetFileTokenForPath(handler.get(), getter_AddRefs(handlerFile));
  }
  
  if (NS_SUCCEEDED(rv)) {
    mimeInfo->SetDefaultApplication(handlerFile);
    mimeInfo->SetPreferredAction(nsIMIMEInfo::useSystemDefault);
    mimeInfo->SetDefaultDescription(handler);
  } else {
    mimeInfo->SetPreferredAction(nsIMIMEInfo::saveToDisk);
  }

  return mimeInfo;
}


already_AddRefed<nsIMIMEInfo>
nsOSHelperAppService::GetMIMEInfoFromOS(const nsACString& aType,
                                        const nsACString& aFileExt,
                                        PRBool     *aFound) {
  *aFound = PR_TRUE;
  nsMIMEInfoBase* retval = GetFromType(PromiseFlatCString(aType)).get();
  PRBool hasDefault = PR_FALSE;
  if (retval)
    retval->GetHasDefaultHandler(&hasDefault);
  if (!retval || !hasDefault) {
    nsRefPtr<nsMIMEInfoBase> miByExt = GetFromExtension(PromiseFlatCString(aFileExt));
    
    if (!miByExt && retval)
      return retval;
    
    
    if (!retval && miByExt) {
      if (!aType.IsEmpty())
        miByExt->SetMIMEType(aType);
      miByExt.swap(retval);

      return retval;
    }
    
    if (!retval) {
      *aFound = PR_FALSE;
      retval = new nsMIMEInfoUnix(aType);
      if (retval) {
        NS_ADDREF(retval);
        if (!aFileExt.IsEmpty())
          retval->AppendExtension(aFileExt);
      }
      
      return retval;
    }

    
    
    
    nsAutoString byExtDefault;
    miByExt->GetDefaultDescription(byExtDefault);
    retval->SetDefaultDescription(byExtDefault);
    retval->CopyBasicDataTo(miByExt);

    miByExt.swap(retval);
  }
  return retval;
}

NS_IMETHODIMP
nsOSHelperAppService::GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                                   PRBool *found,
                                                   nsIHandlerInfo **_retval)
{
  NS_ASSERTION(!aScheme.IsEmpty(), "No scheme was specified!");

  
  
  
  nsresult rv = OSProtocolHandlerExists(nsPromiseFlatCString(aScheme).get(),
                                        found);
  if (NS_FAILED(rv))
    return rv;

  nsMIMEInfoUnix *handlerInfo =
    new nsMIMEInfoUnix(aScheme, nsMIMEInfoBase::eProtocolInfo);
  NS_ENSURE_TRUE(handlerInfo, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*_retval = handlerInfo);

  if (!*found) {
    
    
    return NS_OK;
  }

  nsAutoString desc;
  GetApplicationDescription(aScheme, desc);
  handlerInfo->SetDefaultDescription(desc);

  return NS_OK;
}

void
nsOSHelperAppService::FixFilePermissions(nsILocalFile* aFile)
{
  aFile->SetPermissions(mPermissions); 
}

