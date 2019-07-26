



"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

this.EXPORTED_SYMBOLS = [ "Hosts" ];










this.Hosts = {
  "bottom": BottomHost,
  "side": SidebarHost,
  "window": WindowHost
}




function BottomHost(hostTab) {
  this.hostTab = hostTab;

  new EventEmitter(this);
}

BottomHost.prototype = {
  type: "bottom",

  heightPref: "devtools.toolbox.footer.height",

  


  open: function BH_open() {
    let gBrowser = this.hostTab.ownerDocument.defaultView.gBrowser;
    let ownerDocument = gBrowser.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-horizontal-splitter");

    this.frame = ownerDocument.createElement("iframe");
    this.frame.id = "devtools-toolbox-bottom-iframe";
    this.frame.height = Services.prefs.getIntPref(this.heightPref);

    this._nbox = gBrowser.getNotificationBox(this.hostTab.linkedBrowser);
    this._nbox.appendChild(this._splitter);
    this._nbox.appendChild(this.frame);

    let frameLoad = function() {
      this.frame.removeEventListener("DOMContentLoaded", frameLoad, true);
      this.emit("ready", this.frame);
    }.bind(this);

    this.frame.addEventListener("DOMContentLoaded", frameLoad, true);

    
    this.frame.setAttribute("src", "about:blank");

    focusTab(this.hostTab);
  },

  


  destroy: function BH_destroy() {
    if (this._destroyed) {
      return;
    }
    this._destroyed = true;
    Services.prefs.setIntPref(this.heightPref, this.frame.height);

    this._nbox.removeChild(this._splitter);
    this._nbox.removeChild(this.frame);
  }
}





function SidebarHost(hostTab) {
  this.hostTab = hostTab;

  new EventEmitter(this);
}

SidebarHost.prototype = {
  type: "side",

  widthPref: "devtools.toolbox.sidebar.width",

  


  open: function RH_open() {
    let gBrowser = this.hostTab.ownerDocument.defaultView.gBrowser;
    let ownerDocument = gBrowser.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-side-splitter");

    this.frame = ownerDocument.createElement("iframe");
    this.frame.id = "devtools-toolbox-side-iframe";
    this.frame.width = Services.prefs.getIntPref(this.widthPref);

    this._sidebar = gBrowser.getSidebarContainer(this.hostTab.linkedBrowser);
    this._sidebar.appendChild(this._splitter);
    this._sidebar.appendChild(this.frame);

    let frameLoad = function() {
      this.frame.removeEventListener("DOMContentLoaded", frameLoad, true);
      this.emit("ready", this.frame);
    }.bind(this);

    this.frame.addEventListener("DOMContentLoaded", frameLoad, true);
    this.frame.setAttribute("src", "about:blank");

    focusTab(this.hostTab);
  },

  


  destroy: function RH_destroy() {
    Services.prefs.setIntPref(this.widthPref, this.frame.width);

    this._sidebar.removeChild(this._splitter);
    this._sidebar.removeChild(this.frame);
  }
}




function WindowHost() {
  this._boundUnload = this._boundUnload.bind(this);

  new EventEmitter(this);
}

WindowHost.prototype = {
  type: "window",

  WINDOW_URL: "chrome://browser/content/devtools/framework/toolbox-window.xul",

  


  open: function WH_open() {
    let flags = "chrome,centerscreen,resizable,dialog=no";
    let win = Services.ww.openWindow(null, this.WINDOW_URL, "_blank",
                                     flags, null);

    let frameLoad = function(event) {
      win.removeEventListener("load", frameLoad, true);
      this.frame = win.document.getElementById("toolbox-iframe");
      this.emit("ready", this.frame);
    }.bind(this);

    win.addEventListener("load", frameLoad, true);
    win.addEventListener("unload", this._boundUnload);

    win.focus();

    this._window = win;
  },

  


  _boundUnload: function(event) {
    if (event.target.location != this.WINDOW_URL) {
      return;
    }
    this._window.removeEventListener("unload", this._boundUnload);

    this.emit("window-closed");
  },

  


  destroy: function WH_destroy() {
    this._window.removeEventListener("unload", this._boundUnload);
    this._window.close();
  }
}




function focusTab(tab) {
  let browserWindow = tab.ownerDocument.defaultView;
  browserWindow.focus();
  browserWindow.gBrowser.selectedTab = tab;
}
