




































#ifndef mozilla_FileLocation_h
#define mozilla_FileLocation_h

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "FileUtils.h"

class nsZipArchive;
class nsZipItem;

namespace mozilla {

class FileLocation
{
public:
  










  FileLocation() { }

  


  FileLocation(nsILocalFile *file)
  {
    Init(file);
  }

  



  FileLocation(nsILocalFile *zip, const char *path)
  {
    Init(zip, path);
  }

  FileLocation(nsZipArchive *zip, const char *path)
  {
    Init(zip, path);
  }

  


  FileLocation(const FileLocation &file, const char *path = NULL);

  


  void Init(nsILocalFile *file)
  {
    mBaseZip = NULL;
    mBaseFile = file;
    mPath.Truncate();
  }

  void Init(nsILocalFile *zip, const char *path)
  {
    mBaseZip = NULL;
    mBaseFile = zip;
    mPath = path;
  }

  void Init(nsZipArchive *zip, const char *path)
  {
    mBaseZip = zip;
    mBaseFile = NULL;
    mPath = path;
  }

  


  void GetURIString(nsACString &result) const;

  





  already_AddRefed<nsILocalFile> GetBaseFile();

  


  bool IsZip() const
  {
    return !mPath.IsEmpty();
  }

  


  void GetPath(nsACString &result) const
  {
    result = mPath;
  }

  



  operator bool() const
  {
    return mBaseFile || mBaseZip;
  }

  


  bool Equals(const FileLocation &file) const;

  


  class Data
  {
  public:
    


    nsresult GetSize(PRUint32 *result);

    


    nsresult Copy(char *buf, PRUint32 len);
  protected:
    friend class FileLocation;
    nsZipItem *mItem;
    nsRefPtr<nsZipArchive> mZip;
    mozilla::AutoFDClose mFd;
  };

  



  nsresult GetData(Data &data);
private:
  nsCOMPtr<nsILocalFile> mBaseFile;
  nsRefPtr<nsZipArchive> mBaseZip;
  nsCString mPath;
}; 

} 

#endif 
