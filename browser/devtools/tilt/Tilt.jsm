







































"use strict";

const Cu = Components.utils;


const TILT_NOTIFICATIONS = {

  
  INITIALIZED: "tilt-initialized",

  
  DESTROYED: "tilt-destroyed",

  
  SHOWN: "tilt-shown",

  
  HIDDEN: "tilt-hidden"
};

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/TiltGL.jsm");
Cu.import("resource:///modules/devtools/TiltUtils.jsm");
Cu.import("resource:///modules/devtools/TiltVisualizer.jsm");

let EXPORTED_SYMBOLS = ["Tilt"];







function Tilt(aWindow)
{
  


  this.chromeWindow = aWindow;

  


  this.visualizers = {};

  


  this.NOTIFICATIONS = TILT_NOTIFICATIONS;
}

Tilt.prototype = {

  


  initialize: function T_initialize()
  {
    let id = this.currentWindowId;

    
    if (this.visualizers[id]) {
      this.destroy(id);
      return;
    }

    
    this.visualizers[id] = new TiltVisualizer({
      parentNode: this.chromeWindow.gBrowser.selectedBrowser.parentNode,
      contentWindow: this.chromeWindow.gBrowser.selectedBrowser.contentWindow,
      requestAnimationFrame: this.chromeWindow.mozRequestAnimationFrame,
      inspectorUI: this.chromeWindow.InspectorUI
    });

    
    if (!this.visualizers[id].isInitialized()) {
      this.destroy(id);
      return;
    }

    Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.INITIALIZED, null);
  },

  





  destroy: function T_destroy(aId)
  {
    
    if (!this.visualizers[aId]) {
      return;
    }

    this.visualizers[aId].removeOverlay();
    this.visualizers[aId].cleanup();
    this.visualizers[aId] = null;

    Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.DESTROYED, null);
  },

  



  _whenInitialized: function T__whenInitialized()
  {
    this._whenShown();
  },

  



  _whenDestroyed: function T__whenDestroyed()
  {
    this._whenHidden();
  },

  



  _whenShown: function T__whenShown()
  {
    this.tiltButton.checked = true;
  },

  



  _whenHidden: function T__whenHidden()
  {
    this.tiltButton.checked = false;
  },

  


  _onTabSelect: function T__onTabSelect()
  {
    if (this.visualizers[this.currentWindowId]) {
      Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.SHOWN, null);
    } else {
      Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.HIDDEN, null);
    }
  },

  






  update: function T_update(aNode) {
    let id = this.currentWindowId;

    if (this.visualizers[id]) {
      this.visualizers[id].presenter.highlightNode(aNode);
    }
  },

  



  setup: function T_setup()
  {
    if (this._setupFinished) {
      return;
    }

    
    TiltVisualizer.Prefs.load();

    
    this.tiltButton.hidden = !this.enabled;

    
    Services.obs.addObserver(
      this._whenInitialized.bind(this), TILT_NOTIFICATIONS.INITIALIZED, false);
    Services.obs.addObserver(
      this._whenDestroyed.bind(this), TILT_NOTIFICATIONS.DESTROYED, false);
    Services.obs.addObserver(
      this._whenShown.bind(this), TILT_NOTIFICATIONS.SHOWN, false);
    Services.obs.addObserver(
      this._whenHidden.bind(this), TILT_NOTIFICATIONS.HIDDEN, false);
    Services.obs.addObserver(function(aSubject, aTopic, aWinId) {
      this.destroy(aWinId); }.bind(this),
      this.chromeWindow.InspectorUI.INSPECTOR_NOTIFICATIONS.DESTROYED, false);

    this.chromeWindow.gBrowser.tabContainer.addEventListener("TabSelect",
      this._onTabSelect.bind(this), false);


    
    let onOpened = function() {
      if (this.visualizers[this.currentWindowId]) {
        this.chromeWindow.InspectorUI.stopInspecting();
        this.inspectButton.disabled = true;
        this.highlighterContainer.style.display = "none";
      }
    }.bind(this);

    let onClosed = function() {
      this.inspectButton.disabled = false;
      this.highlighterContainer.style.display = "";
    }.bind(this);

    Services.obs.addObserver(onOpened,
      this.chromeWindow.InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    Services.obs.addObserver(onClosed,
      this.chromeWindow.InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
    Services.obs.addObserver(onOpened,
      TILT_NOTIFICATIONS.INITIALIZED, false);
    Services.obs.addObserver(onClosed,
      TILT_NOTIFICATIONS.DESTROYED, false);


    this._setupFinished = true;
  },

  


  get enabled()
  {
    return (TiltVisualizer.Prefs.enabled &&
           (TiltVisualizer.Prefs.forceEnabled || TiltGL.isWebGLSupported()));
  },

  


  get currentWindowId()
  {
    let gBrowser = this.chromeWindow.gBrowser;
    return TiltUtils.getWindowId(gBrowser.selectedBrowser.contentWindow);
  },

  


  get tiltButton()
  {
    return this.chromeWindow.document.getElementById(
      "inspector-3D-button");
  },

  



  get inspectButton()
  {
    return this.chromeWindow.document.getElementById(
      "inspector-inspect-toolbutton");
  },

  



  get highlighterContainer()
  {
    return this.chromeWindow.document.getElementById(
      "highlighter-container");
  }
};
