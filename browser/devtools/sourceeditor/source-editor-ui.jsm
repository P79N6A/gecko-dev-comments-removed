




"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["SourceEditorUI"];




this.SourceEditorUI = function SourceEditorUI(aEditor)
{
  this.editor = aEditor;
  this._onDirtyChanged = this._onDirtyChanged.bind(this);
}

SourceEditorUI.prototype = {
  



  init: function SEU_init()
  {
    this._ownerWindow = this.editor.parentElement.ownerDocument.defaultView;
  },

  




  onReady: function SEU_onReady()
  {
    if (this._ownerWindow.controllers) {
      this._controller = new SourceEditorController(this.editor);
      this._ownerWindow.controllers.insertControllerAt(0, this._controller);
      this.editor.addEventListener(this.editor.EVENTS.DIRTY_CHANGED,
                                   this._onDirtyChanged);
    }
  },

  



  gotoLine: function SEU_gotoLine()
  {
    let oldLine = this.editor.getCaretPosition ?
                  this.editor.getCaretPosition().line : null;
    let newLine = {value: oldLine !== null ? oldLine + 1 : ""};

    let result = Services.prompt.prompt(this._ownerWindow,
      SourceEditorUI.strings.GetStringFromName("gotoLineCmd.promptTitle"),
      SourceEditorUI.strings.GetStringFromName("gotoLineCmd.promptMessage"),
      newLine, null, {});

    newLine.value = parseInt(newLine.value);
    if (result && !isNaN(newLine.value) && --newLine.value != oldLine) {
      if (this.editor.getLineCount) {
        let lines = this.editor.getLineCount() - 1;
        this.editor.setCaretPosition(Math.max(0, Math.min(lines, newLine.value)));
      } else {
        this.editor.setCaretPosition(Math.max(0, newLine.value));
      }
    }

    return true;
  },

  




  find: function SEU_find()
  {
    let str = {value: this.editor.getSelectedText()};
    if (!str.value && this.editor.lastFind) {
      str.value = this.editor.lastFind.str;
    }

    let result = Services.prompt.prompt(this._ownerWindow,
      SourceEditorUI.strings.GetStringFromName("findCmd.promptTitle"),
      SourceEditorUI.strings.GetStringFromName("findCmd.promptMessage"),
      str, null, {});

    if (result && str.value) {
      let start = this.editor.getSelection().end;
      let pos = this.editor.find(str.value, {ignoreCase: true, start: start});
      if (pos == -1) {
        this.editor.find(str.value, {ignoreCase: true});
      }
      this._onFind();
    }

    return true;
  },

  


  findNext: function SEU_findNext()
  {
    let lastFind = this.editor.lastFind;
    if (lastFind) {
      this.editor.findNext(true);
      this._onFind();
    }

    return true;
  },

  


  findPrevious: function SEU_findPrevious()
  {
    let lastFind = this.editor.lastFind;
    if (lastFind) {
      this.editor.findPrevious(true);
      this._onFind();
    }

    return true;
  },

  



  _onFind: function SEU__onFind()
  {
    let lastFind = this.editor.lastFind;
    if (lastFind && lastFind.index > -1) {
      this.editor.setSelection(lastFind.index, lastFind.index + lastFind.str.length);
    }

    if (this._ownerWindow.goUpdateCommand) {
      this._ownerWindow.goUpdateCommand("cmd_findAgain");
      this._ownerWindow.goUpdateCommand("cmd_findPrevious");
    }
  },

  



  _onUndoRedo: function SEU__onUndoRedo()
  {
    if (this._ownerWindow.goUpdateCommand) {
      this._ownerWindow.goUpdateCommand("se-cmd-undo");
      this._ownerWindow.goUpdateCommand("se-cmd-redo");
    }
  },

  





  _onDirtyChanged: function SEU__onDirtyChanged()
  {
    this._onUndoRedo();
  },

  



  destroy: function SEU_destroy()
  {
    if (this._ownerWindow.controllers) {
      this.editor.removeEventListener(this.editor.EVENTS.DIRTY_CHANGED,
                                      this._onDirtyChanged);
    }

    this._ownerWindow = null;
    this.editor = null;
    this._controller = null;
  },
};









function SourceEditorController(aEditor)
{
  this._editor = aEditor;
}

SourceEditorController.prototype = {
  







  supportsCommand: function SEC_supportsCommand(aCommand)
  {
    let result;

    switch (aCommand) {
      case "cmd_find":
      case "cmd_findAgain":
      case "cmd_findPrevious":
      case "cmd_gotoLine":
      case "se-cmd-undo":
      case "se-cmd-redo":
      case "se-cmd-cut":
      case "se-cmd-paste":
      case "se-cmd-delete":
      case "se-cmd-selectAll":
        result = true;
        break;
      default:
        result = false;
        break;
    }

    return result;
  },

  







  isCommandEnabled: function SEC_isCommandEnabled(aCommand)
  {
    let result;

    switch (aCommand) {
      case "cmd_find":
      case "cmd_gotoLine":
      case "se-cmd-selectAll":
        result = true;
        break;
      case "cmd_findAgain":
      case "cmd_findPrevious":
        result = this._editor.lastFind && this._editor.lastFind.lastFound != -1;
        break;
      case "se-cmd-undo":
        result = this._editor.canUndo();
        break;
      case "se-cmd-redo":
        result = this._editor.canRedo();
        break;
      case "se-cmd-cut":
      case "se-cmd-delete": {
        let selection = this._editor.getSelection();
        result = selection.start != selection.end && !this._editor.readOnly;
        break;
      }
      case "se-cmd-paste": {
        let window = this._editor._view._frameWindow;
        let controller = window.controllers.getControllerForCommand("cmd_paste");
        result = !this._editor.readOnly &&
                 controller.isCommandEnabled("cmd_paste");
        break;
      }
      default:
        result = false;
        break;
    }

    return result;
  },

  






  doCommand: function SEC_doCommand(aCommand)
  {
    switch (aCommand) {
      case "cmd_find":
        this._editor.ui.find();
        break;
      case "cmd_findAgain":
        this._editor.ui.findNext();
        break;
      case "cmd_findPrevious":
        this._editor.ui.findPrevious();
        break;
      case "cmd_gotoLine":
        this._editor.ui.gotoLine();
        break;
      case "se-cmd-selectAll":
        this._editor._view.invokeAction("selectAll");
        break;
      case "se-cmd-undo":
        this._editor.undo();
        break;
      case "se-cmd-redo":
        this._editor.redo();
        break;
      case "se-cmd-cut":
        this._editor.ui._ownerWindow.goDoCommand("cmd_cut");
        break;
      case "se-cmd-paste":
        this._editor.ui._ownerWindow.goDoCommand("cmd_paste");
        break;
      case "se-cmd-delete": {
        let selection = this._editor.getSelection();
        this._editor.setText("", selection.start, selection.end);
        break;
      }
    }
  },

  onEvent: function() { }
};
