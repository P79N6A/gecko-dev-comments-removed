





"use strict";

const { Cu } = require("chrome");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils.js");
const promise = require("promise");
const { XPCOMUtils } = require("resource://gre/modules/XPCOMUtils.jsm");
const { BrowserTabList } = require("devtools/server/actors/webbrowser");

XPCOMUtils.defineLazyGetter(this, "Frames", function() {
  const { Frames } =
    Cu.import("resource://gre/modules/Frames.jsm", {});
  return Frames;
});





function B2GTabList(connection) {
  BrowserTabList.call(this, connection);
  this._listening = false;
}

B2GTabList.prototype = Object.create(BrowserTabList.prototype);

B2GTabList.prototype._getBrowsers = function() {
  return Frames.list().filter(frame => {
    
    return !frame.getAttribute("mozapp");
  });
};

B2GTabList.prototype._getSelectedBrowser = function() {
  return this._getBrowsers().find(frame => {
    
    return !frame.classList.contains("hidden");
  });
};

B2GTabList.prototype._checkListening = function() {
  
  
  this._listenForEventsIf(this._onListChanged && this._mustNotify ||
                          this._actorByBrowser.size > 0);
};

B2GTabList.prototype._listenForEventsIf = function(shouldListen) {
  if (this._listening != shouldListen) {
    let op = shouldListen ? "addObserver" : "removeObserver";
    Frames[op](this);
    this._listening = shouldListen;
  }
};

B2GTabList.prototype.onFrameCreated = function(frame) {
  let mozapp = frame.getAttribute("mozapp");
  if (mozapp) {
    
    return;
  }
  this._notifyListChanged();
  this._checkListening();
};

B2GTabList.prototype.onFrameDestroyed = function(frame) {
  let mozapp = frame.getAttribute("mozapp");
  if (mozapp) {
    
    return;
  }
  let actor = this._actorByBrowser.get(frame);
  if (actor) {
    this._handleActorClose(actor, frame);
  }
};

exports.B2GTabList = B2GTabList;
