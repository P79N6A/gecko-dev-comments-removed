





































"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");

const ORION_SCRIPT = "chrome://browser/content/orion.js";
const ORION_IFRAME = "data:text/html;charset=utf8,<!DOCTYPE html>" +
  "<html style='height:100%' dir='ltr'>" +
  "<body style='height:100%;margin:0;overflow:hidden'>" +
  "<div id='editor' style='height:100%'></div>" +
  "</body></html>";

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";





const ORION_THEMES = {
  textmate: ["chrome://browser/content/orion.css",
             "chrome://browser/content/orion-mozilla.css"],
};





const ORION_EVENTS = {
  ContextMenu: "ContextMenu",
  TextChanged: "ModelChanged",
  Selection: "Selection",
};


var EXPORTED_SYMBOLS = ["SourceEditor"];










function SourceEditor() {
  

  SourceEditor.DEFAULTS.TAB_SIZE =
    Services.prefs.getIntPref(SourceEditor.PREFS.TAB_SIZE);
  SourceEditor.DEFAULTS.EXPAND_TAB =
    Services.prefs.getBoolPref(SourceEditor.PREFS.EXPAND_TAB);
}

SourceEditor.prototype = {
  _view: null,
  _iframe: null,
  _undoStack: null,
  _lines_ruler: null,
  _styler: null,
  _mode: null,
  _expandTab: null,
  _tabSize: null,

  



  parentElement: null,

  






























  init: function SE_init(aElement, aConfig, aCallback)
  {
    if (this._iframe) {
      throw new Error("SourceEditor is already initialized!");
    }

    let doc = aElement.ownerDocument;

    this._iframe = doc.createElementNS(XUL_NS, "iframe");
    this._iframe.flex = 1;

    let onIframeLoad = (function() {
      this._iframe.removeEventListener("load", onIframeLoad, true);

      Services.scriptloader.loadSubScript(ORION_SCRIPT,
        this._iframe.contentWindow.wrappedJSObject, "utf8");

      this._onLoad(aCallback);
    }).bind(this);

    this._iframe.addEventListener("load", onIframeLoad, true);

    this._iframe.setAttribute("src", ORION_IFRAME);

    aElement.appendChild(this._iframe);
    this.parentElement = aElement;
    this._config = aConfig;
  },

  






  _onLoad: function SE__onLoad(aCallback)
  {
    let config = this._config;
    let window = this._iframe.contentWindow.wrappedJSObject;
    let textview = window.orion.textview;

    this._expandTab = typeof config.expandTab != "undefined" ?
                      config.expandTab : SourceEditor.DEFAULTS.EXPAND_TAB;
    this._tabSize = config.tabSize || SourceEditor.DEFAULTS.TAB_SIZE;

    let theme = config.theme || SourceEditor.DEFAULTS.THEME;
    let stylesheet = theme in ORION_THEMES ? ORION_THEMES[theme] : theme;

    this._view = new textview.TextView({
      model: new textview.TextModel(config.placeholderText),
      parent: "editor",
      stylesheet: stylesheet,
      tabSize: this._tabSize,
      readonly: config.readOnly,
    });

    if (config.showLineNumbers) {
      this._lines_ruler = new textview.LineNumberRuler(null, "left",
        {styleClass: "rulerLines"}, {styleClass: "rulerLine odd"},
        {styleClass: "rulerLine even"});

      this._view.addRuler(this._lines_ruler);
    }

    this.setMode(config.mode || SourceEditor.DEFAULTS.MODE);

    this._undoStack = new textview.UndoStack(this._view,
      config.undoLimit || SourceEditor.DEFAULTS.UNDO_LIMIT);

    this._initEditorFeatures();

    (config.keys || []).forEach(function(aKey) {
      let binding = new textview.KeyBinding(aKey.code, aKey.accel, aKey.shift,
                                            aKey.alt);
      this._view.setKeyBinding(binding, aKey.action);

      if (aKey.callback) {
        this._view.setAction(aKey.action, aKey.callback);
      }
    }, this);

    if (aCallback) {
      let iframe = this._view._frame;
      let document = iframe.contentDocument;
      if (!document || document.readyState != "complete") {
        let onIframeLoad = function () {
          iframe.contentWindow.removeEventListener("load", onIframeLoad, false);
          aCallback(this);
        }.bind(this);
        iframe.contentWindow.addEventListener("load", onIframeLoad, false);
      } else {
        aCallback(this);
      }
    }
  },

  



  _initEditorFeatures: function SE__initEditorFeatures()
  {
    let window = this._iframe.contentWindow.wrappedJSObject;
    let textview = window.orion.textview;

    this._view.setAction("tab", this._doTab.bind(this));

    let shiftTabKey = new textview.KeyBinding(Ci.nsIDOMKeyEvent.DOM_VK_TAB,
                                              false, true);
    this._view.setAction("Unindent Lines", this._doUnindentLines.bind(this));
    this._view.setKeyBinding(shiftTabKey, "Unindent Lines");
    this._view.setAction("enter", this._doEnter.bind(this));

    if (this._expandTab) {
      this._view.setAction("deletePrevious", this._doDeletePrevious.bind(this));
    }
  },

  




  _doTab: function SE__doTab()
  {
    let indent = "\t";
    let selection = this.getSelection();
    let model = this._model;
    let firstLine = model.getLineAtOffset(selection.start);
    let firstLineStart = model.getLineStart(firstLine);
    let lastLineOffset = selection.end > selection.start ?
                         selection.end - 1 : selection.end;
    let lastLine = model.getLineAtOffset(lastLineOffset);

    if (this._expandTab) {
      let offsetFromLineStart = firstLine == lastLine ?
                                selection.start - firstLineStart : 0;
      let spaces = this._tabSize - (offsetFromLineStart % this._tabSize);
      indent = (new Array(spaces + 1)).join(" ");
    }

    
    if (firstLine != lastLine) {
      let lines = [""];
      let lastLineEnd = model.getLineEnd(lastLine, true);
      let selectedLines = lastLine - firstLine + 1;

      for (let i = firstLine; i <= lastLine; i++) {
        lines.push(model.getLine(i, true));
      }

      this.startCompoundChange();

      this.setText(lines.join(indent), firstLineStart, lastLineEnd);

      let newSelectionStart = firstLineStart == selection.start ?
                              selection.start : selection.start + indent.length;
      let newSelectionEnd = selection.end + (selectedLines * indent.length);

      this._view.setSelection(newSelectionStart, newSelectionEnd);

      this.endCompoundChange();
    } else {
      this.setText(indent, selection.start, selection.end);
    }

    return true;
  },

  




  _doDeletePrevious: function SE__doDeletePrevious()
  {
    let selection = this.getSelection();
    if (selection.start == selection.end && this._expandTab) {
      let model = this._model;
      let lineIndex = model.getLineAtOffset(selection.start);
      let lineStart = model.getLineStart(lineIndex);
      let offset = selection.start - lineStart;
      if (offset >= this._tabSize && (offset % this._tabSize) == 0) {
        let text = this.getText(lineStart, selection.start);
        if (!/[^ ]/.test(text)) {
          this.setText("", selection.start - this._tabSize, selection.end);
          return true;
        }
      }
    }
    return false;
  },

  




  _doUnindentLines: function SE__doUnindentLines()
  {
    let indent = "\t";

    let selection = this.getSelection();
    let model = this._model;
    let firstLine = model.getLineAtOffset(selection.start);
    let lastLineOffset = selection.end > selection.start ?
                         selection.end - 1 : selection.end;
    let lastLine = model.getLineAtOffset(lastLineOffset);

    if (this._expandTab) {
      indent = (new Array(this._tabSize + 1)).join(" ");
    }

    let lines = [];
    for (let line, i = firstLine; i <= lastLine; i++) {
      line = model.getLine(i, true);
      if (line.indexOf(indent) != 0) {
        return false;
      }
      lines.push(line.substring(indent.length));
    }

    let firstLineStart = model.getLineStart(firstLine);
    let lastLineStart = model.getLineStart(lastLine);
    let lastLineEnd = model.getLineEnd(lastLine, true);

    this.startCompoundChange();

    this.setText(lines.join(""), firstLineStart, lastLineEnd);

    let selectedLines = lastLine - firstLine + 1;
    let newSelectionStart = firstLineStart == selection.start ?
                            selection.start :
                            Math.max(firstLineStart,
                                     selection.start - indent.length);
    let newSelectionEnd = selection.end - (selectedLines * indent.length) +
                          (selection.end == lastLineStart + 1 ? 1 : 0);
    if (firstLine == lastLine) {
      newSelectionEnd = Math.max(lastLineStart, newSelectionEnd);
    }
    this._view.setSelection(newSelectionStart, newSelectionEnd);

    this.endCompoundChange();

    return true;
  },

  




  _doEnter: function SE__doEnter()
  {
    let selection = this.getSelection();
    if (selection.start != selection.end) {
      return false;
    }

    let model = this._model;
    let lineIndex = model.getLineAtOffset(selection.start);
    let lineText = model.getLine(lineIndex, true);
    let lineStart = model.getLineStart(lineIndex);
    let index = 0;
    let lineOffset = selection.start - lineStart;
    while (index < lineOffset && /[ \t]/.test(lineText.charAt(index))) {
      index++;
    }

    if (!index) {
      return false;
    }

    let prefix = lineText.substring(0, index);
    index = lineOffset;
    while (index < lineText.length &&
           /[ \t]/.test(lineText.charAt(index++))) {
      selection.end++;
    }

    this.setText(this.getLineDelimiter() + prefix, selection.start,
                 selection.end);
    return true;
  },

  




  get _model() {
    return this._view.getModel();
  },

  





  get editorElement() {
    return this._iframe;
  },

  











  addEventListener:
  function SE_addEventListener(aEventType, aCallback, aData)
  {
    if (aEventType in ORION_EVENTS) {
      this._view.addEventListener(ORION_EVENTS[aEventType], true,
                                  aCallback, aData);
    } else {
      throw new Error("SourceEditor.addEventListener() unknown event " +
                      "type " + aEventType);
    }
  },

  












  removeEventListener:
  function SE_removeEventListener(aEventType, aCallback, aData)
  {
    if (aEventType in ORION_EVENTS) {
      this._view.removeEventListener(ORION_EVENTS[aEventType], true,
                                     aCallback, aData);
    } else {
      throw new Error("SourceEditor.removeEventListener() unknown event " +
                      "type " + aEventType);
    }
  },

  


  undo: function SE_undo()
  {
    this._undoStack.undo();
  },

  


  redo: function SE_redo()
  {
    this._undoStack.redo();
  },

  





  canUndo: function SE_canUndo()
  {
    return this._undoStack.canUndo();
  },

  





  canRedo: function SE_canRedo()
  {
    return this._undoStack.canRedo();
  },

  




  startCompoundChange: function SE_startCompoundChange()
  {
    this._undoStack.startCompoundChange();
  },

  


  endCompoundChange: function SE_endCompoundChange()
  {
    this._undoStack.endCompoundChange();
  },

  


  focus: function SE_focus()
  {
    this._view.focus();
  },

  





  hasFocus: function SE_hasFocus()
  {
    return this._iframe.ownerDocument.activeElement === this._iframe;
  },

  












  getText: function SE_getText(aStart, aEnd)
  {
    return this._view.getText(aStart, aEnd);
  },

  





  getCharCount: function SE_getCharCount()
  {
    return this._model.getCharCount();
  },

  





  getSelectedText: function SE_getSelectedText()
  {
    let selection = this.getSelection();
    return this.getText(selection.start, selection.end);
  },

  











  setText: function SE_setText(aText, aStart, aEnd)
  {
    this._view.setText(aText, aStart, aEnd);
  },

  


  dropSelection: function SE_dropSelection()
  {
    this.setCaretOffset(this.getCaretOffset());
  },

  







  setSelection: function SE_setSelection(aStart, aEnd)
  {
    this._view.setSelection(aStart, aEnd, true);
  },

  






  getSelection: function SE_getSelection()
  {
    return this._view.getSelection();
  },

  





  getCaretOffset: function SE_getCaretOffset()
  {
    return this._view.getCaretOffset();
  },

  





  setCaretOffset: function SE_setCaretOffset(aOffset)
  {
    this._view.setCaretOffset(aOffset, true);
  },

  





  getLineDelimiter: function SE_getLineDelimiter()
  {
    return this._model.getLineDelimiter();
  },

  





  setMode: function SE_setMode(aMode)
  {
    if (this._styler) {
      this._styler.destroy();
      this._styler = null;
    }

    let window = this._iframe.contentWindow.wrappedJSObject;
    let TextStyler = window.examples.textview.TextStyler;
    let TextMateStyler = window.orion.editor.TextMateStyler;
    let HtmlGrammar = window.orion.editor.HtmlGrammar;

    switch (aMode) {
      case SourceEditor.MODES.JAVASCRIPT:
      case SourceEditor.MODES.CSS:
        this._styler = new TextStyler(this._view, aMode);
        break;

      case SourceEditor.MODES.HTML:
      case SourceEditor.MODES.XML:
        this._styler = new TextMateStyler(this._view, HtmlGrammar.grammar);
        break;
    }

    this._mode = aMode;
  },

  





  getMode: function SE_getMode()
  {
    return this._mode;
  },

  




  set readOnly(aValue)
  {
    this._view.readonly = aValue;
  },

  



  get readOnly()
  {
    return this._view.readonly;
  },

  


  destroy: function SE_destroy()
  {
    this._view.destroy();
    this.parentElement.removeChild(this._iframe);
    this.parentElement = null;
    this._iframe = null;
    this._undoStack = null;
    this._styler = null;
    this._lines_ruler = null;
    this._view = null;
    this._config = null;
  },
};
