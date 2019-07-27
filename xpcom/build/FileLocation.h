





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

  


  explicit FileLocation(nsIFile* aFile);

  



  FileLocation(nsIFile* aZip, const char* aPath);

  FileLocation(nsZipArchive* aZip, const char* aPath);

  


  FileLocation(const FileLocation& aFile, const char* aPath = nullptr);

  


  void Init(nsIFile* aFile);

  void Init(nsIFile* aZip, const char* aPath);

  void Init(nsZipArchive* aZip, const char* aPath);

  


  void GetURIString(nsACString& aResult) const;

  





  already_AddRefed<nsIFile> GetBaseFile();

  


  bool IsZip() const { return !mPath.IsEmpty(); }

  


  void GetPath(nsACString& aResult) const { aResult = mPath; }

  



#if defined(MOZILLA_XPCOMRT_API)
  operator bool() const { return mBaseFile; }
#else
  operator bool() const { return mBaseFile || mBaseZip; }
#endif 

  


  bool Equals(const FileLocation& aFile) const;

  


  class Data
  {
  public:
    


    nsresult GetSize(uint32_t* aResult);

    


    nsresult Copy(char* aBuf, uint32_t aLen);
  protected:
    friend class FileLocation;
#if !defined(MOZILLA_XPCOMRT_API)
    nsZipItem* mItem;
#endif 
    nsRefPtr<nsZipArchive> mZip;
    mozilla::AutoFDClose mFd;
  };

  



  nsresult GetData(Data& aData);
private:
  nsCOMPtr<nsIFile> mBaseFile;
#if !defined(MOZILLA_XPCOMRT_API)
  nsRefPtr<nsZipArchive> mBaseZip;
#endif 
  nsCString mPath;
}; 

} 

#endif 
