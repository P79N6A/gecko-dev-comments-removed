



"use strict";

const {Cu} = require("chrome");

let Promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");










exports.Hosts = {
  "bottom": BottomHost,
  "side": SidebarHost,
  "window": WindowHost
}




function BottomHost(hostTab) {
  this.hostTab = hostTab;

  EventEmitter.decorate(this);
}

BottomHost.prototype = {
  type: "bottom",

  heightPref: "devtools.toolbox.footer.height",

  


  create: function BH_create() {
    let deferred = Promise.defer();

    let gBrowser = this.hostTab.ownerDocument.defaultView.gBrowser;
    let ownerDocument = gBrowser.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-horizontal-splitter");

    this.frame = ownerDocument.createElement("iframe");
    this.frame.className = "devtools-toolbox-bottom-iframe";
    this.frame.height = Services.prefs.getIntPref(this.heightPref);

    this._nbox = gBrowser.getNotificationBox(this.hostTab.linkedBrowser);
    this._nbox.appendChild(this._splitter);
    this._nbox.appendChild(this.frame);

    let frameLoad = function() {
      this.frame.removeEventListener("DOMContentLoaded", frameLoad, true);
      this.emit("ready", this.frame);

      deferred.resolve(this.frame);
    }.bind(this);

    this.frame.tooltip = "aHTMLTooltip";
    this.frame.addEventListener("DOMContentLoaded", frameLoad, true);

    
    this.frame.setAttribute("src", "about:blank");

    focusTab(this.hostTab);

    return deferred.promise;
  },

  


  raise: function BH_raise() {
    focusTab(this.hostTab);
  },

  


  setTitle: function BH_setTitle(title) {
    
  },

  


  destroy: function BH_destroy() {
    if (!this._destroyed) {
      this._destroyed = true;

      Services.prefs.setIntPref(this.heightPref, this.frame.height);
      this._nbox.removeChild(this._splitter);
      this._nbox.removeChild(this.frame);
    }

    return Promise.resolve(null);
  }
}





function SidebarHost(hostTab) {
  this.hostTab = hostTab;

  EventEmitter.decorate(this);
}

SidebarHost.prototype = {
  type: "side",

  widthPref: "devtools.toolbox.sidebar.width",

  


  create: function SH_create() {
    let deferred = Promise.defer();

    let gBrowser = this.hostTab.ownerDocument.defaultView.gBrowser;
    let ownerDocument = gBrowser.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-side-splitter");

    this.frame = ownerDocument.createElement("iframe");
    this.frame.className = "devtools-toolbox-side-iframe";
    this.frame.width = Services.prefs.getIntPref(this.widthPref);

    this._sidebar = gBrowser.getSidebarContainer(this.hostTab.linkedBrowser);
    this._sidebar.appendChild(this._splitter);
    this._sidebar.appendChild(this.frame);

    let frameLoad = function() {
      this.frame.removeEventListener("DOMContentLoaded", frameLoad, true);
      this.emit("ready", this.frame);

      deferred.resolve(this.frame);
    }.bind(this);

    this.frame.addEventListener("DOMContentLoaded", frameLoad, true);
    this.frame.tooltip = "aHTMLTooltip";
    this.frame.setAttribute("src", "about:blank");

    focusTab(this.hostTab);

    return deferred.promise;
  },

  


  raise: function SH_raise() {
    focusTab(this.hostTab);
  },

  


  setTitle: function SH_setTitle(title) {
    
  },

  


  destroy: function SH_destroy() {
    if (!this._destroyed) {
      this._destroyed = true;

      Services.prefs.setIntPref(this.widthPref, this.frame.width);
      this._sidebar.removeChild(this._splitter);
      this._sidebar.removeChild(this.frame);
    }

    return Promise.resolve(null);
  }
}




function WindowHost() {
  this._boundUnload = this._boundUnload.bind(this);

  EventEmitter.decorate(this);
}

WindowHost.prototype = {
  type: "window",

  WINDOW_URL: "chrome://browser/content/devtools/framework/toolbox-window.xul",

  


  create: function WH_create() {
    let deferred = Promise.defer();

    let flags = "chrome,centerscreen,resizable,dialog=no";
    let win = Services.ww.openWindow(null, this.WINDOW_URL, "_blank",
                                     flags, null);

    let frameLoad = function(event) {
      win.removeEventListener("load", frameLoad, true);
      this.frame = win.document.getElementById("toolbox-iframe");
      this.emit("ready", this.frame);

      deferred.resolve(this.frame);
    }.bind(this);

    win.addEventListener("load", frameLoad, true);
    win.addEventListener("unload", this._boundUnload);

    win.focus();

    this._window = win;

    return deferred.promise;
  },

  


  _boundUnload: function(event) {
    if (event.target.location != this.WINDOW_URL) {
      return;
    }
    this._window.removeEventListener("unload", this._boundUnload);

    this.emit("window-closed");
  },

  


  raise: function RH_raise() {
    this._window.focus();
  },

  


  setTitle: function WH_setTitle(title) {
    this._window.document.title = title;
  },

  


  destroy: function WH_destroy() {
    if (!this._destroyed) {
      this._destroyed = true;

      this._window.removeEventListener("unload", this._boundUnload);
      this._window.close();
    }

    return Promise.resolve(null);
  }
}




function focusTab(tab) {
  let browserWindow = tab.ownerDocument.defaultView;
  browserWindow.focus();
  browserWindow.gBrowser.selectedTab = tab;
}
