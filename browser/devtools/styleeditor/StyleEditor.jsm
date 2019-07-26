




"use strict";

this.EXPORTED_SYMBOLS = ["StyleEditor", "StyleEditorFlags"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"]
                   .getService(Ci.inIDOMUtils);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");
Cu.import("resource:///modules/source-editor.jsm");

const LOAD_ERROR = "error-load";
const SAVE_ERROR = "error-save";



const UPDATE_STYLESHEET_THROTTLE_DELAY = 500;


const STYLESHEET_EXPANDO = "-moz-styleeditor-stylesheet-";

const TRANSITIONS_PREF = "devtools.styleeditor.transitions";

const TRANSITION_CLASS = "moz-styleeditor-transitioning";
const TRANSITION_DURATION_MS = 500;
const TRANSITION_RULE = "\
:root.moz-styleeditor-transitioning, :root.moz-styleeditor-transitioning * {\
transition-duration: " + TRANSITION_DURATION_MS + "ms !important; \
transition-delay: 0ms !important;\
transition-timing-function: ease-out !important;\
transition-property: all !important;\
}";




const TRANSITIONS_ENABLED = Services.prefs.getBoolPref(TRANSITIONS_PREF);



















this.StyleEditor = function StyleEditor(aDocument, aStyleSheet)
{
  assert(aDocument, "Argument 'aDocument' is required.");

  this._document = aDocument; 
  this._inputElement = null;  
  this._sourceEditor = null;  

  this._state = {             
    text: "",                 
    selection: {start: 0, end: 0},
    readOnly: false,
    topIndex: 0,              
  };

  this._styleSheet = aStyleSheet;
  this._styleSheetIndex = -1; 
  this._styleSheetFilePath = null; 

  this._loaded = false;

  this._flags = [];           
  this._savedFile = null;     

  this._errorMessage = null;  

  
  this._actionListeners = [];

  
  this._onWindowUnloadBinding = this._onWindowUnload.bind(this);

  this._transitionRefCount = 0;

  this._focusOnSourceEditorReady = false;
}

StyleEditor.prototype = {
  




  get contentDocument() this._document,

  




  get styleSheet()
  {
    assert(this._styleSheet, "StyleSheet must be loaded first.");
    return this._styleSheet;
  },

  







  _getImportedStyleSheetIndex: function SE__getImportedStyleSheetIndex(aIndex, aSheet)
  {
    let index = aIndex;
    for (let j = 0; j < aSheet.cssRules.length; j++) {
      let rule = aSheet.cssRules.item(j);
      if (rule.type == Ci.nsIDOMCSSRule.IMPORT_RULE) {
        
        
        if (!rule.styleSheet) {
          continue;
        }

        if (rule.styleSheet == this.styleSheet) {
          this._styleSheetIndex = index;
          return index;
        }
        index++;
        index = this._getImportedStyleSheetIndex(index, rule.styleSheet);

        if (this._styleSheetIndex != -1) {
          return index;
        }
      } else if (rule.type != Ci.nsIDOMCSSRule.CHARSET_RULE) {
        
        return index;
      }
    }
    return index;
  },

  




  get styleSheetIndex()
  {
    let document = this.contentDocument;
    if (this._styleSheetIndex == -1) {
      let index = 0;
      let sheetIndex = 0;
      while (sheetIndex <= document.styleSheets.length) {
        let sheet = document.styleSheets[sheetIndex];
        if (sheet == this.styleSheet) {
          this._styleSheetIndex = index;
          break;
        }
        index++;
        index = this._getImportedStyleSheetIndex(index, sheet);
        if (this._styleSheetIndex != -1) {
          break;
        }
        sheetIndex++;
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
          readOnly: this._sourceEditor.readOnly,
          topIndex: this._sourceEditor.getTopIndex(),
        };
        this._sourceEditor.destroy();
        this._sourceEditor = null;
      }

      this.window.removeEventListener("unload",
                                      this._onWindowUnloadBinding, false);
      this._triggerAction("Detach");
    }

    this._inputElement = aElement;
    if (!aElement) {
      return;
    }

    
    this.window.addEventListener("unload", this._onWindowUnloadBinding, false);
    this._focusOnSourceEditorReady = false;

    this._sourceEditor = null; 

    let sourceEditor = new SourceEditor();
    let config = {
      initialText: this._state.text,
      showLineNumbers: true,
      mode: SourceEditor.MODES.CSS,
      readOnly: this._state.readOnly,
      keys: this._getKeyBindings()
    };

    sourceEditor.init(aElement, config, function onSourceEditorReady() {
      setupBracketCompletion(sourceEditor);

      sourceEditor.addEventListener(SourceEditor.EVENTS.TEXT_CHANGED,
                                    function onTextChanged(aEvent) {
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
    let callback = function(aFile) {
      if (aFile) {
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
      }
    }.bind(this);

    this._showFilePicker(aFile, false, aParentWindow, callback);
  },

  





  get errorMessage() this._errorMessage,

  




  get isLoaded() this._loaded,

  





  load: function SE_load()
  {
    if (!this._styleSheet) {
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
      try {
        this._friendlyName = decodeURI(this._friendlyName);
      } catch (ex) {
      }
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
    let callback = function(aReturnFile) {
      if (!aReturnFile) {
        if (aCallback) {
          aCallback(null);
        }
        return;
      }

      if (this._sourceEditor) {
        this._state.text = this._sourceEditor.getText();
      }

      let ostream = FileUtils.openSafeFileOutputStream(aReturnFile);
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
        this._savedFile = aReturnFile;
        this._persistExpando();

        if (aCallback) {
          aCallback(aReturnFile);
        }
        this.clearFlag(StyleEditorFlags.UNSAVED);
        this.clearFlag(StyleEditorFlags.ERROR);
      }.bind(this));
    }.bind(this);

    this._showFilePicker(aFile || this._styleSheetFilePath, true, null, callback);
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
    DOMUtils.parseStyleSheet(this.styleSheet, this._state.text);
    this._persistExpando();

    if (!TRANSITIONS_ENABLED) {
      this._triggerAction("Update");
      this._triggerAction("Commit");
      return;
    }

    let content = this.contentDocument;

    
    
    
    if (!this._transitionRefCount) {
      this.styleSheet.insertRule(TRANSITION_RULE, this.styleSheet.cssRules.length);
      content.documentElement.classList.add(TRANSITION_CLASS);
    }

    this._transitionRefCount++;

    
    
    content.defaultView.setTimeout(this._onTransitionEnd.bind(this),
                                   Math.floor(TRANSITION_DURATION_MS * 1.1));

    this._triggerAction("Update");
  },

  



  _onTransitionEnd: function SE__onTransitionEnd()
  {
    if (--this._transitionRefCount == 0) {
      this.contentDocument.documentElement.classList.remove(TRANSITION_CLASS);
      this.styleSheet.deleteRule(this.styleSheet.cssRules.length - 1);
    }

    this._triggerAction("Commit");
  },

  













  _showFilePicker: function SE__showFilePicker(aFile, aSave, aParentWindow, aCallback)
  {
    if (typeof(aFile) == "string") {
      try {
        if (Services.io.extractScheme(aFile) == "file") {
          let uri = Services.io.newURI(aFile, null, null);
          let file = uri.QueryInterface(Ci.nsIFileURL).file;
          aCallback(file);
          return;
        }
      } catch (ex) {
      }
      try {
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        file.initWithPath(aFile);
        aCallback(file);
        return;
      } catch (ex) {
        this._signalError(aSave ? SAVE_ERROR : LOAD_ERROR);
        aCallback(null);
        return;
      }
    }
    if (aFile) {
      aCallback(aFile);
      return;
    }

    let window = aParentWindow
                 ? aParentWindow
                 : this.inputElement.ownerDocument.defaultView;
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let mode = aSave ? fp.modeSave : fp.modeOpen;
    let key = aSave ? "saveStyleSheet" : "importStyleSheet";
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult == Ci.nsIFilePicker.returnCancel) {
        aCallback(null);
      } else {
        aCallback(fp.file);
      }
    };

    fp.init(window, _(key + ".title"), mode);
    fp.appendFilters(_(key + ".filter"), "*.css");
    fp.appendFilters(fp.filterAll);
    fp.open(fpCallback);
    return;
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
        this._styleSheetFilePath = this.styleSheet.href;
      case "chrome":
      case "resource":
        this._loadSourceFromFile(this.styleSheet.href);
        break;
      default:
        this._loadSourceFromCache(this.styleSheet.href);
        break;
    }
  },

  










  _decodeCSSCharset: function SE__decodeCSSCharset(aString, aChannelCharset)
  {
    

    if (aChannelCharset.length > 0) {
      
      return this._convertToUnicode(aString, aChannelCharset);
    }

    let sheet = this.styleSheet;
    if (sheet) {
      
      
      if (sheet.cssRules) {
        let rules = sheet.cssRules;
        if (rules.length
            && rules.item(0).type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
          return this._convertToUnicode(aString, rules.item(0).encoding);
        }
      }

      
      if (sheet.ownerNode && sheet.ownerNode.getAttribute) {
        let linkCharset = sheet.ownerNode.getAttribute("charset");
        if (linkCharset != null) {
          return this._convertToUnicode(aString, linkCharset);
        }
      }

      
      let parentSheet = sheet.parentStyleSheet;
      if (parentSheet && parentSheet.cssRules &&
          parentSheet.cssRules[0].type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
        return this._convertToUnicode(aString,
            parentSheet.cssRules[0].encoding);
      }

      
      if (sheet.ownerNode && sheet.ownerNode.ownerDocument.characterSet) {
        return this._convertToUnicode(aString,
            sheet.ownerNode.ownerDocument.characterSet);
      }
    }

    
    return this._convertToUnicode(aString, "UTF-8");
  },

  








  _convertToUnicode: function SE__convertToUnicode(aString, aCharset) {
    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Ci.nsIScriptableUnicodeConverter);

    try {
      converter.charset = aCharset;
      return converter.ConvertToUnicode(aString);
    } catch(e) {
      return aString;
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
    let channel = Services.io.newChannel(aHref, null, null);
    let chunks = [];
    let channelCharset = "";
    let streamListener = { 
      onStartRequest: function (aRequest, aContext, aStatusCode) {
        if (!Components.isSuccessCode(aStatusCode)) {
          return this._signalError(LOAD_ERROR);
        }
      }.bind(this),
      onDataAvailable: function (aRequest, aContext, aStream, aOffset, aCount) {
        let channel = aRequest.QueryInterface(Ci.nsIChannel);
        if (!channelCharset) {
          channelCharset = channel.contentCharset;
        }
        chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
      },
      onStopRequest: function (aRequest, aContext, aStatusCode) {
        if (!Components.isSuccessCode(aStatusCode)) {
          return this._signalError(LOAD_ERROR);
        }

        this._onSourceLoad(chunks.join(""), channelCharset);
      }.bind(this)
    };

    if (channel instanceof Ci.nsIPrivateBrowsingChannel) {
      let contentWin = this.contentDocument.defaultView;
      let loadContext = contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsILoadContext);
      channel.setPrivate(loadContext.usePrivateBrowsing);
    }
    channel.loadFlags = channel.LOAD_FROM_CACHE;
    channel.asyncOpen(streamListener, null);
  },

  








  _onSourceLoad: function SE__onSourceLoad(aSourceText, aCharset)
  {
    aSourceText = this._decodeCSSCharset(aSourceText, aCharset || "");
    this._restoreExpando();
    this._state.text = prettifyCSS(aSourceText);
    this._loaded = true;
    this._triggerAction("Load");
  },

  





  _appendNewStyleSheet: function SE__appendNewStyleSheet(aText)
  {
    let document = this.contentDocument;
    let parent = document.documentElement;
    let style = document.createElementNS("http://www.w3.org/1999/xhtml", "style");
    style.setAttribute("type", "text/css");
    if (aText) {
      style.appendChild(document.createTextNode(aText));
    }
    parent.appendChild(style);

    this._styleSheet = document.styleSheets[document.styleSheets.length - 1];
    if (aText) {
      this._onSourceLoad(aText);
      this._flags.push(StyleEditorFlags.IMPORTED);
    } else {
      this._flags.push(StyleEditorFlags.NEW);
      this._flags.push(StyleEditorFlags.UNSAVED);
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

    
    let listeners = this._actionListeners.concat();

    
    for (let i = 0; i < listeners.length; i++) {
      let listener = listeners[i];
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

  


  focus: function SE_focus()
  {
    if (this._sourceEditor) {
      this._sourceEditor.focus();
    } else {
      this._focusOnSourceEditorReady = true;
    }
  },

  



  onShow: function SE_onShow()
  {
    if (this._sourceEditor) {
      this._sourceEditor.setTopIndex(this._state.topIndex);
    }
    this.focus();
  },

  



  onHide: function SE_onHide()
  {
    if (this._sourceEditor) {
      this._state.topIndex = this._sourceEditor.getTopIndex();
    }
  },

  






  _persistExpando: function SE__persistExpando()
  {
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

  




  _restoreExpando: function SE__restoreExpando()
  {
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

  





  _getKeyBindings: function SE__getKeyBindings()
  {
    let bindings = [];

    bindings.push({
      action: "StyleEditor.save",
      code: _("saveStyleSheet.commandkey"),
      accel: true,
      callback: function save() {
        this.saveToFile(this._savedFile);
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
  }
};







this.StyleEditorFlags = {
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







function setupBracketCompletion(aSourceEditor)
{
  let editorElement = aSourceEditor.editorElement;
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

  editorElement.addEventListener("keypress", function onKeyPress(aEvent) {
    let pair = pairs[aEvent.charCode];
    if (!pair || aEvent.ctrlKey || aEvent.metaKey ||
        aEvent.accelKey || aEvent.altKey) {
      return true;
    }

    
    let keyCode = pair.closeKeyCode;
    let charCode = pair.closeString.charCodeAt(0);
    let modifiers = 0;
    let utils = editorElement.ownerDocument.defaultView.
                  QueryInterface(Ci.nsIInterfaceRequestor).
                  getInterface(Ci.nsIDOMWindowUtils);
    let handled = utils.sendKeyEvent("keydown", keyCode, 0, modifiers);
    utils.sendKeyEvent("keypress", 0, charCode, modifiers, !handled);
    utils.sendKeyEvent("keyup", keyCode, 0, modifiers);
    
    aSourceEditor.setCaretOffset(aSourceEditor.getCaretOffset() - 1);
  }, false);
}
