




































"use strict";

const EXPORTED_SYMBOLS = ["StyleEditor", "StyleEditorFlags"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/source-editor.jsm");

const LOAD_ERROR = "error-load";
const SAVE_ERROR = "error-save";



const UPDATE_STYLESHEET_THROTTLE_DELAY = 500;


const STYLESHEET_EXPANDO = "-moz-styleeditor-stylesheet-";



















function StyleEditor(aDocument, aStyleSheet)
{
  assert(aDocument, "Argument 'aDocument' is required.");

  this._document = aDocument; 
  this._inputElement = null;  
  this._sourceEditor = null;  

  this._state = {             
    text: "",                 
    selection: {start: 0, end: 0},
    readOnly: false
  };

  this._styleSheet = aStyleSheet;
  this._styleSheetIndex = -1; 

  this._loaded = false;

  this._flags = [];           
  this._savedFile = null;     

  this._errorMessage = null;  

  
  this._actionListeners = [];

  
  this._onWindowUnloadBinding = this._onWindowUnload.bind(this);
  
  this._onInputElementFocusBinding = this._onInputElementFocus.bind(this);
  this._focusOnSourceEditorReady = false;
}

StyleEditor.prototype = {
  




  get contentDocument() this._document,

  




  get styleSheet()
  {
    assert(this._styleSheet, "StyleSheet must be loaded first.")
    return this._styleSheet;
  },

  




  get styleSheetIndex()
  {
    let document = this.contentDocument;
    if (this._styleSheetIndex == -1) {
      for (let i = 0; i < document.styleSheets.length; ++i) {
        if (document.styleSheets[i] == this.styleSheet) {
          this._styleSheetIndex = i;
          break;
        }
      }
    }
    return this._styleSheetIndex;
  },

  






  get inputElement() this._inputElement,

  





  set inputElement(aElement)
  {
    if (aElement == this._inputElement) {
      return; 
    }

    if (this._inputElement) {
      
      if (this._sourceEditor) {
        
        this._state = {
          text: this._sourceEditor.getText(),
          selection: this._sourceEditor.getSelection(),
          readOnly: this._sourceEditor.readOnly
        };
        this._sourceEditor.destroy();
        this._sourceEditor = null;
      }

      this.window.removeEventListener("unload",
                                      this._onWindowUnloadBinding, false);
      this._inputElement.removeEventListener("focus",
        this._onInputElementFocusBinding, true);
      this._triggerAction("Detach");
    }

    this._inputElement = aElement;
    if (!aElement) {
      return;
    }

    
    this.window.addEventListener("unload", this._onWindowUnloadBinding, false);
    this._focusOnSourceEditorReady = false;
    aElement.addEventListener("focus", this._onInputElementFocusBinding, true);

    this._sourceEditor = null; 

    let sourceEditor = new SourceEditor();
    let config = {
      placeholderText: this._state.text, 
      showLineNumbers: true,
      mode: SourceEditor.MODES.CSS,
      readOnly: this._state.readOnly,
      keys: this._getKeyBindings()
    };

    sourceEditor.init(aElement, config, function onSourceEditorReady() {
      sourceEditor.setSelection(this._state.selection.start,
                                this._state.selection.end);

      if (this._focusOnSourceEditorReady) {
        sourceEditor.focus();
      }

      sourceEditor.addEventListener(SourceEditor.EVENTS.TEXT_CHANGED,
                                    function onTextChanged(aEvent) {
        this.updateStyleSheet();
      }.bind(this));

      this._sourceEditor = sourceEditor;
      this._triggerAction("Attach");
    }.bind(this));
  },

  





  get sourceEditor() this._sourceEditor,

  





  set readOnly(aValue)
  {
    this._state.readOnly = aValue;
    if (this._sourceEditor) {
      this._sourceEditor.readOnly = aValue;
    }
  },

  




  get readOnly()
  {
    return this._state.readOnly;
  },

  





  get window()
  {
    if (!this.inputElement) {
      return null;
    }
    return this.inputElement.ownerDocument.defaultView;
  },

  




  get savedFile() this._savedFile,

  









  importFromFile: function SE_importFromFile(aFile, aParentWindow)
  {
    aFile = this._showFilePicker(aFile, false, aParentWindow);
    if (!aFile) {
      return;
    }
    this._savedFile = aFile; 

    NetUtil.asyncFetch(aFile, function onAsyncFetch(aStream, aStatus) {
      if (!Components.isSuccessCode(aStatus)) {
        return this._signalError(LOAD_ERROR);
      }
      let source = NetUtil.readInputStreamToString(aStream, aStream.available());
      aStream.close();

      this._appendNewStyleSheet(source);
      this.clearFlag(StyleEditorFlags.ERROR);
    }.bind(this));
  },

  





  get errorMessage() this._errorMessage,

  




  get isLoaded() this._loaded,

  





  load: function SE_load()
  {
    if (!this._styleSheet) {
      this._flags.push(StyleEditorFlags.NEW);
      this._appendNewStyleSheet();
    }
    this._loadSource();
  },

  




  getFriendlyName: function SE_getFriendlyName()
  {
    if (this.savedFile) { 
      return this.savedFile.leafName;
    }

    if (this.hasFlag(StyleEditorFlags.NEW)) {
      let index = this.styleSheetIndex + 1; 
      return _("newStyleSheet", index);
    }

    if (this.hasFlag(StyleEditorFlags.INLINE)) {
      let index = this.styleSheetIndex + 1; 
      return _("inlineStyleSheet", index);
    }

    if (!this._friendlyName) {
      let sheetURI = this.styleSheet.href;
      let contentURI = this.contentDocument.baseURIObject;
      let contentURIScheme = contentURI.scheme;
      let contentURILeafIndex = contentURI.specIgnoringRef.lastIndexOf("/");
      contentURI = contentURI.specIgnoringRef;

      
      if (contentURILeafIndex > contentURIScheme.length) {
        contentURI = contentURI.substring(0, contentURILeafIndex + 1);
      }

      
      
      this._friendlyName = (sheetURI.indexOf(contentURI) == 0)
                           ? sheetURI.substring(contentURI.length)
                           : sheetURI;
    }
    return this._friendlyName;
  },

  






























  addActionListener: function SE_addActionListener(aListener)
  {
    this._actionListeners.push(aListener);
  },

  





  removeActionListener: function SE_removeActionListener(aListener)
  {
    let index = this._actionListeners.indexOf(aListener);
    if (index != -1) {
      this._actionListeners.splice(index, 1);
    }
  },

  













  






  get flags() this._flags.join(" "),

  








  setFlag: function SE_setFlag(aName)
  {
    let prop = aName.toUpperCase();
    assert(StyleEditorFlags[prop], "Unknown flag: " + prop);

    if (this.hasFlag(aName)) {
      return false;
    }
    this._flags.push(aName);
    this._triggerAction("FlagChange", [aName]);
    return true;
  },

  







  clearFlag: function SE_clearFlag(aName)
  {
    let index = this._flags.indexOf(aName);
    if (index == -1) {
      return false;
    }
    this._flags.splice(index, 1);
    this._triggerAction("FlagChange", [aName]);
    return true;
  },

  









  toggleFlag: function SE_toggleFlag(aCondition, aName)
  {
    return (aCondition) ? this.setFlag(aName) : this.clearFlag(aName);
  },

  







  hasFlag: function SE_hasFlag(aName) (this._flags.indexOf(aName) != -1),

  




  enableStyleSheet: function SE_enableStyleSheet(aEnabled)
  {
    this.styleSheet.disabled = !aEnabled;
    this.toggleFlag(this.styleSheet.disabled, StyleEditorFlags.DISABLED);

    if (this._updateTask) {
      this._updateStyleSheet(); 
    }
  },

  













  saveToFile: function SE_saveToFile(aFile, aCallback)
  {
    aFile = this._showFilePicker(aFile, true);
    if (!aFile) {
      if (aCallback) {
        aCallback(null);
      }
      return;
    }

    if (this._sourceEditor) {
      this._state.text = this._sourceEditor.getText();
    }

    let ostream = FileUtils.openSafeFileOutputStream(aFile);
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let istream = converter.convertToInputStream(this._state.text);

    NetUtil.asyncCopy(istream, ostream, function SE_onStreamCopied(status) {
      if (!Components.isSuccessCode(status)) {
        if (aCallback) {
          aCallback(null);
        }
        this._signalError(SAVE_ERROR);
        return;
      }
      FileUtils.closeSafeFileOutputStream(ostream);

      
      this._friendlyName = null;
      this._savedFile = aFile;
      this._persistExpando();

      if (aCallback) {
        aCallback(aFile);
      }
      this.clearFlag(StyleEditorFlags.UNSAVED);
      this.clearFlag(StyleEditorFlags.ERROR);
    }.bind(this));
  },

  





  updateStyleSheet: function SE_updateStyleSheet(aImmediate)
  {
    let window = this.window;

    if (this._updateTask) {
      
      window.clearTimeout(this._updateTask);
    }

    if (aImmediate) {
      this._updateStyleSheet();
    } else {
      this._updateTask = window.setTimeout(this._updateStyleSheet.bind(this),
                                           UPDATE_STYLESHEET_THROTTLE_DELAY);
    }
  },

  


  _updateStyleSheet: function SE__updateStyleSheet()
  {
    this.setFlag(StyleEditorFlags.UNSAVED);

    if (this.styleSheet.disabled) {
      return;
    }

    this._updateTask = null; 
                             
                             
                             

    if (this.sourceEditor) {
      this._state.text = this.sourceEditor.getText();
    }
    let source = this._state.text;
    let oldNode = this.styleSheet.ownerNode;
    let oldIndex = this.styleSheetIndex;

    let newNode = this.contentDocument.createElement("style");
    newNode.setAttribute("type", "text/css");
    newNode.appendChild(this.contentDocument.createTextNode(source));
    oldNode.parentNode.replaceChild(newNode, oldNode);

    this._styleSheet = this.contentDocument.styleSheets[oldIndex];
    this._persistExpando();

    this._triggerAction("Commit");
  },

  












  _showFilePicker: function SE__showFilePicker(aFile, aSave, aParentWindow)
  {
    if (typeof(aFile) == "string") {
      try {
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        file.initWithPath(aFile);
        return file;
      } catch (ex) {
        this._signalError(aSave ? SAVE_ERROR : LOAD_ERROR);
        return null;
      }
    }
    if (aFile) {
      return aFile;
    }

    let window = aParentWindow
                 ? aParentWindow
                 : this.inputElement.ownerDocument.defaultView;
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let mode = aSave ? fp.modeSave : fp.modeOpen;
    let key = aSave ? "saveStyleSheet" : "importStyleSheet";

    fp.init(window, _(key + ".title"), mode);
    fp.appendFilters(_(key + ".filter"), "*.css");
    fp.appendFilters(fp.filterAll);

    let rv = fp.show();
    return (rv == fp.returnCancel) ? null : fp.file;
  },

  


  _loadSource: function SE__loadSource()
  {
    if (!this.styleSheet.href) {
      
      this._flags.push(StyleEditorFlags.INLINE);
      this._onSourceLoad(this.styleSheet.ownerNode.textContent);
      return;
    }

    let scheme = Services.io.extractScheme(this.styleSheet.href);
    switch (scheme) {
      case "file":
      case "chrome":
      case "resource":
        this._loadSourceFromFile(this.styleSheet.href);
        break;
      default:
        this._loadSourceFromCache(this.styleSheet.href);
        break;
    }
  },

  





  _loadSourceFromFile: function SE__loadSourceFromFile(aHref)
  {
    try {
      NetUtil.asyncFetch(aHref, function onFetch(aStream, aStatus) {
        if (!Components.isSuccessCode(aStatus)) {
          return this._signalError(LOAD_ERROR);
        }
        let source = NetUtil.readInputStreamToString(aStream, aStream.available());
        aStream.close();
        this._onSourceLoad(source);
      }.bind(this));
    } catch (ex) {
      this._signalError(LOAD_ERROR);
    }
  },

  





  _loadSourceFromCache: function SE__loadSourceFromCache(aHref)
  {
    try {
      let cacheService = Cc["@mozilla.org/network/cache-service;1"]
                           .getService(Ci.nsICacheService);
      let session = cacheService.createSession("HTTP", Ci.nsICache.STORE_ANYWHERE, true);
      session.doomEntriesIfExpired = false;
      session.asyncOpenCacheEntry(aHref, Ci.nsICache.ACCESS_READ, {
        onCacheEntryAvailable: this._onCacheEntryAvailable.bind(this)
      });
    } catch (ex) {
      this._signalError(LOAD_ERROR);
    }
  },

   







  _onCacheEntryAvailable: function SE__onCacheEntryAvailable(aEntry, aMode, aStatus)
  {
    if (!Components.isSuccessCode(aStatus)) {
      return this._signalError(LOAD_ERROR);
    }

    let stream = aEntry.openInputStream(0);
    let chunks = [];
    let streamListener = { 
      onStartRequest: function (aRequest, aContext, aStatusCode) {
      },
      onDataAvailable: function (aRequest, aContext, aStream, aOffset, aCount) {
        chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
      },
      onStopRequest: function (aRequest, aContext, aStatusCode) {
        this._onSourceLoad(chunks.join(""));
      }.bind(this),
    };

    let head = aEntry.getMetaDataElement("response-head");
    if (/^Content-Encoding:\s*gzip/mi.test(head)) {
      let converter = Cc["@mozilla.org/streamconv;1?from=gzip&to=uncompressed"]
                        .createInstance(Ci.nsIStreamConverter);
      converter.asyncConvertData("gzip", "uncompressed", streamListener, null);
      streamListener = converter; 
    }

    try {
      streamListener.onStartRequest(null, null);
      while (stream.available()) {
        streamListener.onDataAvailable(null, null, stream, 0, stream.available());
      }
      streamListener.onStopRequest(null, null, 0);
    } catch (ex) {
      this._signalError(LOAD_ERROR);
    } finally {
      try {
        stream.close();
      } catch (ex) {
        
      }
      aEntry.close();
    }
  },

  




  _onSourceLoad: function SE__onSourceLoad(aSourceText)
  {
    this._restoreExpando();
    this._state.text = prettifyCSS(aSourceText);
    this._loaded = true;
    this._triggerAction("Load");
  },

  





  _appendNewStyleSheet: function SE__appendNewStyleSheet(aText)
  {
    let document = this.contentDocument;
    let parent = document.documentElement;
    let style = document.createElement("style");
    style.setAttribute("type", "text/css");
    if (aText) {
      style.appendChild(document.createTextNode(aText));
    }
    parent.appendChild(style);

    this._styleSheet = document.styleSheets[document.styleSheets.length - 1];
    this._flags.push(aText ? StyleEditorFlags.IMPORTED : StyleEditorFlags.NEW);
    if (aText) {
      this._onSourceLoad(aText);
    }
  },

  








  _signalError: function SE__signalError(aErrorCode)
  {
    this._errorMessage = _.apply(null, arguments);
    this.setFlag(StyleEditorFlags.ERROR);
  },

  








  _triggerAction: function SE__triggerAction(aName, aArgs)
  {
    
    if (!aArgs) {
      aArgs = [this];
    } else {
      aArgs.unshift(this);
    }

    
    for (let i = 0; i < this._actionListeners.length; ++i) {
      let listener = this._actionListeners[i];
      let actionHandler = listener["on" + aName];
      if (actionHandler) {
        actionHandler.apply(listener, aArgs);
      }
    }

    
    if (aName == "FlagChange") {
      this._persistExpando();
    }
  },

  


  _onWindowUnload: function SE__onWindowUnload(aEvent)
  {
    if (this._updateTask) {
      this.updateStyleSheet(true);
    }
  },

  






  _onInputElementFocus: function SE__onInputElementFocus(aEvent)
  {
    if (this._sourceEditor) {
      this._sourceEditor.focus();
    } else {
      this._focusOnSourceEditorReady = true;
    }
  },

  






  _persistExpando: function SE__persistExpando() {
    if (!this._styleSheet) {
      return; 
    }
    let name = STYLESHEET_EXPANDO + this.styleSheetIndex;
    let expando = this.contentDocument.getUserData(name);
    if (!expando) {
      expando = {};
      this.contentDocument.setUserData(name, expando, null);
    }
    expando._flags = this._flags;
    expando._savedFile = this._savedFile;
  },

  




  _restoreExpando: function SE__restoreExpando() {
    if (!this._styleSheet) {
      return; 
    }
    let name = STYLESHEET_EXPANDO + this.styleSheetIndex;
    let expando = this.contentDocument.getUserData(name);
    if (expando) {
      this._flags = expando._flags;
      this._savedFile = expando._savedFile;
    }
  },

  





  _getKeyBindings: function () {
    let bindings = [];

    bindings.push({
      action: "StyleEditor.save",
      code: _("saveStyleSheet.commandkey"),
      accel: true,
      callback: function save() {
        this.saveToFile(this._savedFile);
      }.bind(this)
    });
    bindings.push({
      action: "StyleEditor.saveAs",
      code: _("saveStyleSheet.commandkey"),
      accel: true,
      shift: true,
      callback: function saveAs() {
        this.saveToFile();
      }.bind(this)
    });

    return bindings;
  }
};







let StyleEditorFlags = {
  DISABLED:      "disabled",
  ERROR:         "error",
  IMPORTED:      "imported",
  INLINE:        "inline",
  MODIFIED:      "modified",
  NEW:           "new",
  UNSAVED:       "unsaved"
};


const TAB_CHARS   = "\t";

const OS = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
const LINE_SEPARATOR = OS === "WINNT" ? "\r\n" : "\n";











function prettifyCSS(aText)
{
  
  aText = aText.replace(/(?:^\s*<!--[\r\n]*)|(?:\s*-->\s*$)/g, "");

  let parts = [];    
  let partStart = 0; 
  let indent = "";
  let indentLevel = 0;

  for (let i = 0; i < aText.length; i++) {
    let c = aText[i];
    let shouldIndent = false;

    switch (c) {
      case "}":
        if (i - partStart > 1) {
          
          parts.push(indent + aText.substring(partStart, i));
          partStart = i;
        }
        indent = repeat(TAB_CHARS, --indentLevel);
        
      case ";":
      case "{":
        shouldIndent = true;
        break;
    }

    if (shouldIndent) {
      let la = aText[i+1]; 
      if (!/\s/.test(la)) {
        
        
        parts.push(indent + aText.substring(partStart, i + 1));
        if (c == "}") {
          parts.push(""); 
        }
        partStart = i + 1;
      } else {
        return aText; 
      }
    }

    if (c == "{") {
      indent = repeat(TAB_CHARS, ++indentLevel);
    }
  }
  return parts.join(LINE_SEPARATOR);
}








function repeat(aText, aCount)
{
  return (new Array(aCount + 1)).join(aText);
}
