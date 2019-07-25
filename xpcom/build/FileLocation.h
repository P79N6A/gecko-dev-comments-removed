



#ifndef mozilla_FileLocation_h
#define mozilla_FileLocation_h

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIFile.h"
#include "nsIURI.h"
#include "FileUtils.h"

class nsZipArchive;
class nsZipItem;

namespace mozilla {

class FileLocation
{
public:
  










  FileLocation() { }

  


  FileLocation(nsIFile *file)
  {
    Init(file);
  }

  



  FileLocation(nsIFile *zip, const char *path)
  {
    Init(zip, path);
  }

  FileLocation(nsZipArchive *zip, const char *path)
  {
    Init(zip, path);
  }

  


  FileLocation(const FileLocation &file, const char *path = NULL);

  


  void Init(nsIFile *file)
  {
    mBaseZip = NULL;
    mBaseFile = file;
    mPath.Truncate();
  }

  void Init(nsIFile *zip, const char *path)
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

  





  already_AddRefed<nsIFile> GetBaseFile();

  


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
    


    nsresult GetSize(uint32_t *result);

    


    nsresult Copy(char *buf, uint32_t len);
  protected:
    friend class FileLocation;
    nsZipItem *mItem;
    nsRefPtr<nsZipArchive> mZip;
    mozilla::AutoFDClose mFd;
  };

  



  nsresult GetData(Data &data);
private:
  nsCOMPtr<nsIFile> mBaseFile;
  nsRefPtr<nsZipArchive> mBaseZip;
  nsCString mPath;
}; 

} 

#endif 
