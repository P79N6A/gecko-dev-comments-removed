



"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

let TRANSITION_CLASS = "moz-styleeditor-transitioning";
let TRANSITION_DURATION_MS = 500;
let TRANSITION_RULE = "\
:root.moz-styleeditor-transitioning, :root.moz-styleeditor-transitioning * {\
transition-duration: " + TRANSITION_DURATION_MS + "ms !important; \
transition-delay: 0ms !important;\
transition-timing-function: ease-out !important;\
transition-property: all !important;\
}";

let LOAD_ERROR = "error-load";





function StyleEditorActor(aConnection, aParentActor)
{
  this.conn = aConnection;
  this._onDocumentLoaded = this._onDocumentLoaded.bind(this);
  this._onSheetLoaded = this._onSheetLoaded.bind(this);
  this.parentActor = aParentActor;

  
  this._sheets = new Map();

  this._actorPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._actorPool);
}

StyleEditorActor.prototype = {
  


  _actorPool: null,

  


  conn: null,

  


  get window() this.parentActor.window,

  


  get document() this.window.document,

  actorPrefix: "styleEditor",

  form: function()
  {
    return { actor: this.actorID };
  },

  


  disconnect: function()
  {
    if (this._observer) {
      this._observer.disconnect();
      delete this._observer;
    }

    this._sheets.clear();

    this.conn.removeActorPool(this._actorPool);
    this._actorPool = null;
    this.conn = null;
  },

  


  releaseActor: function(actor)
  {
    if (this._actorPool) {
      this._actorPool.removeActor(actor.actorID);
    }
  },

  




  onGetBaseURI: function() {
    return { baseURI: this.document.baseURIObject.spec };
  },

  



  onNewDocument: function() {
    
    this._clearStyleSheetActors();

    
    
    if (this.document.readyState == "complete") {
      this._onDocumentLoaded();
    }
    else {
      this.window.addEventListener("load", this._onDocumentLoaded, false);
    }
    return {};
  },

  



  _onDocumentLoaded: function(event) {
    if (event) {
      this.window.removeEventListener("load", this._onDocumentLoaded, false);
    }

    let documents = [this.document];
    var forms = [];
    for (let doc of documents) {
      let sheetForms = this._addStyleSheets(doc.styleSheets);
      forms = forms.concat(sheetForms);
      
      for (let iframe of doc.getElementsByTagName("iframe")) {
        documents.push(iframe.contentDocument);
      }
    }

    this.conn.send({
      from: this.actorID,
      type: "documentLoad",
      styleSheets: forms
    });
  },

  









  _addStyleSheets: function(styleSheets)
  {
    let sheets = [];
    for (let i = 0; i < styleSheets.length; i++) {
      let styleSheet = styleSheets[i];
      sheets.push(styleSheet);

      
      let imports = this._getImported(styleSheet);
      sheets = sheets.concat(imports);
    }

    let forms = sheets.map((sheet) => {
      let actor = this._createStyleSheetActor(sheet);
      return actor.form();
    });

    return forms;
  },

  







  _getImported: function(styleSheet) {
   let imported = [];

   for (let i = 0; i < styleSheet.cssRules.length; i++) {
      let rule = styleSheet.cssRules[i];
      if (rule.type == Ci.nsIDOMCSSRule.IMPORT_RULE) {
        
        
        if (!rule.styleSheet) {
          continue;
        }
        imported.push(rule.styleSheet);

        
        imported = imported.concat(this._getImported(rule.styleSheet));
      }
      else if (rule.type != Ci.nsIDOMCSSRule.CHARSET_RULE) {
        
        break;
      }
    }
    return imported;
  },

  








  _createStyleSheetActor: function(aStyleSheet)
  {
    if (this._sheets.has(aStyleSheet)) {
      return this._sheets.get(aStyleSheet);
    }
    let actor = new StyleSheetActor(aStyleSheet, this);
    this._actorPool.addActor(actor);
    this._sheets.set(aStyleSheet, actor);
    return actor;
  },

  


  _clearStyleSheetActors: function() {
    for (let actor in this._sheets) {
      this.releaseActor(this._sheets[actor]);
    }
    this._sheets.clear();
  },

  




  onGetStyleSheets: function() {
    let forms = this._addStyleSheets(this.document.styleSheets);
    return { "styleSheets": forms };
  },

  





  _onSheetLoaded: function(event) {
    let style = event.target;
    style.removeEventListener("load", this._onSheetLoaded, false);

    let actor = this._createStyleSheetActor(style.sheet);
    this._notifyStyleSheetsAdded([actor.form()]);
  },

  








  onNewStyleSheet: function(request) {
    let parent = this.document.documentElement;
    let style = this.document.createElementNS("http://www.w3.org/1999/xhtml", "style");
    style.setAttribute("type", "text/css");

    if (request.text) {
      style.appendChild(this.document.createTextNode(request.text));
    }
    parent.appendChild(style);

    let actor = this._createStyleSheetActor(style.sheet);
    return { styleSheet: actor.form() };
  }
};




StyleEditorActor.prototype.requestTypes = {
  "getStyleSheets": StyleEditorActor.prototype.onGetStyleSheets,
  "newStyleSheet": StyleEditorActor.prototype.onNewStyleSheet,
  "getBaseURI": StyleEditorActor.prototype.onGetBaseURI,
  "newDocument": StyleEditorActor.prototype.onNewDocument
};


function StyleSheetActor(aStyleSheet, aParentActor) {
  this.styleSheet = aStyleSheet;
  this.parentActor = aParentActor;

  
  this.text = null;
  this._styleSheetIndex = -1;

  this._transitionRefCount = 0;

  this._onSourceLoad = this._onSourceLoad.bind(this);

  
  let ownerNode = this.styleSheet.ownerNode;
  if (ownerNode) {
    let onSheetLoaded = function(event) {
      ownerNode.removeEventListener("load", onSheetLoaded, false);
      this._notifyPropertyChanged("ruleCount");
    }.bind(this);

    ownerNode.addEventListener("load", onSheetLoaded, false);
  }
}

StyleSheetActor.prototype = {
  actorPrefix: "stylesheet",

  toString: function() {
    return "[StyleSheetActor " + this.actorID + "]";
  },

  disconnect: function() {
    this.parentActor.releaseActor(this);
  },

  


  get window() this.parentActor.window,

  


  get document() this.window.document,

  




  get styleSheetIndex()
  {
    if (this._styleSheetIndex == -1) {
      for (let i = 0; i < this.document.styleSheets.length; i++) {
        if (this.document.styleSheets[i] == this.styleSheet) {
          this._styleSheetIndex = i;
          break;
        }
      }
    }
    return this._styleSheetIndex;
  },

  






  form: function() {
    let form = {
      actor: this.actorID,  
      href: this.styleSheet.href,
      disabled: this.styleSheet.disabled,
      title: this.styleSheet.title,
      styleSheetIndex: this.styleSheetIndex,
      text: this.text
    }

    
    let parent = this.styleSheet.parentStyleSheet;
    if (parent) {
      form.parentActor = this.parentActor._sheets.get(parent).form();
    }

    try {
      form.ruleCount = this.styleSheet.cssRules.length;
    }
    catch(e) {
      
    }

    return form;
  },

  





  onToggleDisabled: function() {
    this.styleSheet.disabled = !this.styleSheet.disabled;
    this._notifyPropertyChanged("disabled");

    return { disabled: this.styleSheet.disabled };
  },

  






  _notifyPropertyChanged: function(property) {
    this.conn.send({
      from: this.actorID,
      type: "propertyChange",
      property: property,
      value: this.form()[property]
    })
  },

  










  _onSourceLoad: function(error, source, charset) {
    let message = {
      from: this.actorID,
      type: "sourceLoad",
    };

    if (error) {
      message.error = error;
    }
    else {
      this.text = this._decodeCSSCharset(source, charset || "");
      message.source = this.text;
    }

    this.conn.send(message);
  },

  


  onFetchSource: function() {
    if (!this.styleSheet.href) {
      
      let source = this.styleSheet.ownerNode.textContent;
      this._onSourceLoad(null, source);
      return {};
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
    return {};
  },

  










  _decodeCSSCharset: function(string, channelCharset)
  {
    

    if (channelCharset.length > 0) {
      
      return this._convertToUnicode(string, channelCharset);
    }

    let sheet = this.styleSheet;
    if (sheet) {
      
      
      if (sheet.cssRules) {
        let rules = sheet.cssRules;
        if (rules.length
            && rules.item(0).type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
          return this._convertToUnicode(string, rules.item(0).encoding);
        }
      }

      
      if (sheet.ownerNode && sheet.ownerNode.getAttribute) {
        let linkCharset = sheet.ownerNode.getAttribute("charset");
        if (linkCharset != null) {
          return this._convertToUnicode(string, linkCharset);
        }
      }

      
      let parentSheet = sheet.parentStyleSheet;
      if (parentSheet && parentSheet.cssRules &&
          parentSheet.cssRules[0].type == Ci.nsIDOMCSSRule.CHARSET_RULE) {
        return this._convertToUnicode(string,
            parentSheet.cssRules[0].encoding);
      }

      
      if (sheet.ownerNode && sheet.ownerNode.ownerDocument.characterSet) {
        return this._convertToUnicode(string,
            sheet.ownerNode.ownerDocument.characterSet);
      }
    }

    
    return this._convertToUnicode(string, "UTF-8");
  },

  









  _convertToUnicode: function(string, charset) {
    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Ci.nsIScriptableUnicodeConverter);

    try {
      converter.charset = charset;
      return converter.ConvertToUnicode(string);
    } catch(e) {
      return string;
    }
  },

  





  _loadSourceFromFile: function(href)
  {
    try {
      NetUtil.asyncFetch(href, (stream, status) => {
        if (!Components.isSuccessCode(status)) {
          this._onSourceLoad(LOAD_ERROR);
          return;
        }
        let source = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();
        this._onSourceLoad(null, source);
      });
    } catch (ex) {
      this._onSourceLoad(LOAD_ERROR);
    }
  },

  





  _loadSourceFromCache: function(href)
  {
    let channel = Services.io.newChannel(href, null, null);
    let chunks = [];
    let channelCharset = "";
    let streamListener = { 
      onStartRequest: (aRequest, aContext, aStatusCode) => {
        if (!Components.isSuccessCode(aStatusCode)) {
          this._onSourceLoad(LOAD_ERROR);
        }
      },
      onDataAvailable: (aRequest, aContext, aStream, aOffset, aCount) => {
        let channel = aRequest.QueryInterface(Ci.nsIChannel);
        if (!channelCharset) {
          channelCharset = channel.contentCharset;
        }
        chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
      },
      onStopRequest: (aRequest, aContext, aStatusCode) => {
        if (!Components.isSuccessCode(aStatusCode)) {
          this._onSourceLoad(LOAD_ERROR);
          return;
        }
        let source = chunks.join("");
        this._onSourceLoad(null, source, channelCharset);
      }
    };

    channel.loadGroup = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebNavigation)
                            .QueryInterface(Ci.nsIDocumentLoader)
                            .loadGroup;
    channel.loadFlags = channel.LOAD_FROM_CACHE;
    channel.asyncOpen(streamListener, null);
  },

  






  onUpdate: function(request) {
    DOMUtils.parseStyleSheet(this.styleSheet, request.text);

    this._notifyPropertyChanged("ruleCount");

    if (request.transition) {
      this._insertTransistionRule();
    }
    else {
      this._notifyStyleApplied();
    }

    return {};
  },

  



  _insertTransistionRule: function() {
    
    
    
    if (this._transitionRefCount == 0) {
      this.styleSheet.insertRule(TRANSITION_RULE, this.styleSheet.cssRules.length);
      this.document.documentElement.classList.add(TRANSITION_CLASS);
    }

    this._transitionRefCount++;

    
    
    this.window.setTimeout(this._onTransitionEnd.bind(this),
                           Math.floor(TRANSITION_DURATION_MS * 1.1));
  },

  



  _onTransitionEnd: function()
  {
    if (--this._transitionRefCount == 0) {
      this.document.documentElement.classList.remove(TRANSITION_CLASS);
      this.styleSheet.deleteRule(this.styleSheet.cssRules.length - 1);
    }

    this._notifyStyleApplied();
  },

  


  _notifyStyleApplied: function()
  {
    this.conn.send({
      from: this.actorID,
      type: "styleApplied"
    })
  }
}

StyleSheetActor.prototype.requestTypes = {
  "toggleDisabled": StyleSheetActor.prototype.onToggleDisabled,
  "fetchSource": StyleSheetActor.prototype.onFetchSource,
  "update": StyleSheetActor.prototype.onUpdate
};

DebuggerServer.addTabActor(StyleEditorActor, "styleEditorActor");

XPCOMUtils.defineLazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
