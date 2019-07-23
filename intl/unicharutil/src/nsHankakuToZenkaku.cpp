




































#include "nsITextTransform.h"
#include "pratom.h"
#include "nsUUDll.h"
#include "nsTextTransformFactory.h"



static const PRUnichar gBasicMapping[0x40] =
{

0xff60,0x3002,0x300c,0x300d,0x3001,0x30fb,0x30f2,0x30a1,        

0x30a3,0x30a5,0x30a7,0x30a9,0x30e3,0x30e5,0x30e7,0x30c3,        

0x30fc,0x30a2,0x30a4,0x30a6,0x30a8,0x30aa,0x30ab,0x30ad,        

0x30af,0x30b1,0x30b3,0x30b5,0x30b7,0x30b9,0x30bb,0x30bd,        

0x30bf,0x30c1,0x30c4,0x30c6,0x30c8,0x30ca,0x30cb,0x30cc,        

0x30cd,0x30ce,0x30cf,0x30d2,0x30d5,0x30d8,0x30db,0x30de,        

0x30df,0x30e0,0x30e1,0x30e2,0x30e4,0x30e6,0x30e8,0x30e9,        

0x30ea,0x30eb,0x30ec,0x30ed,0x30ef,0x30f3,0x309b,0x309c
};


#define NEED_TO_CHECK_NIGORI(u) (((0xff76<=(u))&&((u)<=0xff84))||((0xff8a<=(u))&&((u)<=0xff8e)))


#define NEED_TO_CHECK_MARU(u) ((0xff8a<=(u))&&((u)<=0xff8e))


#define IS_HANKAKU(u) (0xff60==((u) & 0xffe0)) || (0xff80==((u)&0xffe0))
#define IS_NIGORI(u) (0xff9e == (u))
#define IS_MARU(u)   (0xff9f == (u))
#define NIGORI_MODIFIER 1
#define MARU_MODIFIER   2
 

void HankakuToZenkaku (
    const PRUnichar* aSrc, PRInt32 aLen, 
    PRUnichar* aDest, PRInt32 aDestLen, PRInt32* oLen);

void HankakuToZenkaku (
    const PRUnichar* aSrc, PRInt32 aLen, 
    PRUnichar* aDest, PRInt32 aDestLen, PRInt32* oLen)
{
    
    NS_ASSERTION(aDestLen >= aLen, "aDest must be as long as aSrc");

    PRInt32 i,j;
    if ( aLen == 0) {
      *oLen = 0;
      return;
    }
    
    for(i = j = 0; i < (aLen-1); i++,j++,aSrc++, aDest++)
    {
       if(IS_HANKAKU(*aSrc)) {
         
         *aDest = gBasicMapping[(*aSrc) - 0xff60];
         
         
         

         if(IS_NIGORI(*(aSrc+1)) && NEED_TO_CHECK_NIGORI(*aSrc))
         {
            *aDest += NIGORI_MODIFIER;
            i++; aSrc++;
         }
         else if(IS_MARU(*(aSrc+1)) && NEED_TO_CHECK_MARU(*aSrc)) 
         {
            *aDest += MARU_MODIFIER;
            i++; aSrc++;
         }
       }
       else 
       {
         
         *aDest = *aSrc;
       }
    }

    
    if(IS_HANKAKU(*aSrc)) 
         *aDest = gBasicMapping[(*aSrc) - 0xff60];
    else
         *aDest = *aSrc;

    *oLen = j+1;
}



class nsHankakuToZenkaku : public nsITextTransform {
  NS_DECL_ISUPPORTS

public: 

  nsHankakuToZenkaku() ;
  virtual ~nsHankakuToZenkaku() ;
  NS_IMETHOD Change( const PRUnichar* aText, PRInt32 aTextLength, nsString& aResult);
  NS_IMETHOD Change( nsString& aText, nsString& aResult);

};

NS_IMPL_ISUPPORTS1(nsHankakuToZenkaku, nsITextTransform)

nsHankakuToZenkaku::nsHankakuToZenkaku()
{
}
nsHankakuToZenkaku::~nsHankakuToZenkaku()
{
}

NS_IMETHODIMP nsHankakuToZenkaku::Change( const PRUnichar* aText, PRInt32 aTextLength, nsString& aResult)
{
  PRInt32 ol;
  if (!EnsureStringLength(aResult, aTextLength))
    return NS_ERROR_OUT_OF_MEMORY;

  HankakuToZenkaku ( aText, aTextLength, aResult.BeginWriting(), aTextLength, &ol);
  aResult.SetLength(ol);

  return NS_OK;
}

NS_IMETHODIMP nsHankakuToZenkaku::Change( nsString& aText, nsString& aResult)
{
   aResult = aText;
   const PRUnichar* u = aResult.get();
   PRUnichar* ou = (PRUnichar*) u;
   PRInt32 l = aResult.Length();
   PRInt32 ol;
   
   HankakuToZenkaku ( u, l, ou, l, &ol);
   aResult.SetLength(ol);

   return NS_OK;
}

nsresult NS_NewHankakuToZenkaku(nsISupports** oResult)
{
  if(!oResult)
    return NS_ERROR_NULL_POINTER;
  *oResult = new nsHankakuToZenkaku();
  if(*oResult)
     NS_ADDREF(*oResult);
  return (*oResult) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
