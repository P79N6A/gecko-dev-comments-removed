




"use strict";

this.EXPORTED_SYMBOLS = ["StyleSheetEditor"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const Editor  = require("devtools/sourceeditor/editor");
const promise = require("sdk/core/promise");
const {CssLogic} = require("devtools/styleinspector/css-logic");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");

const LOAD_ERROR = "error-load";
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

  let readOnly = false;
  if (styleSheet.isOriginalSource) {
    
    readOnly = true;
  }

  this._state = {   
    text: "",
    selection: {
      start: {line: 0, ch: 0},
      end: {line: 0, ch: 0}
    },
    readOnly: readOnly,
    topIndex: 0,              
  };

  this._styleSheetFilePath = null;
  if (styleSheet.href &&
      Services.io.extractScheme(this.styleSheet.href) == "file") {
    this._styleSheetFilePath = this.styleSheet.href;
  }

  this._onPropertyChange = this._onPropertyChange.bind(this);
  this._onError = this._onError.bind(this);

  this._focusOnSourceEditorReady = false;

  this.styleSheet.on("property-change", this._onPropertyChange);
  this.styleSheet.on("error", this._onError);
}

StyleSheetEditor.prototype = {
  


  get unsaved() {
    return this.sourceEditor && !this.sourceEditor.isClean();
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
      this._friendlyName = CssLogic.shortSource({ href: sheetURI });
      try {
        this._friendlyName = decodeURI(this._friendlyName);
      } catch (ex) {
      }
    }
    return this._friendlyName;
  },

  


  fetchSource: function(callback) {
    this.styleSheet.getText().then((longStr) => {
      longStr.string().then((source) => {
        this._state.text = prettifyCSS(source);
        this.sourceLoaded = true;

        callback(source);
      });
    }, e => {
      this.emit("error", LOAD_ERROR, this.styleSheet.href);
    })
  },

  







  _onPropertyChange: function(property, value) {
    this.emit("property-change", property, value);
  },

  






  _onError: function(event, errorCode) {
    this.emit("error", errorCode);
  },

  




  load: function(inputElement) {
    this._inputElement = inputElement;

    let config = {
      value: this._state.text,
      lineNumbers: true,
      mode: Editor.modes.css,
      readOnly: this._state.readOnly,
      autoCloseBrackets: "{}()[]",
      extraKeys: this._getKeyBindings(),
      contextMenu: "sourceEditorContextMenu"
    };
    let sourceEditor = new Editor(config);

    sourceEditor.appendTo(inputElement).then(() => {
      sourceEditor.on("change", () => {
        this.updateStyleSheet();
      });

      this.sourceEditor = sourceEditor;

      if (this._focusOnSourceEditorReady) {
        this._focusOnSourceEditorReady = false;
        sourceEditor.focus();
      }

      sourceEditor.setFirstVisibleLine(this._state.topIndex);
      sourceEditor.setSelection(this._state.selection.start,
                                this._state.selection.end);

      this.emit("source-editor-load");
    });

    sourceEditor.on("dirty-change", this._onPropertyChange);
  },

  





  getSourceEditor: function() {
    let deferred = promise.defer();

    if (this.sourceEditor) {
      return promise.resolve(this);
    }
    this.on("source-editor-load", () => {
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
      this._sourceEditor.setFirstVisibleLine(this._state.topIndex);
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

    this.styleSheet.update(this._state.text, true);
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
        this.sourceEditor.setClean();

        this.emit("property-change");
      }.bind(this));
    };

    showFilePicker(file || this._styleSheetFilePath, true, this._window, onFile);
  },

  





  _getKeyBindings: function() {
    let bindings = {};

    bindings[Editor.accel(_("saveStyleSheet.commandkey"))] = () => {
      this.saveToFile(this.savedFile);
    };

    bindings["Shift-" + Editor.accel(_("saveStyleSheet.commandkey"))] = () => {
      this.saveToFile();
    };

    return bindings;
  },

  


  destroy: function() {
    this.styleSheet.off("property-change", this._onPropertyChange);
    this.styleSheet.off("error", this._onError);
  }
}


const TAB_CHARS = "\t";

const OS = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
const LINE_SEPARATOR = OS === "WINNT" ? "\r\n" : "\n";











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
        indent = TAB_CHARS.repeat(--indentLevel);
        
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
      indent = TAB_CHARS.repeat(++indentLevel);
    }
  }
  return parts.join(LINE_SEPARATOR);
}

