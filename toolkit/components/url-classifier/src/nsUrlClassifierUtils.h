



































#ifndef nsUrlClassifierUtils_h_
#define nsUrlClassifierUtils_h_

#include "nsAutoPtr.h"
#include "nsIUrlClassifierUtils.h"

class nsUrlClassifierUtils : public nsIUrlClassifierUtils
{
private:
  





  class Charmap
  {
  public:
    Charmap(PRUint32 b0, PRUint32 b1, PRUint32 b2, PRUint32 b3,
            PRUint32 b4, PRUint32 b5, PRUint32 b6, PRUint32 b7)
    {
      mMap[0] = b0; mMap[1] = b1; mMap[2] = b2; mMap[3] = b3;
      mMap[4] = b4; mMap[5] = b5; mMap[6] = b6; mMap[7] = b7;
    }

    


    PRBool Contains(unsigned char c) const
    {
      return mMap[c >> 5] & (1 << (c & 31));
    }

  private:
    
    PRUint32 mMap[8];
  };


public:
  nsUrlClassifierUtils();
  ~nsUrlClassifierUtils() {}

  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUTILS

  
  
  
  
  
  
  PRBool SpecialEncode(const nsACString & url, nsACString & _retval);

private:
  
  nsUrlClassifierUtils(const nsUrlClassifierUtils&);

  
  PRBool ShouldURLEscape(const unsigned char c) const;

  nsAutoPtr<Charmap> mEscapeCharmap;
};

#endif 
