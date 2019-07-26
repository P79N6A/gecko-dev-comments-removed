





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const CONSOLEAPI_CLASS_ID = "{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
    "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
    "resource:///modules/devtools/Target.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
    "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
    "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/promise/core.js");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

this.EXPORTED_SYMBOLS = ["HUDService"];




function HUD_SERVICE()
{
  this.hudReferences = {};
}

HUD_SERVICE.prototype =
{
  



  hudReferences: null,

  




  get consoleUI() {
    return HeadsUpDisplayUICommands;
  },

  




  currentContext: function HS_currentContext() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  











  openWebConsole: function HS_openWebConsole(aTarget, aIframe)
  {
    let hud = new WebConsole(aTarget, aIframe);
    this.hudReferences[hud.hudId] = hud;
    return hud.init();
  },

  





  getHudByWindow: function HS_getHudByWindow(aContentWindow)
  {
    for each (let hud in this.hudReferences) {
      let target = hud.target;
      if (target && target.tab && target.window === aContentWindow) {
        return hud;
      }
    }
    return null;
  },

  






  getHudIdByWindow: function HS_getHudIdByWindow(aContentWindow)
  {
    let hud = this.getHudByWindow(aContentWindow);
    return hud ? hud.hudId : null;
  },

  





  getHudReferenceById: function HS_getHudReferenceById(aId)
  {
    return aId in this.hudReferences ? this.hudReferences[aId] : null;
  },

  






  lastFinishedRequestCallback: null,
};















function WebConsole(aTarget, aIframe)
{
  this.iframe = aIframe;
  this.iframe.className = "web-console-frame";
  this.chromeDocument = this.iframe.ownerDocument;
  this.chromeWindow = this.chromeDocument.defaultView;
  this.hudId = "hud_" + Date.now();
  this.target = aTarget;
}

WebConsole.prototype = {
  chromeWindow: null,
  chromeDocument: null,
  hudId: null,
  target: null,
  iframe: null,
  _destroyer: null,

  _browserWindow: null,

  get browserWindow()
  {
    if (!this._browserWindow) {
      let window = this.chromeWindow.top;
      let element = window.document.documentElement;
      if (element.getAttribute("windowtype") != "navigator:browser") {
        window = HUDService.currentContext();
      }
      this._browserWindow = window;
    }
    return this._browserWindow;
  },

  





  get lastFinishedRequestCallback() HUDService.lastFinishedRequestCallback,

  



  get mainPopupSet()
  {
    return this.browserWindow.document.getElementById("mainPopupSet");
  },

  



  get outputNode()
  {
    return this.ui ? this.ui.outputNode : null;
  },

  get gViewSourceUtils() this.browserWindow.gViewSourceUtils,

  





  init: function WC_init()
  {
    let deferred = Promise.defer();

    let onIframeLoad = function() {
      this.iframe.removeEventListener("load", onIframeLoad, true);
      initUI();
    }.bind(this);

    let initUI = function() {
      this.iframeWindow = this.iframe.contentWindow.wrappedJSObject;
      this.ui = new this.iframeWindow.WebConsoleFrame(this);
      this.ui.init().then(onSuccess, onFailure);
    }.bind(this);

    let onSuccess = function() {
      deferred.resolve(this);
    }.bind(this);

    let onFailure = function(aReason) {
      deferred.reject(aReason);
    };

    let win, doc;
    if ((win = this.iframe.contentWindow) &&
        (doc = win.document) &&
        doc.readyState == "complete") {
      this.iframe.addEventListener("load", onIframeLoad, true);
    }
    else {
      initUI();
    }

    return deferred.promise;
  },

  





  getPanelTitle: function WC_getPanelTitle()
  {
    let url = this.ui ? this.ui.contentLocation : "";
    return l10n.getFormatStr("webConsoleWindowTitleAndURL", [url]);
  },

  




  get jsterm()
  {
    return this.ui ? this.ui.jsterm : null;
  },

  



  _onClearButton: function WC__onClearButton()
  {
    if (this.target.isLocalTab) {
      this.browserWindow.DeveloperToolbar.resetErrorsCount(this.target.tab);
    }
  },

  



  setFilterState: function WC_setFilterState()
  {
    this.ui && this.ui.setFilterState.apply(this.ui, arguments);
  },

  





  openLink: function WC_openLink(aLink)
  {
    this.browserWindow.openUILinkIn(aLink, "tab");
  },

  







  viewSource: function WC_viewSource(aSourceURL, aSourceLine)
  {
    this.gViewSourceUtils.viewSource(aSourceURL, null,
                                     this.iframeWindow.document, aSourceLine);
  },

  











  viewSourceInStyleEditor:
  function WC_viewSourceInStyleEditor(aSourceURL, aSourceLine)
  {
    let styleSheets = {};
    if (this.target.isLocalTab) {
      styleSheets = this.target.window.document.styleSheets;
    }
    for each (let style in styleSheets) {
      if (style.href == aSourceURL) {
        gDevTools.showToolbox(this.target, "styleeditor").then(function(toolbox) {
          toolbox.getCurrentPanel().selectStyleSheet(style, aSourceLine);
        });
        return;
      }
    }
    
    this.viewSource(aSourceURL, aSourceLine);
  },

  






  destroy: function WC_destroy()
  {
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    delete HUDService.hudReferences[this.hudId];

    let tabWindow = this.target.isLocalTab ? this.target.window : null;

    this._destroyer = Promise.defer();

    let popupset = this.mainPopupSet;
    let panels = popupset.querySelectorAll("panel[hudId=" + this.hudId + "]");
    for (let panel of panels) {
      panel.hidePopup();
    }

    let onDestroy = function WC_onDestroyUI() {
      try {
        tabWindow && tabWindow.focus();
      }
      catch (ex) {
        
      }

      let id = WebConsoleUtils.supportsString(this.hudId);
      Services.obs.notifyObservers(id, "web-console-destroyed", null);

      this._destroyer.resolve(null);
    }.bind(this);

    if (this.ui) {
      this.ui.destroy().then(onDestroy);
    }
    else {
      onDestroy();
    }

    return this._destroyer.promise;
  },
};





var HeadsUpDisplayUICommands = {
  






  toggleHUD: function UIC_toggleHUD()
  {
    let window = HUDService.currentContext();
    let target = TargetFactory.forTab(window.gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    return toolbox && toolbox.currentToolId == "webconsole" ?
        toolbox.destroy() :
        gDevTools.showToolbox(target, "webconsole");
  },

  






  getOpenHUD: function UIC_getOpenHUD()
  {
    let tab = HUDService.currentContext().gBrowser.selectedTab;
    if (!tab || !TargetFactory.isKnownTab(tab)) {
      return null;
    }
    let target = TargetFactory.forTab(tab);
    let toolbox = gDevTools.getToolbox(target);
    let panel = toolbox ? toolbox.getPanel("webconsole") : null;
    return panel ? panel.hud : null;
  },
};

const HUDService = new HUD_SERVICE();
