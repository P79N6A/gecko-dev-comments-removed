



































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
class nsIEditor;



#define NS_IHTMLDOCUMENT_IID \
{ 0xbfd644d6, 0x92cc, 0x4560, \
  { 0xa3, 0x29, 0xf0, 0x2b, 0xa0, 0xc9, 0x1c, 0xa5 } }




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

  




  virtual nsresult GetDocumentAllResult(const nsAString& aID,
                                        nsISupports** aResult) = 0;

  



  virtual nsIContent* GetBodyContentExternal() = 0;

  


  virtual void TearingDownEditor(nsIEditor *aEditor) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLDocument, NS_IHTMLDOCUMENT_IID)

#endif 
