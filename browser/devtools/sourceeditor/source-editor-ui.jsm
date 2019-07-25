





































"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["SourceEditorUI"];




function SourceEditorUI(aEditor)
{
  this.editor = aEditor;
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

  



  destroy: function SEU_destroy()
  {
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
        result = true;
        break;
      case "cmd_findAgain":
      case "cmd_findPrevious":
        result = this._editor.lastFind && this._editor.lastFind.lastFound != -1;
        break;
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
    }
  },

  onEvent: function() { }
};
