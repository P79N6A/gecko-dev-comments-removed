





































#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsNameValuePairDB.h"
#include "nsILocalFile.h"

#define NVPDB_VERSION_MAJOR 1
#define NVPDB_VERSION_MINOR 0
#define NVPDB_VERSION_MAINTENANCE   0

#ifdef DEBUG
# define NVPDB_PRINTF(x) \
            PR_BEGIN_MACRO \
              printf x ; \
              printf(", %s %d\n", __FILE__, __LINE__); \
            PR_END_MACRO 
#else
# define NVPDB_PRINTF(x)
#endif

PRBool
nsNameValuePairDB::CheckHeader()
{
  const char *name, *value;
  int num, major, minor, maintenance;
  PRBool foundVersion = PR_FALSE;

  if (!mFile)
    return PR_FALSE;

  if (fseek(mFile, 0L, SEEK_SET) != 0)
    return PR_FALSE;
  mCurrentGroup = 0;
  mAtEndOfGroup = PR_FALSE;
  while (GetNextElement(&name, &value) > 0) {
    if (*name == '\0') 
      continue;
    if (strcmp(name, "Version")==0) {
      foundVersion = PR_TRUE;
      num = sscanf(value, "%d.%d.%d", &major, &minor, &maintenance);
      if (num != 3) {
        NVPDB_PRINTF(("failed to parse version number (%s)", value));
        return PR_FALSE;
      }

      
      
      if (major != NVPDB_VERSION_MAJOR) {
        NVPDB_PRINTF(("version major %d != %d", major, NVPDB_VERSION_MAJOR));
        return PR_FALSE;
      }

      
      
      
      

      
      
      
      

      mMajorNum = major;
      mMinorNum = minor;
      mMaintenanceNum = maintenance;
    }
  }

  return foundVersion;
}












PRInt32
nsNameValuePairDB::GetNextElement(const char** aName, const char** aValue)
{
  return GetNextElement(aName, aValue, mBuf, sizeof(mBuf));
}











PRInt32
nsNameValuePairDB::GetNextElement(const char** aName, const char** aValue,
                                  char *aBuffer, PRUint32 aBufferLen)
{
  char *line, *name, *value;
  unsigned int num;
  int len;
  unsigned int groupNum;

  *aName  = "";
  *aValue = "";

  if (aBufferLen < NVPDB_MIN_BUFLEN) {
    return NVPDB_BUFFER_TOO_SMALL;
  }

  if (mAtEndOfGroup) {
    return NVPDB_END_OF_GROUP;
  }

  
  
  
  line = fgets(aBuffer, aBufferLen, mFile);
  if (!line) {
    if (feof(mFile)) { 
      mAtEndOfGroup = PR_TRUE;
      mAtEndOfCatalog = PR_TRUE;
      return NVPDB_END_OF_FILE;
    }
    return NVPDB_FILE_IO_ERROR;
  }

  
  
  
  len = strlen(line);
  NS_ASSERTION(len!=0, "an empty string is invalid");
  if (len == 0)
    return NVPDB_GARBLED_LINE;
  if (line[len-1] != '\n') {
    len++; 
    while (1) {
      int val = getc(mFile);
      if (val == EOF)
        return -len;
      len++;
      if (val == '\n')
        return -len;
    }
  }
  len--;
  line[len] = '\0';
  

  
  
  
  num = sscanf(line, "%u", &groupNum);
  if ((num != 1) || (groupNum != (unsigned)mCurrentGroup))
    return NVPDB_END_OF_GROUP;

  
  
  
  name = strchr(line, ' ');
  if ((!name) || (name[1]=='\0'))
    return NVPDB_GARBLED_LINE;
  name++;

  
  
  
  
  
  if (*name == '#') {
    *aValue = name;
    return 1;
  }

  
  
  
  value = strchr(name, '=');
  if (!value)
    return NVPDB_GARBLED_LINE;
  *value = '\0';
  value++;

  
  
  
  if (strcmp(name,"end")==0) {
    mAtEndOfGroup = PR_TRUE;
    return NVPDB_END_OF_GROUP;
  }

  
  
  
  *aName = name;
  *aValue = value;
  return 1;
}

PRBool
nsNameValuePairDB::GetNextGroup(const char** aType)
{
  return GetNextGroup(aType, nsnull, 0);
}

PRBool
nsNameValuePairDB::GetNextGroup(const char** aType, const char* aName)
{
  return GetNextGroup(aType, aName, strlen(aName));
}

PRBool
nsNameValuePairDB::GetNextGroup(const char** aType, const char* aName, int aLen)
{
  const char *name, *value;
  long pos = 0;

  *aType = "";

  if (mAtEndOfCatalog)
    return PR_FALSE;

  
  
  
  while (GetNextElement(&name, &value) > 0) 
    continue;
  mCurrentGroup++;
  mAtEndOfGroup = PR_FALSE;
  
  
  if (aName)
    pos = ftell(mFile);

  
  if (GetNextElement(&name, &value) <= 0) {
    mAtEndOfGroup = PR_TRUE;
    mAtEndOfCatalog = PR_TRUE;
    return PR_FALSE;
  }
  if (strcmp(name,"begin"))
    goto GetNext_Error;

  
  if (aName) {
    if (strncmp(value,aName,aLen)) {
      fseek(mFile, pos, SEEK_SET);
      mCurrentGroup--;
      mAtEndOfGroup = PR_TRUE;
      return PR_FALSE;
    }
  }

  *aType = value;
  return PR_TRUE;

GetNext_Error:
  mError = PR_TRUE;
  NVPDB_PRINTF(("GetNext_Error"));
  return PR_FALSE;
}

nsNameValuePairDB::nsNameValuePairDB()
{
  mFile = nsnull;
  mBuf[0] = '\0';
  mMajorNum = 0;
  mMinorNum = 0;
  mMaintenanceNum = 0;
  mCurrentGroup = 0;
  mAtEndOfGroup = PR_FALSE;
  mAtEndOfCatalog = PR_FALSE;
  mError = PR_FALSE;
}

nsNameValuePairDB::~nsNameValuePairDB()
{
  if (mFile) {
    fclose(mFile);
    mFile = nsnull;
  }
}

PRBool
nsNameValuePairDB::OpenForRead(const nsACString & aCatalogName) 
{
  nsresult result;

  nsCOMPtr<nsILocalFile> local_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID,
                                                        &result);
  if (NS_FAILED(result))
    goto error_return;

  local_file->InitWithNativePath(aCatalogName);
  local_file->OpenANSIFileDesc("r", &mFile);
  if (mFile && CheckHeader())
    return PR_TRUE;

error_return:
  mError = PR_TRUE;
  NVPDB_PRINTF(("OpenForRead error"));
  return PR_FALSE;
}

PRBool
nsNameValuePairDB::OpenTmpForWrite(const nsACString& aCatalogName) 
{
  nsresult result;
  nsCOMPtr<nsILocalFile> local_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID,
                                                        &result);
  if (NS_FAILED(result))
    return PR_FALSE;
  local_file->InitWithNativePath(aCatalogName + NS_LITERAL_CSTRING(".tmp"));
  local_file->OpenANSIFileDesc("w+", &mFile);
  if (mFile == nsnull)
    return PR_FALSE;

  
  mAtEndOfGroup = PR_TRUE;
  mCurrentGroup = -1;
  PutStartGroup("Header");
  char buf[64];
  PutElement("", "########################################");
  PutElement("", "#                                      #");
  PutElement("", "#          Name Value Pair DB          #");
  PutElement("", "#                                      #");
  PutElement("", "#   This is a program generated file   #");
  PutElement("", "#                                      #");
  PutElement("", "#             Do not edit              #");
  PutElement("", "#                                      #");
  PutElement("", "########################################");
  PR_snprintf(buf, sizeof(buf), "%d.%d.%d", NVPDB_VERSION_MAJOR,
                NVPDB_VERSION_MINOR, NVPDB_VERSION_MAINTENANCE);
  PutElement("Version", buf);
  PutEndGroup("Header");

  return PR_TRUE;
}

PRBool
nsNameValuePairDB::PutElement(const char* aName, const char* aValue)
{
  if (mAtEndOfGroup) {
    mError = PR_TRUE;
    NVPDB_PRINTF(("PutElement_Error"));
    return PR_FALSE;
  }

  if ((!*aName) && (*aValue == '#'))
    fprintf(mFile, "%u %s\n", mCurrentGroup, aValue);
  else
    fprintf(mFile, "%u %s=%s\n", mCurrentGroup, aName, aValue);
#ifdef DEBUG
  fflush(mFile);
#endif
  return PR_TRUE;
}

PRBool
nsNameValuePairDB::PutEndGroup(const char* aType)
{
  if (mAtEndOfGroup) {
    mError = PR_TRUE;
    NVPDB_PRINTF(("PutEndGroup_Error"));
    return PR_FALSE;
  }

  mAtEndOfGroup = PR_TRUE;
  fprintf(mFile, "%u end=%s\n", mCurrentGroup, aType);
#ifdef DEBUG
  fflush(mFile);
#endif
  return PR_TRUE;
}

PRBool
nsNameValuePairDB::PutStartGroup(const char* aType)
{
  if (!mAtEndOfGroup) {
    mError = PR_TRUE;
    NVPDB_PRINTF(("PutStartGroup_Error"));
#ifdef DEBUG
    fflush(mFile);
#endif
    return PR_FALSE;
  }

  mAtEndOfGroup = PR_FALSE;
  mCurrentGroup++;
  fprintf(mFile, "%u begin=%s\n", mCurrentGroup, aType);
#ifdef DEBUG
  fflush(mFile);
#endif
  return PR_TRUE;
}

PRBool
nsNameValuePairDB::RenameTmp(const char* aCatalogName)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> dir;
  PRBool exists = PR_FALSE;
  nsCAutoString old_name(aCatalogName);
  nsDependentCString current_name(aCatalogName);
  nsCAutoString tmp_name(aCatalogName);
  nsCAutoString old_name_tail;
  nsCAutoString current_name_tail;
  nsCOMPtr<nsILocalFile> old_file;
  nsCOMPtr<nsILocalFile> current_file;
  nsCOMPtr<nsILocalFile> tmp_file;
  nsCAutoString parent_dir;
  nsCAutoString parent_path;
  nsCAutoString cur_path;

  
  
  
  PRInt32 slash = 0, last_slash = -1;
  nsCAutoString fontDirName(aCatalogName);
  
  while ((slash=fontDirName.FindChar('/', slash))>=0) {
    last_slash = slash;
    slash++;
  }
  if (last_slash < 0)
    goto Rename_Error;

  fontDirName.Left(parent_dir, last_slash);
  dir = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    goto Rename_Error;
  dir->InitWithNativePath(parent_dir);
  dir->GetNativePath(parent_path);

  if (!mAtEndOfGroup || mError)
    goto Rename_Error;

  
  
  
  tmp_name.Append(".tmp");
  tmp_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    goto Rename_Error;
  tmp_file->InitWithNativePath(tmp_name);
  tmp_file->Exists(&exists);
  if (!exists)
    goto Rename_Error;

  
  
  
  old_name.Append(".old");
  old_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    goto Rename_Error;
  old_file->InitWithNativePath(old_name);

  
  
  
  current_file = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    goto Rename_Error;
  current_file->InitWithNativePath(current_name);
  current_file->Exists(&exists);
  if (exists) {
    
    
    
    current_file->GetNativePath(cur_path);
    old_name.Right(old_name_tail, old_name.Length() - last_slash - 1);
    rv = current_file->MoveToNative(dir, old_name_tail);
    if (NS_FAILED(rv))
      goto Rename_Error;
  }

  
  
  
  current_name_tail = Substring(current_name, last_slash+1,
                                current_name.Length() - (last_slash + 1));
  rv = tmp_file->MoveToNative(dir, current_name_tail);
  if (NS_FAILED(rv))
    goto Rename_Error;

  
  
  
  if (exists) {
    old_file->Remove(PR_FALSE);
  }

  return PR_TRUE;

Rename_Error:
  mError = PR_TRUE;
  NVPDB_PRINTF(("Rename_Error"));
  return PR_FALSE;
}

