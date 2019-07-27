



#ifndef mozilla_FileLocation_h
#define mozilla_FileLocation_h

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIFile.h"
#include "FileUtils.h"

class nsZipArchive;
class nsZipItem;

namespace mozilla {

class FileLocation
{
public:
  










  FileLocation();
  ~FileLocation();

  


  explicit FileLocation(nsIFile *file);

  



  FileLocation(nsIFile *zip, const char *path);

  FileLocation(nsZipArchive *zip, const char *path);

  


  FileLocation(const FileLocation &file, const char *path = nullptr);

  


  void Init(nsIFile *file);

  void Init(nsIFile *zip, const char *path);

  void Init(nsZipArchive *zip, const char *path);

  


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
