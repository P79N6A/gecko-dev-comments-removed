




#ifndef nsIHTMLDocument_h
#define nsIHTMLDocument_h

#include "nsISupports.h"
#include "nsCompatibility.h"

class nsIContent;
class nsIEditor;
class nsContentList;

#define NS_IHTMLDOCUMENT_IID \
{ 0xcf814492, 0x303c, 0x4718, \
  { 0x9a, 0x3e, 0x39, 0xbc, 0xd5, 0x2c, 0x10, 0xdb } }




class nsIHTMLDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLDOCUMENT_IID)

  


  virtual void SetCompatibilityMode(nsCompatibility aMode) = 0;

  



  virtual void AddedForm() = 0;
  



  virtual void RemovedForm() = 0;
  



  
  
  virtual int32_t GetNumFormsSynchronous() = 0;
  
  virtual bool IsWriting() = 0;

  


  virtual nsContentList* GetForms() = 0;

  



  virtual nsContentList* GetFormControls() = 0;

  








  virtual nsresult ChangeContentEditableCount(nsIContent *aElement,
                                              int32_t aChange) = 0;

  enum EditingState {
    eTearingDown = -2,
    eSettingUp = -1,
    eOff = 0,
    eDesignMode,
    eContentEditable
  };

  


  bool IsEditingOn()
  {
    return GetEditingState() == eDesignMode ||
           GetEditingState() == eContentEditable;
  }

  



  virtual EditingState GetEditingState() = 0;

  




  virtual nsresult SetEditingState(EditingState aState) = 0;

  


  virtual void DisableCookieAccess() = 0;

  


  virtual void TearingDownEditor(nsIEditor *aEditor) = 0;

  virtual void SetIsXHTML(bool aXHTML) = 0;

  virtual void SetDocWriteDisabled(bool aDisabled) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLDocument, NS_IHTMLDOCUMENT_IID)

#endif 
