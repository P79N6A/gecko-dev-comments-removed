








































#ifndef _mozTXTToHTMLConv_h__
#define _mozTXTToHTMLConv_h__

#include "mozITXTToHTMLConv.h"
#include "nsIIOService.h"
#include "nsString.h"
#include "nsCOMPtr.h"


class mozTXTToHTMLConv : public mozITXTToHTMLConv
{



public:


  mozTXTToHTMLConv();
  virtual ~mozTXTToHTMLConv();
  NS_DECL_ISUPPORTS

  NS_DECL_MOZITXTTOHTMLCONV
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSISTREAMCONVERTER




  void ScanTXT(const PRUnichar * aInString, PRInt32 aInStringLength, PRUint32 whattodo, nsString& aOutString);




  void ScanHTML(nsString& aInString, PRUint32 whattodo, nsString &aOutString);




  PRInt32 CiteLevelTXT(const PRUnichar * line,PRUint32& logLineStart);



protected:

  nsCOMPtr<nsIIOService> mIOService; 











  void CompleteAbbreviatedURL(const PRUnichar * aInString, PRInt32 aInLength, 
                              const PRUint32 pos, nsString& aOutString);



private:


  enum LIMTYPE
  {
    LT_IGNORE,     
    LT_DELIMITER,  
    LT_ALPHA,      
    LT_DIGIT
  };













  bool ItMatchesDelimited(const PRUnichar * aInString, PRInt32 aInLength,
      const PRUnichar * rep, PRInt32 aRepLen, LIMTYPE before, LIMTYPE after);





  PRUint32 NumberOfMatches(const PRUnichar * aInString, PRInt32 aInStringLength,
      const PRUnichar* rep, PRInt32 aRepLen, LIMTYPE before, LIMTYPE after);











  void EscapeChar(const PRUnichar ch, nsString& aStringToAppendto,
                  bool inAttribute);




  void EscapeStr(nsString& aInString, bool inAttribute);








  void UnescapeStr(const PRUnichar * aInString, PRInt32 aStartPos,
                   PRInt32 aLength, nsString& aOutString);
























  bool FindURL(const PRUnichar * aInString, PRInt32 aInLength, const PRUint32 pos,
          const PRUint32 whathasbeendone,
          nsString& outputHTML, PRInt32& replaceBefore, PRInt32& replaceAfter);

  enum modetype {
         unknown,
         RFC1738,          

         RFC2396E,         




         freetext,         






         abbreviated       


      

  };








  bool FindURLStart(const PRUnichar * aInString, PRInt32 aInLength, const PRUint32 pos,
            	               const modetype check, PRUint32& start);








  bool FindURLEnd(const PRUnichar * aInString, PRInt32 aInStringLength, const PRUint32 pos,
           const modetype check, const PRUint32 start, PRUint32& end);











  void CalculateURLBoundaries(const PRUnichar * aInString, PRInt32 aInStringLength, 
     const PRUint32 pos, const PRUint32 whathasbeendone,
     const modetype check, const PRUint32 start, const PRUint32 end,
     nsString& txtURL, nsString& desc,
     PRInt32& replaceBefore, PRInt32& replaceAfter);






  bool CheckURLAndCreateHTML(
       const nsString& txtURL, const nsString& desc, const modetype mode,
       nsString& outputHTML);



















  bool StructPhraseHit(const PRUnichar * aInString, PRInt32 aInStringLength, bool col0,
     const PRUnichar* tagTXT,
     PRInt32 aTagTxtLen, 
     const char* tagHTML, const char* attributeHTML,
     nsString& aOutputString, PRUint32& openTags);








  bool
         SmilyHit(const PRUnichar * aInString, PRInt32 aLength, bool col0,
         const char* tagTXT, const char* imageName,
         nsString& outputHTML, PRInt32& glyphTextLen);
















  bool GlyphHit(const PRUnichar * aInString, PRInt32 aInLength, bool col0,
       nsString& aOutString, PRInt32& glyphTextLen);





  bool ShouldLinkify(const nsCString& aURL);
};


const PRInt32 mozTXTToHTMLConv_lastMode = 4;
	                        
const PRInt32 mozTXTToHTMLConv_numberOfModes = 4;  

#endif
