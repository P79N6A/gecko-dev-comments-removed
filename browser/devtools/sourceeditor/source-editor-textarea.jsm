





































"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var EXPORTED_SYMBOLS = ["SourceEditor"];











function SourceEditor() {
  

  SourceEditor.DEFAULTS.TAB_SIZE =
    Services.prefs.getIntPref(SourceEditor.PREFS.TAB_SIZE);
  SourceEditor.DEFAULTS.EXPAND_TAB =
    Services.prefs.getBoolPref(SourceEditor.PREFS.EXPAND_TAB);

  this._listeners = {};
  this._lastSelection = {};
}

SourceEditor.prototype = {
  _textbox: null,
  _editor: null,
  _listeners: null,
  _lineDelimiter: null,
  _editActionListener: null,
  _expandTab: null,
  _tabSize: null,

  



  parentElement: null,

  


















  init: function SE_init(aElement, aConfig, aCallback)
  {
    if (this._textbox) {
      throw new Error("SourceEditor is already initialized!");
    }

    let doc = aElement.ownerDocument;
    let win = doc.defaultView;

    this._textbox = doc.createElementNS(XUL_NS, "textbox");
    this._textbox.flex = 1;
    this._textbox.setAttribute("multiline", true);
    this._textbox.setAttribute("dir", "ltr");

    aElement.appendChild(this._textbox);

    this.parentElement = aElement;
    this._editor = this._textbox.editor;

    this._expandTab = aConfig.expandTab !== undefined ?
                      aConfig.expandTab : SourceEditor.DEFAULTS.EXPAND_TAB;
    this._tabSize = aConfig.tabSize || SourceEditor.DEFAULTS.TAB_SIZE;

    this._textbox.style.MozTabSize = this._tabSize;

    this._textbox.setAttribute("value", aConfig.placeholderText || "");
    this._textbox.setAttribute("class", "monospace");
    this._textbox.style.direction = "ltr";
    this._textbox.readOnly = aConfig.readOnly;

    
    
    this._textbox.addEventListener("select", this._onSelect.bind(this), false);
    this._textbox.addEventListener("keypress", this._onKeyPress.bind(this), false);
    this._textbox.addEventListener("keyup", this._onSelect.bind(this), false);
    this._textbox.addEventListener("click", this._onSelect.bind(this), false);

    
    this.setMode(aConfig.mode || SourceEditor.DEFAULTS.MODE);

    this._editor.transactionManager.maxTransactionCount =
      aConfig.undoLimit || SourceEditor.DEFAULTS.UNDO_LIMIT;

    
    this._editor.transactionManager.clear();
    this._editor.resetModificationCount();

    
    
    this._editActionListener = new EditActionListener(this);
    this._editor.addEditActionListener(this._editActionListener);

    this._lineDelimiter = win.navigator.platform.indexOf("Win") > -1 ?
                          "\r\n" : "\n";

    this._config = aConfig;

    if (aCallback) {
      aCallback(this);
    }
  },

  







  _onKeyPress: function SE__onKeyPress(aEvent)
  {
    if (aEvent.keyCode != aEvent.DOM_VK_TAB || aEvent.shiftKey ||
        aEvent.metaKey || aEvent.ctrlKey || aEvent.altKey) {
      return;
    }

    aEvent.preventDefault();

    let caret = this.getCaretOffset();
    let indent = "\t";

    if (this._expandTab) {
      let text = this._textbox.value;
      let lineStart = caret;
      while (lineStart > 0) {
        let c = text.charAt(lineStart - 1);
        if (c == "\r" || c == "\n") {
          break;
        }
        lineStart--;
      }
      let offset = caret - lineStart;
      let spaces = this._tabSize - (offset % this._tabSize);
      indent = (new Array(spaces + 1)).join(" ");
    }

    this.setText(indent, caret, caret);
    this.setCaretOffset(caret + indent.length);
  },

  






  _onSelect: function SE__onSelect()
  {
    let selection = this.getSelection();
    selection.collapsed = selection.start == selection.end;
    if (selection.collapsed && this._lastSelection.collapsed) {
      this._lastSelection = selection;
      return; 
    }

    if (this._lastSelection.start != selection.start ||
        this._lastSelection.end != selection.end) {
      let sendEvent = {
        oldValue: {start: this._lastSelection.start,
                   end: this._lastSelection.end},
        newValue: {start: selection.start, end: selection.end},
      };

      let listeners = this._listeners[SourceEditor.EVENTS.SELECTION] || [];
      listeners.forEach(function(aListener) {
        aListener.callback.call(null, sendEvent, aListener.data);
      }, this);

      this._lastSelection = selection;
    }
  },

  













  _onTextChanged: function SE__onTextChanged(aEvent)
  {
    let listeners = this._listeners[SourceEditor.EVENTS.TEXT_CHANGED] || [];
    listeners.forEach(function(aListener) {
      aListener.callback.call(null, aEvent, aListener.data);
    }, this);
  },

  





  get editorElement() {
    return this._textbox;
  },

  











  addEventListener:
  function SE_addEventListener(aEventType, aCallback, aData)
  {
    const EVENTS = SourceEditor.EVENTS;
    let listener = {
      type: aEventType,
      data: aData,
      callback: aCallback,
    };

    if (aEventType == EVENTS.CONTEXT_MENU) {
      listener.domType = "contextmenu";
      listener.target = this._textbox;
      listener.handler = this._onContextMenu.bind(this, listener);
      listener.target.addEventListener(listener.domType, listener.handler, false);
    }

    if (!(aEventType in this._listeners)) {
      this._listeners[aEventType] = [];
    }

    this._listeners[aEventType].push(listener);
  },

  












  removeEventListener:
  function SE_removeEventListener(aEventType, aCallback, aData)
  {
    let listeners = this._listeners[aEventType];
    if (!listeners) {
      throw new Error("SourceEditor.removeEventListener() called for an " +
                      "unknown event.");
    }

    const EVENTS = SourceEditor.EVENTS;

    this._listeners[aEventType] = listeners.filter(function(aListener) {
      let isSameListener = aListener.type == aEventType &&
                           aListener.callback === aCallback &&
                           aListener.data === aData;
      if (isSameListener && aListener.domType) {
        aListener.target.removeEventListener(aListener.domType,
                                             aListener.handler, false);
      }
      return !isSameListener;
    }, this);
  },

  









  _onContextMenu: function SE__onContextMenu(aListener, aDOMEvent)
  {
    let input = this._textbox.inputField;
    let rect = this._textbox.getBoundingClientRect();

    
    let sendEvent = {
      x: aDOMEvent.clientX - rect.left + input.scrollLeft,
      y: aDOMEvent.clientY - rect.top + input.scrollTop,
      screenX: aDOMEvent.screenX,
      screenY: aDOMEvent.screenY,
    };

    aDOMEvent.preventDefault();
    aListener.callback.call(null, sendEvent, aListener.data);
  },

  


  undo: function SE_undo()
  {
    this._editor.undo(1);
  },

  


  redo: function SE_redo()
  {
    this._editor.redo(1);
  },

  





  canUndo: function SE_canUndo()
  {
    let isEnabled = {};
    let canUndo = {};
    this._editor.canUndo(isEnabled, canUndo);
    return canUndo.value;
  },

  





  canRedo: function SE_canRedo()
  {
    let isEnabled = {};
    let canRedo = {};
    this._editor.canRedo(isEnabled, canRedo);
    return canRedo.value;
  },

  




  startCompoundChange: function SE_startCompoundChange()
  {
    this._editor.beginTransaction();
  },

  


  endCompoundChange: function SE_endCompoundChange()
  {
    this._editor.endTransaction();
  },

  


  focus: function SE_focus()
  {
    this._textbox.focus();
  },

  





  hasFocus: function SE_hasFocus()
  {
    return this._textbox.ownerDocument.activeElement ===
           this._textbox.inputField;
  },

  












  getText: function SE_getText(aStart, aEnd)
  {
    let value = this._textbox.value || "";
    if (aStart === undefined || aStart === null) {
      aStart = 0;
    }
    if (aEnd === undefined || aEnd === null) {
      aEnd = value.length;
    }
    return value.substring(aStart, aEnd);
  },

  





  getCharCount: function SE_getCharCount()
  {
    return this._textbox.textLength;
  },

  





  getSelectedText: function SE_getSelectedText()
  {
    let selection = this.getSelection();
    return selection.start != selection.end ?
           this.getText(selection.start, selection.end) : "";
  },

  











  setText: function SE_setText(aText, aStart, aEnd)
  {
    if (aStart === undefined) {
      this._textbox.value = aText;
    } else {
      if (aEnd === undefined) {
        aEnd = this._textbox.textLength;
      }

      let value = this._textbox.value || "";
      let removedText = value.substring(aStart, aEnd);
      let prefix = value.substr(0, aStart);
      let suffix = value.substr(aEnd);

      if (suffix) {
        this._editActionListener._setTextRangeEvent = {
          start: aStart,
          removedCharCount: removedText.length,
          addedCharCount: aText.length,
        };
      }

      this._textbox.value = prefix + aText + suffix;
    }
  },

  


  dropSelection: function SE_dropSelection()
  {
    let selection = this._editor.selection;
    selection.collapse(selection.focusNode, selection.focusOffset);
    this._onSelect();
  },

  







  setSelection: function SE_setSelection(aStart, aEnd)
  {
    this._textbox.setSelectionRange(aStart, aEnd);
    this._onSelect();
  },

  






  getSelection: function SE_getSelection()
  {
    return {
      start: this._textbox.selectionStart,
      end: this._textbox.selectionEnd
    };
  },

  





  getCaretOffset: function SE_getCaretOffset()
  {
    let selection = this.getSelection();
    return selection.start < selection.end ?
           selection.end : selection.start;
  },

  





  setCaretOffset: function SE_setCaretOffset(aOffset)
  {
    this.setSelection(aOffset, aOffset);
  },

  





  getLineDelimiter: function SE_getLineDelimiter()
  {
    return this._lineDelimiter;
  },

  








  setMode: function SE_setMode(aMode)
  {
    
  },

  






  getMode: function SE_getMode()
  {
    return SourceEditor.MODES.TEXT;
  },

  




  set readOnly(aValue)
  {
    this._textbox.readOnly = aValue;
  },

  



  get readOnly()
  {
    return this._textbox.readOnly;
  },

  


  destroy: function SE_destroy()
  {
    for (let eventType in this._listeners) {
      this._listeners[eventType].forEach(function(aListener) {
        if (aListener.domType) {
          aListener.target.removeEventListener(aListener.domType,
                                               aListener.handler, false);
        }
      }, this);
    }

    this._editor.removeEditActionListener(this._editActionListener);
    this.parentElement.removeChild(this._textbox);
    this.parentElement = null;
    this._editor = null;
    this._textbox = null;
    this._config = null;
    this._listeners = null;
    this._lastSelection = null;
    this._editActionListener = null;
  },
};













function EditActionListener(aSourceEditor) {
  this._sourceEditor = aSourceEditor;
}

EditActionListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIEditActionListener]),

  WillCreateNode: function() { },
  DidCreateNode: function() { },
  WillInsertNode: function() { },

  DidInsertNode: function EAL_DidInsertNode(aNode)
  {
    if (aNode.nodeType != aNode.TEXT_NODE) {
      return;
    }

    let event;

    if (this._setTextRangeEvent) {
      event = this._setTextRangeEvent;
      delete this._setTextRangeEvent;
    } else {
      event = {
        start: 0,
        removedCharCount: 0,
        addedCharCount: aNode.textContent.length,
      };
    }

    this._sourceEditor._onTextChanged(event);
  },

  WillDeleteNode: function() { },
  DidDeleteNode: function() { },
  WillSplitNode: function() { },
  DidSplitNode: function() { },
  WillJoinNodes: function() { },
  DidJoinNodes: function() { },
  WillInsertText: function() { },

  DidInsertText: function EAL_DidInsertText(aTextNode, aOffset, aString)
  {
    let event = {
      start: aOffset,
      removedCharCount: 0,
      addedCharCount: aString.length,
    };

    this._sourceEditor._onTextChanged(event);
  },

  WillDeleteText: function() { },

  DidDeleteText: function EAL_DidDeleteText(aTextNode, aOffset, aLength)
  {
    let event = {
      start: aOffset,
      removedCharCount: aLength,
      addedCharCount: 0,
    };

    this._sourceEditor._onTextChanged(event);
  },

  WillDeleteSelection: function EAL_WillDeleteSelection()
  {
    if (this._setTextRangeEvent) {
      return;
    }

    let selection = this._sourceEditor.getSelection();
    let str = this._sourceEditor.getSelectedText();

    let event = {
      start: selection.start,
      removedCharCount: str.length,
      addedCharCount: 0,
    };

    this._sourceEditor._onTextChanged(event);
  },

  DidDeleteSelection: function() { },
};
