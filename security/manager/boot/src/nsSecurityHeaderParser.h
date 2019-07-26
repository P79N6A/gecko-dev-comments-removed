



#ifndef nsSecurityHeaderParser_h__
#define nsSecurityHeaderParser_h__

#include "nsString.h"
#include "mozilla/LinkedList.h"
#include "nsCOMPtr.h"


class nsSecurityHeaderDirective : public mozilla::LinkedListElement<nsSecurityHeaderDirective> {
public:
  nsAutoCString mName;
  nsAutoCString mValue;
};



















class nsSecurityHeaderParser {
public:
  nsSecurityHeaderParser(const char *aHeader);
  ~nsSecurityHeaderParser();

  
  nsresult Parse();
  
  mozilla::LinkedList<nsSecurityHeaderDirective> *GetDirectives();

private:
  bool Accept(char aChr);
  bool Accept(bool (*aClassifier) (signed char));
  void Expect(char aChr);
  void Advance();
  void Header();         
  void Directive();      
  void DirectiveName();  
  void DirectiveValue(); 
  void Token();          
  void QuotedString();   
  void QuotedText();     
  void QuotedPair();     

                         
  void LWSMultiple();    
  void LWSCRLF();        
  void LWS();            

  mozilla::LinkedList<nsSecurityHeaderDirective> mDirectives;
  const char *mCursor;
  nsSecurityHeaderDirective *mDirective;

  nsAutoCString mOutput;
  bool mError;
};

#endif 
