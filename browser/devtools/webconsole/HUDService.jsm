





const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const CONSOLEAPI_CLASS_ID = "{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["HUDService", "ConsoleUtils"];

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

XPCOMUtils.defineLazyModuleGetter(this, "PropertyPanel",
                                  "resource:///modules/PropertyPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PropertyTreeView",
                                  "resource:///modules/PropertyPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AutocompletePopup",
                                  "resource:///modules/AutocompletePopup.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkPanel",
                                  "resource:///modules/NetworkPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource:///modules/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "l10n", function() {
  return WebConsoleUtils.l10n;
});

function LogFactory(aMessagePrefix)
{
  function log(aMessage) {
    var _msg = aMessagePrefix + " " + aMessage + "\n";
    dump(_msg);
  }
  return log;
}

let log = LogFactory("*** HUDService:");



const NEW_GROUP_DELAY = 5000;



const SEARCH_DELAY = 200;




const DEFAULT_LOG_LIMIT = 200;



const CATEGORY_NETWORK = 0;
const CATEGORY_CSS = 1;
const CATEGORY_JS = 2;
const CATEGORY_WEBDEV = 3;
const CATEGORY_INPUT = 4;   
const CATEGORY_OUTPUT = 5;  



const SEVERITY_ERROR = 0;
const SEVERITY_WARNING = 1;
const SEVERITY_INFO = 2;
const SEVERITY_LOG = 3;



const LEVELS = {
  error: SEVERITY_ERROR,
  warn: SEVERITY_WARNING,
  info: SEVERITY_INFO,
  log: SEVERITY_LOG,
  trace: SEVERITY_LOG,
  debug: SEVERITY_LOG,
  dir: SEVERITY_LOG,
  group: SEVERITY_LOG,
  groupCollapsed: SEVERITY_LOG,
  groupEnd: SEVERITY_LOG,
  time: SEVERITY_LOG,
  timeEnd: SEVERITY_LOG
};


const MIN_HTTP_ERROR_CODE = 400;

const MAX_HTTP_ERROR_CODE = 599;


const HTML_NS = "http://www.w3.org/1999/xhtml";


const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


const CATEGORY_CLASS_FRAGMENTS = [
  "network",
  "cssparser",
  "exception",
  "console",
  "input",
  "output",
];


const SEVERITY_CLASS_FRAGMENTS = [
  "error",
  "warn",
  "info",
  "log",
];






const MESSAGE_PREFERENCE_KEYS = [

  [ "network",    null,         null,   "networkinfo", ],  
  [ "csserror",   "cssparser",  null,   null,          ],  
  [ "exception",  "jswarn",     null,   null,          ],  
  [ "error",      "warn",       "info", "log",         ],  
  [ null,         null,         null,   null,          ],  
  [ null,         null,         null,   null,          ],  
];


const ANIMATE_OUT = 0;
const ANIMATE_IN = 1;


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;


const MINIMUM_CONSOLE_HEIGHT = 150;



const MINIMUM_PAGE_HEIGHT = 50;


const DEFAULT_CONSOLE_HEIGHT = 0.33;


const CONTENT_SCRIPT_URL = "chrome://browser/content/devtools/HUDService-content.js";

const ERRORS = { LOG_MESSAGE_MISSING_ARGS:
                 "Missing arguments: aMessage, aConsoleNode and aMessageNode are required.",
                 CANNOT_GET_HUD: "Cannot getHeads Up Display with provided ID",
                 MISSING_ARGS: "Missing arguments",
                 LOG_OUTPUT_FAILED: "Log Failure: Could not append messageNode to outputNode",
};


const GROUP_INDENT = 12;


const PREFS_PREFIX = "devtools.webconsole.filter.";



const MESSAGES_IN_INTERVAL = DEFAULT_LOG_LIMIT;




const OUTPUT_INTERVAL = 50; 




const THROTTLE_UPDATES = 1000; 

















function createElement(aDocument, aTag, aAttributes)
{
  let node = aDocument.createElement(aTag);
  if (aAttributes) {
    for (let attr in aAttributes) {
      node.setAttribute(attr, aAttributes[attr]);
    }
  }
  return node;
}















function pruneConsoleOutputIfNecessary(aHUDId, aCategory)
{
  let hudRef = HUDService.getHudReferenceById(aHUDId);
  let outputNode = hudRef.outputNode;
  let logLimit = hudRef.logLimitForCategory(aCategory);

  let messageNodes = outputNode.getElementsByClassName("webconsole-msg-" +
      CATEGORY_CLASS_FRAGMENTS[aCategory]);
  let n = Math.max(0, messageNodes.length - logLimit);
  let toRemove = Array.prototype.slice.call(messageNodes, 0, n);
  toRemove.forEach(hudRef.removeOutputMessage, hudRef);

  return n;
}




function HUD_SERVICE()
{
  
  
  this.onTabClose = this.onTabClose.bind(this);
  this.onWindowUnload = this.onWindowUnload.bind(this);

  
  this.lastConsoleHeight = Services.prefs.getIntPref("devtools.hud.height");

  


  this.filterPrefs = {};

  


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
    window.addEventListener("unload", this.onWindowUnload, false);

    this.registerDisplay(hudId);

    let hud = new HeadsUpDisplay(aTab);
    this.hudReferences[hudId] = hud;

    
    this.createController(window);

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

    this.unregisterDisplay(hudId);

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

  





  getDefaultFilterPrefs: function HS_getDefaultFilterPrefs(aHUDId) {
    return this.filterPrefs[aHUDId];
  },

  





  getFilterPrefs: function HS_getFilterPrefs(aHUDId) {
    return this.filterPrefs[aHUDId];
  },

  






  getFilterState: function HS_getFilterState(aHUDId, aToggleType)
  {
    if (!aHUDId) {
      return false;
    }
    try {
      var bool = this.filterPrefs[aHUDId][aToggleType];
      return bool;
    }
    catch (ex) {
      return false;
    }
  },

  







  setFilterState: function HS_setFilterState(aHUDId, aToggleType, aState)
  {
    this.filterPrefs[aHUDId][aToggleType] = aState;
    this.adjustVisibilityForMessageType(aHUDId, aToggleType, aState);
    Services.prefs.setBoolPref(PREFS_PREFIX + aToggleType, aState);
  },

  






  regroupOutput: function HS_regroupOutput(aOutputNode)
  {
    
    
    let nodes = aOutputNode.querySelectorAll(".hud-msg-node" +
      ":not(.hud-filtered-by-string):not(.hud-filtered-by-type)");
    let lastTimestamp;
    for (let i = 0, n = nodes.length; i < n; i++) {
      let thisTimestamp = nodes[i].timestamp;
      if (lastTimestamp != null &&
          thisTimestamp >= lastTimestamp + NEW_GROUP_DELAY) {
        nodes[i].classList.add("webconsole-new-group");
      }
      else {
        nodes[i].classList.remove("webconsole-new-group");
      }
      lastTimestamp = thisTimestamp;
    }
  },

  













  adjustVisibilityForMessageType:
  function HS_adjustVisibilityForMessageType(aHUDId, aPrefKey, aState)
  {
    let outputNode = this.getHudReferenceById(aHUDId).outputNode;
    let doc = outputNode.ownerDocument;

    
    
    

    let xpath = ".//*[contains(@class, 'hud-msg-node') and " +
      "contains(concat(@class, ' '), 'hud-" + aPrefKey + " ')]";
    let result = doc.evaluate(xpath, outputNode, null,
      Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
    for (let i = 0; i < result.snapshotLength; i++) {
      let node = result.snapshotItem(i);
      if (aState) {
        node.classList.remove("hud-filtered-by-type");
      }
      else {
        node.classList.add("hud-filtered-by-type");
      }
    }

    this.regroupOutput(outputNode);
  },

  








  stringMatchesFilters: function stringMatchesFilters(aString, aFilter)
  {
    if (!aFilter || !aString) {
      return true;
    }

    let searchStr = aString.toLowerCase();
    let filterStrings = aFilter.toLowerCase().split(/\s+/);
    return !filterStrings.some(function (f) {
      return searchStr.indexOf(f) == -1;
    });
  },

  









  adjustVisibilityOnSearchStringChange:
  function HS_adjustVisibilityOnSearchStringChange(aHUDId, aSearchString)
  {
    let outputNode = this.getHudReferenceById(aHUDId).outputNode;

    let nodes = outputNode.getElementsByClassName("hud-msg-node");

    for (let i = 0, n = nodes.length; i < n; ++i) {
      let node = nodes[i];

      
      let text = node.clipboardText;

      
      if (this.stringMatchesFilters(text, aSearchString)) {
        node.classList.remove("hud-filtered-by-string");
      }
      else {
        node.classList.add("hud-filtered-by-string");
      }
    }

    this.regroupOutput(outputNode);
  },

  




  registerDisplay: function HS_registerDisplay(aHUDId)
  {
    
    if (!aHUDId){
      throw new Error(ERRORS.MISSING_ARGS);
    }
    this.filterPrefs[aHUDId] = {
      network: Services.prefs.getBoolPref(PREFS_PREFIX + "network"),
      networkinfo: Services.prefs.getBoolPref(PREFS_PREFIX + "networkinfo"),
      csserror: Services.prefs.getBoolPref(PREFS_PREFIX + "csserror"),
      cssparser: Services.prefs.getBoolPref(PREFS_PREFIX + "cssparser"),
      exception: Services.prefs.getBoolPref(PREFS_PREFIX + "exception"),
      jswarn: Services.prefs.getBoolPref(PREFS_PREFIX + "jswarn"),
      error: Services.prefs.getBoolPref(PREFS_PREFIX + "error"),
      info: Services.prefs.getBoolPref(PREFS_PREFIX + "info"),
      warn: Services.prefs.getBoolPref(PREFS_PREFIX + "warn"),
      log: Services.prefs.getBoolPref(PREFS_PREFIX + "log"),
    };
  },

  






  unregisterDisplay: function HS_unregisterDisplay(aHUDId)
  {
    let hud = this.getHudReferenceById(aHUDId);
    let document = hud.chromeDocument;

    hud.destroy();

    delete this.hudReferences[aHUDId];

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

      this.suspend();
    }
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
    delete this.defaultFilterPrefs;
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

  





  getFilterStringByHUDId: function HS_getFilterStringbyHUDId(aHUDId) {
    return this.getHudReferenceById(aHUDId).filterBox.value;
  },

  






  updateFilterText: function HS_updateFiltertext(aTextBoxNode)
  {
    var hudId = aTextBoxNode.getAttribute("hudId");
    this.adjustVisibilityOnSearchStringChange(hudId, aTextBoxNode.value);
  },

  






  lastFinishedRequestCallback: null,

  










  openNetworkPanel: function HS_openNetworkPanel(aNode, aHttpActivity)
  {
    let doc = aNode.ownerDocument;
    let parent = doc.getElementById("mainPopupSet");
    let netPanel = new NetworkPanel(parent, aHttpActivity);
    netPanel.linkNode = aNode;
    aNode._netPanel = netPanel;

    let panel = netPanel.panel;
    panel.openPopup(aNode, "after_pointer", 0, 0, false, false);
    panel.sizeTo(450, 500);
    panel.setAttribute("hudId", aHttpActivity.hudId);

    panel.addEventListener("popuphiding", function HS_netPanel_onHide() {
      panel.removeEventListener("popuphiding", HS_netPanel_onHide);

      aNode._panelOpen = false;
      aNode._netPanel = null;
    });

    aNode._panelOpen = true;

    return netPanel;
  },

  





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

  







  onWindowUnload: function HS_onWindowUnload(aEvent)
  {
    let window = aEvent.target.defaultView;

    window.removeEventListener("unload", this.onWindowUnload, false);

    let gBrowser = window.gBrowser;
    let tabContainer = gBrowser.tabContainer;

    tabContainer.removeEventListener("TabClose", this.onTabClose, false);

    let tab = tabContainer.firstChild;
    while (tab != null) {
      this.deactivateHUDForContext(tab, false);
      tab = tab.nextSibling;
    }

    if (window.webConsoleCommandController) {
      window.controllers.removeController(window.webConsoleCommandController);
      window.webConsoleCommandController = null;
    }
  },

  






  createController: function HS_createController(aWindow)
  {
    if (!aWindow.webConsoleCommandController) {
      aWindow.webConsoleCommandController = new CommandController(aWindow);
      aWindow.controllers.insertControllerAt(0,
        aWindow.webConsoleCommandController);
    }
  },

  









  animate: function HS_animate(aHUDId, aDirection, aCallback)
  {
    let hudBox = this.getHudReferenceById(aHUDId).HUDBox;
    if (!hudBox.classList.contains("animated")) {
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
    let hudBox = HUDService.hudReferences[aHUDId].HUDBox;
    if (hudBox.classList.contains("animated")) {
      hudBox.classList.remove("animated");
      this.resetHeight(aHUDId);
    }
  },

  




  resetHeight: function HS_resetHeight(aHUDId)
  {
    let HUD = this.hudReferences[aHUDId];
    let innerHeight = HUD.browser.clientHeight;
    let chromeWindow = HUD.chromeWindow;
    if (!HUD.consolePanel) {
      let splitterStyle = chromeWindow.getComputedStyle(HUD.splitter, null);
      innerHeight += parseInt(splitterStyle.height) +
                     parseInt(splitterStyle.borderTopWidth) +
                     parseInt(splitterStyle.borderBottomWidth);
    }

    let boxStyle = chromeWindow.getComputedStyle(HUD.HUDBox, null);
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

    HUD.HUDBox.style.height = height + "px";
  },

  





  storeHeight: function HS_storeHeight(aHUDId)
  {
    let hudBox = this.hudReferences[aHUDId].HUDBox;
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

  






  copySelectedItems: function HS_copySelectedItems(aOutputNode)
  {
    

    let strings = [];
    let newGroup = false;

    let children = aOutputNode.children;

    for (let i = 0; i < children.length; i++) {
      let item = children[i];
      if (!item.selected) {
        continue;
      }

      
      
      if (i > 0 && item.classList.contains("webconsole-new-group")) {
        newGroup = true;
      }

      
      if (!item.classList.contains("hud-filtered-by-type") &&
          !item.classList.contains("hud-filtered-by-string")) {
        let timestampString = l10n.timestampString(item.timestamp);
        if (newGroup) {
          strings.push("--");
          newGroup = false;
        }
        strings.push("[" + timestampString + "] " + item.clipboardText);
      }
    }
    clipboardHelper.copyString(strings.join("\n"));
  }
};













function HeadsUpDisplay(aTab)
{
  this.tab = aTab;
  this.hudId = "hud_" + this.tab.linkedPanel;
  this.chromeDocument = this.tab.ownerDocument;
  this.chromeWindow = this.chromeDocument.defaultView;
  this.notificationBox = this.chromeDocument.getElementById(this.tab.linkedPanel);
  this.browser = this.tab.linkedBrowser;
  this.messageManager = this.browser.messageManager;

  
  
  this.asyncRequests = {};

  
  this.createHUD();

  this._outputQueue = [];
  this._pruneCategoriesQueue = {};

  
  this.jsterm = new JSTerm(this);
  this.jsterm.inputNode.focus();

  
  this.cssNodes = {};

  this._networkRequests = {};

  this._setupMessageManager();
}

HeadsUpDisplay.prototype = {
  





  _networkRequests: null,

  






  _lastOutputFlush: 0,

  





  _outputQueue: null,

  





  _pruneCategoriesQueue: null,

  






  _messageListeners: ["JSTerm:EvalObject", "WebConsole:ConsoleAPI",
    "WebConsole:CachedMessages", "WebConsole:PageError", "JSTerm:EvalResult",
    "JSTerm:AutocompleteProperties", "JSTerm:ClearOutput",
    "JSTerm:InspectObject", "WebConsole:NetworkActivity",
    "WebConsole:FileActivity", "WebConsole:LocationChange",
    "JSTerm:NonNativeConsoleAPI"],

  consolePanel: null,

  contentLocation: "",

  


  groupDepth: 0,

  _saveRequestAndResponseBodies: false,

  




  get saveRequestAndResponseBodies() this._saveRequestAndResponseBodies,

  





  set saveRequestAndResponseBodies(aValue) {
    this._saveRequestAndResponseBodies = aValue;

    let message = {
      preferences: {
        "NetworkMonitor.saveRequestAndResponseBodies":
          this._saveRequestAndResponseBodies,
      },
    };

    this.sendMessageToContent("WebConsole:SetPreferences", message);
  },

  get mainPopupSet()
  {
    return this.chromeDocument.getElementById("mainPopupSet");
  },

  



  createOwnWindowPanel: function HUD_createOwnWindowPanel()
  {
    if (this.uiInOwnWindow) {
      return this.consolePanel;
    }

    let width = 0;
    try {
      width = Services.prefs.getIntPref("devtools.webconsole.width");
    }
    catch (ex) {}

    if (width < 1) {
      width = this.HUDBox.clientWidth || this.chromeWindow.innerWidth;
    }

    let height = this.HUDBox.clientHeight;

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

      this.HUDBox.style.height = "auto";
      this.HUDBox.setAttribute("flex", "1");

      panel.setAttribute("height", height);

      
      if (lastIndex > -1 && lastIndex < this.outputNode.getRowCount()) {
        this.outputNode.ensureIndexIsVisible(lastIndex);
      }

      this.jsterm.inputNode.focus();
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

    if (this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }
    panel.appendChild(this.HUDBox);

    let space = this.chromeDocument.createElement("spacer");
    space.setAttribute("flex", "1");

    let bottomBox = this.chromeDocument.createElement("hbox");
    space.setAttribute("flex", "1");

    let resizer = this.chromeDocument.createElement("resizer");
    resizer.setAttribute("dir", "bottomend");
    resizer.setAttribute("element", config.id);

    bottomBox.appendChild(space);
    bottomBox.appendChild(resizer);

    panel.appendChild(bottomBox);

    this.mainPopupSet.appendChild(panel);

    Services.prefs.setCharPref("devtools.webconsole.position", "window");

    panel.openPopup(null, "overlay", left, top, false, false);

    this.consolePanel = panel;
    this.consoleWindowUnregisterOnHide = true;

    return panel;
  },

  





  getPanelTitle: function HUD_getPanelTitle()
  {
    return l10n.getFormatStr("webConsoleWindowTitleAndURL", [this.contentLocation]);
  },

  positions: {
    above: 0, 
    below: 2,
    window: null
  },

  consoleWindowUnregisterOnHide: true,

  


  positionConsole: function HUD_positionConsole(aPosition)
  {
    if (!(aPosition in this.positions)) {
      throw new Error("Incorrect argument: " + aPosition +
        ". Cannot position Web Console");
    }

    if (aPosition == "window") {
      let closeButton = this.consoleFilterToolbar.
        querySelector(".webconsole-close-button");
      closeButton.setAttribute("hidden", "true");
      this.createOwnWindowPanel();
      this.positionMenuitems.window.setAttribute("checked", true);
      if (this.positionMenuitems.last) {
        this.positionMenuitems.last.setAttribute("checked", false);
      }
      this.positionMenuitems.last = this.positionMenuitems[aPosition];
      this.uiInOwnWindow = true;
      return;
    }

    let height = this.HUDBox.clientHeight;

    
    let nodeIdx = this.positions[aPosition];
    let nBox = this.notificationBox;
    let node = nBox.childNodes[nodeIdx];

    
    if (node == this.HUDBox) {
      return;
    }

    let lastIndex = -1;

    if (this.outputNode.getIndexOfFirstVisibleRow) {
      lastIndex = this.outputNode.getIndexOfFirstVisibleRow() +
                  this.outputNode.getNumberOfVisibleRows() - 1;
    }

    
    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }

    if (aPosition == "below") {
      nBox.appendChild(this.splitter);
      nBox.appendChild(this.HUDBox);
    }
    else {
      nBox.insertBefore(this.splitter, node);
      nBox.insertBefore(this.HUDBox, this.splitter);
    }

    this.positionMenuitems[aPosition].setAttribute("checked", true);
    if (this.positionMenuitems.last) {
      this.positionMenuitems.last.setAttribute("checked", false);
    }
    this.positionMenuitems.last = this.positionMenuitems[aPosition];

    Services.prefs.setCharPref("devtools.webconsole.position", aPosition);

    if (lastIndex > -1 && lastIndex < this.outputNode.getRowCount()) {
      this.outputNode.ensureIndexIsVisible(lastIndex);
    }

    let closeButton = this.consoleFilterToolbar.
      getElementsByClassName("webconsole-close-button")[0];
    closeButton.removeAttribute("hidden");

    this.uiInOwnWindow = false;
    if (this.consolePanel) {
      
      this.consoleWindowUnregisterOnHide = false;
      this.consolePanel.hidePopup();
      this.consolePanel.parentNode.removeChild(this.consolePanel);
      this.consolePanel = null;   
      this.HUDBox.removeAttribute("flex");
      this.HUDBox.removeAttribute("height");
      this.HUDBox.style.height = height + "px";
    }

    if (this.jsterm) {
      this.jsterm.inputNode.focus();
    }
  },

  



  jsterm: null,

  








  _displayCachedConsoleMessages:
  function HUD__displayCachedConsoleMessages(aRemoteMessages)
  {
    if (!aRemoteMessages.length) {
      return;
    }

    aRemoteMessages.forEach(function(aMessage) {
      switch (aMessage._type) {
        case "PageError": {
          let category = this.categoryForScriptError(aMessage.category);
          this.outputMessage(category, this.reportPageError,
                             [category, aMessage]);
          break;
        }
        case "ConsoleAPI":
          this.outputMessage(CATEGORY_WEBDEV, this.logConsoleAPIMessage,
                             [aMessage]);
          break;
      }
    }, this);
  },

  





  makeXULNode:
  function HUD_makeXULNode(aTag)
  {
    return this.chromeDocument.createElement(aTag);
  },

  




  makeHUDNodes: function HUD_makeHUDNodes()
  {
    this.splitter = this.makeXULNode("splitter");
    this.splitter.setAttribute("class", "hud-splitter");

    this.HUDBox = this.makeXULNode("vbox");
    this.HUDBox.setAttribute("id", this.hudId);
    this.HUDBox.setAttribute("class", "hud-box animated");
    this.HUDBox.style.height = 0;
    this.HUDBox.lastTimestamp = 0;

    let outerWrap = this.makeXULNode("vbox");
    outerWrap.setAttribute("class", "hud-outer-wrapper");
    outerWrap.setAttribute("flex", "1");

    let consoleCommandSet = this.makeXULNode("commandset");
    outerWrap.appendChild(consoleCommandSet);

    let consoleWrap = this.makeXULNode("vbox");
    this.consoleWrap = consoleWrap;
    consoleWrap.setAttribute("class", "hud-console-wrapper");
    consoleWrap.setAttribute("flex", "1");

    this.filterSpacer = this.makeXULNode("spacer");
    this.filterSpacer.setAttribute("flex", "1");

    this.filterBox = this.makeXULNode("textbox");
    this.filterBox.setAttribute("class", "compact hud-filter-box");
    this.filterBox.setAttribute("hudId", this.hudId);
    this.filterBox.setAttribute("placeholder", l10n.getStr("stringFilter"));
    this.filterBox.setAttribute("type", "search");

    this.setFilterTextBoxEvents();

    this.createConsoleMenu(this.consoleWrap);

    this.filterPrefs = HUDService.getDefaultFilterPrefs(this.hudId);

    let consoleFilterToolbar = this.makeFilterToolbar();
    consoleFilterToolbar.setAttribute("id", "viewGroup");
    this.consoleFilterToolbar = consoleFilterToolbar;

    this.hintNode = this.makeXULNode("vbox");
    this.hintNode.setAttribute("class", "gcliterm-hint-node");

    let hintParentNode = this.makeXULNode("vbox");
    hintParentNode.setAttribute("flex", "0");
    hintParentNode.setAttribute("class", "gcliterm-hint-parent");
    hintParentNode.setAttribute("pack", "end");
    hintParentNode.appendChild(this.hintNode);
    hintParentNode.hidden = true;

    let hbox = this.makeXULNode("hbox");
    hbox.setAttribute("flex", "1");
    hbox.setAttribute("class", "gcliterm-display");

    this.outputNode = this.makeXULNode("richlistbox");
    this.outputNode.setAttribute("class", "hud-output-node");
    this.outputNode.setAttribute("flex", "1");
    this.outputNode.setAttribute("orient", "vertical");
    this.outputNode.setAttribute("context", this.hudId + "-output-contextmenu");
    this.outputNode.setAttribute("style", "direction: ltr;");
    this.outputNode.setAttribute("seltype", "multiple");

    hbox.appendChild(hintParentNode);
    hbox.appendChild(this.outputNode);

    consoleWrap.appendChild(consoleFilterToolbar);
    consoleWrap.appendChild(hbox);

    outerWrap.appendChild(consoleWrap);

    this.HUDBox.lastTimestamp = 0;

    this.jsTermParentNode = this.consoleWrap;
    this.HUDBox.appendChild(outerWrap);

    return this.HUDBox;
  },

  




  setFilterTextBoxEvents: function HUD_setFilterTextBoxEvents()
  {
    var filterBox = this.filterBox;
    function onChange()
    {
      
      

      if (this.timer == null) {
        let timerClass = Cc["@mozilla.org/timer;1"];
        this.timer = timerClass.createInstance(Ci.nsITimer);
      } else {
        this.timer.cancel();
      }

      let timerEvent = {
        notify: function setFilterTextBoxEvents_timerEvent_notify() {
          HUDService.updateFilterText(filterBox);
        }
      };

      this.timer.initWithCallback(timerEvent, SEARCH_DELAY,
        Ci.nsITimer.TYPE_ONE_SHOT);
    }

    filterBox.addEventListener("command", onChange, false);
    filterBox.addEventListener("input", onChange, false);
  },

  




  makeFilterToolbar: function HUD_makeFilterToolbar()
  {
    const BUTTONS = [
      {
        name: "PageNet",
        category: "net",
        severities: [
          { name: "ConsoleErrors", prefKey: "network" },
          { name: "ConsoleLog", prefKey: "networkinfo" }
        ]
      },
      {
        name: "PageCSS",
        category: "css",
        severities: [
          { name: "ConsoleErrors", prefKey: "csserror" },
          { name: "ConsoleWarnings", prefKey: "cssparser" }
        ]
      },
      {
        name: "PageJS",
        category: "js",
        severities: [
          { name: "ConsoleErrors", prefKey: "exception" },
          { name: "ConsoleWarnings", prefKey: "jswarn" }
        ]
      },
      {
        name: "PageLogging",
        category: "logging",
        severities: [
          { name: "ConsoleErrors", prefKey: "error" },
          { name: "ConsoleWarnings", prefKey: "warn" },
          { name: "ConsoleInfo", prefKey: "info" },
          { name: "ConsoleLog", prefKey: "log" }
        ]
      }
    ];

    let toolbar = this.makeXULNode("toolbar");
    toolbar.setAttribute("class", "hud-console-filter-toolbar");
    toolbar.setAttribute("mode", "full");

#ifdef XP_MACOSX
    this.makeCloseButton(toolbar);
#endif

    for (let i = 0; i < BUTTONS.length; i++) {
      this.makeFilterButton(toolbar, BUTTONS[i]);
    }

    toolbar.appendChild(this.filterSpacer);

    let positionUI = this.createPositionUI();
    toolbar.appendChild(positionUI);

    toolbar.appendChild(this.filterBox);
    this.makeClearConsoleButton(toolbar);

#ifndef XP_MACOSX
    this.makeCloseButton(toolbar);
#endif

    return toolbar;
  },

  






  createPositionUI: function HUD_createPositionUI()
  {
    this._positionConsoleAbove = (function HUD_positionAbove() {
      this.positionConsole("above");
    }).bind(this);

    this._positionConsoleBelow = (function HUD_positionBelow() {
      this.positionConsole("below");
    }).bind(this);
    this._positionConsoleWindow = (function HUD_positionWindow() {
      this.positionConsole("window");
    }).bind(this);

    let button = this.makeXULNode("toolbarbutton");
    button.setAttribute("type", "menu");
    button.setAttribute("label", l10n.getStr("webConsolePosition"));
    button.setAttribute("tooltip", l10n.getStr("webConsolePositionTooltip"));

    let menuPopup = this.makeXULNode("menupopup");
    button.appendChild(menuPopup);

    let itemAbove = this.makeXULNode("menuitem");
    itemAbove.setAttribute("label", l10n.getStr("webConsolePositionAbove"));
    itemAbove.setAttribute("type", "checkbox");
    itemAbove.setAttribute("autocheck", "false");
    itemAbove.addEventListener("command", this._positionConsoleAbove, false);
    menuPopup.appendChild(itemAbove);

    let itemBelow = this.makeXULNode("menuitem");
    itemBelow.setAttribute("label", l10n.getStr("webConsolePositionBelow"));
    itemBelow.setAttribute("type", "checkbox");
    itemBelow.setAttribute("autocheck", "false");
    itemBelow.addEventListener("command", this._positionConsoleBelow, false);
    menuPopup.appendChild(itemBelow);

    let itemWindow = this.makeXULNode("menuitem");
    itemWindow.setAttribute("label", l10n.getStr("webConsolePositionWindow"));
    itemWindow.setAttribute("type", "checkbox");
    itemWindow.setAttribute("autocheck", "false");
    itemWindow.addEventListener("command", this._positionConsoleWindow, false);
    menuPopup.appendChild(itemWindow);

    this.positionMenuitems = {
      last: null,
      above: itemAbove,
      below: itemBelow,
      window: itemWindow,
    };

    return button;
  },

  






  createConsoleMenu: function HUD_createConsoleMenu(aConsoleWrapper) {
    let menuPopup = this.makeXULNode("menupopup");
    let id = this.hudId + "-output-contextmenu";
    menuPopup.setAttribute("id", id);
    menuPopup.addEventListener("popupshowing", function() {
      saveBodiesItem.setAttribute("checked", this.saveRequestAndResponseBodies);
    }.bind(this), true);

    let saveBodiesItem = this.makeXULNode("menuitem");
    saveBodiesItem.setAttribute("label", l10n.getStr("saveBodies.label"));
    saveBodiesItem.setAttribute("accesskey",
                                 l10n.getStr("saveBodies.accesskey"));
    saveBodiesItem.setAttribute("type", "checkbox");
    saveBodiesItem.setAttribute("buttonType", "saveBodies");
    saveBodiesItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    saveBodiesItem.setAttribute("hudId", this.hudId);
    menuPopup.appendChild(saveBodiesItem);

    menuPopup.appendChild(this.makeXULNode("menuseparator"));

    let copyItem = this.makeXULNode("menuitem");
    copyItem.setAttribute("label", l10n.getStr("copyCmd.label"));
    copyItem.setAttribute("accesskey", l10n.getStr("copyCmd.accesskey"));
    copyItem.setAttribute("key", "key_copy");
    copyItem.setAttribute("command", "cmd_copy");
    copyItem.setAttribute("buttonType", "copy");
    menuPopup.appendChild(copyItem);

    let selectAllItem = this.makeXULNode("menuitem");
    selectAllItem.setAttribute("label", l10n.getStr("selectAllCmd.label"));
    selectAllItem.setAttribute("accesskey",
                               l10n.getStr("selectAllCmd.accesskey"));
    selectAllItem.setAttribute("hudId", this.hudId);
    selectAllItem.setAttribute("buttonType", "selectAll");
    selectAllItem.setAttribute("oncommand", "HUDConsoleUI.command(this);");
    menuPopup.appendChild(selectAllItem);

    aConsoleWrapper.appendChild(menuPopup);
    aConsoleWrapper.setAttribute("context", id);
  },

  










  makeFilterButton: function HUD_makeFilterButton(aParent, aDescriptor)
  {
    let toolbarButton = this.makeXULNode("toolbarbutton");
    aParent.appendChild(toolbarButton);

    let toggleFilter = HeadsUpDisplayUICommands.toggleFilter;
    toolbarButton.addEventListener("click", toggleFilter, false);

    let name = aDescriptor.name;
    toolbarButton.setAttribute("type", "menu-button");
    toolbarButton.setAttribute("label", l10n.getStr("btn" + name));
    toolbarButton.setAttribute("tooltip", l10n.getStr("tip" + name));
    toolbarButton.setAttribute("category", aDescriptor.category);
    toolbarButton.setAttribute("hudId", this.hudId);
    toolbarButton.classList.add("webconsole-filter-button");

    let menuPopup = this.makeXULNode("menupopup");
    toolbarButton.appendChild(menuPopup);

    let someChecked = false;
    for (let i = 0; i < aDescriptor.severities.length; i++) {
      let severity = aDescriptor.severities[i];
      let menuItem = this.makeXULNode("menuitem");
      menuItem.setAttribute("label", l10n.getStr("btn" + severity.name));
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("autocheck", "false");
      menuItem.setAttribute("hudId", this.hudId);

      let prefKey = severity.prefKey;
      menuItem.setAttribute("prefKey", prefKey);

      let checked = this.filterPrefs[prefKey];
      menuItem.setAttribute("checked", checked);
      if (checked) {
        someChecked = true;
      }

      menuItem.addEventListener("command", toggleFilter, false);

      menuPopup.appendChild(menuItem);
    }

    toolbarButton.setAttribute("checked", someChecked);
  },

  






  makeCloseButton: function HUD_makeCloseButton(aToolbar)
  {
    this.closeButtonOnCommand = (function HUD_closeButton_onCommand() {
      HUDService.animate(this.hudId, ANIMATE_OUT, (function() {
        HUDService.deactivateHUDForContext(this.tab, true);
      }).bind(this));
    }).bind(this);

    this.closeButton = this.makeXULNode("toolbarbutton");
    this.closeButton.classList.add("webconsole-close-button");
    this.closeButton.addEventListener("command",
      this.closeButtonOnCommand, false);
    aToolbar.appendChild(this.closeButton);
  },

  








  makeClearConsoleButton: function HUD_makeClearConsoleButton(aToolbar)
  {
    let hudId = this.hudId;
    function HUD_clearButton_onCommand() {
      let hud = HUDService.getHudReferenceById(hudId);
      hud.jsterm.clearOutput(true);
    }

    let clearButton = this.makeXULNode("toolbarbutton");
    clearButton.setAttribute("label", l10n.getStr("btnClear"));
    clearButton.classList.add("webconsole-clear-console-button");
    clearButton.addEventListener("command", HUD_clearButton_onCommand, false);

    aToolbar.appendChild(clearButton);
  },

  







  pruneConsoleDirNode: function HUD_pruneConsoleDirNode(aMessageNode)
  {
    if (aMessageNode.parentNode) {
      aMessageNode.parentNode.removeChild(aMessageNode);
    }

    let tree = aMessageNode.querySelector("tree");
    tree.parentNode.removeChild(tree);
    aMessageNode.propertyTreeView = null;
    if (tree.view) {
      tree.view.data = null;
    }
    tree.view = null;
  },

  





  createHUD: function HUD_createHUD()
  {
    if (!this.HUDBox) {
      this.makeHUDNodes();
      let positionPref = Services.prefs.getCharPref("devtools.webconsole.position");
      this.positionConsole(positionPref);
    }
    return this.HUDBox;
  },

  uiInOwnWindow: false,

  
















  logConsoleAPIMessage: function HUD_logConsoleAPIMessage(aMessage)
  {
    let body = null;
    let clipboardText = null;
    let sourceURL = null;
    let sourceLine = 0;
    let level = aMessage.apiMessage.level;
    let args = aMessage.apiMessage.arguments;
    let argsToString = aMessage.argumentsToString;

    switch (level) {
      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
        body = argsToString.join(" ");
        sourceURL = aMessage.apiMessage.filename;
        sourceLine = aMessage.apiMessage.lineNumber;
        break;

      case "trace":
        let filename = WebConsoleUtils.abbreviateSourceURL(args[0].filename);
        let functionName = args[0].functionName ||
                           l10n.getStr("stacktrace.anonymousFunction");
        let lineNumber = args[0].lineNumber;

        body = l10n.getFormatStr("stacktrace.outputMessage",
                                 [filename, functionName, lineNumber]);

        sourceURL = args[0].filename;
        sourceLine = args[0].lineNumber;

        clipboardText = "";

        args.forEach(function(aFrame) {
          clipboardText += aFrame.filename + " :: " +
                           aFrame.functionName + " :: " +
                           aFrame.lineNumber + "\n";
        });

        clipboardText = clipboardText.trimRight();
        break;

      case "dir":
        body = {
          cacheId: aMessage.objectsCacheId,
          resultString: argsToString[0],
          remoteObject: args[0],
          remoteObjectProvider:
            this.jsterm.remoteObjectProvider.bind(this.jsterm),
        };
        clipboardText = body.resultString;
        sourceURL = aMessage.apiMessage.filename;
        sourceLine = aMessage.apiMessage.lineNumber;
        break;

      case "group":
      case "groupCollapsed":
        clipboardText = body = args;
        sourceURL = aMessage.apiMessage.filename;
        sourceLine = aMessage.apiMessage.lineNumber;
        this.groupDepth++;
        break;

      case "groupEnd":
        if (this.groupDepth > 0) {
          this.groupDepth--;
        }
        return;

      case "time":
        if (!args) {
          return;
        }
        if (args.error) {
          Cu.reportError(l10n.getStr(args.error));
          return;
        }
        body = l10n.getFormatStr("timerStarted", [args.name]);
        clipboardText = body;
        sourceURL = aMessage.apiMessage.filename;
        sourceLine = aMessage.apiMessage.lineNumber;
        break;

      case "timeEnd":
        if (!args) {
          return;
        }
        body = l10n.getFormatStr("timeEnd", [args.name, args.duration]);
        clipboardText = body;
        sourceURL = aMessage.apiMessage.filename;
        sourceLine = aMessage.apiMessage.lineNumber;
        break;

      default:
        Cu.reportError("Unknown Console API log level: " + level);
        return;
    }

    let node = ConsoleUtils.createMessageNode(this.chromeDocument,
                                              CATEGORY_WEBDEV,
                                              LEVELS[level],
                                              body,
                                              this.hudId,
                                              sourceURL,
                                              sourceLine,
                                              clipboardText,
                                              level,
                                              aMessage.timeStamp);

    
    
    if (level == "trace") {
      node._stacktrace = args;

      this.makeOutputMessageLink(node, function _traceNodeClickCallback() {
        if (node._panelOpen) {
          return;
        }

        let options = {
          anchor: node,
          data: { object: node._stacktrace },
        };

        let propPanel = this.jsterm.openPropertyPanel(options);
        propPanel.panel.setAttribute("hudId", this.hudId);
      }.bind(this));
    }

    if (level == "dir") {
      
      
      node._evalCacheId = aMessage.objectsCacheId;

      
      
      
      node._onOutput = function _onMessageOutput() {
        node.querySelector("tree").view = node.propertyTreeView;
      };
    }

    return node;
  },

  







  reportPageError: function HUD_reportPageError(aCategory, aScriptError)
  {
    
    
    let severity = SEVERITY_ERROR;
    if ((aScriptError.flags & aScriptError.warningFlag) ||
        (aScriptError.flags & aScriptError.strictFlag)) {
      severity = SEVERITY_WARNING;
    }

    let node = ConsoleUtils.createMessageNode(this.chromeDocument,
                                              aCategory,
                                              severity,
                                              aScriptError.errorMessage,
                                              this.hudId,
                                              aScriptError.sourceName,
                                              aScriptError.lineNumber,
                                              null,
                                              null,
                                              aScriptError.timeStamp);
    return node;
  },

  








  categoryForScriptError: function HUD_categoryForScriptError(aScriptError)
  {
    switch (aScriptError.category) {
      case "CSS Parser":
      case "CSS Loader":
        return CATEGORY_CSS;

      default:
        return CATEGORY_JS;
    }
  },

  







  logNetActivity: function HUD_logNetActivity(aConnectionId)
  {
    let networkInfo = this._networkRequests[aConnectionId];
    if (!networkInfo) {
      return;
    }

    let entry = networkInfo.httpActivity.log.entries[0];
    let request = entry.request;

    let msgNode = this.chromeDocument.createElementNS(XUL_NS, "hbox");

    let methodNode = this.chromeDocument.createElementNS(XUL_NS, "label");
    methodNode.setAttribute("value", request.method);
    methodNode.classList.add("webconsole-msg-body-piece");
    msgNode.appendChild(methodNode);

    let linkNode = this.chromeDocument.createElementNS(XUL_NS, "hbox");
    linkNode.setAttribute("flex", "1");
    linkNode.classList.add("webconsole-msg-body-piece");
    linkNode.classList.add("webconsole-msg-link");
    msgNode.appendChild(linkNode);

    let urlNode = this.chromeDocument.createElementNS(XUL_NS, "label");
    urlNode.setAttribute("crop", "center");
    urlNode.setAttribute("flex", "1");
    urlNode.setAttribute("title", request.url);
    urlNode.setAttribute("value", request.url);
    urlNode.classList.add("hud-clickable");
    urlNode.classList.add("webconsole-msg-body-piece");
    urlNode.classList.add("webconsole-msg-url");
    linkNode.appendChild(urlNode);

    let statusNode = this.chromeDocument.createElementNS(XUL_NS, "label");
    statusNode.setAttribute("value", "");
    statusNode.classList.add("hud-clickable");
    statusNode.classList.add("webconsole-msg-body-piece");
    statusNode.classList.add("webconsole-msg-status");
    linkNode.appendChild(statusNode);

    let clipboardText = request.method + " " + request.url;

    let messageNode = ConsoleUtils.createMessageNode(this.chromeDocument,
                                                     CATEGORY_NETWORK,
                                                     SEVERITY_LOG,
                                                     msgNode,
                                                     this.hudId,
                                                     null,
                                                     null,
                                                     clipboardText);

    messageNode._connectionId = entry.connection;

    this.makeOutputMessageLink(messageNode, function HUD_net_message_link() {
      if (!messageNode._panelOpen) {
        HUDService.openNetworkPanel(messageNode, networkInfo.httpActivity);
      }
    }.bind(this));

    networkInfo.node = messageNode;

    this._updateNetMessage(entry.connection);

    return messageNode;
  },

  







  logFileActivity: function HUD_logFileActivity(aFileURI)
  {
    let chromeDocument = this.chromeDocument;

    let urlNode = chromeDocument.createElementNS(XUL_NS, "label");
    urlNode.setAttribute("crop", "center");
    urlNode.setAttribute("flex", "1");
    urlNode.setAttribute("title", aFileURI);
    urlNode.setAttribute("value", aFileURI);
    urlNode.classList.add("hud-clickable");
    urlNode.classList.add("webconsole-msg-url");

    let outputNode = ConsoleUtils.createMessageNode(chromeDocument,
                                                    CATEGORY_NETWORK,
                                                    SEVERITY_LOG,
                                                    urlNode,
                                                    this.hudId,
                                                    null,
                                                    null,
                                                    aFileURI);

    this.makeOutputMessageLink(outputNode, function HUD__onFileClick() {
      let viewSourceUtils = chromeDocument.defaultView.gViewSourceUtils;
      viewSourceUtils.viewSource(aFileURI, null, chromeDocument);
    });

    return outputNode;
  },

  






  logWarningAboutReplacedAPI: function HUD_logWarningAboutReplacedAPI()
  {
    let message = l10n.getStr("ConsoleAPIDisabled");
    let node = ConsoleUtils.createMessageNode(this.chromeDocument, CATEGORY_JS,
                                              SEVERITY_WARNING, message,
                                              this.hudId);
    return node;
  },

  ERRORS: {
    HUD_BOX_DOES_NOT_EXIST: "Heads Up Display does not exist",
    TAB_ID_REQUIRED: "Tab DOM ID is required",
    PARENTNODE_NOT_FOUND: "parentNode element not found"
  },

  






  _setupMessageManager: function HUD__setupMessageManager()
  {
    this.messageManager.loadFrameScript(CONTENT_SCRIPT_URL, true);

    this._messageListeners.forEach(function(aName) {
      this.messageManager.addMessageListener(aName, this);
    }, this);

    let message = {
      features: ["ConsoleAPI", "JSTerm", "PageError", "NetworkMonitor",
                 "LocationChange"],
      cachedMessages: ["ConsoleAPI", "PageError"],
      NetworkMonitor: { monitorFileActivity: true },
      JSTerm: { notifyNonNativeConsoleAPI: true },
      preferences: {
        "NetworkMonitor.saveRequestAndResponseBodies":
          this.saveRequestAndResponseBodies,
      },
    };
    this.sendMessageToContent("WebConsole:Init", message);
  },

  






  receiveMessage: function HUD_receiveMessage(aMessage)
  {
    if (!aMessage.json || aMessage.json.hudId != this.hudId) {
      return;
    }

    switch (aMessage.name) {
      case "JSTerm:EvalResult":
      case "JSTerm:EvalObject":
      case "JSTerm:AutocompleteProperties":
        this._receiveMessageWithCallback(aMessage.json);
        break;
      case "JSTerm:ClearOutput":
        this.jsterm.clearOutput();
        break;
      case "JSTerm:InspectObject":
        this.jsterm.handleInspectObject(aMessage.json);
        break;
      case "WebConsole:ConsoleAPI":
        this.outputMessage(CATEGORY_WEBDEV, this.logConsoleAPIMessage,
                           [aMessage.json]);
        break;
      case "WebConsole:PageError": {
        let pageError = aMessage.json.pageError;
        let category = this.categoryForScriptError(pageError);
        this.outputMessage(category, this.reportPageError,
                           [category, pageError]);
        break;
      }
      case "WebConsole:CachedMessages":
        this._displayCachedConsoleMessages(aMessage.json.messages);
        this._onInitComplete();
        break;
      case "WebConsole:NetworkActivity":
        this.handleNetworkActivity(aMessage.json);
        break;
      case "WebConsole:FileActivity":
        this.outputMessage(CATEGORY_NETWORK, this.logFileActivity,
                           [aMessage.json.uri]);
        break;
      case "WebConsole:LocationChange":
        this.onLocationChange(aMessage.json);
        break;
      case "JSTerm:NonNativeConsoleAPI":
        this.outputMessage(CATEGORY_JS, this.logWarningAboutReplacedAPI);
        break;
    }
  },

  






  _onInitComplete: function HUD__onInitComplete()
  {
    let id = WebConsoleUtils.supportsString(this.hudId);
    Services.obs.notifyObservers(id, "web-console-created", null);
  },

  










  _receiveMessageWithCallback:
  function HUD__receiveMessageWithCallback(aResponse)
  {
    if (aResponse.id in this.asyncRequests) {
      let request = this.asyncRequests[aResponse.id];
      request.callback(aResponse, request.message);
      delete this.asyncRequests[aResponse.id];
    }
    else {
      Cu.reportError("receiveMessageWithCallback response for stale request " +
                     "ID " + aResponse.id);
    }
  },

  















  sendMessageToContent:
  function HUD_sendMessageToContent(aName, aMessage, aCallback)
  {
    aMessage.hudId = this.hudId;
    if (!("id" in aMessage)) {
      aMessage.id = "HUDChrome-" + HUDService.sequenceId();
    }

    if (aCallback) {
      this.asyncRequests[aMessage.id] = {
        name: aName,
        message: aMessage,
        callback: aCallback,
      };
    }
    this.messageManager.sendAsyncMessage(aName, aMessage);
  },

  













  handleNetworkActivity: function HUD_handleNetworkActivity(aMessage)
  {
    let stage = aMessage.meta.stages[aMessage.meta.stages.length - 1];
    let entry = aMessage.log.entries[0];

    if (stage == "REQUEST_HEADER") {
      let networkInfo = {
        node: null,
        httpActivity: aMessage,
      };

      this._networkRequests[entry.connection] = networkInfo;
      this.outputMessage(CATEGORY_NETWORK, this.logNetActivity,
                         [entry.connection]);
      return;
    }
    else if (!(entry.connection in this._networkRequests)) {
      return;
    }

    let networkInfo = this._networkRequests[entry.connection];
    networkInfo.httpActivity = aMessage;

    if (networkInfo.node) {
      this._updateNetMessage(entry.connection);
    }

    
    
    if (HUDService.lastFinishedRequestCallback &&
        aMessage.meta.stages.indexOf("REQUEST_STOP") > -1 &&
        aMessage.meta.stages.indexOf("TRANSACTION_CLOSE") > -1) {
      HUDService.lastFinishedRequestCallback(aMessage);
    }
  },

  







  _updateNetMessage: function HUD__updateNetMessage(aConnectionId)
  {
    let networkInfo = this._networkRequests[aConnectionId];
    if (!networkInfo || !networkInfo.node) {
      return;
    }

    let messageNode = networkInfo.node;
    let httpActivity = networkInfo.httpActivity;
    let stages = httpActivity.meta.stages;
    let hasTransactionClose = stages.indexOf("TRANSACTION_CLOSE") > -1;
    let hasResponseHeader = stages.indexOf("RESPONSE_HEADER") > -1;
    let entry = httpActivity.log.entries[0];
    let request = entry.request;
    let response = entry.response;

    if (hasTransactionClose || hasResponseHeader) {
      let status = [];
      if (response.httpVersion && response.status) {
        status = [response.httpVersion, response.status, response.statusText];
      }
      if (hasTransactionClose) {
        status.push(l10n.getFormatStr("NetworkPanel.durationMS", [entry.time]));
      }
      let statusText = "[" + status.join(" ") + "]";

      let linkNode = messageNode.querySelector(".webconsole-msg-link");
      let statusNode = linkNode.querySelector(".webconsole-msg-status");
      statusNode.setAttribute("value", statusText);

      messageNode.clipboardText = [request.method, request.url, statusText]
                                  .join(" ");

      if (hasResponseHeader && response.status >= MIN_HTTP_ERROR_CODE &&
          response.status <= MAX_HTTP_ERROR_CODE) {
        ConsoleUtils.setMessageType(messageNode, CATEGORY_NETWORK,
                                    SEVERITY_ERROR);
      }
    }

    if (messageNode._netPanel) {
      messageNode._netPanel.update();
    }
  },

  







  onLocationChange: function HUD_onLocationChange(aMessage)
  {
    this.contentLocation = aMessage.location;
    if (this.consolePanel) {
      this.consolePanel.label = this.getPanelTitle();
    }
  },

  








  makeOutputMessageLink: function HUD_makeOutputMessageLink(aNode, aCallback)
  {
    let linkNode;
    if (aNode.category === CATEGORY_NETWORK) {
      linkNode = aNode.querySelector(".webconsole-msg-link, .webconsole-msg-url");
    }
    else {
      linkNode = aNode.querySelector(".webconsole-msg-body");
      linkNode.classList.add("hud-clickable");
    }

    linkNode.setAttribute("aria-haspopup", "true");

    aNode.addEventListener("mousedown", function(aEvent) {
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    aNode.addEventListener("click", function(aEvent) {
      if (aEvent.detail != 1 || aEvent.button != 0 ||
          (this._startX != aEvent.clientX &&
           this._startY != aEvent.clientY)) {
        return;
      }

      aCallback(this, aEvent);
    }, false);
  },

  

















  outputMessage: function HUD_outputMessage(aCategory, aMethodOrNode, aArguments)
  {
    if (!this._outputQueue.length) {
      
      
      this._lastOutputFlush = Date.now();
    }

    this._outputQueue.push([aCategory, aMethodOrNode, aArguments]);

    if (!this._outputTimeout) {
      this._outputTimeout =
        this.chromeWindow.setTimeout(this._flushMessageQueue.bind(this),
                                     OUTPUT_INTERVAL);
    }
  },

  






  _flushMessageQueue: function HUD__flushMessageQueue()
  {
    let timeSinceFlush = Date.now() - this._lastOutputFlush;
    if (this._outputQueue.length > MESSAGES_IN_INTERVAL &&
        timeSinceFlush < THROTTLE_UPDATES) {
      this._outputTimeout =
        this.chromeWindow.setTimeout(this._flushMessageQueue.bind(this),
                                     OUTPUT_INTERVAL);
      return;
    }

    
    let toDisplay = Math.min(this._outputQueue.length, MESSAGES_IN_INTERVAL);
    if (toDisplay < 1) {
      this._outputTimeout = null;
      return;
    }

    
    let shouldPrune = false;
    if (this._outputQueue.length > toDisplay && this._pruneOutputQueue()) {
      toDisplay = Math.min(this._outputQueue.length, toDisplay);
      shouldPrune = true;
    }

    let batch = this._outputQueue.splice(0, toDisplay);
    if (!batch.length) {
      this._outputTimeout = null;
      return;
    }

    let outputNode = this.outputNode;
    let lastVisibleNode = null;
    let scrolledToBottom = ConsoleUtils.isOutputScrolledToBottom(outputNode);
    let scrollBox = outputNode.scrollBoxObject.element;

    let hudIdSupportsString = WebConsoleUtils.supportsString(this.hudId);

    
    for (let item of batch) {
      let node = this._outputMessageFromQueue(hudIdSupportsString, item);
      if (node) {
        lastVisibleNode = node;
      }
    }

    let oldScrollHeight = 0;

    
    
    let removedNodes = 0;
    if (shouldPrune || !this._outputQueue.length) {
      oldScrollHeight = scrollBox.scrollHeight;

      let categories = Object.keys(this._pruneCategoriesQueue);
      categories.forEach(function _pruneOutput(aCategory) {
        removedNodes += pruneConsoleOutputIfNecessary(this.hudId, aCategory);
      }, this);
      this._pruneCategoriesQueue = {};
    }

    
    if (!this._outputQueue.length) {
      HUDService.regroupOutput(outputNode);
    }

    let isInputOutput = lastVisibleNode &&
      (lastVisibleNode.classList.contains("webconsole-msg-input") ||
       lastVisibleNode.classList.contains("webconsole-msg-output"));

    
    
    
    if (lastVisibleNode && (scrolledToBottom || isInputOutput)) {
      ConsoleUtils.scrollToVisible(lastVisibleNode);
    }
    else if (!scrolledToBottom && removedNodes > 0 &&
             oldScrollHeight != scrollBox.scrollHeight) {
      
      
      scrollBox.scrollTop -= oldScrollHeight - scrollBox.scrollHeight;
    }

    
    if (this._outputQueue.length > 0) {
      this._outputTimeout =
        this.chromeWindow.setTimeout(this._flushMessageQueue.bind(this),
                                     OUTPUT_INTERVAL);
    }
    else {
      this._outputTimeout = null;
    }

    this._lastOutputFlush = Date.now();
  },

  











  _outputMessageFromQueue:
  function HUD__outputMessageFromQueue(aHudIdSupportsString, aItem)
  {
    let [category, methodOrNode, args] = aItem;

    let node = typeof methodOrNode == "function" ?
               methodOrNode.apply(this, args || []) :
               methodOrNode;
    if (!node) {
      return;
    }

    let afterNode = node._outputAfterNode;
    if (afterNode) {
      delete node._outputAfterNode;
    }

    let isFiltered = ConsoleUtils.filterMessageNode(node, this.hudId);

    let isRepeated = false;
    if (node.classList.contains("webconsole-msg-cssparser")) {
      isRepeated = ConsoleUtils.filterRepeatedCSS(node, this.outputNode,
                                                  this.hudId);
    }

    if (!isRepeated &&
        !node.classList.contains("webconsole-msg-network") &&
        (node.classList.contains("webconsole-msg-console") ||
         node.classList.contains("webconsole-msg-exception") ||
         node.classList.contains("webconsole-msg-error"))) {
      isRepeated = ConsoleUtils.filterRepeatedConsole(node, this.outputNode);
    }

    let lastVisible = !isRepeated && !isFiltered;
    if (!isRepeated) {
      this.outputNode.insertBefore(node,
                                   afterNode ? afterNode.nextSibling : null);
      this._pruneCategoriesQueue[node.category] = true;
      if (afterNode) {
        lastVisible = this.outputNode.lastChild == node;
      }
    }

    if (node._onOutput) {
      node._onOutput();
      delete node._onOutput;
    }

    let nodeID = node.getAttribute("id");
    Services.obs.notifyObservers(aHudIdSupportsString,
                                 "web-console-message-created", nodeID);

    return lastVisible ? node : null;
  },

  




  _pruneOutputQueue: function HUD__pruneOutputQueue()
  {
    let nodes = {};

    
    this._outputQueue.forEach(function(aItem, aIndex) {
      let [category] = aItem;
      if (!(category in nodes)) {
        nodes[category] = [];
      }
      nodes[category].push(aIndex);
    }, this);

    let pruned = 0;

    
    for (let category in nodes) {
      let limit = this.logLimitForCategory(category);
      let indexes = nodes[category];
      if (indexes.length > limit) {
        let n = Math.max(0, indexes.length - limit);
        pruned += n;
        for (let i = n - 1; i >= 0; i--) {
          this._pruneItemFromQueue(this._outputQueue[indexes[i]]);
          this._outputQueue.splice(indexes[i], 1);
        }
      }
    }

    return pruned;
  },

  






  _pruneItemFromQueue: function HUD__pruneItemFromQueue(aItem)
  {
    let [category, methodOrNode, args] = aItem;
    if (typeof methodOrNode != "function" &&
        methodOrNode._evalCacheId && !methodOrNode._panelOpen) {
      this.jsterm.clearObjectCache(methodOrNode._evalCacheId);
    }

    if (category == CATEGORY_NETWORK) {
      let connectionId = null;
      if (methodOrNode == this.logNetActivity) {
        connectionId = args[0];
      }
      else if (typeof methodOrNode != "function") {
        connectionId = methodOrNode._connectionId;
      }
      if (connectionId && connectionId in this._networkRequests) {
        delete this._networkRequests[connectionId];
      }
    }
    else if (category == CATEGORY_WEBDEV &&
             methodOrNode == this.logConsoleAPIMessage) {
      let level = args[0].apiMessage.level;
      if (level == "dir") {
        this.jsterm.clearObjectCache(args[0].objectsCacheId);
      }
    }
  },

  








  logLimitForCategory: function HUD_logLimitForCategory(aCategory)
  {
    let logLimit = DEFAULT_LOG_LIMIT;

    try {
      let prefName = CATEGORY_CLASS_FRAGMENTS[aCategory];
      logLimit = Services.prefs.getIntPref("devtools.hud.loglimit." + prefName);
      logLimit = Math.max(logLimit, 1);
    }
    catch (e) { }

    return logLimit;
  },

  





  removeOutputMessage: function HUD_removeOutputMessage(aNode)
  {
    if (aNode._evalCacheId && !aNode._panelOpen) {
      this.jsterm.clearObjectCache(aNode._evalCacheId);
    }

    if (aNode.classList.contains("webconsole-msg-cssparser")) {
      let desc = aNode.childNodes[2].textContent;
      let location = "";
      if (aNode.childNodes[4]) {
        location = aNode.childNodes[4].getAttribute("title");
      }
      delete this.cssNodes[desc + location];
    }
    else if (aNode.classList.contains("webconsole-msg-network")) {
      delete this._networkRequests[aNode._connectionId];
    }
    else if (aNode.classList.contains("webconsole-msg-inspector")) {
      this.pruneConsoleDirNode(aNode);
      return;
    }

    if (aNode.parentNode) {
      aNode.parentNode.removeChild(aNode);
    }
  },

  



  destroy: function HUD_destroy()
  {
    this.sendMessageToContent("WebConsole:Destroy", {});

    this._messageListeners.forEach(function(aName) {
      this.messageManager.removeMessageListener(aName, this);
    }, this);

    if (this.jsterm) {
      this.jsterm.destroy();
    }

    
    
    this.consoleWindowUnregisterOnHide = false;

    let popupset = this.chromeDocument.getElementById("mainPopupSet");
    let panels = popupset.querySelectorAll("panel[hudId=" + this.hudId + "]");
    for (let panel of panels) {
      if (panel != this.consolePanel) {
        panel.hidePopup();
      }
    }

    
    
    if (this.consolePanel && this.consolePanel.parentNode) {
      this.consolePanel.hidePopup();
      this.consolePanel.parentNode.removeChild(this.consolePanel);
      this.consolePanel = null;
    }

    if (this.HUDBox.parentNode) {
      this.HUDBox.parentNode.removeChild(this.HUDBox);
    }

    if (this.splitter.parentNode) {
      this.splitter.parentNode.removeChild(this.splitter);
    }

    delete this.asyncRequests;
    delete this.messageManager;
    delete this.browser;
    delete this.chromeDocument;
    delete this.chromeWindow;
    delete this.outputNode;

    this.positionMenuitems.above.removeEventListener("command",
      this._positionConsoleAbove, false);
    this.positionMenuitems.below.removeEventListener("command",
      this._positionConsoleBelow, false);
    this.positionMenuitems.window.removeEventListener("command",
      this._positionConsoleWindow, false);

    this.closeButton.removeEventListener("command",
      this.closeButtonOnCommand, false);
  },
};









function NodeFactory(aFactoryType, ignored, aDocument)
{
  
  if (aFactoryType == "text") {
    return function factory(aText)
    {
      return aDocument.createTextNode(aText);
    }
  }
  else if (aFactoryType == "xul") {
    return function factory(aTag)
    {
      return aDocument.createElement(aTag);
    }
  }
  else {
    throw new Error('NodeFactory: Unknown factory type: ' + aFactoryType);
  }
}











function JSTerm(aHud)
{
  this.hud = aHud;
  this.document = this.hud.chromeDocument;
  this.parentNode = this.hud.jsTermParentNode;

  this.hudId = this.hud.hudId;

  this.lastCompletion = { value: null };
  this.history = [];
  this.historyIndex = 0;
  this.historyPlaceHolder = 0;  
  this.autocompletePopup = new AutocompletePopup(this.document);
  this.autocompletePopup.onSelect = this.onAutocompleteSelect.bind(this);
  this.autocompletePopup.onClick = this.acceptProposedCompletion.bind(this);
  this.init();
}

JSTerm.prototype = {
  lastInputValue: "",
  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,

  


  init: function JST_init()
  {
    this._generateUI();

    this._keyPress = this.keyPress.bind(this);
    this._inputEventHandler = this.inputEventHandler.bind(this);

    this.inputNode.addEventListener("keypress", this._keyPress, false);
    this.inputNode.addEventListener("input", this._inputEventHandler, false);
    this.inputNode.addEventListener("keyup", this._inputEventHandler, false);
  },

  



  _generateUI: function JST__generateUI()
  {
    this.completeNode = this.hud.makeXULNode("textbox");
    this.completeNode.setAttribute("class", "jsterm-complete-node");
    this.completeNode.setAttribute("multiline", "true");
    this.completeNode.setAttribute("rows", "1");
    this.completeNode.setAttribute("tabindex", "-1");

    this.inputNode = this.hud.makeXULNode("textbox");
    this.inputNode.setAttribute("class", "jsterm-input-node");
    this.inputNode.setAttribute("multiline", "true");
    this.inputNode.setAttribute("rows", "1");

    let inputStack = this.hud.makeXULNode("stack");
    inputStack.setAttribute("class", "jsterm-stack-node");
    inputStack.setAttribute("flex", "1");
    inputStack.appendChild(this.completeNode);
    inputStack.appendChild(this.inputNode);

    let term = this.hud.makeXULNode("hbox");
    term.setAttribute("class", "jsterm-input-container");
    term.setAttribute("style", "direction: ltr;");
    term.appendChild(inputStack);

    this.parentNode.appendChild(term);
  },

  get outputNode() this.hud.outputNode,

  








  evalInContentSandbox: function JST_evalInContentSandbox(aString, aCallback)
  {
    let message = {
      str: aString,
      resultCacheId: "HUDEval-" + HUDService.sequenceId(),
    };

    this.hud.sendMessageToContent("JSTerm:EvalRequest", message, aCallback);

    return message;
  },

  













  _executeResultCallback:
  function JST__executeResultCallback(aResponse, aRequest)
  {
    let errorMessage = aResponse.errorMessage;
    let resultString = aResponse.resultString;

    
    if (!errorMessage &&
        resultString == "undefined" &&
        aResponse.helperResult &&
        !aResponse.inspectable &&
        !aResponse.helperRawOutput) {
      return;
    }

    let afterNode = aRequest.outputNode;

    if (aResponse.errorMessage) {
      this.writeOutput(aResponse.errorMessage, CATEGORY_OUTPUT, SEVERITY_ERROR,
                       afterNode, aResponse.timestamp);
    }
    else if (aResponse.inspectable) {
      let node = this.writeOutputJS(aResponse.resultString,
                                    this._evalOutputClick.bind(this, aResponse),
                                    afterNode, aResponse.timestamp);
      node._evalCacheId = aResponse.childrenCacheId;
    }
    else {
      this.writeOutput(aResponse.resultString, CATEGORY_OUTPUT, SEVERITY_LOG,
                       afterNode, aResponse.timestamp);
    }
  },

  






  execute: function JST_execute(aExecuteString)
  {
    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      this.writeOutput("no value to execute", CATEGORY_OUTPUT, SEVERITY_LOG);
      return;
    }

    let node = this.writeOutput(aExecuteString, CATEGORY_INPUT, SEVERITY_LOG);

    let messageToContent =
      this.evalInContentSandbox(aExecuteString,
                                this._executeResultCallback.bind(this));
    messageToContent.outputNode = node;

    this.history.push(aExecuteString);
    this.historyIndex++;
    this.historyPlaceHolder = this.history.length;
    this.setInputValue("");
    this.clearCompletion();
  },

  





















  openPropertyPanel: function JST_openPropertyPanel(aOptions)
  {
    
    
    
    let buttons = [];

    if (aOptions.updateButtonCallback) {
      buttons.push({
        label: l10n.getStr("update.button"),
        accesskey: l10n.getStr("update.accesskey"),
        oncommand: aOptions.updateButtonCallback,
      });
    }

    let parent = this.document.getElementById("mainPopupSet");
    let title = aOptions.title ?
                l10n.getFormatStr("jsPropertyInspectTitle", [aOptions.title]) :
                l10n.getStr("jsPropertyTitle");

    let propPanel = new PropertyPanel(parent, title, aOptions.data, buttons);

    propPanel.panel.openPopup(aOptions.anchor, "after_pointer", 0, 0, false, false);
    propPanel.panel.sizeTo(350, 450);

    if (aOptions.anchor) {
      propPanel.panel.addEventListener("popuphiding", function onPopupHide() {
        propPanel.panel.removeEventListener("popuphiding", onPopupHide, false);
        aOptions.anchor._panelOpen = false;
      }, false);
      aOptions.anchor._panelOpen = true;
    }

    return propPanel;
  },

  

















  writeOutputJS:
  function JST_writeOutputJS(aOutputMessage, aCallback, aNodeAfter, aTimestamp)
  {
    let node = this.writeOutput(aOutputMessage, CATEGORY_OUTPUT, SEVERITY_LOG,
                                aNodeAfter, aTimestamp);
    if (aCallback) {
      this.hud.makeOutputMessageLink(node, aCallback);
    }
    return node;
  },

  




















  writeOutput:
  function JST_writeOutput(aOutputMessage, aCategory, aSeverity, aNodeAfter,
                           aTimestamp)
  {
    let node = ConsoleUtils.createMessageNode(this.document, aCategory,
                                              aSeverity, aOutputMessage,
                                              this.hudId, null, null, null,
                                              null, aTimestamp);
    node._outputAfterNode = aNodeAfter;
    this.hud.outputMessage(aCategory, node);
    return node;
  },

  






  clearOutput: function JST_clearOutput(aClearStorage)
  {
    let hud = this.hud;
    let outputNode = hud.outputNode;
    let node;
    while ((node = outputNode.firstChild)) {
      hud.removeOutputMessage(node);
    }

    hud.HUDBox.lastTimestamp = 0;
    hud.groupDepth = 0;
    hud._outputQueue.forEach(hud._pruneItemFromQueue, hud);
    hud._outputQueue = [];
    hud._networkRequests = {};
    hud.cssNodes = {};

    if (aClearStorage) {
      hud.sendMessageToContent("ConsoleAPI:ClearCache", {});
    }
  },

  




  resizeInput: function JST_resizeInput()
  {
    let inputNode = this.inputNode;

    
    
    inputNode.style.height = "auto";

    
    let scrollHeight = inputNode.inputField.scrollHeight;
    if (scrollHeight > 0) {
      inputNode.style.height = scrollHeight + "px";
    }
  },

  








  setInputValue: function JST_setInputValue(aNewValue)
  {
    this.inputNode.value = aNewValue;
    this.lastInputValue = aNewValue;
    this.completeNode.value = "";
    this.resizeInput();
  },

  




  inputEventHandler: function JSTF_inputEventHandler(aEvent)
  {
    if (this.lastInputValue != this.inputNode.value) {
      this.resizeInput();
      this.complete(this.COMPLETE_HINT_ONLY);
      this.lastInputValue = this.inputNode.value;
    }
  },

  




  keyPress: function JSTF_keyPress(aEvent)
  {
    if (aEvent.ctrlKey) {
      switch (aEvent.charCode) {
        case 97:
          
          this.inputNode.setSelectionRange(0, 0);
          aEvent.preventDefault();
          break;
        case 101:
          
          this.inputNode.setSelectionRange(this.inputNode.value.length,
                                           this.inputNode.value.length);
          aEvent.preventDefault();
          break;
        default:
          break;
      }
      return;
    }
    else if (aEvent.shiftKey &&
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN) {
      
      
      return;
    }

    let inputUpdated = false;

    switch(aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE:
        if (this.autocompletePopup.isOpen) {
          this.clearCompletion();
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_RETURN:
        if (this.autocompletePopup.isOpen && this.autocompletePopup.selectedIndex > -1) {
          this.acceptProposedCompletion();
        }
        else {
          this.execute();
        }
        aEvent.preventDefault();
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_UP:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_BACKWARD);
        }
        else if (this.canCaretGoPrevious()) {
          inputUpdated = this.historyPeruse(HISTORY_BACK);
        }
        if (inputUpdated) {
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_DOWN:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_FORWARD);
        }
        else if (this.canCaretGoNext()) {
          inputUpdated = this.historyPeruse(HISTORY_FORWARD);
        }
        if (inputUpdated) {
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_TAB:
        
        if (this.complete(this.COMPLETE_HINT_ONLY) &&
            this.lastCompletion &&
            this.acceptProposedCompletion()) {
          aEvent.preventDefault();
        }
        else {
          this.updateCompleteNode(l10n.getStr("Autocomplete.blank"));
          aEvent.preventDefault();
        }
        break;

      default:
        break;
    }
  },

  








  historyPeruse: function JST_historyPeruse(aDirection)
  {
    if (!this.history.length) {
      return false;
    }

    
    if (aDirection == HISTORY_BACK) {
      if (this.historyPlaceHolder <= 0) {
        return false;
      }

      let inputVal = this.history[--this.historyPlaceHolder];
      if (inputVal){
        this.setInputValue(inputVal);
      }
    }
    
    else if (aDirection == HISTORY_FORWARD) {
      if (this.historyPlaceHolder == this.history.length - 1) {
        this.historyPlaceHolder ++;
        this.setInputValue("");
      }
      else if (this.historyPlaceHolder >= (this.history.length)) {
        return false;
      }
      else {
        let inputVal = this.history[++this.historyPlaceHolder];
        if (inputVal){
          this.setInputValue(inputVal);
        }
      }
    }
    else {
      throw new Error("Invalid argument 0");
    }

    return true;
  },

  








  canCaretGoPrevious: function JST_canCaretGoPrevious()
  {
    let node = this.inputNode;
    if (node.selectionStart != node.selectionEnd) {
      return false;
    }

    let multiline = /[\r\n]/.test(node.value);
    return node.selectionStart == 0 ? true :
           node.selectionStart == node.value.length && !multiline;
  },

  








  canCaretGoNext: function JST_canCaretGoNext()
  {
    let node = this.inputNode;
    if (node.selectionStart != node.selectionEnd) {
      return false;
    }

    let multiline = /[\r\n]/.test(node.value);
    return node.selectionStart == node.value.length ? true :
           node.selectionStart == 0 && !multiline;
  },

  history: null,

  
  lastCompletion: null,

  




























  complete: function JSTF_complete(aType, aCallback)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    
    if (!inputValue) {
      this.clearCompletion();
      return false;
    }

    
    if (inputNode.selectionStart == inputNode.selectionEnd &&
        inputNode.selectionEnd != inputValue.length) {
      this.clearCompletion();
      return false;
    }

    
    if (this.lastCompletion.value != inputValue) {
      this._updateCompletionResult(aType, aCallback);
      return false;
    }

    let popup = this.autocompletePopup;
    let accepted = false;

    if (aType != this.COMPLETE_HINT_ONLY && popup.itemCount == 1) {
      this.acceptProposedCompletion();
      accepted = true;
    }
    else if (aType == this.COMPLETE_BACKWARD) {
      popup.selectPreviousItem();
    }
    else if (aType == this.COMPLETE_FORWARD) {
      popup.selectNextItem();
    }

    aCallback && aCallback(this);
    return accepted || popup.itemCount > 0;
  },

  









  _updateCompletionResult:
  function JST__updateCompletionResult(aType, aCallback)
  {
    if (this.lastCompletion.value == this.inputNode.value) {
      return;
    }

    let message = {
      id: "HUDComplete-" + HUDService.sequenceId(),
      input: this.inputNode.value,
    };

    this.lastCompletion = {
      requestId: message.id,
      completionType: aType,
      value: null,
    };
    let callback = this._receiveAutocompleteProperties.bind(this, aCallback);
    this.hud.sendMessageToContent("JSTerm:Autocomplete", message, callback);
  },

  










  _receiveAutocompleteProperties:
  function JST__receiveAutocompleteProperties(aCallback, aMessage)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    if (aMessage.input != inputValue ||
        this.lastCompletion.value == inputValue ||
        aMessage.id != this.lastCompletion.requestId) {
      return;
    }

    let matches = aMessage.matches;
    if (!matches.length) {
      this.clearCompletion();
      return;
    }

    let items = matches.map(function(aMatch) {
      return { label: aMatch };
    });

    let popup = this.autocompletePopup;
    popup.setItems(items);

    let completionType = this.lastCompletion.completionType;
    this.lastCompletion = {
      value: inputValue,
      matchProp: aMessage.matchProp,
    };

    if (items.length > 1 && !popup.isOpen) {
      popup.openPopup(inputNode);
    }
    else if (items.length < 2 && popup.isOpen) {
      popup.hidePopup();
    }

    if (items.length == 1) {
      popup.selectedIndex = 0;
    }

    this.onAutocompleteSelect();

    if (completionType != this.COMPLETE_HINT_ONLY && popup.itemCount == 1) {
      this.acceptProposedCompletion();
    }
    else if (completionType == this.COMPLETE_BACKWARD) {
      popup.selectPreviousItem();
    }
    else if (completionType == this.COMPLETE_FORWARD) {
      popup.selectNextItem();
    }

    aCallback && aCallback(this);
  },

  onAutocompleteSelect: function JSTF_onAutocompleteSelect()
  {
    let currentItem = this.autocompletePopup.selectedItem;
    if (currentItem && this.lastCompletion.value) {
      let suffix = currentItem.label.substring(this.lastCompletion.
                                               matchProp.length);
      this.updateCompleteNode(suffix);
    }
    else {
      this.updateCompleteNode("");
    }
  },

  



  clearCompletion: function JSTF_clearCompletion()
  {
    this.autocompletePopup.clearItems();
    this.lastCompletion = { value: null };
    this.updateCompleteNode("");
    if (this.autocompletePopup.isOpen) {
      this.autocompletePopup.hidePopup();
    }
  },

  






  acceptProposedCompletion: function JSTF_acceptProposedCompletion()
  {
    let updated = false;

    let currentItem = this.autocompletePopup.selectedItem;
    if (currentItem && this.lastCompletion.value) {
      let suffix = currentItem.label.substring(this.lastCompletion.
                                               matchProp.length);
      this.setInputValue(this.inputNode.value + suffix);
      updated = true;
    }

    this.clearCompletion();

    return updated;
  },

  





  updateCompleteNode: function JSTF_updateCompleteNode(aSuffix)
  {
    
    let prefix = aSuffix ? this.inputNode.value.replace(/[\S]/g, " ") : "";
    this.completeNode.value = prefix + aSuffix;
  },

  






  clearObjectCache: function JST_clearObjectCache(aCacheId)
  {
    if (this.hud) {
      this.hud.sendMessageToContent("JSTerm:ClearObjectCache",
                                    { cacheId: aCacheId });
    }
  },

  














  remoteObjectProvider:
  function JST_remoteObjectProvider(aCacheId, aObjectId, aResultCacheId,
                                    aCallback) {
    let message = {
      cacheId: aCacheId,
      objectId: aObjectId,
      resultCacheId: aResultCacheId,
    };

    this.hud.sendMessageToContent("JSTerm:GetEvalObject", message, aCallback);
  },

  








  handleInspectObject: function JST_handleInspectObject(aRequest)
  {
    let options = {
      title: aRequest.input,

      data: {
        rootCacheId: aRequest.objectCacheId,
        panelCacheId: aRequest.objectCacheId,
        remoteObject: aRequest.resultObject,
        remoteObjectProvider: this.remoteObjectProvider.bind(this),
      },
    };

    let propPanel = this.openPropertyPanel(options);
    propPanel.panel.setAttribute("hudId", this.hudId);

    let onPopupHide = function JST__onPopupHide() {
      propPanel.panel.removeEventListener("popuphiding", onPopupHide, false);

      this.clearObjectCache(options.data.panelCacheId);
    }.bind(this);

    propPanel.panel.addEventListener("popuphiding", onPopupHide, false);
  },

  








  _evalOutputClick: function JST__evalOutputClick(aResponse, aLinkNode)
  {
    if (aLinkNode._panelOpen) {
      return;
    }

    let options = {
      title: aResponse.input,
      anchor: aLinkNode,

      
      data: {
        
        rootCacheId: aResponse.childrenCacheId,
        remoteObject: aResponse.resultObject,
        
        panelCacheId: "HUDPanel-" + HUDService.sequenceId(),
        remoteObjectProvider: this.remoteObjectProvider.bind(this),
      },
    };

    options.updateButtonCallback = function JST__evalUpdateButton() {
      this.evalInContentSandbox(aResponse.input,
        this._evalOutputUpdatePanelCallback.bind(this, options, propPanel,
                                                 aResponse));
    }.bind(this);

    let propPanel = this.openPropertyPanel(options);
    propPanel.panel.setAttribute("hudId", this.hudId);

    let onPopupHide = function JST__evalInspectPopupHide() {
      propPanel.panel.removeEventListener("popuphiding", onPopupHide, false);

      this.clearObjectCache(options.data.panelCacheId);

      if (!aLinkNode.parentNode && aLinkNode._evalCacheId) {
        this.clearObjectCache(aLinkNode._evalCacheId);
      }
    }.bind(this);

    propPanel.panel.addEventListener("popuphiding", onPopupHide, false);
  },

  















  _evalOutputUpdatePanelCallback:
  function JST__updatePanelCallback(aOptions, aPropPanel, aOldResponse,
                                    aNewResponse)
  {
    if (aNewResponse.errorMessage) {
      this.writeOutput(aNewResponse.errorMessage, CATEGORY_OUTPUT,
                       SEVERITY_ERROR);
      return;
    }

    if (!aNewResponse.inspectable) {
      this.writeOutput(l10n.getStr("JSTerm.updateNotInspectable"), CATEGORY_OUTPUT, SEVERITY_ERROR);
      return;
    }

    this.clearObjectCache(aOptions.data.panelCacheId);
    this.clearObjectCache(aOptions.data.rootCacheId);

    if (aOptions.anchor && aOptions.anchor._evalCacheId) {
      aOptions.anchor._evalCacheId = aNewResponse.childrenCacheId;
    }

    
    
    aOldResponse.id = aNewResponse.id;
    aOldResponse.childrenCacheId = aNewResponse.childrenCacheId;
    aOldResponse.resultObject = aNewResponse.resultObject;
    aOldResponse.resultString = aNewResponse.resultString;

    aOptions.data.rootCacheId = aNewResponse.childrenCacheId;
    aOptions.data.remoteObject = aNewResponse.resultObject;

    
    
    
    aPropPanel.treeView.data = aOptions.data;
  },

  


  destroy: function JST_destroy()
  {
    this.clearCompletion();
    this.clearOutput();

    this.autocompletePopup.destroy();

    this.inputNode.removeEventListener("keypress", this._keyPress, false);
    this.inputNode.removeEventListener("input", this._inputEventHandler, false);
    this.inputNode.removeEventListener("keyup", this._inputEventHandler, false);

    delete this.history;
    delete this.hud;
    delete this.autocompletePopup;
    delete this.document;
  },
};









ConsoleUtils = {
  


  scroll: true,

  







  scrollToVisible: function ConsoleUtils_scrollToVisible(aNode) {
    if (!this.scroll) {
      return;
    }

    
    let richListBoxNode = aNode.parentNode;
    while (richListBoxNode.tagName != "richlistbox") {
      richListBoxNode = richListBoxNode.parentNode;
    }

    
    let boxObject = richListBoxNode.scrollBoxObject;
    let nsIScrollBoxObject = boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    nsIScrollBoxObject.ensureElementIsVisible(aNode);
  },

  































  createMessageNode:
  function ConsoleUtils_createMessageNode(aDocument, aCategory, aSeverity,
                                          aBody, aHUDId, aSourceURL,
                                          aSourceLine, aClipboardText, aLevel,
                                          aTimeStamp) {
    if (typeof aBody != "string" && aClipboardText == null && aBody.innerText) {
      aClipboardText = aBody.innerText;
    }

    
    
    
    let iconContainer = aDocument.createElementNS(XUL_NS, "vbox");
    iconContainer.classList.add("webconsole-msg-icon-container");
    
    let hud = HUDService.getHudReferenceById(aHUDId);
    iconContainer.style.marginLeft = hud.groupDepth * GROUP_INDENT + "px";

    
    
    let iconNode = aDocument.createElementNS(XUL_NS, "image");
    iconNode.classList.add("webconsole-msg-icon");
    iconContainer.appendChild(iconNode);

    
    let spacer = aDocument.createElementNS(XUL_NS, "spacer");
    spacer.setAttribute("flex", "1");
    iconContainer.appendChild(spacer);

    
    let bodyNode = aDocument.createElementNS(XUL_NS, "description");
    bodyNode.setAttribute("flex", "1");
    bodyNode.classList.add("webconsole-msg-body");

    
    
    let body = aBody;
    
    
    aClipboardText = aClipboardText ||
                     (aBody + (aSourceURL ? " @ " + aSourceURL : "") +
                              (aSourceLine ? ":" + aSourceLine : ""));
    if (!(aBody instanceof Ci.nsIDOMNode)) {
      aBody = aDocument.createTextNode(aLevel == "dir" ?
                                       aBody.resultString : aBody);
    }

    if (!aBody.nodeType) {
      aBody = aDocument.createTextNode(aBody.toString());
    }
    if (typeof aBody == "string") {
      aBody = aDocument.createTextNode(aBody);
    }

    bodyNode.appendChild(aBody);

    let repeatContainer = aDocument.createElementNS(XUL_NS, "hbox");
    repeatContainer.setAttribute("align", "start");
    let repeatNode = aDocument.createElementNS(XUL_NS, "label");
    repeatNode.setAttribute("value", "1");
    repeatNode.classList.add("webconsole-msg-repeat");
    repeatContainer.appendChild(repeatNode);

    
    let timestampNode = aDocument.createElementNS(XUL_NS, "label");
    timestampNode.classList.add("webconsole-timestamp");
    let timestamp = aTimeStamp || Date.now();
    let timestampString = l10n.timestampString(timestamp);
    timestampNode.setAttribute("value", timestampString);

    
    
    let locationNode;
    if (aSourceURL) {
      locationNode = this.createLocationNode(aDocument, aSourceURL,
                                             aSourceLine);
    }

    
    let node = aDocument.createElementNS(XUL_NS, "richlistitem");
    node.clipboardText = aClipboardText;
    node.classList.add("hud-msg-node");

    node.timestamp = timestamp;
    ConsoleUtils.setMessageType(node, aCategory, aSeverity);

    node.appendChild(timestampNode);
    node.appendChild(iconContainer);
    
    if (aLevel == "dir") {
      
      
      let bodyContainer = aDocument.createElement("vbox");
      bodyContainer.setAttribute("flex", "1");
      bodyContainer.appendChild(bodyNode);
      
      let tree = createElement(aDocument, "tree", {
        flex: 1,
        hidecolumnpicker: "true"
      });

      let treecols = aDocument.createElement("treecols");
      let treecol = createElement(aDocument, "treecol", {
        primary: "true",
        flex: 1,
        hideheader: "true",
        ignoreincolumnpicker: "true"
      });
      treecols.appendChild(treecol);
      tree.appendChild(treecols);

      tree.appendChild(aDocument.createElement("treechildren"));

      bodyContainer.appendChild(tree);
      node.appendChild(bodyContainer);
      node.classList.add("webconsole-msg-inspector");
      
      let treeView = node.propertyTreeView = new PropertyTreeView();

      treeView.data = {
        rootCacheId: body.cacheId,
        panelCacheId: body.cacheId,
        remoteObject: body.remoteObject,
        remoteObjectProvider: body.remoteObjectProvider,
      };

      tree.setAttribute("rows", treeView.rowCount);
    }
    else {
      node.appendChild(bodyNode);
    }
    node.appendChild(repeatContainer);
    if (locationNode) {
      node.appendChild(locationNode);
    }

    node.setAttribute("id", "console-msg-" + HUDService.sequenceId());

    return node;
  },

  











  setMessageType:
  function ConsoleUtils_setMessageType(aMessageNode, aNewCategory,
                                       aNewSeverity) {
    
    if ("category" in aMessageNode) {
      let oldCategory = aMessageNode.category;
      let oldSeverity = aMessageNode.severity;
      aMessageNode.classList.remove("webconsole-msg-" +
                                    CATEGORY_CLASS_FRAGMENTS[oldCategory]);
      aMessageNode.classList.remove("webconsole-msg-" +
                                    SEVERITY_CLASS_FRAGMENTS[oldSeverity]);
      let key = "hud-" + MESSAGE_PREFERENCE_KEYS[oldCategory][oldSeverity];
      aMessageNode.classList.remove(key);
    }

    
    aMessageNode.category = aNewCategory;
    aMessageNode.severity = aNewSeverity;
    aMessageNode.classList.add("webconsole-msg-" +
                               CATEGORY_CLASS_FRAGMENTS[aNewCategory]);
    aMessageNode.classList.add("webconsole-msg-" +
                               SEVERITY_CLASS_FRAGMENTS[aNewSeverity]);
    let key = "hud-" + MESSAGE_PREFERENCE_KEYS[aNewCategory][aNewSeverity];
    aMessageNode.classList.add(key);
  },

  













  createLocationNode:
  function ConsoleUtils_createLocationNode(aDocument, aSourceURL,
                                           aSourceLine) {
    let locationNode = aDocument.createElementNS(XUL_NS, "label");

    
    
    let text = WebConsoleUtils.abbreviateSourceURL(aSourceURL);
    if (aSourceLine) {
      text += ":" + aSourceLine;
    }
    locationNode.setAttribute("value", text);

    
    locationNode.setAttribute("crop", "center");
    locationNode.setAttribute("title", aSourceURL);
    locationNode.classList.add("webconsole-location");
    locationNode.classList.add("text-link");

    
    locationNode.addEventListener("click", function() {
      if (aSourceURL == "Scratchpad") {
        let win = Services.wm.getMostRecentWindow("devtools:scratchpad");
        if (win) {
          win.focus();
        }
        return;
      }
      let viewSourceUtils = aDocument.defaultView.gViewSourceUtils;
      viewSourceUtils.viewSource(aSourceURL, null, aDocument, aSourceLine);
    }, true);

    return locationNode;
  },

  










  filterMessageNode: function ConsoleUtils_filterMessageNode(aNode, aHUDId) {
    let isFiltered = false;

    
    let prefKey = MESSAGE_PREFERENCE_KEYS[aNode.category][aNode.severity];
    if (prefKey && !HUDService.getFilterState(aHUDId, prefKey)) {
      
      aNode.classList.add("hud-filtered-by-type");
      isFiltered = true;
    }

    
    let search = HUDService.getFilterStringByHUDId(aHUDId);
    let text = aNode.clipboardText;

    
    if (!HUDService.stringMatchesFilters(text, search)) {
      aNode.classList.add("hud-filtered-by-string");
      isFiltered = true;
    }

    return isFiltered;
  },

  








  mergeFilteredMessageNode:
  function ConsoleUtils_mergeFilteredMessageNode(aOriginal, aFiltered) {
    
    
    let repeatNode = aOriginal.childNodes[3].firstChild;
    if (!repeatNode) {
      return aOriginal; 
    }

    let occurrences = parseInt(repeatNode.getAttribute("value")) + 1;
    repeatNode.setAttribute("value", occurrences);
  },

  










  filterRepeatedCSS:
  function ConsoleUtils_filterRepeatedCSS(aNode, aOutput, aHUDId) {
    let hud = HUDService.getHudReferenceById(aHUDId);

    
    let description = aNode.childNodes[2].textContent;
    let location;

    
    
    if (aNode.childNodes[4]) {
      
      location = aNode.childNodes[4].getAttribute("title");
    }
    else {
      location = "";
    }

    let dupe = hud.cssNodes[description + location];
    if (!dupe) {
      
      hud.cssNodes[description + location] = aNode;
      return false;
    }

    this.mergeFilteredMessageNode(dupe, aNode);

    return true;
  },

  












  filterRepeatedConsole:
  function ConsoleUtils_filterRepeatedConsole(aNode, aOutput) {
    let lastMessage = aOutput.lastChild;

    
    if (lastMessage && lastMessage.childNodes[2] &&
        !aNode.classList.contains("webconsole-msg-inspector") &&
        aNode.childNodes[2].textContent ==
        lastMessage.childNodes[2].textContent) {
      this.mergeFilteredMessageNode(lastMessage, aNode);
      return true;
    }

    return false;
  },

  







  isOutputScrolledToBottom:
  function ConsoleUtils_isOutputScrolledToBottom(aOutputNode)
  {
    let lastNodeHeight = aOutputNode.lastChild ?
                         aOutputNode.lastChild.clientHeight : 0;
    let scrollBox = aOutputNode.scrollBoxObject.element;

    return scrollBox.scrollTop + scrollBox.clientHeight >=
           scrollBox.scrollHeight - lastNodeHeight / 2;
  },
};





HeadsUpDisplayUICommands = {
  refreshCommand: function UIC_refreshCommand() {
    var window = HUDService.currentContext();
    if (!window) {
      return;
    }

    let command = window.document.getElementById("Tools:WebConsole");
    if (this.getOpenHUD() != null) {
      command.setAttribute("checked", true);
    } else {
      command.removeAttribute("checked");
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

  







  toggleFilter: function UIC_toggleFilter(aEvent) {
    let hudId = this.getAttribute("hudId");
    switch (this.tagName) {
      case "toolbarbutton": {
        let originalTarget = aEvent.originalTarget;
        let classes = originalTarget.classList;

        if (originalTarget.localName !== "toolbarbutton") {
          
          
          
          break;
        }

        if (!classes.contains("toolbarbutton-menubutton-button") &&
            originalTarget.getAttribute("type") === "menu-button") {
          
          
          
          break;
        }

        let state = this.getAttribute("checked") !== "true";
        this.setAttribute("checked", state);

        
        
        
        let menuItems = this.querySelectorAll("menuitem");
        for (let i = 0; i < menuItems.length; i++) {
          menuItems[i].setAttribute("checked", state);
          let prefKey = menuItems[i].getAttribute("prefKey");
          HUDService.setFilterState(hudId, prefKey, state);
        }
        break;
      }

      case "menuitem": {
        let state = this.getAttribute("checked") !== "true";
        this.setAttribute("checked", state);

        let prefKey = this.getAttribute("prefKey");
        HUDService.setFilterState(hudId, prefKey, state);

        
        let menuPopup = this.parentNode;

        let someChecked = false;
        let menuItem = menuPopup.firstChild;
        while (menuItem) {
          if (menuItem.getAttribute("checked") === "true") {
            someChecked = true;
            break;
          }
          menuItem = menuItem.nextSibling;
        }
        let toolbarButton = menuPopup.parentNode;
        toolbarButton.setAttribute("checked", someChecked);
        break;
      }
    }

    return true;
  },

  command: function UIC_command(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    switch (filter) {
      case "copy": {
        let outputNode = HUDService.hudReferences[hudId].outputNode;
        HUDService.copySelectedItems(outputNode);
        break;
      }
      case "selectAll": {
        HUDService.hudReferences[hudId].outputNode.selectAll();
        break;
      }
      case "saveBodies": {
        let checked = aButton.getAttribute("checked") === "true";
        HUDService.hudReferences[hudId].saveRequestAndResponseBodies = checked;
        break;
      }
    }
  },
};





let WebConsoleObserver = {
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









function CommandController(aWindow) {
  this.window = aWindow;
}

CommandController.prototype = {
  






  _getFocusedOutputNode: function CommandController_getFocusedOutputNode()
  {
    let element = this.window.document.commandDispatcher.focusedElement;
    if (element && element.classList.contains("hud-output-node")) {
      return element;
    }
    return null;
  },

  







  copy: function CommandController_copy(aOutputNode)
  {
    HUDService.copySelectedItems(aOutputNode);
  },

  






  selectAll: function CommandController_selectAll(aOutputNode)
  {
    aOutputNode.selectAll();
  },

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    return this.isCommandEnabled(aCommand);
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    let outputNode = this._getFocusedOutputNode();
    if (!outputNode) {
      return false;
    }

    switch (aCommand) {
      case "cmd_copy":
        
        return outputNode.selectedCount > 0;
      case "cmd_selectAll":
        
        return true;
    }
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    let outputNode = this._getFocusedOutputNode();
    switch (aCommand) {
      case "cmd_copy":
        this.copy(outputNode);
        break;
      case "cmd_selectAll":
        this.selectAll(outputNode);
        break;
    }
  }
};

XPCOMUtils.defineLazyGetter(this, "HUDService", function () {
  return new HUD_SERVICE();
});

