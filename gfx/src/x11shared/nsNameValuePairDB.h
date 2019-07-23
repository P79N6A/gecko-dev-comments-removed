





































#ifndef NSNAMEVALUEPAIRDB_H
#define NSNAMEVALUEPAIRDB_H

#include "nsString.h"

#define FC_BUF_LEN 1024

#define NVPDB_MIN_BUFLEN 100




#define NVPDB_END_OF_FILE       0
#define NVPDB_BUFFER_TOO_SMALL -1
#define NVPDB_END_OF_GROUP     -2
#define NVPDB_FILE_IO_ERROR    -3
#define NVPDB_GARBLED_LINE     -4

class nsNameValuePairDB {
public:
  nsNameValuePairDB();
  ~nsNameValuePairDB();

  inline PRBool HadError() { return mError; };
  
  
  
  PRBool  GetNextGroup(const char** aType);
  PRBool  GetNextGroup(const char** aType, const char* aName);
  PRBool  GetNextGroup(const char** aType, const char* aName, int aLen);
  PRInt32 GetNextElement(const char** aName, const char** aValue);
  PRInt32 GetNextElement(const char** aName, const char** aValue,
                         char *aBuffer, PRUint32 aBufferLen);
  PRBool  OpenForRead(const nsACString& aCatalogName);     
  PRBool  OpenTmpForWrite(const nsACString& aCatalogName); 
  PRBool  PutElement(const char* aName, const char* aValue);
  PRBool  PutEndGroup(const char* aType);
  PRBool  PutStartGroup(const char* aType);
  PRBool  RenameTmp(const char* aCatalogName);

protected:
  PRBool         CheckHeader();
  PRUint16       mMajorNum;
  PRUint16       mMinorNum;
  PRUint16       mMaintenanceNum;
  FILE*          mFile;
  char           mBuf[FC_BUF_LEN];
  PRInt32        mCurrentGroup;
  PRPackedBool   mAtEndOfGroup;
  PRPackedBool   mAtEndOfCatalog;
  PRPackedBool   mError;
private:
};

#endif

