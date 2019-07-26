


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const events = require("sdk/event/core");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});






function ContentObserver(tabActor) {
  this._contentWindow = tabActor.window;
  this._onContentGlobalCreated = this._onContentGlobalCreated.bind(this);
  this._onInnerWindowDestroyed = this._onInnerWindowDestroyed.bind(this);
  this.startListening();
}

module.exports.ContentObserver = ContentObserver;

ContentObserver.prototype = {
  


  startListening: function() {
    Services.obs.addObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.addObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  stopListening: function() {
    Services.obs.removeObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.removeObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  _onContentGlobalCreated: function(subject, topic, data) {
    if (subject == this._contentWindow) {
      events.emit(this, "global-created", subject);
    }
  },

  


  _onInnerWindowDestroyed: function(subject, topic, data) {
    let id = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    events.emit(this, "global-destroyed", id);
  }
};



ContentObserver.GetInnerWindowID = function(window) {
  return window
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils)
    .currentInnerWindowID;
};
