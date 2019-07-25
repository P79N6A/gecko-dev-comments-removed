



"use strict";





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
  },
}


MarionetteRootActor.prototype.requestTypes = {
  "getMarionetteID": MarionetteRootActor.prototype.getMarionetteID,
  "sayHello": MarionetteRootActor.prototype.sayHello
};






function MarionetteDriverActor(aConnection)
{
  this.uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                   .getService(Ci.nsIUUIDGenerator);

  this.conn = aConnection;
  this.messageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                          .getService(Ci.nsIMessageBroadcaster);
  this.browsers = {}; 
  this.curBrowser = null; 
  this.context = "content";
  this.scriptTimeout = null;
  this.timer = null;
  this.marionetteLog = new MarionetteLogObj();
  this.marionettePerf = new MarionettePerfData();
  this.command_id = null;
  this.mainFrame = null; 
  this.curFrame = null; 
  this.importedScripts = FileUtils.getFile('TmpD', ['marionettescriptchrome']);
  
  
  this.messageManager.addMessageListener("Marionette:ok", this);
  this.messageManager.addMessageListener("Marionette:done", this);
  this.messageManager.addMessageListener("Marionette:error", this);
  this.messageManager.addMessageListener("Marionette:log", this);
  this.messageManager.addMessageListener("Marionette:shareData", this);
  this.messageManager.addMessageListener("Marionette:register", this);
  this.messageManager.addMessageListener("Marionette:goUrl", this);
  this.messageManager.addMessageListener("Marionette:runEmulatorCmd", this);
}

MarionetteDriverActor.prototype = {

  
  actorPrefix: "marionette",

  







  sendAsync: function MDA_sendAsync(name, values) {
    this.messageManager.broadcastAsyncMessage("Marionette:" + name + this.curBrowser.curFrameId, values);
  },

  



  








  sendToClient: function MDA_sendToClient(msg, command_id) {
    logger.info("sendToClient: " + JSON.stringify(msg) + ", " + command_id + ", " + this.command_id);
    if (this.command_id != null &&
        command_id != null &&
        this.command_id != command_id) {
      return;
    }
    this.conn.send(msg);
    if (command_id != null) {
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
      if (appName != "B2G" && this.context == "content") {
        type = 'navigator:browser';
      }
      return Services.wm.getMostRecentWindow(type);
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
    this.curBrowser.startSession(newSession);
    try {
      if (!Services.prefs.getBoolPref("marionette.contentListener") || !newSession) {
        this.curBrowser.loadFrameScript("chrome://marionette/content/marionette-listener.js", win);
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

  



  






  newSession: function MDA_newSession() {

    function waitForWindow() {
      let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      let win = this.getCurrentWindow();
      if (!win || (appName != "B2G" && !win.gBrowser)) { 
        checkTimer.initWithCallback(waitForWindow.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.startBrowser(win, true);
      }
    }

    if (!Services.prefs.getBoolPref("marionette.contentListener")) {
      waitForWindow.call(this);
    }
    else if ((appName == "B2G") && (this.curBrowser == null)) {
      
      this.addBrowser(this.getCurrentWindow());
      this.curBrowser.startSession(false);
      this.messageManager.broadcastAsyncMessage("Marionette:restart", {});
    }
    else {
      this.sendError("Session already running", 500, null);
    }
  },

  getSessionCapabilities: function MDA_getSessionCapabilities(){
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

    this.sendResponse(value);
  },

  






  log: function MDA_log(aRequest) {
    this.marionetteLog.log(aRequest.value, aRequest.level);
    this.sendOk();
  },

  


  getLogs: function MDA_getLogs() {
    this.sendResponse(this.marionetteLog.getLogs());
  },
  
  


  addPerfData: function MDA_addPerfData(aRequest) {
    this.marionettePerf.addPerfData(aRequest.suite, aRequest.name, aRequest.value);
    this.sendOk();
  },

  


  getPerfData: function MDA_getPerfData() {
    this.sendResponse(this.marionettePerf.getPerfData());
  },

  





  setContext: function MDA_setContext(aRequest) {
    let context = aRequest.value;
    if (context != "content" && context != "chrome") {
      this.sendError("invalid context", 500, null);
    }
    else {
      this.context = context;
      this.sendOk();
    }
  },

  











  createExecuteSandbox: function MDA_createExecuteSandbox(aWindow, marionette, args, specialPowers) {
    try {
      args = this.curBrowser.elementManager.convertWrappedArguments(args, aWindow);
    }
    catch(e) {
      this.sendError(e.message, e.code, e.stack);
      return;
    }

    let _chromeSandbox = new Cu.Sandbox(aWindow,
       { sandboxPrototype: aWindow, wantXrays: false, sandboxName: ''});
    _chromeSandbox.__namedArgs = this.curBrowser.elementManager.applyNamedArgs(args);
    _chromeSandbox.__marionetteParams = args;
    _chromeSandbox.testUtils = utils;

    marionette.exports.forEach(function(fn) {
      _chromeSandbox[fn] = marionette[fn].bind(marionette);
    });

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
     directInject, async) {
    try {
      if (directInject && async &&
          (this.scriptTimeout == null || this.scriptTimeout == 0)) {
        this.sendError("Please set a timeout", 21, null);
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
        this.sendError("finish() not called", 500, null);
        return;
      }

      if (!async) {
        this.sendResponse(this.curBrowser.elementManager.wrapValue(res));
      }
    }
    catch (e) {
      this.sendError(e.name + ': ' + e.message, 17, e.stack);
    }
  },

  










  execute: function MDA_execute(aRequest, directInject) {
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    if (this.context == "content") {
      this.sendAsync("executeScript", {value: aRequest.value,
                                       args: aRequest.args,
                                       newSandbox:aRequest.newSandbox});
      return;
    }

    let curWindow = this.getCurrentWindow();
    let marionette = new Marionette(this, curWindow, "chrome", this.marionetteLog, this.marionettePerf);
    let _chromeSandbox = this.createExecuteSandbox(curWindow, marionette, aRequest.args, aRequest.specialPowers);
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
      this.executeScriptInSandbox(_chromeSandbox, script, directInject, false);
    }
    catch (e) {
      this.sendError(e.name + ': ' + e.message, 17, e.stack);
    }
  },

  





  setScriptTimeout: function MDA_setScriptTimeout(aRequest) {
    let timeout = parseInt(aRequest.value);
    if(isNaN(timeout)){
      this.sendError("Not a Number", 500, null);
    }
    else {
      this.scriptTimeout = timeout;
      this.sendAsync("setScriptTimeout", {value: timeout});
    }
  },

  







  executeJSScript: function MDA_executeJSScript(aRequest) {
    
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    if (this.context == "chrome") {
      if (aRequest.timeout) {
        this.executeWithCallback(aRequest, aRequest.timeout);
      }
      else {
        this.execute(aRequest, true);
      }
    }
    else {
      this.sendAsync("executeJSScript", {value:aRequest.value, args:aRequest.args, timeout:aRequest.timeout});
   }
  },

  














  executeWithCallback: function MDA_executeWithCallback(aRequest, directInject) {
    if (aRequest.newSandbox == undefined) {
      
      
      aRequest.newSandbox = true;
    }
    this.command_id = this.uuidGen.generateUUID().toString();

    if (this.context == "content") {
      this.sendAsync("executeAsyncScript", {value: aRequest.value,
                                            args: aRequest.args,
                                            id: this.command_id,
                                            newSandbox: aRequest.newSandbox});
      return;
    }

    let curWindow = this.getCurrentWindow();
    let original_onerror = curWindow.onerror;
    let that = this;
    let marionette = new Marionette(this, curWindow, "chrome", this.marionetteLog, this.marionettePerf);
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

    let _chromeSandbox = this.createExecuteSandbox(curWindow, marionette, aRequest.args, aRequest.specialPowers);
    if (!_chromeSandbox)
      return;

    try {

      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (this.timer != null) {
        this.timer.initWithCallback(function() {
          chromeAsyncReturnFunc("timed out", 28);
        }, that.scriptTimeout, Ci.nsITimer.TYPE_ONESHOT);
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

      this.executeScriptInSandbox(_chromeSandbox, script, directInject, true);
    } catch (e) {
      this.sendError(e.name + ": " + e.message, 17, e.stack, marionette.command_id);
    }
  },

  





  goUrl: function MDA_goUrl(aRequest) {
    if (this.context != "chrome") {
      this.sendAsync("goUrl", aRequest);
      return;
    }

    this.getCurrentWindow().location.href = aRequest.value;
    let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    function checkLoad() { 
      if (curWindow.document.readyState == "complete") { 
        sendOk();
        return;
      } 
      checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  getUrl: function MDA_getUrl() {
    if (this.context == "chrome") {
      this.sendResponse(this.getCurrentWindow().location.href);
    }
    else {
      this.sendAsync("getUrl", {});
    }
  },

  


  getTitle: function MDA_getTitle() {
    this.sendAsync("getTitle", {});
  },

  


  getPageSource: function MDA_getPageSource(){
    if (this.context == "chrome"){
      var curWindow = this.getCurrentWindow();
      var XMLSerializer = curWindow.XMLSerializer; 
      var pageSource = new XMLSerializer().serializeToString(curWindow.document);
      this.sendResponse(pageSource);
    }
    else {
      this.sendAsync("getPageSource", {});
    }
  },

  


  goBack: function MDA_goBack() {
    this.sendAsync("goBack", {});
  },

  


  goForward: function MDA_goForward() {
    this.sendAsync("goForward", {});
  },

  


  refresh: function MDA_refresh() {
    this.sendAsync("refresh", {});
  },

  


  getWindow: function MDA_getWindow() {
    for (let i in this.browsers) {
      if (this.curBrowser == this.browsers[i]) {
        this.sendResponse(i);
      }
    }
  },

  


  getWindows: function MDA_getWindows() {
    let res = [];
    let winEn = this.getWinEnumerator(); 
    while(winEn.hasMoreElements()) {
      let foundWin = winEn.getNext();
      let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
      winId = winId + ((appName == "B2G") ? '-b2g' : '');
      res.push(winId)
    }
    this.sendResponse(res);
  },

  






  switchToWindow: function MDA_switchToWindow(aRequest) {
    let winEn = this.getWinEnumerator(); 
    while(winEn.hasMoreElements()) {
      let foundWin = winEn.getNext();
      let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
      winId = winId + ((appName == "B2G") ? '-b2g' : '');
      if (aRequest.value == foundWin.name || aRequest.value == winId) {
        if (this.browsers[winId] == undefined) {
          
          this.startBrowser(foundWin, false);
        }
        else {
          utils.window = foundWin;
          this.curBrowser = this.browsers[winId];
        }
        foundWin.focus();
        this.sendOk();
        return;
      }
    }
    this.sendError("Unable to locate window " + aRequest.value, 23, null);
  },
 
  








  switchToFrame: function MDA_switchToFrame(aRequest) {
    let curWindow = this.getCurrentWindow();
    if (this.context == "chrome") {
      let foundFrame = null;
      if ((aRequest.value == null) && (aRequest.element == null)) {
        this.curFrame = null;
        this.mainFrame.focus();
        this.sendOk();
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
              this.curFrame.focus();
              this.sendOk();
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
        this.curFrame.focus();
        this.sendOk();
      } else {
        this.sendError("Unable to locate frame: " + aRequest.value, 8, null);
      }
    }
    else {
      this.sendAsync("switchToFrame", aRequest);
    }
  },

  





  setSearchTimeout: function MDA_setSearchTimeout(aRequest) {
    if (this.context == "chrome") {
      try {
        this.curBrowser.elementManager.setSearchTimeout(aRequest.value);
        this.sendOk();
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("setSearchTimeout", {value: aRequest.value});
    }
  },

  






  findElement: function MDA_findElement(aRequest) {
    if (this.context == "chrome") {
      let id;
      try {
        let on_success = this.sendResponse.bind(this);
        let on_error = this.sendError.bind(this);
        id = this.curBrowser.elementManager.find(this.getCurrentWindow(),aRequest, on_success, on_error, false);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
        return;
      }
    }
    else {
      this.sendAsync("findElementContent", {value: aRequest.value, using: aRequest.using, element: aRequest.element});
    }
  },

  






  findElements: function MDA_findElements(aRequest) {
    if (this.context == "chrome") {
      let id;
      try {
        let on_success = this.sendResponse.bind(this);
        let on_error = this.sendError.bind(this);
        id = this.curBrowser.elementManager.find(this.getCurrentWindow(), aRequest, on_success, on_error, true);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
        return;
      }
    }
    else {
      this.sendAsync("findElementsContent", {value: aRequest.value, using: aRequest.using, element: aRequest.element});
    }
  },

  






  clickElement: function MDA_clickElement(aRequest) {
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        el.click();
        this.sendOk();
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("clickElement", {element: aRequest.element});
    }
  },

  







  getElementAttribute: function MDA_getElementAttribute(aRequest) {
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        this.sendResponse(utils.getElementAttribute(el, aRequest.name));
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("getElementAttribute", {element: aRequest.element, name: aRequest.name});
    }
  },

  






  getElementText: function MDA_getElementText(aRequest) {
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        let lines = [];
        this.getVisibleText(el, lines);
        lines = lines.join("\n");
        this.sendResponse(lines);
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("getElementText", {element: aRequest.element});
    }
  },

  






  getElementTagName: function MDA_getElementTagName(aRequest) {
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        this.sendResponse(el.tagName.toLowerCase());
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("getElementTagName", {element: aRequest.element});
    }
  },

  






  isElementDisplayed: function MDA_isElementDisplayed(aRequest) {
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        this.sendResponse(utils.isElementDisplayed(el));
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("isElementDisplayed", {element:aRequest.element});
    }
  },

  






  isElementEnabled: function MDA_isElementEnabled(aRequest) {
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        if (el.disabled != undefined) {
          this.sendResponse(!!!el.disabled);
        }
        else {
        this.sendResponse(true);
        }
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("isElementEnabled", {element:aRequest.element});
    }
  },

  






  isElementSelected: function MDA_isElementSelected(aRequest) {
    if (this.context == "chrome") {
      try {
        
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        if (el.checked != undefined) {
          this.sendResponse(!!el.checked);
        }
        else if (el.selected != undefined) {
          this.sendResponse(!!el.selected);
        }
        else {
          this.sendResponse(true);
        }
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("isElementSelected", {element:aRequest.element});
    }
  },

  







  sendKeysToElement: function MDA_sendKeysToElement(aRequest) {
    if (this.context == "chrome") {
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        el.focus();
        utils.sendString(aRequest.value.join(""), utils.window);
        this.sendOk();
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("sendKeysToElement", {element:aRequest.element, value: aRequest.value});
    }
  },

  






  clearElement: function MDA_clearElement(aRequest) {
    if (this.context == "chrome") {
      
      try {
        let el = this.curBrowser.elementManager.getKnownElement(aRequest.element, this.getCurrentWindow());
        if (el.nodeName == "textbox") {
          el.value = "";
        }
        else if (el.nodeName == "checkbox") {
          el.checked = false;
        }
        this.sendOk();
      }
      catch (e) {
        this.sendError(e.message, e.code, e.stack);
      }
    }
    else {
      this.sendAsync("clearElement", {element:aRequest.element});
    }
  },

  







  closeWindow: function MDA_closeWindow() {
    if (appName == "B2G") {
      
      this.sendOk();
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
        this.messageManager.removeDelayedFrameScript("chrome://marionette/content/marionette-listener.js"); 
        this.getCurrentWindow().close();
        this.sendOk();
      }
      catch (e) {
        this.sendError("Could not close window: " + e.message, 13, e.stack);
      }
    }
  }, 

  








  deleteSession: function MDA_deleteSession() {
    if (this.curBrowser != null) {
      if (appName == "B2G") {
        this.messageManager.broadcastAsyncMessage("Marionette:sleepSession" + this.curBrowser.mainContentId, {});
        this.curBrowser.knownFrames.splice(this.curBrowser.knownFrames.indexOf(this.curBrowser.mainContentId), 1);
      }
      else {
        
        Services.prefs.setBoolPref("marionette.contentListener", false);
      }
      this.curBrowser.closeTab();
      
      for (let win in this.browsers) {
        for (let i in this.browsers[win].knownFrames) {
          this.messageManager.broadcastAsyncMessage("Marionette:deleteSession" + this.browsers[win].knownFrames[i], {});
        }
      }
      let winEnum = this.getWinEnumerator();
      while (winEnum.hasMoreElements()) {
        winEnum.getNext().messageManager.removeDelayedFrameScript("chrome://marionette/content/marionette-listener.js"); 
      }
    }
    this.sendOk();
    this.messageManager.removeMessageListener("Marionette:ok", this);
    this.messageManager.removeMessageListener("Marionette:done", this);
    this.messageManager.removeMessageListener("Marionette:error", this);
    this.messageManager.removeMessageListener("Marionette:log", this);
    this.messageManager.removeMessageListener("Marionette:shareData", this);
    this.messageManager.removeMessageListener("Marionette:register", this);
    this.messageManager.removeMessageListener("Marionette:goUrl", this);
    this.messageManager.removeMessageListener("Marionette:runEmulatorCmd", this);
    this.curBrowser = null;
    try {
      this.importedScripts.remove(false);
    }
    catch (e) {
    }
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
    this.sendToClient({emulator_cmd: cmd, id: this._emu_cb_id});
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
      this.sendError(e.message, e.code, e.stack);
      return;
    }
  },
  
  importScript: function MDA_importScript(aRequest) {
    if (this.context == "chrome") {
      let file;
      if (this.importedScripts.exists()) {
        file = FileUtils.openFileOutputStream(this.importedScripts, FileUtils.MODE_APPEND | FileUtils.MODE_WRONLY);
      }
      else {
        file = FileUtils.openFileOutputStream(this.importedScripts, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
      }
      file.write(aRequest.script, aRequest.script.length);
      file.close();
      this.sendOk();
    }
    else {
      this.sendAsync("importScript", {script: aRequest.script});
    }
  },

  


  receiveMessage: function MDA_receiveMessage(message) {
    switch (message.name) {
      case "DOMContentLoaded":
        this.sendOk();
        this.messageManager.removeMessageListener("DOMContentLoaded", this, true);
        break;
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
        this.sendToClient(message.json);
        break;
      case "Marionette:register":
        
        
        let nullPrevious = (this.curBrowser.curFrameId == null);
        let curWin = this.getCurrentWindow();
        let frameObject = curWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).getOuterWindowWithId(message.json.value);
        let browserType;
        try {
          browserType = message.target.getAttribute("type");
        } catch (ex) {
          
        }
        let reg;
        if (!browserType || browserType != "content") {
          reg = this.curBrowser.register(message.json.value, message.json.href); 
        }
        this.curBrowser.elementManager.seenItems[reg] = frameObject; 
        if (nullPrevious && (this.curBrowser.curFrameId != null)) {
          this.sendAsync("newSession", {B2G: (appName == "B2G")});
          if (this.curBrowser.newSession) {
            this.sendResponse(reg);
          }
        }
        return reg;
    }
  },
  


  handleEvent: function MDA_handleEvent(evt) {
    if (evt.type == "DOMContentLoaded") {
      this.sendOk();
      this.curBrowser.browser.removeEventListener("DOMContentLoaded", this, false);
    }
  },
};

MarionetteDriverActor.prototype.requestTypes = {
  "newSession": MarionetteDriverActor.prototype.newSession,
  "getSessionCapabilities": MarionetteDriverActor.prototype.getSessionCapabilities,
  "log": MarionetteDriverActor.prototype.log,
  "getLogs": MarionetteDriverActor.prototype.getLogs,
  "addPerfData": MarionetteDriverActor.prototype.addPerfData,
  "getPerfData": MarionetteDriverActor.prototype.getPerfData,
  "setContext": MarionetteDriverActor.prototype.setContext,
  "executeScript": MarionetteDriverActor.prototype.execute,
  "setScriptTimeout": MarionetteDriverActor.prototype.setScriptTimeout,
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
  "isElementEnabled": MarionetteDriverActor.prototype.isElementEnabled,
  "isElementSelected": MarionetteDriverActor.prototype.isElementSelected,
  "sendKeysToElement": MarionetteDriverActor.prototype.sendKeysToElement,
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
  "closeWindow": MarionetteDriverActor.prototype.closeWindow
};









function BrowserObj(win) {
  this.DESKTOP = "desktop";
  this.B2G = "B2G";
  this.browser;
  this.tab = null;
  this.knownFrames = [];
  this.curFrameId = null;
  this.startPage = "about:blank";
  this.mainContentId = null; 
  this.messageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                          .getService(Ci.nsIMessageBroadcaster);
  this.newSession = true; 
  this.elementManager = new ElementManager([SELECTOR, NAME, LINK_TEXT, PARTIAL_LINK_TEXT]);
  this.setBrowser(win);
}

BrowserObj.prototype = {
  





  setBrowser: function BO_setBrowser(win) {
    if (appName != "B2G") {
      this.browser = win.gBrowser; 
    }
  },
  










  startSession: function BO_startSession(newTab) {
    if (appName == "B2G") {
      return;
    }
    if (newTab) {
      this.addTab(this.startPage);
      
      this.browser.selectedTab = this.tab;
      let newTabBrowser = this.browser.getBrowserForTab(this.tab);
      
      newTabBrowser.ownerDocument.defaultView.focus();
    }
    else {
      
      if (this.browser != undefined && this.browser.selectedTab != undefined) {
        this.tab = this.browser.selectedTab;
      }
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

  









  register: function BO_register(id, href) {
    let uid = id + ((appName == "B2G") ? '-b2g' : '');
    if (this.curFrameId == null) {
      if ((!this.newSession) || (this.newSession && ((appName == "B2G") || href.indexOf(this.startPage) > -1))) {
        this.curFrameId = uid;
        this.mainContentId = uid;
      }
    }
    this.knownFrames.push(uid); 
    return uid;
  },
}
