




































#ifndef nsAHtml5EncodingDeclarationHandler_h_
#define nsAHtml5EncodingDeclarationHandler_h_

class nsAHtml5EncodingDeclarationHandler {
  public:
  
    virtual void internalEncodingDeclaration(nsString* aEncoding) = 0;
    
    virtual ~nsAHtml5EncodingDeclarationHandler() {
    }
};

#endif 
