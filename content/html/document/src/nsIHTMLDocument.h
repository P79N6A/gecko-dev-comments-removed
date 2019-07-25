




































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

namespace mozilla {
namespace dom {
class Element;
} 
} 

#define NS_IHTMLDOCUMENT_IID \
{ 0x8cc90664, 0xb0fe, 0x4cdb, \
 { 0xa2, 0xdd, 0x25, 0xcd, 0x8c, 0x2b, 0xfd, 0x08 } }





class nsIHTMLDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLDOCUMENT_IID)

  virtual mozilla::dom::Element* GetImageMap(const nsAString& aMapName) = 0;

  


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

  



  virtual nsIContent* GetBodyContentExternal() = 0;

  


  virtual void TearingDownEditor(nsIEditor *aEditor) = 0;

  virtual void SetIsXHTML(PRBool aXHTML) = 0;

  virtual void SetDocWriteDisabled(PRBool aDisabled) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLDocument, NS_IHTMLDOCUMENT_IID)

#endif 
