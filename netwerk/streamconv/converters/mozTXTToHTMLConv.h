








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




  void ScanTXT(const PRUnichar * aInString, int32_t aInStringLength, uint32_t whattodo, nsString& aOutString);




  void ScanHTML(nsString& aInString, uint32_t whattodo, nsString &aOutString);




  int32_t CiteLevelTXT(const PRUnichar * line,uint32_t& logLineStart);



protected:

  nsCOMPtr<nsIIOService> mIOService; 











  void CompleteAbbreviatedURL(const PRUnichar * aInString, int32_t aInLength, 
                              const uint32_t pos, nsString& aOutString);



private:


  enum LIMTYPE
  {
    LT_IGNORE,     
    LT_DELIMITER,  
    LT_ALPHA,      
    LT_DIGIT
  };













  bool ItMatchesDelimited(const PRUnichar * aInString, int32_t aInLength,
      const PRUnichar * rep, int32_t aRepLen, LIMTYPE before, LIMTYPE after);





  uint32_t NumberOfMatches(const PRUnichar * aInString, int32_t aInStringLength,
      const PRUnichar* rep, int32_t aRepLen, LIMTYPE before, LIMTYPE after);











  void EscapeChar(const PRUnichar ch, nsString& aStringToAppendto,
                  bool inAttribute);




  void EscapeStr(nsString& aInString, bool inAttribute);








  void UnescapeStr(const PRUnichar * aInString, int32_t aStartPos,
                   int32_t aLength, nsString& aOutString);
























  bool FindURL(const PRUnichar * aInString, int32_t aInLength, const uint32_t pos,
          const uint32_t whathasbeendone,
          nsString& outputHTML, int32_t& replaceBefore, int32_t& replaceAfter);

  enum modetype {
         unknown,
         RFC1738,          

         RFC2396E,         




         freetext,         






         abbreviated       


      

  };








  bool FindURLStart(const PRUnichar * aInString, int32_t aInLength, const uint32_t pos,
            	               const modetype check, uint32_t& start);








  bool FindURLEnd(const PRUnichar * aInString, int32_t aInStringLength, const uint32_t pos,
           const modetype check, const uint32_t start, uint32_t& end);











  void CalculateURLBoundaries(const PRUnichar * aInString, int32_t aInStringLength, 
     const uint32_t pos, const uint32_t whathasbeendone,
     const modetype check, const uint32_t start, const uint32_t end,
     nsString& txtURL, nsString& desc,
     int32_t& replaceBefore, int32_t& replaceAfter);






  bool CheckURLAndCreateHTML(
       const nsString& txtURL, const nsString& desc, const modetype mode,
       nsString& outputHTML);



















  bool StructPhraseHit(const PRUnichar * aInString, int32_t aInStringLength, bool col0,
     const PRUnichar* tagTXT,
     int32_t aTagTxtLen, 
     const char* tagHTML, const char* attributeHTML,
     nsString& aOutputString, uint32_t& openTags);








  bool
         SmilyHit(const PRUnichar * aInString, int32_t aLength, bool col0,
         const char* tagTXT, const char* imageName,
         nsString& outputHTML, int32_t& glyphTextLen);
















  bool GlyphHit(const PRUnichar * aInString, int32_t aInLength, bool col0,
       nsString& aOutString, int32_t& glyphTextLen);





  bool ShouldLinkify(const nsCString& aURL);
};


const int32_t mozTXTToHTMLConv_lastMode = 4;
	                        
const int32_t mozTXTToHTMLConv_numberOfModes = 4;  

#endif
