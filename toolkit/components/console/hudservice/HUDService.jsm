









































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["HUDService"];

XPCOMUtils.defineLazyServiceGetter(this, "scriptError",
                                   "@mozilla.org/scripterror;1",
                                   "nsIScriptError");

XPCOMUtils.defineLazyServiceGetter(this, "activityDistributor",
                                   "@mozilla.org/network/http-activity-distributor;1",
                                   "nsIHttpActivityDistributor");

XPCOMUtils.defineLazyServiceGetter(this, "sss",
                                   "@mozilla.org/content/style-sheet-service;1",
                                   "nsIStyleSheetService");

function LogFactory(aMessagePrefix)
{
  function log(aMessage) {
    var _msg = aMessagePrefix + " " + aMessage + "\n";
    dump(_msg);
  }
  return log;
}

let log = LogFactory("*** HUDService:");

const ELEMENT_NS_URI = "http://www.w3.org/1999/xhtml";
const ELEMENT_NS = "html:";
const HUD_STYLESHEET_URI = "chrome://global/skin/headsUpDisplay.css";
const HUD_STRINGS_URI = "chrome://global/locale/headsUpDisplay.properties";

XPCOMUtils.defineLazyGetter(this, "stringBundle", function () {
  return Services.strings.createBundle(HUD_STRINGS_URI);
});

const ERRORS = { LOG_MESSAGE_MISSING_ARGS:
                 "Missing arguments: aMessage, aConsoleNode and aMessageNode are required.",
                 CANNOT_GET_HUD: "Cannot getHeads Up Display with provided ID",
                 MISSING_ARGS: "Missing arguments",
                 LOG_OUTPUT_FAILED: "Log Failure: Could not append messageNode to outputNode",
};

function HUD_SERVICE()
{
  
  if (appName() == "FIREFOX") {
    var mixins = new FirefoxApplicationHooks();
  }
  else {
    throw new Error("Unsupported Application");
  }

  this.mixins = mixins;
  this.storage = new ConsoleStorage();
  this.defaultFilterPrefs = this.storage.defaultDisplayPrefs;
  this.defaultGlobalConsolePrefs = this.storage.defaultGlobalConsolePrefs;

  
  var uri = Services.io.newURI(HUD_STYLESHEET_URI, null, null);
  sss.loadAndRegisterSheet(uri, sss.AGENT_SHEET);

  
  this.startHTTPObservation();
};

HUD_SERVICE.prototype =
{
  





  getStr: function HS_getStr(aName)
  {
    return stringBundle.GetStringFromName(aName);
  },

  





  getFormatStr: function HS_getFormatStr(aName, aArray)
  {
    return stringBundle.formatStringFromName(aName, aArray, aArray.length);
  },

  




  get consoleUI() {
    return HeadsUpDisplayUICommands;
  },

  



  activatedContexts: [],

  


  _headsUpDisplays: {},

  


  displayRegistry: {},

  


  uriRegistry: {},

  


  loadGroups: {},

  



  sequencer: null,

  


  filterPrefs: {},

  










  setOnErrorHandler: function HS_setOnErrorHandler(aWindow) {
    var self = this;
    var window = aWindow.wrappedJSObject;
    var console = window.console;
    var origOnerrorFunc = window.onerror;
    window.onerror = function windowOnError(aErrorMsg, aURL, aLineNumber)
    {
      var lineNum = "";
      if (aLineNumber) {
        lineNum = self.getFormatStr("errLine", [aLineNumber]);
      }
      console.error(aErrorMsg + " @ " + aURL + " " + lineNum);
      if (origOnerrorFunc) {
        origOnerrorFunc(aErrorMsg, aURL, aLineNumber);
      }
      return false;
    };
  },

  






  registerActiveContext: function HS_registerActiveContext(aContextDOMId)
  {
    this.activatedContexts.push(aContextDOMId);
  },

  




  currentContext: function HS_currentContext() {
    return this.mixins.getCurrentContext();
  },

  





  unregisterActiveContext: function HS_deregisterActiveContext(aContextDOMId)
  {
    var domId = aContextDOMId.split("_")[1];
    var idx = this.activatedContexts.indexOf(domId);
    if (idx > -1) {
      this.activatedContexts.splice(idx, 1);
    }
  },

  





  canActivateContext: function HS_canActivateContext(aContextDOMId)
  {
    var domId = aContextDOMId.split("_")[1];
    for (var idx in this.activatedContexts) {
      if (this.activatedContexts[idx] == domId){
        return true;
      }
    }
    return false;
  },

  





  activateHUDForContext: function HS_activateHUDForContext(aContext)
  {
    var window = aContext.linkedBrowser.contentWindow;
    var id = aContext.linkedBrowser.parentNode.getAttribute("id");
    this.registerActiveContext(id);
    HUDService.windowInitializer(window);
  },

  





  deactivateHUDForContext: function HS_deactivateHUDForContext(aContext)
  {
    var gBrowser = HUDService.currentContext().gBrowser;
    var window = aContext.linkedBrowser.contentWindow;
    var browser = gBrowser.getBrowserForDocument(window.top.document);
    var tabId = gBrowser.getNotificationBox(browser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var displayNode = this.getHeadsUpDisplay(hudId);

    this.unregisterActiveContext(hudId);
    this.unregisterDisplay(hudId);
    window.wrappedJSObject.console = null;

  },

  





  clearDisplay: function HS_clearDisplay(aId)
  {
    var displayNode = this.getOutputNodeById(aId);
    var outputNode = displayNode.querySelectorAll(".hud-output-node")[0];

    while (outputNode.firstChild) {
      outputNode.removeChild(outputNode.firstChild);
    }
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
  },

  



  hudWeakReferences: {},

  






  registerHUDWeakReference:
  function HS_registerHUDWeakReference(aHUDRef, aHUDId)
  {
    this.hudWeakReferences[aHUDId] = aHUDRef;
  },

  





  deleteHeadsUpDisplay: function HS_deleteHeadsUpDisplay(aHUDId)
  {
    delete this.hudWeakReferences[aHUDId].get();
  },

  






  registerDisplay: function HS_registerDisplay(aHUDId, aURISpec)
  {
    

    if (!aHUDId || !aURISpec){
      throw new Error(ERRORS.MISSING_ARGS);
    }
    this.filterPrefs[aHUDId] = this.defaultFilterPrefs;
    this.displayRegistry[aHUDId] = aURISpec;
    this._headsUpDisplays[aHUDId] = { id: aHUDId, };
    this.registerActiveContext(aHUDId);
    
    this.storage.createDisplay(aHUDId);

    var huds = this.uriRegistry[aURISpec];
    var foundHUDId = false;

    if (huds) {
      var len = huds.length;
      for (var i = 0; i < len; i++) {
        if (huds[i] == aHUDId) {
          foundHUDId = true;
          break;
        }
      }
      if (!foundHUDId) {
        this.uriRegistry[aURISpec].push(aHUDId);
      }
    }
    else {
      this.uriRegistry[aURISpec] = [aHUDId];
    }
  },

  





  unregisterDisplay: function HS_unregisterDisplay(aId)
  {
    
    
    var outputNode = this.mixins.getOutputNodeById(aId);
    var parent = outputNode.parentNode;
    var splitters = parent.querySelectorAll("splitter");
    var len = splitters.length;
    for (var i = 0; i < len; i++) {
      if (splitters[i].getAttribute("class") == "hud-splitter") {
        splitters[i].parentNode.removeChild(splitters[i]);
        break;
      }
    }
    
    parent.removeChild(outputNode);
    
    delete this._headsUpDisplays[aId];
    
    this.deleteHeadsUpDisplay(aId);
    
    this.storage.removeDisplay(aId);
    let displays = this.displays();

    var uri  = this.displayRegistry[aId];
    var specHudArr = this.uriRegistry[uri];

    for (var i = 0; i < specHudArr.length; i++) {
      if (specHudArr[i] == aId) {
        specHudArr.splice(i, 1);
      }
    }
    delete displays[aId];
    delete this.displayRegistry[aId];
  },

  




  shutdown: function HS_shutdown()
  {
    for (var displayId in this._headsUpDisplays) {
      this.unregisterDisplay(displayId);
    }
    
    delete this.storage;

     var xulWindow = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
       .getInterface(Ci.nsIWebNavigation)
       .QueryInterface(Ci.nsIDocShellTreeItem)
       .rootTreeItem
       .QueryInterface(Ci.nsIInterfaceRequestor)
       .getInterface(Ci.nsIDOMWindow);

    xulWindow = XPCNativeWrapper.unwrap(xulWindow);
    var gBrowser = xulWindow.gBrowser;
    gBrowser.tabContainer.removeEventListener("TabClose", this.onTabClose, false);
  },

  





  getDisplayByURISpec: function HS_getDisplayByURISpec(aURISpec)
  {
    
    var hudIds = this.uriRegistry[aURISpec];
    if (hudIds.length == 1) {
      
      return this.getHeadsUpDisplay(hudIds[0]);
    }
    else {
      
      
      return this.getHeadsUpDisplay(hudIds[0]);
    }
  },

  





  getHeadsUpDisplay: function HS_getHeadsUpDisplay(aId)
  {
    return this.mixins.getOutputNodeById(aId);
  },

  





  getOutputNodeById: function HS_getOutputNodeById(aId)
  {
    return this.mixins.getOutputNodeById(aId);
  },

  




  displays: function HS_displays() {
    return this._headsUpDisplays;
  },

  





  getHUDIdsForURISpec: function HS_getHUDIdsForURISpec(aURISpec)
  {
    if (this.uriRegistry[aURISpec]) {
      return this.uriRegistry[aURISpec];
    }
    return [];
  },

  



  displaysIndex: function HS_displaysIndex()
  {
    var props = [];
    for (var prop in this._headsUpDisplays) {
      props.push(prop);
    }
    return props;
  },

  





  getFilterStringByHUDId: function HS_getFilterStringbyHUDId(aHUDId) {
    var hud = this.getHeadsUpDisplay(aHUDId);
    var filterStr = hud.querySelectorAll(".hud-filter-box")[0].value;
    return filterStr || null;
  },

  



  hudFilterStrings: {},

  






  updateFilterText: function HS_updateFiltertext(aTextBoxNode)
  {
    var hudId = aTextBoxNode.getAttribute(hudId);
    this.hudFilterStrings[hudId] = aTextBoxNode.value || null;
  },

  






  filterLogMessage:
  function HS_filterLogMessage(aFilterString, aMessageNode)
  {
    aFilterString = aFilterString.toLowerCase();
    var messageText = aMessageNode.innerHTML.toLowerCase();
    var idx = messageText.indexOf(aFilterString);
    if (idx > -1) {
      return { strLength: aFilterString.length, strIndex: idx };
    }
    else {
      return null;
    }
  },

  





  getFilterTextBox: function HS_getFilterTextBox(aHUDId)
  {
    var hud = this.getHeadsUpDisplay(aHUDId);
    return hud.querySelectorAll(".hud-filter-box")[0];
  },

  










  logHUDMessage: function HS_logHUDMessage(aMessage,
                                           aConsoleNode,
                                           aMessageNode,
                                           aFilterState,
                                           aFilterString)
  {
    if (!aFilterState) {
      
      return;
    }

    if (!aMessage) {
      throw new Error(ERRORS.MISSING_ARGS);
    }

    if (aFilterString) {
      var filtered = this.filterLogMessage(aFilterString, aMessageNode);
      if (filtered) {
        
        aConsoleNode.appendChild(aMessageNode);
        aMessageNode.scrollIntoView(false);
      }
      else {
        
        
        var hiddenMessage = ConsoleUtils.hideLogMessage(aMessageNode);
        aConsoleNode.appendChild(hiddenMessage);
      }
    }
    else {
      
      aConsoleNode.appendChild(aMessageNode);
      aMessageNode.scrollIntoView(false);
    }
    
    this.storage.recordEntry(aMessage.hudId, aMessage);
  },

  








  logConsoleMessage: function HS_logConsoleMessage(aMessage,
                                                   aConsoleNode,
                                                   aMessageNode,
                                                   aFilterState,
                                                   aFilterString)
  {
    if (aFilterState){
      aConsoleNode.appendChild(aMessageNode);
      aMessageNode.scrollIntoView(false);
    }
    
    this.storage.recordEntry(aMessage.hudId, aMessage);
  },

  









  logMessage: function HS_logMessage(aMessage, aConsoleNode, aMessageNode)
  {
    if (!aMessage) {
      throw new Error(ERRORS.MISSING_ARGS);
    }

    var hud = this.getHeadsUpDisplay(aMessage.hudId);
    
    var filterState = this.getFilterState(aMessage.hudId, aMessage.logLevel);
    var filterString = this.getFilterStringByHUDId(aMessage.hudId);

    switch (aMessage.origin) {
      case "network":
      case "HUDConsole":
      case "console-listener":
        this.logHUDMessage(aMessage, aConsoleNode, aMessageNode, filterState, filterString);
        break;
      default:
        
        break;
    }
  },

  




  reportConsoleServiceMessage:
  function HS_reportConsoleServiceMessage(aConsoleMessage)
  {
    this.logActivity("console-listener", null, aConsoleMessage);
  },

  




  reportConsoleServiceContentScriptError:
  function HS_reportConsoleServiceContentScriptError(aScriptError)
  {
    try {
      var uri = Services.io.newURI(aScriptError.sourceName, null, null);
    }
    catch(ex) {
      var uri = { spec: "" };
    }
    this.logActivity("console-listener", uri, aScriptError);
  },

  






  generateConsoleMessage:
  function HS_generateConsoleMessage(aMessage, flag)
  {
    let message = scriptError; 
    message.init(aMessage.message, null, null, 0, 0, flag,
                 "HUDConsole");
    return message;
  },

  




  registerApplicationHooks:
  function HS_registerApplications(aAppName, aHooksObject)
  {
    switch(aAppName) {
      case "FIREFOX":
        this.applicationHooks = aHooksObject;
        return;
      default:
        throw new Error("MOZ APPLICATION UNSUPPORTED");
    }
  },

  




  applicationHooks: null,

  





  getLoadContext: function HS_getLoadContext(aChannel)
  {
    if (!aChannel) {
      return null;
    }
    var loadContext;
    var callbacks = aChannel.notificationCallbacks;

    loadContext =
      aChannel.notificationCallbacks.getInterface(Ci.nsILoadContext);
    if (!loadContext) {
      loadContext =
        aChannel.QueryInterface(Ci.nsIRequest).loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext);
    }
    return loadContext;
  },

  





  getWindowFromContext: function HS_getWindowFromContext(aLoadContext)
  {
    if (!aLoadContext) {
      throw new Error("loadContext is null");
    }
    if (aLoadContext.isContent) {
      if (aLoadContext.associatedWindow) {
        return aLoadContext.associatedWindow;
      }
      else if (aLoadContext.topWindow) {
        return aLoadContext.topWindow;
      }
    }
    throw new Error("Cannot get window from " + aLoadContext);
  },

  getChromeWindowFromContentWindow:
  function HS_getChromeWindowFromContentWindow(aContentWindow)
  {
    if (!aContentWindow) {
      throw new Error("Cannot get contentWindow via nsILoadContext");
    }
    var win = aContentWindow.QueryInterface(Ci.nsIDOMWindow)
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem)
      .rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindow)
      .QueryInterface(Ci.nsIDOMChromeWindow);
    return win;
  },

  





  getOutputNodeFromWindow:
  function HS_getOutputNodeFromWindow(aWindow)
  {
    var browser = gBrowser.getBrowserForDocument(aWindow.top.document);
    var tabId = gBrowser.getNotificationBox(browser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var displayNode = this.getHeadsUpDisplay(hudId);
    return displayNode.querySelectorAll(".hud-output-node")[0];
  },

  





  getOutputNodeFromRequest: function HS_getOutputNodeFromRequest(aRequest)
  {
    var context = this.getLoadContext(aRequest);
    var window = this.getWindowFromContext(context);
    return this.getOutputNodeFromWindow(window);
  },

  getLoadContextFromChannel: function HS_getLoadContextFromChannel(aChannel)
  {
    try {
      return aChannel.QueryInterface(Ci.nsIChannel).notificationCallbacks.getInterface(Ci.nsILoadContext);
    }
    catch (ex) {
      
    }
    try {
      return aChannel.QueryInterface(Ci.nsIChannel).loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext);
    }
    catch (ex) {
      
    }
    return null;
  },

  getWindowFromLoadContext:
  function HS_getWindowFromLoadContext(aLoadContext)
  {
    if (aLoadContext.topWindow) {
      return aLoadContext.topWindow;
    }
    else {
      return aLoadContext.associatedWindow;
    }
  },

  




  startHTTPObservation: function HS_httpObserverFactory()
  {
    
    var self = this;
    var httpObserver = {
      observeActivity :
      function (aChannel, aActivityType, aActivitySubtype,
                aTimestamp, aExtraSizeData, aExtraStringData)
      {
        var loadGroup;
        if (aActivityType ==
            activityDistributor.ACTIVITY_TYPE_HTTP_TRANSACTION) {
          try {
            var loadContext = self.getLoadContextFromChannel(aChannel);
            
            
            var window = self.getWindowFromLoadContext(loadContext);
            window = XPCNativeWrapper.unwrap(window);
            var chromeWin = self.getChromeWindowFromContentWindow(window);
            var vboxes =
              chromeWin.document.getElementsByTagName("vbox");
            var hudId;
            for (var i = 0; i < vboxes.length; i++) {
              if (vboxes[i].getAttribute("class") == "hud-box") {
                hudId = vboxes[i].getAttribute("id");
              }
            }
            loadGroup = self.getLoadGroup(hudId);
          }
          catch (ex) {
            loadGroup = aChannel.QueryInterface(Ci.nsIChannel)
                        .QueryInterface(Ci.nsIRequest).loadGroup;
          }

          if (!loadGroup) {
              return;
          }

          aChannel = aChannel.QueryInterface(Ci.nsIHttpChannel);

          var transCodes = this.httpTransactionCodes;

          var httpActivity = {
            channel: aChannel,
            loadGroup: loadGroup,
            type: aActivityType,
            subType: aActivitySubtype,
            timestamp: aTimestamp,
            extraSizeData: aExtraSizeData,
            extraStringData: aExtraStringData,
            stage: transCodes[aActivitySubtype],
          };
          if (aActivitySubtype ==
              activityDistributor.ACTIVITY_SUBTYPE_REQUEST_HEADER ) {
                
                
                httpActivity.httpId = self.sequenceId();
                let loggedNode =
                  self.logActivity("network", aChannel.URI, httpActivity);
                self.httpTransactions[aChannel] =
                  new Number(httpActivity.httpId);
          }
        }
      },

      httpTransactionCodes: {
        0x5001: "REQUEST_HEADER",
        0x5002: "REQUEST_BODY_SENT",
        0x5003: "RESPONSE_START",
        0x5004: "RESPONSE_HEADER",
        0x5005: "RESPONSE_COMPLETE",
        0x5006: "TRANSACTION_CLOSE",
      }
    };

    activityDistributor.addObserver(httpObserver);
  },

  
  
  httpTransactions: {},

  






  logNetActivity: function HS_logNetActivity(aType, aURI, aActivityObject)
  {
    var displayNode, outputNode, hudId;
    try {
      displayNode =
      this.getDisplayByLoadGroup(aActivityObject.loadGroup,
                                 {URI: aURI}, aActivityObject);
      if (!displayNode) {
        return;
      }
      outputNode = displayNode.querySelectorAll(".hud-output-node")[0];
      hudId = displayNode.getAttribute("id");

      if (!outputNode) {
        outputNode = this.getOutputNodeFromRequest(aActivityObject.request);
        hudId = outputNode.ownerDocument.querySelectorAll(".hud-box")[0].
                getAttribute("id");
      }

      
      if (!this.getFilterState(hudId, "network")) {
        return;
      }

      
      
      var domId = "hud-log-node-" + this.sequenceId();

      var message = { logLevel: aType,
                      activityObj: aActivityObject,
                      hudId: hudId,
                      origin: "network",
                      domId: domId,
                    };
      var msgType = this.getStr("typeNetwork");
      var msg = msgType + " " +
        aActivityObject.channel.requestMethod +
        " " +
        aURI.spec;
      message.message = msg;
      var messageObject =
      this.messageFactory(message, aType, outputNode, aActivityObject);
      this.logMessage(messageObject.messageObject, outputNode, messageObject.messageNode);
    }
    catch (ex) {
      Cu.reportError(ex);
    }
  },

  






  logConsoleActivity: function HS_logConsoleActivity(aURI, aActivityObject)
  {
    var displayNode, outputNode, hudId;
    try {
        var hudIds = this.uriRegistry[aURI.spec];
        hudId = hudIds[0];
    }
    catch (ex) {
      
      
      
      if (!displayNode) {
        return;
      }
    }

    var _msgLogLevel = this.scriptMsgLogLevel[aActivityObject.flags];
    var msgLogLevel = this.getStr(_msgLogLevel);

    var logLevel = "warn";

    if (aActivityObject.flags in this.scriptErrorFlags) {
      logLevel = this.scriptErrorFlags[aActivityObject.flags];
    }

    
    var filterState = this.getFilterState(hudId, logLevel);

    if (!filterState) {
      
      return;
    }

    
    
    var message = {
      activity: aActivityObject,
      origin: "console-listener",
      hudId: hudId,
    };

    var lineColSubs = [aActivityObject.columnNumber,
                       aActivityObject.lineNumber];
    var lineCol = this.getFormatStr("errLineCol", lineColSubs);

    var errFileSubs = [aActivityObject.sourceName];
    var errFile = this.getFormatStr("errFile", errFileSubs);

    var msgCategory = this.getStr("msgCategory");

    message.logLevel = logLevel;
    message.level = logLevel;

    message.message = msgLogLevel + " " +
                      aActivityObject.errorMessage + " " +
                      errFile + " " +
                      lineCol + " " +
                      msgCategory + " " + aActivityObject.category;

    displayNode = this.getHeadsUpDisplay(hudId);
    outputNode = displayNode.querySelectorAll(".hud-output-node")[0];

    var messageObject =
    this.messageFactory(message, message.level, outputNode, aActivityObject);

    this.logMessage(messageObject.messageObject, outputNode, messageObject.messageNode);
  },

  










  logActivity: function HS_logActivity(aType, aURI, aActivityObject)
  {
    var displayNode, outputNode, hudId;

    if (aType == "network") {
      var result = this.logNetActivity(aType, aURI, aActivityObject);
    }
    else if (aType == "console-listener") {
      this.logConsoleActivity(aURI, aActivityObject);
    }
  },

  






  updateLoadGroup: function HS_updateLoadGroup(aId, aLoadGroup)
  {
    if (this.loadGroups[aId] == undefined) {
      this.loadGroups[aId] = { id: aId,
                               loadGroup: Cu.getWeakReference(aLoadGroup) };
    }
    else {
      this.loadGroups[aId].loadGroup = Cu.getWeakReference(aLoadGroup);
    }
  },

  





  getLoadGroup: function HS_getLoadGroup(aId)
  {
    try {
      return this.loadGroups[aId].loadGroup.get();
    }
    catch (ex) {
      return null;
    }
  },

  





  getDisplayByLoadGroup:
  function HS_getDisplayByLoadGroup(aLoadGroup, aChannel, aActivityObject)
  {
    if (!aLoadGroup) {
      return null;
    }
    var trackedLoadGroups = this.getAllLoadGroups();
    var len = trackedLoadGroups.length;
    for (var i = 0; i < len; i++) {
      try {
        var unwrappedLoadGroup =
        XPCNativeWrapper.unwrap(trackedLoadGroups[i].loadGroup);
        if (aLoadGroup == unwrappedLoadGroup) {
          return this.getOutputNodeById(trackedLoadGroups[i].hudId);
        }
      }
      catch (ex) {
        
      }
    }
    
    
    return null;
  },

  




  getAllLoadGroups: function HS_getAllLoadGroups()
  {
    var loadGroups = [];
    for (var hudId in this.loadGroups) {
      let loadGroupObj = { loadGroup: this.loadGroups[hudId].loadGroup.get(),
                           hudId: this.loadGroups[hudId].id,
                         };
      loadGroups.push(loadGroupObj);
    }
    return loadGroups;
  },

  






  getActivityOutputNode: function HS_getActivityOutputNode(aURI)
  {
    
    var display = this.getDisplayByURISpec(aURI.spec);
    if (display) {
      return this.getOutputNodeById(display);
    }
    else {
      throw new Error("Cannot get outputNode by hudId");
    }
  },

  








  messageFactory:
  function messageFactory(aMessage, aLevel, aOutputNode, aActivityObject)
  {
    
    return new LogMessage(aMessage, aLevel, aOutputNode,  aActivityObject);
  },

  






  initializeJSTerm: function HS_initializeJSTerm(aContext, aParentNode)
  {
    
    var context = Cu.getWeakReference(aContext);
    var firefoxMixin = new JSTermFirefoxMixin(context, aParentNode);
    var jsTerm = new JSTerm(context, aParentNode, firefoxMixin);
    
    
  },

  





  getContentWindowFromHUDId: function HS_getContentWindowFromHUDId(aHUDId)
  {
    var hud = this.getHeadsUpDisplay(aHUDId);
    var nodes = hud.parentNode.childNodes;

    for (var i = 0; i < nodes.length; i++) {
      if (nodes[i].contentWindow) {
        return nodes[i].contentWindow;
      }
    }
    throw new Error("HS_getContentWindowFromHUD: Cannot get contentWindow");
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

  scriptErrorFlags: {
    0: "error",
    1: "warn",
    2: "exception",
    4: "strict"
  },

  


  scriptMsgLogLevel: {
    0: "typeError",
    1: "typeWarning",
    2: "typeException",
    4: "typeStrict",
  },

  





  onTabClose: function HS_onTabClose(aEvent)
  {
    var browser = aEvent.target;
    var tabId = gBrowser.getNotificationBox(browser).getAttribute("id");
    var hudId = "hud_" + tabId;
    this.unregisterDisplay(hudId);
  },

  





  windowInitializer: function HS_WindowInitalizer(aContentWindow)
  {
    var xulWindow = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem)
      .rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindow);

    if (aContentWindow.document.location.href == "about:blank" &&
        HUDWindowObserver.initialConsoleCreated == false) {
      
      
      return;
    }

    let xulWindow = XPCNativeWrapper.unwrap(xulWindow);
    let gBrowser = xulWindow.gBrowser;


    var container = gBrowser.tabContainer;
    container.addEventListener("TabClose", this.onTabClose, false);

    if (gBrowser && !HUDWindowObserver.initialConsoleCreated) {
      HUDWindowObserver.initialConsoleCreated = true;
    }

    let _browser =
      gBrowser.getBrowserForDocument(aContentWindow.document.wrappedJSObject);
    let nBox = gBrowser.getNotificationBox(_browser);
    let nBoxId = nBox.getAttribute("id");
    let hudId = "hud_" + nBoxId;

    if (!this.canActivateContext(hudId)) {
      return;
    }

    this.registerDisplay(hudId, aContentWindow.document.location.href);

    
    let _console = aContentWindow.wrappedJSObject.console;
    if (!_console) {
      
      let hudNode;
      let childNodes = nBox.childNodes;

      for (var i = 0; i < childNodes.length; i++) {
        let id = childNodes[i].getAttribute("id");
        if (id.split("_")[0] == "hud") {
          hudNode = childNodes[i];
          break;
        }
      }

      if (!hudNode) {
        
        let config = { parentNode: nBox,
                       contentWindow: aContentWindow
                     };

        let _hud = new HeadsUpDisplay(config);

        let hudWeakRef = Cu.getWeakReference(_hud);
        HUDService.registerHUDWeakReference(hudWeakRef, hudId);
      }
      else {
        
        let config = { hudNode: hudNode,
                       consoleOnly: true,
                       contentWindow: aContentWindow
                     };

        let _hud = new HeadsUpDisplay(config);

        let hudWeakRef = Cu.getWeakReference(_hud);
        HUDService.registerHUDWeakReference(hudWeakRef, hudId);

        aContentWindow.wrappedJSObject.console = _hud.console;
      }
    }
    
    this.setOnErrorHandler(aContentWindow);
  }
};










function HeadsUpDisplay(aConfig)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aConfig.consoleOnly) {
    this.HUDBox = aConfig.hudNode;
    this.parentNode = aConfig.hudNode.parentNode;
    this.notificationBox = this.parentNode;
    this.contentWindow = aConfig.contentWindow;
    this.uriSpec = aConfig.contentWindow.location.href;
    this.reattachConsole();
    this.HUDBox.querySelectorAll(".jsterm-input-node")[0].focus();
    return;
  }

  this.HUDBox = null;

  if (aConfig.parentNode) {
    
    
    
    
    
    this.parentNode = aConfig.parentNode;
    this.notificationBox = aConfig.parentNode;
    this.chromeDocument = aConfig.parentNode.ownerDocument;
    this.contentWindow = aConfig.contentWindow;
    this.uriSpec = aConfig.contentWindow.location.href;
    this.hudId = "hud_" + aConfig.parentNode.getAttribute("id");
  }
  else {
    
    
    
    let windowEnum = Services.wm.getEnumerator("navigator:browser");
    let parentNode;
    let contentDocument;
    let contentWindow;
    let chromeDocument;

    
    

    while (windowEnum.hasMoreElements()) {
      let window = windowEnum.getNext();
      try {
        let gBrowser = window.gBrowser;
        let _browsers = gBrowser.browsers;
        let browserLen = _browsers.length;

        for (var i = 0; i < browserLen; i++) {
          var _notificationBox = gBrowser.getNotificationBox(_browsers[i]);
          this.notificationBox = _notificationBox;

          if (_notificationBox.getAttribute("id") == aConfig.parentNodeId) {
            this.parentNodeId = _notificationBox.getAttribute("id");
            this.hudId = "hud_" + this.parentNodeId;

            parentNode = _notificationBox;

            this.contentDocument =
              _notificationBox.childNodes[0].contentDocument;
            this.contentWindow =
              _notificationBox.childNodes[0].contentWindow;
            this.uriSpec = aConfig.contentWindow.location.href;

            this.chromeDocument =
              _notificationBox.ownerDocument;

            break;
          }
        }
      }
      catch (ex) {
        Cu.reportError(ex);
      }

      if (parentNode) {
        break;
      }
    }
    if (!parentNode) {
      throw new Error(this.ERRORS.PARENTNODE_NOT_FOUND);
    }
    this.parentNode = parentNode;
  }
  
  try  {
    this.HTMLFactory = NodeFactory("html", "html", this.chromeDocument);
  }
  catch(ex) {
    Cu.reportError(ex);
  }

  this.XULFactory = NodeFactory("xul", "xul", this.chromeDocument);
  this.textFactory = NodeFactory("text", "xul", this.chromeDocument);

  
  let hudBox = this.createHUD();

  let splitter = this.chromeDocument.createElement("splitter");
  splitter.setAttribute("collapse", "before");
  splitter.setAttribute("resizeafter", "flex");
  splitter.setAttribute("class", "hud-splitter");

  let grippy = this.chromeDocument.createElement("grippy");
  this.notificationBox.insertBefore(splitter,
                                    this.notificationBox.childNodes[1]);
  splitter.appendChild(grippy);

  let console = this.createConsole();

  this.contentWindow.wrappedJSObject.console = console;

  
  try {
    this.createConsoleInput(this.contentWindow, this.consoleWrap, this.outputNode);
    this.HUDBox.querySelectorAll(".jsterm-input-node")[0].focus();
  }
  catch (ex) {
    Cu.reportError(ex);
  }
}

HeadsUpDisplay.prototype = {
  





  getStr: function HUD_getStr(aName)
  {
    return stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function HUD_getFormatStr(aName, aArray)
  {
    return stringBundle.formatStringFromName(aName, aArray, aArray.length);
  },

  



  jsterm: null,

  





  createConsoleInput:
  function HUD_createConsoleInput(aWindow, aParentNode, aExistingConsole)
  {
    var context = Cu.getWeakReference(aWindow);

    if (appName() == "FIREFOX") {
      let outputCSSClassOverride = "hud-msg-node hud-console";
      let mixin = new JSTermFirefoxMixin(context, aParentNode, aExistingConsole, outputCSSClassOverride);
      this.jsterm = new JSTerm(context, aParentNode, mixin);
    }
    else {
      throw new Error("Unsupported Gecko Application");
    }
  },

  




  reattachConsole: function HUD_reattachConsole()
  {
    this.hudId = this.HUDBox.getAttribute("id");

    
    this.outputNode = this.HUDBox.querySelectorAll(".hud-output-node")[0];

    this.chromeDocument = this.HUDBox.ownerDocument;

    if (this.outputNode) {
      
      this.createConsole();
    }
    else {
      throw new Error("Cannot get output node");
    }
  },

  




  get loadGroup()
  {
    var loadGroup = this.contentWindow
                    .QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocumentLoader).loadGroup;
    return loadGroup;
  },

  





  makeHTMLNode:
  function HUD_makeHTMLNode(aTag)
  {
    var element;

    if (this.HTMLFactory) {
      element = this.HTMLFactory(aTag);
    }
    else {
      var ns = ELEMENT_NS;
      var nsUri = ELEMENT_NS_URI;
      var tag = ns + aTag;
      element = this.chromeDocument.createElementNS(nsUri, tag);
    }

    return element;
  },

  





  makeXULNode:
  function HUD_makeXULNode(aTag)
  {
    return this.XULFactory(aTag);
  },

  




  clearConsoleOutput: function HUD_clearConsoleOutput()
  {
    for each (var node in this.outputNode.childNodes) {
      this.outputNode.removeChild(node);
    }
  },

  




  makeHUDNodes: function HUD_makeHUDNodes()
  {
    let self = this;
    this.HUDBox = this.makeXULNode("vbox");
    this.HUDBox.setAttribute("id", this.hudId);
    this.HUDBox.setAttribute("class", "hud-box");

    var height = Math.ceil((this.contentWindow.innerHeight * .33)) + "px";
    var style = "height: " + height + ";";
    this.HUDBox.setAttribute("style", style);

    let outerWrap = this.makeXULNode("vbox");
    outerWrap.setAttribute("class", "hud-outer-wrapper");
    outerWrap.setAttribute("flex", "1");

    let consoleCommandSet = this.makeXULNode("commandset");
    outerWrap.appendChild(consoleCommandSet);

    let consoleWrap = this.makeXULNode("vbox");
    this.consoleWrap = consoleWrap;
    consoleWrap.setAttribute("class", "hud-console-wrapper");
    consoleWrap.setAttribute("flex", "1");

    this.outputNode = this.makeXULNode("vbox");
    this.outputNode.setAttribute("class", "hud-output-node");
    this.outputNode.setAttribute("flex", "1");

    this.filterBox = this.makeXULNode("textbox");
    this.filterBox.setAttribute("class", "hud-filter-box");
    this.filterBox.setAttribute("hudId", this.hudId);
    this.filterBox.setAttribute("placeholder", this.getStr("stringFilter"));

    this.filterClearButton = this.makeXULNode("button");
    this.filterClearButton.setAttribute("class", "hud-filter-clear");
    this.filterClearButton.setAttribute("label", this.getStr("stringFilterClear"));
    this.filterClearButton.setAttribute("hudId", this.hudId);

    this.setFilterTextBoxEvents();

    this.consoleClearButton = this.makeXULNode("button");
    this.consoleClearButton.setAttribute("class", "hud-console-clear");
    this.consoleClearButton.setAttribute("label", this.getStr("btnClear"));
    this.consoleClearButton.setAttribute("buttonType", "clear");
    this.consoleClearButton.setAttribute("hudId", this.hudId);
    var command = "HUDConsoleUI.command(this)";
    this.consoleClearButton.setAttribute("oncommand", command);

    this.filterPrefs = HUDService.getDefaultFilterPrefs(this.hudId);

    let consoleFilterToolbar = this.makeFilterToolbar();
    consoleFilterToolbar.setAttribute("id", "viewGroup");
    this.consoleFilterToolbar = consoleFilterToolbar;
    consoleWrap.appendChild(consoleFilterToolbar);

    consoleWrap.appendChild(this.outputNode);
    outerWrap.appendChild(consoleWrap);

    this.jsTermParentNode = outerWrap;
    this.HUDBox.appendChild(outerWrap);
    return this.HUDBox;
  },


  




  setFilterTextBoxEvents: function HUD_setFilterTextBoxEvents()
  {
    var self = this;
    function keyPress(aEvent)
    {
      HUDService.updateFilterText(aEvent.target);
    }
    this.filterBox.addEventListener("keydown", keyPress, false);

    function filterClick(aEvent) {
      self.filterBox.value = "";
    }
    this.filterClearButton.addEventListener("click", filterClick, false);
  },

  




  makeFilterToolbar: function HUD_makeFilterToolbar()
  {
    let buttons = ["Network", "CSSParser", "Exception", "Error",
                   "Info", "Warn", "Log",];

    let toolbar = this.makeXULNode("toolbar");
    toolbar.setAttribute("class", "hud-console-filter-toolbar");
    toolbar.setAttribute("mode", "text");

    toolbar.appendChild(this.consoleClearButton);
    let btn;
    for (var i = 0; i < buttons.length; i++) {
      if (buttons[i] == "Clear") {
        btn = this.makeButton(buttons[i], "plain");
      }
      else {
        btn = this.makeButton(buttons[i], "checkbox");
      }
      toolbar.appendChild(btn);
    }
    toolbar.appendChild(this.filterBox);
    toolbar.appendChild(this.filterClearButton);
    return toolbar;
  },

  makeButton: function HUD_makeButton(aName, aType)
  {
    var self = this;
    let prefKey = aName.toLowerCase();
    let btn = this.makeXULNode("toolbarbutton");

    if (aType == "checkbox") {
      btn.setAttribute("type", aType);
    }
    btn.setAttribute("hudId", this.hudId);
    btn.setAttribute("buttonType", prefKey);
    btn.setAttribute("class", "hud-filter-btn");
    let key = "btn" + aName;
    btn.setAttribute("label", this.getStr(key));
    key = "tip" + aName;
    btn.setAttribute("tooltip", this.getStr(key));

    if (aType == "checkbox") {
      btn.setAttribute("checked", this.filterPrefs[prefKey]);
      function toggle(btn) {
        self.consoleFilterCommands.toggle(btn);
      };

      btn.setAttribute("oncommand", "HUDConsoleUI.toggleFilter(this);");
    }
    else {
      var command = "HUDConsoleUI.command(this)";
      btn.setAttribute("oncommand", command);
    }
    return btn;
  },

  createHUD: function HUD_createHUD()
  {
    let self = this;
    if (this.HUDBox) {
      return this.HUDBox;
    }
    else  {
      this.makeHUDNodes();

      let nodes = this.notificationBox.insertBefore(this.HUDBox,
        this.notificationBox.childNodes[0]);

      return this.HUDBox;
    }
  },

  get console() { return this._console || this.createConsole(); },

  getLogCount: function HUD_getLogCount()
  {
    return this.outputNode.childNodes.length;
  },

  getLogNodes: function HUD_getLogNodes()
  {
    return this.outputNode.childNodes;
  },

  






  createConsole: function HUD_createConsole()
  {
    return new HUDConsole(this);
  },

  ERRORS: {
    HUD_BOX_DOES_NOT_EXIST: "Heads Up Display does not exist",
    TAB_ID_REQUIRED: "Tab DOM ID is required",
    PARENTNODE_NOT_FOUND: "parentNode element not found"
  }
};












function HUDConsole(aHeadsUpDisplay)
{
  let hud = aHeadsUpDisplay;
  let hudId = hud.hudId;
  let outputNode = hud.outputNode;
  let chromeDocument = hud.chromeDocument;
  let makeHTMLNode = hud.makeHTMLNode;

  aHeadsUpDisplay._console = this;

  HUDService.updateLoadGroup(hudId, hud.loadGroup);

  let sendToHUDService = function console_send(aLevel, aArguments)
  {
    
    var filterState = HUDService.getFilterState(hudId, aLevel);

    if (!filterState) {
      
      return;
    }

    let ts = ConsoleUtils.timestamp();
    let messageNode = hud.makeHTMLNode("div");

    let klass = "hud-msg-node hud-" + aLevel;

    messageNode.setAttribute("class", klass);

    let argumentArray = [];
    for (var i = 0; i < aArguments.length; i++) {
      argumentArray.push(aArguments[i]);
    }

    let message = argumentArray.join(' ');
    let timestampedMessage = ConsoleUtils.timestampString(ts) + ": " +
      message;

    messageNode.appendChild(chromeDocument.createTextNode(timestampedMessage));

    
    let messageObject = {
      logLevel: aLevel,
      hudId: hud.hudId,
      message: message,
      timestamp: ts,
      origin: "HUDConsole",
    };

    HUDService.logMessage(messageObject, hud.outputNode, messageNode);
  }

  
  
  this.log = function console_log()
  {
    sendToHUDService("log", arguments);
  },

  this.info = function console_info()
  {
    sendToHUDService("info", arguments);
  },

  this.warn = function console_warn()
  {
    sendToHUDService("warn", arguments);
  },

  this.error = function console_error()
  {
    sendToHUDService("error", arguments);
  },

  this.exception = function console_exception()
  {
    sendToHUDService("exception", arguments);
  }
};








function NodeFactory(aFactoryType, aNameSpace, aDocument)
{
  
  const ELEMENT_NS_URI = "http://www.w3.org/1999/xhtml";

  if (aFactoryType == "text") {
    function factory(aText) {
      return aDocument.createTextNode(aText);
    }
    return factory;
  }
  else {
    if (aNameSpace == "xul") {
      function factory(aTag)
      {
        return aDocument.createElement(aTag);
      }
      return factory;
    }
    else {
      function factory(aTag)
      {
        var tag = "html:" + aTag;
        return aDocument.createElementNS(ELEMENT_NS_URI, tag);
      }
      return factory;
    }
  }
}





const STATE_NORMAL = 0;
const STATE_QUOTE = 2;
const STATE_DQUOTE = 3;

const OPEN_BODY = '{[('.split('');
const CLOSE_BODY = '}])'.split('');
const OPEN_CLOSE_BODY = {
  '{': '}',
  '[': ']',
  '(': ')'
};




















function findCompletionBeginning(aStr)
{
  let bodyStack = [];

  let state = STATE_NORMAL;
  let start = 0;
  let c;
  for (let i = 0; i < aStr.length; i++) {
    c = aStr[i];

    switch (state) {
      
      case STATE_NORMAL:
        if (c == '"') {
          state = STATE_DQUOTE;
        }
        else if (c == '\'') {
          state = STATE_QUOTE;
        }
        else if (c == ';') {
          start = i + 1;
        }
        else if (c == ' ') {
          start = i + 1;
        }
        else if (OPEN_BODY.indexOf(c) != -1) {
          bodyStack.push({
            token: c,
            start: start
          });
          start = i + 1;
        }
        else if (CLOSE_BODY.indexOf(c) != -1) {
          var last = bodyStack.pop();
          if (OPEN_CLOSE_BODY[last.token] != c) {
            return {
              err: "syntax error"
            };
          }
          if (c == '}') {
            start = i + 1;
          }
          else {
            start = last.start;
          }
        }
        break;

      
      case STATE_DQUOTE:
        if (c == '\\') {
          i ++;
        }
        else if (c == '\n') {
          return {
            err: "unterminated string literal"
          };
        }
        else if (c == '"') {
          state = STATE_NORMAL;
        }
        break;

      
      case STATE_QUOTE:
        if (c == '\\') {
          i ++;
        }
        else if (c == '\n') {
          return {
            err: "unterminated string literal"
          };
          return;
        }
        else if (c == '\'') {
          state = STATE_NORMAL;
        }
        break;
    }
  }

  return {
    state: state,
    startPos: start
  };
}




















function JSPropertyProvider(aScope, aInputValue)
{
  let obj = aScope;

  
  
  let beginning = findCompletionBeginning(aInputValue);

  
  if (beginning.err) {
    return null;
  }

  
  
  if (beginning.state != STATE_NORMAL) {
    return null;
  }

  let completionPart = aInputValue.substring(beginning.startPos);

  
  if (completionPart.trim() == "") {
    return null;
  }

  let properties = completionPart.split('.');
  let matchProp;
  if (properties.length > 1) {
      matchProp = properties[properties.length - 1].trimLeft();
      properties.pop();
      for each (var prop in properties) {
        prop = prop.trim();

        
        
        if (typeof obj === "undefined" || obj === null) {
          return null;
        }

        
        
        if (obj.__lookupGetter__(prop)) {
          return null;
        }
        obj = obj[prop];
      }
  }
  else {
    matchProp = properties[0].trimLeft();
  }

  
  
  if (typeof obj === "undefined" || obj === null) {
    return null;
  }

  let matches = [];
  for (var prop in obj) {
    matches.push(prop);
  }

  matches = matches.filter(function(item) {
    return item.indexOf(matchProp) == 0;
  }).sort();

  return {
    matchProp: matchProp,
    matches: matches
  };
}
























function JSTerm(aContext, aParentNode, aMixin)
{
  

  this.application = appName();
  this.context = aContext;
  this.parentNode = aParentNode;
  this.mixins = aMixin;

  this.elementFactory =
    NodeFactory("html", "html", aParentNode.ownerDocument);

  this.xulElementFactory =
    NodeFactory("xul", "xul", aParentNode.ownerDocument);

  this.textFactory = NodeFactory("text", "xul", aParentNode.ownerDocument);

  this.setTimeout = aParentNode.ownerDocument.defaultView.setTimeout;

  this.historyIndex = 0;
  this.historyPlaceHolder = 0;  
  this.log = LogFactory("*** JSTerm:");
  this.init();
}

JSTerm.prototype = {

  propertyProvider: JSPropertyProvider,

  COMPLETE_FORWARD: 0,
  COMPLETE_BACKWARD: 1,
  COMPLETE_HINT_ONLY: 2,

  init: function JST_init()
  {
    this.createSandbox();
    this.inputNode = this.mixins.inputNode;
    this.scrollToNode = this.mixins.scrollToNode;
    let eventHandler = this.keyDown();
    this.inputNode.addEventListener('keypress', eventHandler, false);
    this.outputNode = this.mixins.outputNode;
    if (this.mixins.cssClassOverride) {
      this.cssClassOverride = this.mixins.cssClassOverride;
    }
  },

  get codeInputString()
  {
    
    
    return this.inputNode.value;
  },

  generateUI: function JST_generateUI()
  {
    this.mixins.generateUI();
  },

  attachUI: function JST_attachUI()
  {
    this.mixins.attachUI();
  },

  createSandbox: function JST_setupSandbox()
  {
    
    this._window.wrappedJSObject.jsterm = {};
    this.console = this._window.wrappedJSObject.console;
    this.sandbox = new Cu.Sandbox(this._window);
    this.sandbox.window = this._window;
    this.sandbox.console = this.console;
    this.sandbox.__proto__ = this._window.wrappedJSObject;
  },

  get _window()
  {
    return this.context.get().QueryInterface(Ci.nsIDOMWindowInternal);
  },

  execute: function JST_execute(aExecuteString)
  {
    
    var str = aExecuteString || this.inputNode.value;
    if (!str) {
      this.console.log("no value to execute");
      return;
    }

    this.writeOutput(str);

    try {
      var execStr = "with(window) {" + str + "}";
      var result =
        Cu.evalInSandbox(execStr,  this.sandbox, "default", "HUD Console", 1);

      if (result || result === false || result === " ") {
        this.writeOutput(result);
      }
      else if (result === undefined) {
        this.writeOutput("undefined");
      }
      else if (result === null) {
        this.writeOutput("null");
      }
    }
    catch (ex) {
      if (ex) {
        this.console.error(ex);
      }
    }

    this.history.push(str);
    this.historyIndex++;
    this.historyPlaceHolder = this.history.length;
    this.inputNode.value = "";
  },

  writeOutput: function JST_writeOutput(aOutputMessage)
  {
    var node = this.elementFactory("div");
    if (this.cssClassOverride) {
      node.setAttribute("class", this.cssClassOverride);
    }
    else {
      node.setAttribute("class", "jsterm-output-line");
    }
    var textNode = this.textFactory(aOutputMessage);
    node.appendChild(textNode);
    this.outputNode.appendChild(node);
    node.scrollIntoView(false);
  },

  clearOutput: function JST_clearOutput()
  {
    let outputNode = this.outputNode;

    while (outputNode.firstChild) {
      outputNode.removeChild(outputNode.firstChild);
    }
  },

  keyDown: function JSTF_keyDown(aEvent)
  {
    var self = this;
    function handleKeyDown(aEvent) {
      
      var setTimeout = aEvent.target.ownerDocument.defaultView.setTimeout;
      var target = aEvent.target;
      var tmp;

      if (aEvent.ctrlKey) {
        switch (aEvent.charCode) {
          case 97:
            
            tmp = self.codeInputString;
            setTimeout(function() {
              self.inputNode.value = tmp;
              self.inputNode.setSelectionRange(0, 0);
            }, 0);
            break;
          case 101:
            
            tmp = self.codeInputString;
            self.inputNode.value = "";
            setTimeout(function(){
              var endPos = tmp.length + 1;
              self.inputNode.value = tmp;
            }, 0);
            break;
          default:
            return;
        }
        return;
      }
      else if (aEvent.shiftKey && aEvent.keyCode == 13) {
        
        
        return;
      }
      else {
        switch(aEvent.keyCode) {
          case 13:
            
            self.execute();
            break;
          case 38:
            
            if (self.caretInFirstLine()){
              self.historyPeruse(true);
              if (aEvent.cancelable) {
                let inputEnd = self.inputNode.value.length;
                self.inputNode.setSelectionRange(inputEnd, inputEnd);
                aEvent.preventDefault();
              }
            }
            break;
          case 40:
            
            if (self.caretInLastLine()){
              self.historyPeruse(false);
              if (aEvent.cancelable) {
                let inputEnd = self.inputNode.value.length;
                self.inputNode.setSelectionRange(inputEnd, inputEnd);
                aEvent.preventDefault();
              }
            }
            break;
          case 9:
            
            
            
            
            if (aEvent.shiftKey) {
              self.complete(self.COMPLETE_BACKWARD);
            }
            else {
              self.complete(self.COMPLETE_FORWARD);
            }
            var bool = aEvent.cancelable;
            if (bool) {
              aEvent.preventDefault();
            }
            else {
              
            }
            aEvent.target.focus();
            break;
          case 8:
            
          case 46:
            
            
            break;
          default:
            
            
            
            
            
            var value = self.inputNode.value;
            setTimeout(function() {
              if (self.inputNode.value !== value) {
                self.complete(self.COMPLETE_HINT_ONLY);
              }
            }, 0);
            break;
        }
        return;
      }
    }
    return handleKeyDown;
  },

  historyPeruse: function JST_historyPeruse(aFlag) {
    if (!this.history.length) {
      return;
    }

    
    if (aFlag) {
      if (this.historyPlaceHolder <= 0) {
        return;
      }

      let inputVal = this.history[--this.historyPlaceHolder];
      if (inputVal){
        this.inputNode.value = inputVal;
      }
    }
    
    else {
      if (this.historyPlaceHolder == this.history.length - 1) {
        this.historyPlaceHolder ++;
        this.inputNode.value = "";
        return;
      }
      else if (this.historyPlaceHolder >= (this.history.length)) {
        return;
      }
      else {
        let inputVal = this.history[++this.historyPlaceHolder];
        if (inputVal){
          this.inputNode.value = inputVal;
        }
      }
    }
  },

  refocus: function JSTF_refocus()
  {
    this.inputNode.blur();
    this.inputNode.focus();
  },

  caretInFirstLine: function JSTF_caretInFirstLine()
  {
    var firstLineBreak = this.codeInputString.indexOf("\n");
    return ((firstLineBreak == -1) ||
            (this.codeInputString.selectionStart <= firstLineBreak));
  },

  caretInLastLine: function JSTF_caretInLastLine()
  {
    var lastLineBreak = this.codeInputString.lastIndexOf("\n");
    return (this.inputNode.selectionEnd > lastLineBreak);
  },

  history: [],

  
  lastCompletion: null,

  

























  complete: function JSTF_complete(type)
  {
    let inputNode = this.inputNode;
    let inputValue = inputNode.value;
    let selStart = inputNode.selectionStart, selEnd = inputNode.selectionEnd;

    
    if (selStart > selEnd) {
      let newSelEnd = selStart;
      selStart = selEnd;
      selEnd = newSelEnd;
    }

    
    if (selEnd != inputValue.length) {
      this.lastCompletion = null;
      return;
    }

    
    inputValue = inputValue.substring(0, selStart);

    let matches;
    let matchIndexToUse;
    let matchOffset;
    let completionStr;

    
    
    if (this.lastCompletion && inputValue == this.lastCompletion.value) {
      matches = this.lastCompletion.matches;
      matchOffset = this.lastCompletion.matchOffset;
      if (type === this.COMPLETE_BACKWARD) {
        this.lastCompletion.index --;
      }
      else if (type === this.COMPLETE_FORWARD) {
        this.lastCompletion.index ++;
      }
      matchIndexToUse = this.lastCompletion.index;
    }
    else {
      
      let completion = this.propertyProvider(this.sandbox.window, inputValue);
      if (!completion) {
        return;
      }
      matches = completion.matches;
      matchIndexToUse = 0;
      matchOffset = completion.matchProp.length
      
      this.lastCompletion = {
        index: 0,
        value: inputValue,
        matches: matches,
        matchOffset: matchOffset
      };
    }

    if (matches.length != 0) {
      
      if (matchIndexToUse < 0) {
        matchIndexToUse = matches.length + (matchIndexToUse % matches.length);
        if (matchIndexToUse == matches.length) {
          matchIndexToUse = 0;
        }
      }
      else {
        matchIndexToUse = matchIndexToUse % matches.length;
      }

      completionStr = matches[matchIndexToUse].substring(matchOffset);
      this.inputNode.value = inputValue +  completionStr;

      selEnd = inputValue.length + completionStr.length;

      
      
      
      if (matches.length > 1 || type === this.COMPLETE_HINT_ONLY) {
        inputNode.setSelectionRange(selStart, selEnd);
      }
      else {
        inputNode.setSelectionRange(selEnd, selEnd);
      }
    }
  }
};







function
JSTermFirefoxMixin(aContext,
                   aParentNode,
                   aExistingConsole,
                   aCSSClassOverride)
{
  
  
  
  this.cssClassOverride = aCSSClassOverride;
  this.context = aContext;
  this.parentNode = aParentNode;
  this.existingConsoleNode = aExistingConsole;
  this.setTimeout = aParentNode.ownerDocument.defaultView.setTimeout;

  if (aParentNode.ownerDocument) {
    this.elementFactory =
      NodeFactory("html", "html", aParentNode.ownerDocument);

    this.xulElementFactory =
      NodeFactory("xul", "xul", aParentNode.ownerDocument);

    this.textFactory = NodeFactory("text", "xul", aParentNode.ownerDocument);
    this.generateUI();
    this.attachUI();
  }
  else {
    throw new Error("aParentNode should be a DOM node with an ownerDocument property ");
  }
}

JSTermFirefoxMixin.prototype = {
  





  generateUI: function JSTF_generateUI()
  {
    let inputNode = this.xulElementFactory("textbox");
    inputNode.setAttribute("class", "jsterm-input-node");

    if (this.existingConsoleNode == undefined) {
      
      let term = this.elementFactory("div");
      term.setAttribute("class", "jsterm-wrapper-node");
      term.setAttribute("flex", "1");

      let outputNode = this.elementFactory("div");
      outputNode.setAttribute("class", "jsterm-output-node");

      let scrollToNode = this.elementFactory("div");
      scrollToNode.setAttribute("class", "jsterm-scroll-to-node");

      
      outputNode.appendChild(scrollToNode);
      term.appendChild(outputNode);
      term.appendChild(inputNode);

      this.scrollToNode = scrollToNode;
      this.outputNode = outputNode;
      this.inputNode = inputNode;
      this.term = term;
    }
    else {
      this.inputNode = inputNode;
      this.term = inputNode;
      this.outputNode = this.existingConsoleNode;
    }
  },

  get inputValue()
  {
    return this.inputNode.value;
  },

  attachUI: function JSTF_attachUI()
  {
    this.parentNode.appendChild(this.term);
  }
};




function LogMessage(aMessage, aLevel, aOutputNode, aActivityObject)
{
  if (!aOutputNode || !aOutputNode.ownerDocument) {
    throw new Error("aOutputNode is required and should be type nsIDOMNode");
  }
  if (!aMessage.origin) {
    throw new Error("Cannot create and log a message without an origin");
  }
  this.message = aMessage;
  if (aMessage.domId) {
    
    
    this.domId = aMessage.domId;
  }
  this.activityObject = aActivityObject;
  this.outputNode = aOutputNode;
  this.level = aLevel;
  this.origin = aMessage.origin;

  this.elementFactory =
  NodeFactory("html", "html", aOutputNode.ownerDocument);

  this.xulElementFactory =
  NodeFactory("xul", "xul", aOutputNode.ownerDocument);

  this.textFactory = NodeFactory("text", "xul", aOutputNode.ownerDocument);

  this.createLogNode();
}

LogMessage.prototype = {

  




  createLogNode: function LM_createLogNode()
  {
    this.messageNode = this.elementFactory("div");

    var ts = ConsoleUtils.timestamp();
    var timestampedMessage = ConsoleUtils.timestampString(ts) + ": " +
      this.message.message;
    var messageTxtNode = this.textFactory(timestampedMessage);

    this.messageNode.appendChild(messageTxtNode);

    var klass = "hud-msg-node hud-" + this.level;
    this.messageNode.setAttribute("class", klass);

    var self = this;

    var messageObject = {
      logLevel: self.level,
      message: self.message,
      timestamp: ts,
      activity: self.activityObject,
      origin: self.origin,
      hudId: self.message.hudId,
    };

    this.messageObject = messageObject;
  }
};







function FirefoxApplicationHooks()
{ }

FirefoxApplicationHooks.prototype = {

  


  get chromeWindows()
  {
    var windows = [];
    var enumerator = Services.ww.getWindowEnumerator(null);
    while (enumerator.hasMoreElements()) {
      windows.push(enumerator.getNext());
    }
    return windows;
  },

  





  getOutputNodeById: function FAH_getOutputNodeById(aId)
  {
    if (!aId) {
      throw new Error("FAH_getOutputNodeById: id is null!!");
    }
    var enumerator = Services.ww.getWindowEnumerator(null);
    while (enumerator.hasMoreElements()) {
      let window = enumerator.getNext();
      let node = window.document.getElementById(aId);
      if (node) {
        return node;
      }
    }
    throw new Error("Cannot get outputNode by id");
  },

  




  getCurrentContext: function FAH_getCurrentContext()
  {
    return Services.wm.getMostRecentWindow("navigator:browser");
  }
};










ConsoleUtils = {

  




  timestamp: function ConsoleUtils_timestamp()
  {
    return Date.now();
  },

  






  timestampString: function ConsoleUtils_timestampString(ms)
  {
    
    var d = new Date(ms ? ms : null);

    function pad(n, mil)
    {
      if (mil) {
        return n < 100 ? "0" + n : n;
      }
      else {
        return n < 10 ? "0" + n : n;
      }
    }

    return pad(d.getHours()) + ":"
      + pad(d.getMinutes()) + ":"
      + pad(d.getSeconds()) + ":"
      + pad(d.getMilliseconds(), true);
  },

  





  hideLogMessage: function ConsoleUtils_hideLogMessage(aMessageNode) {
    var klass = aMessageNode.getAttribute("class");
    klass += " hud-hidden";
    aMessageNode.setAttribute("class", klass);
    return aMessageNode;
  }
};








function NodeFactory(aFactoryType, aNameSpace, aDocument)
{
  
  const ELEMENT_NS_URI = "http://www.w3.org/1999/xhtml";

  if (aFactoryType == "text") {
    function factory(aText) {
      return aDocument.createTextNode(aText);
    }
    return factory;
  }
  else {
    if (aNameSpace == "xul") {
      function factory(aTag) {
        return aDocument.createElement(aTag);
      }
      return factory;
    }
    else {
      function factory(aTag) {
        var tag = "html:" + aTag;
        return aDocument.createElementNS(ELEMENT_NS_URI, tag);
      }
      return factory;
    }
  }
}






HeadsUpDisplayUICommands = {
  toggleHUD: function UIC_toggleHUD() {
    var window = HUDService.currentContext();
    var gBrowser = window.gBrowser;
    var linkedBrowser = gBrowser.selectedTab.linkedBrowser;
    var tabId = gBrowser.getNotificationBox(linkedBrowser).getAttribute("id");
    var hudId = "hud_" + tabId;
    var hud = gBrowser.selectedTab.ownerDocument.getElementById(hudId);
    if (hud) {
      HUDService.deactivateHUDForContext(gBrowser.selectedTab);
    }
    else {
      HUDService.activateHUDForContext(gBrowser.selectedTab);
    }
  },

  toggleFilter: function UIC_toggleFilter(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    var state = HUDService.getFilterState(hudId, filter);
    if (state) {
      HUDService.setFilterState(hudId, filter, false);
      aButton.setAttribute("checked", false);
    }
    else {
      HUDService.setFilterState(hudId, filter, true);
      aButton.setAttribute("checked", true);
    }
  },

  command: function UIC_command(aButton) {
    var filter = aButton.getAttribute("buttonType");
    var hudId = aButton.getAttribute("hudId");
    if (filter == "clear") {
      HUDService.clearDisplay(hudId);
    }
  },

};





var prefs = Services.prefs;

const GLOBAL_STORAGE_INDEX_ID = "GLOBAL_CONSOLE";
const PREFS_BRANCH_PREF = "devtools.hud.display.filter";
const PREFS_PREFIX = "devtools.hud.display.filter.";
const PREFS = { network: PREFS_PREFIX + "network",
                cssparser: PREFS_PREFIX + "cssparser",
                exception: PREFS_PREFIX + "exception",
                error: PREFS_PREFIX + "error",
                info: PREFS_PREFIX + "info",
                warn: PREFS_PREFIX + "warn",
                log: PREFS_PREFIX + "log",
                global: PREFS_PREFIX + "global",
              };

function ConsoleStorage()
{
  this.sequencer = null;
  this.consoleDisplays = {};
  
  this.displayIndexes = {};
  this.globalStorageIndex = [];
  this.globalDisplay = {};
  this.createDisplay(GLOBAL_STORAGE_INDEX_ID);
  
  

  
  this.displayPrefs = {};

  
  let filterPrefs;
  let defaultDisplayPrefs;

  try {
    filterPrefs = prefs.getBoolPref(PREFS_BRANCH_PREF);
  }
  catch (ex) {
    filterPrefs = false;
  }

  
  
  

  if (filterPrefs) {
    defaultDisplayPrefs = {
      network: (prefs.getBoolPref(PREFS.network) ? true: false),
      cssparser: (prefs.getBoolPref(PREFS.cssparser) ? true: false),
      exception: (prefs.getBoolPref(PREFS.exception) ? true: false),
      error: (prefs.getBoolPref(PREFS.error) ? true: false),
      info: (prefs.getBoolPref(PREFS.info) ? true: false),
      warn: (prefs.getBoolPref(PREFS.warn) ? true: false),
      log: (prefs.getBoolPref(PREFS.log) ? true: false),
      global: (prefs.getBoolPref(PREFS.global) ? true: false),
    };
  }
  else {
    prefs.setBoolPref(PREFS_BRANCH_PREF, false);
    
    prefs.setBoolPref(PREFS.network, true);
    prefs.setBoolPref(PREFS.cssparser, true);
    prefs.setBoolPref(PREFS.exception, true);
    prefs.setBoolPref(PREFS.error, true);
    prefs.setBoolPref(PREFS.info, true);
    prefs.setBoolPref(PREFS.warn, true);
    prefs.setBoolPref(PREFS.log, true);
    prefs.setBoolPref(PREFS.global, false);

    defaultDisplayPrefs = {
      network: prefs.getBoolPref(PREFS.network),
      cssparser: prefs.getBoolPref(PREFS.cssparser),
      exception: prefs.getBoolPref(PREFS.exception),
      error: prefs.getBoolPref(PREFS.error),
      info: prefs.getBoolPref(PREFS.info),
      warn: prefs.getBoolPref(PREFS.warn),
      log: prefs.getBoolPref(PREFS.log),
      global: prefs.getBoolPref(PREFS.global),
    };
  }
  this.defaultDisplayPrefs = defaultDisplayPrefs;
}

ConsoleStorage.prototype = {

  updateDefaultDisplayPrefs:
  function CS_updateDefaultDisplayPrefs(aPrefsObject) {
    prefs.setBoolPref(PREFS.network, (aPrefsObject.network ? true : false));
    prefs.setBoolPref(PREFS.cssparser, (aPrefsObject.cssparser ? true : false));
    prefs.setBoolPref(PREFS.exception, (aPrefsObject.exception ? true : false));
    prefs.setBoolPref(PREFS.error, (aPrefsObject.error ? true : false));
    prefs.setBoolPref(PREFS.info, (aPrefsObject.info ? true : false));
    prefs.setBoolPref(PREFS.warn, (aPrefsObject.warn ? true : false));
    prefs.setBoolPref(PREFS.log, (aPrefsObject.log ? true : false));
    prefs.setBoolPref(PREFS.global, (aPrefsObject.global ? true : false));
  },

  sequenceId: function CS_sequencerId()
  {
    if (!this.sequencer) {
      this.sequencer = this.createSequencer();
    }
    return this.sequencer.next();
  },

  createSequencer: function CS_createSequencer()
  {
    function sequencer(aInt) {
      while(1) {
        aInt++;
        yield aInt;
      }
    }
    return sequencer(-1);
  },

  globalStore: function CS_globalStore(aIndex)
  {
    return this.displayStore(GLOBAL_CONSOLE_DOM_NODE_ID);
  },

  displayStore: function CS_displayStore(aId)
  {
    var self = this;
    var idx = -1;
    var id = aId;
    var aLength = self.displayIndexes[id].length;

    function displayStoreGenerator(aInt, aLength)
    {
      
      
      while(1) {
        
        aInt++;
        var indexIt = self.displayIndexes[id];
        var index = indexIt[aInt];
        if (aLength < aInt) {
          
          var newLength = self.displayIndexes[id].length;
          if (newLength > aLength) {
            aLength = newLength;
          }
          else {
            throw new StopIteration();
          }
        }
        var entry = self.consoleDisplays[id][index];
        yield entry;
      }
    }

    return displayStoreGenerator(-1, aLength);
  },

  recordEntries: function CS_recordEntries(aHUDId, aConfigArray)
  {
    var len = aConfigArray.length;
    for (var i = 0; i < len; i++){
      this.recordEntry(aHUDId, aConfigArray[i]);
    }
  },


  recordEntry: function CS_recordEntry(aHUDId, aConfig)
  {
    var id = this.sequenceId();

    this.globalStorageIndex[id] = { hudId: aHUDId };

    var displayStorage = this.consoleDisplays[aHUDId];

    var displayIndex = this.displayIndexes[aHUDId];

    if (displayStorage && displayIndex) {
      var entry = new ConsoleEntry(aConfig, id);
      displayIndex.push(entry.id);
      displayStorage[entry.id] = entry;
      return entry;
    }
    else {
      throw new Error("Cannot get displayStorage or index object for id " + aHUDId);
    }
  },

  getEntry: function CS_getEntry(aId)
  {
    var display = this.globalStorageIndex[aId];
    var storName = display.hudId;
    return this.consoleDisplays[storName][aId];
  },

  updateEntry: function CS_updateEntry(aUUID)
  {
    
    
  },

  createDisplay: function CS_createdisplay(aId)
  {
    if (!this.consoleDisplays[aId]) {
      this.consoleDisplays[aId] = {};
      this.displayIndexes[aId] = [];
    }
  },

  removeDisplay: function CS_removeDisplay(aId)
  {
    try {
      delete this.consoleDisplays[aId];
      delete this.displayIndexes[aId];
    }
    catch (ex) {
      Cu.reportError("Could not remove console display for id " + aId);
    }
  }
};









function ConsoleEntry(aConfig, id)
{
  if (!aConfig.logLevel && aConfig.message) {
    throw new Error("Missing Arguments when creating a console entry");
  }

  this.config = aConfig;
  this.id = id;
  for (var prop in aConfig) {
    if (!(typeof aConfig[prop] == "function")){
      this[prop] = aConfig[prop];
    }
  }

  if (aConfig.logLevel == "network") {
    this.transactions = { };
    if (aConfig.activity) {
      this.transactions[aConfig.activity.stage] = aConfig.activity;
    }
  }

}

ConsoleEntry.prototype = {

  updateTransaction: function CE_updateTransaction(aActivity) {
    this.transactions[aActivity.stage] = aActivity;
  }
};





HUDWindowObserver = {
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.nsIObserver,]
  ),

  init: function HWO_init()
  {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, "content-document-global-created", false);
  },

  observe: function HWO_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "content-document-global-created") {
      HUDService.windowInitializer(aSubject);
    }
    else if (aTopic == "xpcom-shutdown") {
      this.uninit();
    }
  },

  uninit: function HWO_uninit()
  {
    Services.obs.removeObserver(this, "content-document-global-created");
    HUDService.shutdown();
  },

  



  initialConsoleCreated: false,
};











HUDConsoleObserver = {
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.nsIObserver]
  ),

  init: function HCO_init()
  {
    Services.console.registerListener(this);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  observe: function HCO_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      Services.console.unregisterListener(this);
    }

    if (aSubject instanceof Ci.nsIScriptError) {
      switch (aSubject.category) {
        case "XPConnect JavaScript":
        case "component javascript":
        case "chrome javascript":
          
          
          return;
        case "HUDConsole":
        case "CSS Parser":
        case "content javascript":
          HUDService.reportConsoleServiceContentScriptError(aSubject);
          return;
        default:
          HUDService.reportConsoleServiceMessage(aSubject);
          return;
      }
    }
  }
};










function appName()
{
  let APP_ID = Services.appinfo.QueryInterface(Ci.nsIXULRuntime).ID;

  let APP_ID_TABLE = {
    "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}": "FIREFOX" ,
    "{3550f703-e582-4d05-9a08-453d09bdfdc6}": "THUNDERBIRD",
    "{a23983c0-fd0e-11dc-95ff-0800200c9a66}": "FENNEC" ,
    "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}": "SEAMONKEY",
  };

  let name = APP_ID_TABLE[APP_ID];

  if (name){
    return name;
  }
  throw new Error("appName: UNSUPPORTED APPLICATION UUID");
}





try {
  
  
  
  var HUDService = new HUD_SERVICE();
  HUDWindowObserver.init();
  HUDConsoleObserver.init();
}
catch (ex) {
  Cu.reportError("HUDService failed initialization.\n" + ex);
  
  
}
