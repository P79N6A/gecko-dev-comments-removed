





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

XPCOMUtils.defineLazyModuleGetter(this, "PropertyPanel",
                                  "resource:///modules/PropertyPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PropertyTreeView",
                                  "resource:///modules/PropertyPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkPanel",
                                  "resource:///modules/NetworkPanel.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AutocompletePopup",
                                  "resource:///modules/AutocompletePopup.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource:///modules/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "l10n", function() {
  return WebConsoleUtils.l10n;
});



const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const MIXED_CONTENT_LEARN_MORE = "https://developer.mozilla.org/en/Security/MixedContent";



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














function WebConsoleFrame(aWebConsoleOwner, aPosition)
{
  this.owner = aWebConsoleOwner;
  this.hudId = this.owner.hudId;

  this._cssNodes = {};
  this._outputQueue = [];
  this._pruneCategoriesQueue = {};
  this._networkRequests = {};

  this._toggleFilter = this._toggleFilter.bind(this);
  this._onPositionConsoleCommand = this._onPositionConsoleCommand.bind(this);

  this._initDefaultFilterPrefs();
  this._commandController = new CommandController(this);
  this.positionConsole(aPosition, window);

  this.jsterm = new JSTerm(this);
  this.jsterm.inputNode.focus();
}

WebConsoleFrame.prototype = {
  




  owner: null,

  



  get popupset() this.owner.mainPopupSet,

  





  _networkRequests: null,

  






  _lastOutputFlush: 0,

  





  _outputQueue: null,

  





  _pruneCategoriesQueue: null,

  






  _flushCallback: null,

  




  _cssNodes: null,

  




  filterPrefs: null,

  


  groupDepth: 0,

  




  jsterm: null,

  



  outputNode: null,

  



  filterBox: null,

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

    this.owner.sendMessageToContent("WebConsole:SetPreferences", message);
  },

  



  _initUI: function WCF__initUI()
  {
    let doc = this.document;

    this.filterBox = doc.querySelector(".hud-filter-box");
    this.outputNode = doc.querySelector(".hud-output-node");
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");

    this._setFilterTextBoxEvents();
    this._initPositionUI();
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

    let contextMenuId = this.outputNode.getAttribute("context");
    let contextMenu = doc.getElementById(contextMenuId);
    contextMenu.addEventListener("popupshowing", function() {
      saveBodies.setAttribute("checked", this.saveRequestAndResponseBodies);
    }.bind(this));

    this.closeButton = doc.getElementById("webconsole-close-button");
    this.closeButton.addEventListener("command",
                                      this.owner.onCloseButton.bind(this.owner));

    let clearButton = doc.getElementsByClassName("webconsole-clear-console-button")[0];
    clearButton.addEventListener("command",
                                 this.owner.onClearButton.bind(this.owner));
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

  



  _initPositionUI: function WCF__initPositionUI()
  {
    let doc = this.document;

    let itemAbove = doc.querySelector("menuitem[consolePosition='above']");
    itemAbove.addEventListener("command", this._onPositionConsoleCommand, false);

    let itemBelow = doc.querySelector("menuitem[consolePosition='below']");
    itemBelow.addEventListener("command", this._onPositionConsoleCommand, false);

    let itemWindow = doc.querySelector("menuitem[consolePosition='window']");
    itemWindow.addEventListener("command", this._onPositionConsoleCommand, false);

    this.positionMenuitems = {
      last: null,
      above: itemAbove,
      below: itemBelow,
      window: itemWindow,
    };
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

  






  _onPositionConsoleCommand: function WCF__onPositionConsoleCommand(aEvent)
  {
    let position = aEvent.target.getAttribute("consolePosition");
    this.owner.positionConsole(position);
  },

  














  positionConsole: function WCF_positionConsole(aPosition, aNewWindow)
  {
    this.window = aNewWindow;
    this.document = this.window.document;
    this.rootElement = this.document.documentElement;

    
    this.window.controllers.insertControllerAt(0, this._commandController);

    let oldOutputNode = this.outputNode;

    this._initUI();
    this.jsterm && this.jsterm._initUI();

    this.closeButton.hidden = aPosition == "window";

    this.positionMenuitems[aPosition].setAttribute("checked", true);
    if (this.positionMenuitems.last) {
      this.positionMenuitems.last.setAttribute("checked", false);
    }
    this.positionMenuitems.last = this.positionMenuitems[aPosition];

    if (oldOutputNode && oldOutputNode.childNodes.length) {
      let parentNode = this.outputNode.parentNode;
      parentNode.replaceChild(oldOutputNode, this.outputNode);
      this.outputNode = oldOutputNode;
    }

    this.jsterm && this.jsterm.inputNode.focus();
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

  






  receiveMessage: function WCF_receiveMessage(aMessage)
  {
    if (!aMessage.json || aMessage.json.hudId != this.hudId) {
      return;
    }

    switch (aMessage.name) {
      case "JSTerm:EvalResult":
      case "JSTerm:EvalObject":
      case "JSTerm:AutocompleteProperties":
        this.owner._receiveMessageWithCallback(aMessage.json);
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
        let category = Utils.categoryForScriptError(pageError);
        this.outputMessage(category, this.reportPageError,
                           [category, pageError]);
        break;
      }
      case "WebConsole:CachedMessages":
        this._displayCachedConsoleMessages(aMessage.json.messages);
        this.owner._onInitComplete();
        break;
      case "WebConsole:NetworkActivity":
        this.handleNetworkActivity(aMessage.json);
        break;
      case "WebConsole:FileActivity":
        this.outputMessage(CATEGORY_NETWORK, this.logFileActivity,
                           [aMessage.json.uri]);
        break;
      case "WebConsole:LocationChange":
        this.owner.onLocationChange(aMessage.json);
        break;
      case "JSTerm:NonNativeConsoleAPI":
        this.outputMessage(CATEGORY_JS, this.logWarningAboutReplacedAPI);
        break;
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

        
        let menuPopup = target.parentNode;

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

  








  filterRepeatedCSS: function WCF_filterRepeatedCSS(aNode)
  {
    
    let description = aNode.childNodes[2].textContent;
    let location;

    
    
    if (aNode.childNodes[4]) {
      
      location = aNode.childNodes[4].getAttribute("title");
    }
    else {
      location = "";
    }

    let dupe = this._cssNodes[description + location];
    if (!dupe) {
      
      this._cssNodes[description + location] = aNode;
      return false;
    }

    this.mergeFilteredMessageNode(dupe, aNode);

    return true;
  },

  










  filterRepeatedConsole: function WCF_filterRepeatedConsole(aNode)
  {
    let lastMessage = this.outputNode.lastChild;

    
    if (lastMessage && lastMessage.childNodes[2] &&
        !aNode.classList.contains("webconsole-msg-inspector") &&
        aNode.childNodes[2].textContent ==
        lastMessage.childNodes[2].textContent) {
      this.mergeFilteredMessageNode(lastMessage, aNode);
      return true;
    }

    return false;
  },

  








  _displayCachedConsoleMessages:
  function WCF__displayCachedConsoleMessages(aRemoteMessages)
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
        body = {
          cacheId: aMessage.objectsCacheId,
          remoteObjects: args,
          argsToString: argsToString,
        };
        clipboardText = argsToString.join(" ");
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

    let node = this.createMessageNode(CATEGORY_WEBDEV, LEVELS[level], body,
                                      sourceURL, sourceLine, clipboardText,
                                      level, aMessage.timeStamp);

    
    
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

  












  _consoleLogClick:
  function WCF__consoleLogClick(aMessage, aAnchor, aRemoteObject)
  {
    if (aAnchor._panelOpen) {
      return;
    }

    let options = {
      title: aAnchor.textContent,
      anchor: aAnchor,

      
      data: {
        
        rootCacheId: aMessage._evalCacheId,
        remoteObject: aRemoteObject,
        
        panelCacheId: "HUDPanel-" + gSequenceId(),
        remoteObjectProvider: this.jsterm.remoteObjectProvider.bind(this.jsterm),
      },
    };

    let propPanel = this.jsterm.openPropertyPanel(options);
    propPanel.panel.setAttribute("hudId", this.hudId);

    let onPopupHide = function JST__evalInspectPopupHide() {
      propPanel.panel.removeEventListener("popuphiding", onPopupHide, false);

      this.jsterm.clearObjectCache(options.data.panelCacheId);

      if (!aMessage.parentNode && aMessage._evalCacheId) {
        this.jsterm.clearObjectCache(aMessage._evalCacheId);
      }
    }.bind(this);

    propPanel.panel.addEventListener("popuphiding", onPopupHide, false);
  },

  







  reportPageError: function WCF_reportPageError(aCategory, aScriptError)
  {
    
    
    let severity = SEVERITY_ERROR;
    if ((aScriptError.flags & aScriptError.warningFlag) ||
        (aScriptError.flags & aScriptError.strictFlag)) {
      severity = SEVERITY_WARNING;
    }

    let node = this.createMessageNode(aCategory, severity,
                                      aScriptError.errorMessage,
                                      aScriptError.sourceName,
                                      aScriptError.lineNumber, null, null,
                                      aScriptError.timeStamp);
    return node;
  },

  







  logNetActivity: function WCF_logNetActivity(aConnectionId)
  {
    let networkInfo = this._networkRequests[aConnectionId];
    if (!networkInfo) {
      return;
    }

    let entry = networkInfo.httpActivity.log.entries[0];
    let request = entry.request;

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
      WebConsoleUtils.isMixedHTTPSRequest(request.url,
                                          this.owner.contentLocation);
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

    messageNode._connectionId = entry.connection;

    this.makeOutputMessageLink(messageNode, function WCF_net_message_link() {
      if (!messageNode._panelOpen) {
        this.openNetworkPanel(messageNode, networkInfo.httpActivity);
      }
    }.bind(this));

    networkInfo.node = messageNode;

    this._updateNetMessage(entry.connection);

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
      let viewSourceUtils = this.owner.gViewSourceUtils;
      viewSourceUtils.viewSource(aFileURI, null, this.document);
    }.bind(this));

    return outputNode;
  },

  






  logWarningAboutReplacedAPI: function WCF_logWarningAboutReplacedAPI()
  {
    return this.createMessageNode(CATEGORY_JS, SEVERITY_WARNING,
                                  l10n.getStr("ConsoleAPIDisabled"));
  },

  













  handleNetworkActivity: function WCF_handleNetworkActivity(aMessage)
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

    
    
    if (this.owner.lastFinishedRequestCallback &&
        aMessage.meta.stages.indexOf("REQUEST_STOP") > -1 &&
        aMessage.meta.stages.indexOf("TRANSACTION_CLOSE") > -1) {
      this.owner.lastFinishedRequestCallback(aMessage);
    }
  },

  







  _updateNetMessage: function WCF__updateNetMessage(aConnectionId)
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
        this.setMessageType(messageNode, CATEGORY_NETWORK, SEVERITY_ERROR);
      }
    }

    if (messageNode._netPanel) {
      messageNode._netPanel.update();
    }
  },

  










  openNetworkPanel: function WCF_openNetworkPanel(aNode, aHttpActivity)
  {
    let netPanel = new NetworkPanel(this.popupset, aHttpActivity);
    netPanel.linkNode = aNode;
    aNode._netPanel = netPanel;

    let panel = netPanel.panel;
    panel.openPopup(aNode, "after_pointer", 0, 0, false, false);
    panel.sizeTo(450, 500);
    panel.setAttribute("hudId", aHttpActivity.hudId);

    panel.addEventListener("popuphiding", function WCF_netPanel_onHide() {
      panel.removeEventListener("popuphiding", WCF_netPanel_onHide);

      aNode._panelOpen = false;
      aNode._netPanel = null;
    });

    aNode._panelOpen = true;

    return netPanel;
  },

  

















  outputMessage: function WCF_outputMessage(aCategory, aMethodOrNode, aArguments)
  {
    if (!this._outputQueue.length) {
      
      
      this._lastOutputFlush = Date.now();
    }

    this._outputQueue.push([aCategory, aMethodOrNode, aArguments]);

    if (!this._outputTimeout) {
      this._outputTimeout =
        this.window.setTimeout(this._flushMessageQueue.bind(this),
                               OUTPUT_INTERVAL);
    }
  },

  






  _flushMessageQueue: function WCF__flushMessageQueue()
  {
    let timeSinceFlush = Date.now() - this._lastOutputFlush;
    if (this._outputQueue.length > MESSAGES_IN_INTERVAL &&
        timeSinceFlush < THROTTLE_UPDATES) {
      this._outputTimeout =
        this.window.setTimeout(this._flushMessageQueue.bind(this),
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
      this._outputTimeout =
        this.window.setTimeout(this._flushMessageQueue.bind(this),
                               OUTPUT_INTERVAL);
    }
    else {
      this._outputTimeout = null;
      this._flushCallback && this._flushCallback();
    }

    this._lastOutputFlush = Date.now();
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

    let isRepeated = false;
    if (node.classList.contains("webconsole-msg-cssparser")) {
      isRepeated = this.filterRepeatedCSS(node);
    }

    if (!isRepeated &&
        !node.classList.contains("webconsole-msg-network") &&
        (node.classList.contains("webconsole-msg-console") ||
         node.classList.contains("webconsole-msg-exception") ||
         node.classList.contains("webconsole-msg-error"))) {
      isRepeated = this.filterRepeatedConsole(node);
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

  







  pruneConsoleDirNode: function WCF_pruneConsoleDirNode(aMessageNode)
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

  





  removeOutputMessage: function WCF_removeOutputMessage(aNode)
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
      delete this._cssNodes[desc + location];
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
        str = aBody.resultString;
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
    repeatContainer.appendChild(repeatNode);

    
    let timestampNode = this.document.createElementNS(XUL_NS, "label");
    timestampNode.classList.add("webconsole-timestamp");
    let timestamp = aTimeStamp || Date.now();
    let timestampString = l10n.timestampString(timestamp);
    timestampNode.setAttribute("value", timestampString);

    
    
    let locationNode;
    if (aSourceURL) {
      locationNode = this.createLocationNode(aSourceURL, aSourceLine);
    }

    node.clipboardText = aClipboardText;
    node.classList.add("hud-msg-node");

    node.timestamp = timestamp;
    this.setMessageType(node, aCategory, aSeverity);

    node.appendChild(timestampNode);
    node.appendChild(iconContainer);
    
    if (aLevel == "dir") {
      
      
      let bodyContainer = this.document.createElement("vbox");
      bodyContainer.flex = 1;
      bodyContainer.appendChild(bodyNode);
      
      let tree = this.document.createElement("tree");
      tree.setAttribute("hidecolumnpicker", "true");
      tree.flex = 1;

      let treecols = this.document.createElement("treecols");
      let treecol = this.document.createElement("treecol");
      treecol.setAttribute("primary", "true");
      treecol.setAttribute("hideheader", "true");
      treecol.setAttribute("ignoreincolumnpicker", "true");
      treecol.flex = 1;
      treecols.appendChild(treecol);
      tree.appendChild(treecols);

      tree.appendChild(this.document.createElement("treechildren"));

      bodyContainer.appendChild(tree);
      node.appendChild(bodyContainer);
      node.classList.add("webconsole-msg-inspector");
      
      let treeView = node.propertyTreeView = new PropertyTreeView();

      treeView.data = {
        rootCacheId: body.cacheId,
        panelCacheId: body.cacheId,
        remoteObject: Array.isArray(body.remoteObject) ? body.remoteObject : [],
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

    node.setAttribute("id", "console-msg-" + gSequenceId());

    return node;
  },

  












  _makeConsoleLogMessageBody:
  function WCF__makeConsoleLogMessageBody(aMessage, aContainer, aBody)
  {
    aMessage._evalCacheId = aBody.cacheId;

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

    aBody.remoteObjects.forEach(function(aItem, aIndex) {
      if (aContainer.firstChild) {
        aContainer.appendChild(this.document.createTextNode(" "));
      }

      let text = aBody.argsToString[aIndex];
      if (!Array.isArray(aItem)) {
        aContainer.appendChild(this.document.createTextNode(text));
        return;
      }

      let elem = this.document.createElement("description");
      elem.classList.add("hud-clickable");
      elem.setAttribute("aria-haspopup", "true");
      elem.appendChild(this.document.createTextNode(text));

      this._addMessageLinkCallback(elem,
        this._consoleLogClick.bind(this, aMessage, elem, aItem));

      aContainer.appendChild(elem);
    }, this);
  },

  











  createLocationNode: function WCF_createLocationNode(aSourceURL, aSourceLine)
  {
    let locationNode = this.document.createElementNS(XUL_NS, "label");

    
    
    let text = WebConsoleUtils.abbreviateSourceURL(aSourceURL);
    if (aSourceLine) {
      text += ":" + aSourceLine;
    }
    locationNode.setAttribute("value", text);

    
    locationNode.setAttribute("crop", "center");
    locationNode.setAttribute("title", aSourceURL);
    locationNode.setAttribute("tooltiptext", aSourceURL);
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
      let viewSourceUtils = this.owner.gViewSourceUtils;
      viewSourceUtils.viewSource(aSourceURL, null, this.document, aSourceLine);
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

  


  copySelectedItems: function WCF_copySelectedItems()
  {
    
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
        strings.push("[" + timestampString + "] " + item.clipboardText);
      }
    }

    clipboardHelper.copyString(strings.join("\n"), this.document);
  },

  



  destroy: function WCF_destroy()
  {
    if (this.jsterm) {
      this.jsterm.destroy();
    }
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
  this.autocompletePopup = new AutocompletePopup(this.hud.owner.chromeDocument);
  this.autocompletePopup.onSelect = this.onAutocompleteSelect.bind(this);
  this.autocompletePopup.onClick = this.acceptProposedCompletion.bind(this);
  this._keyPress = this.keyPress.bind(this);
  this._inputEventHandler = this.inputEventHandler.bind(this);
  this._initUI();
}

JSTerm.prototype = {
  



  lastCompletion: null,

  



  lastInputValue: "",

  



  history: null,

  



  get outputNode() this.hud.outputNode,

  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,

  



  _initUI: function JST__initUI()
  {
    let doc = this.hud.document;
    this.completeNode = doc.querySelector(".jsterm-complete-node");
    this.inputNode = doc.querySelector(".jsterm-input-node");
    this.inputNode.addEventListener("keypress", this._keyPress, false);
    this.inputNode.addEventListener("input", this._inputEventHandler, false);
    this.inputNode.addEventListener("keyup", this._inputEventHandler, false);

    this.lastInputValue && this.setInputValue(this.lastInputValue);
  },

  








  evalInContentSandbox: function JST_evalInContentSandbox(aString, aCallback)
  {
    let message = {
      str: aString,
      resultCacheId: "HUDEval-" + gSequenceId(),
    };

    this.hud.owner.sendMessageToContent("JSTerm:EvalRequest", message, aCallback);

    return message;
  },

  
















  _executeResultCallback:
  function JST__executeResultCallback(aCallback, aResponse, aRequest)
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

    if (aCallback) {
      let oldFlushCallback = this.hud._flushCallback;
      this.hud._flushCallback = function() {
        aCallback();
        oldFlushCallback && oldFlushCallback();
        this.hud._flushCallback = oldFlushCallback;
      }.bind(this);
    }

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

  








  execute: function JST_execute(aExecuteString, aCallback)
  {
    
    aExecuteString = aExecuteString || this.inputNode.value;
    if (!aExecuteString) {
      this.writeOutput("no value to execute", CATEGORY_OUTPUT, SEVERITY_LOG);
      return;
    }

    let node = this.writeOutput(aExecuteString, CATEGORY_INPUT, SEVERITY_LOG);

    let onResult = this._executeResultCallback.bind(this, aCallback);
    let messageToContent = this.evalInContentSandbox(aExecuteString, onResult);
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

    let parent = this.hud.popupset;
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
      hud.owner.sendMessageToContent("ConsoleAPI:ClearCache", {});
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
      id: "HUDComplete-" + gSequenceId(),
      input: this.inputNode.value,
    };

    this.lastCompletion = {
      requestId: message.id,
      completionType: aType,
      value: null,
    };
    let callback = this._receiveAutocompleteProperties.bind(this, aCallback);
    this.hud.owner.sendMessageToContent("JSTerm:Autocomplete", message, callback);
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
      this.hud.owner.sendMessageToContent("JSTerm:ClearObjectCache",
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

    this.hud.owner.sendMessageToContent("JSTerm:GetEvalObject", message, aCallback);
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
        
        panelCacheId: "HUDPanel-" + gSequenceId(),
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

  supportsCommand: function CommandController_supportsCommand(aCommand)
  {
    return this.isCommandEnabled(aCommand);
  },

  isCommandEnabled: function CommandController_isCommandEnabled(aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        
        return this.owner.outputNode.selectedCount > 0;
      case "cmd_fontSizeEnlarge":
      case "cmd_fontSizeReduce":
      case "cmd_fontSizeReset":
      case "cmd_selectAll":
        return true;
    }
  },

  doCommand: function CommandController_doCommand(aCommand)
  {
    switch (aCommand) {
      case "cmd_copy":
        this.copy();
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
    }
  }
};

function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;

