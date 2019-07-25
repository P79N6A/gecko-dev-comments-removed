





































"use strict";

const Cu = Components.utils;


const TILT_NOTIFICATIONS = {

  
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
      this.destroy(id, true);
      return;
    }

    
    this.visualizers[id] = new TiltVisualizer({
      chromeWindow: this.chromeWindow,
      contentWindow: this.chromeWindow.gBrowser.selectedBrowser.contentWindow,
      parentNode: this.chromeWindow.gBrowser.selectedBrowser.parentNode,
      requestAnimationFrame: this.chromeWindow.mozRequestAnimationFrame,
      notifications: this.NOTIFICATIONS
    });

    
    if (!this.visualizers[id].isInitialized()) {
      this.destroy(id);
      return;
    }

    Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.INITIALIZING, null);
  },

  







  destroy: function T_destroy(aId, aAnimateFlag)
  {
    
    if (!this.visualizers[aId]) {
      return;
    }

    if (!this.isDestroying) {
      this.isDestroying = true;

      let finalize = function T_finalize(aId) {
        this.visualizers[aId].removeOverlay();
        this.visualizers[aId].cleanup();
        this.visualizers[aId] = null;

        this.isDestroying = false;
        this.chromeWindow.gBrowser.selectedBrowser.focus();
        Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.DESTROYED, null);
      };

      if (!aAnimateFlag) {
        finalize.call(this, aId);
        return;
      }

      let controller = this.visualizers[aId].controller;
      let presenter = this.visualizers[aId].presenter;

      let content = presenter.contentWindow;
      let pageXOffset = content.pageXOffset * presenter.transforms.zoom;
      let pageYOffset = content.pageYOffset * presenter.transforms.zoom;

      Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.DESTROYING, null);
      TiltUtils.setDocumentZoom(this.chromeWindow, presenter.transforms.zoom);

      controller.removeEventListeners();
      controller.arcball.reset([-pageXOffset, -pageYOffset]);
      presenter.executeDestruction(finalize.bind(this, aId));
    }
  },

  



  _whenInitializing: function T__whenInitializing()
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
    if (this.currentInstance) {
      Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.SHOWN, null);
    } else {
      Services.obs.notifyObservers(null, TILT_NOTIFICATIONS.HIDDEN, null);
    }
  },

  






  update: function T_update(aNode) {
    if (this.currentInstance) {
      this.currentInstance.presenter.highlightNode(aNode, "moveIntoView");
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
      this._whenInitializing.bind(this), TILT_NOTIFICATIONS.INITIALIZING, false);
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
      if (this.inspector && this.highlighter && this.currentInstance) {
        this.inspector.stopInspecting();
        this.inspector.inspectToolbutton.disabled = true;
        this.highlighter.hide();
      }
    }.bind(this);

    let onClosed = function() {
      if (this.inspector && this.highlighter) {
        this.inspector.inspectToolbutton.disabled = false;
        this.highlighter.show();
      }
    }.bind(this);

    Services.obs.addObserver(onOpened,
      this.chromeWindow.InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    Services.obs.addObserver(onClosed,
      this.chromeWindow.InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
    Services.obs.addObserver(onOpened,
      TILT_NOTIFICATIONS.INITIALIZING, false);
    Services.obs.addObserver(onClosed,
      TILT_NOTIFICATIONS.DESTROYED, false);


    this._setupFinished = true;
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

  


  get inspector()
  {
    return this.chromeWindow.InspectorUI;
  },

  


  get highlighter()
  {
    return this.inspector.highlighter;
  },

  


  get tiltButton()
  {
    return this.chromeWindow.document.getElementById(
      "inspector-3D-button");
  }
};
