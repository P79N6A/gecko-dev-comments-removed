







































#include "nsIControllerCommandTable.h"
#include "nsComposerController.h"
#include "nsComposerCommands.h"

#define NS_REGISTER_ONE_COMMAND(_cmdClass, _cmdName)                    \
  {                                                                     \
    _cmdClass* theCmd;                                                  \
    NS_NEWXPCOM(theCmd, _cmdClass);                                     \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }

#define NS_REGISTER_FIRST_COMMAND(_cmdClass, _cmdName)                  \
  {                                                                     \
    _cmdClass* theCmd;                                                  \
    NS_NEWXPCOM(theCmd, _cmdClass);                                     \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd));

#define NS_REGISTER_NEXT_COMMAND(_cmdClass, _cmdName)                   \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                        NS_STATIC_CAST(nsIControllerCommand *, theCmd));

#define NS_REGISTER_LAST_COMMAND(_cmdClass, _cmdName)                   \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }

#define NS_REGISTER_STYLE_COMMAND(_cmdClass, _cmdName, _styleTag)       \
  {                                                                     \
    _cmdClass* theCmd = new _cmdClass(_styleTag);                       \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }
  
#define NS_REGISTER_TAG_COMMAND(_cmdClass, _cmdName, _tagName)          \
  {                                                                     \
    _cmdClass* theCmd = new _cmdClass(_tagName);                        \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                         \
    rv = inCommandTable->RegisterCommand(_cmdName,                      \
                       NS_STATIC_CAST(nsIControllerCommand *, theCmd)); \
  }
  


nsresult
nsComposerController::RegisterEditorDocStateCommands(
                        nsIControllerCommandTable *inCommandTable)
{
  nsresult rv;

  
  NS_REGISTER_FIRST_COMMAND(nsDocumentStateCommand, "obs_documentCreated")
  NS_REGISTER_NEXT_COMMAND(nsDocumentStateCommand, "obs_documentWillBeDestroyed")
  NS_REGISTER_LAST_COMMAND(nsDocumentStateCommand, "obs_documentLocationChanged")

  
  NS_REGISTER_FIRST_COMMAND(nsSetDocumentStateCommand, "cmd_setDocumentModified")
  NS_REGISTER_NEXT_COMMAND(nsSetDocumentStateCommand, "cmd_setDocumentUseCSS")
  NS_REGISTER_NEXT_COMMAND(nsSetDocumentStateCommand, "cmd_setDocumentReadOnly")
  NS_REGISTER_NEXT_COMMAND(nsSetDocumentStateCommand, "cmd_insertBrOnReturn")
  NS_REGISTER_NEXT_COMMAND(nsSetDocumentStateCommand, "cmd_enableObjectResizing")
  NS_REGISTER_LAST_COMMAND(nsSetDocumentStateCommand, "cmd_enableInlineTableEditing")

  NS_REGISTER_ONE_COMMAND(nsSetDocumentOptionsCommand, "cmd_setDocumentOptions")

  return NS_OK;
}


nsresult
nsComposerController::RegisterHTMLEditorCommands(
                        nsIControllerCommandTable *inCommandTable)
{
  nsresult rv;
  
  
  NS_REGISTER_ONE_COMMAND(nsPasteNoFormattingCommand, "cmd_pasteNoFormatting");

  
  NS_REGISTER_ONE_COMMAND(nsIndentCommand, "cmd_indent");
  NS_REGISTER_ONE_COMMAND(nsOutdentCommand, "cmd_outdent");

  
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_bold", "b");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_italic", "i");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_underline", "u");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_tt", "tt");

  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_strikethrough", "strike");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_superscript", "sup");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_subscript", "sub");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_nobreak", "nobr");

  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_em", "em");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_strong", "strong");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_cite", "cite");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_abbr", "abbr");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_acronym", "acronym");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_code", "code");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_samp", "samp");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_var", "var");
  NS_REGISTER_STYLE_COMMAND(nsStyleUpdatingCommand, "cmd_removeLinks", "href");

  
  NS_REGISTER_STYLE_COMMAND(nsListCommand,     "cmd_ol", "ol");
  NS_REGISTER_STYLE_COMMAND(nsListCommand,     "cmd_ul", "ul");
  NS_REGISTER_STYLE_COMMAND(nsListItemCommand, "cmd_dt", "dt");
  NS_REGISTER_STYLE_COMMAND(nsListItemCommand, "cmd_dd", "dd");
  NS_REGISTER_ONE_COMMAND(nsRemoveListCommand, "cmd_removeList");

  
  NS_REGISTER_ONE_COMMAND(nsParagraphStateCommand,       "cmd_paragraphState");
  NS_REGISTER_ONE_COMMAND(nsFontFaceStateCommand,        "cmd_fontFace");
  NS_REGISTER_ONE_COMMAND(nsFontSizeStateCommand,        "cmd_fontSize");
  NS_REGISTER_ONE_COMMAND(nsFontColorStateCommand,       "cmd_fontColor");
  NS_REGISTER_ONE_COMMAND(nsBackgroundColorStateCommand, "cmd_backgroundColor");
  NS_REGISTER_ONE_COMMAND(nsHighlightColorStateCommand,  "cmd_highlight");

  NS_REGISTER_ONE_COMMAND(nsAlignCommand, "cmd_align");
  NS_REGISTER_ONE_COMMAND(nsRemoveStylesCommand, "cmd_removeStyles");

  NS_REGISTER_ONE_COMMAND(nsIncreaseFontSizeCommand, "cmd_increaseFont");
  NS_REGISTER_ONE_COMMAND(nsDecreaseFontSizeCommand, "cmd_decreaseFont");

  
  NS_REGISTER_ONE_COMMAND(nsInsertHTMLCommand, "cmd_insertHTML");
  NS_REGISTER_TAG_COMMAND(nsInsertTagCommand, "cmd_insertLinkNoUI", "a");
  NS_REGISTER_TAG_COMMAND(nsInsertTagCommand, "cmd_insertImageNoUI", "img");
  NS_REGISTER_TAG_COMMAND(nsInsertTagCommand, "cmd_insertHR", "hr");

  NS_REGISTER_ONE_COMMAND(nsAbsolutePositioningCommand, "cmd_absPos");
  NS_REGISTER_ONE_COMMAND(nsDecreaseZIndexCommand, "cmd_decreaseZIndex");
  NS_REGISTER_ONE_COMMAND(nsIncreaseZIndexCommand, "cmd_increaseZIndex");

  return NS_OK;
}
