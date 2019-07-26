




"use strict";

const Cu = Components.utils;


const TILT_NOTIFICATIONS = {
  
  STARTUP: "tilt-startup",

  
  INITIALIZING: "tilt-initializing",

  
  
  INITIALIZED: "tilt-initialized",

  
  DESTROYING: "tilt-destroying",

  
  
  BEFORE_DESTROYED: "tilt-before-destroyed",

  
  DESTROYED: "tilt-destroyed",

  
  SHOWN: "tilt-shown",

  
  HIDDEN: "tilt-hidden",

  
  HIGHLIGHTING: "tilt-highlighting",

  
  UNHIGHLIGHTING: "tilt-unhighlighting",

  
  NODE_REMOVED: "tilt-node-removed"
};

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/TiltGL.jsm");
Cu.import("resource:///modules/devtools/TiltUtils.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource:///modules/devtools/TiltVisualizer.jsm");

this.EXPORTED_SYMBOLS = ["TiltManager"];

this.TiltManager = {
  _instances: new WeakMap(),
  getTiltForBrowser: function(aChromeWindow)
  {
    if (this._instances.has(aChromeWindow)) {
      return this._instances.get(aChromeWindow);
    } else {
      let tilt = new Tilt(aChromeWindow);
      this._instances.set(aChromeWindow, tilt);
      return tilt;
    }
  },
}







this.Tilt = function Tilt(aWindow)
{
  


  this.chromeWindow = aWindow;

  


  this.visualizers = {};

  


  this.NOTIFICATIONS = TILT_NOTIFICATIONS;

  EventEmitter.decorate(this);

  this.setup();
}

Tilt.prototype = {

  


  toggle: function T_toggle()
  {
    let contentWindow = this.chromeWindow.gBrowser.selectedBrowser.contentWindow;
    let id = this.currentWindowId;
    let self = this;

    contentWindow.addEventListener("beforeunload", function onUnload() {
      contentWindow.removeEventListener("beforeunload", onUnload, false);
      self.destroy(id, true);
    }, false);

    
    if (this.visualizers[id]) {
      this.destroy(id, true);
      return;
    }

    
    this.visualizers[id] = new TiltVisualizer({
      chromeWindow: this.chromeWindow,
      contentWindow: contentWindow,
      parentNode: this.chromeWindow.gBrowser.selectedBrowser.parentNode,
      notifications: this.NOTIFICATIONS,
      tab: this.chromeWindow.gBrowser.selectedTab
    });

    Services.obs.notifyObservers(contentWindow, TILT_NOTIFICATIONS.STARTUP, null);
    this.visualizers[id].init();

    
    if (!this.visualizers[id].isInitialized()) {
      this.destroy(id);
      this.failureCallback && this.failureCallback();
      return;
    }

    this.lastInstanceId = id;
    this.emit("change", this.chromeWindow.gBrowser.selectedTab);
    Services.obs.notifyObservers(contentWindow, TILT_NOTIFICATIONS.INITIALIZING, null);
  },

  







  destroy: function T_destroy(aId, aAnimateFlag)
  {
    
    if (!this.visualizers[aId] || this._isDestroying) {
      return;
    }
    this._isDestroying = true;

    let controller = this.visualizers[aId].controller;
    let presenter = this.visualizers[aId].presenter;

    let content = presenter.contentWindow;
    let pageXOffset = content.pageXOffset * presenter.transforms.zoom;
    let pageYOffset = content.pageYOffset * presenter.transforms.zoom;
    TiltUtils.setDocumentZoom(this.chromeWindow, presenter.transforms.zoom);

    
    if (!aAnimateFlag) {
      this._finish(aId);
      return;
    }

    
    Services.obs.notifyObservers(content, TILT_NOTIFICATIONS.DESTROYING, null);

    controller.removeEventListeners();
    controller.arcball.reset([-pageXOffset, -pageYOffset]);
    presenter.executeDestruction(this._finish.bind(this, aId));
  },

  





  _finish: function T__finish(aId)
  {
    let contentWindow = this.visualizers[aId].presenter.contentWindow;
    this.visualizers[aId].removeOverlay();
    this.visualizers[aId].cleanup();
    this.visualizers[aId] = null;

    this._isDestroying = false;
    this.chromeWindow.gBrowser.selectedBrowser.focus();
    this.emit("change", this.chromeWindow.gBrowser.selectedTab);
    Services.obs.notifyObservers(contentWindow, TILT_NOTIFICATIONS.DESTROYED, null);
  },

  


  _onTabSelect: function T__onTabSelect()
  {
    if (this.visualizers[this.lastInstanceId]) {
      let contentWindow = this.visualizers[this.lastInstanceId].presenter.contentWindow;
      Services.obs.notifyObservers(contentWindow, TILT_NOTIFICATIONS.HIDDEN, null);
    }

    if (this.currentInstance) {
      let contentWindow = this.currentInstance.presenter.contentWindow;
      Services.obs.notifyObservers(contentWindow, TILT_NOTIFICATIONS.SHOWN, null);
    }

    this.lastInstanceId = this.currentWindowId;
  },

  


  setup: function T_setup()
  {
    
    TiltVisualizer.Prefs.load();

    this.chromeWindow.gBrowser.tabContainer.addEventListener(
      "TabSelect", this._onTabSelect.bind(this), false);
  },

  


  get enabled()
  {
    return (TiltVisualizer.Prefs.enabled &&
           (TiltGL.isWebGLForceEnabled() || TiltGL.isWebGLSupported()));
  },

  


  get currentWindowId()
  {
    return TiltUtils.getWindowId(
      this.chromeWindow.gBrowser.selectedBrowser.contentWindow);
  },

  


  get currentInstance()
  {
    return this.visualizers[this.currentWindowId];
  },
};
