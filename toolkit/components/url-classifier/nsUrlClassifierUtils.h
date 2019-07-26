



#ifndef nsUrlClassifierUtils_h_
#define nsUrlClassifierUtils_h_

#include "nsAutoPtr.h"
#include "nsIUrlClassifierUtils.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "mozilla/Attributes.h"

class nsUrlClassifierUtils MOZ_FINAL : public nsIUrlClassifierUtils
{
private:
  





  class Charmap
  {
  public:
    Charmap(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3,
            uint32_t b4, uint32_t b5, uint32_t b6, uint32_t b7)
    {
      mMap[0] = b0; mMap[1] = b1; mMap[2] = b2; mMap[3] = b3;
      mMap[4] = b4; mMap[5] = b5; mMap[6] = b6; mMap[7] = b7;
    }

    


    bool Contains(unsigned char c) const
    {
      return mMap[c >> 5] & (1 << (c & 31));
    }

  private:
    
    uint32_t mMap[8];
  };


public:
  nsUrlClassifierUtils();
  ~nsUrlClassifierUtils() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUTILS

  nsresult Init();

  nsresult CanonicalizeHostname(const nsACString & hostname,
                                nsACString & _retval);
  nsresult CanonicalizePath(const nsACString & url, nsACString & _retval);

  
  
  
  
  
  
  bool SpecialEncode(const nsACString & url,
                       bool foldSlashes,
                       nsACString & _retval);

  void ParseIPAddress(const nsACString & host, nsACString & _retval);
  void CanonicalNum(const nsACString & num,
                    uint32_t bytes,
                    bool allowOctal,
                    nsACString & _retval);

private:
  
  nsUrlClassifierUtils(const nsUrlClassifierUtils&);

  
  bool ShouldURLEscape(const unsigned char c) const;

  void CleanupHostname(const nsACString & host, nsACString & _retval);

  nsAutoPtr<Charmap> mEscapeCharmap;
};

#endif 
