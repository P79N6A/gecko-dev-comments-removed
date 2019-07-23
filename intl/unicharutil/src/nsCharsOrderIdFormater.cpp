




































#include "pratom.h"
#include "nsUUDll.h"
#include "nsCharsOrderIdFormater.h"


nsCharsOrderIdFormater::nsCharsOrderIdFormater( nsCharsList* aList)
{
  mList = aList;
  mBase = aList->Length();
}

nsCharsOrderIdFormater::~nsCharsOrderIdFormater()
{
  delete mList;
}

NS_IMETHOD ToString( PRUint32 aOrder, nsString& aResult) 
{
  aResult = "";

  PRUint32 current;
  PRUint32 remain = aOrder;

  do {
    current = aOrder % mBase;
    remain = aOrder / mBase;
    aResult.Insert(mList->Get(current),0); 
   
  } while( remain != 0);
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsCharsOrderIdFormater, nsIOrderIdFormater)

class nsCharsOrderIdFormaterFactory : public nsIFactory {
  NS_DECL_ISUPPORTS
public:
  nsCharsOrderIdFormaterFactory(const nsCID &aCID) {
    mCID = aCID;
  };
  virtual ~nsCharsOrderIdFormaterFactory() {
  };
  NS_IMETHOD CreateInstance(nsISupports *aDelegate,
                            const nsIID &aIID,
                            void **aResult);
  NS_IMETHOD LockFactory(PRBool aLock) {
    return NS_OK;
  };

private:
  nsCID mCID;
}

NS_IMPL_ISUPPORTS1(nsCharsOrderIdFormaterFactory, nsIFactory)

NS_IMETHODIMP nsCharsOrderIdFormaterFactory::CreateInstance(
   nsISupports *aDelegate,
   const nsIID &aIID,
   void **aResult)
{
   if(NULL == aResult)
     return NS_ERROR_NULL_POINTER;
   *aResult = NULL;

   nsISupports *inst = nsnull;
   if(mCID.Equals(kLowerAToZOrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('a', 'z'));
   } else if(mCID.Equals(kUpperAToZOrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('A', 'Z'));
   } else if(mCID.Equals(k0To9OrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('0', '9'));
   } else if(mCID.Equals(kHeavenlyStemOrderIdCID)) {
      static PRUnichar gHeavenlyStemList[] = {
          0x7532, 0x4e59, 0x4e19, 0x4e01, 0x620a,
          0x5df1, 0x5e9a, 0x8f9b, 0x58ce, 0x7678        
      };
      nsAutoString tmp(gHeavenlyStemList, 
                       sizeof(gHeavenlyStemList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kEarthlyBranchOrderIdCID)) {
      static PRUnichar gEarthlyBranchList[] = {
          0x5b50, 0x4e11, 0x5bc5, 0x536f, 0x8fb0, 0x5df3,
          0x5348, 0x672a, 0x7533, 0x9149, 0x620c, 0x4ea5
      };
      nsAutoString tmp(gEarthlyBranchList, 
                       sizeof(gEarthlyBranchList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kBoPoMoFoOrderIdCID)) {
      
      
      
      
      
      inst = new nsCharOrderIdFormater(new nsRangeCharsList(0x3105, 0x3129));
   } else if(mCID.Equals(kKatakanaOrderIdCID)) {
      
      static PRUnichar gKatakanaList[] = {
         0x3042, 0x3044, 0x3046, 0x3048, 0x304a
      };
      nsAutoString tmp(gKatakanaClassList, sizeof(gKatakanaList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kHiraganaOrderIdCID)) {
      
      static PRUnichar gHiraganaList[] = {
         0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa
      };
      nsAutoString tmp(gHiraganaList, sizeof(gHiraganaList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else {
      return NS_ERROR_ILLEGAL_VALUE;
   }


   if(NULL == inst )
     return NS_ERROR_OUT_OF_MEMORY;
   nsresult res = inst->QueryInterface(aIID, aResult);
   if(NS_FAILED(res)) {
     delete inst;
   }
   return res;
}
    
