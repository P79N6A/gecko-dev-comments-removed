





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const CONSOLEAPI_CLASS_ID = "{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource:///modules/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "l10n", function() {
  return WebConsoleUtils.l10n;
});


var EXPORTED_SYMBOLS = ["HUDService"];

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



const ANIMATE_OUT = 0;
const ANIMATE_IN = 1;


const MINIMUM_CONSOLE_HEIGHT = 150;



const MINIMUM_PAGE_HEIGHT = 50;


const DEFAULT_CONSOLE_HEIGHT = 0.33;


const CONTENT_SCRIPT_URL = "chrome://browser/content/devtools/HUDService-content.js";


const UI_IFRAME_URL = "chrome://browser/content/devtools/webconsole.xul";




function HUD_SERVICE()
{
  
  
  this.onTabClose = this.onTabClose.bind(this);
  this.onTabSelect = this.onTabSelect.bind(this);
  this.onWindowUnload = this.onWindowUnload.bind(this);

  
  this.lastConsoleHeight = Services.prefs.getIntPref("devtools.hud.height");

  


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

  









  activateHUDForContext: function HS_activateHUDForContext(aTab, aAnimated)
  {
    let hudId = "hud_" + aTab.linkedPanel;
    if (hudId in this.hudReferences) {
      return this.hudReferences[hudId];
    }

    this.wakeup();

    let window = aTab.ownerDocument.defaultView;
    let gBrowser = window.gBrowser;

    gBrowser.tabContainer.addEventListener("TabClose", this.onTabClose, false);
    gBrowser.tabContainer.addEventListener("TabSelect", this.onTabSelect, false);
    window.addEventListener("unload", this.onWindowUnload, false);

    let hud = new WebConsole(aTab);
    this.hudReferences[hudId] = hud;

    if (!aAnimated || hud.consolePanel) {
      this.disableAnimation(hudId);
    }

    HeadsUpDisplayUICommands.refreshCommand();

    return hud;
  },

  








  deactivateHUDForContext: function HS_deactivateHUDForContext(aTab, aAnimated)
  {
    let hudId = "hud_" + aTab.linkedPanel;
    if (!(hudId in this.hudReferences)) {
      return;
    }

    if (!aAnimated) {
      this.storeHeight(hudId);
    }

    let hud = this.getHudReferenceById(hudId);
    let document = hud.chromeDocument;

    hud.destroy();

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
      tabContainer.removeEventListener("TabClose", this.onTabClose, false);
      tabContainer.removeEventListener("TabSelect", this.onTabSelect, false);

      this.suspend();
    }

    let contentWindow = aTab.linkedBrowser.contentWindow;
    contentWindow.focus();

    HeadsUpDisplayUICommands.refreshCommand();

    let id = WebConsoleUtils.supportsString(hudId);
    Services.obs.notifyObservers(id, "web-console-destroyed", null);
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
      this.deactivateHUDForContext(hud.tab, false);
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

  





  onTabClose: function HS_onTabClose(aEvent)
  {
    this.deactivateHUDForContext(aEvent.target, false);
  },

  





  onTabSelect: function HS_onTabSelect(aEvent)
  {
    HeadsUpDisplayUICommands.refreshCommand();
  },

  







  onWindowUnload: function HS_onWindowUnload(aEvent)
  {
    let window = aEvent.target.defaultView;

    window.removeEventListener("unload", this.onWindowUnload, false);

    let gBrowser = window.gBrowser;
    let tabContainer = gBrowser.tabContainer;

    tabContainer.removeEventListener("TabClose", this.onTabClose, false);
    tabContainer.removeEventListener("TabSelect", this.onTabSelect, false);

    let tab = tabContainer.firstChild;
    while (tab != null) {
      this.deactivateHUDForContext(tab, false);
      tab = tab.nextSibling;
    }
  },

  









  animate: function HS_animate(aHUDId, aDirection, aCallback)
  {
    let hudBox = this.getHudReferenceById(aHUDId).iframe;
    if (!hudBox.hasAttribute("animated")) {
      if (aCallback) {
        aCallback();
      }
      return;
    }

    switch (aDirection) {
      case ANIMATE_OUT:
        hudBox.style.height = 0;
        break;
      case ANIMATE_IN:
        this.resetHeight(aHUDId);
        break;
    }

    if (aCallback) {
      hudBox.addEventListener("transitionend", aCallback, false);
    }
  },

  






  disableAnimation: function HS_disableAnimation(aHUDId)
  {
    let hudBox = HUDService.hudReferences[aHUDId].iframe;
    if (hudBox.hasAttribute("animated")) {
      hudBox.removeAttribute("animated");
      this.resetHeight(aHUDId);
    }
  },

  




  resetHeight: function HS_resetHeight(aHUDId)
  {
    let HUD = this.hudReferences[aHUDId];
    let innerHeight = HUD.tab.linkedBrowser.clientHeight;
    let chromeWindow = HUD.chromeWindow;
    if (!HUD.consolePanel) {
      let splitterStyle = chromeWindow.getComputedStyle(HUD.splitter, null);
      innerHeight += parseInt(splitterStyle.height) +
                     parseInt(splitterStyle.borderTopWidth) +
                     parseInt(splitterStyle.borderBottomWidth);
    }

    let boxStyle = chromeWindow.getComputedStyle(HUD.iframe, null);
    innerHeight += parseInt(boxStyle.height) +
                   parseInt(boxStyle.borderTopWidth) +
                   parseInt(boxStyle.borderBottomWidth);

    let height = this.lastConsoleHeight > 0 ? this.lastConsoleHeight :
      Math.ceil(innerHeight * DEFAULT_CONSOLE_HEIGHT);

    if ((innerHeight - height) < MINIMUM_PAGE_HEIGHT) {
      height = innerHeight - MINIMUM_PAGE_HEIGHT;
    }

    if (isNaN(height) || height < MINIMUM_CONSOLE_HEIGHT) {
      height = MINIMUM_CONSOLE_HEIGHT;
    }

    HUD.iframe.style.height = height + "px";
  },

  





  storeHeight: function HS_storeHeight(aHUDId)
  {
    let hudBox = this.hudReferences[aHUDId].iframe;
    let window = hudBox.ownerDocument.defaultView;
    let style = window.getComputedStyle(hudBox, null);
    let height = parseInt(style.height);
    height += parseInt(style.borderTopWidth);
    height += parseInt(style.borderBottomWidth);
    this.lastConsoleHeight = height;

    let pref = Services.prefs.getIntPref("devtools.hud.height");
    if (pref > -1) {
      Services.prefs.setIntPref("devtools.hud.height", height);
    }
  },
};












function WebConsole(aTab)
{
  this.tab = aTab;
  this._onIframeLoad = this._onIframeLoad.bind(this);
  this._asyncRequests = {};
  this._init();
}

WebConsole.prototype = {
  



  tab: null,

  





  get lastFinishedRequestCallback() HUDService.lastFinishedRequestCallback,

  






  _asyncRequests: null,

  






  _messageListeners: ["JSTerm:EvalObject", "WebConsole:ConsoleAPI",
    "WebConsole:CachedMessages", "WebConsole:PageError", "JSTerm:EvalResult",
    "JSTerm:AutocompleteProperties", "JSTerm:ClearOutput",
    "JSTerm:InspectObject", "WebConsole:NetworkActivity",
    "WebConsole:FileActivity", "WebConsole:LocationChange",
    "JSTerm:NonNativeConsoleAPI"],

  



  consolePanel: null,

  



  contentLocation: "",

  



  get mainPopupSet()
  {
    return this.chromeDocument.getElementById("mainPopupSet");
  },

  



  get outputNode()
  {
    return this.ui ? this.ui.outputNode : null;
  },

  get gViewSourceUtils() this.chromeWindow.gViewSourceUtils,

  



  _init: function WC__init()
  {
    this.chromeDocument = this.tab.ownerDocument;
    this.chromeWindow = this.chromeDocument.defaultView;
    this.messageManager = this.tab.linkedBrowser.messageManager;
    this.hudId = "hud_" + this.tab.linkedPanel;
    this.notificationBox = this.chromeDocument
                           .getElementById(this.tab.linkedPanel);

    this._initUI();
  },

  



  _initUI: function WC__initUI()
  {
    this.splitter = this.chromeDocument.createElement("splitter");
    this.splitter.setAttribute("class", "web-console-splitter");

    this.iframe = this.chromeDocument.createElement("iframe");
    this.iframe.setAttribute("id", this.hudId);
    this.iframe.setAttribute("class", "web-console-frame");
    this.iframe.setAttribute("animated", "true");
    this.iframe.setAttribute("tooltip", "aHTMLTooltip");
    this.iframe.style.height = 0;
    this.iframe.addEventListener("load", this._onIframeLoad, true);
    this.iframe.setAttribute("src", UI_IFRAME_URL);

    let position = Services.prefs.getCharPref("devtools.webconsole.position");
    this.positionConsole(position);
  },

  



  _onIframeLoad: function WC__onIframeLoad()
  {
    this.iframe.removeEventListener("load", this._onIframeLoad, true);

    let position = Services.prefs.getCharPref("devtools.webconsole.position");

    this.iframeWindow = this.iframe.contentWindow.wrappedJSObject;
    this.ui = new this.iframeWindow.WebConsoleFrame(this, position);
    this._setupMessageManager();
  },

  




  _createOwnWindowPanel: function WC__createOwnWindowPanel()
  {
    if (this.consolePanel) {
      return;
    }

    let width = 0;
    try {
      width = Services.prefs.getIntPref("devtools.webconsole.width");
    }
    catch (ex) {}

    if (width < 1) {
      width = this.iframe.clientWidth || this.chromeWindow.innerWidth;
    }

    let height = this.iframe.clientHeight;

    let top = 0;
    try {
      top = Services.prefs.getIntPref("devtools.webconsole.top");
    }
    catch (ex) {}

    let left = 0;
    try {
      left = Services.prefs.getIntPref("devtools.webconsole.left");
    }
    catch (ex) {}

    let panel = this.chromeDocument.createElementNS(XUL_NS, "panel");

    let config = { id: "console_window_" + this.hudId,
                   label: this.getPanelTitle(),
                   titlebar: "normal",
                   noautohide: "true",
                   norestorefocus: "true",
                   close: "true",
                   flex: "1",
                   hudId: this.hudId,
                   width: width,
                   position: "overlap",
                   top: top,
                   left: left,
                 };

    for (let attr in config) {
      panel.setAttribute(attr, config[attr]);
    }

    panel.classList.add("web-console-panel");

    let onPopupShown = (function HUD_onPopupShown() {
      panel.removeEventListener("popupshown", onPopupShown, false);

      

      let height = panel.clientHeight;

      this.iframe.style.height = "auto";
      this.iframe.flex = 1;

      panel.setAttribute("height", height);
    }).bind(this);

    panel.addEventListener("popupshown", onPopupShown,false);

    let onPopupHidden = (function HUD_onPopupHidden(aEvent) {
      if (aEvent.target != panel) {
        return;
      }

      panel.removeEventListener("popuphidden", onPopupHidden, false);

      let width = 0;
      try {
        width = Services.prefs.getIntPref("devtools.webconsole.width");
      }
      catch (ex) { }

      if (width > 0) {
        Services.prefs.setIntPref("devtools.webconsole.width", panel.clientWidth);
      }

      
      if (this.consoleWindowUnregisterOnHide) {
        HUDService.deactivateHUDForContext(this.tab, false);
      }
    }).bind(this);

    panel.addEventListener("popuphidden", onPopupHidden, false);

    let lastIndex = -1;

    if (this.outputNode && this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }

    this._beforePositionConsole("window", lastIndex);

    panel.appendChild(this.iframe);

    let space = this.chromeDocument.createElement("spacer");
    space.flex = 1;

    let bottomBox = this.chromeDocument.createElement("hbox");

    let resizer = this.chromeDocument.createElement("resizer");
    resizer.setAttribute("dir", "bottomend");
    resizer.setAttribute("element", config.id);

    bottomBox.appendChild(space);
    bottomBox.appendChild(resizer);

    panel.appendChild(bottomBox);

    this.mainPopupSet.appendChild(panel);

    panel.openPopup(null, "overlay", left, top, false, false);

    this.consolePanel = panel;
    this.consoleWindowUnregisterOnHide = true;
  },

  





  getPanelTitle: function WC_getPanelTitle()
  {
    return l10n.getFormatStr("webConsoleWindowTitleAndURL",
                             [this.contentLocation]);
  },

  positions: {
    above: 0, 
    below: 2,
    window: null
  },

  consoleWindowUnregisterOnHide: true,

  





  positionConsole: function WC_positionConsole(aPosition)
  {
    if (!(aPosition in this.positions)) {
      throw new Error("Incorrect argument: " + aPosition +
        ". Cannot position Web Console");
    }

    if (aPosition == "window") {
      this._createOwnWindowPanel();
      return;
    }

    let height = this.iframe.clientHeight;

    
    let nodeIdx = this.positions[aPosition];
    let nBox = this.notificationBox;
    let node = nBox.childNodes[nodeIdx];

    
    if (node == this.iframe) {
      return;
    }

    let lastIndex = -1;

    if (this.outputNode && this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    
    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }

    this._beforePositionConsole(aPosition, lastIndex);

    if (aPosition == "below") {
      nBox.appendChild(this.splitter);
      nBox.appendChild(this.iframe);
    }
    else {
      nBox.insertBefore(this.splitter, node);
      nBox.insertBefore(this.iframe, this.splitter);
    }

    if (this.consolePanel) {
      
      this.consoleWindowUnregisterOnHide = false;
      this.consolePanel.hidePopup();
      this.consolePanel.parentNode.removeChild(this.consolePanel);
      this.consolePanel = null;   
      this.iframe.removeAttribute("flex");
      this.iframe.removeAttribute("height");
      this.iframe.style.height = height + "px";
    }
  },

  








  _beforePositionConsole:
  function WC__beforePositionConsole(aPosition, aLastIndex)
  {
    if (!this.ui) {
      return;
    }

    let onLoad = function() {
      this.iframe.removeEventListener("load", onLoad, true);
      this.iframeWindow = this.iframe.contentWindow.wrappedJSObject;
      this.ui.positionConsole(aPosition, this.iframeWindow);

      if (aLastIndex > -1 && aLastIndex < this.outputNode.getRowCount()) {
        this.outputNode.ensureIndexIsVisible(aLastIndex);
      }

      this._currentUIPosition = aPosition;
      Services.prefs.setCharPref("devtools.webconsole.position", aPosition);
    }.bind(this);

    this.iframe.addEventListener("load", onLoad, true);
  },

  




  get jsterm()
  {
    return this.ui ? this.ui.jsterm : null;
  },

  


  onCloseButton: function WC_onCloseButton()
  {
    HUDService.animate(this.hudId, ANIMATE_OUT, function() {
      HUDService.deactivateHUDForContext(this.tab, true);
    }.bind(this));
  },

  


  onClearButton: function WC_onClearButton()
  {
    this.ui.jsterm.clearOutput(true);
    this.chromeWindow.DeveloperToolbar.resetErrorsCount(this.tab);
  },

  






  _setupMessageManager: function WC__setupMessageManager()
  {
    this.messageManager.loadFrameScript(CONTENT_SCRIPT_URL, true);

    this._messageListeners.forEach(function(aName) {
      this.messageManager.addMessageListener(aName, this.ui);
    }, this);

    let message = {
      features: ["ConsoleAPI", "JSTerm", "PageError", "NetworkMonitor",
                 "LocationChange"],
      cachedMessages: ["ConsoleAPI", "PageError"],
      NetworkMonitor: { monitorFileActivity: true },
      JSTerm: { notifyNonNativeConsoleAPI: true },
      preferences: {
        "NetworkMonitor.saveRequestAndResponseBodies":
          this.ui.saveRequestAndResponseBodies,
      },
    };

    this.sendMessageToContent("WebConsole:Init", message);
  },

  






  _onInitComplete: function WC__onInitComplete()
  {
    let id = WebConsoleUtils.supportsString(this.hudId);
    Services.obs.notifyObservers(id, "web-console-created", null);
  },

  










  _receiveMessageWithCallback:
  function WC__receiveMessageWithCallback(aResponse)
  {
    if (aResponse.id in this._asyncRequests) {
      let request = this._asyncRequests[aResponse.id];
      request.callback(aResponse, request.message);
      delete this._asyncRequests[aResponse.id];
    }
    else {
      Cu.reportError("receiveMessageWithCallback response for stale request " +
                     "ID " + aResponse.id);
    }
  },

  















  sendMessageToContent:
  function WC_sendMessageToContent(aName, aMessage, aCallback)
  {
    aMessage.hudId = this.hudId;
    if (!("id" in aMessage)) {
      aMessage.id = "HUDChrome-" + HUDService.sequenceId();
    }

    if (aCallback) {
      this._asyncRequests[aMessage.id] = {
        name: aName,
        message: aMessage,
        callback: aCallback,
      };
    }
    this.messageManager.sendAsyncMessage(aName, aMessage);
  },

  







  onLocationChange: function WC_onLocationChange(aMessage)
  {
    this.contentLocation = aMessage.location;
    if (this.consolePanel) {
      this.consolePanel.label = this.getPanelTitle();
    }
  },

  



  setFilterState: function WC_setFilterState()
  {
    this.ui && this.ui.setFilterState.apply(this.ui, arguments);
  },

  





  openLink: function WC_openLink(aLink)
  {
    this.chromeWindow.openUILinkIn(aLink, "tab");
  },

  



  destroy: function WC_destroy()
  {
    this.sendMessageToContent("WebConsole:Destroy", {});

    this._messageListeners.forEach(function(aName) {
      this.messageManager.removeMessageListener(aName, this.ui);
    }, this);

    
    
    this.consoleWindowUnregisterOnHide = false;

    let popupset = this.mainPopupSet;
    let panels = popupset.querySelectorAll("panel[hudId=" + this.hudId + "]");
    for (let panel of panels) {
      if (panel != this.consolePanel) {
        panel.hidePopup();
      }
    }

    if (this.ui) {
      this.ui.destroy();
    }

    
    
    if (this.consolePanel && this.consolePanel.parentNode) {
      this.consolePanel.hidePopup();
      this.consolePanel.parentNode.removeChild(this.consolePanel);
      this.consolePanel = null;
    }

    if (this.iframe.parentNode) {
      this.iframe.parentNode.removeChild(this.iframe);
    }

    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }
  },
};





var HeadsUpDisplayUICommands = {
  refreshCommand: function UIC_refreshCommand() {
    var window = HUDService.currentContext();
    if (!window) {
      return;
    }

    let command = window.document.getElementById("Tools:WebConsole");
    if (this.getOpenHUD() != null) {
      command.setAttribute("checked", true);
    } else {
      command.setAttribute("checked", false);
    }
  },

  toggleHUD: function UIC_toggleHUD() {
    var window = HUDService.currentContext();
    var gBrowser = window.gBrowser;
    var linkedBrowser = gBrowser.selectedTab.linkedBrowser;
    var tabId = gBrowser.getNotificationBox(linkedBrowser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var ownerDocument = gBrowser.selectedTab.ownerDocument;
    var hud = ownerDocument.getElementById(hudId);
    var hudRef = HUDService.hudReferences[hudId];

    if (hudRef && hud) {
      if (hudRef.consolePanel) {
        hudRef.consolePanel.hidePopup();
      }
      else {
        HUDService.storeHeight(hudId);

        HUDService.animate(hudId, ANIMATE_OUT, function() {
          
          
          
          
          if (ownerDocument.getElementById(hudId)) {
            HUDService.deactivateHUDForContext(gBrowser.selectedTab, true);
          }
        });
      }
    }
    else {
      HUDService.activateHUDForContext(gBrowser.selectedTab, true);
      HUDService.animate(hudId, ANIMATE_IN);
    }
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


XPCOMUtils.defineLazyGetter(this, "HUDService", function () {
  return new HUD_SERVICE();
});

