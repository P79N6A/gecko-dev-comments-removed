



"use strict";

const { Ci } = require("chrome");
const Services = require("Services");
const { DebuggerServer } = require("../main");
const { getChildDocShells, TabActor } = require("./webbrowser");
const makeDebugger = require("./utils/make-debugger");





















function ChromeActor(aConnection) {
  TabActor.call(this, aConnection);

  
  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: dbg => dbg.findAllGlobals(),
    shouldAddNewGlobalAsDebuggee: () => true
  });

  
  this.listenForNewDocShells = true;

  
  let window = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);

  
  
  if (!window) {
    window = Services.wm.getMostRecentWindow(null);
  }
  
  let docShell = window ? window.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDocShell)
                        : null;
  Object.defineProperty(this, "docShell", {
    value: docShell,
    configurable: true
  });
}
exports.ChromeActor = ChromeActor;

ChromeActor.prototype = Object.create(TabActor.prototype);

ChromeActor.prototype.constructor = ChromeActor;

ChromeActor.prototype.isRootActor = true;





Object.defineProperty(ChromeActor.prototype, "docShells", {
  get: function () {
    
    let docShells = [];
    let e = Services.ww.getWindowEnumerator();
    while (e.hasMoreElements()) {
      let window = e.getNext();
      let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell);
      docShells = docShells.concat(getChildDocShells(docShell));
    }

    return docShells;
  }
});

ChromeActor.prototype.observe = function(aSubject, aTopic, aData) {
  TabActor.prototype.observe.call(this, aSubject, aTopic, aData);
  if (!this.attached) {
    return;
  }
  if (aTopic == "chrome-webnavigation-create") {
    aSubject.QueryInterface(Ci.nsIDocShell);
    this._onDocShellCreated(aSubject);
  } else if (aTopic == "chrome-webnavigation-destroy") {
    this._onDocShellDestroy(aSubject);
  }
}

ChromeActor.prototype._attach = function() {
  if (this.attached) {
    return false;
  }

  TabActor.prototype._attach.call(this);

  
  Services.obs.addObserver(this, "chrome-webnavigation-create", false);
  Services.obs.addObserver(this, "chrome-webnavigation-destroy", false);

  
  let docShells = [];
  let e = Services.ww.getWindowEnumerator();
  while (e.hasMoreElements()) {
    let window = e.getNext();
    let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShell);
    if (docShell == this.docShell) {
      continue;
    }
    this._progressListener.watch(docShell);
  }
};

ChromeActor.prototype._detach = function() {
  if (!this.attached) {
    return false;
  }

  Services.obs.removeObserver(this, "chrome-webnavigation-create");
  Services.obs.removeObserver(this, "chrome-webnavigation-destroy");

  
  let docShells = [];
  let e = Services.ww.getWindowEnumerator();
  while (e.hasMoreElements()) {
    let window = e.getNext();
    let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShell);
    if (docShell == this.docShell) {
      continue;
    }
    this._progressListener.unwatch(docShell);
  }

  TabActor.prototype._detach.call(this);
};






ChromeActor.prototype.preNest = function() {
  
  let e = Services.wm.getEnumerator(null);
  while (e.hasMoreElements()) {
    let win = e.getNext();
    let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.suppressEventHandling(true);
    windowUtils.suspendTimeouts();
  }
}




ChromeActor.prototype.postNest = function(aNestData) {
  
  let e = Services.wm.getEnumerator(null);
  while (e.hasMoreElements()) {
    let win = e.getNext();
    let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.resumeTimeouts();
    windowUtils.suppressEventHandling(false);
  }
}
