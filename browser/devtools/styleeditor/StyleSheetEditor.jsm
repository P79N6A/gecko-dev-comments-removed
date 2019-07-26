




"use strict";

this.EXPORTED_SYMBOLS = ["StyleSheetEditor"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js").Promise;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/sourceeditor/source-editor.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");


const SAVE_ERROR = "error-save";



const UPDATE_STYLESHEET_THROTTLE_DELAY = 500;



















function StyleSheetEditor(styleSheet, win, file, isNew) {
  EventEmitter.decorate(this);

  this.styleSheet = styleSheet;
  this._inputElement = null;
  this._sourceEditor = null;
  this._window = win;
  this._isNew = isNew;
  this.savedFile = file;

  this.errorMessage = null;

  this._state = {   
    text: "",
    selection: {start: 0, end: 0},
    readOnly: false,
    topIndex: 0,              
  };

  this._styleSheetFilePath = null;
  if (styleSheet.href &&
      Services.io.extractScheme(this.styleSheet.href) == "file") {
    this._styleSheetFilePath = this.styleSheet.href;
  }

  this._onSourceLoad = this._onSourceLoad.bind(this);
  this._onPropertyChange = this._onPropertyChange.bind(this);
  this._onError = this._onError.bind(this);

  this._focusOnSourceEditorReady = false;

  this.styleSheet.once("source-load", this._onSourceLoad);
  this.styleSheet.on("property-change", this._onPropertyChange);
  this.styleSheet.on("error", this._onError);
}

StyleSheetEditor.prototype = {
  


  get sourceEditor() {
    return this._sourceEditor;
  },

  


  get unsaved() {
    return this._sourceEditor && this._sourceEditor.dirty;
  },

  



  get isNew() {
    return this._isNew;
  },

  




  get friendlyName() {
    if (this.savedFile) { 
      return this.savedFile.leafName;
    }

    if (this._isNew) {
      let index = this.styleSheet.styleSheetIndex + 1; 
      return _("newStyleSheet", index);
    }

    if (!this.styleSheet.href) {
      let index = this.styleSheet.styleSheetIndex + 1; 
      return _("inlineStyleSheet", index);
    }

    if (!this._friendlyName) {
      let sheetURI = this.styleSheet.href;
      let contentURI = this.styleSheet.debuggee.baseURI;
      let contentURIScheme = contentURI.scheme;
      let contentURILeafIndex = contentURI.specIgnoringRef.lastIndexOf("/");
      contentURI = contentURI.specIgnoringRef;

      
      if (contentURILeafIndex > contentURIScheme.length) {
        contentURI = contentURI.substring(0, contentURILeafIndex + 1);
      }

      
      
      this._friendlyName = (sheetURI.indexOf(contentURI) == 0)
                           ? sheetURI.substring(contentURI.length)
                           : sheetURI;
      try {
        this._friendlyName = decodeURI(this._friendlyName);
      } catch (ex) {
      }
    }
    return this._friendlyName;
  },

  


  fetchSource: function() {
    this.styleSheet.fetchSource();
  },

  







  _onSourceLoad: function(event, source) {
    this._state.text = prettifyCSS(source);
    this.sourceLoaded = true;
    this.emit("source-load");
  },

  







  _onPropertyChange: function(event, property) {
    this.emit("property-change", property);
  },

  






  _onError: function(event, errorCode) {
    this.emit("error", errorCode);
  },

  




  load: function(inputElement) {
    this._inputElement = inputElement;

    let sourceEditor = new SourceEditor();
    let config = {
      initialText: this._state.text,
      showLineNumbers: true,
      mode: SourceEditor.MODES.CSS,
      readOnly: this._state.readOnly,
      keys: this._getKeyBindings()
    };

    sourceEditor.init(inputElement, config, function onSourceEditorReady() {
      setupBracketCompletion(sourceEditor);
      sourceEditor.addEventListener(SourceEditor.EVENTS.TEXT_CHANGED,
                                    function onTextChanged(event) {
        this.updateStyleSheet();
      }.bind(this));

      this._sourceEditor = sourceEditor;

      if (this._focusOnSourceEditorReady) {
        this._focusOnSourceEditorReady = false;
        sourceEditor.focus();
      }

      sourceEditor.setTopIndex(this._state.topIndex);
      sourceEditor.setSelection(this._state.selection.start,
                                this._state.selection.end);

      this.emit("source-editor-load");
    }.bind(this));

    sourceEditor.addEventListener(SourceEditor.EVENTS.DIRTY_CHANGED,
                                  this._onPropertyChange);
  },

  





  getSourceEditor: function() {
    let deferred = promise.defer();

    if (this.sourceEditor) {
      return promise.resolve(this);
    }
    this.on("source-editor-load", (event) => {
      deferred.resolve(this);
    });
    return deferred.promise;
  },

  


  focus: function() {
    if (this._sourceEditor) {
      this._sourceEditor.focus();
    } else {
      this._focusOnSourceEditorReady = true;
    }
  },

  


  onShow: function() {
    if (this._sourceEditor) {
      this._sourceEditor.setTopIndex(this._state.topIndex);
    }
    this.focus();
  },

  


  toggleDisabled: function() {
    this.styleSheet.toggleDisabled();
  },

  





  updateStyleSheet: function(immediate) {
    if (this._updateTask) {
      
      this._window.clearTimeout(this._updateTask);
    }

    if (immediate) {
      this._updateStyleSheet();
    } else {
      this._updateTask = this._window.setTimeout(this._updateStyleSheet.bind(this),
                                           UPDATE_STYLESHEET_THROTTLE_DELAY);
    }
  },

  


  _updateStyleSheet: function() {
    if (this.styleSheet.disabled) {
      return;  
    }

    this._updateTask = null; 
                             
                             
                             

    if (this.sourceEditor) {
      this._state.text = this.sourceEditor.getText();
    }

    this.styleSheet.update(this._state.text);
  },

  














  saveToFile: function(file, callback) {
    let onFile = (returnFile) => {
      if (!returnFile) {
        if (callback) {
          callback(null);
        }
        return;
      }

      if (this._sourceEditor) {
        this._state.text = this._sourceEditor.getText();
      }

      let ostream = FileUtils.openSafeFileOutputStream(returnFile);
      let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                        .createInstance(Ci.nsIScriptableUnicodeConverter);
      converter.charset = "UTF-8";
      let istream = converter.convertToInputStream(this._state.text);

      NetUtil.asyncCopy(istream, ostream, function onStreamCopied(status) {
        if (!Components.isSuccessCode(status)) {
          if (callback) {
            callback(null);
          }
          this.emit("error", SAVE_ERROR);
          return;
        }
        FileUtils.closeSafeFileOutputStream(ostream);
        
        this._friendlyName = null;
        this.savedFile = returnFile;

        if (callback) {
          callback(returnFile);
        }
        this.sourceEditor.dirty = false;
      }.bind(this));
    };

    showFilePicker(file || this._styleSheetFilePath, true, this._window, onFile);
  },

  





  _getKeyBindings: function() {
    let bindings = [];

    bindings.push({
      action: "StyleEditor.save",
      code: _("saveStyleSheet.commandkey"),
      accel: true,
      callback: function save() {
        this.saveToFile(this.savedFile);
        return true;
      }.bind(this)
    });

    bindings.push({
      action: "StyleEditor.saveAs",
      code: _("saveStyleSheet.commandkey"),
      accel: true,
      shift: true,
      callback: function saveAs() {
        this.saveToFile();
        return true;
      }.bind(this)
    });

    return bindings;
  },

  


  destroy: function() {
    this.styleSheet.off("source-load", this._onSourceLoad);
    this.styleSheet.off("property-change", this._onPropertyChange);
    this.styleSheet.off("error", this._onError);
  }
}


const TAB_CHARS = "\t";

const OS = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
const LINE_SEPARATOR = OS === "WINNT" ? "\r\n" : "\n";








function repeat(text, aCount)
{
  return (new Array(aCount + 1)).join(text);
}











function prettifyCSS(text)
{
  
  text = text.replace(/(?:^\s*<!--[\r\n]*)|(?:\s*-->\s*$)/g, "");

  let parts = [];    
  let partStart = 0; 
  let indent = "";
  let indentLevel = 0;

  for (let i = 0; i < text.length; i++) {
    let c = text[i];
    let shouldIndent = false;

    switch (c) {
      case "}":
        if (i - partStart > 1) {
          
          parts.push(indent + text.substring(partStart, i));
          partStart = i;
        }
        indent = repeat(TAB_CHARS, --indentLevel);
        
      case ";":
      case "{":
        shouldIndent = true;
        break;
    }

    if (shouldIndent) {
      let la = text[i+1]; 
      if (!/\s/.test(la)) {
        
        
        parts.push(indent + text.substring(partStart, i + 1));
        if (c == "}") {
          parts.push(""); 
        }
        partStart = i + 1;
      } else {
        return text; 
      }
    }

    if (c == "{") {
      indent = repeat(TAB_CHARS, ++indentLevel);
    }
  }
  return parts.join(LINE_SEPARATOR);
}








function setupBracketCompletion(sourceEditor)
{
  let editorElement = sourceEditor.editorElement;
  let pairs = {
    123: { 
      closeString: "}",
      closeKeyCode: Ci.nsIDOMKeyEvent.DOM_VK_CLOSE_BRACKET
    },
    40: { 
      closeString: ")",
      closeKeyCode: Ci.nsIDOMKeyEvent.DOM_VK_0
    },
    91: { 
      closeString: "]",
      closeKeyCode: Ci.nsIDOMKeyEvent.DOM_VK_CLOSE_BRACKET
    },
  };

  editorElement.addEventListener("keypress", function onKeyPress(event) {
    let pair = pairs[event.charCode];
    if (!pair || event.ctrlKey || event.metaKey ||
        event.accelKey || event.altKey) {
      return true;
    }

    
    let keyCode = pair.closeKeyCode;
    let charCode = pair.closeString.charCodeAt(0);
    let modifiers = 0;
    let utils = editorElement.ownerDocument.defaultView.
                  QueryInterface(Ci.nsIInterfaceRequestor).
                  getInterface(Ci.nsIDOMWindowUtils);
                  
    if (utils.sendKeyEvent("keydown", keyCode, 0, modifiers)) {
      utils.sendKeyEvent("keypress", 0, charCode, modifiers);
    }
    utils.sendKeyEvent("keyup", keyCode, 0, modifiers);
    
    sourceEditor.setCaretOffset(sourceEditor.getCaretOffset() - 1);
  }, false);
}

