



"use strict";

const FRAME_SCRIPT = "chrome://marionette/content/marionette-listener.js";
const BROWSER_STARTUP_FINISHED = "browser-delayed-startup-finished";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


Cu.import("resource://gre/modules/Log.jsm");
let logger = Log.repository.getLogger("Marionette");
logger.info('marionette-server.js loaded');

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-common.js");
Cu.import("resource://gre/modules/Services.jsm");
loader.loadSubScript("chrome://marionette/content/marionette-frame-manager.js");
Cu.import("chrome://marionette/content/marionette-elements.js");
let utils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);
loader.loadSubScript("chrome://marionette/content/marionette-sendkeys.js", utils);


Services.prefs.setBoolPref('security.turn_off_all_security_so_that_viruses_can_take_over_this_computer',
                           true);
let specialpowers = {};
loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js",
                     specialpowers);
specialpowers.specialPowersObserver = new specialpowers.SpecialPowersObserver();
specialpowers.specialPowersObserver.init();

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

function isMulet() {
  let isMulet = false;
  try {
   isMulet = Services.prefs.getBoolPref("b2g.is_mulet");
  } catch (ex) { }
  return isMulet;
}

Services.prefs.setBoolPref("marionette.contentListener", false);
let appName = isMulet() ? "B2G" : Services.appinfo.name;

let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");
this.DevToolsUtils = DevToolsUtils;
loader.loadSubScript("resource://gre/modules/devtools/transport/transport.js");

let bypassOffline = false;
let qemu = "0";
let device = null;

XPCOMUtils.defineLazyServiceGetter(this, "cookieManager",
                                   "@mozilla.org/cookiemanager;1",
                                   "nsICookieManager");

try {
  XPCOMUtils.defineLazyGetter(this, "libcutils", function () {
    Cu.import("resource://gre/modules/systemlibs.js");
    return libcutils;
  });
  if (libcutils) {
    qemu = libcutils.property_get("ro.kernel.qemu");
    logger.info("B2G emulator: " + (qemu == "1" ? "yes" : "no"));
    device = libcutils.property_get("ro.product.device");
    logger.info("Device detected is " + device);
    bypassOffline = (qemu == "1" || device == "panda");
  }
}
catch(e) {}

if (bypassOffline) {
  logger.info("Bypassing offline status.");
  Services.prefs.setBoolPref("network.gonk.manage-offline-status", false);
  Services.io.manageOfflineStatus = false;
  Services.io.offline = false;
}





let systemMessageListenerReady = false;
Services.obs.addObserver(function() {
  systemMessageListenerReady = true;
}, "system-message-listener-ready", false);



let delayedBrowserStarted = false;
Services.obs.addObserver(function () {
  delayedBrowserStarted = true;
}, BROWSER_STARTUP_FINISHED, false);




function FrameSendNotInitializedError(frame) {
  this.code = 54;
  this.frame = frame;
  this.message = "Error sending message to frame (NS_ERROR_NOT_INITIALIZED)";
  this.toString = function() {
    return this.message + " " + this.frame + "; frame has closed.";
  }
}

function FrameSendFailureError(frame) {
  this.code = 55;
  this.frame = frame;
  this.message = "Error sending message to frame (NS_ERROR_FAILURE)";
  this.toString = function() {
    return this.message + " " + this.frame + "; frame not responding.";
  }
}






function MarionetteServerConnection(aPrefix, aTransport, aServer)
{
  this.uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                   .getService(Ci.nsIUUIDGenerator);

  this.prefix = aPrefix;
  this.server = aServer;
  this.conn = aTransport;
  this.conn.hooks = this;

  
  
  
  this.actorID = "0";
  this.sessionId = null;

  this.globalMessageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                             .getService(Ci.nsIMessageBroadcaster);
  this.messageManager = this.globalMessageManager;
  this.browsers = {}; 
  this.curBrowser = null; 
  this.context = "content";
  this.scriptTimeout = null;
  this.searchTimeout = null;
  this.pageTimeout = null;
  this.timer = null;
  this.inactivityTimer = null;
  this.heartbeatCallback = function () {}; 
  this.marionetteLog = new MarionetteLogObj();
  this.command_id = null;
  this.mainFrame = null; 
  this.curFrame = null; 
  this.mainContentFrameId = null;
  this.importedScripts = FileUtils.getFile('TmpD', ['marionetteChromeScripts']);
  this.importedScriptHashes = {"chrome" : [], "content": []};
  this.currentFrameElement = null;
  this.testName = null;
  this.mozBrowserClose = null;
  this.sandbox = null;
  this.oopFrameId = null; 
  this.sessionCapabilities = {
    
    "browserName": appName,
    "browserVersion": Services.appinfo.version,
    "platformName": Services.appinfo.OS.toUpperCase(),
    "platformVersion": Services.appinfo.platformVersion,

    
    "handlesAlerts": false,
    "nativeEvents": false,
    "raisesAccessibilityExceptions": false,
    "rotatable": appName == "B2G",
    "secureSsl": false,
    "takesElementScreenshot": true,
    "takesScreenshot": true,

    
    "platform": Services.appinfo.OS.toUpperCase(),

    
    "XULappId" : Services.appinfo.ID,
    "appBuildId" : Services.appinfo.appBuildID,
    "device": qemu == "1" ? "qemu" : (!device ? "desktop" : device),
    "version": Services.appinfo.version
  };

  this.observing = null;
}

MarionetteServerConnection.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  


  onPacket: function MSC_onPacket(aPacket) {
    
    if (this.requestTypes && this.requestTypes[aPacket.name]) {
      try {
        this.logRequest(aPacket.name, aPacket);
        this.requestTypes[aPacket.name].bind(this)(aPacket);
      } catch(e) {
        this.conn.send({ error: ("error occurred while processing '" +
                                 aPacket.name),
                        message: e.message });
      }
    } else {
      this.conn.send({ error: "unrecognizedPacketType",
                       message: ('Marionette does not ' +
                                 'recognize the packet type "' +
                                 aPacket.name + '"') });
    }
  },

  onClosed: function MSC_onClosed(aStatus) {
    this.server._connectionClosed(this);
    this.sessionTearDown();
  },

  



  






  switchToGlobalMessageManager: function MDA_switchToGlobalMM() {
    if (this.curBrowser && this.curBrowser.frameManager.currentRemoteFrame !== null) {
      this.curBrowser.frameManager.removeMessageManagerListeners(this.messageManager);
      this.sendAsync("sleepSession", null, null, true);
      this.curBrowser.frameManager.currentRemoteFrame = null;
    }
    this.messageManager = this.globalMessageManager;
  },

  







  sendAsync: function MDA_sendAsync(name, values, commandId, ignoreFailure) {
    let success = true;
    if (commandId) {
      values.command_id = commandId;
    }
    if (this.curBrowser.frameManager.currentRemoteFrame !== null) {
      try {
        this.messageManager.sendAsyncMessage(
          "Marionette:" + name + this.curBrowser.frameManager.currentRemoteFrame.targetFrameId, values);
      }
      catch(e) {
        if (!ignoreFailure) {
          success = false;
          let error = e;
          switch(e.result) {
            case Components.results.NS_ERROR_FAILURE:
              error = new FrameSendFailureError(this.curBrowser.frameManager.currentRemoteFrame);
              break;
            case Components.results.NS_ERROR_NOT_INITIALIZED:
              error = new FrameSendNotInitializedError(this.curBrowser.frameManager.currentRemoteFrame);
              break;
            default:
              break;
          }
          let code = error.hasOwnProperty('code') ? e.code : 500;
          this.sendError(error.toString(), code, error.stack, commandId);
        }
      }
    }
    else {
      this.messageManager.broadcastAsyncMessage(
        "Marionette:" + name + this.curBrowser.curFrameId, values);
    }
    return success;
  },

  logRequest: function MDA_logRequest(type, data) {
    logger.debug("Got request: " + type + ", data: " + JSON.stringify(data) + ", id: " + this.command_id);
  },

  








  sendToClient: function MDA_sendToClient(msg, command_id) {
    logger.info("sendToClient: " + JSON.stringify(msg) + ", " + command_id +
                ", " + this.command_id);
    if (!command_id) {
      logger.warn("got a response with no command_id");
      return;
    }
    else if (command_id != -1) {
      
      
      if (!this.command_id) {
        
        
        
        logger.warn("ignoring duplicate response for command_id " + command_id);
        return;
      }
      else if (this.command_id != command_id) {
        logger.warn("ignoring out-of-sync response");
        return;
      }
    }
    this.conn.send(msg);
    if (command_id != -1) {
      
      
      
      this.command_id = null;
    }
  },

  








  sendResponse: function MDA_sendResponse(value, command_id) {
    if (typeof(value) == 'undefined')
        value = null;
    this.sendToClient({from:this.actorID,
                       sessionId: this.sessionId,
                       value: value}, command_id);
  },

  sayHello: function MDA_sayHello() {
    this.conn.send({ from: "root",
                     applicationType: "gecko",
                     traits: [] });
  },

  getMarionetteID: function MDA_getMarionette() {
    this.conn.send({ "from": "root", "id": this.actorID });
  },

  






  sendOk: function MDA_sendOk(command_id) {
    this.sendToClient({from:this.actorID, ok: true}, command_id);
  },

  












  sendError: function MDA_sendError(message, status, trace, command_id) {
    let error_msg = {message: message, status: status, stacktrace: trace};
    this.sendToClient({from:this.actorID, error: error_msg}, command_id);
  },

  




  getCurrentWindow: function MDA_getCurrentWindow() {
    let type = null;
    if (this.curFrame == null) {
      if (this.curBrowser == null) {
        if (this.context == "content") {
          type = 'navigator:browser';
        }
        return Services.wm.getMostRecentWindow(type);
      }
      else {
        return this.curBrowser.window;
      }
    }
    else {
      return this.curFrame;
    }
  },

  




  getWinEnumerator: function MDA_getWinEnumerator() {
    let type = null;
    if (appName != "B2G" && this.context == "content") {
      type = 'navigator:browser';
    }
    return Services.wm.getEnumerator(type);
  },

  

  addFrameCloseListener: function MDA_addFrameCloseListener(action) {
    let curWindow = this.getCurrentWindow();
    let self = this;
    this.mozBrowserClose = function(e) {
      if (e.target.id == self.oopFrameId) {
        curWindow.removeEventListener('mozbrowserclose', self.mozBrowserClose, true);
        self.switchToGlobalMessageManager();
        self.sendError("The frame closed during the " + action +  ", recovering to allow further communications", 55, null, self.command_id);
      }
    };
    curWindow.addEventListener('mozbrowserclose', this.mozBrowserClose, true);
  },

  








  addBrowser: function MDA_addBrowser(win) {
    let browser = new BrowserObj(win, this);
    let winId = win.QueryInterface(Ci.nsIInterfaceRequestor).
                    getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
    winId = winId + ((appName == "B2G") ? '-b2g' : '');
    this.browsers[winId] = browser;
    this.curBrowser = this.browsers[winId];
    if (this.curBrowser.elementManager.seenItems[winId] == undefined) {
      
      this.curBrowser.elementManager.seenItems[winId] = Cu.getWeakReference(win);
    }
  },

  










  startBrowser: function MDA_startBrowser(win, newSession) {
    this.mainFrame = win;
    this.curFrame = null;
    this.addBrowser(win);
    this.curBrowser.newSession = newSession;
    this.curBrowser.startSession(newSession, win, this.whenBrowserStarted.bind(this));
  },

  








  whenBrowserStarted: function MDA_whenBrowserStarted(win, newSession) {
    utils.window = win;

    try {
      let mm = win.window.messageManager;
      if (!newSession) {
        
        
        
        
        
        if (mm.childCount === 0) {
          this.sendOk(this.command_id);
        } else {
          this.curBrowser.frameRegsPending = mm.childCount;
        }
      }

      if (!Services.prefs.getBoolPref("marionette.contentListener") || !newSession) {
        mm.loadFrameScript(FRAME_SCRIPT, true, true);
        Services.prefs.setBoolPref("marionette.contentListener", true);
      }
    }
    catch (e) {
      
      logger.info("could not load listener into content for page: " + win.location.href);
    }
  },

  







  getVisibleText: function MDA_getVisibleText(el, lines) {
    let nodeName = el.nodeName;
    try {
      if (utils.isElementDisplayed(el)) {
        if (el.value) {
          lines.push(el.value);
        }
        for (var child in el.childNodes) {
          this.getVisibleText(el.childNodes[child], lines);
        };
      }
    }
    catch (e) {
      if (nodeName == "#text") {
        lines.push(el.textContent);
      }
    }
  },

  getCommandId: function MDA_getCommandId() {
    return this.uuidGen.generateUUID().toString();
  },

  


  deleteFile: function(filename) {
    let file = FileUtils.getFile('TmpD', [filename.toString()]);
    if (file.exists()) {
      file.remove(true);
    }
  },

  











  






  newSession: function MDA_newSession(aRequest) {
    logger.info("The newSession request is " + JSON.stringify(aRequest))
    this.command_id = this.getCommandId();
    this.newSessionCommandId = this.command_id;

    this.scriptTimeout = 10000;
    if (aRequest && aRequest.parameters) {
      this.sessionId = aRequest.parameters.session_id ? aRequest.parameters.session_id : null;
      logger.info("Session Id is set to: " + this.sessionId);
      try {
        this.setSessionCapabilities(aRequest.parameters.capabilities);
      } catch (e) {
        
        this.sendError(e.message + " " + JSON.stringify(e.errors), 71, null,
                       this.command_id);
        return;
      }
    }

    if (appName == "Firefox") {
      this._dialogWindowRef = null;
      let modalHandler = this.handleDialogLoad.bind(this);
      this.observing = {
        "tabmodal-dialog-loaded": modalHandler,
        "common-dialog-loaded": modalHandler
      }
      for (let topic in this.observing) {
        Services.obs.addObserver(this.observing[topic], topic, false);
      }
    }

    function waitForWindow() {
      let win = this.getCurrentWindow();
      if (!win) {
        
        let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        checkTimer.initWithCallback(waitForWindow.bind(this), 100,
                                    Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else if (win.document.readyState != "complete") {
        
        let listener = (evt) => {
          
          
          if (evt.target != win.document) {
            return;
          }
          win.removeEventListener("load", listener);
          waitForWindow.call(this);
        };
        win.addEventListener("load", listener, true);
      }
      else {
        let clickToStart;
        try {
          clickToStart = Services.prefs.getBoolPref('marionette.debugging.clicktostart');
        } catch (e) { }
        if (clickToStart && (appName != "B2G")) {
          let pService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                           .getService(Ci.nsIPromptService);
          pService.alert(win, "", "Click to start execution of marionette tests");
        }
        this.startBrowser(win, true);
      }
    }

    function runSessionStart() {
      if (!Services.prefs.getBoolPref("marionette.contentListener")) {
        waitForWindow.call(this);
      }
      else if ((appName != "Firefox") && (this.curBrowser === null)) {
        
        this.addBrowser(this.getCurrentWindow());
        this.curBrowser.startSession(false, this.getCurrentWindow(),
                                     this.whenBrowserStarted);
        this.messageManager.broadcastAsyncMessage("Marionette:restart", {});
      }
      else {
        this.sendError("Session already running", 500, null,
                       this.command_id);
      }
      this.switchToGlobalMessageManager();
    }

    if (!delayedBrowserStarted && (appName != "B2G")) {
      let self = this;
      Services.obs.addObserver(function onStart () {
        Services.obs.removeObserver(onStart, BROWSER_STARTUP_FINISHED);
        runSessionStart.call(self);
      }, BROWSER_STARTUP_FINISHED, false);
    } else {
      runSessionStart.call(this);
    }
  },

  










  getSessionCapabilities: function MDA_getSessionCapabilities() {
    this.command_id = this.getCommandId();

    if (!this.sessionId) {
      this.sessionId = this.uuidGen.generateUUID().toString();
    }

    
    
    
    if (appName == "B2G")
      this.sessionCapabilities.b2g = true;

    this.sendResponse(this.sessionCapabilities, this.command_id);
  },

  












  setSessionCapabilities: function(newCaps) {
    const copy = (from, to={}) => {
      let errors = {};
      for (let key in from) {
        if (key === "desiredCapabilities"){
          
          
          to = copy(from[key], to);
        } else if (key === "requiredCapabilities") {
          for (let caps in from[key]) {
            if (from[key][caps] !== this.sessionCapabilities[caps]) {
              errors[caps] = from[key][caps] + " does not equal " + this.sessionCapabilities[caps]
            }
          }
        }
        to[key] = from[key];
      }
      if (Object.keys(errors).length === 0){
        return to;
      }
      else {
        throw { "message": "Not all requiredCapabilities could be met",
                "errors": errors}
      }
    };

    
    let caps = copy(this.sessionCapabilities);
    caps = copy(newCaps, caps);
    this.sessionCapabilities = caps;
  },

  






  log: function MDA_log(aRequest) {
    this.command_id = this.getCommandId();
    this.marionetteLog.log(aRequest.parameters.value, aRequest.parameters.level);
    this.sendOk(this.command_id);
  },

  


  getLogs: function MDA_getLogs() {
    this.command_id = this.getCommandId();
    this.sendResponse(this.marionetteLog.getLogs(), this.command_id);
  },

  





  setContext: function MDA_setContext(aRequest) {
    this.command_id = this.getCommandId();
    let context = aRequest.parameters.value;
    if (context != "content" && context != "chrome") {
      this.sendError("invalid context", 500, null, this.command_id);
    }
    else {
      this.context = context;
      this.sendOk(this.command_id);
    }
  },

  


  getContext: function MDA_getContext() {
    this.command_id = this.getCommandId();
    this.sendResponse(this.context, this.command_id);
  },

  











  createExecuteSandbox: function MDA_createExecuteSandbox(aWindow, marionette, specialPowers, command_id) {
    let _chromeSandbox = new Cu.Sandbox(aWindow,
       { sandboxPrototype: aWindow, wantXrays: false, sandboxName: ''});
    _chromeSandbox.global = _chromeSandbox;
    _chromeSandbox.testUtils = utils;

    marionette.exports.forEach(function(fn) {
      try {
        _chromeSandbox[fn] = marionette[fn].bind(marionette);
      }
      catch(e) {
        _chromeSandbox[fn] = marionette[fn];
      }
    });

    _chromeSandbox.isSystemMessageListenerReady =
        function() { return systemMessageListenerReady; }

    if (specialPowers == true) {
      loader.loadSubScript("chrome://specialpowers/content/specialpowersAPI.js",
                           _chromeSandbox);
      loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserverAPI.js",
                           _chromeSandbox);
      loader.loadSubScript("chrome://specialpowers/content/ChromePowers.js",
                           _chromeSandbox);
    }

    return _chromeSandbox;
  },

  



  applyArgumentsToSandbox: function MDA_applyArgumentsToSandbox(win, sandbox, args, command_id) {
    try {
      sandbox.__marionetteParams = this.curBrowser.elementManager.convertWrappedArguments(args, win);
    }
    catch(e) {
      this.sendError(e.message, e.code, e.stack, command_id);
    }
    sandbox.__namedArgs = this.curBrowser.elementManager.applyNamedArgs(args);
  },

  













  executeScriptInSandbox: function MDA_executeScriptInSandbox(sandbox, script,
     directInject, async, command_id, timeout) {

    if (directInject && async &&
        (timeout == null || timeout == 0)) {
      this.sendError("Please set a timeout", 21, null, command_id);
      return;
    }

    if (this.importedScripts.exists()) {
      let stream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
      stream.init(this.importedScripts, -1, 0, 0);
      let data = NetUtil.readInputStreamToString(stream, stream.available());
      stream.close();
      script = data + script;
    }

    let res = Cu.evalInSandbox(script, sandbox, "1.8", "dummy file", 0);

    if (directInject && !async &&
        (res == undefined || res.passed == undefined)) {
      this.sendError("finish() not called", 500, null, command_id);
      return;
    }

    if (!async) {
      this.sendResponse(this.curBrowser.elementManager.wrapValue(res),
                        command_id);
    }
  },

  










  execute: function MDA_execute(aRequest, directInject) {
    let inactivityTimeout = aRequest.parameters.inactivityTimeout;
    let timeout = aRequest.parameters.scriptTimeout ? aRequest.parameters.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();
    let script;
    let newSandbox = aRequest.parameters.newSandbox;
    if (newSandbox == undefined) {
      
      
      newSandbox = true;
    }
    if (this.context == "content") {
      this.sendAsync("executeScript",
                     {
                       script: aRequest.parameters.script,
                       args: aRequest.parameters.args,
                       newSandbox: newSandbox,
                       timeout: timeout,
                       specialPowers: aRequest.parameters.specialPowers,
                       filename: aRequest.parameters.filename,
                       line: aRequest.parameters.line
                     },
                     command_id);
      return;
    }

    
    let that = this;
    if (inactivityTimeout) {
     let inactivityTimeoutHandler = function(message, status) {
      let error_msg = {message: value, status: status};
      that.sendToClient({from: that.actorID, error: error_msg},
                        marionette.command_id);
     };
     let setTimer = function() {
      that.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (that.inactivityTimer != null) {
       that.inactivityTimer.initWithCallback(function() {
        inactivityTimeoutHandler("timed out due to inactivity", 28);
       }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }
     }
     setTimer();
     this.heartbeatCallback = function resetInactivityTimer() {
      that.inactivityTimer.cancel();
      setTimer();
     }
    }


    let curWindow = this.getCurrentWindow();
    if (!this.sandbox || newSandbox) {
      let marionette = new Marionette(this, curWindow, "chrome",
                                      this.marionetteLog,
                                      timeout, this.heartbeatCallback, this.testName);
      this.sandbox = this.createExecuteSandbox(curWindow,
                                               marionette,
                                               aRequest.parameters.specialPowers,
                                               command_id);
      if (!this.sandbox)
        return;
    }
    this.applyArgumentsToSandbox(curWindow, this.sandbox, aRequest.parameters.args,
                                 command_id)

    try {
      this.sandbox.finish = function chromeSandbox_finish() {
        if (that.inactivityTimer != null) {
          that.inactivityTimer.cancel();
        }
        return that.sandbox.generate_results();
      };

      if (directInject) {
        script = aRequest.parameters.script;
      }
      else {
        script = "let func = function() {" +
                       aRequest.parameters.script +
                     "};" +
                     "func.apply(null, __marionetteParams);";
      }
      this.executeScriptInSandbox(this.sandbox, script, directInject,
                                  false, command_id, timeout);
    }
    catch (e) {
      let error = createStackMessage(e,
                                     "execute_script",
                                     aRequest.parameters.filename,
                                     aRequest.parameters.line,
                                     script);
      this.sendError(error[0], 17, error[1], command_id);
    }
  },

  





  setScriptTimeout: function MDA_setScriptTimeout(aRequest) {
    this.command_id = this.getCommandId();
    let timeout = parseInt(aRequest.parameters.ms);
    if(isNaN(timeout)){
      this.sendError("Not a Number", 500, null, this.command_id);
    }
    else {
      this.scriptTimeout = timeout;
      this.sendOk(this.command_id);
    }
  },

  







  executeJSScript: function MDA_executeJSScript(aRequest) {
    let timeout = aRequest.parameters.scriptTimeout ? aRequest.parameters.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();

    
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    if (this.context == "chrome") {
      if (aRequest.parameters.async) {
        this.executeWithCallback(aRequest, aRequest.parameters.async);
      }
      else {
        this.execute(aRequest, true);
      }
    }
    else {
      this.sendAsync("executeJSScript",
                     {
                       script: aRequest.parameters.script,
                       args: aRequest.parameters.args,
                       newSandbox: aRequest.parameters.newSandbox,
                       async: aRequest.parameters.async,
                       timeout: timeout,
                       inactivityTimeout: aRequest.parameters.inactivityTimeout,
                       specialPowers: aRequest.parameters.specialPowers,
                       filename: aRequest.parameters.filename,
                       line: aRequest.parameters.line,
                     },
                     command_id);
   }
  },

  














  executeWithCallback: function MDA_executeWithCallback(aRequest, directInject) {
    let inactivityTimeout = aRequest.parameters.inactivityTimeout;
    let timeout = aRequest.parameters.scriptTimeout ? aRequest.parameters.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();
    let script;
    let newSandbox = aRequest.parameters.newSandbox;
    if (newSandbox == undefined) {
      
      
      newSandbox = true;
    }

    if (this.context == "content") {
      this.sendAsync("executeAsyncScript",
                     {
                       script: aRequest.parameters.script,
                       args: aRequest.parameters.args,
                       id: this.command_id,
                       newSandbox: newSandbox,
                       timeout: timeout,
                       inactivityTimeout: inactivityTimeout,
                       specialPowers: aRequest.parameters.specialPowers,
                       filename: aRequest.parameters.filename,
                       line: aRequest.parameters.line
                     },
                     command_id);
      return;
    }

    
    let that = this;
    if (inactivityTimeout) {
     this.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
     if (this.inactivityTimer != null) {
      this.inactivityTimer.initWithCallback(function() {
       chromeAsyncReturnFunc("timed out due to inactivity", 28);
      }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
     }
     this.heartbeatCallback = function resetInactivityTimer() {
      that.inactivityTimer.cancel();
      that.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (that.inactivityTimer != null) {
       that.inactivityTimer.initWithCallback(function() {
        chromeAsyncReturnFunc("timed out due to inactivity", 28);
       }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }
     }
    }

    let curWindow = this.getCurrentWindow();
    let original_onerror = curWindow.onerror;
    that.timeout = timeout;

    function chromeAsyncReturnFunc(value, status, stacktrace) {
      if (that._emu_cbs && Object.keys(that._emu_cbs).length) {
        value = "Emulator callback still pending when finish() called";
        status = 500;
        that._emu_cbs = null;
      }

      if (value == undefined)
        value = null;

      if (command_id == that.command_id) {
        if (that.timer != null) {
          that.timer.cancel();
          that.timer = null;
        }

        curWindow.onerror = original_onerror;

        if (status == 0 || status == undefined) {
          that.sendToClient({from: that.actorID, value: that.curBrowser.elementManager.wrapValue(value), status: status},
                            that.command_id);
        }
        else {
          let error_msg = {message: value, status: status, stacktrace: stacktrace};
          that.sendToClient({from: that.actorID, error: error_msg},
                            that.command_id);
        }
      }

      if (that.inactivityTimer != null) {
        that.inactivityTimer.cancel();
      }
    }

    
    
    
    
    if (aRequest.parameters.debug_script) {
      curWindow.onerror = function (errorMsg, url, lineNumber) {
        chromeAsyncReturnFunc(errorMsg + " at: " + url + " line: " + lineNumber, 17);
        return true;
      };
    }

    function chromeAsyncFinish() {
      chromeAsyncReturnFunc(that.sandbox.generate_results(), 0);
    }

    if (!this.sandbox || newSandbox) {
      let marionette = new Marionette(this, curWindow, "chrome",
                                      this.marionetteLog,
                                      timeout, this.heartbeatCallback, this.testName);
      this.sandbox = this.createExecuteSandbox(curWindow,
                                               marionette,
                                               aRequest.parameters.specialPowers,
                                               command_id);
      if (!this.sandbox)
        return;
    }
    this.applyArgumentsToSandbox(curWindow, this.sandbox, aRequest.parameters.args,
                                 command_id)

    try {

      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (this.timer != null) {
        this.timer.initWithCallback(function() {
          chromeAsyncReturnFunc("timed out", 28);
        }, that.timeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }

      this.sandbox.returnFunc = chromeAsyncReturnFunc;
      this.sandbox.finish = chromeAsyncFinish;

      if (directInject) {
        script = aRequest.parameters.script;
      }
      else {
        script =  '__marionetteParams.push(returnFunc);'
                + 'let marionetteScriptFinished = returnFunc;'
                + 'let __marionetteFunc = function() {' + aRequest.parameters.script + '};'
                + '__marionetteFunc.apply(null, __marionetteParams);';
      }

      this.executeScriptInSandbox(this.sandbox, script, directInject,
                                  true, command_id, timeout);
    } catch (e) {
      let error = createStackMessage(e,
                                     "execute_async_script",
                                     aRequest.parameters.filename,
                                     aRequest.parameters.line,
                                     script);
      chromeAsyncReturnFunc(error[0], 17, error[1]);
    }
  },

  

























  get: function MDA_get(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context != "chrome") {
      aRequest.command_id = command_id;
      aRequest.parameters.pageTimeout = this.pageTimeout;
      this.sendAsync("get", aRequest.parameters, command_id);
      return;
    }

    
    
    
    
    
    if (appName == "Firefox") {
      this.sendError("Cannot navigate in chrome context", 13, null, command_id);
      return;
    }

    this.getCurrentWindow().location.href = aRequest.parameters.url;
    let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let start = new Date().getTime();
    let end = null;

    function checkLoad() {
      end = new Date().getTime();
      let elapse = end - start;
      if (this.pageTimeout == null || elapse <= this.pageTimeout){
        if (curWindow.document.readyState == "complete") {
          sendOk(command_id);
          return;
        }
        else{
          checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
        }
      }
      else{
        sendError("Error loading page", 13, null, command_id);
        return;
      }
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  









  getCurrentUrl: function MDA_getCurrentUrl() {
    let isB2G = appName == "B2G";
    this.command_id = this.getCommandId();
    if (this.context === "chrome") {
      this.sendResponse(this.getCurrentWindow().location.href, this.command_id);
    }
    else {
      if (isB2G) {
        this.sendAsync("getCurrentUrl", {}, this.command_id);
      }
      else {
        this.sendResponse(this.curBrowser
                              .tab
                              .linkedBrowser
                              .contentWindowAsCPOW.location.href, this.command_id);
      }
    }
  },

  


  getTitle: function MDA_getTitle() {
    this.command_id = this.getCommandId();
    if (this.context == "chrome"){
      var curWindow = this.getCurrentWindow();
      var title = curWindow.document.documentElement.getAttribute('title');
      this.sendResponse(title, this.command_id);
    }
    else {
      this.sendAsync("getTitle", {}, this.command_id);
    }
  },

  


  getWindowType: function MDA_getWindowType() {
    this.command_id = this.getCommandId();
      var curWindow = this.getCurrentWindow();
      var type = curWindow.document.documentElement.getAttribute('windowtype');
      this.sendResponse(type, this.command_id);
  },

  


  getPageSource: function MDA_getPageSource(){
    this.command_id = this.getCommandId();
    if (this.context == "chrome"){
      let curWindow = this.getCurrentWindow();
      let XMLSerializer = curWindow.XMLSerializer;
      let pageSource = new XMLSerializer().serializeToString(curWindow.document);
      this.sendResponse(pageSource, this.command_id);
    }
    else {
      this.sendAsync("getPageSource", {}, this.command_id);
    }
  },

  


  goBack: function MDA_goBack() {
    this.command_id = this.getCommandId();
    this.sendAsync("goBack", {}, this.command_id);
  },

  


  goForward: function MDA_goForward() {
    this.command_id = this.getCommandId();
    this.sendAsync("goForward", {}, this.command_id);
  },

  


  refresh: function MDA_refresh() {
    this.command_id = this.getCommandId();
    this.sendAsync("refresh", {}, this.command_id);
  },

  









  getWindowHandle: function MDA_getWindowHandle() {
    this.command_id = this.getCommandId();
    
    if (this.curBrowser.curFrameId && appName != 'B2G') {
      this.sendResponse(this.curBrowser.curFrameId, this.command_id);
      return;
    }
    for (let i in this.browsers) {
      if (this.curBrowser == this.browsers[i]) {
        this.sendResponse(i, this.command_id);
        return;
      }
    }
  },


  








  getWindowHandles: function MDA_getWindowHandles() {
    this.command_id = this.getCommandId();
    let res = [];
    let winEn = this.getWinEnumerator();
    while (winEn.hasMoreElements()) {
      let win = winEn.getNext();
      if (win.gBrowser && appName != 'B2G') {
        let tabbrowser = win.gBrowser;
        for (let i = 0; i < tabbrowser.browsers.length; ++i) {
          let contentWindow = tabbrowser.getBrowserAtIndex(i).contentWindowAsCPOW;
          if (contentWindow !== null) {
            let winId = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                     .getInterface(Ci.nsIDOMWindowUtils)
                                     .outerWindowID;
            winId += (appName == "B2G") ? "-b2g" : "";
            res.push(winId);
          }
        }
      } else {
        
        let winId = win.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils)
                       .outerWindowID;
        winId += (appName == "B2G") ? "-b2g" : "";
        res.push(winId);
      }
    }
    this.sendResponse(res, this.command_id);
  },

  









  getChromeWindowHandle: function MDA_getChromeWindowHandle() {
    this.command_id = this.getCommandId();
    for (let i in this.browsers) {
      if (this.curBrowser == this.browsers[i]) {
        this.sendResponse(i, this.command_id);
        return;
      }
    }
  },

  





  getChromeWindowHandles: function MDA_getChromeWindowHandles() {
    this.command_id = this.getCommandId();
    let res = [];
    let winEn = this.getWinEnumerator();
    while (winEn.hasMoreElements()) {
      let foundWin = winEn.getNext();
      let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils)
                          .outerWindowID;
      winId = winId + ((appName == "B2G") ? "-b2g" : "");
      res.push(winId);
    }
    this.sendResponse(res, this.command_id);
  },

  


  getWindowPosition: function MDA_getWindowPosition() {
    this.command_id = this.getCommandId();
    let curWindow = this.getCurrentWindow();
    this.sendResponse({ x: curWindow.screenX, y: curWindow.screenY}, this.command_id);
  },

  








  setWindowPosition: function MDA_setWindowPosition(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (appName !== "Firefox") {
      this.sendError("Unable to set the window position on mobile", 61, null,
                      command_id);

    }
    else {
      let x = parseInt(aRequest.parameters.x);;
      let y  = parseInt(aRequest.parameters.y);

      if (isNaN(x) || isNaN(y)) {
        this.sendError("x and y arguments should be integers", 13, null, command_id);
        return;
      }
      let curWindow = this.getCurrentWindow();
      curWindow.moveTo(x, y);
      this.sendOk(command_id);
    }
  },

  






  switchToWindow: function MDA_switchToWindow(aRequest) {
    let command_id = this.command_id = this.getCommandId();

    let checkWindow = function (win, outerId, contentWindowId, ind) {
      if (aRequest.parameters.name == win.name ||
          aRequest.parameters.name == contentWindowId ||
          aRequest.parameters.name == outerId) {
        
        this.sandbox = null;
        if (this.browsers[outerId] === undefined) {
          
          this.startBrowser(win, false);
        } else {
          utils.window = win;
          this.curBrowser = this.browsers[outerId];
          if (contentWindowId) {
            
            this.curBrowser.curFrameId = contentWindowId;
            win.gBrowser.selectTabAtIndex(ind);
          }
          this.sendOk(command_id);
        }
        return true;
      }
      return false;
    }

    let winEn = this.getWinEnumerator();
    while (winEn.hasMoreElements()) {
      let win = winEn.getNext();
      let outerId = win.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils)
                       .outerWindowID;
      outerId += (appName == "B2G") ? "-b2g" : "";
      if (win.gBrowser && appName != 'B2G') {
        let tabbrowser = win.gBrowser;
        for (let i = 0; i < tabbrowser.browsers.length; ++i) {
          let contentWindow = tabbrowser.getBrowserAtIndex(i).contentWindowAsCPOW;
          if (contentWindow) {
            let contentWindowId = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                               .getInterface(Ci.nsIDOMWindowUtils)
                                               .outerWindowID;
            contentWindowId += (appName == "B2G") ? "-b2g" : "";
            if (checkWindow.call(this, win, outerId, contentWindowId, i)) {
              return;
            }
          }
        }
      } else {
        
        
        if (checkWindow.call(this, win, outerId)) {
          return;
        }
      }
    }
    this.sendError("Unable to locate window " + aRequest.parameters.name, 23, null,
                   command_id);
  },

  getActiveFrame: function MDA_getActiveFrame() {
    this.command_id = this.getCommandId();

    if (this.context == "chrome") {
      if (this.curFrame) {
        let frameUid = this.curBrowser.elementManager.addToKnownElements(this.curFrame.frameElement);
        this.sendResponse(frameUid, this.command_id);
      } else {
        
        this.sendResponse(null, this.command_id);
      }
    } else {
      
      this.sendResponse(this.currentFrameElement, this.command_id);
    }
  },

  








  switchToFrame: function MDA_switchToFrame(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let curWindow = this.getCurrentWindow();
    let checkLoad = function() {
      let errorRegex = /about:.+(error)|(blocked)\?/;
      let curWindow = this.getCurrentWindow();
      if (curWindow.document.readyState == "complete") {
        this.sendOk(command_id);
        return;
      }
      else if (curWindow.document.readyState == "interactive" && errorRegex.exec(curWindow.document.baseURI)) {
        this.sendError("Error loading page", 13, null, command_id);
        return;
      }

      checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    if (this.context == "chrome") {
      let foundFrame = null;
      if ((aRequest.parameters.id == null) && (aRequest.parameters.element == null)) {
        this.curFrame = null;
        if (aRequest.parameters.focus) {
          this.mainFrame.focus();
        }
        checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
        return;
      }
      if (aRequest.parameters.element != undefined) {
        if (this.curBrowser.elementManager.seenItems[aRequest.parameters.element]) {
          let wantedFrame = this.curBrowser.elementManager.getKnownElement(aRequest.parameters.element, curWindow); 
          
          if (wantedFrame.tagName == "xul:browser" || wantedFrame.tagName == "browser") {
            curWindow = wantedFrame.contentWindow;
            this.curFrame = curWindow;
            if (aRequest.parameters.focus) {
              this.curFrame.focus();
            }
            checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
            return;
          }
          
          let frames = curWindow.document.getElementsByTagName("iframe");
          let numFrames = frames.length;
          for (let i = 0; i < numFrames; i++) {
            if (XPCNativeWrapper(frames[i]) == XPCNativeWrapper(wantedFrame)) {
              curWindow = frames[i].contentWindow;
              this.curFrame = curWindow;
              if (aRequest.parameters.focus) {
                this.curFrame.focus();
              }
              checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
              return;
          }
        }
      }
    }
    switch(typeof(aRequest.parameters.id)) {
      case "string" :
        let foundById = null;
        let frames = curWindow.document.getElementsByTagName("iframe");
        let numFrames = frames.length;
        for (let i = 0; i < numFrames; i++) {
          
          let frame = frames[i];
          if (frame.getAttribute("name") == aRequest.parameters.id) {
            foundFrame = i;
            curWindow = frame.contentWindow;
            break;
          } else if ((foundById == null) && (frame.id == aRequest.parameters.id)) {
            foundById = i;
          }
        }
        if ((foundFrame == null) && (foundById != null)) {
          foundFrame = foundById;
          curWindow = frames[foundById].contentWindow;
        }
        break;
      case "number":
        if (curWindow.frames[aRequest.parameters.id] != undefined) {
          foundFrame = aRequest.parameters.id;
          curWindow = curWindow.frames[foundFrame].frameElement.contentWindow;
        }
        break;
      }
      if (foundFrame != null) {
        this.curFrame = curWindow;
        if (aRequest.parameters.focus) {
          this.curFrame.focus();
        }
        checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
      } else {
        this.sendError("Unable to locate frame: " + aRequest.parameters.id, 8, null,
                       command_id);
      }
    }
    else {
      if ((!aRequest.parameters.id) && (!aRequest.parameters.element) &&
          (this.curBrowser.frameManager.currentRemoteFrame !== null)) {
        
        
        
        
        this.switchToGlobalMessageManager();
      }
      aRequest.command_id = command_id;
      this.sendAsync("switchToFrame", aRequest.parameters, command_id);
    }
  },

  





  setSearchTimeout: function MDA_setSearchTimeout(aRequest) {
    this.command_id = this.getCommandId();
    let timeout = parseInt(aRequest.parameters.ms);
    if (isNaN(timeout)) {
      this.sendError("Not a Number", 500, null, this.command_id);
    }
    else {
      this.searchTimeout = timeout;
      this.sendOk(this.command_id);
    }
  },

  






  timeouts: function MDA_timeouts(aRequest){
    
    this.command_id = this.getCommandId();
    let timeout_type = aRequest.parameters.type;
    let timeout = parseInt(aRequest.parameters.ms);
    if (isNaN(timeout)) {
      this.sendError("Not a Number", 500, null, this.command_id);
    }
    else {
      if (timeout_type == "implicit") {
        this.setSearchTimeout(aRequest);
      }
      else if (timeout_type == "script") {
        this.setScriptTimeout(aRequest);
      }
      else {
        this.pageTimeout = timeout;
        this.sendOk(this.command_id);
      }
    }
  },

  





  singleTap: function MDA_singleTap(aRequest) {
    this.command_id = this.getCommandId();
    let serId = aRequest.parameters.id;
    let x = aRequest.parameters.x;
    let y = aRequest.parameters.y;
    if (this.context == "chrome") {
      this.sendError("Command 'singleTap' is not available in chrome context", 500, null, this.command_id);
    }
    else {
      this.addFrameCloseListener("tap");
      this.sendAsync("singleTap",
                     {
                       id: serId,
                       corx: x,
                       cory: y
                     },
                     this.command_id);
    }
  },

  





  actionChain: function MDA_actionChain(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      this.sendError("Command 'actionChain' is not available in chrome context", 500, null, this.command_id);
    }
    else {
      this.addFrameCloseListener("action chain");
      this.sendAsync("actionChain",
                     {
                       chain: aRequest.parameters.chain,
                       nextId: aRequest.parameters.nextId
                     },
                     this.command_id);
    }
  },

  








  multiAction: function MDA_multiAction(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
       this.sendError("Command 'multiAction' is not available in chrome context", 500, null, this.command_id);
    }
    else {
      this.addFrameCloseListener("multi action chain");
      this.sendAsync("multiAction",
                     {
                       value: aRequest.parameters.value,
                       maxlen: aRequest.parameters.max_length
                     },
                     this.command_id);
   }
 },

  






  findElement: function MDA_findElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      let id;
      try {
        let on_success = this.sendResponse.bind(this);
        let on_error = this.sendError.bind(this);
        id = this.curBrowser.elementManager.find(
                              this.getCurrentWindow(),
                              aRequest.parameters,
                              this.searchTimeout,
                              on_success,
                              on_error,
                              false,
                              command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
        return;
      }
    }
    else {
      this.sendAsync("findElementContent",
                     {
                       value: aRequest.parameters.value,
                       using: aRequest.parameters.using,
                       element: aRequest.parameters.element,
                       searchTimeout: this.searchTimeout
                     },
                     command_id);
    }
  },

  







  findChildElement: function MDA_findChildElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    this.sendAsync("findElementContent",
                    {
                       value: aRequest.parameters.value,
                       using: aRequest.parameters.using,
                       element: aRequest.parameters.id,
                       searchTimeout: this.searchTimeout
                     },
                     command_id);
  },

  






  findElements: function MDA_findElements(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      let id;
      try {
        let on_success = this.sendResponse.bind(this);
        let on_error = this.sendError.bind(this);
        id = this.curBrowser.elementManager.find(this.getCurrentWindow(),
                                                 aRequest.parameters,
                                                 this.searchTimeout,
                                                 on_success,
                                                 on_error,
                                                 true,
                                                 command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
        return;
      }
    }
    else {
      this.sendAsync("findElementsContent",
                     {
                       value: aRequest.parameters.value,
                       using: aRequest.parameters.using,
                       element: aRequest.parameters.element,
                       searchTimeout: this.searchTimeout
                     },
                     command_id);
    }
  },

  







  findChildElements: function MDA_findChildElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    this.sendAsync("findElementsContent",
                    {
                       value: aRequest.parameters.value,
                       using: aRequest.parameters.using,
                       element: aRequest.parameters.id,
                       searchTimeout: this.searchTimeout
                     },
                     command_id);
  },

  


  getActiveElement: function MDA_getActiveElement(){
    let command_id = this.command_id = this.getCommandId();
    this.sendAsync("getActiveElement", {}, command_id);
  },

  






  clickElement: function MDA_clickElementent(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        el.click();
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      
      
      
      
      this.addFrameCloseListener("click");
      this.sendAsync("clickElement",
                     { id: aRequest.parameters.id },
                     command_id);
    }
  },

  







  getElementAttribute: function MDA_getElementAttribute(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        this.sendResponse(utils.getElementAttribute(el, aRequest.parameters.name),
                          command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementAttribute",
                     {
                       id: aRequest.parameters.id,
                       name: aRequest.parameters.name
                     },
                     command_id);
    }
  },

  






  getElementText: function MDA_getElementText(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        let lines = [];
        this.getVisibleText(el, lines);
        lines = lines.join("\n");
        this.sendResponse(lines, command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementText",
                     { id: aRequest.parameters.id },
                     command_id);
    }
  },

  






  getElementTagName: function MDA_getElementTagName(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        this.sendResponse(el.tagName.toLowerCase(), command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementTagName",
                     { id: aRequest.parameters.id },
                     command_id);
    }
  },

  






  isElementDisplayed: function MDA_isElementDisplayed(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        this.sendResponse(utils.isElementDisplayed(el), command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("isElementDisplayed",
                     { id:aRequest.parameters.id },
                     command_id);
    }
  },

  







  getElementValueOfCssProperty: function MDA_getElementValueOfCssProperty(aRequest){
    let command_id = this.command_id = this.getCommandId();
    this.sendAsync("getElementValueOfCssProperty",
                   {id: aRequest.parameters.id, propertyName: aRequest.parameters.propertyName},
                   command_id);
  },

  





  submitElement: function MDA_submitElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      this.sendError("Command 'submitElement' is not available in chrome context", 500, null, this.command_id);
    }
    else {
      this.sendAsync("submitElement", {id: aRequest.parameters.id}, command_id);
    }
  },

  






  isElementEnabled: function(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    let id = aRequest.parameters.id;
    if (this.context == "chrome") {
      try {
        
        let win = this.getCurrentWindow();
        let el = this.curBrowser.elementManager.getKnownElement(id, win);
        this.sendResponse(!!!el.disabled, command_id);
      } catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    } else {
      this.sendAsync("isElementEnabled", {id: id}, command_id);
    }
  },

  






  isElementSelected: function MDA_isElementSelected(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        if (el.checked != undefined) {
          this.sendResponse(!!el.checked, command_id);
        }
        else if (el.selected != undefined) {
          this.sendResponse(!!el.selected, command_id);
        }
        else {
          this.sendResponse(true, command_id);
        }
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("isElementSelected",
                     { id:aRequest.parameters.id },
                     command_id);
    }
  },

  getElementSize: function MDA_getElementSize(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        let clientRect = el.getBoundingClientRect();
        this.sendResponse({width: clientRect.width, height: clientRect.height},
                          command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementSize",
                     { id:aRequest.parameters.id },
                     command_id);
    }
  },

  getElementRect: function MDA_getElementRect(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        let clientRect = el.getBoundingClientRect();
        this.sendResponse({x: clientRect.x + this.getCurrentWindow().pageXOffset,
                           y: clientRect.y + this.getCurrentWindow().pageYOffset,
                           width: clientRect.width, height: clientRect.height},
                           command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementRect",
                     { id:aRequest.parameters.id },
                     command_id);
    }
  },

  







  sendKeysToElement: function MDA_sendKeysToElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      let currentWindow = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(
        aRequest.parameters.id, currentWindow);
      utils.sendKeysToElement(currentWindow, el, aRequest.parameters.value,
                              this.sendOk.bind(this), this.sendError.bind(this),
                              command_id, this.context);
    }
    else {
      this.sendAsync("sendKeysToElement",
                     {
                       id:aRequest.parameters.id,
                       value: aRequest.parameters.value
                     },
                     command_id);
    }
  },

  




  setTestName: function MDA_setTestName(aRequest) {
    this.command_id = this.getCommandId();
    this.testName = aRequest.parameters.value;
    this.sendAsync("setTestName",
                   { value: aRequest.parameters.value },
                   this.command_id);
  },

  






  clearElement: function MDA_clearElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.parameters.id, this.getCurrentWindow());
        if (el.nodeName == "textbox") {
          el.value = "";
        }
        else if (el.nodeName == "checkbox") {
          el.checked = false;
        }
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("clearElement",
                     { id:aRequest.parameters.id },
                     command_id);
    }
  },

  








  getElementLocation: function MDA_getElementLocation(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("getElementLocation", {id: aRequest.parameters.id},
                   this.command_id);
  },

  


  addCookie: function MDA_addCookie(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("addCookie",
                   { cookie:aRequest.parameters.cookie },
                   this.command_id);
  },

  





  getCookies: function MDA_getCookies() {
    this.command_id = this.getCommandId();
    this.sendAsync("getCookies", {}, this.command_id);
  },

  


  deleteAllCookies: function MDA_deleteAllCookies() {
    this.command_id = this.getCommandId();
    this.sendAsync("deleteAllCookies", {}, this.command_id);
  },

  


  deleteCookie: function MDA_deleteCookie(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("deleteCookie",
                   { name:aRequest.parameters.name },
                   this.command_id);
  },

  





  close: function MDA_close() {
    let command_id = this.command_id = this.getCommandId();
    if (appName == "B2G") {
      
      this.sendOk(command_id);
    }
    else {
      
      let numOpenWindows = 0;
      let winEnum = this.getWinEnumerator();
      while (winEnum.hasMoreElements()) {
        let win = winEnum.getNext();
        
        if (win.gBrowser) {
          numOpenWindows += win.gBrowser.browsers.length;
        } else {
          numOpenWindows += 1;
        }
      }

      
      if (numOpenWindows === 1) {
        try {
          this.sessionTearDown();
        }
        catch (e) {
          this.sendError("Could not clear session", 500,
                         e.name + ": " + e.message, command_id);
          return;
        }
        this.sendOk(command_id);
        return;
      }

      try {
        if (this.messageManager != this.globalMessageManager) {
          this.messageManager.removeDelayedFrameScript(FRAME_SCRIPT);
        }
        if (this.curBrowser.tab) {
          this.curBrowser.closeTab();
        } else {
          this.getCurrentWindow().close();
        }
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError("Could not close window: " + e.message, 13, e.stack,
                       command_id);
      }
    }
  },

  





  closeChromeWindow: function MDA_closeChromeWindow() {
    let command_id = this.command_id = this.getCommandId();
    if (appName == "B2G") {
      
      this.sendOk(command_id);
    }
    else {
      
      let numOpenWindows = 0;
      let winEnum = this.getWinEnumerator();
      while (winEnum.hasMoreElements()) {
        numOpenWindows += 1;
        winEnum.getNext();
      }

      
      if (numOpenWindows === 1) {
        try {
          this.sessionTearDown();
        }
        catch (e) {
          this.sendError("Could not clear session", 500,
                         e.name + ": " + e.message, command_id);
          return;
        }
        this.sendOk(command_id);
        return;
      }

      try {
        this.messageManager.removeDelayedFrameScript(FRAME_SCRIPT);
        this.getCurrentWindow().close();
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError("Could not close window: " + e.message, 13, e.stack,
                       command_id);
      }
    }
  },

  








  sessionTearDown: function MDA_sessionTearDown() {
    if (this.curBrowser != null) {
      if (appName == "B2G") {
        this.globalMessageManager.broadcastAsyncMessage(
            "Marionette:sleepSession" + this.curBrowser.mainContentId, {});
        this.curBrowser.knownFrames.splice(
            this.curBrowser.knownFrames.indexOf(this.curBrowser.mainContentId), 1);
      }
      else {
        
        Services.prefs.setBoolPref("marionette.contentListener", false);
      }
      
      for (let win in this.browsers) {
        let browser = this.browsers[win];
        for (let i in browser.knownFrames) {
          this.globalMessageManager.broadcastAsyncMessage("Marionette:deleteSession" + browser.knownFrames[i], {});
        }
      }
      let winEnum = this.getWinEnumerator();
      while (winEnum.hasMoreElements()) {
        winEnum.getNext().messageManager.removeDelayedFrameScript(FRAME_SCRIPT);
      }
      this.curBrowser.frameManager.removeSpecialPowers();
      this.curBrowser.frameManager.removeMessageManagerListeners(this.globalMessageManager);
    }
    this.switchToGlobalMessageManager();
    
    this.curFrame = null;
    if (this.mainFrame) {
      this.mainFrame.focus();
    }
    this.sessionId = null;
    this.deleteFile('marionetteChromeScripts');
    this.deleteFile('marionetteContentScripts');

    if (this.observing !== null) {
      for (let topic in this.observing) {
        Services.obs.removeObserver(this.observing[topic], topic);
      }
      this.observing = null;
    }
  },

  



  deleteSession: function MDA_deleteSession() {
    let command_id = this.command_id = this.getCommandId();
    try {
      this.sessionTearDown();
    }
    catch (e) {
      this.sendError("Could not delete session", 500, e.name + ": " + e.message, command_id);
      return;
    }
    this.sendOk(command_id);
  },

  


  getAppCacheStatus: function MDA_getAppCacheStatus(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("getAppCacheStatus", {}, this.command_id);
  },

  _emu_cb_id: 0,
  _emu_cbs: null,
  runEmulatorCmd: function runEmulatorCmd(cmd, callback) {
    if (callback) {
      if (!this._emu_cbs) {
        this._emu_cbs = {};
      }
      this._emu_cbs[this._emu_cb_id] = callback;
    }
    this.sendToClient({emulator_cmd: cmd, id: this._emu_cb_id}, -1);
    this._emu_cb_id += 1;
  },

  runEmulatorShell: function runEmulatorShell(args, callback) {
    if (callback) {
      if (!this._emu_cbs) {
        this._emu_cbs = {};
      }
      this._emu_cbs[this._emu_cb_id] = callback;
    }
    this.sendToClient({emulator_shell: args, id: this._emu_cb_id}, -1);
    this._emu_cb_id += 1;
  },

  emulatorCmdResult: function emulatorCmdResult(message) {
    if (this.context != "chrome") {
      this.sendAsync("emulatorCmdResult", message, -1);
      return;
    }

    if (!this._emu_cbs) {
      return;
    }

    let cb = this._emu_cbs[message.id];
    delete this._emu_cbs[message.id];
    if (!cb) {
      return;
    }
    try {
      cb(message.result);
    }
    catch(e) {
      this.sendError(e.message, e.code, e.stack, -1);
      return;
    }
  },

  importScript: function MDA_importScript(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    let converter =
      Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].
          createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let result = {};
    let data = converter.convertToByteArray(aRequest.parameters.script, result);
    let ch = Components.classes["@mozilla.org/security/hash;1"]
                       .createInstance(Components.interfaces.nsICryptoHash);
    ch.init(ch.MD5);
    ch.update(data, data.length);
    let hash = ch.finish(true);
    if (this.importedScriptHashes[this.context].indexOf(hash) > -1) {
        
        this.sendOk(command_id);
        return;
    }
    this.importedScriptHashes[this.context].push(hash);
    if (this.context == "chrome") {
      let file;
      if (this.importedScripts.exists()) {
        file = FileUtils.openFileOutputStream(this.importedScripts,
            FileUtils.MODE_APPEND | FileUtils.MODE_WRONLY);
      }
      else {
        
        this.importedScripts.createUnique(
            Components.interfaces.nsIFile.NORMAL_FILE_TYPE, parseInt("0666", 8));
        file = FileUtils.openFileOutputStream(this.importedScripts,
            FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
        this.importedScripts.permissions = parseInt("0666", 8); 
      }
      file.write(aRequest.parameters.script, aRequest.parameters.script.length);
      file.close();
      this.sendOk(command_id);
    }
    else {
      this.sendAsync("importScript",
                     { script: aRequest.parameters.script },
                     command_id);
    }
  },

  clearImportedScripts: function MDA_clearImportedScripts(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    try {
      if (this.context == "chrome") {
        this.deleteFile('marionetteChromeScripts');
      }
      else {
        this.deleteFile('marionetteContentScripts');
      }
    }
    catch (e) {
      this.sendError("Could not clear imported scripts", 500, e.name + ": " + e.message, command_id);
      return;
    }
    this.sendOk(command_id);
  },

  

















  takeScreenshot: function MDA_takeScreenshot(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      var win = this.getCurrentWindow();
      var canvas = win.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
      var doc;
      if (appName == "B2G") {
        doc = win.document.body;
      } else {
        doc = win.document.getElementsByTagName('window')[0];
      }
      var docRect = doc.getBoundingClientRect();
      var width = docRect.width;
      var height = docRect.height;

      
      
      var scale = win.devicePixelRatio;
      canvas.setAttribute("width", Math.round(width * scale));
      canvas.setAttribute("height", Math.round(height * scale));

      var context = canvas.getContext("2d");
      var flags;
      if (appName == "B2G") {
        flags =
          context.DRAWWINDOW_DRAW_CARET |
          context.DRAWWINDOW_DRAW_VIEW |
          context.DRAWWINDOW_USE_WIDGET_LAYERS;
      } else {
        
        
        flags =
          context.DRAWWINDOW_DRAW_VIEW |
          context.DRAWWINDOW_USE_WIDGET_LAYERS;
      }
      context.scale(scale, scale);
      context.drawWindow(win, 0, 0, width, height, "rgb(255,255,255)", flags);
      var dataUrl = canvas.toDataURL("image/png", "");
      var data = dataUrl.substring(dataUrl.indexOf(",") + 1);
      this.sendResponse(data, this.command_id);
    }
    else {
      this.sendAsync("takeScreenshot",
                   {id: aRequest.parameters.id,
                    highlights: aRequest.parameters.highlights,
                    full: aRequest.parameters.full},
                   this.command_id);
    }
  },

  






  getScreenOrientation: function MDA_getScreenOrientation(aRequest) {
    this.command_id = this.getCommandId();
    let curWindow = this.getCurrentWindow();
    let or = curWindow.screen.mozOrientation;
    this.sendResponse(or, this.command_id);
  },

  










  setScreenOrientation: function MDA_setScreenOrientation(aRequest) {
    const ors = ["portrait", "landscape",
                 "portrait-primary", "landscape-primary",
                 "portrait-secondary", "landscape-secondary"];

    this.command_id = this.getCommandId();
    let or = String(aRequest.parameters.orientation);

    let mozOr = or.toLowerCase();
    if (ors.indexOf(mozOr) < 0) {
      this.sendError("Unknown screen orientation: " + or, 500, null,
                     this.command_id);
      return;
    }

    let curWindow = this.getCurrentWindow();
    if (!curWindow.screen.mozLockOrientation(mozOr)) {
      this.sendError("Unable to set screen orientation: " + or, 500,
                     null, this.command_id);
    }
    this.sendOk(this.command_id);
  },

  







  getWindowSize: function MDA_getWindowSize(aRequest) {
    this.command_id = this.getCommandId();
    let curWindow = this.getCurrentWindow();
    let curWidth = curWindow.outerWidth;
    let curHeight = curWindow.outerHeight;
    this.sendResponse({width: curWidth, height: curHeight}, this.command_id);
  },

  









  setWindowSize: function MDA_setWindowSize(aRequest) {
    this.command_id = this.getCommandId();

    if (appName !== "Firefox") {
      this.sendError("Not supported on mobile", 405, null, this.command_id);
      return;
    }

    try {
      var width = parseInt(aRequest.parameters.width);
      var height = parseInt(aRequest.parameters.height);
    }
    catch(e) {
      this.sendError(e.message, e.code, e.stack, this.command_id);
      return;
    }

    let curWindow = this.getCurrentWindow();
    if (width >= curWindow.screen.availWidth && height >= curWindow.screen.availHeight) {
      this.sendError("Invalid requested size, cannot maximize", 405, null, this.command_id);
      return;
    }

    curWindow.resizeTo(width, height);
    this.sendOk(this.command_id);
  },

  




  maximizeWindow: function MDA_maximizeWindow (aRequest) {
    this.command_id = this.getCommandId();

    if (appName !== "Firefox") {
      this.sendError("Not supported for mobile", 405, null, this.command_id);
      return;
    }

    let curWindow = this.getCurrentWindow();
    curWindow.moveTo(0,0);
    curWindow.resizeTo(curWindow.screen.availWidth, curWindow.screen.availHeight);
    this.sendOk(this.command_id);
  },

  



  get activeDialogWindow () {
    if (this._dialogWindowRef !== null) {
      let dialogWin = this._dialogWindowRef.get();
      if (dialogWin && dialogWin.parent) {
        return dialogWin;
      }
    }
    return null;
  },

  get activeDialogUI () {
    let dialogWin = this.activeDialogWindow;
    if (dialogWin) {
      return dialogWin.Dialog.ui;
    }
    return this.curBrowser.getTabModalUI();
  },

  



  dismissDialog: function MDA_dismissDialog() {
    this.command_id = this.getCommandId();
    if (this.activeDialogUI === null) {
      this.sendError("No tab modal was open when attempting to dismiss the dialog",
                     27, null, this.command_id);
      return;
    }

    let {button0, button1} = this.activeDialogUI;
    (button1 ? button1 : button0).click();
    this.sendOk(this.command_id);
  },

  



  acceptDialog: function MDA_acceptDialog() {
    this.command_id = this.getCommandId();
    if (this.activeDialogUI === null) {
      this.sendError("No tab modal was open when attempting to accept the dialog",
                     27, null, this.command_id);
      return;
    }

    let {button0} = this.activeDialogUI;
    button0.click();
    this.sendOk(this.command_id);
  },

  



  getTextFromDialog: function MDA_getTextFromDialog() {
    this.command_id = this.getCommandId();
    if (this.activeDialogUI === null) {
      this.sendError("No tab modal was open when attempting to get the dialog text",
                     27, null, this.command_id);
      return;
    }

    let {infoBody} = this.activeDialogUI;
    this.sendResponse(infoBody.textContent, this.command_id);
  },

  




  sendKeysToDialog: function MDA_sendKeysToDialog(aRequest) {
    this.command_id = this.getCommandId();
    if (this.activeDialogUI === null) {
      this.sendError("No tab modal was open when attempting to send keys to a dialog",
                     27, null, this.command_id);
      return;
    }

    
    let {loginContainer, loginTextbox} = this.activeDialogUI;
    if (loginContainer.hidden) {
      this.sendError("This prompt does not accept text input",
                     11, null, this.command_id);
    }

    let win = this.activeDialogWindow ? this.activeDialogWindow : this.getCurrentWindow();
    utils.sendKeysToElement(win, loginTextbox, aRequest.parameters.value,
                            this.sendOk.bind(this), this.sendError.bind(this),
                            this.command_id, "chrome");
  },

  



  generateFrameId: function MDA_generateFrameId(id) {
    let uid = id + (appName == "B2G" ? "-b2g" : "");
    return uid;
  },

  





  handleDialogLoad: function MDA_handleModalLoad(subject, topic) {
    
    
    let clickToStart;
    try {
      clickToStart = Services.prefs.getBoolPref('marionette.debugging.clicktostart');
    } catch (e) { }
    if (clickToStart) {
      Services.prefs.setBoolPref('marionette.debugging.clicktostart', false);
      return;
    }

    if (topic == "common-dialog-loaded") {
      this._dialogWindowRef = Cu.getWeakReference(subject);
    }

    if (this.command_id) {
      
      
      
      this.sendToClient({from:this.actorID, ok: true, value: null}, this.command_id);
    }
  },

  


  receiveMessage: function MDA_receiveMessage(message) {
    
    if (this.mozBrowserClose !== null){
      let curWindow = this.getCurrentWindow();
      curWindow.removeEventListener('mozbrowserclose', this.mozBrowserClose, true);
      this.mozBrowserClose = null;
    }

    switch (message.name) {
      case "Marionette:done":
        this.sendResponse(message.json.value, message.json.command_id);
        break;
      case "Marionette:ok":
        this.sendOk(message.json.command_id);
        break;
      case "Marionette:error":
        this.sendError(message.json.message, message.json.status, message.json.stacktrace, message.json.command_id);
        break;
      case "Marionette:log":
        
        logger.info(message.json.message);
        break;
      case "Marionette:shareData":
        
        if (message.json.log) {
          this.marionetteLog.addLogs(message.json.log);
        }
        break;
      case "Marionette:runEmulatorCmd":
      case "Marionette:runEmulatorShell":
        this.sendToClient(message.json, -1);
        break;
      case "Marionette:switchToFrame":
        this.oopFrameId = this.curBrowser.frameManager.switchToFrame(message);
        this.messageManager = this.curBrowser.frameManager.currentRemoteFrame.messageManager.get();
        break;
      case "Marionette:switchToModalOrigin":
        this.curBrowser.frameManager.switchToModalOrigin(message);
        this.messageManager = this.curBrowser.frameManager.currentRemoteFrame.messageManager.get();
        break;
      case "Marionette:switchedToFrame":
        logger.info("Switched to frame: " + JSON.stringify(message.json));
        if (message.json.restorePrevious) {
          this.currentFrameElement = this.previousFrameElement;
        }
        else {
          if (message.json.storePrevious) {
            
            
            
            this.previousFrameElement = this.currentFrameElement;
          }
          this.currentFrameElement = message.json.frameValue;
        }
        break;
      case "Marionette:getVisibleCookies":
        let [currentPath, host] = message.json.value;
        let isForCurrentPath = function(aPath) {
          return currentPath.indexOf(aPath) != -1;
        }
        let results = [];
        let enumerator = cookieManager.enumerator;
        while (enumerator.hasMoreElements()) {
          let cookie = enumerator.getNext().QueryInterface(Ci['nsICookie']);
          
          let hostname = host;
          do {
            if ((cookie.host == '.' + hostname || cookie.host == hostname)
                && isForCurrentPath(cookie.path)) {
              results.push({
                'name': cookie.name,
                'value': cookie.value,
                'path': cookie.path,
                'host': cookie.host,
                'secure': cookie.isSecure,
                'expiry': cookie.expires
              });
              break;
            }
            hostname = hostname.replace(/^.*?\./, '');
          } while (hostname.indexOf('.') != -1);
        }
        return results;
      case "Marionette:addCookie":
        let cookieToAdd = message.json.value;
        Services.cookies.add(cookieToAdd.domain, cookieToAdd.path, cookieToAdd.name,
                             cookieToAdd.value, cookieToAdd.secure, false, false,
                             cookieToAdd.expiry);
        return true;
      case "Marionette:deleteCookie":
        let cookieToDelete = message.json.value;
        cookieManager.remove(cookieToDelete.host, cookieToDelete.name,
                             cookieToDelete.path, false);
        return true;
      case "Marionette:register":
        
        
        let nullPrevious = (this.curBrowser.curFrameId == null);
        let listenerWindow = null;
        try {
          listenerWindow = Services.wm.getOuterWindowWithId(message.json.value);
        } catch (ex) { }

        
        if (this.curBrowser.frameManager.currentRemoteFrame !== null &&
            (!listenerWindow ||
             this.messageManager == this.curBrowser.frameManager.currentRemoteFrame.messageManager.get())) {
          
          
          
          
          
          
          
          
          
          
          this.curBrowser.frameManager.currentRemoteFrame.targetFrameId = this.generateFrameId(message.json.value);
          this.sendOk(this.command_id);
        }

        let browserType;
        try {
          browserType = message.target.getAttribute("type");
        } catch (ex) {
          
        }
        let reg = {};
        
        let mainContent = (this.curBrowser.mainContentId == null);
        if (!browserType || browserType != "content") {
          
          let listenerId = message.json.value;
          reg.id = this.curBrowser.register(this.generateFrameId(listenerId), listenerId);
        }
        
        mainContent = ((mainContent == true) && (this.curBrowser.mainContentId != null));
        if (mainContent) {
          this.mainContentFrameId = this.curBrowser.curFrameId;
        }
        this.curBrowser.elementManager.seenItems[reg.id] = Cu.getWeakReference(listenerWindow);
        if (nullPrevious && (this.curBrowser.curFrameId != null)) {
          if (!this.sendAsync("newSession",
              { B2G: (appName == "B2G"),
                raisesAccessibilityExceptions:
                  this.sessionCapabilities.raisesAccessibilityExceptions },
              this.newSessionCommandId)) {
            return;
          }
          if (this.curBrowser.newSession) {
            this.getSessionCapabilities();
            this.newSessionCommandId = null;
          }
        }
        if (this.curBrowser.frameRegsPending) {
          if (this.curBrowser.frameRegsPending > 0) {
            this.curBrowser.frameRegsPending -= 1;
          }
          if (this.curBrowser.frameRegsPending === 0) {
            
            
            this.sendOk(this.command_id);
          }
        }
        return [reg, mainContent];
      case "Marionette:emitTouchEvent":
        let globalMessageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                             .getService(Ci.nsIMessageBroadcaster);
        globalMessageManager.broadcastAsyncMessage(
          "MarionetteMainListener:emitTouchEvent", message.json);
        return;
    }
  }
};

MarionetteServerConnection.prototype.requestTypes = {
  "getMarionetteID": MarionetteServerConnection.prototype.getMarionetteID,
  "sayHello": MarionetteServerConnection.prototype.sayHello,
  "newSession": MarionetteServerConnection.prototype.newSession,
  "getSessionCapabilities": MarionetteServerConnection.prototype.getSessionCapabilities,
  "log": MarionetteServerConnection.prototype.log,
  "getLogs": MarionetteServerConnection.prototype.getLogs,
  "setContext": MarionetteServerConnection.prototype.setContext,
  "getContext": MarionetteServerConnection.prototype.getContext,
  "executeScript": MarionetteServerConnection.prototype.execute,
  "setScriptTimeout": MarionetteServerConnection.prototype.setScriptTimeout,
  "timeouts": MarionetteServerConnection.prototype.timeouts,
  "singleTap": MarionetteServerConnection.prototype.singleTap,
  "actionChain": MarionetteServerConnection.prototype.actionChain,
  "multiAction": MarionetteServerConnection.prototype.multiAction,
  "executeAsyncScript": MarionetteServerConnection.prototype.executeWithCallback,
  "executeJSScript": MarionetteServerConnection.prototype.executeJSScript,
  "setSearchTimeout": MarionetteServerConnection.prototype.setSearchTimeout,
  "findElement": MarionetteServerConnection.prototype.findElement,
  "findChildElement": MarionetteServerConnection.prototype.findChildElements, 
  "findElements": MarionetteServerConnection.prototype.findElements,
  "findChildElements":MarionetteServerConnection.prototype.findChildElements, 
  "clickElement": MarionetteServerConnection.prototype.clickElement,
  "getElementAttribute": MarionetteServerConnection.prototype.getElementAttribute,
  "getElementText": MarionetteServerConnection.prototype.getElementText,
  "getElementTagName": MarionetteServerConnection.prototype.getElementTagName,
  "isElementDisplayed": MarionetteServerConnection.prototype.isElementDisplayed,
  "getElementValueOfCssProperty": MarionetteServerConnection.prototype.getElementValueOfCssProperty,
  "submitElement": MarionetteServerConnection.prototype.submitElement,
  "getElementSize": MarionetteServerConnection.prototype.getElementSize,  
  "getElementRect": MarionetteServerConnection.prototype.getElementRect,
  "isElementEnabled": MarionetteServerConnection.prototype.isElementEnabled,
  "isElementSelected": MarionetteServerConnection.prototype.isElementSelected,
  "sendKeysToElement": MarionetteServerConnection.prototype.sendKeysToElement,
  "getElementLocation": MarionetteServerConnection.prototype.getElementLocation,  
  "getElementPosition": MarionetteServerConnection.prototype.getElementLocation,  
  "clearElement": MarionetteServerConnection.prototype.clearElement,
  "getTitle": MarionetteServerConnection.prototype.getTitle,
  "getWindowType": MarionetteServerConnection.prototype.getWindowType,
  "getPageSource": MarionetteServerConnection.prototype.getPageSource,
  "get": MarionetteServerConnection.prototype.get,
  "goUrl": MarionetteServerConnection.prototype.get,  
  "getCurrentUrl": MarionetteServerConnection.prototype.getCurrentUrl,
  "getUrl": MarionetteServerConnection.prototype.getCurrentUrl,  
  "goBack": MarionetteServerConnection.prototype.goBack,
  "goForward": MarionetteServerConnection.prototype.goForward,
  "refresh":  MarionetteServerConnection.prototype.refresh,
  "getWindowHandle": MarionetteServerConnection.prototype.getWindowHandle,
  "getCurrentWindowHandle":  MarionetteServerConnection.prototype.getWindowHandle,  
  "getChromeWindowHandle": MarionetteServerConnection.prototype.getChromeWindowHandle,
  "getCurrentChromeWindowHandle": MarionetteServerConnection.prototype.getChromeWindowHandle,
  "getWindow":  MarionetteServerConnection.prototype.getWindowHandle,  
  "getWindowHandles": MarionetteServerConnection.prototype.getWindowHandles,
  "getChromeWindowHandles": MarionetteServerConnection.prototype.getChromeWindowHandles,
  "getCurrentWindowHandles": MarionetteServerConnection.prototype.getWindowHandles,  
  "getWindows":  MarionetteServerConnection.prototype.getWindowHandles,  
  "getWindowPosition": MarionetteServerConnection.prototype.getWindowPosition,
  "setWindowPosition": MarionetteServerConnection.prototype.setWindowPosition,
  "getActiveFrame": MarionetteServerConnection.prototype.getActiveFrame,
  "switchToFrame": MarionetteServerConnection.prototype.switchToFrame,
  "switchToWindow": MarionetteServerConnection.prototype.switchToWindow,
  "deleteSession": MarionetteServerConnection.prototype.deleteSession,
  "emulatorCmdResult": MarionetteServerConnection.prototype.emulatorCmdResult,
  "importScript": MarionetteServerConnection.prototype.importScript,
  "clearImportedScripts": MarionetteServerConnection.prototype.clearImportedScripts,
  "getAppCacheStatus": MarionetteServerConnection.prototype.getAppCacheStatus,
  "close": MarionetteServerConnection.prototype.close,
  "closeWindow": MarionetteServerConnection.prototype.close,  
  "closeChromeWindow": MarionetteServerConnection.prototype.closeChromeWindow,
  "setTestName": MarionetteServerConnection.prototype.setTestName,
  "takeScreenshot": MarionetteServerConnection.prototype.takeScreenshot,
  "screenShot": MarionetteServerConnection.prototype.takeScreenshot,  
  "screenshot": MarionetteServerConnection.prototype.takeScreenshot,  
  "addCookie": MarionetteServerConnection.prototype.addCookie,
  "getCookies": MarionetteServerConnection.prototype.getCookies,
  "getAllCookies": MarionetteServerConnection.prototype.getCookies,  
  "deleteAllCookies": MarionetteServerConnection.prototype.deleteAllCookies,
  "deleteCookie": MarionetteServerConnection.prototype.deleteCookie,
  "getActiveElement": MarionetteServerConnection.prototype.getActiveElement,
  "getScreenOrientation": MarionetteServerConnection.prototype.getScreenOrientation,
  "setScreenOrientation": MarionetteServerConnection.prototype.setScreenOrientation,
  "getWindowSize": MarionetteServerConnection.prototype.getWindowSize,
  "setWindowSize": MarionetteServerConnection.prototype.setWindowSize,
  "maximizeWindow": MarionetteServerConnection.prototype.maximizeWindow,
  "dismissDialog": MarionetteServerConnection.prototype.dismissDialog,
  "acceptDialog": MarionetteServerConnection.prototype.acceptDialog,
  "getTextFromDialog": MarionetteServerConnection.prototype.getTextFromDialog,
  "sendKeysToDialog": MarionetteServerConnection.prototype.sendKeysToDialog
};









function BrowserObj(win, server) {
  this.DESKTOP = "desktop";
  this.B2G = "B2G";
  this.browser;
  this.window = win;
  this.knownFrames = [];
  this.curFrameId = null;
  this.startPage = "about:blank";
  this.mainContentId = null; 
  this.newSession = true; 
  this.elementManager = new ElementManager([NAME, LINK_TEXT, PARTIAL_LINK_TEXT]);
  this.setBrowser(win);
  this.frameManager = new FrameManager(server); 

  
  this.frameManager.addMessageManagerListeners(server.messageManager);
}

BrowserObj.prototype = {
  get tab () {
    
    return this.browser ? this.browser.selectedTab : null;
  },

  



  getTabModalUI: function MDA__getTabModaUI () {
    let browserForTab = this.browser.getBrowserForTab(this.tab);
    if (!browserForTab.hasAttribute('tabmodalPromptShowing')) {
      return null;
    }
    
    
    let modals = browserForTab.parentNode
                              .getElementsByTagNameNS(XUL_NS, 'tabmodalprompt');
    return modals[0].ui;
  },

  





  setBrowser: function BO_setBrowser(win) {
    switch (appName) {
      case "Firefox":
        if (!isMulet()) {
          this.browser = win.gBrowser;
        } else {
          
          appName = "B2G";
        }
        break;
      case "Fennec":
        this.browser = win.BrowserApp;
        break;
    }
  },
  


  startSession: function BO_startSession(newSession, win, callback) {
    if (appName == "Firefox" &&
        win.gMultiProcessBrowser &&
        !win.gBrowser.selectedBrowser.isRemoteBrowser) {
      win.XULBrowserWindow.forceInitialBrowserRemote();
    }
    callback(win, newSession);
  },

  


  closeTab: function BO_closeTab() {
    if (this.browser &&
        this.browser.removeTab &&
        this.tab != null && (appName != "B2G")) {
      this.browser.removeTab(this.tab);
    }
  },

  





  addTab: function BO_addTab(uri) {
    return this.browser.addTab(uri, true);
  },

  









  register: function BO_register(uid, id) {
    if (this.curFrameId == null) {
      let currWinId = null;
      if (this.browser) {
        
        
        
        let winAsCPOW = this.browser.getBrowserForTab(this.tab).contentWindowAsCPOW;
        currWinId = winAsCPOW.QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsIDOMWindowUtils)
                             .outerWindowID;
      }
      if ((!this.newSession) ||
          (this.newSession &&
            ((appName != "Firefox") ||
             id === currWinId))) {
        this.curFrameId = uid;
        this.mainContentId = uid;
      }
    }
    this.knownFrames.push(uid); 
    return uid;
  },
}





this.MarionetteServer = function MarionetteServer(port, forceLocal) {
  let flags = Ci.nsIServerSocket.KeepWhenOffline;
  if (forceLocal) {
    flags |= Ci.nsIServerSocket.LoopbackOnly;
  }
  let socket = new ServerSocket(port, flags, 0);
  logger.info("Listening on port " + socket.port + "\n");
  socket.asyncListen(this);
  this.listener = socket;
  this.nextConnID = 0;
  this.connections = {};
};

MarionetteServer.prototype = {
  onSocketAccepted: function(serverSocket, clientSocket)
  {
    logger.debug("accepted connection on " + clientSocket.host + ":" + clientSocket.port);

    let input = clientSocket.openInputStream(0, 0, 0);
    let output = clientSocket.openOutputStream(0, 0, 0);
    let aTransport = new DebuggerTransport(input, output);
    let connID = "conn" + this.nextConnID++ + '.';
    let conn = new MarionetteServerConnection(connID, aTransport, this);
    this.connections[connID] = conn;

    
    conn.sayHello();
    aTransport.ready();
  },

  closeListener: function() {
    this.listener.close();
    this.listener = null;
  },

  _connectionClosed: function DS_connectionClosed(aConnection) {
    delete this.connections[aConnection.prefix];
  }
};
