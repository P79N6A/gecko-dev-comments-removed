



"use strict";

this.EXPORTED_SYMBOLS = ["StyleEditorDebuggee", "StyleSheet"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");

XPCOMUtils.defineLazyModuleGetter(this, "promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js", "Promise");














let StyleEditorDebuggee = function(target) {
  EventEmitter.decorate(this);

  this.styleSheets = [];

  this.clear = this.clear.bind(this);
  this._onNewDocument = this._onNewDocument.bind(this);
  this._onDocumentLoad = this._onDocumentLoad.bind(this);

  this._target = target;
  this._actor = this.target.form.styleEditorActor;

  this.client.addListener("documentLoad", this._onDocumentLoad);
  this._target.on("navigate", this._onNewDocument);

  this._onNewDocument();
}

StyleEditorDebuggee.prototype = {
  


  styleSheets: null,

  


  baseURI: null,

  


  get target() {
    return this._target;
  },

  


  get client() {
    return this._target.client;
  },

  







  styleSheetFromHref: function(href) {
    for (let sheet of this.styleSheets) {
      if (sheet.href == href) {
        return sheet;
      }
    }
    return null;
  },

  


  clear: function() {
    this.baseURI = null;
    this.clearStyleSheets();
  },

  


  clearStyleSheets: function() {
    for (let stylesheet of this.styleSheets) {
      stylesheet.destroy();
    }
    this.styleSheets = [];
    this.emit("stylesheets-cleared");
  },

  



  _onNewDocument: function() {
    this.clear();

    this._getBaseURI();

    let message = { type: "newDocument" };
    this._sendRequest(message);
  },

  


  _getBaseURI: function() {
    let message = { type: "getBaseURI" };
    this._sendRequest(message, (response) => {
      this.baseURI = Services.io.newURI(response.baseURI, null, null);
    });
  },

  








  _onDocumentLoad: function(type, request) {
    if (this.styleSheets.length > 0) {
      this.clearStyleSheets();
    }
    let sheets = [];
    for (let form of request.styleSheets) {
      let sheet = this._addStyleSheet(form);
      sheets.push(sheet);
    }
    this.emit("document-load", sheets);
  },

  






  _addStyleSheet: function(form) {
    let sheet = new StyleSheet(form, this);
    this.styleSheets.push(sheet);
    return sheet;
  },

  








  createStyleSheet: function(text, callback) {
    let message = { type: "newStyleSheet", text: text };
    this._sendRequest(message, (response) => {
      let sheet = this._addStyleSheet(response.styleSheet);
      callback(sheet);
    });
  },

  







  _sendRequest: function(message, callback) {
    message.to = this._actor;
    this.client.request(message, callback);
  },

  


  destroy: function() {
    this.clear();

    this._target.off("navigate", this._onNewDocument);
  }
}
















let StyleSheet = function(form, debuggee) {
  EventEmitter.decorate(this);

  this.debuggee = debuggee;
  this._client = debuggee.client;
  this._actor = form.actor;

  this._onSourceLoad = this._onSourceLoad.bind(this);
  this._onPropertyChange = this._onPropertyChange.bind(this);
  this._onStyleApplied = this._onStyleApplied.bind(this);

  this._client.addListener("sourceLoad", this._onSourceLoad);
  this._client.addListener("propertyChange", this._onPropertyChange);
  this._client.addListener("styleApplied", this._onStyleApplied);

  
  this._client.addListener("sourceLoad-" + this._actor, this._onSourceLoad);
  this._client.addListener("propertyChange-" + this._actor, this._onPropertyChange);
  this._client.addListener("styleApplied-" + this._actor, this._onStyleApplied);


  
  for (let attr in form) {
    this[attr] = form[attr];
  }
}

StyleSheet.prototype = {
  


  toggleDisabled: function() {
    let message = { type: "toggleDisabled" };
    this._sendRequest(message);
  },

  



  fetchSource: function() {
    let message = { type: "fetchSource" };
    this._sendRequest(message);
  },

  





  update: function(sheetText) {
    let message = { type: "update", text: sheetText, transition: true };
    this._sendRequest(message);
  },

  







  _onSourceLoad: function(type, request) {
    if (request.from == this._actor) {
      if (request.error) {
        return this.emit("error", request.error);
      }
      this.emit("source-load", request.source);
    }
  },

  







  _onPropertyChange: function(type, request) {
    if (request.from == this._actor) {
      this[request.property] = request.value;
      this.emit("property-change", request.property);
    }
  },

  


  _onStyleApplied: function(type, request) {
    if (request.from == this._actor) {
      this.emit("style-applied");
    }
  },

  







  _sendRequest: function(message, callback) {
    message.to = this._actor;
    this._client.request(message, callback);
  },

  


  destroy: function() {
    this._client.removeListener("sourceLoad", this._onSourceLoad);
    this._client.removeListener("propertyChange", this._onPropertyChange);
    this._client.removeListener("styleApplied", this._onStyleApplied);

    this._client.removeListener("sourceLoad-" + this._actor, this._onSourceLoad);
    this._client.removeListener("propertyChange-" + this._actor, this._onPropertyChange);
    this._client.removeListener("styleApplied-" + this._actor, this._onStyleApplied);
  }
}
