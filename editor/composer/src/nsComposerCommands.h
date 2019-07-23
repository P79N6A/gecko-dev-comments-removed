







































#ifndef nsComposerCommands_h_
#define nsComposerCommands_h_

#include "nsIControllerCommand.h"
#include "nsString.h"

class nsIEditor;










class nsBaseComposerCommand : public nsIControllerCommand
{
public:

              nsBaseComposerCommand();
  virtual     ~nsBaseComposerCommand() {}
    
  
  NS_DECL_ISUPPORTS
    
  
  NS_IMETHOD IsCommandEnabled(const char * aCommandName, nsISupports *aCommandRefCon, PRBool *_retval) = 0;
  NS_IMETHOD DoCommand(const char * aCommandName, nsISupports *aCommandRefCon) = 0;

};


#define NS_DECL_COMPOSER_COMMAND(_cmd)                  \
class _cmd : public nsBaseComposerCommand               \
{                                                       \
public:                                                 \
  NS_DECL_NSICONTROLLERCOMMAND                          \
};


class nsBaseStateUpdatingCommand : public nsBaseComposerCommand
{
public:

              nsBaseStateUpdatingCommand(const char* aTagName);
  virtual     ~nsBaseStateUpdatingCommand();
    
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICONTROLLERCOMMAND

protected:

  
  virtual nsresult  GetCurrentState(nsIEditor *aEditor, const char* aTagName, nsICommandParams *aParams) = 0;
  
  
  virtual nsresult  ToggleState(nsIEditor *aEditor, const char* aTagName) = 0;

protected:

  const char* mTagName;
};




class nsStyleUpdatingCommand : public nsBaseStateUpdatingCommand
{
public:

            nsStyleUpdatingCommand(const char* aTagName);
           
protected:

  
  virtual nsresult  GetCurrentState(nsIEditor *aEditor, const char* aTagName, nsICommandParams *aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor *aEditor, const char* aTagName);
  
};


class nsInsertTagCommand : public nsBaseComposerCommand
{
public:

              nsInsertTagCommand(const char* aTagName);
  virtual     ~nsInsertTagCommand();
    
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICONTROLLERCOMMAND

protected:

  const char* mTagName;
};


class nsListCommand : public nsBaseStateUpdatingCommand
{
public:

            nsListCommand(const char* aTagName);

protected:

  
  virtual nsresult  GetCurrentState(nsIEditor *aEditor, const char* aTagName, nsICommandParams *aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor *aEditor, const char* aTagName);
};

class nsListItemCommand : public nsBaseStateUpdatingCommand
{
public:

            nsListItemCommand(const char* aTagName);

protected:

  
  virtual nsresult  GetCurrentState(nsIEditor *aEditor, const char* aTagName, nsICommandParams *aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor *aEditor, const char* aTagName);
};


class nsMultiStateCommand : public nsBaseComposerCommand
{
public:
  
                   nsMultiStateCommand();
  virtual          ~nsMultiStateCommand();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICONTROLLERCOMMAND

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams) =0;
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState) = 0;
  
};


class nsParagraphStateCommand : public nsMultiStateCommand
{
public:
                   nsParagraphStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsFontFaceStateCommand : public nsMultiStateCommand
{
public:
                   nsFontFaceStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsFontSizeStateCommand : public nsMultiStateCommand
{
public:
                   nsFontSizeStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor,
                                   nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsHighlightColorStateCommand : public nsMultiStateCommand
{
public:
                   nsHighlightColorStateCommand();

protected:

  NS_IMETHOD IsCommandEnabled(const char *aCommandName, nsISupports *aCommandRefCon, PRBool *_retval);
  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);

};

class nsFontColorStateCommand : public nsMultiStateCommand
{
public:
                   nsFontColorStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsAlignCommand : public nsMultiStateCommand
{
public:
                   nsAlignCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsBackgroundColorStateCommand : public nsMultiStateCommand
{
public:
                   nsBackgroundColorStateCommand();

protected:

  virtual nsresult GetCurrentState(nsIEditor *aEditor, nsICommandParams* aParams);
  virtual nsresult SetState(nsIEditor *aEditor, nsString& newState);
};

class nsAbsolutePositioningCommand : public nsBaseStateUpdatingCommand
{
public:
                   nsAbsolutePositioningCommand();

protected:

  NS_IMETHOD IsCommandEnabled(const char *aCommandName, nsISupports *aCommandRefCon, PRBool *_retval);
  virtual nsresult  GetCurrentState(nsIEditor *aEditor, const char* aTagName, nsICommandParams *aParams);
  virtual nsresult  ToggleState(nsIEditor *aEditor, const char* aTagName);
};



NS_DECL_COMPOSER_COMMAND(nsCloseCommand)
NS_DECL_COMPOSER_COMMAND(nsDocumentStateCommand)
NS_DECL_COMPOSER_COMMAND(nsSetDocumentStateCommand)
NS_DECL_COMPOSER_COMMAND(nsSetDocumentOptionsCommand)


NS_DECL_COMPOSER_COMMAND(nsDecreaseZIndexCommand)
NS_DECL_COMPOSER_COMMAND(nsIncreaseZIndexCommand)




NS_DECL_COMPOSER_COMMAND(nsNewCommands)   


NS_DECL_COMPOSER_COMMAND(nsPasteNoFormattingCommand)


NS_DECL_COMPOSER_COMMAND(nsIndentCommand)
NS_DECL_COMPOSER_COMMAND(nsOutdentCommand)

NS_DECL_COMPOSER_COMMAND(nsRemoveListCommand)
NS_DECL_COMPOSER_COMMAND(nsRemoveStylesCommand)
NS_DECL_COMPOSER_COMMAND(nsIncreaseFontSizeCommand)
NS_DECL_COMPOSER_COMMAND(nsDecreaseFontSizeCommand)


NS_DECL_COMPOSER_COMMAND(nsInsertHTMLCommand)

#endif 
