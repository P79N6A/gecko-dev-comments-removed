





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const CONSOLEAPI_CLASS_ID = "{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
                                  "resource:///modules/devtools/Target.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

this.EXPORTED_SYMBOLS = ["HUDService"];

function LogFactory(aMessagePrefix)
{
  function log(aMessage) {
    var _msg = aMessagePrefix + " " + aMessage + "\n";
    dump(_msg);
  }
  return log;
}

let log = LogFactory("*** HUDService:");


const HTML_NS = "http://www.w3.org/1999/xhtml";


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";




function HUD_SERVICE()
{
  
  
  this.onWindowUnload = this.onWindowUnload.bind(this);

  


  this.hudReferences = {};
};

HUD_SERVICE.prototype =
{
  




  get consoleUI() {
    return HeadsUpDisplayUICommands;
  },

  



  sequencer: null,

  




  currentContext: function HS_currentContext() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  











  activateHUDForContext: function HS_activateHUDForContext(aTab, aIframe,
                                                           aTarget)
  {
    let hudId = "hud_" + aTab.linkedPanel;
    if (hudId in this.hudReferences) {
      return this.hudReferences[hudId];
    }

    this.wakeup();

    let window = aTab.ownerDocument.defaultView;
    let gBrowser = window.gBrowser;

    window.addEventListener("unload", this.onWindowUnload, false);

    let hud = new WebConsole(aTab, aIframe, aTarget);
    this.hudReferences[hudId] = hud;

    return hud;
  },

  






  deactivateHUDForContext: function HS_deactivateHUDForContext(aTab)
  {
    let hudId = "hud_" + aTab.linkedPanel;
    if (!(hudId in this.hudReferences)) {
      return;
    }

    let hud = this.getHudReferenceById(hudId);
    let document = hud.chromeDocument;

    hud.destroy(function() {
      let id = WebConsoleUtils.supportsString(hudId);
      Services.obs.notifyObservers(id, "web-console-destroyed", null);
    });

    delete this.hudReferences[hudId];

    if (Object.keys(this.hudReferences).length == 0) {
      let autocompletePopup = document.
                              getElementById("webConsole_autocompletePopup");
      if (autocompletePopup) {
        autocompletePopup.parentNode.removeChild(autocompletePopup);
      }

      let window = document.defaultView;

      window.removeEventListener("unload", this.onWindowUnload, false);

      let gBrowser = window.gBrowser;
      let tabContainer = gBrowser.tabContainer;

      this.suspend();
    }

    let contentWindow = aTab.linkedBrowser.contentWindow;
    contentWindow.focus();
  },

  




  sequenceId: function HS_sequencerId()
  {
    if (!this.sequencer) {
      this.sequencer = this.createSequencer(-1);
    }
    return this.sequencer.next();
  },

  





  wakeup: function HS_wakeup()
  {
    if (Object.keys(this.hudReferences).length > 0) {
      return;
    }

    WebConsoleObserver.init();
  },

  





  suspend: function HS_suspend()
  {
    delete this.lastFinishedRequestCallback;

    WebConsoleObserver.uninit();
  },

  




  shutdown: function HS_shutdown()
  {
    for (let hud of this.hudReferences) {
      this.deactivateHUDForContext(hud.tab);
    }
  },

  





  getHudByWindow: function HS_getHudByWindow(aContentWindow)
  {
    let hudId = this.getHudIdByWindow(aContentWindow);
    return hudId ? this.hudReferences[hudId] : null;
  },

  






  getHudIdByWindow: function HS_getHudIdByWindow(aContentWindow)
  {
    let window = this.currentContext();
    let index =
      window.gBrowser.getBrowserIndexForDocument(aContentWindow.document);
    if (index == -1) {
      return null;
    }

    let tab = window.gBrowser.tabs[index];
    let hudId = "hud_" + tab.linkedPanel;
    return hudId in this.hudReferences ? hudId : null;
  },

  





  getHudReferenceById: function HS_getHudReferenceById(aId)
  {
    return aId in this.hudReferences ? this.hudReferences[aId] : null;
  },

  






  lastFinishedRequestCallback: null,

  





  createSequencer: function HS_createSequencer(aInt)
  {
    function sequencer(aInt)
    {
      while(1) {
        aInt++;
        yield aInt;
      }
    }
    return sequencer(aInt);
  },

  







  onWindowUnload: function HS_onWindowUnload(aEvent)
  {
    let window = aEvent.target.defaultView;

    window.removeEventListener("unload", this.onWindowUnload, false);

    let gBrowser = window.gBrowser;
    let tabContainer = gBrowser.tabContainer;

    let tab = tabContainer.firstChild;
    while (tab != null) {
      this.deactivateHUDForContext(tab);
      tab = tab.nextSibling;
    }
  },
};
















function WebConsole(aTab, aIframe, aTarget)
{
  this.tab = aTab;
  if (this.tab == null) {
    throw new Error('Missing tab');
  }

  this.iframe = aIframe;
  if (this.iframe == null) {
    console.trace();
    throw new Error('Missing iframe');
  }

  this.chromeDocument = this.tab.ownerDocument;
  this.chromeWindow = this.chromeDocument.defaultView;
  this.hudId = "hud_" + this.tab.linkedPanel;

  this.target = aTarget;

  this._onIframeLoad = this._onIframeLoad.bind(this);

  this.iframe.className = "web-console-frame";
  this.iframe.addEventListener("load", this._onIframeLoad, true);

  this.positionConsole();
}

WebConsole.prototype = {
  



  tab: null,

  chromeWindow: null,
  chromeDocument: null,

  





  get lastFinishedRequestCallback() HUDService.lastFinishedRequestCallback,

  



  get mainPopupSet()
  {
    return this.chromeDocument.getElementById("mainPopupSet");
  },

  



  get outputNode()
  {
    return this.ui ? this.ui.outputNode : null;
  },

  get gViewSourceUtils() this.chromeWindow.gViewSourceUtils,

  



  _onIframeLoad: function WC__onIframeLoad()
  {
    this.iframe.removeEventListener("load", this._onIframeLoad, true);

    this.iframeWindow = this.iframe.contentWindow.wrappedJSObject;
    this.ui = new this.iframeWindow.WebConsoleFrame(this);
  },

  





  getPanelTitle: function WC_getPanelTitle()
  {
    let url = this.ui ? this.ui.contentLocation : "";
    return l10n.getFormatStr("webConsoleWindowTitleAndURL", [url]);
  },

  consoleWindowUnregisterOnHide: true,

  


  positionConsole: function WC_positionConsole()
  {
    let lastIndex = -1;

    if (this.outputNode && this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    this._beforePositionConsole(lastIndex);
  },

  






  _beforePositionConsole:
  function WC__beforePositionConsole(aLastIndex)
  {
    if (!this.ui) {
      return;
    }

    let onLoad = function() {
      this.iframe.removeEventListener("load", onLoad, true);
      this.iframeWindow = this.iframe.contentWindow.wrappedJSObject;
      this.ui.positionConsole(this.iframeWindow);

      if (aLastIndex > -1 && aLastIndex < this.outputNode.getRowCount()) {
        this.outputNode.ensureIndexIsVisible(aLastIndex);
      }
    }.bind(this);

    this.iframe.addEventListener("load", onLoad, true);
  },

  




  get jsterm()
  {
    return this.ui ? this.ui.jsterm : null;
  },

  



  _onClearButton: function WC__onClearButton()
  {
    this.chromeWindow.DeveloperToolbar.resetErrorsCount(this.tab);
  },

  



  setFilterState: function WC_setFilterState()
  {
    this.ui && this.ui.setFilterState.apply(this.ui, arguments);
  },

  





  openLink: function WC_openLink(aLink)
  {
    this.chromeWindow.openUILinkIn(aLink, "tab");
  },

  







  viewSource: function WC_viewSource(aSourceURL, aSourceLine)
  {
    this.gViewSourceUtils.viewSource(aSourceURL, null,
                                     this.iframeWindow.document, aSourceLine);
  },

  











  viewSourceInStyleEditor:
  function WC_viewSourceInStyleEditor(aSourceURL, aSourceLine)
  {
    let styleSheets = this.tab.linkedBrowser.contentWindow.document.styleSheets;
    for each (let style in styleSheets) {
      if (style.href == aSourceURL) {
        let target = TargetFactory.forTab(this.tab);
        let gDevTools = this.chromeWindow.gDevTools;
        let toolbox = gDevTools.getToolboxForTarget(target);
        toolbox.once("styleeditor-selected",
          function _onStyleEditorReady(aEvent, aPanel) {
            aPanel.selectStyleSheet(style, aSourceLine);
          });
        toolbox.selectTool("styleeditor");
        return;
      }
    }
    
    this.viewSource(aSourceURL, aSourceLine);
  },

  







  destroy: function WC_destroy(aOnDestroy)
  {
    
    
    this.consoleWindowUnregisterOnHide = false;

    let popupset = this.mainPopupSet;
    let panels = popupset.querySelectorAll("panel[hudId=" + this.hudId + "]");
    for (let panel of panels) {
      panel.hidePopup();
    }

    let onDestroy = function WC_onDestroyUI() {
      
      
      if (this.consolePanel && this.consolePanel.parentNode) {
        this.consolePanel.hidePopup();
        this.consolePanel.parentNode.removeChild(this.consolePanel);
        this.consolePanel = null;
      }

      if (this.iframe.parentNode) {
        this.iframe.parentNode.removeChild(this.iframe);
      }

      aOnDestroy && aOnDestroy();
    }.bind(this);

    if (this.ui) {
      this.ui.destroy(onDestroy);
    }
    else {
      onDestroy();
    }
  },
};





var HeadsUpDisplayUICommands = {
  toggleHUD: function UIC_toggleHUD(aOptions)
  {
    var window = HUDService.currentContext();
    let target = TargetFactory.forTab(window.gBrowser.selectedTab);
    gDevTools.toggleToolboxForTarget(target, "webconsole");
  },

  toggleRemoteHUD: function UIC_toggleRemoteHUD()
  {
    if (this.getOpenHUD()) {
      this.toggleHUD();
      return;
    }

    let host = Services.prefs.getCharPref("devtools.debugger.remote-host");
    let port = Services.prefs.getIntPref("devtools.debugger.remote-port");

    let check = { value: false };
    let input = { value: host + ":" + port };

    let result = Services.prompt.prompt(null,
      l10n.getStr("remoteWebConsolePromptTitle"),
      l10n.getStr("remoteWebConsolePromptMessage"),
      input, null, check);

    if (!result) {
      return;
    }

    let parts = input.value.split(":");
    if (parts.length != 2) {
      return;
    }

    [host, port] = parts;
    if (!host.length || !port.length) {
      return;
    }

    Services.prefs.setCharPref("devtools.debugger.remote-host", host);
    Services.prefs.setIntPref("devtools.debugger.remote-port", port);

    this.toggleHUD({
      host: host,
      port: port,
    });
  },

  





  getOpenHUD: function UIC_getOpenHUD() {
    let chromeWindow = HUDService.currentContext();
    let hudId = "hud_" + chromeWindow.gBrowser.selectedTab.linkedPanel;
    return hudId in HUDService.hudReferences ? hudId : null;
  },
};





var WebConsoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  init: function WCO_init()
  {
    Services.obs.addObserver(this, "quit-application-granted", false);
  },

  observe: function WCO_observe(aSubject, aTopic)
  {
    if (aTopic == "quit-application-granted") {
      HUDService.shutdown();
    }
  },

  uninit: function WCO_uninit()
  {
    Services.obs.removeObserver(this, "quit-application-granted");
  },
};

const HUDService = new HUD_SERVICE();

