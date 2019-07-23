






































#include "nsString.h"
#include "nsIComponentManager.h"
#include "nsEditorController.h"
#include "nsEditorCommands.h"
#include "nsIControllerCommandTable.h"



#define NS_REGISTER_ONE_COMMAND(_cmdClass, _cmdName)                                      \
  {                                                                                       \
    _cmdClass* theCmd;                                                                    \
    NS_NEWXPCOM(theCmd, _cmdClass);                                                       \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                                           \
    rv = inCommandTable->RegisterCommand(_cmdName,                                        \
                                   static_cast<nsIControllerCommand *>(theCmd));          \
  }

#define NS_REGISTER_FIRST_COMMAND(_cmdClass, _cmdName)                                    \
  {                                                                                       \
    _cmdClass* theCmd;                                                                    \
    NS_NEWXPCOM(theCmd, _cmdClass);                                                       \
    if (!theCmd) return NS_ERROR_OUT_OF_MEMORY;                                           \
    rv = inCommandTable->RegisterCommand(_cmdName,                                        \
                                   static_cast<nsIControllerCommand *>(theCmd));

#define NS_REGISTER_NEXT_COMMAND(_cmdClass, _cmdName)                                     \
    rv = inCommandTable->RegisterCommand(_cmdName,                                        \
                                   static_cast<nsIControllerCommand *>(theCmd));

#define NS_REGISTER_LAST_COMMAND(_cmdClass, _cmdName)                                     \
    rv = inCommandTable->RegisterCommand(_cmdName,                                        \
                                   static_cast<nsIControllerCommand *>(theCmd));          \
  }



nsresult nsEditorController::RegisterEditorCommands(nsIControllerCommandTable *inCommandTable)
{
  nsresult rv;
 
  
  
  
  NS_REGISTER_ONE_COMMAND(nsUndoCommand, "cmd_undo");
  NS_REGISTER_ONE_COMMAND(nsRedoCommand, "cmd_redo");
  NS_REGISTER_ONE_COMMAND(nsClearUndoCommand, "cmd_clearUndo");

  NS_REGISTER_ONE_COMMAND(nsCutCommand, "cmd_cut");
  NS_REGISTER_ONE_COMMAND(nsCutOrDeleteCommand, "cmd_cutOrDelete");
  NS_REGISTER_ONE_COMMAND(nsCopyCommand, "cmd_copy");
  NS_REGISTER_ONE_COMMAND(nsCopyOrDeleteCommand, "cmd_copyOrDelete");
  NS_REGISTER_ONE_COMMAND(nsSelectAllCommand, "cmd_selectAll");
  
  NS_REGISTER_ONE_COMMAND(nsPasteCommand, "cmd_paste");
  NS_REGISTER_ONE_COMMAND(nsSwitchTextDirectionCommand, "cmd_switchTextDirection");
  
  NS_REGISTER_FIRST_COMMAND(nsDeleteCommand, "cmd_delete");
  NS_REGISTER_NEXT_COMMAND(nsDeleteCommand, "cmd_deleteCharBackward");
  NS_REGISTER_NEXT_COMMAND(nsDeleteCommand, "cmd_deleteCharForward");
  NS_REGISTER_NEXT_COMMAND(nsDeleteCommand, "cmd_deleteWordBackward");
  NS_REGISTER_NEXT_COMMAND(nsDeleteCommand, "cmd_deleteWordForward");
  NS_REGISTER_NEXT_COMMAND(nsDeleteCommand, "cmd_deleteToBeginningOfLine");
  NS_REGISTER_LAST_COMMAND(nsDeleteCommand, "cmd_deleteToEndOfLine");

  NS_REGISTER_FIRST_COMMAND(nsSelectionMoveCommands, "cmd_scrollTop");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_scrollBottom");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_moveTop");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_moveBottom");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectTop");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectBottom");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_lineNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_linePrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectLineNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectLinePrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_charPrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_charNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectCharPrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectCharNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_beginLine");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_endLine");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectBeginLine");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectEndLine");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_wordPrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_wordNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectWordPrevious");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectWordNext");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_scrollPageUp");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_scrollPageDown");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_scrollLineUp");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_scrollLineDown");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_movePageUp");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_movePageDown");
  NS_REGISTER_NEXT_COMMAND(nsSelectionMoveCommands, "cmd_selectPageUp");
  NS_REGISTER_LAST_COMMAND(nsSelectionMoveCommands, "cmd_selectPageDown");
    
  
  NS_REGISTER_ONE_COMMAND(nsInsertPlaintextCommand, "cmd_insertText");
  NS_REGISTER_ONE_COMMAND(nsPasteQuotationCommand,  "cmd_pasteQuote");


  return NS_OK;
}

