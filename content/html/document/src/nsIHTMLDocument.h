




































#ifndef nsIHTMLDocument_h
#define nsIHTMLDocument_h

#include "nsISupports.h"
#include "nsCompatibility.h"

class nsIDOMHTMLFormElement;
class nsIContent;
class nsIScriptElement;
class nsIEditor;
class nsContentList;
class nsWrapperCache;

#define NS_IHTMLDOCUMENT_IID \
{ 0x51a360fa, 0xd659, 0x4d85, \
  { 0xa5, 0xc5, 0x4a, 0xbb, 0x0d, 0x97, 0x0f, 0x7a } }





class nsIHTMLDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLDOCUMENT_IID)

  


  virtual void SetCompatibilityMode(nsCompatibility aMode) = 0;

  virtual nsresult ResolveName(const nsAString& aName,
                               nsIContent *aForm,
                               nsISupports **aResult,
                               nsWrapperCache **aCache) = 0;

  



  virtual void ScriptLoading(nsIScriptElement *aScript) = 0;

  



  virtual void ScriptExecuted(nsIScriptElement *aScript) = 0;

  



  virtual void AddedForm() = 0;
  



  virtual void RemovedForm() = 0;
  



  
  
  virtual PRInt32 GetNumFormsSynchronous() = 0;
  
  virtual PRBool IsWriting() = 0;

  virtual PRBool GetIsFrameset() = 0;
  virtual void SetIsFrameset(PRBool aFrameset) = 0;

  


  virtual nsContentList* GetForms() = 0;

  



  virtual nsContentList* GetFormControls() = 0;

  








  virtual nsresult ChangeContentEditableCount(nsIContent *aElement,
                                              PRInt32 aChange) = 0;

  enum EditingState {
    eTearingDown = -2,
    eSettingUp = -1,
    eOff = 0,
    eDesignMode,
    eContentEditable
  };

  


  PRBool IsEditingOn()
  {
    return GetEditingState() == eDesignMode ||
           GetEditingState() == eContentEditable;
  }

  



  virtual EditingState GetEditingState() = 0;

  




  virtual nsresult SetEditingState(EditingState aState) = 0;

  


  virtual void DisableCookieAccess() = 0;

  


  virtual void TearingDownEditor(nsIEditor *aEditor) = 0;

  virtual void SetIsXHTML(PRBool aXHTML) = 0;

  virtual void SetDocWriteDisabled(PRBool aDisabled) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLDocument, NS_IHTMLDOCUMENT_IID)

#endif 
