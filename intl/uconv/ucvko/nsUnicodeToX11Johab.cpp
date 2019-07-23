









































#include "nsUnicodeToX11Johab.h"
#include "nsUCvKODll.h"


typedef char byte;


NS_IMPL_ADDREF(nsUnicodeToX11Johab)
NS_IMPL_RELEASE(nsUnicodeToX11Johab)
nsresult nsUnicodeToX11Johab::QueryInterface(REFNSIID aIID,
                                          void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtr = NULL;

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

  if (aIID.Equals(NS_GET_IID(nsIUnicodeEncoder))) {
    *aInstancePtr = (void*) ((nsIUnicodeEncoder*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsICharRepresentable))) {
    *aInstancePtr = (void*) ((nsICharRepresentable*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) ((nsISupports*)((nsIUnicodeEncoder*)this));
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}


NS_IMETHODIMP nsUnicodeToX11Johab::SetOutputErrorBehavior(
      PRInt32 aBehavior,
      nsIUnicharEncoder * aEncoder, PRUnichar aChar)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}










































#define START 1
#define LEADING_CONSONANT 1
#define VOWEL 2



nsUnicodeToX11Johab::nsUnicodeToX11Johab()
{
   Reset();
   state = START;
   l = 0x5f;
   v = 0;
   t = 0;

}
nsUnicodeToX11Johab::~nsUnicodeToX11Johab()
{
}


































#define canConvert(ch) \
 (((0xac00 <=(ch))&&((ch)<= 0xd7a3))    /* Modern hangul syllables         */\
   || ((0x1100 <=(ch))&&((ch)<= 0x1112)) /* modern leading consonants (19)  */\
   || ((0x1113 <=(ch))&&((ch)<= 0x1159) /* ancient leading consonants (71) */\
       && (lconBase[ch-0x1100] != 0))                                        */\
   || ((ch) == 0x115f)                 /* leading consonants filler       */\
   || ((0x1160 <=(ch))&&((ch)<= 0x1175))  /* modern vowels (21)              */\
   || ((0x1176 <=(ch))&&((ch)<= 0x11a2) /* ancient vowels (45)             */\
       && (vowBase[(ch)-0x1160] != 0  ))                                        */\
   || ((0x11a8 <=(ch))&&((ch)<= 0x11c2))/* modern trailing consonants (27) */\
   || ((0x11c3 <=(ch))&&((ch)<= 0x11f9) /* ancient trailing consonants (55)*/\
       && (tconBase[(ch)-0x11a7] != 0 )))











NS_IMETHODIMP nsUnicodeToX11Johab::Convert(
      const PRUnichar * input, PRInt32 * aSrcLength,
      char * output, PRInt32 * aDestLength)
{


                charOff = byteOff = 0;

      for (; charOff < *aSrcLength; charOff++)
      {
          PRUnichar ch = input[charOff];
          if (0xac00 <= ch && ch <= 0xd7a3)
          {
              if ( state != START )
                  composeHangul(output);
              ch -= 0xac00;
              l = (ch / 588);        
              v = ( ch / 28 ) % 21  + 1;
              t = ch % 28;
              composeHangul(output);
          } else if (0x1100 <= ch && ch <= 0x115f)
          {  
              if ( state != START )
                  composeHangul(output);
              l = ch - 0x1100;
              state = LEADING_CONSONANT;
          } else if (1160 <= ch && ch <= 0x11a2)
          {  
              v = ch - 0x1160;
              state = VOWEL;
          } else if (0x11a8 <= ch && ch <= 0x11f9)
          {  
              t = ch - 0x11a7;
              composeHangul(output);



           }
       }

       if ( state != START )
           composeHangul( output );



                 *aDestLength = byteOff;
                 return NS_OK;
}




NS_IMETHODIMP nsUnicodeToX11Johab::Finish(
      char * output, PRInt32 * aDestLength)
{
      byteOff = 0;
      PRInt32 len = 0;
      if ( state != START )
      {
          composeHangul( output );
          len = byteOff;
      }
      byteOff = charOff = 0;

                *aDestLength = len;


   return NS_OK;
}


NS_IMETHODIMP nsUnicodeToX11Johab::Reset()

  {
      byteOff = charOff = 0;
      state = START;
      l = 0x5f;
      v = t = 0;
               return NS_OK;
  }







NS_IMETHODIMP nsUnicodeToX11Johab::GetMaxLength(
      const PRUnichar * aSrc, PRInt32 aSrcLength,
      PRInt32 * aDestLength)
{
   *aDestLength = (aSrcLength + 1) *  6;
   return NS_OK;
}






static const PRUint16 lconBase[] = {
       
       1, 11, 21, 31, 41, 51,
       61, 71, 81, 91, 101, 111,
       121, 131, 141, 151, 161, 171,
       181, 

       
       0, 0, 0, 0, 0, 0,       
       0, 0, 0, 0, 0, 201,     
       0, 221, 251, 0, 0, 0,   
       0, 0, 281, 0, 0, 0,     
       191, 0, 211, 0, 231, 0, 
       0, 241, 0, 0, 0, 291,   
       0, 0, 0, 0, 0, 0,       
       0, 0, 0, 261, 0, 0,     
       0, 0, 0, 0, 0, 0,       
       0, 0, 0, 271, 0, 0,     
       0, 0, 0, 0, 0, 0,       
       0, 0, 0, 0, 301,        
       0, 0, 0, 0, 0,          
       0,                      
  };





static const PRUint16 vowBase[] = {
      
      0,311,314,317,320,323,   
      326,329,332,335,339,343, 
      347,351,355,358,361,364, 
      367,370,374,378,         
 
      
      0, 0, 0, 0, 0, 0,        
      0, 0, 0, 0, 0, 0,        
      0, 0, 381, 384, 0, 0,    
      387, 0, 0, 0, 0, 0,      
      0, 0, 0, 390, 393, 0,    
      396, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 399, 0,      
      0, 402, 0                
  };





static const PRUint16 tconBase[] = {

      0, 
      405, 409, 413, 417, 421,
      425, 429, 433, 437, 441,
      445, 459, 453, 457, 461,
      465, 469, 473, 477, 481,
      485, 489, 493, 497, 501,
      505, 509,


      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 513, 517,  
      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 0, 0, 0,      
      0, 0, 0, 525, 0, 0,    
      0, 0, 0, 0, 0, 0,      
      521                    
  };






static const PRUint8 lconMap1[] = {
      0,0,0,0,0,0,     
      0,0,0,1,3,3,     
      3,1,2,4,4,4,     
      2,1,3,0,         


      3, 4, 3, 3, 3, 4,   
      4, 3, 4, 3, 3, 3,   
      1, 1, 3, 3, 3, 1,   
      3, 4, 4, 4, 4, 2,   
      3, 3, 3, 3, 3, 2,   
      4, 2, 2, 4, 0, 0,   
      3, 4, 3, 0, 1, 3,   
      2, 3, 1             
  };





static const PRUint8 lconMap2[] = {

      5,5,5,5,5,5,     
      5,5,5,6,8,8,     
      8,6,7,9,9,9,     
      7,6,8,5,         


      8, 9, 8, 8, 8, 9,   
      9, 8, 9, 8, 8, 8,   
      6, 6, 8, 8, 8, 6,   
      8, 9, 9, 9, 9, 7,   
      8, 8, 8, 8, 8, 7,   
      9, 7, 7, 9, 5, 5,   
      8, 9, 8, 5, 6, 8,   
      7, 8, 6             
  };




static const PRUint8 vowType[] = {
      0,0,0,0,0,0,
      0,0,0,1,1,1,
      1,1,0,0,0,0,
      0,1,1,0,


      1, 0, 1, 1, 1, 0,   
      0, 1, 0, 1, 1, 1,   
      1, 1, 0, 0, 0, 0,   
      0, 0, 0, 0, 0, 0,   
      0, 0, 0, 0, 0, 0,   
      0, 0, 0, 0, 0, 0,   
      0, 0, 0, 0, 0, 0,   
      0, 0, 0             
  };





static const PRUint8 tconType[] = {
      0, 1, 1, 1, 2, 1,
      1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1,
      1, 1, 1, 1,


      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1, 1, 1, 1, 1, 1,  
      1                  
  };





static const PRUint8 tconMap[] = {
      0, 0, 2, 0, 2, 1,  
      2, 1, 2, 3, 0, 0,  
      0, 3, 3, 1, 1, 1,  
      3, 3, 0, 1,        


      3, 3, 3, 3, 3, 3,   
      3, 3, 3, 1, 0, 0,   
      3, 3, 3, 1, 0, 3,   
      0, 0, 0, 0, 0, 3,   
      0, 1, 1, 1, 1, 3,   
      1, 3, 3, 3, 2, 2,   
      3, 3, 3, 1, 3, 0,   
      3, 2, 3             
  };



void nsUnicodeToX11Johab::composeHangul(char* output)
  {

	        PRUint16 ind;
 
      if ( lconBase[l] != 0 )
      {   
          ind = lconBase[l] + ( t > 0 ? lconMap2[v] : lconMap1[v] );
          output[byteOff++] = (byte) (ind / 256);
          output[byteOff++] = (byte) (ind % 256);
      }

      if ( vowBase[v] != 0 )
      {   
          ind = vowBase[v];
          if ( vowType[v] == 1)
          {   
              
              ind += ( (l == 0 || l == 15) ? 0 : 1)
                     + (t > 0 ?  2 : 0 );
          }
          else
          { 
              ind += tconType[t];
          }

          output[byteOff++] = (byte) (ind / 256);
          output[byteOff++] = (byte) (ind % 256);
      }

      if ( tconBase[t] != 0 )  
      {   
          ind = tconBase[t] + tconMap[v];
          output[byteOff++] = (byte) (ind / 256);
          output[byteOff++] = (byte) (ind % 256);
      } else  if (vowBase[v] == 0) 
      {   
          output[byteOff++] = (byte) 0;
          output[byteOff++] = (byte) 0;
      }

      state = START;
      l = 0x5f;
      v = t = 0;
  }



NS_IMETHODIMP nsUnicodeToX11Johab::FillInfo(PRUint32* aInfo)
{
   
   PRUint32 b = 0xac00 >> 5;
   PRUint32 e = 0xd7a3 >> 5;
   aInfo[ e ] |= (0xFFFFFFFFL >> (31 - ((0xd7a3) & 0x1f)));
   for( ; b < e ; b++)
      aInfo[b] |= 0xFFFFFFFFL;

   PRUnichar i;

   
   for(i=0x1100;i<=0x1112;i++)
      SET_REPRESENTABLE(aInfo, i);
   
   for(i=0x1113;i<=0x1159;i++)
      if(lconBase[i-0x1100]!=0)
         SET_REPRESENTABLE(aInfo, i);
   
   SET_REPRESENTABLE(aInfo, 0x115f);
   
   for(i=0x1160;i<=0x1175;i++)
      SET_REPRESENTABLE(aInfo, i);
   
   for(i=0x1176;i<=0x11a2;i++)
      if(vowBase[i-0x1160]!=0)
         SET_REPRESENTABLE(aInfo, i);
   
   for(i=0x11a8;i<=0x11c2;i++)
      SET_REPRESENTABLE(aInfo, i);
   
   for(i=0x11c3;i<=0x11f9;i++)
      if(tconBase[i-0x11a7]!=0)
         SET_REPRESENTABLE(aInfo, i);
   return NS_OK;
}
