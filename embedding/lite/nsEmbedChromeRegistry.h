




































#include "nsIChromeRegistry.h"
#include "nsSupportsArray.h"


#define NS_EMBEDCHROMEREGISTRY_CID \
  { 0xc3434a5, 0xd52e, 0x4bb7, \
    { 0xbe, 0xbe, 0xf5, 0x72, 0xd8, 0xed, 0xbe, 0x2b } }


class nsEmbedChromeRegistry : public nsIChromeRegistry
{
public:
    nsEmbedChromeRegistry();
    virtual ~nsEmbedChromeRegistry() {};
    nsresult Init();
    
    NS_DECL_ISUPPORTS

    NS_DECL_NSICHROMEREGISTRY

    nsresult ReadChromeRegistry();
    nsresult ProcessNewChromeBuffer(char* aBuffer, PRInt32 aLength);
    PRInt32 ProcessChromeLine(const char* aBuffer, PRInt32 aLength);
    nsresult RegisterChrome(const nsACString& aChromeType,
                            const nsACString& aChromeProfile,
                            const nsACString& aChromeLocType,
                            const nsACString& aChromeLocation);
    nsresult RegisterChrome(PRInt32 aChromeType, 
                            PRBool aChromeIsProfile, 
                            PRBool aChromeIsURL, 
                            const nsACString& aChromeLocation);


    
private:
    nsCOMPtr<nsISupportsArray> mEmptyArray;
};
