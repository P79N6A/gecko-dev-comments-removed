





"use strict";

const {Cc, Ci, Cu} = require("chrome");

const {Utils: WebConsoleUtils, CONSOLE_WORKER_IDS} = require("devtools/toolkit/webconsole/utils");

loader.lazyServiceGetter(this, "clipboardHelper",
                         "@mozilla.org/widget/clipboardhelper;1",
                         "nsIClipboardHelper");
loader.lazyImporter(this, "Services", "resource://gre/modules/Services.jsm");
loader.lazyImporter(this, "promise", "resource://gre/modules/Promise.jsm", "Promise");
loader.lazyGetter(this, "EventEmitter", () => require("devtools/toolkit/event-emitter"));
loader.lazyGetter(this, "AutocompletePopup",
                  () => require("devtools/shared/autocomplete-popup").AutocompletePopup);
loader.lazyGetter(this, "ToolSidebar",
                  () => require("devtools/framework/sidebar").ToolSidebar);
loader.lazyGetter(this, "NetworkPanel",
                  () => require("devtools/webconsole/network-panel").NetworkPanel);
loader.lazyGetter(this, "ConsoleOutput",
                  () => require("devtools/webconsole/console-output").ConsoleOutput);
loader.lazyGetter(this, "Messages",
                  () => require("devtools/webconsole/console-output").Messages);
loader.lazyGetter(this, "asyncStorage",
                  () => require("devtools/toolkit/shared/async-storage"));
loader.lazyImporter(this, "EnvironmentClient", "resource://gre/modules/devtools/dbg-client.jsm");
loader.lazyImporter(this, "ObjectClient", "resource://gre/modules/devtools/dbg-client.jsm");
loader.lazyImporter(this, "VariablesView", "resource:///modules/devtools/VariablesView.jsm");
loader.lazyImporter(this, "VariablesViewController", "resource:///modules/devtools/VariablesViewController.jsm");
loader.lazyImporter(this, "PluralForm", "resource://gre/modules/PluralForm.jsm");
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

const XHTML_NS = "http://www.w3.org/1999/xhtml";

const MIXED_CONTENT_LEARN_MORE = "https://developer.mozilla.org/docs/Security/MixedContent";

const INSECURE_PASSWORDS_LEARN_MORE = "https://developer.mozilla.org/docs/Security/InsecurePasswords";

const STRICT_TRANSPORT_SECURITY_LEARN_MORE = "https://developer.mozilla.org/docs/Security/HTTP_Strict_Transport_Security";

const WEAK_SIGNATURE_ALGORITHM_LEARN_MORE = "https://developer.mozilla.org/docs/Security/Weak_Signature_Algorithm";

const HELP_URL = "https://developer.mozilla.org/docs/Tools/Web_Console/Helpers";

const VARIABLES_VIEW_URL = "chrome://browser/content/devtools/widgets/VariablesView.xul";

const CONSOLE_DIR_VIEW_HEIGHT = 0.6;

const IGNORED_SOURCE_URLS = ["debugger eval code"];



const SEARCH_DELAY = 200;




const DEFAULT_LOG_LIMIT = 200;



const CATEGORY_NETWORK = 0;
const CATEGORY_CSS = 1;
const CATEGORY_JS = 2;
const CATEGORY_WEBDEV = 3;
const CATEGORY_INPUT = 4;   
const CATEGORY_OUTPUT = 5;  
const CATEGORY_SECURITY = 6;



const SEVERITY_ERROR = 0;
const SEVERITY_WARNING = 1;
const SEVERITY_INFO = 2;
const SEVERITY_LOG = 3;


const CATEGORY_CLASS_FRAGMENTS = [
  "network",
  "cssparser",
  "exception",
  "console",
  "input",
  "output",
  "security",
];


const SEVERITY_CLASS_FRAGMENTS = [
  "error",
  "warn",
  "info",
  "log",
];






const MESSAGE_PREFERENCE_KEYS = [

  [ "network",    "netwarn",    "netxhr",  "networkinfo", ],  
  [ "csserror",   "cssparser",  null,      "csslog",      ],  
  [ "exception",  "jswarn",     null,      "jslog",       ],  
  [ "error",      "warn",       "info",    "log",         ],  
  [ null,         null,         null,      null,          ],  
  [ null,         null,         null,      null,          ],  
  [ "secerror",   "secwarn",    null,      null,          ],  
];



const LEVELS = {
  error: SEVERITY_ERROR,
  exception: SEVERITY_ERROR,
  assert: SEVERITY_ERROR,
  warn: SEVERITY_WARNING,
  info: SEVERITY_INFO,
  log: SEVERITY_LOG,
  trace: SEVERITY_LOG,
  table: SEVERITY_LOG,
  debug: SEVERITY_LOG,
  dir: SEVERITY_LOG,
  group: SEVERITY_LOG,
  groupCollapsed: SEVERITY_LOG,
  groupEnd: SEVERITY_LOG,
  time: SEVERITY_LOG,
  timeEnd: SEVERITY_LOG,
  count: SEVERITY_LOG
};



const WORKERTYPES_PREFKEYS = [ 'sharedworkers', 'serviceworkers', 'windowlessworkers' ];


const MIN_HTTP_ERROR_CODE = 400;

const MAX_HTTP_ERROR_CODE = 599;


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;


const GROUP_INDENT = 12;



const MESSAGES_IN_INTERVAL = DEFAULT_LOG_LIMIT;




const OUTPUT_INTERVAL = 20; 




const MAX_CLEANUP_TIME = 10; 




const THROTTLE_UPDATES = 1000; 


const FILTER_PREFS_PREFIX = "devtools.webconsole.filter.";


const MIN_FONT_SIZE = 10;

const PREF_CONNECTION_TIMEOUT = "devtools.debugger.remote-timeout";
const PREF_PERSISTLOG = "devtools.webconsole.persistlog";
const PREF_MESSAGE_TIMESTAMP = "devtools.webconsole.timestampMessages";
const PREF_INPUT_HISTORY_COUNT = "devtools.webconsole.inputHistoryCount";













function WebConsoleFrame(aWebConsoleOwner)
{
  this.owner = aWebConsoleOwner;
  this.hudId = this.owner.hudId;
  this.window = this.owner.iframeWindow;

  this._repeatNodes = {};
  this._outputQueue = [];
  this._itemDestroyQueue = [];
  this._pruneCategoriesQueue = {};
  this._networkRequests = {};
  this.filterPrefs = {};

  this.output = new ConsoleOutput(this);

  this._toggleFilter = this._toggleFilter.bind(this);
  this._onPanelSelected = this._onPanelSelected.bind(this);
  this._flushMessageQueue = this._flushMessageQueue.bind(this);
  this._onToolboxPrefChanged = this._onToolboxPrefChanged.bind(this);

  this._outputTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._outputTimerInitialized = false;

  EventEmitter.decorate(this);
}
exports.WebConsoleFrame = WebConsoleFrame;

WebConsoleFrame.prototype = {
  




  owner: null,

  







  proxy: null,

  



  get popupset() this.owner.mainPopupSet,

  




  _initDefer: null,

  





  _networkRequests: null,

  






  _lastOutputFlush: 0,

  





  _outputQueue: null,

  





  _pruneCategoriesQueue: null,

  






  _flushCallback: null,

  





  _outputTimer: null,
  _outputTimerInitialized: null,

  




  _repeatNodes: null,

  




  filterPrefs: null,

  




  _filterPrefsPrefix: FILTER_PREFS_PREFIX,

  


  groupDepth: 0,

  



  contentLocation: "",

  




  jsterm: null,

  



  outputNode: null,

  



  output: null,

  



  filterBox: null,

  



  get webConsoleClient() this.proxy ? this.proxy.webConsoleClient : null,

  _destroyer: null,

  
  _saveRequestAndResponseBodies: false,

  
  _chevronWidth: 0,
  
  _inputCharWidth: 0,

  






  getSaveRequestAndResponseBodies:
  function WCF_getSaveRequestAndResponseBodies() {
    let deferred = promise.defer();
    let toGet = [
      "NetworkMonitor.saveRequestAndResponseBodies"
    ];

    
    this.webConsoleClient.getPreferences(toGet, aResponse => {
      if (!aResponse.error) {
        this._saveRequestAndResponseBodies = aResponse.preferences[toGet[0]];
        deferred.resolve(this._saveRequestAndResponseBodies);
      }
      else {
        deferred.reject(aResponse.error);
      }
    });

    return deferred.promise;
  },

  





  setSaveRequestAndResponseBodies:
  function WCF_setSaveRequestAndResponseBodies(aValue) {
    if (!this.webConsoleClient) {
      
      return promise.resolve(null);
    }

    let deferred = promise.defer();
    let newValue = !!aValue;
    let toSet = {
      "NetworkMonitor.saveRequestAndResponseBodies": newValue,
    };

    
    this.webConsoleClient.setPreferences(toSet, aResponse => {
      if (!aResponse.error) {
        this._saveRequestAndResponseBodies = newValue;
        deferred.resolve(aResponse);
      }
      else {
        deferred.reject(aResponse.error);
      }
    });

    return deferred.promise;
  },

  



  get persistLog() {
    
    
    
    
    return this.owner._browserConsole || Services.prefs.getBoolPref(PREF_PERSISTLOG);
  },

  




  init: function()
  {
    this._initUI();
    let connectionInited = this._initConnection();

    
    
    let allReady = this.jsterm.historyLoaded.catch(() => {}).then(() => {
      return connectionInited;
    });

    
    
    
    let notifyObservers = () => {
      let id = WebConsoleUtils.supportsString(this.hudId);
      Services.obs.notifyObservers(id, "web-console-created", null);
    };
    allReady.then(notifyObservers, notifyObservers);

    return allReady;
  },

  







  _initConnection: function WCF__initConnection()
  {
    if (this._initDefer) {
      return this._initDefer.promise;
    }

    this._initDefer = promise.defer();
    this.proxy = new WebConsoleConnectionProxy(this, this.owner.target);

    this.proxy.connect().then(() => { 
      this._initDefer.resolve(this);
    }, (aReason) => { 
      let node = this.createMessageNode(CATEGORY_JS, SEVERITY_ERROR,
                                        aReason.error + ": " + aReason.message);
      this.outputMessage(CATEGORY_JS, node, [aReason]);
      this._initDefer.reject(aReason);
    });

    return this._initDefer.promise;
  },

  



  _initUI: function WCF__initUI()
  {
    this.document = this.window.document;
    this.rootElement = this.document.documentElement;

    this._initDefaultFilterPrefs();

    
    this._commandController = new CommandController(this);
    this.window.controllers.insertControllerAt(0, this._commandController);

    this._contextMenuHandler = new ConsoleContextMenu(this);

    let doc = this.document;

    this.filterBox = doc.querySelector(".hud-filter-box");
    this.outputNode = doc.getElementById("output-container");
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");

    this._setFilterTextBoxEvents();
    this._initFilterButtons();

    let fontSize = this.owner._browserConsole ?
                   Services.prefs.getIntPref("devtools.webconsole.fontSize") : 0;

    if (fontSize != 0) {
      fontSize = Math.max(MIN_FONT_SIZE, fontSize);

      this.outputNode.style.fontSize = fontSize + "px";
      this.completeNode.style.fontSize = fontSize + "px";
      this.inputNode.style.fontSize = fontSize + "px";
    }

    if (this.owner._browserConsole) {
      for (let id of ["Enlarge", "Reduce", "Reset"]) {
        this.document.getElementById("cmd_fullZoom" + id)
                     .removeAttribute("disabled");
      }
    }

    
    
    this._updateCharSize();

    let updateSaveBodiesPrefUI = (aElement) => {
      this.getSaveRequestAndResponseBodies().then(aValue => {
        aElement.setAttribute("checked", aValue);
        this.emit("save-bodies-ui-toggled");
      });
    }

    let reverseSaveBodiesPref = ({ target: aElement }) => {
      this.getSaveRequestAndResponseBodies().then(aValue => {
        this.setSaveRequestAndResponseBodies(!aValue);
        aElement.setAttribute("checked", aValue);
        this.emit("save-bodies-pref-reversed");
      });
    }

    let saveBodiesDisabled = !this.getFilterState("networkinfo") &&
                             !this.getFilterState("netxhr") &&
                             !this.getFilterState("network");

    let saveBodies = doc.getElementById("saveBodies");
    saveBodies.addEventListener("command", reverseSaveBodiesPref);
    saveBodies.disabled = saveBodiesDisabled;

    let saveBodiesContextMenu = doc.getElementById("saveBodiesContextMenu");
    saveBodiesContextMenu.addEventListener("command", reverseSaveBodiesPref);
    saveBodiesContextMenu.disabled = saveBodiesDisabled;

    saveBodies.parentNode.addEventListener("popupshowing", () => {
      updateSaveBodiesPrefUI(saveBodies);
      saveBodies.disabled = !this.getFilterState("networkinfo") &&
                            !this.getFilterState("netxhr") &&
                            !this.getFilterState("network");
    });

    saveBodiesContextMenu.parentNode.addEventListener("popupshowing", () => {
      updateSaveBodiesPrefUI(saveBodiesContextMenu);
      saveBodiesContextMenu.disabled = !this.getFilterState("networkinfo") &&
                                       !this.getFilterState("netxhr") &&
                                       !this.getFilterState("network");
    });

    let clearButton = doc.getElementsByClassName("webconsole-clear-console-button")[0];
    clearButton.addEventListener("command", () => {
      this.owner._onClearButton();
      this.jsterm.clearOutput(true);
    });

    this.jsterm = new JSTerm(this);
    this.jsterm.init();

    let toolbox = gDevTools.getToolbox(this.owner.target);
    if (toolbox) {
      toolbox.on("webconsole-selected", this._onPanelSelected);
    }

    




    this._addFocusCallback(this.outputNode, (evt) => {
      if ((evt.target.nodeName.toLowerCase() != "a") &&
          (evt.target.parentNode.nodeName.toLowerCase() != "a")) {
        this.jsterm.inputNode.focus();
      }
    });

    
    gDevTools.on("pref-changed", this._onToolboxPrefChanged);
    this._onToolboxPrefChanged("pref-changed", {
      pref: PREF_MESSAGE_TIMESTAMP,
      newValue: Services.prefs.getBoolPref(PREF_MESSAGE_TIMESTAMP),
    });

    
    this.jsterm.inputNode.focus();
  },

  




  _onPanelSelected: function WCF__onPanelSelected(evt, id)
  {
    this.jsterm.inputNode.focus();
  },

  



  _initDefaultFilterPrefs: function WCF__initDefaultFilterPrefs()
  {
    let prefs = ["network", "networkinfo", "csserror", "cssparser", "csslog",
                 "exception", "jswarn", "jslog", "error", "info", "warn", "log",
                 "secerror", "secwarn", "netwarn", "netxhr", "sharedworkers",
                 "serviceworkers", "windowlessworkers"];
    for (let pref of prefs) {
      this.filterPrefs[pref] = Services.prefs
                               .getBoolPref(this._filterPrefsPrefix + pref);
    }
  },

  








  _updateReflowActivityListener:
    function WCF__updateReflowActivityListener(aCallback)
  {
    if (this.webConsoleClient) {
      let pref = this._filterPrefsPrefix + "csslog";
      if (Services.prefs.getBoolPref(pref)) {
        this.webConsoleClient.startListeners(["ReflowActivity"], aCallback);
      } else {
        this.webConsoleClient.stopListeners(["ReflowActivity"], aCallback);
      }
    }
  },

  



  _setFilterTextBoxEvents: function WCF__setFilterTextBoxEvents()
  {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let timerEvent = this.adjustVisibilityOnSearchStringChange.bind(this);

    let onChange = function _onChange() {
      
      
      timer.cancel();
      timer.initWithCallback(timerEvent, SEARCH_DELAY,
                             Ci.nsITimer.TYPE_ONE_SHOT);
    };

    this.filterBox.addEventListener("command", onChange, false);
    this.filterBox.addEventListener("input", onChange, false);
  },

  










  _initFilterButtons: function WCF__initFilterButtons()
  {
    let categories = this.document
                     .querySelectorAll(".webconsole-filter-button[category]");
    Array.forEach(categories, function(aButton) {
      aButton.addEventListener("contextmenu", (aEvent) => {
        aButton.open = true;
      }, false);
      aButton.addEventListener("click", this._toggleFilter, false);

      let someChecked = false;
      let severities = aButton.querySelectorAll("menuitem[prefKey]");
      Array.forEach(severities, function(aMenuItem) {
        aMenuItem.addEventListener("command", this._toggleFilter, false);

        let prefKey = aMenuItem.getAttribute("prefKey");
        let checked = this.filterPrefs[prefKey];
        aMenuItem.setAttribute("checked", checked);
        someChecked = someChecked || checked;
      }, this);

      aButton.setAttribute("checked", someChecked);
      aButton.setAttribute("aria-pressed", someChecked);
    }, this);

    if (!this.owner._browserConsole) {
      
      
      
      let jslog = this.document.querySelector("menuitem[prefKey=jslog]");
      jslog.hidden = true;
    }

    if (Services.appinfo.OS == "Darwin") {
      let net = this.document.querySelector("toolbarbutton[category=net]");
      let accesskey = net.getAttribute("accesskeyMacOSX");
      net.setAttribute("accesskey", accesskey);

      let logging = this.document.querySelector("toolbarbutton[category=logging]");
      logging.removeAttribute("accesskey");
    }
  },

  






  changeFontSize: function WCF_changeFontSize(aSize)
  {
    let fontSize = this.window
                   .getComputedStyle(this.outputNode, null)
                   .getPropertyValue("font-size").replace("px", "");

    if (this.outputNode.style.fontSize) {
      fontSize = this.outputNode.style.fontSize.replace("px", "");
    }

    if (aSize == "+" || aSize == "-") {
      fontSize = parseInt(fontSize, 10);

      if (aSize == "+") {
        fontSize += 1;
      }
      else {
        fontSize -= 1;
      }

      if (fontSize < MIN_FONT_SIZE) {
        fontSize = MIN_FONT_SIZE;
      }

      Services.prefs.setIntPref("devtools.webconsole.fontSize", fontSize);
      fontSize = fontSize + "px";

      this.completeNode.style.fontSize = fontSize;
      this.inputNode.style.fontSize = fontSize;
      this.outputNode.style.fontSize = fontSize;
    }
    else {
      this.completeNode.style.fontSize = "";
      this.inputNode.style.fontSize = "";
      this.outputNode.style.fontSize = "";
      Services.prefs.clearUserPref("devtools.webconsole.fontSize");
    }
    this._updateCharSize();
  },

  





  _updateCharSize: function WCF__updateCharSize()
  {
    let doc = this.document;
    let tempLabel = doc.createElementNS(XHTML_NS, "span");
    let style = tempLabel.style;
    style.position = "fixed";
    style.padding = "0";
    style.margin = "0";
    style.width = "auto";
    style.color = "transparent";
    WebConsoleUtils.copyTextStyles(this.inputNode, tempLabel);
    tempLabel.textContent = "x";
    doc.documentElement.appendChild(tempLabel);
    this._inputCharWidth = tempLabel.offsetWidth;
    tempLabel.parentNode.removeChild(tempLabel);
    
    
    this._chevronWidth = +doc.defaultView.getComputedStyle(this.inputNode)
                             .paddingLeft.replace(/[^0-9.]/g, "") - 4;
  },

  







  _toggleFilter: function WCF__toggleFilter(aEvent)
  {
    let target = aEvent.target;
    let tagName = target.tagName;
    
    let isRightClick = (aEvent.button === 2); 
    if (tagName != aEvent.currentTarget.tagName || isRightClick) {
      return;
    }

    switch (tagName) {
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

        
        
        let state = target.getAttribute("checked") !== "true";
        if (aEvent.getModifierState("Alt")) {
          let buttons = this.document
                        .querySelectorAll(".webconsole-filter-button");
          Array.forEach(buttons, (button) => {
            if (button !== target) {
              button.setAttribute("checked", false);
              button.setAttribute("aria-pressed", false);
              this._setMenuState(button, false);
            }
          });
          state = true;
        }
        target.setAttribute("checked", state);
        target.setAttribute("aria-pressed", state);

        
        
        
        this._setMenuState(target, state);

        
        
        
        if (target.getAttribute("category") == "css" && state) {
          let csslogMenuItem = target.querySelector("menuitem[prefKey=csslog]");
          csslogMenuItem.setAttribute("checked", false);
          this.setFilterState("csslog", false);
        }

        break;
      }

      case "menuitem": {
        let state = target.getAttribute("checked") !== "true";
        target.setAttribute("checked", state);

        let prefKey = target.getAttribute("prefKey");
        this.setFilterState(prefKey, state);

        
        if (prefKey == "networkinfo" || prefKey == "netxhr" || prefKey == "network") {
          let checkState = !this.getFilterState("networkinfo") &&
                           !this.getFilterState("netxhr") &&
                           !this.getFilterState("network");
          this.document.getElementById("saveBodies").disabled = checkState;
          this.document.getElementById("saveBodiesContextMenu").disabled = checkState;
        }

        
        let menuPopup = target.parentNode;

        let someChecked = false;
        let menuItem = menuPopup.firstChild;
        while (menuItem) {
          if (menuItem.hasAttribute("prefKey") &&
              menuItem.getAttribute("checked") === "true") {
            someChecked = true;
            break;
          }
          menuItem = menuItem.nextSibling;
        }
        let toolbarButton = menuPopup.parentNode;
        toolbarButton.setAttribute("checked", someChecked);
        toolbarButton.setAttribute("aria-pressed", someChecked);
        break;
      }
    }
  },

  








  _setMenuState: function WCF__setMenuState(aTarget, aState)
  {
    let menuItems = aTarget.querySelectorAll("menuitem");
    Array.forEach(menuItems, (item) => {
      item.setAttribute("checked", aState);
      let prefKey = item.getAttribute("prefKey");
      this.setFilterState(prefKey, aState);
    });
  },

  






  setFilterState: function WCF_setFilterState(aToggleType, aState)
  {
    this.filterPrefs[aToggleType] = aState;
    this.adjustVisibilityForMessageType(aToggleType, aState);
    Services.prefs.setBoolPref(this._filterPrefsPrefix + aToggleType, aState);
    this._updateReflowActivityListener();
  },

  





  getFilterState: function WCF_getFilterState(aToggleType)
  {
    return this.filterPrefs[aToggleType];
  },

  








  stringMatchesFilters: function WCF_stringMatchesFilters(aString, aFilter)
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

  











  adjustVisibilityForMessageType:
  function WCF_adjustVisibilityForMessageType(aPrefKey, aState)
  {
    let outputNode = this.outputNode;
    let doc = this.document;

    
    
    

    let attribute = WORKERTYPES_PREFKEYS.indexOf(aPrefKey) == -1
                      ? 'filter' : 'workerType';

    let xpath = ".//*[contains(@class, 'message') and " +
      "@" + attribute + "='" + aPrefKey + "']";
    let result = doc.evaluate(xpath, outputNode, null,
      Ci.nsIDOMXPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, null);
    for (let i = 0; i < result.snapshotLength; i++) {
      let node = result.snapshotItem(i);
      if (aState) {
        node.classList.remove("filtered-by-type");
      }
      else {
        node.classList.add("filtered-by-type");
      }
    }
  },

  



  adjustVisibilityOnSearchStringChange:
  function WCF_adjustVisibilityOnSearchStringChange()
  {
    let nodes = this.outputNode.getElementsByClassName("message");
    let searchString = this.filterBox.value;

    for (let i = 0, n = nodes.length; i < n; ++i) {
      let node = nodes[i];

      
      let text = node.textContent;

      
      if (this.stringMatchesFilters(text, searchString)) {
        node.classList.remove("filtered-by-string");
      }
      else {
        node.classList.add("filtered-by-string");
      }
    }
  },

  








  filterMessageNode: function WCF_filterMessageNode(aNode)
  {
    let isFiltered = false;

    
    let prefKey = MESSAGE_PREFERENCE_KEYS[aNode.category][aNode.severity];
    if (prefKey && !this.getFilterState(prefKey)) {
      
      aNode.classList.add("filtered-by-type");
      isFiltered = true;
    }

    
    if ("workerType" in aNode && !this.getFilterState(aNode.workerType)) {
      aNode.classList.add("filtered-by-type");
      isFiltered = true;
    }

    
    let search = this.filterBox.value;
    let text = aNode.clipboardText;

    
    if (!this.stringMatchesFilters(text, search)) {
      aNode.classList.add("filtered-by-string");
      isFiltered = true;
    }

    if (isFiltered && aNode.classList.contains("inlined-variables-view")) {
      aNode.classList.add("hidden-message");
    }

    return isFiltered;
  },

  








  mergeFilteredMessageNode:
  function WCF_mergeFilteredMessageNode(aOriginal, aFiltered)
  {
    let repeatNode = aOriginal.getElementsByClassName("message-repeats")[0];
    if (!repeatNode) {
      return; 
    }

    let occurrences = parseInt(repeatNode.getAttribute("value")) + 1;
    repeatNode.setAttribute("value", occurrences);
    repeatNode.textContent = occurrences;
    let str = l10n.getStr("messageRepeats.tooltip2");
    repeatNode.title = PluralForm.get(occurrences, str)
                       .replace("#1", occurrences);
  },

  









  _filterRepeatedMessage: function WCF__filterRepeatedMessage(aNode)
  {
    let repeatNode = aNode.getElementsByClassName("message-repeats")[0];
    if (!repeatNode) {
      return null;
    }

    let uid = repeatNode._uid;
    let dupeNode = null;

    if (aNode.category == CATEGORY_CSS ||
        aNode.category == CATEGORY_SECURITY) {
      dupeNode = this._repeatNodes[uid];
      if (!dupeNode) {
        this._repeatNodes[uid] = aNode;
      }
    }
    else if ((aNode.category == CATEGORY_WEBDEV ||
              aNode.category == CATEGORY_JS) &&
             aNode.category != CATEGORY_NETWORK &&
             !aNode.classList.contains("inlined-variables-view")) {
      let lastMessage = this.outputNode.lastChild;
      if (!lastMessage) {
        return null;
      }

      let lastRepeatNode = lastMessage.getElementsByClassName("message-repeats")[0];
      if (lastRepeatNode && lastRepeatNode._uid == uid) {
        dupeNode = lastMessage;
      }
    }

    if (dupeNode) {
      this.mergeFilteredMessageNode(dupeNode, aNode);
      return dupeNode;
    }

    return null;
  },

  







  displayCachedMessages: function WCF_displayCachedMessages(aRemoteMessages)
  {
    if (!aRemoteMessages.length) {
      return;
    }

    aRemoteMessages.forEach(function(aMessage) {
      switch (aMessage._type) {
        case "PageError": {
          let category = Utils.categoryForScriptError(aMessage);
          this.outputMessage(category, this.reportPageError,
                             [category, aMessage]);
          break;
        }
        case "LogMessage":
          this.handleLogMessage(aMessage);
          break;
        case "ConsoleAPI":
          this.outputMessage(CATEGORY_WEBDEV, this.logConsoleAPIMessage,
                             [aMessage]);
          break;
      }
    }, this);
  },

  








  logConsoleAPIMessage: function WCF_logConsoleAPIMessage(aMessage)
  {
    let body = null;
    let clipboardText = null;
    let sourceURL = aMessage.filename;
    let sourceLine = aMessage.lineNumber;
    let level = aMessage.level;
    let args = aMessage.arguments;
    let objectActors = new Set();
    let node = null;

    
    args.forEach((aValue) => {
      if (WebConsoleUtils.isActorGrip(aValue)) {
        objectActors.add(aValue.actor);
      }
    });

    switch (level) {
      case "log":
      case "info":
      case "warn":
      case "error":
      case "exception":
      case "assert":
      case "debug": {
        let msg = new Messages.ConsoleGeneric(aMessage);
        node = msg.init(this.output).render().element;
        break;
      }
      case "table": {
        let msg = new Messages.ConsoleTable(aMessage);
        node = msg.init(this.output).render().element;
        break;
      }
      case "trace": {
        let msg = new Messages.ConsoleTrace(aMessage);
        node = msg.init(this.output).render().element;
        break;
      }
      case "dir": {
        body = { arguments: args };
        let clipboardArray = [];
        args.forEach((aValue) => {
          clipboardArray.push(VariablesView.getString(aValue));
        });
        clipboardText = clipboardArray.join(" ");
        break;
      }

      case "group":
      case "groupCollapsed":
        clipboardText = body = aMessage.groupName;
        this.groupDepth++;
        break;

      case "groupEnd":
        if (this.groupDepth > 0) {
          this.groupDepth--;
        }
        break;

      case "time": {
        let timer = aMessage.timer;
        if (!timer) {
          return null;
        }
        if (timer.error) {
          Cu.reportError(l10n.getStr(timer.error));
          return null;
        }
        body = l10n.getFormatStr("timerStarted", [timer.name]);
        clipboardText = body;
        break;
      }

      case "timeEnd": {
        let timer = aMessage.timer;
        if (!timer) {
          return null;
        }
        let duration = Math.round(timer.duration * 100) / 100;
        body = l10n.getFormatStr("timeEnd", [timer.name, duration]);
        clipboardText = body;
        break;
      }

      case "count": {
        let counter = aMessage.counter;
        if (!counter) {
          return null;
        }
        if (counter.error) {
          Cu.reportError(l10n.getStr(counter.error));
          return null;
        }
        let msg = new Messages.ConsoleGeneric(aMessage);
        node = msg.init(this.output).render().element;
        break;
      }

      default:
        Cu.reportError("Unknown Console API log level: " + level);
        return null;
    }

    
    
    switch (level) {
      case "group":
      case "groupCollapsed":
      case "groupEnd":
      case "time":
      case "timeEnd":
      case "count":
        for (let actor of objectActors) {
          this._releaseObject(actor);
        }
        objectActors.clear();
    }

    if (level == "groupEnd") {
      return null; 
    }

    if (!node) {
      node = this.createMessageNode(CATEGORY_WEBDEV, LEVELS[level], body,
                                    sourceURL, sourceLine, clipboardText,
                                    level, aMessage.timeStamp);
      if (aMessage.private) {
        node.setAttribute("private", true);
      }
    }

    if (objectActors.size > 0) {
      node._objectActors = objectActors;

      if (!node._messageObject) {
        let repeatNode = node.getElementsByClassName("message-repeats")[0];
        repeatNode._uid += [...objectActors].join("-");
      }
    }

    let workerTypeID = CONSOLE_WORKER_IDS.indexOf(aMessage.workerType);
    if (workerTypeID != -1) {
      node.workerType = WORKERTYPES_PREFKEYS[workerTypeID];
      node.setAttribute('workerType', WORKERTYPES_PREFKEYS[workerTypeID]);
    }

    return node;
  },

  






  handleConsoleAPICall: function WCF_handleConsoleAPICall(aMessage)
  {
    this.outputMessage(CATEGORY_WEBDEV, this.logConsoleAPIMessage, [aMessage]);
  },

  







  reportPageError: function WCF_reportPageError(aCategory, aScriptError)
  {
    
    
    let severity = 'error';
    if (aScriptError.warning || aScriptError.strict) {
      severity = 'warning';
    }

    let category = 'js';
    switch(aCategory) {
      case CATEGORY_CSS:
        category = 'css';
        break;
      case CATEGORY_SECURITY:
        category = 'security';
        break;
    }

    let objectActors = new Set();

    
    for (let prop of ["errorMessage", "lineText"]) {
      let grip = aScriptError[prop];
      if (WebConsoleUtils.isActorGrip(grip)) {
        objectActors.add(grip.actor);
      }
    }

    let errorMessage = aScriptError.errorMessage;
    if (errorMessage.type && errorMessage.type == "longString") {
      errorMessage = errorMessage.initial;
    }

    
    let msg = new Messages.Simple(errorMessage, {
      location: {
        url: aScriptError.sourceName,
        line: aScriptError.lineNumber,
        column: aScriptError.columnNumber
      },
      category: category,
      severity: severity,
      timestamp: aScriptError.timeStamp,
      private: aScriptError.private,
      filterDuplicates: true
    });

    let node = msg.init(this.output).render().element;

    
    let msgBody = node.getElementsByClassName("message-body")[0];
    
    this.addMoreInfoLink(msgBody, aScriptError);

    if (objectActors.size > 0) {
      node._objectActors = objectActors;
    }

    return node;
  },

  






  handlePageError: function WCF_handlePageError(aPageError)
  {
    let category = Utils.categoryForScriptError(aPageError);
    this.outputMessage(category, this.reportPageError, [category, aPageError]);
  },

  






  handleLogMessage: function WCF_handleLogMessage(aPacket)
  {
    if (aPacket.message) {
      this.outputMessage(CATEGORY_JS, this._reportLogMessage, [aPacket]);
    }
  },

  








  _reportLogMessage: function WCF__reportLogMessage(aPacket)
  {
    let msg = aPacket.message;
    if (msg.type && msg.type == "longString") {
      msg = msg.initial;
    }
    let node = this.createMessageNode(CATEGORY_JS, SEVERITY_LOG, msg, null,
                                      null, null, null, aPacket.timeStamp);
    if (WebConsoleUtils.isActorGrip(aPacket.message)) {
      node._objectActors = new Set([aPacket.message.actor]);
    }
    return node;
  },

  







  logNetEvent: function WCF_logNetEvent(aActor)
  {
    let actorId = aActor.actor;
    let networkInfo = this._networkRequests[actorId];
    if (!networkInfo) {
      return null;
    }

    let request = networkInfo.request;
    let clipboardText = request.method + " " + request.url;
    let severity = SEVERITY_LOG;
    if (networkInfo.isXHR) {
      clipboardText = request.method + " XHR " + request.url;
      severity = SEVERITY_INFO;
    }
    let mixedRequest =
      WebConsoleUtils.isMixedHTTPSRequest(request.url, this.contentLocation);
    if (mixedRequest) {
      severity = SEVERITY_WARNING;
    }

    let methodNode = this.document.createElementNS(XHTML_NS, "span");
    methodNode.className = "method";
    methodNode.textContent = request.method + " ";

    let messageNode = this.createMessageNode(CATEGORY_NETWORK, severity,
                                             methodNode, null, null,
                                             clipboardText);
    if (networkInfo.private) {
      messageNode.setAttribute("private", true);
    }
    messageNode._connectionId = actorId;
    messageNode.url = request.url;

    let body = methodNode.parentNode;
    body.setAttribute("aria-haspopup", true);

    if (networkInfo.isXHR) {
      let xhrNode = this.document.createElementNS(XHTML_NS, "span");
      xhrNode.className = "xhr";
      xhrNode.textContent = l10n.getStr("webConsoleXhrIndicator");
      body.appendChild(xhrNode);
      body.appendChild(this.document.createTextNode(" "));
    }

    let displayUrl = request.url;
    let pos = displayUrl.indexOf("?");
    if (pos > -1) {
      displayUrl = displayUrl.substr(0, pos);
    }

    let urlNode = this.document.createElementNS(XHTML_NS, "a");
    urlNode.className = "url";
    urlNode.setAttribute("title", request.url);
    urlNode.href = request.url;
    urlNode.textContent = displayUrl;
    urlNode.draggable = false;
    body.appendChild(urlNode);
    body.appendChild(this.document.createTextNode(" "));

    if (mixedRequest) {
      messageNode.classList.add("mixed-content");
      this.makeMixedContentNode(body);
    }

    let statusNode = this.document.createElementNS(XHTML_NS, "a");
    statusNode.className = "status";
    body.appendChild(statusNode);

    let onClick = () => {
      if (!messageNode._panelOpen) {
        this.openNetworkPanel(messageNode, networkInfo);
      }
    };

    this._addMessageLinkCallback(urlNode, onClick);
    this._addMessageLinkCallback(statusNode, onClick);

    networkInfo.node = messageNode;

    this._updateNetMessage(actorId);

    return messageNode;
  },

  





  makeMixedContentNode: function WCF_makeMixedContentNode(aLinkNode)
  {
    let mixedContentWarning = "[" + l10n.getStr("webConsoleMixedContentWarning") + "]";

    
    let mixedContentWarningNode = this.document.createElementNS(XHTML_NS, "a");
    mixedContentWarningNode.title = MIXED_CONTENT_LEARN_MORE;
    mixedContentWarningNode.href = MIXED_CONTENT_LEARN_MORE;
    mixedContentWarningNode.className = "learn-more-link";
    mixedContentWarningNode.textContent = mixedContentWarning;
    mixedContentWarningNode.draggable = false;

    aLinkNode.appendChild(mixedContentWarningNode);

    this._addMessageLinkCallback(mixedContentWarningNode, (aEvent) => {
      aEvent.stopPropagation();
      this.owner.openLink(MIXED_CONTENT_LEARN_MORE);
    });
  },

  








  addMoreInfoLink: function WCF_addMoreInfoLink(aNode, aScriptError)
  {
    let url;
    switch (aScriptError.category) {
     case "Insecure Password Field":
       url = INSECURE_PASSWORDS_LEARN_MORE;
     break;
     case "Mixed Content Message":
     case "Mixed Content Blocker":
      url = MIXED_CONTENT_LEARN_MORE;
     break;
     case "Invalid HSTS Headers":
      url = STRICT_TRANSPORT_SECURITY_LEARN_MORE;
     break;
     case "SHA-1 Signature":
      url = WEAK_SIGNATURE_ALGORITHM_LEARN_MORE;
     break;
     default:
      
      return;
    }

    this.addLearnMoreWarningNode(aNode, url);
  },

  











  addLearnMoreWarningNode:
  function WCF_addLearnMoreWarningNode(aNode, aURL)
  {
    let moreInfoLabel = "[" + l10n.getStr("webConsoleMoreInfoLabel") + "]";

    let warningNode = this.document.createElementNS(XHTML_NS, "a");
    warningNode.title = aURL;
    warningNode.href = aURL;
    warningNode.draggable = false;
    warningNode.textContent = moreInfoLabel;
    warningNode.className = "learn-more-link";

    this._addMessageLinkCallback(warningNode, (aEvent) => {
      aEvent.stopPropagation();
      this.owner.openLink(aURL);
    });

    aNode.appendChild(warningNode);
  },

  







  logFileActivity: function WCF_logFileActivity(aFileURI)
  {
    let urlNode = this.document.createElementNS(XHTML_NS, "a");
    urlNode.setAttribute("title", aFileURI);
    urlNode.className = "url";
    urlNode.textContent = aFileURI;
    urlNode.draggable = false;
    urlNode.href = aFileURI;

    let outputNode = this.createMessageNode(CATEGORY_NETWORK, SEVERITY_LOG,
                                            urlNode, null, null, aFileURI);

    this._addMessageLinkCallback(urlNode, () => {
      this.owner.viewSource(aFileURI);
    });

    return outputNode;
  },

  





  handleFileActivity: function WCF_handleFileActivity(aFileURI)
  {
    this.outputMessage(CATEGORY_NETWORK, this.logFileActivity, [aFileURI]);
  },

  





  logReflowActivity: function WCF_logReflowActivity(aMessage)
  {
    let {start, end, sourceURL, sourceLine} = aMessage;
    let duration = Math.round((end - start) * 100) / 100;
    let node = this.document.createElementNS(XHTML_NS, "span");
    if (sourceURL) {
      node.textContent = l10n.getFormatStr("reflow.messageWithLink", [duration]);
      let a = this.document.createElementNS(XHTML_NS, "a");
      a.href = "#";
      a.draggable = "false";
      let filename = WebConsoleUtils.abbreviateSourceURL(sourceURL);
      let functionName = aMessage.functionName || l10n.getStr("stacktrace.anonymousFunction");
      a.textContent = l10n.getFormatStr("reflow.messageLinkText",
                         [functionName, filename, sourceLine]);
      this._addMessageLinkCallback(a, () => {
        this.owner.viewSourceInDebugger(sourceURL, sourceLine);
      });
      node.appendChild(a);
    } else {
      node.textContent = l10n.getFormatStr("reflow.messageWithNoLink", [duration]);
    }
    return this.createMessageNode(CATEGORY_CSS, SEVERITY_LOG, node);
  },


  handleReflowActivity: function WCF_handleReflowActivity(aMessage)
  {
    this.outputMessage(CATEGORY_CSS, this.logReflowActivity, [aMessage]);
  },

  



  logWarningAboutReplacedAPI: function WCF_logWarningAboutReplacedAPI()
  {
    let node = this.createMessageNode(CATEGORY_JS, SEVERITY_WARNING,
                                      l10n.getStr("ConsoleAPIDisabled"));
    this.outputMessage(CATEGORY_JS, node);
  },

  





  handleNetworkEvent: function WCF_handleNetworkEvent(aActor)
  {
    let networkInfo = {
      node: null,
      actor: aActor.actor,
      discardRequestBody: true,
      discardResponseBody: true,
      startedDateTime: aActor.startedDateTime,
      request: {
        url: aActor.url,
        method: aActor.method,
      },
      isXHR: aActor.isXHR,
      response: {},
      timings: {},
      updates: [], 
      private: aActor.private,
    };

    this._networkRequests[aActor.actor] = networkInfo;
    this.outputMessage(CATEGORY_NETWORK, this.logNetEvent, [aActor]);
  },

  









  handleNetworkEventUpdate:
  function WCF_handleNetworkEventUpdate(aActorId, aType, aPacket)
  {
    let networkInfo = this._networkRequests[aActorId];
    if (!networkInfo) {
      return;
    }

    networkInfo.updates.push(aType);

    switch (aType) {
      case "requestHeaders":
        networkInfo.request.headersSize = aPacket.headersSize;
        break;
      case "requestPostData":
        networkInfo.discardRequestBody = aPacket.discardRequestBody;
        networkInfo.request.bodySize = aPacket.dataSize;
        break;
      case "responseStart":
        networkInfo.response.httpVersion = aPacket.response.httpVersion;
        networkInfo.response.status = aPacket.response.status;
        networkInfo.response.statusText = aPacket.response.statusText;
        networkInfo.response.headersSize = aPacket.response.headersSize;
        networkInfo.discardResponseBody = aPacket.response.discardResponseBody;
        break;
      case "responseContent":
        networkInfo.response.content = {
          mimeType: aPacket.mimeType,
        };
        networkInfo.response.bodySize = aPacket.contentSize;
        networkInfo.discardResponseBody = aPacket.discardResponseBody;
        break;
      case "eventTimings":
        networkInfo.totalTime = aPacket.totalTime;
        break;
    }

    if (networkInfo.node && this._updateNetMessage(aActorId)) {
      this.emit("new-messages", new Set([{
        update: true,
        node: networkInfo.node,
        response: aPacket,
      }]));
    }

    
    
    if (this.owner.lastFinishedRequestCallback &&
        networkInfo.updates.indexOf("responseContent") > -1 &&
        networkInfo.updates.indexOf("eventTimings") > -1) {
      this.owner.lastFinishedRequestCallback(networkInfo, this);
    }
  },

  









  _updateNetMessage: function WCF__updateNetMessage(aActorId)
  {
    let networkInfo = this._networkRequests[aActorId];
    if (!networkInfo || !networkInfo.node) {
      return;
    }

    let messageNode = networkInfo.node;
    let updates = networkInfo.updates;
    let hasEventTimings = updates.indexOf("eventTimings") > -1;
    let hasResponseStart = updates.indexOf("responseStart") > -1;
    let request = networkInfo.request;
    let methodText = (networkInfo.isXHR)? request.method + ' XHR' : request.method;
    let response = networkInfo.response;
    let updated = false;

    if (hasEventTimings || hasResponseStart) {
      let status = [];
      if (response.httpVersion && response.status) {
        status = [response.httpVersion, response.status, response.statusText];
      }
      if (hasEventTimings) {
        status.push(l10n.getFormatStr("NetworkPanel.durationMS",
                                      [networkInfo.totalTime]));
      }
      let statusText = "[" + status.join(" ") + "]";

      let statusNode = messageNode.getElementsByClassName("status")[0];
      statusNode.textContent = statusText;

      messageNode.clipboardText = [methodText, request.url, statusText]
                                  .join(" ");

      if (hasResponseStart && response.status >= MIN_HTTP_ERROR_CODE &&
          response.status <= MAX_HTTP_ERROR_CODE) {
        this.setMessageType(messageNode, CATEGORY_NETWORK, SEVERITY_ERROR);
      }

      updated = true;
    }

    if (messageNode._netPanel) {
      messageNode._netPanel.update();
    }

    return updated;
  },

  










  openNetworkPanel: function WCF_openNetworkPanel(aNode, aHttpActivity)
  {
    let actor = aHttpActivity.actor;

    if (actor) {
      this.webConsoleClient.getRequestHeaders(actor, (aResponse) => {
        if (aResponse.error) {
          Cu.reportError("WCF_openNetworkPanel getRequestHeaders:" +
                         aResponse.error);
          return;
        }

        aHttpActivity.request.headers = aResponse.headers;

        this.webConsoleClient.getRequestCookies(actor, onRequestCookies);
      });
    }

    let onRequestCookies = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getRequestCookies:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.request.cookies = aResponse.cookies;

      this.webConsoleClient.getResponseHeaders(actor, onResponseHeaders);
    };

    let onResponseHeaders = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseHeaders:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.headers = aResponse.headers;

      this.webConsoleClient.getResponseCookies(actor, onResponseCookies);
    };

    let onResponseCookies = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseCookies:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.cookies = aResponse.cookies;

      this.webConsoleClient.getRequestPostData(actor, onRequestPostData);
    };

    let onRequestPostData = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getRequestPostData:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.request.postData = aResponse.postData;
      aHttpActivity.discardRequestBody = aResponse.postDataDiscarded;

      this.webConsoleClient.getResponseContent(actor, onResponseContent);
    };

    let onResponseContent = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseContent:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.content = aResponse.content;
      aHttpActivity.discardResponseBody = aResponse.contentDiscarded;

      this.webConsoleClient.getEventTimings(actor, onEventTimings);
    };

    let onEventTimings = (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getEventTimings:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.timings = aResponse.timings;

      openPanel();
    };

    let openPanel = () => {
      aNode._netPanel = netPanel;

      let panel = netPanel.panel;
      panel.openPopup(aNode, "after_pointer", 0, 0, false, false);
      panel.sizeTo(450, 500);
      panel.setAttribute("hudId", this.hudId);

      panel.addEventListener("popuphiding", function WCF_netPanel_onHide() {
        panel.removeEventListener("popuphiding", WCF_netPanel_onHide);

        aNode._panelOpen = false;
        aNode._netPanel = null;
      });

      aNode._panelOpen = true;
    };

    let netPanel = new NetworkPanel(this.popupset, aHttpActivity, this);
    netPanel.linkNode = aNode;

    if (!actor) {
      openPanel();
    }

    return netPanel;
  },

  







  onLocationChange: function WCF_onLocationChange(aURI, aTitle)
  {
    this.contentLocation = aURI;
    if (this.owner.onLocationChange) {
      this.owner.onLocationChange(aURI, aTitle);
    }
  },

  







  handleTabNavigated: function WCF_handleTabNavigated(aEvent, aPacket)
  {
    if (aEvent == "will-navigate") {
      if (this.persistLog) {
        let marker = new Messages.NavigationMarker(aPacket, Date.now());
        this.output.addMessage(marker);
      }
      else {
        this.jsterm.clearOutput();
      }
    }

    if (aPacket.url) {
      this.onLocationChange(aPacket.url, aPacket.title);
    }

    if (aEvent == "navigate" && !aPacket.nativeConsoleAPI) {
      this.logWarningAboutReplacedAPI();
    }
  },

  



















  outputMessage: function WCF_outputMessage(aCategory, aMethodOrNode, aArguments)
  {
    if (!this._outputQueue.length) {
      
      
      this._lastOutputFlush = Date.now();
    }

    this._outputQueue.push([aCategory, aMethodOrNode, aArguments]);

    this._initOutputTimer();
  },

  






  _flushMessageQueue: function WCF__flushMessageQueue()
  {
    this._outputTimerInitialized = false;
    if (!this._outputTimer) {
      return;
    }

    let startTime = Date.now();
    let timeSinceFlush = startTime - this._lastOutputFlush;
    let shouldThrottle = this._outputQueue.length > MESSAGES_IN_INTERVAL &&
        timeSinceFlush < THROTTLE_UPDATES;

    
    let toDisplay = Math.min(this._outputQueue.length, MESSAGES_IN_INTERVAL);

    
    
    
    if (shouldThrottle || toDisplay < 1) {
      while (this._itemDestroyQueue.length) {
        if ((Date.now() - startTime) > MAX_CLEANUP_TIME) {
          break;
        }
        this._destroyItem(this._itemDestroyQueue.pop());
      }

      this._initOutputTimer();
      return;
    }

    
    let shouldPrune = false;
    if (this._outputQueue.length > toDisplay && this._pruneOutputQueue()) {
      toDisplay = Math.min(this._outputQueue.length, toDisplay);
      shouldPrune = true;
    }

    let batch = this._outputQueue.splice(0, toDisplay);
    let outputNode = this.outputNode;
    let lastVisibleNode = null;
    let scrollNode = outputNode.parentNode;
    let hudIdSupportsString = WebConsoleUtils.supportsString(this.hudId);

    
    
    
    let scrolledToBottom = shouldPrune ||
                           Utils.isOutputScrolledToBottom(outputNode);

    
    let messages = new Set();
    for (let i = 0; i < batch.length; i++) {
      let item = batch[i];
      let result = this._outputMessageFromQueue(hudIdSupportsString, item);
      if (result) {
        messages.add({
          node: result.isRepeated ? result.isRepeated : result.node,
          response: result.message,
          update: !!result.isRepeated,
        });

        if (result.visible && result.node == this.outputNode.lastChild) {
          lastVisibleNode = result.node;
        }
      }
    }

    let oldScrollHeight = 0;
    let removedNodes = 0;

    
    if (shouldPrune || !this._outputQueue.length) {
      
      
      if (!scrolledToBottom) {
        oldScrollHeight = scrollNode.scrollHeight;
      }

      let categories = Object.keys(this._pruneCategoriesQueue);
      categories.forEach(function _pruneOutput(aCategory) {
        removedNodes += this.pruneOutputIfNecessary(aCategory);
      }, this);
      this._pruneCategoriesQueue = {};
    }

    let isInputOutput = lastVisibleNode &&
                        (lastVisibleNode.category == CATEGORY_INPUT ||
                         lastVisibleNode.category == CATEGORY_OUTPUT);

    
    
    
    if (lastVisibleNode && (scrolledToBottom || isInputOutput)) {
      Utils.scrollToVisible(lastVisibleNode);
    }
    else if (!scrolledToBottom && removedNodes > 0 &&
             oldScrollHeight != scrollNode.scrollHeight) {
      
      
      scrollNode.scrollTop -= oldScrollHeight - scrollNode.scrollHeight;
    }

    if (messages.size) {
      this.emit("new-messages", messages);
    }

    
    if (this._outputQueue.length === 0 && this._flushCallback) {
      if (this._flushCallback() === false) {
        this._flushCallback = null;
      }
    }

    this._initOutputTimer();

    this._lastOutputFlush = Date.now();
  },

  



  _initOutputTimer: function WCF__initOutputTimer()
  {
    let panelIsDestroyed = !this._outputTimer;
    let alreadyScheduled = this._outputTimerInitialized;
    let nothingToDo = !this._itemDestroyQueue.length &&
                      !this._outputQueue.length;

    
    if (panelIsDestroyed || alreadyScheduled || nothingToDo) {
      return;
    }

    this._outputTimerInitialized = true;
    this._outputTimer.initWithCallback(this._flushMessageQueue,
                                       OUTPUT_INTERVAL,
                                       Ci.nsITimer.TYPE_ONE_SHOT);
  },

  














  _outputMessageFromQueue:
  function WCF__outputMessageFromQueue(aHudIdSupportsString, aItem)
  {
    let [category, methodOrNode, args] = aItem;

    
    
    let message = (args && args.length) ? args[args.length-1] : null;

    let node = typeof methodOrNode == "function" ?
               methodOrNode.apply(this, args || []) :
               methodOrNode;
    if (!node) {
      return null;
    }

    let afterNode = node._outputAfterNode;
    if (afterNode) {
      delete node._outputAfterNode;
    }

    let isFiltered = this.filterMessageNode(node);

    let isRepeated = this._filterRepeatedMessage(node);

    let visible = !isRepeated && !isFiltered;
    if (!isRepeated) {
      this.outputNode.insertBefore(node,
                                   afterNode ? afterNode.nextSibling : null);
      this._pruneCategoriesQueue[node.category] = true;

      let nodeID = node.getAttribute("id");
      Services.obs.notifyObservers(aHudIdSupportsString,
                                   "web-console-message-created", nodeID);

    }

    if (node._onOutput) {
      node._onOutput();
      delete node._onOutput;
    }

    return {
      visible: visible,
      node: node,
      isRepeated: isRepeated,
      message: message
    };
  },

  




  _pruneOutputQueue: function WCF__pruneOutputQueue()
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
      let limit = Utils.logLimitForCategory(category);
      let indexes = nodes[category];
      if (indexes.length > limit) {
        let n = Math.max(0, indexes.length - limit);
        pruned += n;
        for (let i = n - 1; i >= 0; i--) {
          this._itemDestroyQueue.push(this._outputQueue[indexes[i]]);
          this._outputQueue.splice(indexes[i], 1);
        }
      }
    }

    return pruned;
  },

  








  _destroyItem: function WCF__destroyItem(aItem)
  {
    
    
    let [category, methodOrNode, args] = aItem;
    if (typeof methodOrNode != "function" && methodOrNode._objectActors) {
      for (let actor of methodOrNode._objectActors) {
        this._releaseObject(actor);
      }
      methodOrNode._objectActors.clear();
    }

    if (methodOrNode == this.output._flushMessageQueue &&
        args[0]._objectActors) {
      for (let arg of args) {
        if (!arg._objectActors) {
          continue;
        }
        for (let actor of arg._objectActors) {
          this._releaseObject(actor);
        }
        arg._objectActors.clear();
      }
    }

    if (category == CATEGORY_NETWORK) {
      let connectionId = null;
      if (methodOrNode == this.logNetEvent) {
        connectionId = args[0].actor;
      }
      else if (typeof methodOrNode != "function") {
        connectionId = methodOrNode._connectionId;
      }
      if (connectionId && connectionId in this._networkRequests) {
        delete this._networkRequests[connectionId];
        this._releaseObject(connectionId);
      }
    }
    else if (category == CATEGORY_WEBDEV &&
             methodOrNode == this.logConsoleAPIMessage) {
      args[0].arguments.forEach((aValue) => {
        if (WebConsoleUtils.isActorGrip(aValue)) {
          this._releaseObject(aValue.actor);
        }
      });
    }
    else if (category == CATEGORY_JS &&
             methodOrNode == this.reportPageError) {
      let pageError = args[1];
      for (let prop of ["errorMessage", "lineText"]) {
        let grip = pageError[prop];
        if (WebConsoleUtils.isActorGrip(grip)) {
          this._releaseObject(grip.actor);
        }
      }
    }
    else if (category == CATEGORY_JS &&
             methodOrNode == this._reportLogMessage) {
      if (WebConsoleUtils.isActorGrip(args[0].message)) {
        this._releaseObject(args[0].message.actor);
      }
    }
  },

  








  pruneOutputIfNecessary: function WCF_pruneOutputIfNecessary(aCategory)
  {
    let logLimit = Utils.logLimitForCategory(aCategory);
    let messageNodes = this.outputNode.querySelectorAll(".message[category=" +
                       CATEGORY_CLASS_FRAGMENTS[aCategory] + "]");
    let n = Math.max(0, messageNodes.length - logLimit);
    [...messageNodes].slice(0, n).forEach(this.removeOutputMessage, this);
    return n;
  },

  





  removeOutputMessage: function WCF_removeOutputMessage(aNode)
  {
    if (aNode._messageObject) {
      aNode._messageObject.destroy();
    }

    if (aNode._objectActors) {
      for (let actor of aNode._objectActors) {
        this._releaseObject(actor);
      }
      aNode._objectActors.clear();
    }

    if (aNode.category == CATEGORY_CSS ||
        aNode.category == CATEGORY_SECURITY) {
      let repeatNode = aNode.getElementsByClassName("message-repeats")[0];
      if (repeatNode && repeatNode._uid) {
        delete this._repeatNodes[repeatNode._uid];
      }
    }
    else if (aNode._connectionId &&
             aNode.category == CATEGORY_NETWORK) {
      delete this._networkRequests[aNode._connectionId];
      this._releaseObject(aNode._connectionId);
    }
    else if (aNode.classList.contains("inlined-variables-view")) {
      let view = aNode._variablesView;
      if (view) {
        view.controller.releaseActors();
      }
      aNode._variablesView = null;
    }

    aNode.remove();
  },

  



























  createMessageNode:
  function WCF_createMessageNode(aCategory, aSeverity, aBody, aSourceURL,
                                 aSourceLine, aClipboardText, aLevel, aTimeStamp)
  {
    if (typeof aBody != "string" && aClipboardText == null && aBody.innerText) {
      aClipboardText = aBody.innerText;
    }

    let indentNode = this.document.createElementNS(XHTML_NS, "span");
    indentNode.className = "indent";

    
    let indent = this.groupDepth * GROUP_INDENT;
    indentNode.style.width = indent + "px";

    
    
    
    let iconContainer = this.document.createElementNS(XHTML_NS, "span");
    iconContainer.className = "icon";

    
    let bodyNode = this.document.createElementNS(XHTML_NS, "span");
    bodyNode.className = "message-body-wrapper message-body devtools-monospace";

    
    let body = aBody;
    
    
    aClipboardText = aClipboardText ||
                     (aBody + (aSourceURL ? " @ " + aSourceURL : "") +
                              (aSourceLine ? ":" + aSourceLine : ""));

    let timestamp = aTimeStamp || Date.now();

    
    let node = this.document.createElementNS(XHTML_NS, "div");
    node.id = "console-msg-" + gSequenceId();
    node.className = "message";
    node.clipboardText = aClipboardText;
    node.timestamp = timestamp;
    this.setMessageType(node, aCategory, aSeverity);

    if (aBody instanceof Ci.nsIDOMNode) {
      bodyNode.appendChild(aBody);
    }
    else {
      let str = undefined;
      if (aLevel == "dir") {
        str = VariablesView.getString(aBody.arguments[0]);
      }
      else {
        str = aBody;
      }

      if (str !== undefined) {
        aBody = this.document.createTextNode(str);
        bodyNode.appendChild(aBody);
      }
    }

    
    let repeatNode = null;
    if (aCategory != CATEGORY_INPUT &&
        aCategory != CATEGORY_OUTPUT &&
        aCategory != CATEGORY_NETWORK &&
        !(aCategory == CATEGORY_CSS && aSeverity == SEVERITY_LOG)) {
      repeatNode = this.document.createElementNS(XHTML_NS, "span");
      repeatNode.setAttribute("value", "1");
      repeatNode.className = "message-repeats";
      repeatNode.textContent = 1;
      repeatNode._uid = [bodyNode.textContent, aCategory, aSeverity, aLevel,
                         aSourceURL, aSourceLine].join(":");
    }

    
    let timestampNode = this.document.createElementNS(XHTML_NS, "span");
    timestampNode.className = "timestamp devtools-monospace";

    let timestampString = l10n.timestampString(timestamp);
    timestampNode.textContent = timestampString + " ";

    
    
    let locationNode;
    if (aSourceURL && IGNORED_SOURCE_URLS.indexOf(aSourceURL) == -1) {
      locationNode = this.createLocationNode({url: aSourceURL,
                                              line: aSourceLine});
    }

    node.appendChild(timestampNode);
    node.appendChild(indentNode);
    node.appendChild(iconContainer);

    
    if (aLevel == "dir") {
      bodyNode.style.height = (this.window.innerHeight *
                               CONSOLE_DIR_VIEW_HEIGHT) + "px";

      let options = {
        objectActor: body.arguments[0],
        targetElement: bodyNode,
        hideFilterInput: true,
      };
      this.jsterm.openVariablesView(options).then((aView) => {
        node._variablesView = aView;
        if (node.classList.contains("hidden-message")) {
          node.classList.remove("hidden-message");
        }
      });

      node.classList.add("inlined-variables-view");
    }

    node.appendChild(bodyNode);
    if (repeatNode) {
      node.appendChild(repeatNode);
    }
    if (locationNode) {
      node.appendChild(locationNode);
    }
    node.appendChild(this.document.createTextNode("\n"));

    return node;
  },

  











  createLocationNode:
  function WCF_createLocationNode({url, line, column}, aTarget)
  {
    if (!url) {
      url = "";
    }
    let locationNode = this.document.createElementNS(XHTML_NS, "a");
    let filenameNode = this.document.createElementNS(XHTML_NS, "span");

    
    
    let filename;
    let fullURL;
    let isScratchpad = false;

    if (/^Scratchpad\/\d+$/.test(url)) {
      filename = url;
      fullURL = url;
      isScratchpad = true;
    }
    else {
      fullURL = url.split(" -> ").pop();
      filename = WebConsoleUtils.abbreviateSourceURL(fullURL);
    }

    filenameNode.className = "filename";
    filenameNode.textContent = " " + (filename || l10n.getStr("unknownLocation"));
    locationNode.appendChild(filenameNode);

    locationNode.href = isScratchpad || !fullURL ? "#" : fullURL;
    locationNode.draggable = false;
    if (aTarget) {
      locationNode.target = aTarget;
    }
    locationNode.setAttribute("title", url);
    locationNode.className = "message-location theme-link devtools-monospace";

    
    let onClick = () => {
      let target = locationNode.target;
      if (target == "scratchpad" || isScratchpad) {
        this.owner.viewSourceInScratchpad(url, line);
        return;
      }

      let category = locationNode.parentNode.category;
      if (target == "styleeditor" || category == CATEGORY_CSS) {
        this.owner.viewSourceInStyleEditor(fullURL, line);
      }
      else if (target == "jsdebugger" ||
               category == CATEGORY_JS || category == CATEGORY_WEBDEV) {
        this.owner.viewSourceInDebugger(fullURL, line);
      }
      else {
        this.owner.viewSource(fullURL, line);
      }
    };

    if (fullURL) {
      this._addMessageLinkCallback(locationNode, onClick);
    }

    if (line) {
      let lineNumberNode = this.document.createElementNS(XHTML_NS, "span");
      lineNumberNode.className = "line-number";
      lineNumberNode.textContent = ":" + line + (column >= 0 ? ":" + column : "");
      locationNode.appendChild(lineNumberNode);
      locationNode.sourceLine = line;
    }

    return locationNode;
  },

  










  setMessageType:
  function WCF_setMessageType(aMessageNode, aCategory, aSeverity)
  {
    aMessageNode.category = aCategory;
    aMessageNode.severity = aSeverity;
    aMessageNode.setAttribute("category", CATEGORY_CLASS_FRAGMENTS[aCategory]);
    aMessageNode.setAttribute("severity", SEVERITY_CLASS_FRAGMENTS[aSeverity]);
    aMessageNode.setAttribute("filter", MESSAGE_PREFERENCE_KEYS[aCategory][aSeverity]);
  },

  








  _addMessageLinkCallback: function WCF__addMessageLinkCallback(aNode, aCallback)
  {
    aNode.addEventListener("mousedown", (aEvent) => {
      this._mousedown = true;
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    aNode.addEventListener("click", (aEvent) => {
      let mousedown = this._mousedown;
      this._mousedown = false;

      aEvent.preventDefault();

      
      if (aEvent.detail != 1 || aEvent.button != 0) {
        return;
      }

      
      
      if (mousedown &&
          (this._startX != aEvent.clientX) &&
          (this._startY != aEvent.clientY))
      {
        this._startX = this._startY = undefined;
        return;
      }

      this._startX = this._startY = undefined;

      aCallback.call(this, aEvent);
    }, false);
  },

  _addFocusCallback: function WCF__addFocusCallback(aNode, aCallback)
  {
    aNode.addEventListener("mousedown", (aEvent) => {
      this._mousedown = true;
      this._startX = aEvent.clientX;
      this._startY = aEvent.clientY;
    }, false);

    aNode.addEventListener("click", (aEvent) => {
      let mousedown = this._mousedown;
      this._mousedown = false;

      
      if (aEvent.detail != 1 || aEvent.button != 0) {
        return;
      }

      
      
      
      
      if (mousedown &&
          (Math.abs(aEvent.clientX - this._startX) >= 2) &&
          (Math.abs(aEvent.clientY - this._startY) >= 1))
      {
        this._startX = this._startY = undefined;
        return;
      }

      this._startX = this._startY = undefined;

      aCallback.call(this, aEvent);
    }, false);
  },

  










  _onToolboxPrefChanged: function WCF__onToolboxPrefChanged(aEvent, aData)
  {
    if (aData.pref == PREF_MESSAGE_TIMESTAMP) {
      if (aData.newValue) {
        this.outputNode.classList.remove("hideTimestamps");
      }
      else {
        this.outputNode.classList.add("hideTimestamps");
      }
    }
  },

  










  copySelectedItems: function WCF_copySelectedItems(aOptions)
  {
    aOptions = aOptions || { linkOnly: false, contextmenu: false };

    
    let strings = [];

    let children = this.output.getSelectedMessages();
    if (!children.length && aOptions.contextmenu) {
      children = [this._contextMenuHandler.lastClickedMessage];
    }

    for (let item of children) {
      
      if (!item.classList.contains("filtered-by-type") &&
          !item.classList.contains("filtered-by-string")) {
        let timestampString = l10n.timestampString(item.timestamp);
        if (aOptions.linkOnly) {
          strings.push(item.url);
        }
        else {
          strings.push(item.clipboardText);
        }
      }
    }

    clipboardHelper.copyString(strings.join("\n"), this.document);
  },

  








  objectPropertiesProvider:
  function WCF_objectPropertiesProvider(aActor, aCallback)
  {
    this.webConsoleClient.inspectObjectProperties(aActor,
      function(aResponse) {
        if (aResponse.error) {
          Cu.reportError("Failed to retrieve the object properties from the " +
                         "server. Error: " + aResponse.error);
          return;
        }
        aCallback(aResponse.properties);
      });
  },

  






  _releaseObject: function WCF__releaseObject(aActor)
  {
    if (this.proxy) {
      this.proxy.releaseActor(aActor);
    }
  },

  


  openSelectedItemInTab: function WCF_openSelectedItemInTab()
  {
    let item = this.output.getSelectedMessages(1)[0] ||
               this._contextMenuHandler.lastClickedMessage;

    if (!item || !item.url) {
      return;
    }

    this.owner.openLink(item.url);
  },

  







  destroy: function WCF_destroy()
  {
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    this._destroyer = promise.defer();

    let toolbox = gDevTools.getToolbox(this.owner.target);
    if (toolbox) {
      toolbox.off("webconsole-selected", this._onPanelSelected);
    }

    gDevTools.off("pref-changed", this._onToolboxPrefChanged);

    this._repeatNodes = {};
    this._outputQueue.forEach(this._destroyItem, this);
    this._outputQueue = [];
    this._itemDestroyQueue.forEach(this._destroyItem, this);
    this._itemDestroyQueue = [];
    this._pruneCategoriesQueue = {};
    this._networkRequests = {};

    if (this._outputTimerInitialized) {
      this._outputTimerInitialized = false;
      this._outputTimer.cancel();
    }
    this._outputTimer = null;
    if (this.jsterm) {
      this.jsterm.destroy();
      this.jsterm = null;
    }
    this.output.destroy();
    this.output = null;

    if (this._contextMenuHandler) {
      this._contextMenuHandler.destroy();
      this._contextMenuHandler = null;
    }

    this._commandController = null;

    let onDestroy = () => {
      this._destroyer.resolve(null);
    };

    if (this.proxy) {
      this.proxy.disconnect().then(onDestroy);
      this.proxy = null;
    }
    else {
      onDestroy();
    }

    return this._destroyer.promise;
  },
};





function simpleValueEvalMacro(aItem, aCurrentString)
{
  return VariablesView.simpleValueEvalMacro(aItem, aCurrentString, "_self");
};





function overrideValueEvalMacro(aItem, aCurrentString)
{
  return VariablesView.overrideValueEvalMacro(aItem, aCurrentString, "_self");
};





function getterOrSetterEvalMacro(aItem, aCurrentString)
{
  return VariablesView.getterOrSetterEvalMacro(aItem, aCurrentString, "_self");
}












function JSTerm(aWebConsoleFrame)
{
  this.hud = aWebConsoleFrame;
  this.hudId = this.hud.hudId;
  this.inputHistoryCount = Services.prefs.getIntPref(PREF_INPUT_HISTORY_COUNT);

  this.lastCompletion = { value: null };
  this._loadHistory();

  this._objectActorsInVariablesViews = new Map();

  this._keyPress = this._keyPress.bind(this);
  this._inputEventHandler = this._inputEventHandler.bind(this);
  this._focusEventHandler = this._focusEventHandler.bind(this);
  this._onKeypressInVariablesView = this._onKeypressInVariablesView.bind(this);
  this._blurEventHandler = this._blurEventHandler.bind(this);

  EventEmitter.decorate(this);
}

JSTerm.prototype = {
  SELECTED_FRAME: -1,

  



  _loadHistory: function() {
    this.history = [];
    this.historyIndex = this.historyPlaceHolder = 0;

    this.historyLoaded = asyncStorage.getItem("webConsoleHistory").then(value => {
      if (Array.isArray(value)) {
        
        
        this.history = value.concat(this.history);

        
        
        this.historyIndex = this.history.length;

        
        
        this.historyPlaceHolder = this.history.length;
      }
    }, console.error);
  },

  






  clearHistory: function() {
    this.history = [];
    this.historyIndex = this.historyPlaceHolder = 0;
    return this.storeHistory();
  },

  




  storeHistory: function() {
    return asyncStorage.setItem("webConsoleHistory", this.history);
  },

  



  lastCompletion: null,

  




  _autocompleteCache: null,

  





  _autocompleteQuery: null,

  





  _lastFrameActorId: null,

  




  sidebar: null,

  




  _variablesView: null,

  






  _lazyVariablesView: true,

  







  _objectActorsInVariablesViews: null,

  



  lastInputValue: "",

  





  _inputChanged: false,

  





  _autocompletePopupNavigated: false,

  



  history: null,
  autocompletePopup: null,
  inputNode: null,
  completeNode: null,

  



  get outputNode() this.hud.outputNode,

  



  get webConsoleClient() this.hud.webConsoleClient,

  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,
  COMPLETE_PAGEUP: 3,
  COMPLETE_PAGEDOWN: 4,

  


  init: function JST_init()
  {
    let autocompleteOptions = {
      onSelect: this.onAutocompleteSelect.bind(this),
      onClick: this.acceptProposedCompletion.bind(this),
      panelId: "webConsole_autocompletePopup",
      listBoxId: "webConsole_autocompletePopupListBox",
      position: "before_start",
      theme: "auto",
      direction: "ltr",
      autoSelect: true
    };
    this.autocompletePopup = new AutocompletePopup(this.hud.document,
                                                   autocompleteOptions);

    let doc = this.hud.document;
    let inputContainer = doc.querySelector(".jsterm-input-container");
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");

    if (this.hud.owner._browserConsole &&
        !Services.prefs.getBoolPref("devtools.chrome.enabled")) {
      inputContainer.style.display = "none";
    }
    else {
      let okstring = l10n.getStr("selfxss.okstring");
      let msg = l10n.getFormatStr("selfxss.msg", [okstring]);
      this._onPaste = WebConsoleUtils.pasteHandlerGen(this.inputNode,
                                                      doc.getElementById("webconsole-notificationbox"),
                                                      msg, okstring);
      this.inputNode.addEventListener("keypress", this._keyPress, false);
      this.inputNode.addEventListener("paste", this._onPaste);
      this.inputNode.addEventListener("drop", this._onPaste);
      this.inputNode.addEventListener("input", this._inputEventHandler, false);
      this.inputNode.addEventListener("keyup", this._inputEventHandler, false);
      this.inputNode.addEventListener("focus", this._focusEventHandler, false);
    }

    this.hud.window.addEventListener("blur", this._blurEventHandler, false);
    this.lastInputValue && this.setInputValue(this.lastInputValue);
  },

  












  _executeResultCallback:
  function JST__executeResultCallback(aAfterMessage, aCallback, aResponse)
  {
    if (!this.hud) {
      return;
    }
    if (aResponse.error) {
      Cu.reportError("Evaluation error " + aResponse.error + ": " +
                     aResponse.message);
      return;
    }
    let errorMessage = aResponse.exceptionMessage;
    let result = aResponse.result;
    let helperResult = aResponse.helperResult;
    let helperHasRawOutput = !!(helperResult || {}).rawOutput;

    if (helperResult && helperResult.type) {
      switch (helperResult.type) {
        case "clearOutput":
          this.clearOutput();
          break;
        case "clearHistory":
          this.clearHistory();
          break;
        case "inspectObject":
          if (aAfterMessage) {
            if (!aAfterMessage._objectActors) {
              aAfterMessage._objectActors = new Set();
            }
            aAfterMessage._objectActors.add(helperResult.object.actor);
          }
          this.openVariablesView({
            label: VariablesView.getString(helperResult.object, { concise: true }),
            objectActor: helperResult.object,
          });
          break;
        case "error":
          try {
            errorMessage = l10n.getStr(helperResult.message);
          }
          catch (ex) {
            errorMessage = helperResult.message;
          }
          break;
        case "help":
          this.hud.owner.openLink(HELP_URL);
          break;
        case "copyValueToClipboard":
          clipboardHelper.copyString(helperResult.value);
          break;
      }
    }

    
    if (!errorMessage && result && typeof result == "object" &&
        result.type == "undefined" &&
        helperResult && !helperHasRawOutput) {
      aCallback && aCallback();
      return;
    }

    let msg = new Messages.JavaScriptEvalOutput(aResponse, errorMessage);
    this.hud.output.addMessage(msg);

    if (aCallback) {
      let oldFlushCallback = this.hud._flushCallback;
      this.hud._flushCallback = () => {
        aCallback(msg.element);
        if (oldFlushCallback) {
          oldFlushCallback();
          this.hud._flushCallback = oldFlushCallback;
          return true;
        }

        return false;
      };
    }

    msg._afterMessage = aAfterMessage;
    msg._objectActors = new Set();

    if (WebConsoleUtils.isActorGrip(aResponse.exception)) {
      msg._objectActors.add(aResponse.exception.actor);
    }

    if (WebConsoleUtils.isActorGrip(result)) {
      msg._objectActors.add(result.actor);
    }
  },

  











  execute: function JST_execute(aExecuteString, aCallback)
  {
    let deferred = promise.defer();
    let callback = function(msg) {
      deferred.resolve(msg);
      if (aCallback) {
        aCallback(msg);
      }
    }

    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      return;
    }

    let selectedNodeActor = null;
    let inspectorSelection = this.hud.owner.getInspectorSelection();
    if (inspectorSelection && inspectorSelection.nodeFront) {
      selectedNodeActor = inspectorSelection.nodeFront.actorID;
    }

    let message = new Messages.Simple(aExecuteString, {
      category: "input",
      severity: "log",
    });
    this.hud.output.addMessage(message);
    let onResult = this._executeResultCallback.bind(this, message, callback);

    let options = {
      frame: this.SELECTED_FRAME,
      selectedNodeActor: selectedNodeActor,
    };

    this.requestEvaluation(aExecuteString, options).then(onResult, onResult);

    
    
    
    this.history[this.historyIndex++] = aExecuteString;
    this.historyPlaceHolder = this.history.length;

    if (this.history.length > this.inputHistoryCount) {
      this.history.splice(0, this.history.length - this.inputHistoryCount);
      this.historyIndex = this.historyPlaceHolder = this.history.length;
    }
    this.storeHistory();
    WebConsoleUtils.usageCount++;
    this.setInputValue("");
    this.clearCompletion();
    return deferred.promise;
  },

  























  requestEvaluation: function JST_requestEvaluation(aString, aOptions = {})
  {
    let deferred = promise.defer();

    function onResult(aResponse) {
      if (!aResponse.error) {
        deferred.resolve(aResponse);
      }
      else {
        deferred.reject(aResponse);
      }
    }

    let frameActor = null;
    if ("frame" in aOptions) {
      frameActor = this.getFrameActor(aOptions.frame);
    }

    let evalOptions = {
      bindObjectActor: aOptions.bindObjectActor,
      frameActor: frameActor,
      selectedNodeActor: aOptions.selectedNodeActor,
    };

    this.webConsoleClient.evaluateJSAsync(aString, onResult, evalOptions);
    return deferred.promise;
  },

  







  getFrameActor: function JST_getFrameActor(aFrame)
  {
    let state = this.hud.owner.getDebuggerFrames();
    if (!state) {
      return null;
    }

    let grip;
    if (aFrame == this.SELECTED_FRAME) {
      grip = state.frames[state.selected];
    }
    else {
      grip = state.frames[aFrame];
    }

    return grip ? grip.actor : null;
  },

  




















  openVariablesView: function JST_openVariablesView(aOptions)
  {
    let onContainerReady = (aWindow) => {
      let container = aWindow.document.querySelector("#variables");
      let view = this._variablesView;
      if (!view || aOptions.targetElement) {
        let viewOptions = {
          container: container,
          hideFilterInput: aOptions.hideFilterInput,
        };
        view = this._createVariablesView(viewOptions);
        if (!aOptions.targetElement) {
          this._variablesView = view;
          aWindow.addEventListener("keypress", this._onKeypressInVariablesView);
        }
      }
      aOptions.view = view;
      this._updateVariablesView(aOptions);

      if (!aOptions.targetElement && aOptions.autofocus) {
        aWindow.focus();
      }

      this.emit("variablesview-open", view, aOptions);
      return view;
    };

    let openPromise;
    if (aOptions.targetElement) {
      let deferred = promise.defer();
      openPromise = deferred.promise;
      let document = aOptions.targetElement.ownerDocument;
      let iframe = document.createElementNS(XHTML_NS, "iframe");

      iframe.addEventListener("load", function onIframeLoad(aEvent) {
        iframe.removeEventListener("load", onIframeLoad, true);
        iframe.style.visibility = "visible";
        deferred.resolve(iframe.contentWindow);
      }, true);

      iframe.flex = 1;
      iframe.style.visibility = "hidden";
      iframe.setAttribute("src", VARIABLES_VIEW_URL);
      aOptions.targetElement.appendChild(iframe);
    }
    else {
      if (!this.sidebar) {
        this._createSidebar();
      }
      openPromise = this._addVariablesViewSidebarTab();
    }

    return openPromise.then(onContainerReady);
  },

  





  _createSidebar: function JST__createSidebar()
  {
    let tabbox = this.hud.document.querySelector("#webconsole-sidebar");
    this.sidebar = new ToolSidebar(tabbox, this, "webconsole");
    this.sidebar.show();
  },

  






  _addVariablesViewSidebarTab: function JST__addVariablesViewSidebarTab()
  {
    let deferred = promise.defer();

    let onTabReady = () => {
      let window = this.sidebar.getWindowForTab("variablesview");
      deferred.resolve(window);
    };

    let tabPanel = this.sidebar.getTabPanel("variablesview");
    if (tabPanel) {
      if (this.sidebar.getCurrentTabID() == "variablesview") {
        onTabReady();
      }
      else {
        this.sidebar.once("variablesview-selected", onTabReady);
        this.sidebar.select("variablesview");
      }
    }
    else {
      this.sidebar.once("variablesview-ready", onTabReady);
      this.sidebar.addTab("variablesview", VARIABLES_VIEW_URL, true);
    }

    return deferred.promise;
  },

  







  _onKeypressInVariablesView: function JST__onKeypressInVariablesView(aEvent)
  {
    let tag = aEvent.target.nodeName;
    if (aEvent.keyCode != Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE || aEvent.shiftKey ||
        aEvent.altKey || aEvent.ctrlKey || aEvent.metaKey ||
        ["input", "textarea", "select", "textbox"].indexOf(tag) > -1) {
        return;
    }

    this._sidebarDestroy();
    this.inputNode.focus();
    aEvent.stopPropagation();
  },

  











  _createVariablesView: function JST__createVariablesView(aOptions)
  {
    let view = new VariablesView(aOptions.container);
    view.toolbox = gDevTools.getToolbox(this.hud.owner.target);
    view.searchPlaceholder = l10n.getStr("propertiesFilterPlaceholder");
    view.emptyText = l10n.getStr("emptyPropertiesList");
    view.searchEnabled = !aOptions.hideFilterInput;
    view.lazyEmpty = this._lazyVariablesView;

    VariablesViewController.attach(view, {
      getEnvironmentClient: aGrip => {
        return new EnvironmentClient(this.hud.proxy.client, aGrip);
      },
      getObjectClient: aGrip => {
        return new ObjectClient(this.hud.proxy.client, aGrip);
      },
      getLongStringClient: aGrip => {
        return this.webConsoleClient.longString(aGrip);
      },
      releaseActor: aActor => {
        this.hud._releaseObject(aActor);
      },
      simpleValueEvalMacro: simpleValueEvalMacro,
      overrideValueEvalMacro: overrideValueEvalMacro,
      getterOrSetterEvalMacro: getterOrSetterEvalMacro,
    });

    
    view.on("fetched", (aEvent, aType, aVar) => {
      this.emit("variablesview-fetched", aVar);
    });

    return view;
  },

  











  _updateVariablesView: function JST__updateVariablesView(aOptions)
  {
    let view = aOptions.view;
    view.empty();

    
    
    view.controller.releaseActors(aActor => {
      return view._consoleLastObjectActor != aActor;
    });

    if (aOptions.objectActor &&
        (!this.hud.owner._browserConsole ||
         Services.prefs.getBoolPref("devtools.chrome.enabled"))) {
      
      view.eval = this._variablesViewEvaluate.bind(this, aOptions);
      view.switch = this._variablesViewSwitch.bind(this, aOptions);
      view.delete = this._variablesViewDelete.bind(this, aOptions);
    }
    else {
      view.eval = null;
      view.switch = null;
      view.delete = null;
    }

    let { variable, expanded } = view.controller.setSingleVariable(aOptions);
    variable.evaluationMacro = simpleValueEvalMacro;

    if (aOptions.objectActor) {
      view._consoleLastObjectActor = aOptions.objectActor.actor;
    }
    else if (aOptions.rawObject) {
      view._consoleLastObjectActor = null;
    }
    else {
      throw new Error("Variables View cannot open without giving it an object " +
                      "display.");
    }

    expanded.then(() => {
      this.emit("variablesview-updated", view, aOptions);
    });
  },

  











  _variablesViewEvaluate:
  function JST__variablesViewEvaluate(aOptions, aVar, aValue)
  {
    let updater = this._updateVariablesView.bind(this, aOptions);
    let onEval = this._silentEvalCallback.bind(this, updater);
    let string = aVar.evaluationMacro(aVar, aValue);

    let evalOptions = {
      frame: this.SELECTED_FRAME,
      bindObjectActor: aOptions.objectActor.actor,
    };

    this.requestEvaluation(string, evalOptions).then(onEval, onEval);
  },

  









  _variablesViewDelete: function JST__variablesViewDelete(aOptions, aVar)
  {
    let onEval = this._silentEvalCallback.bind(this, null);

    let evalOptions = {
      frame: this.SELECTED_FRAME,
      bindObjectActor: aOptions.objectActor.actor,
    };

    this.requestEvaluation("delete _self" + aVar.symbolicName, evalOptions)
        .then(onEval, onEval);
  },

  











  _variablesViewSwitch:
  function JST__variablesViewSwitch(aOptions, aVar, aNewName)
  {
    let updater = this._updateVariablesView.bind(this, aOptions);
    let onEval = this._silentEvalCallback.bind(this, updater);

    let evalOptions = {
      frame: this.SELECTED_FRAME,
      bindObjectActor: aOptions.objectActor.actor,
    };

    let newSymbolicName = aVar.ownerView.symbolicName + '["' + aNewName + '"]';
    if (newSymbolicName == aVar.symbolicName) {
      return;
    }

    let code = "_self" + newSymbolicName + " = _self" + aVar.symbolicName + ";" +
               "delete _self" + aVar.symbolicName;

    this.requestEvaluation(code, evalOptions).then(onEval, onEval);
  },

  













  _silentEvalCallback: function JST__silentEvalCallback(aCallback, aResponse)
  {
    if (aResponse.error) {
      Cu.reportError("Web Console evaluation failed. " + aResponse.error + ":" +
                     aResponse.message);

      aCallback && aCallback(aResponse);
      return;
    }

    if (aResponse.exceptionMessage) {
      let message = new Messages.Simple(aResponse.exceptionMessage, {
        category: "output",
        severity: "error",
        timestamp: aResponse.timestamp,
      });
      this.hud.output.addMessage(message);
      message._objectActors = new Set();
      if (WebConsoleUtils.isActorGrip(aResponse.exception)) {
        message._objectActors.add(aResponse.exception.actor);
      }
    }

    let helper = aResponse.helperResult || { type: null };
    let helperGrip = null;
    if (helper.type == "inspectObject") {
      helperGrip = helper.object;
    }

    let grips = [aResponse.result, helperGrip];
    for (let grip of grips) {
      if (WebConsoleUtils.isActorGrip(grip)) {
        this.hud._releaseObject(grip.actor);
      }
    }

    aCallback && aCallback(aResponse);
  },


  








  clearOutput: function JST_clearOutput(aClearStorage)
  {
    let hud = this.hud;
    let outputNode = hud.outputNode;
    let node;
    while ((node = outputNode.firstChild)) {
      hud.removeOutputMessage(node);
    }

    hud.groupDepth = 0;
    hud._outputQueue.forEach(hud._destroyItem, hud);
    hud._outputQueue = [];
    hud._networkRequests = {};
    hud._repeatNodes = {};

    if (aClearStorage) {
      this.webConsoleClient.clearMessagesCache();
    }

    this.emit("messages-cleared");
  },

  




  clearPrivateMessages: function JST_clearPrivateMessages()
  {
    let nodes = this.hud.outputNode.querySelectorAll(".message[private]");
    for (let node of nodes) {
      this.hud.removeOutputMessage(node);
    }
    this.emit("private-messages-cleared");
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
    this._inputChanged = true;
  },

  



  _inputEventHandler: function JST__inputEventHandler()
  {
    if (this.lastInputValue != this.inputNode.value) {
      this.resizeInput();
      this.complete(this.COMPLETE_HINT_ONLY);
      this.lastInputValue = this.inputNode.value;
      this._inputChanged = true;
    }
  },

  



  _blurEventHandler: function JST__blurEventHandler()
  {
    if (this.autocompletePopup) {
      this.clearCompletion();
    }
  },

  





  _keyPress: function JST__keyPress(aEvent)
  {
    let inputNode = this.inputNode;
    let inputUpdated = false;

    if (aEvent.ctrlKey) {
      switch (aEvent.charCode) {
        case 101:
          
          if (Services.appinfo.OS == "WINNT") {
            break;
          }
          let lineEndPos = inputNode.value.length;
          if (this.hasMultilineInput()) {
            
            for (let i = inputNode.selectionEnd; i<lineEndPos; i++) {
              if (inputNode.value.charAt(i) == "\r" ||
                  inputNode.value.charAt(i) == "\n") {
                lineEndPos = i;
                break;
              }
            }
          }
          inputNode.setSelectionRange(lineEndPos, lineEndPos);
          aEvent.preventDefault();
          this.clearCompletion();
          break;

        case 110:
          
          
          
          if (Services.appinfo.OS == "Darwin" &&
              this.canCaretGoNext() &&
              this.historyPeruse(HISTORY_FORWARD)) {
            aEvent.preventDefault();
            
            
            
            inputNode.focus();
          }
          this.clearCompletion();
          break;

        case 112:
          
          
          
          if (Services.appinfo.OS == "Darwin" &&
              this.canCaretGoPrevious() &&
              this.historyPeruse(HISTORY_BACK)) {
            aEvent.preventDefault();
            
            
            
            inputNode.focus();
          }
          this.clearCompletion();
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

    switch (aEvent.keyCode) {
      case Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE:
        if (this.autocompletePopup.isOpen) {
          this.clearCompletion();
          aEvent.preventDefault();
          aEvent.stopPropagation();
        }
        else if (this.sidebar) {
          this._sidebarDestroy();
          aEvent.preventDefault();
          aEvent.stopPropagation();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_RETURN:
        if (this._autocompletePopupNavigated &&
            this.autocompletePopup.isOpen &&
            this.autocompletePopup.selectedIndex > -1) {
          this.acceptProposedCompletion();
        }
        else {
          this.execute();
          this._inputChanged = false;
        }
        aEvent.preventDefault();
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_UP:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_BACKWARD);
          if (inputUpdated) {
            this._autocompletePopupNavigated = true;
          }
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
          if (inputUpdated) {
            this._autocompletePopupNavigated = true;
          }
        }
        else if (this.canCaretGoNext()) {
          inputUpdated = this.historyPeruse(HISTORY_FORWARD);
        }
        if (inputUpdated) {
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_PAGEUP);
          if (inputUpdated) {
            this._autocompletePopupNavigated = true;
          }
        }
        else {
          this.hud.outputNode.parentNode.scrollTop =
            Math.max(0,
              this.hud.outputNode.parentNode.scrollTop -
              this.hud.outputNode.parentNode.clientHeight
            );
        }
        aEvent.preventDefault();
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN:
        if (this.autocompletePopup.isOpen) {
          inputUpdated = this.complete(this.COMPLETE_PAGEDOWN);
          if (inputUpdated) {
            this._autocompletePopupNavigated = true;
          }
        }
        else {
          this.hud.outputNode.parentNode.scrollTop =
            Math.min(this.hud.outputNode.parentNode.scrollHeight,
              this.hud.outputNode.parentNode.scrollTop +
              this.hud.outputNode.parentNode.clientHeight
            );
        }
        aEvent.preventDefault();
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_HOME:
        if (this.autocompletePopup.isOpen) {
          this.autocompletePopup.selectedIndex = 0;
          aEvent.preventDefault();
        } else if (this.inputNode.value.length <= 0) {
          this.hud.outputNode.parentNode.scrollTop = 0;
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_END:
        if (this.autocompletePopup.isOpen) {
          this.autocompletePopup.selectedIndex = this.autocompletePopup.itemCount - 1;
          aEvent.preventDefault();
        } else if (this.inputNode.value.length <= 0) {
          this.hud.outputNode.parentNode.scrollTop = this.hud.outputNode.parentNode.scrollHeight;
          aEvent.preventDefault();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_LEFT:
        if (this.autocompletePopup.isOpen || this.lastCompletion.value) {
          this.clearCompletion();
        }
        break;

      case Ci.nsIDOMKeyEvent.DOM_VK_RIGHT: {
        let cursorAtTheEnd = this.inputNode.selectionStart ==
                             this.inputNode.selectionEnd &&
                             this.inputNode.selectionStart ==
                             this.inputNode.value.length;
        let haveSuggestion = this.autocompletePopup.isOpen ||
                             this.lastCompletion.value;
        let useCompletion = cursorAtTheEnd || this._autocompletePopupNavigated;
        if (haveSuggestion && useCompletion &&
            this.complete(this.COMPLETE_HINT_ONLY) &&
            this.lastCompletion.value &&
            this.acceptProposedCompletion()) {
          aEvent.preventDefault();
        }
        if (this.autocompletePopup.isOpen) {
          this.clearCompletion();
        }
        break;
      }
      case Ci.nsIDOMKeyEvent.DOM_VK_TAB:
        
        if (this.complete(this.COMPLETE_HINT_ONLY) &&
            this.lastCompletion &&
            this.acceptProposedCompletion()) {
          aEvent.preventDefault();
        }
        else if (this._inputChanged) {
          this.updateCompleteNode(l10n.getStr("Autocomplete.blank"));
          aEvent.preventDefault();
        }
        break;
      default:
        break;
    }
  },

  



  _focusEventHandler: function JST__focusEventHandler()
  {
    this._inputChanged = false;
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

      
      
      
      
      if (this.historyPlaceHolder+1 == this.historyIndex) {
        this.history[this.historyIndex] = this.inputNode.value || "";
      }

      this.setInputValue(inputVal);
    }
    
    else if (aDirection == HISTORY_FORWARD) {
      if (this.historyPlaceHolder >= (this.history.length-1)) {
        return false;
      }

      let inputVal = this.history[++this.historyPlaceHolder];
      this.setInputValue(inputVal);
    }
    else {
      throw new Error("Invalid argument 0");
    }

    return true;
  },

  





  hasMultilineInput: function JST_hasMultilineInput()
  {
    return /[\r\n]/.test(this.inputNode.value);
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

  
































  complete: function JSTF_complete(aType, aCallback)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    let frameActor = this.getFrameActor(this.SELECTED_FRAME);

    
    if (!inputValue) {
      this.clearCompletion();
      aCallback && aCallback(this);
      this.emit("autocomplete-updated");
      return false;
    }

    
    if (inputNode.selectionStart != inputNode.selectionEnd) {
      this.clearCompletion();
      aCallback && aCallback(this);
      this.emit("autocomplete-updated");
      return false;
    }

    
    if (this.lastCompletion.value != inputValue || frameActor != this._lastFrameActorId) {
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
    else if (aType == this.COMPLETE_PAGEUP) {
      popup.selectPreviousPageItem();
    }
    else if (aType == this.COMPLETE_PAGEDOWN) {
      popup.selectNextPageItem();
    }

    aCallback && aCallback(this);
    this.emit("autocomplete-updated");
    return accepted || popup.itemCount > 0;
  },

  









  _updateCompletionResult:
  function JST__updateCompletionResult(aType, aCallback)
  {
    let frameActor = this.getFrameActor(this.SELECTED_FRAME);
    if (this.lastCompletion.value == this.inputNode.value && frameActor == this._lastFrameActorId) {
      return;
    }

    let requestId = gSequenceId();
    let cursor = this.inputNode.selectionStart;
    let input = this.inputNode.value.substring(0, cursor);
    let cache = this._autocompleteCache;

    
    
    
    

    
    if (!/[a-zA-Z0-9]$/.test(input) || frameActor != this._lastFrameActorId) {
      this._autocompleteQuery = null;
      this._autocompleteCache = null;
    }

    if (this._autocompleteQuery && input.startsWith(this._autocompleteQuery)) {
      let filterBy = input;
      
      let lastNonAlpha = input.match(/[^a-zA-Z0-9_$][a-zA-Z0-9_$]*$/);
      
      
      if (lastNonAlpha) {
        filterBy = input.substring(input.lastIndexOf(lastNonAlpha) + 1);
      }

      let newList = cache.sort().filter(function(l) {
        return l.startsWith(filterBy);
      });

      this.lastCompletion = {
        requestId: null,
        completionType: aType,
        value: null,
      };

      let response = { matches: newList, matchProp: filterBy };
      this._receiveAutocompleteProperties(null, aCallback, response);
      return;
    }

    this._lastFrameActorId = frameActor;

    this.lastCompletion = {
      requestId: requestId,
      completionType: aType,
      value: null,
    };

    let callback = this._receiveAutocompleteProperties.bind(this, requestId,
                                                            aCallback);

    this.webConsoleClient.autocomplete(input, cursor, callback, frameActor);
  },

  












  _receiveAutocompleteProperties:
  function JST__receiveAutocompleteProperties(aRequestId, aCallback, aMessage)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    if (this.lastCompletion.value == inputValue ||
        aRequestId != this.lastCompletion.requestId) {
      return;
    }
    
    let cursor = inputNode.selectionStart;
    let inputUntilCursor = inputValue.substring(0, cursor);

    if (aRequestId != null && /[a-zA-Z0-9.]$/.test(inputUntilCursor)) {
      this._autocompleteCache = aMessage.matches;
      this._autocompleteQuery = inputUntilCursor;
    }

    let matches = aMessage.matches;
    let lastPart = aMessage.matchProp;
    if (!matches.length) {
      this.clearCompletion();
      aCallback && aCallback(this);
      this.emit("autocomplete-updated");
      return;
    }

    let items = matches.reverse().map(function(aMatch) {
      return { preLabel: lastPart, label: aMatch };
    });

    let popup = this.autocompletePopup;
    popup.setItems(items);

    let completionType = this.lastCompletion.completionType;
    this.lastCompletion = {
      value: inputValue,
      matchProp: lastPart,
    };

    if (items.length > 1 && !popup.isOpen) {
      let str = this.inputNode.value.substr(0, this.inputNode.selectionStart);
      let offset = str.length - (str.lastIndexOf("\n") + 1) - lastPart.length;
      let x = offset * this.hud._inputCharWidth;
      popup.openPopup(inputNode, x + this.hud._chevronWidth);
      this._autocompletePopupNavigated = false;
    }
    else if (items.length < 2 && popup.isOpen) {
      popup.hidePopup();
      this._autocompletePopupNavigated = false;
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
    this.emit("autocomplete-updated");
  },

  onAutocompleteSelect: function JSTF_onAutocompleteSelect()
  {
    
    if (this.inputNode.selectionStart != this.inputNode.value.length) {
      return;
    }

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
      this._autocompletePopupNavigated = false;
    }
  },

  






  acceptProposedCompletion: function JSTF_acceptProposedCompletion()
  {
    let updated = false;

    let currentItem = this.autocompletePopup.selectedItem;
    if (currentItem && this.lastCompletion.value) {
      let suffix = currentItem.label.substring(this.lastCompletion.
                                               matchProp.length);
      let cursor = this.inputNode.selectionStart;
      let value = this.inputNode.value;
      this.setInputValue(value.substr(0, cursor) + suffix + value.substr(cursor));
      let newCursor = cursor + suffix.length;
      this.inputNode.selectionStart = this.inputNode.selectionEnd = newCursor;
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


  



  _sidebarDestroy: function JST__sidebarDestroy()
  {
    if (this._variablesView) {
      this._variablesView.controller.releaseActors();
      this._variablesView = null;
    }

    if (this.sidebar) {
      this.sidebar.hide();
      this.sidebar.destroy();
      this.sidebar = null;
    }

    this.emit("sidebar-closed");
  },

  


  destroy: function JST_destroy()
  {
    this._sidebarDestroy();

    this.clearCompletion();
    this.clearOutput();

    this.autocompletePopup.destroy();
    this.autocompletePopup = null;

    let popup = this.hud.owner.chromeWindow.document
                .getElementById("webConsole_autocompletePopup");
    if (popup) {
      popup.parentNode.removeChild(popup);
    }

    if (this._onPaste) {
      this.inputNode.removeEventListener("paste", this._onPaste, false);
      this.inputNode.removeEventListener("drop", this._onPaste, false);
      this._onPaste = null;
    }

    this.inputNode.removeEventListener("keypress", this._keyPress, false);
    this.inputNode.removeEventListener("input", this._inputEventHandler, false);
    this.inputNode.removeEventListener("keyup", this._inputEventHandler, false);
    this.inputNode.removeEventListener("focus", this._focusEventHandler, false);
    this.hud.window.removeEventListener("blur", this._blurEventHandler, false);

    this.hud = null;
  },
};




var Utils = {
  






  scrollToVisible: function Utils_scrollToVisible(aNode)
  {
    aNode.scrollIntoView(false);
  },

  







  isOutputScrolledToBottom: function Utils_isOutputScrolledToBottom(aOutputNode)
  {
    let lastNodeHeight = aOutputNode.lastChild ?
                         aOutputNode.lastChild.clientHeight : 0;
    let scrollNode = aOutputNode.parentNode;
    return scrollNode.scrollTop + scrollNode.clientHeight >=
           scrollNode.scrollHeight - lastNodeHeight / 2;
  },

  








  categoryForScriptError: function Utils_categoryForScriptError(aScriptError)
  {
    let category = aScriptError.category;

    if (/^(?:CSS|Layout)\b/.test(category)) {
      return CATEGORY_CSS;
    }

    switch (category) {
      case "Mixed Content Blocker":
      case "Mixed Content Message":
      case "CSP":
      case "Invalid HSTS Headers":
      case "Invalid HPKP Headers":
      case "SHA-1 Signature":
      case "Insecure Password Field":
      case "SSL":
      case "CORS":
      case "Iframe Sandbox":
      case "Tracking Protection":
        return CATEGORY_SECURITY;

      default:
        return CATEGORY_JS;
    }
  },

  








  logLimitForCategory: function Utils_logLimitForCategory(aCategory)
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
};









function CommandController(aWebConsole)
{
  this.owner = aWebConsole;
}

CommandController.prototype = {
  


  selectAll: function CommandController_selectAll()
  {
    this.owner.output.selectAllMessages();
  },

  


  openURL: function CommandController_openURL()
  {
    this.owner.openSelectedItemInTab();
  },

  copyURL: function CommandController_copyURL()
  {
    this.owner.copySelectedItems({ linkOnly: true, contextmenu: true });
  },

  


  copyLastClicked: function CommandController_copy()
  {
    this.owner.copySelectedItems({ linkOnly: false, contextmenu: true });
  },

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    if (!this.owner || !this.owner.output) {
      return false;
    }
    return this.isCommandEnabled(aCommand);
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    switch (aCommand) {
      case "consoleCmd_openURL":
      case "consoleCmd_copyURL": {
        
        let selectedItem = this.owner.output.getSelectedMessages(1)[0] ||
                           this.owner._contextMenuHandler.lastClickedMessage;
        return selectedItem && "url" in selectedItem;
      }
      case "cmd_copy": {
        
        
        return this.owner._contextMenuHandler.lastClickedMessage &&
              !this.owner.output.getSelectedMessages(1)[0];
      }
      case "consoleCmd_clearOutput":
      case "cmd_selectAll":
      case "cmd_find":
        return true;
      case "cmd_fontSizeEnlarge":
      case "cmd_fontSizeReduce":
      case "cmd_fontSizeReset":
      case "cmd_close":
        return this.owner.owner._browserConsole;
    }
    return false;
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    switch (aCommand) {
      case "consoleCmd_openURL":
        this.openURL();
        break;
      case "consoleCmd_copyURL":
        this.copyURL();
        break;
      case "consoleCmd_clearOutput":
        this.owner.jsterm.clearOutput(true);
        break;
      case "cmd_copy":
        this.copyLastClicked();
        break;
      case "cmd_find":
        this.owner.filterBox.focus();
        break;
      case "cmd_selectAll":
        this.selectAll();
        break;
      case "cmd_fontSizeEnlarge":
        this.owner.changeFontSize("+");
        break;
      case "cmd_fontSizeReduce":
        this.owner.changeFontSize("-");
        break;
      case "cmd_fontSizeReset":
        this.owner.changeFontSize("");
        break;
      case "cmd_close":
        this.owner.window.close();
        break;
    }
  }
};















function WebConsoleConnectionProxy(aWebConsole, aTarget)
{
  this.owner = aWebConsole;
  this.target = aTarget;

  this._onPageError = this._onPageError.bind(this);
  this._onLogMessage = this._onLogMessage.bind(this);
  this._onConsoleAPICall = this._onConsoleAPICall.bind(this);
  this._onNetworkEvent = this._onNetworkEvent.bind(this);
  this._onNetworkEventUpdate = this._onNetworkEventUpdate.bind(this);
  this._onFileActivity = this._onFileActivity.bind(this);
  this._onReflowActivity = this._onReflowActivity.bind(this);
  this._onTabNavigated = this._onTabNavigated.bind(this);
  this._onAttachConsole = this._onAttachConsole.bind(this);
  this._onCachedMessages = this._onCachedMessages.bind(this);
  this._connectionTimeout = this._connectionTimeout.bind(this);
  this._onLastPrivateContextExited = this._onLastPrivateContextExited.bind(this);
}

WebConsoleConnectionProxy.prototype = {
  





  owner: null,

  



  target: null,

  





  client: null,

  





  webConsoleClient: null,

  



  connected: false,

  




  _connectTimer: null,

  _connectDefer: null,
  _disconnecter: null,

  





  _consoleActor: null,

  





  _hasNativeConsoleAPI: false,

  






  connect: function WCCP_connect()
  {
    if (this._connectDefer) {
      return this._connectDefer.promise;
    }

    this._connectDefer = promise.defer();

    let timeout = Services.prefs.getIntPref(PREF_CONNECTION_TIMEOUT);
    this._connectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._connectTimer.initWithCallback(this._connectionTimeout,
                                        timeout, Ci.nsITimer.TYPE_ONE_SHOT);

    let connPromise = this._connectDefer.promise;
    connPromise.then(() => {
      this._connectTimer.cancel();
      this._connectTimer = null;
    }, () => {
      this._connectTimer = null;
    });

    let client = this.client = this.target.client;

    client.addListener("logMessage", this._onLogMessage);
    client.addListener("pageError", this._onPageError);
    client.addListener("consoleAPICall", this._onConsoleAPICall);
    client.addListener("networkEvent", this._onNetworkEvent);
    client.addListener("networkEventUpdate", this._onNetworkEventUpdate);
    client.addListener("fileActivity", this._onFileActivity);
    client.addListener("reflowActivity", this._onReflowActivity);
    client.addListener("lastPrivateContextExited", this._onLastPrivateContextExited);
    this.target.on("will-navigate", this._onTabNavigated);
    this.target.on("navigate", this._onTabNavigated);

    this._consoleActor = this.target.form.consoleActor;
    if (this.target.isTabActor) {
      let tab = this.target.form;
      this.owner.onLocationChange(tab.url, tab.title);
    }
    this._attachConsole();

    return connPromise;
  },

  



  _connectionTimeout: function WCCP__connectionTimeout()
  {
    let error = {
      error: "timeout",
      message: l10n.getStr("connectionTimeout"),
    };

    this._connectDefer.reject(error);
  },

  



  _attachConsole: function WCCP__attachConsole()
  {
    let listeners = ["PageError", "ConsoleAPI", "NetworkActivity",
                     "FileActivity"];
    this.client.attachConsole(this._consoleActor, listeners,
                              this._onAttachConsole);
  },

  









  _onAttachConsole: function WCCP__onAttachConsole(aResponse, aWebConsoleClient)
  {
    if (aResponse.error) {
      Cu.reportError("attachConsole failed: " + aResponse.error + " " +
                     aResponse.message);
      this._connectDefer.reject(aResponse);
      return;
    }

    this.webConsoleClient = aWebConsoleClient;

    this._hasNativeConsoleAPI = aResponse.nativeConsoleAPI;

    let msgs = ["PageError", "ConsoleAPI"];
    this.webConsoleClient.getCachedMessages(msgs, this._onCachedMessages);

    this.owner._updateReflowActivityListener();
  },

  






  _onCachedMessages: function WCCP__onCachedMessages(aResponse)
  {
    if (aResponse.error) {
      Cu.reportError("Web Console getCachedMessages error: " + aResponse.error +
                     " " + aResponse.message);
      this._connectDefer.reject(aResponse);
      return;
    }

    if (!this._connectTimer) {
      
      
      Cu.reportError("Web Console getCachedMessages error: invalid state.");
    }

    this.owner.displayCachedMessages(aResponse.messages);

    if (!this._hasNativeConsoleAPI) {
      this.owner.logWarningAboutReplacedAPI();
    }

    this.connected = true;
    this._connectDefer.resolve(this);
  },

  









  _onPageError: function WCCP__onPageError(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handlePageError(aPacket.pageError);
    }
  },

  









  _onLogMessage: function WCCP__onLogMessage(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handleLogMessage(aPacket);
    }
  },

  









  _onConsoleAPICall: function WCCP__onConsoleAPICall(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handleConsoleAPICall(aPacket.message);
    }
  },

  









  _onNetworkEvent: function WCCP__onNetworkEvent(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handleNetworkEvent(aPacket.eventActor);
    }
  },

  









  _onNetworkEventUpdate: function WCCP__onNetworkEvenUpdatet(aType, aPacket)
  {
    if (this.owner) {
      this.owner.handleNetworkEventUpdate(aPacket.from, aPacket.updateType,
                                          aPacket);
    }
  },

  









  _onFileActivity: function WCCP__onFileActivity(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handleFileActivity(aPacket.uri);
    }
  },

  _onReflowActivity: function WCCP__onReflowActivity(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.handleReflowActivity(aPacket);
    }
  },

  









  _onLastPrivateContextExited:
  function WCCP__onLastPrivateContextExited(aType, aPacket)
  {
    if (this.owner && aPacket.from == this._consoleActor) {
      this.owner.jsterm.clearPrivateMessages();
    }
  },

  









  _onTabNavigated: function WCCP__onTabNavigated(aEvent, aPacket)
  {
    if (!this.owner) {
      return;
    }

    this.owner.handleTabNavigated(aEvent, aPacket);
  },

  





  releaseActor: function WCCP_releaseActor(aActor)
  {
    if (this.client) {
      this.client.release(aActor);
    }
  },

  





  disconnect: function WCCP_disconnect()
  {
    if (this._disconnecter) {
      return this._disconnecter.promise;
    }

    this._disconnecter = promise.defer();

    if (!this.client) {
      this._disconnecter.resolve(null);
      return this._disconnecter.promise;
    }

    this.client.removeListener("logMessage", this._onLogMessage);
    this.client.removeListener("pageError", this._onPageError);
    this.client.removeListener("consoleAPICall", this._onConsoleAPICall);
    this.client.removeListener("networkEvent", this._onNetworkEvent);
    this.client.removeListener("networkEventUpdate", this._onNetworkEventUpdate);
    this.client.removeListener("fileActivity", this._onFileActivity);
    this.client.removeListener("reflowActivity", this._onReflowActivity);
    this.client.removeListener("lastPrivateContextExited", this._onLastPrivateContextExited);
    this.target.off("will-navigate", this._onTabNavigated);
    this.target.off("navigate", this._onTabNavigated);

    this.client = null;
    this.webConsoleClient = null;
    this.target = null;
    this.connected = false;
    this.owner = null;
    this._disconnecter.resolve(null);

    return this._disconnecter.promise;
  },
};

function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;












function ConsoleContextMenu(aOwner)
{
  this.owner = aOwner;
  this.popup = this.owner.document.getElementById("output-contextmenu");
  this.build = this.build.bind(this);
  this.popup.addEventListener("popupshowing", this.build);
}

ConsoleContextMenu.prototype = {
  lastClickedMessage: null,

  


  build: function CCM_build(aEvent)
  {
    let metadata = this.getSelectionMetadata(aEvent.rangeParent);
    for (let element of this.popup.children) {
      element.hidden = this.shouldHideMenuItem(element, metadata);
    }
  },

  







  getSelectionMetadata: function CCM_getSelectionMetadata(aClickElement)
  {
    let metadata = {
      selectionType: "",
      selection: new Set(),
    };
    let selectedItems = this.owner.output.getSelectedMessages();
    if (!selectedItems.length) {
      let clickedItem = this.owner.output.getMessageForElement(aClickElement);
      if (clickedItem) {
        this.lastClickedMessage = clickedItem;
        selectedItems = [clickedItem];
      }
    }

    metadata.selectionType = selectedItems.length > 1 ? "multiple" : "single";

    let selection = metadata.selection;
    for (let item of selectedItems) {
      switch (item.category) {
        case CATEGORY_NETWORK:
          selection.add("network");
          break;
        case CATEGORY_CSS:
          selection.add("css");
          break;
        case CATEGORY_JS:
          selection.add("js");
          break;
        case CATEGORY_WEBDEV:
          selection.add("webdev");
          break;
      }
    }

    return metadata;
  },

  







  shouldHideMenuItem: function CCM_shouldHideMenuItem(aMenuItem, aMetadata)
  {
    let selectionType = aMenuItem.getAttribute("selectiontype");
    if (selectionType && !aMetadata.selectionType == selectionType) {
      return true;
    }

    let selection = aMenuItem.getAttribute("selection");
    if (!selection) {
      return false;
    }

    let shouldHide = true;
    let itemData = selection.split("|");
    for (let type of aMetadata.selection) {
      
      if (itemData.indexOf(type) !== -1) {
        shouldHide = false;
        break;
      }
    }

    return shouldHide;
  },

  


  destroy: function CCM_destroy()
  {
    this.popup.removeEventListener("popupshowing", this.build);
    this.popup = null;
    this.owner = null;
    this.lastClickedMessage = null;
  },
};
