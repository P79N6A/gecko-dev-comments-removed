



"use strict";





const FRAME_SCRIPT = "chrome://marionette/content/marionette-listener.js";

let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-log-obj.js");
loader.loadSubScript("chrome://marionette/content/marionette-perf.js");
Cu.import("chrome://marionette/content/marionette-elements.js");
let utils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);

let specialpowers = {};
loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js",
                     specialpowers);
specialpowers.specialPowersObserver = new specialpowers.SpecialPowersObserver();
specialpowers.specialPowersObserver.init();

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");  

Services.prefs.setBoolPref("marionette.contentListener", false);
let appName = Services.appinfo.name;


Cu.import("resource://gre/modules/services-common/log4moz.js");
let logger = Log4Moz.repository.getLogger("Marionette");
logger.info('marionette-actors.js loaded');





let systemMessageListenerReady = false;
Services.obs.addObserver(function() {
  systemMessageListenerReady = true;
}, "system-message-listener-ready", false);





function createRootActor(aConnection)
{
  return new MarionetteRootActor(aConnection);
}






function MarionetteRootActor(aConnection)
{
  this.conn = aConnection;
  this._marionetteActor = new MarionetteDriverActor(this.conn);
  this._marionetteActorPool = null; 

  this._marionetteActorPool = new ActorPool(this.conn);
  this._marionetteActorPool.addActor(this._marionetteActor);
  this.conn.addActorPool(this._marionetteActorPool);
}

MarionetteRootActor.prototype = {
  





  sayHello: function MRA_sayHello() {
    return { from: "root",
             applicationType: "gecko",
             traits: [] };
  },

  


  disconnect: function MRA_disconnect() {
    this._marionetteActor.deleteSession();
  },

  






  getMarionetteID: function MRA_getMarionette() {
    return { "from": "root",
             "id": this._marionetteActor.actorID } ;
  }
};


MarionetteRootActor.prototype.requestTypes = {
  "getMarionetteID": MarionetteRootActor.prototype.getMarionetteID,
  "sayHello": MarionetteRootActor.prototype.sayHello
};





function MarionetteRemoteFrame(windowId, frameId) {
  this.windowId = windowId;
  this.frameId = frameId;
  this.targetFrameId = null;
  this.messageManager = null;
  this.command_id = null;
}

let remoteFrames = [];






function MarionetteDriverActor(aConnection)
{
  this.uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                   .getService(Ci.nsIUUIDGenerator);

  this.conn = aConnection;
  this.globalMessageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                             .getService(Ci.nsIMessageBroadcaster);
  this.messageManager = this.globalMessageManager;
  this.browsers = {}; 
  this.curBrowser = null; 
  this.context = "content";
  this.scriptTimeout = null;
  this.pageTimeout = null;
  this.timer = null;
  this.marionetteLog = new MarionetteLogObj();
  this.marionettePerf = new MarionettePerfData();
  this.command_id = null;
  this.mainFrame = null; 
  this.curFrame = null; 
  this.importedScripts = FileUtils.getFile('TmpD', ['marionettescriptchrome']);
  this.currentRemoteFrame = null; 
  this.testName = null;
  this.mozBrowserClose = null;

  
  this.addMessageManagerListeners(this.messageManager);
}

MarionetteDriverActor.prototype = {

  
  actorPrefix: "marionette",

  



  






  switchToGlobalMessageManager: function MDA_switchToGlobalMM() {
    if (this.currentRemoteFrame !== null) {
      this.removeMessageManagerListeners(this.messageManager);
      try {
        
        this.sendAsync("sleepSession");
      }
      catch(e) {}
    }
    this.messageManager = this.globalMessageManager;
    this.currentRemoteFrame = null;
  },

  







  sendAsync: function MDA_sendAsync(name, values) {
    if (this.currentRemoteFrame !== null) {
      this.messageManager.sendAsyncMessage(
        "Marionette:" + name + this.currentRemoteFrame.targetFrameId, values);
    }
    else {
      this.messageManager.broadcastAsyncMessage(
        "Marionette:" + name + this.curBrowser.curFrameId, values);
    }
  },

  






  addMessageManagerListeners: function MDA_addMessageManagerListeners(messageManager) {
    messageManager.addMessageListener("Marionette:ok", this);
    messageManager.addMessageListener("Marionette:done", this);
    messageManager.addMessageListener("Marionette:error", this);
    messageManager.addMessageListener("Marionette:log", this);
    messageManager.addMessageListener("Marionette:shareData", this);
    messageManager.addMessageListener("Marionette:register", this);
    messageManager.addMessageListener("Marionette:runEmulatorCmd", this);
    messageManager.addMessageListener("Marionette:switchToFrame", this);
  },

  






  removeMessageManagerListeners: function MDA_removeMessageManagerListeners(messageManager) {
    messageManager.removeMessageListener("Marionette:ok", this);
    messageManager.removeMessageListener("Marionette:done", this);
    messageManager.removeMessageListener("Marionette:error", this);
    messageManager.removeMessageListener("Marionette:log", this);
    messageManager.removeMessageListener("Marionette:shareData", this);
    messageManager.removeMessageListener("Marionette:register", this);
    messageManager.removeMessageListener("Marionette:runEmulatorCmd", this);
    messageManager.removeMessageListener("Marionette:switchToFrame", this);
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
    this.sendToClient({from:this.actorID, value: value}, command_id);
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

  








  addBrowser: function MDA_addBrowser(win) {
    let browser = new BrowserObj(win);
    let winId = win.QueryInterface(Ci.nsIInterfaceRequestor).
                    getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
    winId = winId + ((appName == "B2G") ? '-b2g' : '');
    this.browsers[winId] = browser;
    this.curBrowser = this.browsers[winId];
    if (this.curBrowser.elementManager.seenItems[winId] == undefined) {
      
      this.curBrowser.elementManager.seenItems[winId] = win;
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
    try {
      if (!Services.prefs.getBoolPref("marionette.contentListener") || !newSession) {
        this.curBrowser.loadFrameScript(FRAME_SCRIPT, win);
      }
    }
    catch (e) {
      
      logger.info("could not load listener into content for page: " + win.location.href);
    }
    utils.window = win;
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

  











  






  newSession: function MDA_newSession() {
    this.command_id = this.getCommandId();
    this.newSessionCommandId = this.command_id;

    this.scriptTimeout = 10000;

    function waitForWindow() {
      let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      let win = this.getCurrentWindow();
      if (!win ||
          (appName == "Firefox" && !win.gBrowser) ||
          (appName == "Fennec" && !win.BrowserApp)) { 
        checkTimer.initWithCallback(waitForWindow.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.startBrowser(win, true);
      }
    }

    this.switchToGlobalMessageManager();

    if (!Services.prefs.getBoolPref("marionette.contentListener")) {
      waitForWindow.call(this);
    }
    else if ((appName != "Firefox") && (this.curBrowser == null)) {
      
      this.addBrowser(this.getCurrentWindow());
      this.curBrowser.startSession(false, this.getCurrentWindow(), this.whenBrowserStarted);
      this.messageManager.broadcastAsyncMessage("Marionette:restart", {});
    }
    else {
      this.sendError("Session already running", 500, null, this.command_id);
    }
  },

  getSessionCapabilities: function MDA_getSessionCapabilities(){
    this.command_id = this.getCommandId();

    let rotatable = appName == "B2G" ? true : false;

    let value = {
          'appBuildId' : Services.appinfo.appBuildID,
          'XULappId' : Services.appinfo.ID,
          'cssSelectorsEnabled': true,
          'browserName': appName,
          'handlesAlerts': false,
          'javascriptEnabled': true,
          'nativeEvents': false,
          'platform': Services.appinfo.OS,
          'rotatable': rotatable,
          'takesScreenshot': false,
          'version': Services.appinfo.version
    };

    this.sendResponse(value, this.command_id);
  },

  getStatus: function MDA_getStatus(){
    this.command_id = this.getCommandId();

    let arch;
    try {
      arch = (Services.appinfo.XPCOMABI || 'unknown').split('-')[0]
    }
    catch (ignored) {
      arch = 'unknown'
    };

    let value = {
          'os': {
            'arch': arch,
            'name': Services.appinfo.OS,
            'version': 'unknown'
          },
          'build': {
            'revision': 'unknown',
            'time': Services.appinfo.platformBuildID,
            'version': Services.appinfo.version
          }
    };

    this.sendResponse(value, this.command_id);
  },

  






  log: function MDA_log(aRequest) {
    this.command_id = this.getCommandId();
    this.marionetteLog.log(aRequest.value, aRequest.level);
    this.sendOk(this.command_id);
  },

  


  getLogs: function MDA_getLogs() {
    this.command_id = this.getCommandId();
    this.sendResponse(this.marionetteLog.getLogs(), this.command_id);
  },
  
  


  addPerfData: function MDA_addPerfData(aRequest) {
    this.command_id = this.getCommandId();
    this.marionettePerf.addPerfData(aRequest.suite, aRequest.name, aRequest.value);
    this.sendOk(this.command_id);
  },

  


  getPerfData: function MDA_getPerfData() {
    this.command_id = this.getCommandId();
    this.sendResponse(this.marionettePerf.getPerfData(), this.command_id);
  },

  





  setContext: function MDA_setContext(aRequest) {
    this.command_id = this.getCommandId();
    this.logRequest("setContext", aRequest);
    let context = aRequest.value;
    if (context != "content" && context != "chrome") {
      this.sendError("invalid context", 500, null, this.command_id);
    }
    else {
      this.context = context;
      this.sendOk(this.command_id);
    }
  },

  











  createExecuteSandbox: function MDA_createExecuteSandbox(aWindow, marionette, args, specialPowers, command_id) {
    try {
      args = this.curBrowser.elementManager.convertWrappedArguments(args, aWindow);
    }
    catch(e) {
      this.sendError(e.message, e.code, e.stack, command_id);
      return;
    }

    let _chromeSandbox = new Cu.Sandbox(aWindow,
       { sandboxPrototype: aWindow, wantXrays: false, sandboxName: ''});
    _chromeSandbox.__namedArgs = this.curBrowser.elementManager.applyNamedArgs(args);
    _chromeSandbox.__marionetteParams = args;
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
      script = data + script;
    }

    let res = Cu.evalInSandbox(script, sandbox, "1.8");

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
    let timeout = aRequest.scriptTimeout ? aRequest.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();
    this.logRequest("execute", aRequest);
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    if (this.context == "content") {
      this.sendAsync("executeScript", {value: aRequest.value,
                                       args: aRequest.args,
                                       newSandbox: aRequest.newSandbox,
                                       timeout: timeout,
                                       command_id: command_id,
                                       specialPowers: aRequest.specialPowers});
      return;
    }

    let curWindow = this.getCurrentWindow();
    let marionette = new Marionette(this, curWindow, "chrome",
                                    this.marionetteLog, this.marionettePerf,
                                    timeout, this.testName);
    let _chromeSandbox = this.createExecuteSandbox(curWindow,
                                                   marionette,
                                                   aRequest.args,
                                                   aRequest.specialPowers,
                                                   command_id);
    if (!_chromeSandbox)
      return;

    try {
      _chromeSandbox.finish = function chromeSandbox_finish() {
        return marionette.generate_results();
      };

      let script;
      if (directInject) {
        script = aRequest.value;
      }
      else {
        script = "let func = function() {" +
                       aRequest.value + 
                     "};" +
                     "func.apply(null, __marionetteParams);";
      }
      this.executeScriptInSandbox(_chromeSandbox, script, directInject,
                                  false, command_id, timeout);
    }
    catch (e) {
      this.sendError(e.name + ': ' + e.message, 17, e.stack, command_id);
    }
  },

  





  setScriptTimeout: function MDA_setScriptTimeout(aRequest) {
    this.command_id = this.getCommandId();
    let timeout = parseInt(aRequest.value);
    if(isNaN(timeout)){
      this.sendError("Not a Number", 500, null, this.command_id);
    }
    else {
      this.scriptTimeout = timeout;
      this.sendOk(this.command_id);
    }
  },

  







  executeJSScript: function MDA_executeJSScript(aRequest) {
    let timeout = aRequest.scriptTimeout ? aRequest.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();
    
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    if (this.context == "chrome") {
      if (aRequest.async) {
        this.executeWithCallback(aRequest, aRequest.async);
      }
      else {
        this.execute(aRequest, true);
      }
    }
    else {
      this.sendAsync("executeJSScript", { value: aRequest.value,
                                          args: aRequest.args,
                                          newSandbox: aRequest.newSandbox,
                                          async: aRequest.async,
                                          timeout: timeout,
                                          command_id: command_id,
                                          specialPowers: aRequest.specialPowers });
   }
  },

  














  executeWithCallback: function MDA_executeWithCallback(aRequest, directInject) {
    let timeout = aRequest.scriptTimeout ? aRequest.scriptTimeout : this.scriptTimeout;
    let command_id = this.command_id = this.getCommandId();
    this.logRequest("executeWithCallback", aRequest);
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }

    if (this.context == "content") {
      this.sendAsync("executeAsyncScript", {value: aRequest.value,
                                            args: aRequest.args,
                                            id: this.command_id,
                                            newSandbox: aRequest.newSandbox,
                                            timeout: timeout,
                                            command_id: command_id,
                                            specialPowers: aRequest.specialPowers});
      return;
    }

    let curWindow = this.getCurrentWindow();
    let original_onerror = curWindow.onerror;
    let that = this;
    that.timeout = timeout;
    let marionette = new Marionette(this, curWindow, "chrome",
                                    this.marionetteLog, this.marionettePerf,
                                    timeout, this.testName);
    marionette.command_id = this.command_id;

    function chromeAsyncReturnFunc(value, status) {
      if (that._emu_cbs && Object.keys(that._emu_cbs).length) {
        value = "Emulator callback still pending when finish() called";
        status = 500;
        that._emu_cbs = null;
      }

      if (value == undefined)
        value = null;
      if (that.command_id == marionette.command_id) {
        if (that.timer != null) {
          that.timer.cancel();
          that.timer = null;
        }

        curWindow.onerror = original_onerror;

        if (status == 0 || status == undefined) {
          that.sendToClient({from: that.actorID, value: that.curBrowser.elementManager.wrapValue(value), status: status},
                            marionette.command_id);
        }
        else {
          let error_msg = {message: value, status: status, stacktrace: null};
          that.sendToClient({from: that.actorID, error: error_msg},
                            marionette.command_id);
        }
      }
    }

    curWindow.onerror = function (errorMsg, url, lineNumber) {
      chromeAsyncReturnFunc(errorMsg + " at: " + url + " line: " + lineNumber, 17);
      return true;
    };

    function chromeAsyncFinish() {
      chromeAsyncReturnFunc(marionette.generate_results(), 0);
    }

    let _chromeSandbox = this.createExecuteSandbox(curWindow,
                                                   marionette,
                                                   aRequest.args,
                                                   aRequest.specialPowers,
                                                   command_id);
    if (!_chromeSandbox)
      return;

    try {

      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (this.timer != null) {
        this.timer.initWithCallback(function() {
          chromeAsyncReturnFunc("timed out", 28);
        }, that.timeout, Ci.nsITimer.TYPE_ONESHOT);
      }

      _chromeSandbox.returnFunc = chromeAsyncReturnFunc;
      _chromeSandbox.finish = chromeAsyncFinish;

      let script;
      if (directInject) {
        script = aRequest.value;
      }
      else {
        script =  '__marionetteParams.push(returnFunc);'
                + 'let marionetteScriptFinished = returnFunc;'
                + 'let __marionetteFunc = function() {' + aRequest.value + '};'
                + '__marionetteFunc.apply(null, __marionetteParams);';
      }

      this.executeScriptInSandbox(_chromeSandbox, script, directInject,
                                  true, command_id, timeout);
    } catch (e) {
      chromeAsyncReturnFunc(e.name + ": " + e.message, 17);
    }
  },

  





  goUrl: function MDA_goUrl(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context != "chrome") {
      aRequest.command_id = command_id;
      aRequest.pageTimeout = this.pageTimeout;
      this.sendAsync("goUrl", aRequest);
      return;
    }

    this.getCurrentWindow().location.href = aRequest.value;
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

  


  getUrl: function MDA_getUrl() {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      this.sendResponse(this.getCurrentWindow().location.href, this.command_id);
    }
    else {
      this.sendAsync("getUrl", {command_id: this.command_id});
    }
  },

  


  getTitle: function MDA_getTitle() {
    this.command_id = this.getCommandId();
    this.sendAsync("getTitle", {command_id: this.command_id});
  },

  


  getPageSource: function MDA_getPageSource(){
    this.command_id = this.getCommandId();
    if (this.context == "chrome"){
      var curWindow = this.getCurrentWindow();
      var XMLSerializer = curWindow.XMLSerializer; 
      var pageSource = new XMLSerializer().serializeToString(curWindow.document);
      this.sendResponse(pageSource, this.command_id);
    }
    else {
      this.sendAsync("getPageSource", {command_id: this.command_id});
    }
  },

  


  goBack: function MDA_goBack() {
    this.command_id = this.getCommandId();
    this.sendAsync("goBack", {command_id: this.command_id});
  },

  


  goForward: function MDA_goForward() {
    this.command_id = this.getCommandId();
    this.sendAsync("goForward", {command_id: this.command_id});
  },

  


  refresh: function MDA_refresh() {
    this.command_id = this.getCommandId();
    this.sendAsync("refresh", {command_id: this.command_id});
  },

  


  getWindow: function MDA_getWindow() {
    this.command_id = this.getCommandId();
    for (let i in this.browsers) {
      if (this.curBrowser == this.browsers[i]) {
        this.sendResponse(i, this.command_id);
      }
    }
  },

  


  getWindows: function MDA_getWindows() {
    this.command_id = this.getCommandId();
    let res = [];
    let winEn = this.getWinEnumerator(); 
    while(winEn.hasMoreElements()) {
      let foundWin = winEn.getNext();
      let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
      winId = winId + ((appName == "B2G") ? '-b2g' : '');
      res.push(winId)
    }
    this.sendResponse(res, this.command_id);
  },

  






  switchToWindow: function MDA_switchToWindow(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    let winEn = this.getWinEnumerator(); 
    while(winEn.hasMoreElements()) {
      let foundWin = winEn.getNext();
      let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils)
                          .outerWindowID;
      winId = winId + ((appName == "B2G") ? '-b2g' : '');
      if (aRequest.value == foundWin.name || aRequest.value == winId) {
        if (this.browsers[winId] == undefined) {
          
          this.startBrowser(foundWin, false);
        }
        else {
          utils.window = foundWin;
          this.curBrowser = this.browsers[winId];
        }
        this.sendOk(command_id);
        return;
      }
    }
    this.sendError("Unable to locate window " + aRequest.value, 23, null,
                   command_id);
  },
 
  








  switchToFrame: function MDA_switchToFrame(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    this.logRequest("switchToFrame", aRequest);
    let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let checkLoad = function() { 
      let errorRegex = /about:.+(error)|(blocked)\?/;
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
    let curWindow = this.getCurrentWindow();
    if (this.context == "chrome") {
      let foundFrame = null;
      if ((aRequest.value == null) && (aRequest.element == null)) {
        this.curFrame = null;
        if (aRequest.focus) {
          this.mainFrame.focus();
        }
        checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
        return;
      }
      if (aRequest.element != undefined) {
        if (this.curBrowser.elementManager.seenItems[aRequest.element] != undefined) {
          let wantedFrame = this.curBrowser.elementManager.getKnownElement(aRequest.element, curWindow); 
          let numFrames = curWindow.frames.length;
          for (let i = 0; i < numFrames; i++) {
            if (curWindow.frames[i].frameElement == wantedFrame) {
              curWindow = curWindow.frames[i]; 
              this.curFrame = curWindow;
              if (aRequest.focus) {
                this.curFrame.focus();
              }
              checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
              return;
          }
        }
      }
    }
    switch(typeof(aRequest.value)) {
      case "string" :
        let foundById = null;
        let numFrames = curWindow.frames.length;
        for (let i = 0; i < numFrames; i++) {
          
          let frame = curWindow.frames[i];
          let frameElement = frame.frameElement;
          if (frame.name == aRequest.value) {
            foundFrame = i;
            break;
          } else if ((foundById == null) && (frameElement.id == aRequest.value)) {
            foundById = i;
          }
        }
        if ((foundFrame == null) && (foundById != null)) {
          foundFrame = foundById;
        }
        break;
      case "number":
        if (curWindow.frames[aRequest.value] != undefined) {
          foundFrame = aRequest.value;
        }
        break;
      }
      if (foundFrame != null) {
        curWindow = curWindow.frames[foundFrame];
        this.curFrame = curWindow;
        if (aRequest.focus) {
          this.curFrame.focus();
        }
        checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
      } else {
        this.sendError("Unable to locate frame: " + aRequest.value, 8, null,
                       command_id);
      }
    }
    else {
      if ((!aRequest.value) && (!aRequest.element) &&
          (this.currentRemoteFrame !== null)) {
        
        
        
        
        this.switchToGlobalMessageManager();
      }
      aRequest.command_id = command_id;
      this.sendAsync("switchToFrame", aRequest);
    }
  },

  





  setSearchTimeout: function MDA_setSearchTimeout(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        this.curBrowser.elementManager.setSearchTimeout(aRequest.value);
        this.sendOk(this.command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, this.command_id);
      }
    }
    else {
      this.sendAsync("setSearchTimeout", {value: aRequest.value,
                                          command_id: this.command_id});
    }
  },

  






  timeouts: function MDA_timeouts(aRequest){
    
    this.command_id = this.getCommandId();
    let timeout_type = aRequest.timeoutType;
    let timeout = parseInt(aRequest.ms);
    if (isNaN(timeout)) {
      this.sendError("Not a Number", 500, null, this.command_id);
    }
    else {
      if (timeout_type == "implicit") {
        aRequest.value = aRequest.ms;
        this.setSearchTimeout(aRequest);
      }
      else if (timeout_type == "script") {
        aRequest.value = aRequest.ms;
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
    let serId = aRequest.element;
    let x = aRequest.x;
    let y = aRequest.y;
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("singleTap", {value: serId,
                                   corx: x,
                                   cory: y,
                                   command_id: this.command_id});
    }
  },

  





  doubleTap: function MDA_doubleTap(aRequest) {
    this.command_id = this.getCommandId();
    let serId = aRequest.element;
    let x = aRequest.x;
    let y = aRequest.y;
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("doubleTap", {value: serId,
                                   corx: x,
                                   cory: y,
                                   command_id: this.command_id});
    }
  },

  





  press: function MDA_press(aRequest) {
    this.command_id = this.getCommandId();
    let element = aRequest.element;
    let x = aRequest.x;
    let y = aRequest.y;
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("press", {value: element,
                               corx: x,
                               cory: y,
                               command_id: this.command_id});
    }
  },

  





  cancelTouch: function MDA_cancelTouch(aRequest) {
    this.command_id = this.getCommandId();
    let element = aRequest.element;
    let touchId = aRequest.touchId;
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("cancelTouch", {value: element,
                                     touchId: touchId,
                                     command_id: this.command_id});
    }
  },

  





  release: function MDA_release(aRequest) {
    this.command_id = this.getCommandId();
    let element = aRequest.element;
    let touchId = aRequest.touchId;
    let x = aRequest.x;
    let y = aRequest.y;
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("release", {value: element,
                                 touchId: touchId,
                                 corx: x,
                                 cory: y,
                                 command_id: this.command_id});
    }
  },

  





  actionChain: function MDA_actionChain(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("actionChain", {chain: aRequest.chain,
                                     nextId: aRequest.nextId,
                                     command_id: this.command_id});
    }
  },

  








  multiAction: function MDA_multiAction(aRequest) {
    this.command_id = this.getCommandId();
    if (this.context == "chrome") {
       this.sendError("Not in Chrome", 500, null, this.command_id);
    }
    else {
      this.sendAsync("multiAction", {value: aRequest.value,
                                     maxlen: aRequest.max_length,
                                     command_id: this.command_id});
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
                              aRequest,
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
      this.sendAsync("findElementContent", {value: aRequest.value,
                                            using: aRequest.using,
                                            element: aRequest.element,
                                            command_id: command_id});
    }
  },

  






  findElements: function MDA_findElements(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      let id;
      try {
        let on_success = this.sendResponse.bind(this);
        let on_error = this.sendError.bind(this);
        id = this.curBrowser.elementManager.find(this.getCurrentWindow(),
                                                 aRequest,
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
      this.sendAsync("findElementsContent", {value: aRequest.value,
                                             using: aRequest.using,
                                             element: aRequest.element,
                                             command_id: command_id});
    }
  },

  


  getActiveElement: function MDA_getActiveElement(){
    let command_id = this.command_id = this.getCommandId();
    this.sendAsync("getActiveElement", {command_id: command_id});
  },

  






  clickElement: function MDA_clickElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        el.click();
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      
      
      
      
      let curWindow = this.getCurrentWindow();
      let self = this;
      this.mozBrowserClose = function() { 
        curWindow.removeEventListener('mozbrowserclose', self.mozBrowserClose, true);
        self.switchToGlobalMessageManager();
        self.sendError("The frame closed during the click, recovering to allow further communications", 500, null, command_id);
      };
      curWindow.addEventListener('mozbrowserclose', this.mozBrowserClose, true);
      this.sendAsync("clickElement", {element: aRequest.element,
                                      command_id: command_id});
    }
  },

  







  getElementAttribute: function MDA_getElementAttribute(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        this.sendResponse(utils.getElementAttribute(el, aRequest.name),
                          command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementAttribute", {element: aRequest.element,
                                             name: aRequest.name,
                                             command_id: command_id});
    }
  },

  






  getElementText: function MDA_getElementText(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
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
      this.sendAsync("getElementText", {element: aRequest.element,
                                        command_id: command_id});
    }
  },

  






  getElementTagName: function MDA_getElementTagName(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        this.sendResponse(el.tagName.toLowerCase(), command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementTagName", {element: aRequest.element,
                                           command_id: command_id});
    }
  },

  






  isElementDisplayed: function MDA_isElementDisplayed(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        this.sendResponse(utils.isElementDisplayed(el), command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("isElementDisplayed", {element:aRequest.element,
                                            command_id: command_id});
    }
  },

  






  isElementEnabled: function MDA_isElementEnabled(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        if (el.disabled != undefined) {
          this.sendResponse(!!!el.disabled, command_id);
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
      this.sendAsync("isElementEnabled", {element:aRequest.element,
                                          command_id: command_id});
    }
  },

  






  isElementSelected: function MDA_isElementSelected(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
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
      this.sendAsync("isElementSelected", {element:aRequest.element,
                                           command_id: command_id});
    }
  },

  getElementSize: function MDA_getElementSize(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        let clientRect = el.getBoundingClientRect();  
        this.sendResponse({width: clientRect.width, height: clientRect.height},
                          command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("getElementSize", {element:aRequest.element,
                                        command_id: command_id});
    }
  },

  







  sendKeysToElement: function MDA_sendKeysToElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
        el.focus();
        utils.sendString(aRequest.value.join(""), utils.window);
        this.sendOk(command_id);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack, command_id);
      }
    }
    else {
      this.sendAsync("sendKeysToElement", {element:aRequest.element,
                                           value: aRequest.value,
                                           command_id: command_id});
    }
  },

  




  setTestName: function MDA_setTestName(aRequest) {
    this.command_id = this.getCommandId();
    this.logRequest("setTestName", aRequest);
    this.testName = aRequest.value;
    this.sendAsync("setTestName", {value: aRequest.value,
                                   command_id: this.command_id});
  },

  






  clearElement: function MDA_clearElement(aRequest) {
    let command_id = this.command_id = this.getCommandId();
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(
            aRequest.element, this.getCurrentWindow());
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
      this.sendAsync("clearElement", {element:aRequest.element,
                                      command_id: command_id});
    }
  },

  getElementPosition: function MDA_getElementPosition(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("getElementPosition", {element:aRequest.element,
                                          command_id: this.command_id});
  },

  


  addCookie: function MDA_addCookie(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("addCookie", {cookie:aRequest.cookie,
                                 command_id: this.command_id});
  },

  


  getAllCookies: function MDA_getAllCookies() {
    this.command_id = this.getCommandId();
    this.sendAsync("getAllCookies", {command_id: this.command_id});
  },

  


  deleteAllCookies: function MDA_deleteAllCookies() {
    this.command_id = this.getCommandId();
    this.sendAsync("deleteAllCookies", {command_id: this.command_id});
  },

  


  deleteCookie: function MDA_deleteCookie(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("deleteCookie", {name:aRequest.name,
                                    command_id: this.command_id});
  },

  







  closeWindow: function MDA_closeWindow() {
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

      
      if (numOpenWindows === 1){
        this.deleteSession();
        return;
      }

      try{
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

  








  deleteSession: function MDA_deleteSession() {
    let command_id = this.command_id = this.getCommandId();
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
      this.curBrowser.closeTab();
      
      for (let win in this.browsers) {
        for (let i in this.browsers[win].knownFrames) {
          this.globalMessageManager.broadcastAsyncMessage("Marionette:deleteSession" + this.browsers[win].knownFrames[i], {});
        }
      }
      let winEnum = this.getWinEnumerator();
      while (winEnum.hasMoreElements()) {
        winEnum.getNext().messageManager.removeDelayedFrameScript(FRAME_SCRIPT); 
      }
    }
    this.sendOk(command_id);
    this.removeMessageManagerListeners(this.globalMessageManager);
    this.switchToGlobalMessageManager();
    
    this.curFrame = null;
    if (this.mainFrame) {
      this.mainFrame.focus();
    }
    this.curBrowser = null;
    try {
      this.importedScripts.remove(false);
    }
    catch (e) {
    }
  },

  


  getAppCacheStatus: function MDA_getAppCacheStatus(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("getAppCacheStatus", {command_id: this.command_id});
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

  emulatorCmdResult: function emulatorCmdResult(message) {
    if (this.context != "chrome") {
      this.sendAsync("emulatorCmdResult", message);
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
      file.write(aRequest.script, aRequest.script.length);
      file.close();
      this.sendOk(command_id);
    }
    else {
      this.sendAsync("importScript", {script: aRequest.script,
                                      command_id: command_id});
    }
  },

  



  screenShot: function MDA_saveScreenshot(aRequest) {
    this.command_id = this.getCommandId();
    this.sendAsync("screenShot", {element: aRequest.element,
                                  highlights: aRequest.highlights,
                                  command_id: this.command_id});
  },

  



  generateFrameId: function MDA_generateFrameId(id) {
    let uid = id + (appName == "B2G" ? "-b2g" : "");
    return uid;
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
        if (message.json.perf) {
          this.marionettePerf.appendPerfData(message.json.perf);
        }
        break;
      case "Marionette:runEmulatorCmd":
        this.sendToClient(message.json, -1);
        break;
      case "Marionette:switchToFrame":
        
        let thisWin = this.getCurrentWindow();
        let frameWindow = thisWin.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIDOMWindowUtils)
                                 .getOuterWindowWithId(message.json.win);
        let thisFrame = frameWindow.document.getElementsByTagName("iframe")[message.json.frame];
        let mm = thisFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;

        
        
        for (let i = 0; i < remoteFrames.length; i++) {
          let frame = remoteFrames[i];
          if ((frame.messageManager == mm)) {
            this.currentRemoteFrame = frame;
            this.currentRemoteFrame.command_id = message.json.command_id;
            this.messageManager = frame.messageManager;
            this.addMessageManagerListeners(this.messageManager);
            this.messageManager.sendAsyncMessage("Marionette:restart", {});
            return;
          }
        }

        
        
        this.addMessageManagerListeners(mm);
        mm.loadFrameScript(FRAME_SCRIPT, true);
        this.messageManager = mm;
        let aFrame = new MarionetteRemoteFrame(message.json.win, message.json.frame);
        aFrame.messageManager = this.messageManager;
        aFrame.command_id = message.json.command_id;
        remoteFrames.push(aFrame);
        this.currentRemoteFrame = aFrame;
        break;
      case "Marionette:register":
        
        
        let nullPrevious = (this.curBrowser.curFrameId == null);
        let curWin = this.getCurrentWindow();
        let listenerWindow = curWin.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIDOMWindowUtils)
                                   .getOuterWindowWithId(message.json.value);

        if (!listenerWindow || (listenerWindow.location.href != message.json.href) &&
            (this.currentRemoteFrame !== null)) {
          
          
          
          
          
          
          
          
          
          this.currentRemoteFrame.targetFrameId = this.generateFrameId(message.json.value);
          this.sendAsync(
              "setState",
              {scriptTimeout: this.scriptTimeout,
               searchTimeout: this.curBrowser.elementManager.searchTimeout,
               command_id: this.currentRemoteFrame.command_id});
        }

        let browserType;
        try {
          browserType = message.target.getAttribute("type");
        } catch (ex) {
          
        }
        let reg = {};
        if (!browserType || browserType != "content") {
          reg.id = this.curBrowser.register(this.generateFrameId(message.json.value),
                                         message.json.href); 
        }
        this.curBrowser.elementManager.seenItems[reg.id] = listenerWindow; 
        reg.importedScripts = this.importedScripts.path;
        if (nullPrevious && (this.curBrowser.curFrameId != null)) {
          this.sendAsync("newSession", {B2G: (appName == "B2G")});
          if (this.curBrowser.newSession) {
            this.sendResponse(reg.id, this.newSessionCommandId);
            this.newSessionCommandId = null;
          }
        }
        return reg;
    }
  }
};

MarionetteDriverActor.prototype.requestTypes = {
  "newSession": MarionetteDriverActor.prototype.newSession,
  "getSessionCapabilities": MarionetteDriverActor.prototype.getSessionCapabilities,
  "getStatus": MarionetteDriverActor.prototype.getStatus,
  "log": MarionetteDriverActor.prototype.log,
  "getLogs": MarionetteDriverActor.prototype.getLogs,
  "addPerfData": MarionetteDriverActor.prototype.addPerfData,
  "getPerfData": MarionetteDriverActor.prototype.getPerfData,
  "setContext": MarionetteDriverActor.prototype.setContext,
  "executeScript": MarionetteDriverActor.prototype.execute,
  "setScriptTimeout": MarionetteDriverActor.prototype.setScriptTimeout,
  "timeouts": MarionetteDriverActor.prototype.timeouts,
  "singleTap": MarionetteDriverActor.prototype.singleTap,
  "doubleTap": MarionetteDriverActor.prototype.doubleTap,
  "press": MarionetteDriverActor.prototype.press,
  "release": MarionetteDriverActor.prototype.release,
  "cancelTouch": MarionetteDriverActor.prototype.cancelTouch,
  "actionChain": MarionetteDriverActor.prototype.actionChain,
  "multiAction": MarionetteDriverActor.prototype.multiAction,
  "executeAsyncScript": MarionetteDriverActor.prototype.executeWithCallback,
  "executeJSScript": MarionetteDriverActor.prototype.executeJSScript,
  "setSearchTimeout": MarionetteDriverActor.prototype.setSearchTimeout,
  "findElement": MarionetteDriverActor.prototype.findElement,
  "findElements": MarionetteDriverActor.prototype.findElements,
  "clickElement": MarionetteDriverActor.prototype.clickElement,
  "getElementAttribute": MarionetteDriverActor.prototype.getElementAttribute,
  "getElementText": MarionetteDriverActor.prototype.getElementText,
  "getElementTagName": MarionetteDriverActor.prototype.getElementTagName,
  "isElementDisplayed": MarionetteDriverActor.prototype.isElementDisplayed,
  "getElementSize": MarionetteDriverActor.prototype.getElementSize,
  "isElementEnabled": MarionetteDriverActor.prototype.isElementEnabled,
  "isElementSelected": MarionetteDriverActor.prototype.isElementSelected,
  "sendKeysToElement": MarionetteDriverActor.prototype.sendKeysToElement,
  "getElementPosition": MarionetteDriverActor.prototype.getElementPosition,
  "clearElement": MarionetteDriverActor.prototype.clearElement,
  "getTitle": MarionetteDriverActor.prototype.getTitle,
  "getPageSource": MarionetteDriverActor.prototype.getPageSource,
  "goUrl": MarionetteDriverActor.prototype.goUrl,
  "getUrl": MarionetteDriverActor.prototype.getUrl,
  "goBack": MarionetteDriverActor.prototype.goBack,
  "goForward": MarionetteDriverActor.prototype.goForward,
  "refresh":  MarionetteDriverActor.prototype.refresh,
  "getWindow":  MarionetteDriverActor.prototype.getWindow,
  "getWindows":  MarionetteDriverActor.prototype.getWindows,
  "switchToFrame": MarionetteDriverActor.prototype.switchToFrame,
  "switchToWindow": MarionetteDriverActor.prototype.switchToWindow,
  "deleteSession": MarionetteDriverActor.prototype.deleteSession,
  "emulatorCmdResult": MarionetteDriverActor.prototype.emulatorCmdResult,
  "importScript": MarionetteDriverActor.prototype.importScript,
  "getAppCacheStatus": MarionetteDriverActor.prototype.getAppCacheStatus,
  "closeWindow": MarionetteDriverActor.prototype.closeWindow,
  "setTestName": MarionetteDriverActor.prototype.setTestName,
  "screenShot": MarionetteDriverActor.prototype.screenShot,
  "addCookie": MarionetteDriverActor.prototype.addCookie,
  "getAllCookies": MarionetteDriverActor.prototype.getAllCookies,
  "deleteAllCookies": MarionetteDriverActor.prototype.deleteAllCookies,
  "deleteCookie": MarionetteDriverActor.prototype.deleteCookie,
  "getActiveElement": MarionetteDriverActor.prototype.getActiveElement
};









function BrowserObj(win) {
  this.DESKTOP = "desktop";
  this.B2G = "B2G";
  this.browser;
  this.tab = null;
  this.window = win;
  this.knownFrames = [];
  this.curFrameId = null;
  this.startPage = "about:blank";
  this.mainContentId = null; 
  this.newSession = true; 
  this.elementManager = new ElementManager([SELECTOR, NAME, LINK_TEXT, PARTIAL_LINK_TEXT]);
  this.setBrowser(win);
}

BrowserObj.prototype = {
  





  setBrowser: function BO_setBrowser(win) {
    switch (appName) {
      case "Firefox":
        this.browser = win.gBrowser;
        break;
      case "Fennec":
        this.browser = win.BrowserApp;
        break;
    }
  },
  










  startSession: function BO_startSession(newTab, win, callback) {
    if (appName != "Firefox") {
      callback(win, newTab);
    }
    else if (newTab) {
      this.addTab(this.startPage);
      
      this.browser.selectedTab = this.tab;
      let newTabBrowser = this.browser.getBrowserForTab(this.tab);
      
      newTabBrowser.addEventListener("load", function onLoad() {
        newTabBrowser.removeEventListener("load", onLoad, true);
        callback(win, newTab);
      }, true);
    }
    else {
      
      if (this.browser != undefined && this.browser.selectedTab != undefined) {
        this.tab = this.browser.selectedTab;
      }
      callback(win, newTab);
    }
  },

  


  closeTab: function BO_closeTab() {
    if (this.tab != null && (appName != "B2G")) {
      this.browser.removeTab(this.tab);
      this.tab = null;
    }
  },

  





  addTab: function BO_addTab(uri) {
    this.tab = this.browser.addTab(uri, true);
  },

  







  loadFrameScript: function BO_loadFrameScript(script, frame) {
    frame.window.messageManager.loadFrameScript(script, true);
    Services.prefs.setBoolPref("marionette.contentListener", true);
  },

  









  register: function BO_register(uid, href) {
    if (this.curFrameId == null) {
      if ((!this.newSession) || (this.newSession && 
          ((appName != "Firefox") || href.indexOf(this.startPage) > -1))) {
        this.curFrameId = uid;
        this.mainContentId = uid;
      }
    }
    this.knownFrames.push(uid); 
    return uid;
  },
}
