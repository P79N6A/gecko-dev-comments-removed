





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

XPCOMUtils.defineLazyModuleGetter(this, "GripClient",
                                  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkPanel",
                                  "resource:///modules/NetworkPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AutocompletePopup",
                                  "resource:///modules/devtools/AutocompletePopup.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "VariablesView",
                                  "resource:///modules/devtools/VariablesView.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ToolSidebar",
                                  "resource:///modules/devtools/Sidebar.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
                                  "resource:///modules/devtools/EventEmitter.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);



const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const MIXED_CONTENT_LEARN_MORE = "https://developer.mozilla.org/en/Security/MixedContent";

const HELP_URL = "https://developer.mozilla.org/docs/Tools/Web_Console/Helpers";

const VARIABLES_VIEW_URL = "chrome://browser/content/devtools/widgets/VariablesView.xul";

const CONSOLE_DIR_VIEW_HEIGHT = 0.6;

const IGNORED_SOURCE_URLS = ["debugger eval code", "self-hosted"];



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


const HISTORY_BACK = -1;
const HISTORY_FORWARD = 1;


const GROUP_INDENT = 12;



const MESSAGES_IN_INTERVAL = DEFAULT_LOG_LIMIT;




const OUTPUT_INTERVAL = 50; 




const THROTTLE_UPDATES = 1000; 


const FILTER_PREFS_PREFIX = "devtools.webconsole.filter.";


const MIN_FONT_SIZE = 10;

const PREF_CONNECTION_TIMEOUT = "devtools.debugger.remote-timeout";












function WebConsoleFrame(aWebConsoleOwner)
{
  this.owner = aWebConsoleOwner;
  this.hudId = this.owner.hudId;

  this._cssNodes = {};
  this._outputQueue = [];
  this._pruneCategoriesQueue = {};
  this._networkRequests = {};

  this._toggleFilter = this._toggleFilter.bind(this);
  this._flushMessageQueue = this._flushMessageQueue.bind(this);

  this._outputTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._outputTimerInitialized = false;
}

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

  




  _cssNodes: null,

  




  filterPrefs: null,

  


  groupDepth: 0,

  



  contentLocation: "",

  




  jsterm: null,

  



  outputNode: null,

  



  filterBox: null,

  



  get webConsoleClient() this.proxy ? this.proxy.webConsoleClient : null,

  _destroyer: null,

  _saveRequestAndResponseBodies: false,

  




  get saveRequestAndResponseBodies() this._saveRequestAndResponseBodies,

  





  set saveRequestAndResponseBodies(aValue) {
    let newValue = !!aValue;
    let preferences = {
      "NetworkMonitor.saveRequestAndResponseBodies": newValue,
    };

    this.webConsoleClient.setPreferences(preferences, function(aResponse) {
      if (!aResponse.error) {
        this._saveRequestAndResponseBodies = newValue;
      }
    }.bind(this));
  },

  




  init: function WCF_init()
  {
    this._initUI();
    return this._initConnection();
  },

  







  _initConnection: function WCF__initConnection()
  {
    if (this._initDefer) {
      return this._initDefer.promise;
    }

    this._initDefer = Promise.defer();
    this.proxy = new WebConsoleConnectionProxy(this, this.owner.target);

    this.proxy.connect().then(() => { 
      this.saveRequestAndResponseBodies = this._saveRequestAndResponseBodies;
      this._initDefer.resolve(this);
    }, (aReason) => { 
      let node = this.createMessageNode(CATEGORY_JS, SEVERITY_ERROR,
                                        aReason.error + ": " + aReason.message);
      this.outputMessage(CATEGORY_JS, node);
      this._initDefer.reject(aReason);
    }).then(() => {
      let id = WebConsoleUtils.supportsString(this.hudId);
      Services.obs.notifyObservers(id, "web-console-created", null);
    });

    return this._initDefer.promise;
  },

  



  _initUI: function WCF__initUI()
  {
    
    
    this.window = window;
    this.document = this.window.document;
    this.rootElement = this.document.documentElement;

    this._initDefaultFilterPrefs();

    
    this._commandController = new CommandController(this);
    this.window.controllers.insertControllerAt(0, this._commandController);

    let doc = this.document;

    this.filterBox = doc.querySelector(".hud-filter-box");
    this.outputNode = doc.querySelector(".hud-output-node");
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");

    this._setFilterTextBoxEvents();
    this._initFilterButtons();

    let fontSize = Services.prefs.getIntPref("devtools.webconsole.fontSize");

    if (fontSize != 0) {
      fontSize = Math.max(MIN_FONT_SIZE, fontSize);

      this.outputNode.style.fontSize = fontSize + "px";
      this.completeNode.style.fontSize = fontSize + "px";
      this.inputNode.style.fontSize = fontSize + "px";
    }

    let saveBodies = doc.getElementById("saveBodies");
    saveBodies.addEventListener("command", function() {
      this.saveRequestAndResponseBodies = !this.saveRequestAndResponseBodies;
    }.bind(this));
    saveBodies.setAttribute("checked", this.saveRequestAndResponseBodies);
    saveBodies.disabled = !this.getFilterState("networkinfo") &&
                          !this.getFilterState("network");

    saveBodies.parentNode.addEventListener("popupshowing", function() {
      saveBodies.setAttribute("checked", this.saveRequestAndResponseBodies);
      saveBodies.disabled = !this.getFilterState("networkinfo") &&
                            !this.getFilterState("network");
    }.bind(this));

    
    let saveBodiesContextMenu = doc.getElementById("saveBodiesContextMenu");
    saveBodiesContextMenu.addEventListener("command", function() {
      this.saveRequestAndResponseBodies = !this.saveRequestAndResponseBodies;
    }.bind(this));
    saveBodiesContextMenu.setAttribute("checked",
                                       this.saveRequestAndResponseBodies);
    saveBodiesContextMenu.disabled = !this.getFilterState("networkinfo") &&
                                     !this.getFilterState("network");

    saveBodiesContextMenu.parentNode.addEventListener("popupshowing", function() {
      saveBodiesContextMenu.setAttribute("checked",
                                         this.saveRequestAndResponseBodies);
      saveBodiesContextMenu.disabled = !this.getFilterState("networkinfo") &&
                                       !this.getFilterState("network");
    }.bind(this));

    let clearButton = doc.getElementsByClassName("webconsole-clear-console-button")[0];
    clearButton.addEventListener("command", function WCF__onClearButton() {
      this.owner._onClearButton();
      this.jsterm.clearOutput(true);
    }.bind(this));

    this.jsterm = new JSTerm(this);
    this.jsterm.init();
    this.jsterm.inputNode.focus();
  },

  



  _initDefaultFilterPrefs: function WCF__initDefaultFilterPrefs()
  {
    this.filterPrefs = {
      network: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "network"),
      networkinfo: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "networkinfo"),
      csserror: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "csserror"),
      cssparser: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "cssparser"),
      exception: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "exception"),
      jswarn: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "jswarn"),
      error: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "error"),
      info: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "info"),
      warn: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "warn"),
      log: Services.prefs.getBoolPref(FILTER_PREFS_PREFIX + "log"),
    };
  },

  



  _setFilterTextBoxEvents: function WCF__setFilterTextBoxEvents()
  {
    let timer = null;
    let timerEvent = this.adjustVisibilityOnSearchStringChange.bind(this);

    let onChange = function _onChange() {
      let timer;

      
      
      if (timer == null) {
        let timerClass = Cc["@mozilla.org/timer;1"];
        timer = timerClass.createInstance(Ci.nsITimer);
      }
      else {
        timer.cancel();
      }

      timer.initWithCallback(timerEvent, SEARCH_DELAY,
                             Ci.nsITimer.TYPE_ONE_SHOT);
    }.bind(this);

    this.filterBox.addEventListener("command", onChange, false);
    this.filterBox.addEventListener("input", onChange, false);
  },

  










  _initFilterButtons: function WCF__initFilterButtons()
  {
    let categories = this.document
                     .querySelectorAll(".webconsole-filter-button[category]");
    Array.forEach(categories, function(aButton) {
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
    }, this);
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
  },

  







  _toggleFilter: function WCF__toggleFilter(aEvent)
  {
    let target = aEvent.target;
    let tagName = target.tagName;
    if (tagName != aEvent.currentTarget.tagName) {
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
        target.setAttribute("checked", state);

        
        
        
        let menuItems = target.querySelectorAll("menuitem");
        for (let i = 0; i < menuItems.length; i++) {
          menuItems[i].setAttribute("checked", state);
          let prefKey = menuItems[i].getAttribute("prefKey");
          this.setFilterState(prefKey, state);
        }
        break;
      }

      case "menuitem": {
        let state = target.getAttribute("checked") !== "true";
        target.setAttribute("checked", state);

        let prefKey = target.getAttribute("prefKey");
        this.setFilterState(prefKey, state);

        
        if (prefKey == "networkinfo" || prefKey == "network") {
          let checkState = !this.getFilterState("networkinfo") &&
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
        break;
      }
    }
  },

  






  setFilterState: function WCF_setFilterState(aToggleType, aState)
  {
    this.filterPrefs[aToggleType] = aState;
    this.adjustVisibilityForMessageType(aToggleType, aState);
    Services.prefs.setBoolPref(FILTER_PREFS_PREFIX + aToggleType, aState);
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

    this.regroupOutput();
  },

  



  adjustVisibilityOnSearchStringChange:
  function WCF_adjustVisibilityOnSearchStringChange()
  {
    let nodes = this.outputNode.getElementsByClassName("hud-msg-node");
    let searchString = this.filterBox.value;

    for (let i = 0, n = nodes.length; i < n; ++i) {
      let node = nodes[i];

      
      let text = node.clipboardText;

      
      if (this.stringMatchesFilters(text, searchString)) {
        node.classList.remove("hud-filtered-by-string");
      }
      else {
        node.classList.add("hud-filtered-by-string");
      }
    }

    this.regroupOutput();
  },

  








  filterMessageNode: function WCF_filterMessageNode(aNode)
  {
    let isFiltered = false;

    
    let prefKey = MESSAGE_PREFERENCE_KEYS[aNode.category][aNode.severity];
    if (prefKey && !this.getFilterState(prefKey)) {
      
      aNode.classList.add("hud-filtered-by-type");
      isFiltered = true;
    }

    
    let search = this.filterBox.value;
    let text = aNode.clipboardText;

    
    if (!this.stringMatchesFilters(text, search)) {
      aNode.classList.add("hud-filtered-by-string");
      isFiltered = true;
    }

    return isFiltered;
  },

  








  mergeFilteredMessageNode:
  function WCF_mergeFilteredMessageNode(aOriginal, aFiltered)
  {
    
    
    let repeatNode = aOriginal.childNodes[3].firstChild;
    if (!repeatNode) {
      return; 
    }

    let occurrences = parseInt(repeatNode.getAttribute("value")) + 1;
    repeatNode.setAttribute("value", occurrences);
  },

  








  _filterRepeatedMessage: function WCF__filterRepeatedMessage(aNode)
  {
    let repeatNode = aNode.getElementsByClassName("webconsole-msg-repeat")[0];
    if (!repeatNode) {
      return false;
    }

    let uid = repeatNode._uid;
    let dupeNode = null;

    if (aNode.classList.contains("webconsole-msg-cssparser")) {
      dupeNode = this._cssNodes[uid];
      if (!dupeNode) {
        this._cssNodes[uid] = aNode;
      }
    }
    else if (!aNode.classList.contains("webconsole-msg-network") &&
             !aNode.classList.contains("webconsole-msg-inspector") &&
             (aNode.classList.contains("webconsole-msg-console") ||
              aNode.classList.contains("webconsole-msg-exception") ||
              aNode.classList.contains("webconsole-msg-error"))) {
      let lastMessage = this.outputNode.lastChild;
      if (!lastMessage) {
        return false;
      }

      let lastRepeatNode = lastMessage
                           .getElementsByClassName("webconsole-msg-repeat")[0];
      if (lastRepeatNode && lastRepeatNode._uid == uid) {
        dupeNode = lastMessage;
      }
    }

    if (dupeNode) {
      this.mergeFilteredMessageNode(dupeNode, aNode);
      return true;
    }

    return false;
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
      case "debug":
      case "dir": {
        body = { arguments: args };
        let clipboardArray = [];
        args.forEach((aValue) => {
          clipboardArray.push(VariablesView.getString(aValue));
          if (aValue && typeof aValue == "object" &&
              aValue.type == "longString") {
            clipboardArray.push(l10n.getStr("longStringEllipsis"));
          }
        });
        clipboardText = clipboardArray.join(" ");
        break;
      }

      case "trace": {
        let filename = WebConsoleUtils.abbreviateSourceURL(aMessage.filename);
        let functionName = aMessage.functionName ||
                           l10n.getStr("stacktrace.anonymousFunction");

        body = l10n.getFormatStr("stacktrace.outputMessage",
                                 [filename, functionName, sourceLine]);

        clipboardText = "";

        aMessage.stacktrace.forEach(function(aFrame) {
          clipboardText += aFrame.filename + " :: " +
                           aFrame.functionName + " :: " +
                           aFrame.lineNumber + "\n";
        });

        clipboardText = clipboardText.trimRight();
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
          return;
        }
        if (timer.error) {
          Cu.reportError(l10n.getStr(timer.error));
          return;
        }
        body = l10n.getFormatStr("timerStarted", [timer.name]);
        clipboardText = body;
        break;
      }

      case "timeEnd": {
        let timer = aMessage.timer;
        if (!timer) {
          return;
        }
        body = l10n.getFormatStr("timeEnd", [timer.name, timer.duration]);
        clipboardText = body;
        break;
      }

      default:
        Cu.reportError("Unknown Console API log level: " + level);
        return;
    }

    
    
    switch (level) {
      case "group":
      case "groupCollapsed":
      case "groupEnd":
      case "trace":
      case "time":
      case "timeEnd":
        for (let actor of objectActors) {
          this._releaseObject(actor);
        }
        objectActors.clear();
    }

    if (level == "groupEnd") {
      return; 
    }

    let node = this.createMessageNode(CATEGORY_WEBDEV, LEVELS[level], body,
                                      sourceURL, sourceLine, clipboardText,
                                      level, aMessage.timeStamp);

    if (objectActors.size > 0) {
      node._objectActors = objectActors;
    }

    
    
    if (level == "trace") {
      node._stacktrace = aMessage.stacktrace;

      this.makeOutputMessageLink(node, () =>
        this.jsterm.openVariablesView({ rawObject: node._stacktrace }));
    }

    return node;
  },

  






  handleConsoleAPICall: function WCF_handleConsoleAPICall(aMessage)
  {
    this.outputMessage(CATEGORY_WEBDEV, this.logConsoleAPIMessage, [aMessage]);
  },

  










  _consoleLogClick: function WCF__consoleLogClick(aAnchor, aObjectActor)
  {
    let options = {
      label: aAnchor.textContent,
      objectActor: aObjectActor,
    };
    this.jsterm.openVariablesView(options);
  },

  







  reportPageError: function WCF_reportPageError(aCategory, aScriptError)
  {
    
    
    let severity = SEVERITY_ERROR;
    if (aScriptError.warning || aScriptError.strict) {
      severity = SEVERITY_WARNING;
    }

    let node = this.createMessageNode(aCategory, severity,
                                      aScriptError.errorMessage,
                                      aScriptError.sourceName,
                                      aScriptError.lineNumber, null, null,
                                      aScriptError.timeStamp);
    return node;
  },

  






  handlePageError: function WCF_handlePageError(aPageError)
  {
    let category = Utils.categoryForScriptError(aPageError);
    this.outputMessage(category, this.reportPageError, [category, aPageError]);
  },

  







  logNetEvent: function WCF_logNetEvent(aActorId)
  {
    let networkInfo = this._networkRequests[aActorId];
    if (!networkInfo) {
      return;
    }

    let request = networkInfo.request;

    let msgNode = this.document.createElementNS(XUL_NS, "hbox");

    let methodNode = this.document.createElementNS(XUL_NS, "label");
    methodNode.setAttribute("value", request.method);
    methodNode.classList.add("webconsole-msg-body-piece");
    msgNode.appendChild(methodNode);

    let linkNode = this.document.createElementNS(XUL_NS, "hbox");
    linkNode.flex = 1;
    linkNode.classList.add("webconsole-msg-body-piece");
    linkNode.classList.add("webconsole-msg-link");
    msgNode.appendChild(linkNode);

    let urlNode = this.document.createElementNS(XUL_NS, "label");
    urlNode.flex = 1;
    urlNode.setAttribute("crop", "center");
    urlNode.setAttribute("title", request.url);
    urlNode.setAttribute("tooltiptext", request.url);
    urlNode.setAttribute("value", request.url);
    urlNode.classList.add("hud-clickable");
    urlNode.classList.add("webconsole-msg-body-piece");
    urlNode.classList.add("webconsole-msg-url");
    linkNode.appendChild(urlNode);

    let severity = SEVERITY_LOG;
    let mixedRequest =
      WebConsoleUtils.isMixedHTTPSRequest(request.url, this.contentLocation);
    if (mixedRequest) {
      urlNode.classList.add("webconsole-mixed-content");
      this.makeMixedContentNode(linkNode);
      
      
      severity = SEVERITY_WARNING;
    }

    let statusNode = this.document.createElementNS(XUL_NS, "label");
    statusNode.setAttribute("value", "");
    statusNode.classList.add("hud-clickable");
    statusNode.classList.add("webconsole-msg-body-piece");
    statusNode.classList.add("webconsole-msg-status");
    linkNode.appendChild(statusNode);

    let clipboardText = request.method + " " + request.url;

    let messageNode = this.createMessageNode(CATEGORY_NETWORK, severity,
                                             msgNode, null, null, clipboardText);

    messageNode._connectionId = aActorId;
    messageNode.url = request.url;

    this.makeOutputMessageLink(messageNode, function WCF_net_message_link() {
      if (!messageNode._panelOpen) {
        this.openNetworkPanel(messageNode, networkInfo);
      }
    }.bind(this));

    networkInfo.node = messageNode;

    this._updateNetMessage(aActorId);

    return messageNode;
  },

  





  makeMixedContentNode: function WCF_makeMixedContentNode(aLinkNode)
  {
    let mixedContentWarning = "[" + l10n.getStr("webConsoleMixedContentWarning") + "]";

    
    let mixedContentWarningNode = this.document.createElement("label");
    mixedContentWarningNode.setAttribute("value", mixedContentWarning);
    mixedContentWarningNode.setAttribute("title", mixedContentWarning);
    mixedContentWarningNode.classList.add("hud-clickable");
    mixedContentWarningNode.classList.add("webconsole-mixed-content-link");

    aLinkNode.appendChild(mixedContentWarningNode);

    mixedContentWarningNode.addEventListener("click", function(aEvent) {
      this.owner.openLink(MIXED_CONTENT_LEARN_MORE);
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }.bind(this));
  },

  







  logFileActivity: function WCF_logFileActivity(aFileURI)
  {
    let urlNode = this.document.createElementNS(XUL_NS, "label");
    urlNode.flex = 1;
    urlNode.setAttribute("crop", "center");
    urlNode.setAttribute("title", aFileURI);
    urlNode.setAttribute("tooltiptext", aFileURI);
    urlNode.setAttribute("value", aFileURI);
    urlNode.classList.add("hud-clickable");
    urlNode.classList.add("webconsole-msg-url");

    let outputNode = this.createMessageNode(CATEGORY_NETWORK, SEVERITY_LOG,
                                            urlNode, null, null, aFileURI);

    this.makeOutputMessageLink(outputNode, function WCF__onFileClick() {
      this.owner.viewSource(aFileURI);
    }.bind(this));

    return outputNode;
  },

  





  handleFileActivity: function WCF_handleFileActivity(aFileURI)
  {
    this.outputMessage(CATEGORY_NETWORK, this.logFileActivity, [aFileURI]);
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
      response: {},
      timings: {},
      updates: [], 
    };

    this._networkRequests[aActor.actor] = networkInfo;
    this.outputMessage(CATEGORY_NETWORK, this.logNetEvent, [aActor.actor]);
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

    if (networkInfo.node) {
      this._updateNetMessage(aActorId);
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
    let response = networkInfo.response;

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

      let linkNode = messageNode.querySelector(".webconsole-msg-link");
      let statusNode = linkNode.querySelector(".webconsole-msg-status");
      statusNode.setAttribute("value", statusText);

      messageNode.clipboardText = [request.method, request.url, statusText]
                                  .join(" ");

      if (hasResponseStart && response.status >= MIN_HTTP_ERROR_CODE &&
          response.status <= MAX_HTTP_ERROR_CODE) {
        this.setMessageType(messageNode, CATEGORY_NETWORK, SEVERITY_ERROR);
      }
    }

    if (messageNode._netPanel) {
      messageNode._netPanel.update();
    }
  },

  










  openNetworkPanel: function WCF_openNetworkPanel(aNode, aHttpActivity)
  {
    let actor = aHttpActivity.actor;

    if (actor) {
      this.webConsoleClient.getRequestHeaders(actor, function(aResponse) {
        if (aResponse.error) {
          Cu.reportError("WCF_openNetworkPanel getRequestHeaders:" +
                         aResponse.error);
          return;
        }

        aHttpActivity.request.headers = aResponse.headers;

        this.webConsoleClient.getRequestCookies(actor, onRequestCookies);
      }.bind(this));
    }

    let onRequestCookies = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getRequestCookies:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.request.cookies = aResponse.cookies;

      this.webConsoleClient.getResponseHeaders(actor, onResponseHeaders);
    }.bind(this);

    let onResponseHeaders = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseHeaders:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.headers = aResponse.headers;

      this.webConsoleClient.getResponseCookies(actor, onResponseCookies);
    }.bind(this);

    let onResponseCookies = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseCookies:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.cookies = aResponse.cookies;

      this.webConsoleClient.getRequestPostData(actor, onRequestPostData);
    }.bind(this);

    let onRequestPostData = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getRequestPostData:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.request.postData = aResponse.postData;
      aHttpActivity.discardRequestBody = aResponse.postDataDiscarded;

      this.webConsoleClient.getResponseContent(actor, onResponseContent);
    }.bind(this);

    let onResponseContent = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getResponseContent:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.response.content = aResponse.content;
      aHttpActivity.discardResponseBody = aResponse.contentDiscarded;

      this.webConsoleClient.getEventTimings(actor, onEventTimings);
    }.bind(this);

    let onEventTimings = function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("WCF_openNetworkPanel getEventTimings:" +
                       aResponse.error);
        return;
      }

      aHttpActivity.timings = aResponse.timings;

      openPanel();
    }.bind(this);

    let openPanel = function() {
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
    }.bind(this);

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

  

















  outputMessage: function WCF_outputMessage(aCategory, aMethodOrNode, aArguments)
  {
    if (!this._outputQueue.length) {
      
      
      this._lastOutputFlush = Date.now();
    }

    this._outputQueue.push([aCategory, aMethodOrNode, aArguments]);

    if (!this._outputTimerInitialized) {
      this._initOutputTimer();
    }
  },

  






  _flushMessageQueue: function WCF__flushMessageQueue()
  {
    if (!this._outputTimer) {
      return;
    }

    let timeSinceFlush = Date.now() - this._lastOutputFlush;
    if (this._outputQueue.length > MESSAGES_IN_INTERVAL &&
        timeSinceFlush < THROTTLE_UPDATES) {
      this._initOutputTimer();
      return;
    }

    
    let toDisplay = Math.min(this._outputQueue.length, MESSAGES_IN_INTERVAL);
    if (toDisplay < 1) {
      this._outputTimerInitialized = false;
      return;
    }

    
    let shouldPrune = false;
    if (this._outputQueue.length > toDisplay && this._pruneOutputQueue()) {
      toDisplay = Math.min(this._outputQueue.length, toDisplay);
      shouldPrune = true;
    }

    let batch = this._outputQueue.splice(0, toDisplay);
    if (!batch.length) {
      this._outputTimerInitialized = false;
      return;
    }

    let outputNode = this.outputNode;
    let lastVisibleNode = null;
    let scrolledToBottom = Utils.isOutputScrolledToBottom(outputNode);
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
        removedNodes += this.pruneOutputIfNecessary(aCategory);
      }, this);
      this._pruneCategoriesQueue = {};
    }

    
    if (!this._outputQueue.length) {
      this.regroupOutput();
    }

    let isInputOutput = lastVisibleNode &&
      (lastVisibleNode.classList.contains("webconsole-msg-input") ||
       lastVisibleNode.classList.contains("webconsole-msg-output"));

    
    
    
    if (lastVisibleNode && (scrolledToBottom || isInputOutput)) {
      Utils.scrollToVisible(lastVisibleNode);
    }
    else if (!scrolledToBottom && removedNodes > 0 &&
             oldScrollHeight != scrollBox.scrollHeight) {
      
      
      scrollBox.scrollTop -= oldScrollHeight - scrollBox.scrollHeight;
    }

    
    if (this._outputQueue.length > 0) {
      this._initOutputTimer();
    }
    else {
      this._outputTimerInitialized = false;
      this._flushCallback && this._flushCallback();
    }

    this._lastOutputFlush = Date.now();
  },

  



  _initOutputTimer: function WCF__initOutputTimer()
  {
    if (!this._outputTimer) {
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

    let isFiltered = this.filterMessageNode(node);

    let isRepeated = this._filterRepeatedMessage(node);

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
          this._pruneItemFromQueue(this._outputQueue[indexes[i]]);
          this._outputQueue.splice(indexes[i], 1);
        }
      }
    }

    return pruned;
  },

  






  _pruneItemFromQueue: function WCF__pruneItemFromQueue(aItem)
  {
    let [category, methodOrNode, args] = aItem;
    if (typeof methodOrNode != "function" && methodOrNode._objectActors) {
      for (let actor of methodOrNode._objectActors) {
        this._releaseObject(actor);
      }
      methodOrNode._objectActors.clear();
    }

    if (category == CATEGORY_NETWORK) {
      let connectionId = null;
      if (methodOrNode == this.logNetEvent) {
        connectionId = args[0];
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
  },

  








  pruneOutputIfNecessary: function WCF_pruneOutputIfNecessary(aCategory)
  {
    let outputNode = this.outputNode;
    let logLimit = Utils.logLimitForCategory(aCategory);

    let messageNodes = outputNode.getElementsByClassName("webconsole-msg-" +
        CATEGORY_CLASS_FRAGMENTS[aCategory]);
    let n = Math.max(0, messageNodes.length - logLimit);
    let toRemove = Array.prototype.slice.call(messageNodes, 0, n);
    toRemove.forEach(this.removeOutputMessage, this);

    return n;
  },

  





  removeOutputMessage: function WCF_removeOutputMessage(aNode)
  {
    if (aNode._objectActors) {
      for (let actor of aNode._objectActors) {
        this._releaseObject(actor);
      }
      aNode._objectActors.clear();
    }

    if (aNode.classList.contains("webconsole-msg-cssparser")) {
      let repeatNode = aNode.getElementsByClassName("webconsole-msg-repeat")[0];
      if (repeatNode && repeatNode._uid) {
        delete this._cssNodes[repeatNode._uid];
      }
    }
    else if (aNode._connectionId &&
             aNode.classList.contains("webconsole-msg-network")) {
      delete this._networkRequests[aNode._connectionId];
      this._releaseObject(aNode._connectionId);
    }
    else if (aNode.classList.contains("webconsole-msg-inspector")) {
      let view = aNode._variablesView;
      let actors = view ?
                   this.jsterm._objectActorsInVariablesViews.get(view) :
                   new Set();
      for (let actor of actors) {
        this._releaseObject(actor);
      }
      actors.clear();
      aNode._variablesView = null;
    }

    if (aNode.parentNode) {
      aNode.parentNode.removeChild(aNode);
    }
  },

  


  regroupOutput: function WCF_regroupOutput()
  {
    
    
    let nodes = this.outputNode.querySelectorAll(".hud-msg-node" +
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

  



























  createMessageNode:
  function WCF_createMessageNode(aCategory, aSeverity, aBody, aSourceURL,
                                 aSourceLine, aClipboardText, aLevel, aTimeStamp)
  {
    if (typeof aBody != "string" && aClipboardText == null && aBody.innerText) {
      aClipboardText = aBody.innerText;
    }

    
    
    
    let iconContainer = this.document.createElementNS(XUL_NS, "vbox");
    iconContainer.classList.add("webconsole-msg-icon-container");
    
    iconContainer.style.marginLeft = this.groupDepth * GROUP_INDENT + "px";

    
    
    let iconNode = this.document.createElementNS(XUL_NS, "image");
    iconNode.classList.add("webconsole-msg-icon");
    iconContainer.appendChild(iconNode);

    
    let spacer = this.document.createElementNS(XUL_NS, "spacer");
    spacer.flex = 1;
    iconContainer.appendChild(spacer);

    
    let bodyNode = this.document.createElementNS(XUL_NS, "description");
    bodyNode.flex = 1;
    bodyNode.classList.add("webconsole-msg-body");

    
    let body = aBody;
    
    
    aClipboardText = aClipboardText ||
                     (aBody + (aSourceURL ? " @ " + aSourceURL : "") +
                              (aSourceLine ? ":" + aSourceLine : ""));

    
    let node = this.document.createElementNS(XUL_NS, "richlistitem");

    if (aBody instanceof Ci.nsIDOMNode) {
      bodyNode.appendChild(aBody);
    }
    else {
      let str = undefined;
      if (aLevel == "dir") {
        str = VariablesView.getString(aBody.arguments[0]);
      }
      else if (["log", "info", "warn", "error", "debug"].indexOf(aLevel) > -1 &&
               typeof aBody == "object") {
        this._makeConsoleLogMessageBody(node, bodyNode, aBody);
      }
      else {
        str = aBody;
      }

      if (str !== undefined) {
        aBody = this.document.createTextNode(str);
        bodyNode.appendChild(aBody);
      }
    }

    let repeatContainer = this.document.createElementNS(XUL_NS, "hbox");
    repeatContainer.setAttribute("align", "start");
    let repeatNode = this.document.createElementNS(XUL_NS, "label");
    repeatNode.setAttribute("value", "1");
    repeatNode.classList.add("webconsole-msg-repeat");
    repeatNode._uid = [bodyNode.textContent, aCategory, aSeverity, aLevel,
                       aSourceURL, aSourceLine].join(":");
    repeatContainer.appendChild(repeatNode);

    
    let timestampNode = this.document.createElementNS(XUL_NS, "label");
    timestampNode.classList.add("webconsole-timestamp");
    let timestamp = aTimeStamp || Date.now();
    let timestampString = l10n.timestampString(timestamp);
    timestampNode.setAttribute("value", timestampString);

    
    
    let locationNode;
    if (aSourceURL && IGNORED_SOURCE_URLS.indexOf(aSourceURL) == -1) {
      locationNode = this.createLocationNode(aSourceURL, aSourceLine);
    }

    node.clipboardText = aClipboardText;
    node.classList.add("hud-msg-node");

    node.timestamp = timestamp;
    this.setMessageType(node, aCategory, aSeverity);

    node.appendChild(timestampNode);
    node.appendChild(iconContainer);

    
    if (aLevel == "dir") {
      let viewContainer = this.document.createElement("hbox");
      viewContainer.flex = 1;
      viewContainer.height = this.outputNode.clientHeight *
                             CONSOLE_DIR_VIEW_HEIGHT;

      let options = {
        objectActor: body.arguments[0],
        targetElement: viewContainer,
        hideFilterInput: true,
      };
      this.jsterm.openVariablesView(options)
        .then((aView) => node._variablesView = aView);

      let bodyContainer = this.document.createElement("vbox");
      bodyContainer.flex = 1;
      bodyContainer.appendChild(bodyNode);
      bodyContainer.appendChild(viewContainer);
      node.appendChild(bodyContainer);
      node.classList.add("webconsole-msg-inspector");
    }
    else {
      node.appendChild(bodyNode);
    }
    node.appendChild(repeatContainer);
    if (locationNode) {
      node.appendChild(locationNode);
    }

    node.setAttribute("id", "console-msg-" + gSequenceId());

    return node;
  },

  













  _makeConsoleLogMessageBody:
  function WCF__makeConsoleLogMessageBody(aMessage, aContainer, aBody)
  {
    Object.defineProperty(aMessage, "_panelOpen", {
      get: function() {
        let nodes = aContainer.querySelectorAll(".hud-clickable");
        return Array.prototype.some.call(nodes, function(aNode) {
          return aNode._panelOpen;
        });
      },
      enumerable: true,
      configurable: false
    });

    aBody.arguments.forEach(function(aItem) {
      if (aContainer.firstChild) {
        aContainer.appendChild(this.document.createTextNode(" "));
      }

      let text = VariablesView.getString(aItem);
      let inspectable = !VariablesView.isPrimitive({ value: aItem });

      if (aItem && typeof aItem != "object" || !inspectable) {
        aContainer.appendChild(this.document.createTextNode(text));

        if (aItem.type == "longString") {
          let ellipsis = this.document.createElement("description");
          ellipsis.classList.add("hud-clickable");
          ellipsis.classList.add("longStringEllipsis");
          ellipsis.textContent = l10n.getStr("longStringEllipsis");

          let formatter = function(s) '"' + s + '"';

          this._addMessageLinkCallback(ellipsis,
            this._longStringClick.bind(this, aMessage, aItem, formatter));

          aContainer.appendChild(ellipsis);
        }
        return;
      }

      
      let elem = this.document.createElement("description");
      elem.classList.add("hud-clickable");
      elem.setAttribute("aria-haspopup", "true");
      elem.appendChild(this.document.createTextNode(text));

      this._addMessageLinkCallback(elem,
        this._consoleLogClick.bind(this, elem, aItem));

      aContainer.appendChild(elem);
    }, this);
  },

  

















  _longStringClick:
  function WCF__longStringClick(aMessage, aActor, aFormatter, aEllipsis, aEvent)
  {
    aEvent.preventDefault();

    if (!aFormatter) {
      aFormatter = function(s) s;
    }

    let longString = this.webConsoleClient.longString(aActor);
    longString.substring(longString.initial.length, longString.length,
      function WCF__onSubstring(aResponse) {
        if (aResponse.error) {
          Cu.reportError("WCF__longStringClick substring failure: " +
                         aResponse.error);
          return;
        }

        let node = aEllipsis.previousSibling;
        node.textContent = aFormatter(longString.initial + aResponse.substring);
        aEllipsis.parentNode.removeChild(aEllipsis);

        if (aMessage.category == CATEGORY_WEBDEV ||
            aMessage.category == CATEGORY_OUTPUT) {
          aMessage.clipboardText = aMessage.textContent;
        }
      });
  },

  











  createLocationNode: function WCF_createLocationNode(aSourceURL, aSourceLine)
  {
    let locationNode = this.document.createElementNS(XUL_NS, "label");

    
    
    let text;

    if (/^Scratchpad\/\d+$/.test(aSourceURL)) {
      text = aSourceURL;
    }
    else {
      text = WebConsoleUtils.abbreviateSourceURL(aSourceURL);
    }

    if (aSourceLine) {
      text += ":" + aSourceLine;
      locationNode.sourceLine = aSourceLine;
    }

    locationNode.setAttribute("value", text);

    
    locationNode.setAttribute("crop", "center");
    locationNode.setAttribute("title", aSourceURL);
    locationNode.setAttribute("tooltiptext", aSourceURL);
    locationNode.classList.add("webconsole-location");
    locationNode.classList.add("text-link");

    
    locationNode.addEventListener("click", function() {
      if (/^Scratchpad\/\d+$/.test(aSourceURL)) {
        let wins = Services.wm.getEnumerator("devtools:scratchpad");

        while (wins.hasMoreElements()) {
          let win = wins.getNext();

          if (win.Scratchpad.uniqueName === aSourceURL) {
            win.focus();
            return;
          }
        }
      }
      else if (locationNode.parentNode.category == CATEGORY_CSS) {
        this.owner.viewSourceInStyleEditor(aSourceURL, aSourceLine);
      }
      else if (locationNode.parentNode.category == CATEGORY_JS ||
               locationNode.parentNode.category == CATEGORY_WEBDEV) {
        this.owner.viewSourceInDebugger(aSourceURL, aSourceLine);
      }
      else {
        this.owner.viewSource(aSourceURL, aSourceLine);
      }
    }.bind(this), true);

    return locationNode;
  },

  











  setMessageType:
  function WCF_setMessageType(aMessageNode, aNewCategory, aNewSeverity)
  {
    
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

  








  makeOutputMessageLink: function WCF_makeOutputMessageLink(aNode, aCallback)
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

    this._addMessageLinkCallback(aNode, aCallback);
  },

  








  _addMessageLinkCallback: function WCF__addMessageLinkCallback(aNode, aCallback)
  {
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

  







  copySelectedItems: function WCF_copySelectedItems(aOptions)
  {
    aOptions = aOptions || { linkOnly: false };

    
    let strings = [];
    let newGroup = false;

    let children = this.outputNode.children;

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

        if (aOptions.linkOnly) {
          strings.push(item.url);
        }
        else {
          strings.push("[" + timestampString + "] " + item.clipboardText);
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
    let item = this.outputNode.selectedItem;

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

    this._destroyer = Promise.defer();

    this._cssNodes = {};
    this._outputQueue = [];
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

    this._commandController = null;

    let onDestroy = function() {
      this._destroyer.resolve(null);
    }.bind(this);

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










function JSTerm(aWebConsoleFrame)
{
  this.hud = aWebConsoleFrame;
  this.hudId = this.hud.hudId;

  this.lastCompletion = { value: null };
  this.history = [];
  this.historyIndex = 0;
  this.historyPlaceHolder = 0;  
  this._objectActorsInVariablesViews = new Map();

  this._keyPress = this.keyPress.bind(this);
  this._inputEventHandler = this.inputEventHandler.bind(this);
  this._fetchVarProperties = this._fetchVarProperties.bind(this);
  this._fetchVarLongString = this._fetchVarLongString.bind(this);

  EventEmitter.decorate(this);
}

JSTerm.prototype = {
  SELECTED_FRAME: -1,

  



  lastCompletion: null,

  




  sidebar: null,

  




  _variablesView: null,

  






  _lazyVariablesView: true,

  







  _objectActorsInVariablesViews: null,

  



  lastInputValue: "",

  



  history: null,

  autocompletePopup: null,
  inputNode: null,
  completeNode: null,

  



  get outputNode() this.hud.outputNode,

  



  get webConsoleClient() this.hud.webConsoleClient,

  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,

  


  init: function JST_init()
  {
    let chromeDocument = this.hud.owner.chromeWindow.document;
    let autocompleteOptions = {
      onSelect: this.onAutocompleteSelect.bind(this),
      onClick: this.acceptProposedCompletion.bind(this),
      panelId: "webConsole_autocompletePopup",
      listBoxId: "webConsole_autocompletePopupListBox",
      position: "before_start",
      theme: "light",
      direction: "ltr",
      autoSelect: true
    };
    this.autocompletePopup = new AutocompletePopup(chromeDocument,
                                                   autocompleteOptions);

    let doc = this.hud.document;
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");
    this.inputNode.addEventListener("keypress", this._keyPress, false);
    this.inputNode.addEventListener("input", this._inputEventHandler, false);
    this.inputNode.addEventListener("keyup", this._inputEventHandler, false);

    this.lastInputValue && this.setInputValue(this.lastInputValue);
  },

  












  _executeResultCallback:
  function JST__executeResultCallback(aAfterNode, aCallback, aResponse)
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
    let inspectable = false;
    if (result && !VariablesView.isPrimitive({ value: result })) {
      inspectable = true;
    }
    let helperResult = aResponse.helperResult;
    let helperHasRawOutput = !!(helperResult || {}).rawOutput;
    let resultString = VariablesView.getString(result);

    if (helperResult && helperResult.type) {
      switch (helperResult.type) {
        case "clearOutput":
          this.clearOutput();
          break;
        case "inspectObject":
          if (aAfterNode) {
            if (!aAfterNode._objectActors) {
              aAfterNode._objectActors = new Set();
            }
            aAfterNode._objectActors.add(helperResult.object.actor);
          }
          this.openVariablesView({
            label: VariablesView.getString(helperResult.object),
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
      }
    }

    
    if (!errorMessage && result && typeof result == "object" &&
        result.type == "undefined" &&
        helperResult && !helperHasRawOutput) {
      aCallback && aCallback();
      return;
    }

    if (aCallback) {
      let oldFlushCallback = this.hud._flushCallback;
      this.hud._flushCallback = function() {
        aCallback();
        oldFlushCallback && oldFlushCallback();
        this.hud._flushCallback = oldFlushCallback;
      }.bind(this);
    }

    let node;

    if (errorMessage) {
      node = this.writeOutput(errorMessage, CATEGORY_OUTPUT, SEVERITY_ERROR,
                              aAfterNode, aResponse.timestamp);
    }
    else if (inspectable) {
      node = this.writeOutputJS(resultString,
                                this._evalOutputClick.bind(this, aResponse),
                                aAfterNode, aResponse.timestamp);
    }
    else {
      node = this.writeOutput(resultString, CATEGORY_OUTPUT, SEVERITY_LOG,
                              aAfterNode, aResponse.timestamp);
    }

    node._objectActors = new Set();

    let error = aResponse.exception;
    if (WebConsoleUtils.isActorGrip(error)) {
      node._objectActors.add(error.actor);
    }

    if (WebConsoleUtils.isActorGrip(result)) {
      node._objectActors.add(result.actor);

      if (result.type == "longString") {
        
        

        let body = node.querySelector(".webconsole-msg-body");
        let ellipsis = this.hud.document.createElement("description");
        ellipsis.classList.add("hud-clickable");
        ellipsis.classList.add("longStringEllipsis");
        ellipsis.textContent = l10n.getStr("longStringEllipsis");

        let formatter = function(s) '"' + s + '"';
        let onclick = this.hud._longStringClick.bind(this.hud, node, result,
                                                    formatter);
        this.hud._addMessageLinkCallback(ellipsis, onclick);

        body.appendChild(ellipsis);

        node.clipboardText += " " + ellipsis.textContent;
      }
    }
  },

  








  execute: function JST_execute(aExecuteString, aCallback)
  {
    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      this.writeOutput(l10n.getStr("executeEmptyInput"), CATEGORY_OUTPUT,
                       SEVERITY_LOG);
      return;
    }

    let node = this.writeOutput(aExecuteString, CATEGORY_INPUT, SEVERITY_LOG);
    let onResult = this._executeResultCallback.bind(this, node, aCallback);

    let options = { frame: this.SELECTED_FRAME };
    this.requestEvaluation(aExecuteString, options).then(onResult, onResult);

    this.history.push(aExecuteString);
    this.historyIndex++;
    this.historyPlaceHolder = this.history.length;
    this.setInputValue("");
    this.clearCompletion();
  },

  




















  requestEvaluation: function JST_requestEvaluation(aString, aOptions = {})
  {
    let deferred = Promise.defer();

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
    };

    this.webConsoleClient.evaluateJS(aString, onResult, evalOptions);
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
        }
      }
      aOptions.view = view;
      this._updateVariablesView(aOptions);
      this.emit("variablesview-open", view, aOptions);
      return view;
    };

    let promise;
    if (aOptions.targetElement) {
      let deferred = Promise.defer();
      promise = deferred.promise;
      let document = aOptions.targetElement.ownerDocument;
      let iframe = document.createElement("iframe");

      iframe.addEventListener("load", function onIframeLoad(aEvent) {
        iframe.removeEventListener("load", onIframeLoad, true);
        deferred.resolve(iframe.contentWindow);
      }, true);

      iframe.flex = 1;
      iframe.setAttribute("src", VARIABLES_VIEW_URL);
      aOptions.targetElement.appendChild(iframe);
    }
    else {
      this._createSidebar();
      promise = this._addVariablesViewSidebarTab();
    }

    return promise.then(onContainerReady);
  },

  





  _createSidebar: function JST__createSidebar()
  {
    if (!this.sidebar) {
      let tabbox = this.hud.document.querySelector("#webconsole-sidebar");
      this.sidebar = new ToolSidebar(tabbox, this);
    }
    this.sidebar.show();
  },

  






  _addVariablesViewSidebarTab: function JST__addVariablesViewSidebarTab()
  {
    let deferred = Promise.defer();

    let onTabReady = () => {
      let window = this.sidebar.getWindowForTab("variablesview");
      deferred.resolve(window);
    };

    let tab = this.sidebar.getTab("variablesview");
    if (tab) {
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

  











  _createVariablesView: function JST__createVariablesView(aOptions)
  {
    let view = new VariablesView(aOptions.container);
    view.searchPlaceholder = l10n.getStr("propertiesFilterPlaceholder");
    view.emptyText = l10n.getStr("emptyPropertiesList");
    view.searchEnabled = !aOptions.hideFilterInput;
    view.lazyEmpty = this._lazyVariablesView;
    view.lazyAppend = this._lazyVariablesView;
    this._objectActorsInVariablesViews.set(view, new Set());
    return view;
  },

  











  _updateVariablesView: function JST__updateVariablesView(aOptions)
  {
    let view = aOptions.view;
    view.createHierarchy();
    view.empty();

    let actors = this._objectActorsInVariablesViews.get(view);
    for (let actor of actors) {
      
      
      if (view._consoleLastObjectActor != actor) {
        this.hud._releaseObject(actor);
      }
    }

    actors.clear();

    if (aOptions.objectActor) {
      
      view.eval = this._variablesViewEvaluate.bind(this, aOptions);
      view.switch = this._variablesViewSwitch.bind(this, aOptions);
      view.delete = this._variablesViewDelete.bind(this, aOptions);
    }
    else {
      view.eval = null;
      view.switch = null;
      view.delete = null;
    }

    let scope = view.addScope(aOptions.label);
    scope.expanded = true;
    scope.locked = true;

    let container = scope.addVar();
    container.evaluationMacro = this._variablesViewSimpleValueEvalMacro;

    if (aOptions.objectActor) {
      this._fetchVarProperties(container, aOptions.objectActor);
      view._consoleLastObjectActor = aOptions.objectActor.actor;
    }
    else if (aOptions.rawObject) {
      container.populate(aOptions.rawObject);
      view.commitHierarchy();
      view._consoleLastObjectActor = null;
    }
    else {
      throw new Error("Variables View cannot open without giving it an object " +
                      "display.");
    }

    this.emit("variablesview-updated", view, aOptions);
  },

  









  _variablesViewEvaluate: function JST__variablesViewEvaluate(aOptions, aString)
  {
    let updater = this._updateVariablesView.bind(this, aOptions);
    let onEval = this._silentEvalCallback.bind(this, updater);

    let evalOptions = {
      frame: this.SELECTED_FRAME,
      bindObjectActor: aOptions.objectActor.actor,
    };

    this.requestEvaluation(aString, evalOptions).then(onEval, onEval);
  },

  











  _variablesViewSimpleValueEvalMacro:
  function JST__variablesViewSimpleValueEvalMacro(aItem, aCurrentString)
  {
    return "_self" + aItem.symbolicName + "=" + aCurrentString;
  },


  











  _variablesViewOverrideValueEvalMacro:
  function JST__variablesViewOverrideValueEvalMacro(aItem, aCurrentString)
  {
    let parent = aItem.ownerView;
    let symbolicName = parent.symbolicName;
    if (symbolicName.indexOf("_self") != 0) {
      parent._symbolicName = "_self" + symbolicName;
    }

    let result = VariablesView.overrideValueEvalMacro.apply(this, arguments);

    parent._symbolicName = symbolicName;

    return result;
  },

  











  _variablesViewGetterOrSetterEvalMacro:
  function JST__variablesViewGetterOrSetterEvalMacro(aItem, aCurrentString)
  {
    let propertyObject = aItem.ownerView;
    let parentObject = propertyObject.ownerView;
    let parent = parentObject.symbolicName;
    parentObject._symbolicName = "_self" + parent;

    let result = VariablesView.getterOrSetterEvalMacro.apply(this, arguments);

    parentObject._symbolicName = parent;

    return result;
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

    let exception = aResponse.exception;
    if (exception) {
      let node = this.writeOutput(aResponse.exceptionMessage,
                                  CATEGORY_OUTPUT, SEVERITY_ERROR,
                                  null, aResponse.timestamp);
      node._objectActors = new Set();
      if (WebConsoleUtils.isActorGrip(exception)) {
        node._objectActors.add(exception.actor);
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

  









  _fetchVarProperties: function JST__fetchVarProperties(aVar, aGrip)
  {
    
    if (aVar._fetched) {
      return;
    }
    aVar._fetched = true;

    let grip = aGrip || aVar.value;
    if (!grip) {
      throw new Error("No object actor grip was given for the variable.");
    }

    let view = aVar._variablesView;
    let actors = this._objectActorsInVariablesViews.get(view);

    function addActorForDescriptor(aGrip) {
      if (WebConsoleUtils.isActorGrip(aGrip)) {
        actors.add(aGrip.actor);
      }
    }

    let onNewProperty = (aProperty) => {
      if (aProperty.getter || aProperty.setter) {
        aProperty.evaluationMacro = this._variablesViewOverrideValueEvalMacro;
        let getter = aProperty.get("get");
        let setter = aProperty.get("set");
        if (getter) {
          getter.evaluationMacro = this._variablesViewGetterOrSetterEvalMacro;
        }
        if (setter) {
          setter.evaluationMacro = this._variablesViewGetterOrSetterEvalMacro;
        }
      }
      else {
        aProperty.evaluationMacro = this._variablesViewSimpleValueEvalMacro;
      }

      let grips = [aProperty.value, aProperty.gettter, aProperty.settter];
      grips.forEach(addActorForDescriptor);

      let inspectable = !VariablesView.isPrimitive({ value: aProperty.value });
      let longString = WebConsoleUtils.isActorGrip(aProperty.value) &&
                       aProperty.value.type == "longString";
      if (inspectable) {
        aProperty.onexpand = this._fetchVarProperties;
      }
      else if (longString) {
        aProperty.onexpand = this._fetchVarLongString;
        aProperty.showArrow();
      }
    };

    let client = new GripClient(this.hud.proxy.client, grip);
    client.getPrototypeAndProperties((aResponse) => {
      let { ownProperties, prototype } = aResponse;
      let sortable = VariablesView.NON_SORTABLE_CLASSES.indexOf(grip.class) == -1;

      
      if (ownProperties) {
        aVar.addProperties(ownProperties, {
          sorted: sortable,
          callback: onNewProperty,
        });
      }

      
      if (prototype && prototype.type != "null") {
        let proto = aVar.addProperty("__proto__", { value: prototype });
        onNewProperty(proto);
      }

      aVar._retrieved = true;
      view.commitHierarchy();
      this.emit("variablesview-fetched", aVar);
    });
  },

  





  _fetchVarLongString: function JST__fetchVarLongString(aVar)
  {
    if (aVar._fetched) {
      return;
    }
    aVar._fetched = true;

    let grip = aVar.value;
    if (!grip) {
      throw new Error("No long string actor grip was given for the variable.");
    }

    let client = this.webConsoleClient.longString(grip);
    client.substring(grip.initial.length, grip.length, (aResponse) => {
      if (aResponse.error) {
        Cu.reportError("JST__fetchVarLongString substring failure: " +
                       aResponse.error + ": " + aResponse.message);
        return;
      }

      aVar.onexpand = null;
      aVar.setGrip(grip.initial + aResponse.substring);
      aVar.hideArrow();
      aVar._retrieved = true;
    });
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
    let node = this.hud.createMessageNode(aCategory, aSeverity, aOutputMessage,
                                          null, null, null, null, aTimestamp);
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

    hud.groupDepth = 0;
    hud._outputQueue.forEach(hud._pruneItemFromQueue, hud);
    hud._outputQueue = [];
    hud._networkRequests = {};
    hud._cssNodes = {};

    if (aClearStorage) {
      this.webConsoleClient.clearMessagesCache();
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
      let inputNode = this.inputNode;
      let closePopup = false;
      switch (aEvent.charCode) {
        case 97:
          
          if (Services.appinfo.OS == "WINNT") {
            closePopup = true;
            break;
          }
          let lineBeginPos = 0;
          if (this.hasMultilineInput()) {
            
            for (let i = inputNode.selectionStart-1; i >= 0; i--) {
              if (inputNode.value.charAt(i) == "\r" ||
                  inputNode.value.charAt(i) == "\n") {
                lineBeginPos = i+1;
                break;
              }
            }
          }
          inputNode.setSelectionRange(lineBeginPos, lineBeginPos);
          aEvent.preventDefault();
          closePopup = true;
          break;
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
          break;
        case 110:
          
          
          
          if (Services.appinfo.OS == "Darwin" &&
              this.canCaretGoNext() &&
              this.historyPeruse(HISTORY_FORWARD)) {
            aEvent.preventDefault();
          }
          closePopup = true;
          break;
        case 112:
          
          
          
          if (Services.appinfo.OS == "Darwin" &&
              this.canCaretGoPrevious() &&
              this.historyPeruse(HISTORY_BACK)) {
            aEvent.preventDefault();
          }
          closePopup = true;
          break;
        default:
          break;
      }
      if (closePopup) {
        if (this.autocompletePopup.isOpen) {
          this.clearCompletion();
        }
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

    let requestId = gSequenceId();
    let input = this.inputNode.value;
    let cursor = this.inputNode.selectionStart;

    
    
    this.lastCompletion = {
      requestId: requestId,
      completionType: aType,
      value: null,
    };

    let callback = this._receiveAutocompleteProperties.bind(this, requestId,
                                                            aCallback);
    this.webConsoleClient.autocomplete(input, cursor, callback);
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

    let matches = aMessage.matches;
    let lastPart = aMessage.matchProp;
    if (!matches.length) {
      this.clearCompletion();
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

  






  _evalOutputClick: function JST__evalOutputClick(aResponse)
  {
    this.openVariablesView({
      label: VariablesView.getString(aResponse.result),
      objectActor: aResponse.result,
    });
  },

  


  destroy: function JST_destroy()
  {
    if (this._variablesView) {
      let actors = this._objectActorsInVariablesViews.get(this._variablesView);
      for (let actor of actors) {
        this.hud._releaseObject(actor);
      }
      actors.clear();
      this._variablesView = null;
    }

    if (this.sidebar) {
      this.sidebar.destroy();
      this.sidebar = null;
    }

    this.clearCompletion();
    this.clearOutput();

    this.autocompletePopup.destroy();
    this.autocompletePopup = null;

    let popup = this.hud.owner.chromeWindow.document
                .getElementById("webConsole_autocompletePopup");
    if (popup) {
      popup.parentNode.removeChild(popup);
    }

    this.inputNode.removeEventListener("keypress", this._keyPress, false);
    this.inputNode.removeEventListener("input", this._inputEventHandler, false);
    this.inputNode.removeEventListener("keyup", this._inputEventHandler, false);

    this.hud = null;
  },
};




var Utils = {
  


  scroll: true,

  







  scrollToVisible: function Utils_scrollToVisible(aNode)
  {
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

  







  isOutputScrolledToBottom: function Utils_isOutputScrolledToBottom(aOutputNode)
  {
    let lastNodeHeight = aOutputNode.lastChild ?
                         aOutputNode.lastChild.clientHeight : 0;
    let scrollBox = aOutputNode.scrollBoxObject.element;

    return scrollBox.scrollTop + scrollBox.clientHeight >=
           scrollBox.scrollHeight - lastNodeHeight / 2;
  },

  








  categoryForScriptError: function Utils_categoryForScriptError(aScriptError)
  {
    switch (aScriptError.category) {
      case "CSS Parser":
      case "CSS Loader":
        return CATEGORY_CSS;

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
  



  copy: function CommandController_copy()
  {
    this.owner.copySelectedItems();
  },

  


  selectAll: function CommandController_selectAll()
  {
    this.owner.outputNode.selectAll();
  },

  


  openURL: function CommandController_openURL()
  {
    this.owner.openSelectedItemInTab();
  },

  copyURL: function CommandController_copyURL()
  {
    this.owner.copySelectedItems({ linkOnly: true });
  },

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    return this.isCommandEnabled(aCommand);
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        
        return this.owner.outputNode.selectedCount > 0;
      case "consoleCmd_openURL":
      case "consoleCmd_copyURL": {
        
        let selectedItem = this.owner.outputNode.selectedItem;
        return selectedItem && selectedItem.url;
      }
      case "cmd_fontSizeEnlarge":
      case "cmd_fontSizeReduce":
      case "cmd_fontSizeReset":
      case "cmd_selectAll":
        return true;
      case "cmd_close":
        return this.owner.owner._browserConsole;
    }
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        this.copy();
        break;
      case "consoleCmd_openURL":
        this.openURL();
        break;
      case "consoleCmd_copyURL":
        this.copyURL();
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
  this._onConsoleAPICall = this._onConsoleAPICall.bind(this);
  this._onNetworkEvent = this._onNetworkEvent.bind(this);
  this._onNetworkEventUpdate = this._onNetworkEventUpdate.bind(this);
  this._onFileActivity = this._onFileActivity.bind(this);
  this._onTabNavigated = this._onTabNavigated.bind(this);
  this._onAttachConsole = this._onAttachConsole.bind(this);
  this._onCachedMessages = this._onCachedMessages.bind(this);
  this._connectionTimeout = this._connectionTimeout.bind(this);
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

    this._connectDefer = Promise.defer();

    let timeout = Services.prefs.getIntPref(PREF_CONNECTION_TIMEOUT);
    this._connectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._connectTimer.initWithCallback(this._connectionTimeout,
                                        timeout, Ci.nsITimer.TYPE_ONE_SHOT);

    let promise = this._connectDefer.promise;
    promise.then(function _onSucess() {
      this._connectTimer.cancel();
      this._connectTimer = null;
    }.bind(this), function _onFailure() {
      this._connectTimer = null;
    }.bind(this));

    let client = this.client = this.target.client;

    client.addListener("pageError", this._onPageError);
    client.addListener("consoleAPICall", this._onConsoleAPICall);
    client.addListener("networkEvent", this._onNetworkEvent);
    client.addListener("networkEventUpdate", this._onNetworkEventUpdate);
    client.addListener("fileActivity", this._onFileActivity);
    this.target.on("will-navigate", this._onTabNavigated);
    this.target.on("navigate", this._onTabNavigated);

    this._consoleActor = this.target.form.consoleActor;
    if (!this.target.chrome) {
      let tab = this.target.form;
      this.owner.onLocationChange(tab.url, tab.title);
    }
    this._attachConsole();

    return promise;
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

  









  _onTabNavigated: function WCCP__onTabNavigated(aType, aPacket)
  {
    if (!this.owner) {
      return;
    }

    if (aPacket.url) {
      this.owner.onLocationChange(aPacket.url, aPacket.title);
    }

    if (aType == "navigate" && !aPacket.nativeConsoleAPI) {
      this.owner.logWarningAboutReplacedAPI();
    }
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

    this._disconnecter = Promise.defer();

    if (!this.client) {
      this._disconnecter.resolve(null);
      return this._disconnecter.promise;
    }

    this.client.removeListener("pageError", this._onPageError);
    this.client.removeListener("consoleAPICall", this._onConsoleAPICall);
    this.client.removeListener("networkEvent", this._onNetworkEvent);
    this.client.removeListener("networkEventUpdate", this._onNetworkEventUpdate);
    this.client.removeListener("fileActivity", this._onFileActivity);
    this.client.removeListener("tabNavigated", this._onTabNavigated);

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


function goUpdateConsoleCommands() {
  goUpdateCommand("consoleCmd_openURL");
  goUpdateCommand("consoleCmd_copyURL");
}







const CONTEXTMENU_ID = "output-contextmenu";




let ConsoleContextMenu = {
  




  build: function CCM_build(aEvent)
  {
    let popup = aEvent.target;
    if (popup.id !== CONTEXTMENU_ID) {
      return;
    }

    let view = document.querySelector(".hud-output-node");
    let metadata = this.getSelectionMetadata(view);

    for (let i = 0, l = popup.childNodes.length; i < l; ++i) {
      let element = popup.childNodes[i];
      element.hidden = this.shouldHideMenuItem(element, metadata);
    }
  },

  








  getSelectionMetadata: function CCM_getSelectionMetadata(aView)
  {
    let metadata = {
      selectionType: "",
      selection: new Set(),
    };
    let selectedItems = aView.selectedItems;

    metadata.selectionType = (selectedItems > 1) ? "multiple" : "single";

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
};
