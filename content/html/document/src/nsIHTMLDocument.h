



































#ifndef nsIHTMLDocument_h___
#define nsIHTMLDocument_h___

#include "nsISupports.h"
#include "nsCompatibility.h"
#include "nsContentList.h"

class nsIImageMap;
class nsString;
class nsIDOMNodeList;
class nsIDOMHTMLCollection;
class nsIDOMHTMLFormElement;
class nsIDOMHTMLMapElement;
class nsHTMLStyleSheet;
class nsIStyleSheet;
class nsICSSLoader;
class nsIContent;
class nsIDOMHTMLBodyElement;
class nsIScriptElement;

#define NS_IHTMLDOCUMENT_IID \
{ 0x61e989a8, 0x70cd, 0x4582, \
  { 0x84, 0x5e, 0x6e, 0x5e, 0x12, 0x55, 0x9a, 0x83 } }




class nsIHTMLDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLDOCUMENT_IID)

  virtual nsresult AddImageMap(nsIDOMHTMLMapElement* aMap) = 0;

  virtual nsIDOMHTMLMapElement *GetImageMap(const nsAString& aMapName) = 0;

  virtual void RemoveImageMap(nsIDOMHTMLMapElement* aMap) = 0;

  


  virtual void SetCompatibilityMode(nsCompatibility aMode) = 0;

  virtual nsresult ResolveName(const nsAString& aName,
                               nsIDOMHTMLFormElement *aForm,
                               nsISupports **aResult) = 0;

  



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

  




  virtual nsresult GetDocumentAllResult(const nsAString& aID,
                                        nsISupports** aResult) = 0;

  


  virtual void DisableCookieAccess() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLDocument, NS_IHTMLDOCUMENT_IID)

#endif 
