






































#ifndef _NSKEYGENHANDLER_H_
#define _NSKEYGENHANDLER_H_

#include "nsIFormProcessor.h" 

typedef struct SECKeySizeChoiceInfoStr {
    PRUnichar *name;
    int size;
} SECKeySizeChoiceInfo;

nsresult GetSlotWithMechanism(PRUint32 mechanism,
                              nsIInterfaceRequestor *ctx,
                              PK11SlotInfo **retSlot);

#define DEFAULT_RSA_KEYGEN_PE 65537L
#define DEFAULT_RSA_KEYGEN_ALG SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION

SECKEYECParams *decode_ec_params(char *curve);

class nsKeygenFormProcessor : public nsIFormProcessor { 
public: 
  nsKeygenFormProcessor(); 
  virtual ~nsKeygenFormProcessor();
  nsresult Init();

  NS_IMETHOD ProcessValue(nsIDOMHTMLElement *aElement, 
                          const nsAString& aName, 
                          nsAString& aValue); 

  NS_IMETHOD ProvideContent(const nsAString& aFormType, 
                            nsVoidArray& aContent, 
                            nsAString& aAttribute); 
  NS_DECL_ISUPPORTS 

  static NS_METHOD Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);

protected:
  nsresult GetPublicKey(nsAString& aValue, nsAString& aChallenge, 
			nsAFlatString& akeyType, nsAString& aOutPublicKey,
			nsAString& aPqg);
  nsresult GetSlot(PRUint32 aMechanism, PK11SlotInfo** aSlot);
private:
  nsCOMPtr<nsIInterfaceRequestor> m_ctx;

}; 

#endif 
