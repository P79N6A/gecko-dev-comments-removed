




#ifndef nsComposerCommands_h_
#define nsComposerCommands_h_

#include "nsIControllerCommand.h"
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsIAtom;
class nsICommandParams;
class nsIEditor;
class nsISupports;
class nsString;










class nsBaseComposerCommand : public nsIControllerCommand
{
protected:
  virtual ~nsBaseComposerCommand() {}

public:

  nsBaseComposerCommand();

  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD IsCommandEnabled(const char * aCommandName, nsISupports *aCommandRefCon, bool *_retval) = 0;
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
  explicit nsBaseStateUpdatingCommand(nsIAtom* aTagName);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICONTROLLERCOMMAND

protected:
  virtual ~nsBaseStateUpdatingCommand();

  
  virtual nsresult  GetCurrentState(nsIEditor* aEditor, nsICommandParams* aParams) = 0;
  
  
  virtual nsresult  ToggleState(nsIEditor* aEditor) = 0;

protected:
  nsIAtom* mTagName;
};




class nsStyleUpdatingCommand : public nsBaseStateUpdatingCommand
{
public:
  explicit nsStyleUpdatingCommand(nsIAtom* aTagName);
           
protected:

  
  virtual nsresult  GetCurrentState(nsIEditor* aEditor, nsICommandParams* aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor* aEditor);
};


class nsInsertTagCommand : public nsBaseComposerCommand
{
public:
  explicit nsInsertTagCommand(nsIAtom* aTagName);
    
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSICONTROLLERCOMMAND

protected:
  virtual ~nsInsertTagCommand();

  nsIAtom* mTagName;
};


class nsListCommand : public nsBaseStateUpdatingCommand
{
public:
  explicit nsListCommand(nsIAtom* aTagName);

protected:

  
  virtual nsresult  GetCurrentState(nsIEditor* aEditor, nsICommandParams* aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor* aEditor);
};

class nsListItemCommand : public nsBaseStateUpdatingCommand
{
public:
  explicit nsListItemCommand(nsIAtom* aTagName);

protected:

  
  virtual nsresult  GetCurrentState(nsIEditor* aEditor, nsICommandParams* aParams);
  
  
  virtual nsresult  ToggleState(nsIEditor* aEditor);
};


class nsMultiStateCommand : public nsBaseComposerCommand
{
public:
  
  nsMultiStateCommand();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICONTROLLERCOMMAND

protected:
  virtual ~nsMultiStateCommand();

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

  NS_IMETHOD IsCommandEnabled(const char *aCommandName, nsISupports *aCommandRefCon, bool *_retval);
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

  NS_IMETHOD IsCommandEnabled(const char *aCommandName, nsISupports *aCommandRefCon, bool *_retval);
  virtual nsresult  GetCurrentState(nsIEditor* aEditor, nsICommandParams* aParams);
  virtual nsresult  ToggleState(nsIEditor* aEditor);
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
