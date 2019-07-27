



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
    .getService(Ci.mozIJSSubScriptLoader);

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
this.DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");

XPCOMUtils.defineLazyServiceGetter(
    this, "cookieManager", "@mozilla.org/cookiemanager;1", "nsICookieManager");

Cu.import("chrome://marionette/content/actions.js");
Cu.import("chrome://marionette/content/elements.js");
Cu.import("chrome://marionette/content/emulator.js");
Cu.import("chrome://marionette/content/error.js");
Cu.import("chrome://marionette/content/modal.js");
Cu.import("chrome://marionette/content/proxy.js");
Cu.import("chrome://marionette/content/simpletest.js");

loader.loadSubScript("chrome://marionette/content/common.js");


let utils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);
loader.loadSubScript("chrome://marionette/content/sendkeys.js", utils);
loader.loadSubScript("chrome://marionette/content/frame-manager.js");

this.EXPORTED_SYMBOLS = ["GeckoDriver", "Context"];

const FRAME_SCRIPT = "chrome://marionette/content/listener.js";
const BROWSER_STARTUP_FINISHED = "browser-delayed-startup-finished";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const SECURITY_PREF = "security.turn_off_all_security_so_that_viruses_can_take_over_this_computer";
const CLICK_TO_START_PREF = "marionette.debugging.clicktostart";
const CONTENT_LISTENER_PREF = "marionette.contentListener";

const logger = Log.repository.getLogger("Marionette");
const uuidGen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
const globalMessageManager = Cc["@mozilla.org/globalmessagemanager;1"]
    .getService(Ci.nsIMessageBroadcaster);
let specialpowers = {};





let systemMessageListenerReady = false;
Services.obs.addObserver(function() {
  systemMessageListenerReady = true;
}, "system-message-listener-ready", false);



let delayedBrowserStarted = false;
Services.obs.addObserver(function () {
  delayedBrowserStarted = true;
}, BROWSER_STARTUP_FINISHED, false);

this.Context = {
  CHROME: "chrome",
  CONTENT: "content",
};

this.Context.fromString = function(s) {
  s = s.toUpperCase();
  if (s in this) {
    return this[s];
  }
  return null;
};

















this.GeckoDriver = function(appName, device, emulator) {
  this.appName = appName;
  this.emulator = emulator;

  this.sessionId = null;
  
  this.browsers = {};
  
  this.curBrowser = null;
  this.context = Context.CONTENT;
  this.scriptTimeout = null;
  this.searchTimeout = null;
  this.pageTimeout = null;
  this.timer = null;
  this.inactivityTimer = null;
  
  this.heartbeatCallback = function() {};
  this.marionetteLog = new MarionetteLogObj();
  
  this.mainFrame = null;
  
  this.curFrame = null;
  this.mainContentFrameId = null;
  this.importedScripts = FileUtils.getFile("TmpD", ["marionetteChromeScripts"]);
  this.importedScriptHashes = {};
  this.importedScriptHashes[Context.CONTENT] = [];
  this.importedScriptHashes[Context.CHROME] = [];
  this.currentFrameElement = null;
  this.testName = null;
  this.mozBrowserClose = null;
  this.enabled_security_pref = false;
  this.sandbox = null;
  
  this.oopFrameId = null;
  this.observing = null;
  this._browserIds = new WeakMap();
  this.actions = new ActionChain(utils);

  this.sessionCapabilities = {
    
    "browserName": this.appName,
    "browserVersion": Services.appinfo.version,
    "platformName": Services.appinfo.OS.toUpperCase(),
    "platformVersion": Services.appinfo.platformVersion,

    
    "handlesAlerts": false,
    "nativeEvents": false,
    "raisesAccessibilityExceptions": false,
    "rotatable": this.appName == "B2G",
    "secureSsl": false,
    "takesElementScreenshot": true,
    "takesScreenshot": true,

    
    "platform": Services.appinfo.OS.toUpperCase(),

    
    "XULappId" : Services.appinfo.ID,
    "appBuildId" : Services.appinfo.appBuildID,
    "device": device,
    "version": Services.appinfo.version
  };

  this.mm = globalMessageManager;
  this.listener = proxy.toListener(() => this.mm, this.sendAsync.bind(this));

  this.dialog = null;
  let handleDialog = (subject, topic) => {
    let winr;
    if (topic == modal.COMMON_DIALOG_LOADED) {
      winr = Cu.getWeakReference(subject);
    }
    this.dialog = new modal.Dialog(() => this.curBrowser, winr);
  };
  modal.addHandler(handleDialog);
};

GeckoDriver.prototype.QueryInterface = XPCOMUtils.generateQI([
  Ci.nsIMessageListener,
  Ci.nsIObserver,
  Ci.nsISupportsWeakReference
]);









GeckoDriver.prototype.switchToGlobalMessageManager = function() {
  if (this.curBrowser && this.curBrowser.frameManager.currentRemoteFrame !== null) {
    this.curBrowser.frameManager.removeMessageManagerListeners(this.mm);
    this.sendAsync("sleepSession");
    this.curBrowser.frameManager.currentRemoteFrame = null;
  }
  this.mm = globalMessageManager;
};














GeckoDriver.prototype.sendAsync = function(name, msg, cmdId) {
  let curRemoteFrame = this.curBrowser.frameManager.currentRemoteFrame;
  name = "Marionette:" + name;

  if (cmdId) {
    msg.command_id = cmdId;
  }

  if (curRemoteFrame === null) {
    this.curBrowser.executeWhenReady(() => {
      this.mm.broadcastAsyncMessage(name + this.curBrowser.curFrameId, msg);
    });
  } else {
    let remoteFrameId = curRemoteFrame.targetFrameId;
    try {
      this.mm.sendAsyncMessage(name + remoteFrameId, msg);
    } catch (e) {
      switch(e.result) {
        case Components.results.NS_ERROR_FAILURE:
          throw new FrameSendFailureError(curRemoteFrame);
        case Components.results.NS_ERROR_NOT_INITIALIZED:
          throw new FrameSendNotInitializedError(curRemoteFrame);
        default:
          throw new WebDriverError(e.toString());
      }
    }
  }
};






GeckoDriver.prototype.getCurrentWindow = function() {
  let typ = null;
  if (this.curFrame === null) {
    if (this.curBrowser === null) {
      if (this.context == Context.CONTENT) {
        typ = 'navigator:browser';
      }
      return Services.wm.getMostRecentWindow(typ);
    } else {
      return this.curBrowser.window;
    }
  } else {
    return this.curFrame;
  }
};






GeckoDriver.prototype.getWinEnumerator = function() {
  let typ = null;
  if (this.appName != "B2G" && this.context == Context.CONTENT) {
    typ = "navigator:browser";
  }
  return Services.wm.getEnumerator(typ);
};

GeckoDriver.prototype.addFrameCloseListener = function(action) {
  let win = this.getCurrentWindow();
  this.mozBrowserClose = e => {
    if (e.target.id == this.oopFrameId) {
      win.removeEventListener("mozbrowserclose", this.mozBrowserClose, true);
      this.switchToGlobalMessageManager();
      throw new FrameSendFailureError(
          `The frame closed during the ${action}, recovering to allow further communications`);
    }
  };
  win.addEventListener("mozbrowserclose", this.mozBrowserClose, true);
};










GeckoDriver.prototype.addBrowser = function(win) {
  let browser = new BrowserObj(win, this);
  let winId = win.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  winId = winId + ((this.appName == "B2G") ? "-b2g" : "");
  this.browsers[winId] = browser;
  this.curBrowser = this.browsers[winId];
  if (typeof this.curBrowser.elementManager.seenItems[winId] == "undefined") {
    
    
    this.curBrowser.elementManager.seenItems[winId] = Cu.getWeakReference(win);
  }
};













GeckoDriver.prototype.startBrowser = function(win, isNewSession=false) {
  this.mainFrame = win;
  this.curFrame = null;
  this.addBrowser(win);
  this.curBrowser.isNewSession = isNewSession;
  this.curBrowser.startSession(isNewSession, win, this.whenBrowserStarted.bind(this));
};










GeckoDriver.prototype.whenBrowserStarted = function(win, isNewSession) {
  utils.window = win;

  try {
    let mm = win.window.messageManager;
    if (!isNewSession) {
      
      
      
      
      
      if (mm.childCount !== 0) {
        this.curBrowser.frameRegsPending = mm.childCount;
      }
    }

    if (!Services.prefs.getBoolPref("marionette.contentListener") || !isNewSession) {
      mm.loadFrameScript(FRAME_SCRIPT, true, true);
      Services.prefs.setBoolPref("marionette.contentListener", true);
    }
  } catch (e) {
    
    logger.error(
        `Could not load listener into content for page ${win.location.href}: ${e}`);
  }
};









GeckoDriver.prototype.getVisibleText = function(el, lines) {
  try {
    if (utils.isElementDisplayed(el)) {
      if (el.value) {
        lines.push(el.value);
      }
      for (let child in el.childNodes) {
        this.getVisibleText(el.childNodes[child], lines);
      }
    }
  } catch (e) {
    if (el.nodeName == "#text") {
      lines.push(el.textContent);
    }
  }
};







GeckoDriver.prototype.deleteFile = function(filename) {
  let file = FileUtils.getFile("TmpD", [filename.toString()]);
  if (file.exists()) {
    file.remove(true);
  }
};





GeckoDriver.prototype.registerBrowser = function(id, be) {
  let nullPrevious = this.curBrowser.curFrameId === null;
  let listenerWindow = Services.wm.getOuterWindowWithId(id);

  
  if (this.curBrowser.frameManager.currentRemoteFrame !== null &&
      (!listenerWindow || this.mm == this.curBrowser.frameManager
          .currentRemoteFrame.messageManager.get())) {
    
    
    
    
    
    
    
    
    
    
    
    this.curBrowser.frameManager.currentRemoteFrame.targetFrameId =
        this.generateFrameId(id);
  }

  let reg = {};
  
  let mainContent = this.curBrowser.mainContentId === null;
  if (be.getAttribute("type") != "content") {
    
    let uid = this.generateFrameId(id);
    reg.id = uid;
    reg.remotenessChange = this.curBrowser.register(uid, be);
  }

  
  mainContent = mainContent && this.curBrowser.mainContentId !== null;
  if (mainContent) {
    this.mainContentFrameId = this.curBrowser.curFrameId;
  }

  this.curBrowser.elementManager.seenItems[reg.id] =
      Cu.getWeakReference(listenerWindow);
  if (nullPrevious && (this.curBrowser.curFrameId !== null)) {
    this.sendAsync("newSession",
        {
          B2G: (this.appName == "B2G"),
          raisesAccessibilityExceptions:
              this.sessionCapabilities.raisesAccessibilityExceptions
        },
        this.newSessionCommandId);
    if (this.curBrowser.isNewSession) {
      this.newSessionCommandId = null;
    }
  }

  return [reg, mainContent];
};

GeckoDriver.prototype.registerPromise = function() {
  const li = "Marionette:register";

  return new Promise((resolve) => {
    this.mm.addMessageListener(li, function cb(msg) {
      let wid = msg.json.value;
      let be = msg.target;
      let rv = this.registerBrowser(wid, be);

      if (this.curBrowser.frameRegsPending > 0) {
        this.curBrowser.frameRegsPending--;
      }

      if (this.curBrowser.frameRegsPending === 0) {
        this.mm.removeMessageListener(li, cb);
        resolve();
      }

      
      return rv;
    }.bind(this));
  });
};

GeckoDriver.prototype.listeningPromise = function() {
  const li = "Marionette:listenersAttached";
  return new Promise((resolve) => {
    this.mm.addMessageListener(li, function() {
      this.mm.removeMessageListener(li, this);
      resolve();
    }.bind(this));
  });
};


GeckoDriver.prototype.newSession = function(cmd, resp) {
  this.sessionId = cmd.parameters.sessionId ||
      cmd.parameters.session_id ||
      uuidGen.generateUUID().toString();

  this.newSessionCommandId = cmd.id;
  this.setSessionCapabilities(cmd.parameters.capabilities);
  this.scriptTimeout = 10000;

  
  
  let sec = false;
  try {
    sec = Services.prefs.getBoolPref(SECURITY_PREF);
  } catch (e) {}
  if (!sec) {
    this.enabled_security_pref = true;
    Services.prefs.setBoolPref(SECURITY_PREF, true);
  }

  if (!specialpowers.hasOwnProperty("specialPowersObserver")) {
    loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js",
        specialpowers);
    specialpowers.specialPowersObserver = new specialpowers.SpecialPowersObserver();
    specialpowers.specialPowersObserver.init();
    specialpowers.specialPowersObserver._loadFrameScript();
  }

  let registerBrowsers = this.registerPromise();
  let browserListening = this.listeningPromise();

  let waitForWindow = function() {
    let win = this.getCurrentWindow();
    if (!win) {
      
      let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      checkTimer.initWithCallback(waitForWindow.bind(this), 100,
          Ci.nsITimer.TYPE_ONE_SHOT);
    } else if (win.document.readyState != "complete") {
      
      let listener = ev => {
        
        
        if (ev.target != win.document) {
          return;
        }
        win.removeEventListener("load", listener);
        waitForWindow.call(this);
      };
      win.addEventListener("load", listener, true);
    } else {
      let clickToStart;
      try {
        clickToStart = Services.prefs.getBoolPref(CLICK_TO_START_PREF);
      } catch (e) {}
      if (clickToStart && (this.appName != "B2G")) {
        let pService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
            .getService(Ci.nsIPromptService);
        pService.alert(win, "", "Click to start execution of marionette tests");
      }
      this.startBrowser(win, true);
    }
  };

  let runSessionStart = function() {
    if (!Services.prefs.getBoolPref(CONTENT_LISTENER_PREF)) {
      waitForWindow.call(this);
    } else if (this.appName != "Firefox" && this.curBrowser === null) {
      
      this.addBrowser(this.getCurrentWindow());
      this.curBrowser.startSession(this.whenBrowserStarted.bind(this));
      this.mm.broadcastAsyncMessage("Marionette:restart", {});
    } else {
      throw new WebDriverError("Session already running");
    }
    this.switchToGlobalMessageManager();
  };

  if (!delayedBrowserStarted && this.appName != "B2G") {
    let self = this;
    Services.obs.addObserver(function onStart() {
      Services.obs.removeObserver(onStart, BROWSER_STARTUP_FINISHED);
      runSessionStart.call(self);
    }, BROWSER_STARTUP_FINISHED, false);
  } else {
    runSessionStart.call(this);
  }

  yield registerBrowsers;
  yield browserListening;

  resp.sessionId = this.sessionId;
  resp.value = this.sessionCapabilities;
};












GeckoDriver.prototype.getSessionCapabilities = function(cmd, resp) {
  resp.value = this.sessionCapabilities;
};














GeckoDriver.prototype.setSessionCapabilities = function(newCaps) {
  const copy = (from, to={}) => {
    let errors = {};

    for (let key in from) {
      if (key === "desiredCapabilities") {
        
        
        to = copy(from[key], to);
      } else if (key === "requiredCapabilities") {
        for (let caps in from[key]) {
          if (from[key][caps] !== this.sessionCapabilities[caps]) {
            errors[caps] = from[key][caps] + " does not equal " +
                this.sessionCapabilities[caps];
          }
        }
      }
      to[key] = from[key];
    }

    if (Object.keys(errors).length === 0) {
      return to;
    }

    throw new SessionNotCreatedError(
        `Not all requiredCapabilities could be met: ${JSON.stringify(errors)}`);
  };

  
  let caps = copy(this.sessionCapabilities);
  caps = copy(newCaps, caps);
  this.sessionCapabilities = caps;
};









GeckoDriver.prototype.log = function(cmd, resp) {
  this.marionetteLog.log(cmd.parameters.value, cmd.parameters.level);
};


GeckoDriver.prototype.getLogs = function(cmd, resp) {
  resp.value = this.marionetteLog.getLogs();
};









GeckoDriver.prototype.setContext = function(cmd, resp) {
  let val = cmd.parameters.value;
  let ctx = Context.fromString(val);
  if (ctx === null) {
    throw new WebDriverError(`Invalid context: ${val}`);
  }
  this.context = ctx;
};


GeckoDriver.prototype.getContext = function(cmd, resp) {
  resp.value = this.context.toString();
};

















GeckoDriver.prototype.createExecuteSandbox = function(win, mn, sp) {
  let sb = new Cu.Sandbox(win,
      {sandboxPrototype: win, wantXrays: false, sandboxName: ""});
  sb.global = sb;
  sb.testUtils = utils;

  mn.exports.forEach(function(fn) {
    if (typeof mn[fn] === 'function') {
      sb[fn] = mn[fn].bind(mn);
    } else {
      sb[fn] = mn[fn];
    }
  });

  sb.isSystemMessageListenerReady = () => systemMessageListenerReady;

  if (sp) {
    let pow = [
      "chrome://specialpowers/content/specialpowersAPI.js",
      "chrome://specialpowers/content/SpecialPowersObserverAPI.js",
      "chrome://specialpowers/content/ChromePowers.js",
    ];
    pow.map(s => loader.loadSubScript(s, sb));
  }

  return sb;
};





GeckoDriver.prototype.applyArgumentsToSandbox = function(win, sb, args) {
  sb.__marionetteParams = this.curBrowser.elementManager.convertWrappedArguments(args, win);
  sb.__namedArgs = this.curBrowser.elementManager.applyNamedArgs(args);
};


















GeckoDriver.prototype.executeScriptInSandbox = function(
    resp,
    sandbox,
    script,
    directInject,
    async,
    timeout) {
  if (directInject && async && (timeout === null || timeout === 0)) {
    throw new TimeoutError("Please set a timeout");
  }

  if (this.importedScripts.exists()) {
    let stream = Cc["@mozilla.org/network/file-input-stream;1"]
        .createInstance(Ci.nsIFileInputStream);
    stream.init(this.importedScripts, -1, 0, 0);
    let data = NetUtil.readInputStreamToString(stream, stream.available());
    stream.close();
    script = data + script;
  }

  let res = Cu.evalInSandbox(script, sandbox, "1.8", "dummy file", 0);

  if (directInject && !async &&
      (typeof res == "undefined" || typeof res.passed == "undefined")) {
    throw new WebDriverError("finish() not called");
  }

  if (!async) {
    
    
    
    resp.value = this.curBrowser.elementManager.wrapValue(res);
  }
};








GeckoDriver.prototype.execute = function(cmd, resp, directInject) {
  let {inactivityTimeout,
       scriptTimeout,
       script,
       newSandbox,
       args,
       specialPowers,
       filename,
       line} = cmd.parameters;

  if (!scriptTimeout) {
    scriptTimeout = this.scriptTimeout;
  }
  if (typeof newSandbox == "undefined") {
    newSandbox = true;
  }

  if (this.context == Context.CONTENT) {
    resp.value = yield this.listener.executeScript({
      script: script,
      args: args,
      newSandbox: newSandbox,
      timeout: scriptTimeout,
      specialPowers: specialPowers,
      filename: filename,
      line: line
    });
    return;
  }

  
  let that = this;
  if (inactivityTimeout) {
    let setTimer = function() {
      that.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (that.inactivityTimer !== null) {
        that.inactivityTimer.initWithCallback(function() {
          throw new ScriptTimeoutError("timed out due to inactivity");
        }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }
    };
    setTimer();
    this.heartbeatCallback = function() {
      that.inactivityTimer.cancel();
      setTimer();
    };
  }

  let win = this.getCurrentWindow();
  if (!this.sandbox || newSandbox) {
    let marionette = new Marionette(
        this,
        win,
        "chrome",
        this.marionetteLog,
        scriptTimeout,
        this.heartbeatCallback,
        this.testName);
    this.sandbox = this.createExecuteSandbox(
        win,
        marionette,
        specialPowers);
    if (!this.sandbox) {
      return;
    }
  }
  this.applyArgumentsToSandbox(win, this.sandbox, args);

  try {
    this.sandbox.finish = () => {
      if (this.inactivityTimer !== null) {
        this.inactivityTimer.cancel();
      }
      return this.sandbox.generate_results();
    };

    if (!directInject) {
      script = "let func = function() { " + script + " }; func.apply(null, __marionetteParams);";
    }
    this.executeScriptInSandbox(
        resp,
        this.sandbox,
        script,
        directInject,
        false ,
        scriptTimeout);
  } catch (e) {
    throw new JavaScriptError(e, "execute_script", filename, line, script);
  }
};







GeckoDriver.prototype.setScriptTimeout = function(cmd, resp) {
  let ms = parseInt(cmd.parameters.ms);
  if (isNaN(ms)) {
    throw new WebDriverError("Not a Number");
  }
  this.scriptTimeout = ms;
};





GeckoDriver.prototype.executeJSScript = function(cmd, resp) {
  
  
  
  if (typeof cmd.newSandbox == "undefined") {
    
    
    cmd.newSandbox = true;
  }

  switch (this.context) {
    case Context.CHROME:
      if (cmd.parameters.async) {
        yield this.executeWithCallback(cmd, resp, cmd.parameters.async);
      } else {
        this.execute(cmd, resp, true );
      }
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.executeJSScript({
        script: cmd.parameters.script,
        args: cmd.parameters.args,
        newSandbox: cmd.parameters.newSandbox,
        async: cmd.parameters.async,
        timeout: cmd.parameters.scriptTimeout ?
            cmd.parameters.scriptTimeout : this.scriptTimeout,
        inactivityTimeout: cmd.parameters.inactivityTimeout,
        specialPowers: cmd.parameters.specialPowers,
        filename: cmd.parameters.filename,
        line: cmd.parameters.line,
      });
      break;
 }
};
















GeckoDriver.prototype.executeWithCallback = function(cmd, resp, directInject) {
  let {script,
      args,
      newSandbox,
      inactivityTimeout,
      scriptTimeout,
      specialPowers,
      filename,
      line} = cmd.parameters;

  if (!scriptTimeout) {
    scriptTimeout = this.scriptTimeout;
  }
  if (typeof newSandbox == "undefined") {
    newSandbox = true;
  }

  if (this.context == Context.CONTENT) {
    resp.value = yield this.listener.executeAsyncScript({
      script: script,
      args: args,
      id: cmd.id,
      newSandbox: newSandbox,
      timeout: scriptTimeout,
      inactivityTimeout: inactivityTimeout,
      specialPowers: specialPowers,
      filename: filename,
      line: line
    });
    return;
  }

  
  let that = this;
  if (inactivityTimeout) {
    this.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    if (this.inactivityTimer !== null) {
      this.inactivityTimer.initWithCallback(function() {
       chromeAsyncReturnFunc(new ScriptTimeoutError("timed out due to inactivity"));
      }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    this.heartbeatCallback = function resetInactivityTimer() {
      that.inactivityTimer.cancel();
      that.inactivityTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (that.inactivityTimer !== null) {
        that.inactivityTimer.initWithCallback(function() {
          chromeAsyncReturnFunc(new ScriptTimeoutError("timed out due to inactivity"));
        }, inactivityTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }
    };
  }

  let win = this.getCurrentWindow();
  let origOnError = win.onerror;
  that.timeout = scriptTimeout;

  let res = yield new Promise(function(resolve, reject) {
    let chromeAsyncReturnFunc = function(val) {
      if (that.emulator.cbs.length > 0) {
        that.emulator.cbs = [];
        throw new WebDriverError("Emulator callback still pending when finish() called");
      }

      if (cmd.id == that.sandbox.command_id) {
        if (that.timer !== null) {
          that.timer.cancel();
          that.timer = null;
        }

        win.onerror = origOnError;

        if (error.isError(val)) {
          reject(val);
        } else {
          resolve(val);
        }
      }

      if (that.inactivityTimer !== null) {
        that.inactivityTimer.cancel();
      }
    };

    let chromeAsyncFinish = function() {
      let res = that.sandbox.generate_results();
      chromeAsyncReturnFunc(res);
    };

    let chromeAsyncError = function(e, func, file, line, script) {
      let err = new JavaScriptError(e, func, file, line, script);
      chromeAsyncReturnFunc(err);
    };

    if (!this.sandbox || newSandbox) {
      let marionette = new Marionette(
          this,
          win,
          "chrome",
          this.marionetteLog,
          scriptTimeout,
          this.heartbeatCallback,
          this.testName);
      this.sandbox = this.createExecuteSandbox(win, marionette, specialPowers);
      if (!this.sandbox) {
        return;
      }
    }
    this.sandbox.command_id = cmd.id;
    this.sandbox.runEmulatorCmd = (cmd, cb) => {
      let ecb = new EmulatorCallback();
      ecb.onresult = cb;
      ecb.onerror = chromeAsyncError;
      this.emulator.pushCallback(ecb);
      this.emulator.send({emulator_cmd: cmd, id: ecb.id});
    };
    this.sandbox.runEmulatorShell = (args, cb) => {
      let ecb = new EmulatorCallback();
      ecb.onresult = cb;
      ecb.onerror = chromeAsyncError;
      this.emulator.pushCallback(ecb);
      this.emulator.send({emulator_shell: args, id: ecb.id});
    };
    this.applyArgumentsToSandbox(win, this.sandbox, args);

    
    
    
    
    if (cmd.parameters.debug_script) {
      win.onerror = function(msg, url, line) {
        let err = new JavaScriptError(`${msg} at: ${url} line: ${line}`);
        chromeAsyncReturnFunc(err);
        return true;
      };
    }

    try {
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      if (this.timer !== null) {
        this.timer.initWithCallback(function() {
          chromeAsyncReturnFunc(new ScriptTimeoutError("timed out"));
        }, that.timeout, Ci.nsITimer.TYPE_ONE_SHOT);
      }

      this.sandbox.returnFunc = chromeAsyncReturnFunc;
      this.sandbox.finish = chromeAsyncFinish;

      if (!directInject) {
        script =  "__marionetteParams.push(returnFunc);" +
            "let marionetteScriptFinished = returnFunc;" +
            "let __marionetteFunc = function() {" + script + "};" +
            "__marionetteFunc.apply(null, __marionetteParams);";
      }

      this.executeScriptInSandbox(
          resp,
          this.sandbox,
          script,
          directInject,
          true ,
          scriptTimeout);
    } catch (e) {
      chromeAsyncError(e, "execute_async_script", filename, line, script);
    }
  }.bind(this));

  resp.value = that.curBrowser.elementManager.wrapValue(res);
};



























GeckoDriver.prototype.get = function(cmd, resp) {
  let url = cmd.parameters.url;

  switch (this.context) {
    case Context.CONTENT:
      let get = this.listener.get({url: url, pageTimeout: this.pageTimeout});
      let id = this.listener.curId;

      
      
      
      this.curBrowser.pendingCommands.push(() => {
        cmd.parameters.command_id = id;
        this.mm.broadcastAsyncMessage(
            "Marionette:pollForReadyState" + this.curBrowser.curFrameId,
            cmd.parameters);
      });

      yield get;
      break;

    case Context.CHROME:
      
      
      
      
      
      if (this.appName == "Firefox") {
        throw new UnknownError("Cannot navigate in chrome context");
      }

      this.getCurrentWindow().location.href = url;
      yield this.pageLoadPromise();
      break;
  }
};

GeckoDriver.prototype.pageLoadPromise = function() {
  let win = this.getCurrentWindow();
  let timeout = this.pageTimeout;
  let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  let start = new Date().getTime();
  let end = null;

  return new Promise((resolve) => {
    let checkLoad = function() {
      end = new Date().getTime();
      let elapse = end - start;
      if (timeout === null || elapse <= timeout) {
        if (win.document.readyState == "complete") {
          resolve();
        } else {
          checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
        }
      } else {
        throw new UnknownError("Error loading page");
      }
    };
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  });
};











GeckoDriver.prototype.getCurrentUrl = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      resp.value = this.getCurrentWindow().location.href;
      break;

    case Context.CONTENT:
      let isB2G = this.appName == "B2G";
      resp.value = yield this.listener.getCurrentUrl({isB2G: isB2G});
      break;
  }
};


GeckoDriver.prototype.getTitle = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      resp.value = win.document.documentElement.getAttribute("title");
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getTitle();
      break;
  }
};


GeckoDriver.prototype.getWindowType = function(cmd, resp) {
  let win = this.getCurrentWindow();
  resp.value = win.document.documentElement.getAttribute("windowtype");
};


GeckoDriver.prototype.getPageSource = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let s = new win.XMLSerializer();
      resp.value = s.serializeToString(win.document);
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getPageSource();
      break;
  }
};


GeckoDriver.prototype.goBack = function(cmd, resp) {
  yield this.listener.goBack();
};


GeckoDriver.prototype.goForward = function(cmd, resp) {
  yield this.listener.goForward();
};


GeckoDriver.prototype.refresh = function(cmd, resp) {
  yield this.listener.refresh();
};












GeckoDriver.prototype.getWindowHandle = function(cmd, resp) {
  
  if (this.curBrowser.curFrameId && this.appName != "B2G") {
    resp.value = this.curBrowser.curFrameId;
    return;
  }

  for (let i in this.browsers) {
    if (this.curBrowser == this.browsers[i]) {
      resp.value = i;
      return;
    }
  }
};




GeckoDriver.prototype.updateIdForBrowser = function (browser, newId) {
  this._browserIds.set(browser.permanentKey, newId);
};






GeckoDriver.prototype.getIdForBrowser = function getIdForBrowser(browser) {
  if (browser === null) {
    return null;
  }
  let permKey = browser.permanentKey;
  if (this._browserIds.has(permKey)) {
    return this._browserIds.get(permKey);
  }

  let winId = browser.outerWindowID;
  if (winId) {
    winId += "";
    this._browserIds.set(permKey, winId);
    return winId;
  }
  return null;
},











GeckoDriver.prototype.getWindowHandles = function(cmd, resp) {
  let rv = [];
  let winEn = this.getWinEnumerator();
  while (winEn.hasMoreElements()) {
    let win = winEn.getNext();
    if (win.gBrowser && this.appName != "B2G") {
      let tabbrowser = win.gBrowser;
      for (let i = 0; i < tabbrowser.browsers.length; ++i) {
        let winId = this.getIdForBrowser(tabbrowser.getBrowserAtIndex(i));
        if (winId !== null) {
          rv.push(winId);
        }
      }
    } else {
      
      let winId = win.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIDOMWindowUtils)
          .outerWindowID;
      winId += (this.appName == "B2G") ? "-b2g" : "";
      rv.push(winId);
    }
  }
  resp.value = rv;
};












GeckoDriver.prototype.getChromeWindowHandle = function(cmd, resp) {
  for (let i in this.browsers) {
    if (this.curBrowser == this.browsers[i]) {
      resp.value = i;
      return;
    }
  }
};








GeckoDriver.prototype.getChromeWindowHandles = function(cmd, resp) {
  let rv = [];
  let winEn = this.getWinEnumerator();
  while (winEn.hasMoreElements()) {
    let foundWin = winEn.getNext();
    let winId = foundWin.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .outerWindowID;
    winId = winId + ((this.appName == "B2G") ? "-b2g" : "");
    rv.push(winId);
  }
  resp.value = rv;
};







GeckoDriver.prototype.getWindowPosition = function(cmd, resp) {
  let win = this.getCurrentWindow();
  resp.value = {x: win.screenX, y: win.screenY};
};











GeckoDriver.prototype.setWindowPosition = function(cmd, resp) {
  if (this.appName != "Firefox") {
    throw new WebDriverError("Unable to set the window position on mobile");
  }

  let x = parseInt(cmd.parameters.x);
  let y  = parseInt(cmd.parameters.y);
  if (isNaN(x) || isNaN(y)) {
    throw new UnknownError("x and y arguments should be integers");
  }

  let win = this.getCurrentWindow();
  win.moveTo(x, y);
};








GeckoDriver.prototype.switchToWindow = function(cmd, resp) {
  let switchTo = cmd.parameters.name;
  let isB2G = this.appName == "B2G";
  let found;

  let getOuterWindowId = function(win) {
    let rv = win.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .outerWindowID;
    rv += isB2G ? "-b2g" : "";
    return rv;
  };

  let byNameOrId = function(name, outerId, contentWindowId) {
    return switchTo == name ||
        switchTo == contentWindowId ||
        switchTo == outerId;
  };

  let winEn = this.getWinEnumerator();
  while (winEn.hasMoreElements()) {
    let win = winEn.getNext();
    let outerId = getOuterWindowId(win);

    if (win.gBrowser && !isB2G) {
      let tabbrowser = win.gBrowser;
      for (let i = 0; i < tabbrowser.browsers.length; ++i) {
        let browser = tabbrowser.getBrowserAtIndex(i);
        let contentWindowId = this.getIdForBrowser(browser);
        if (byNameOrId(win.name, contentWindowId, outerId)) {
          found = {
            win: win,
            outerId: outerId,
            tabIndex: i,
            contentId: contentWindowId
          };
          break;
        }
      }
    } else {
      if (byNameOrId(win.name, outerId)) {
        found = {win: win, outerId: outerId};
        break;
      }
    }
  }

  if (found) {
    
    
    this.sandbox = null;

    
    
    
    if (!(found.outerId in this.browsers)) {
      let registerBrowsers, browserListening;
      if (found.contentId) {
        registerBrowsers = this.registerPromise();
        browserListening = this.listeningPromise();
      }

      this.startBrowser(found.win, false );

      if (registerBrowsers && browserListening) {
        yield registerBrowsers;
        yield browserListening;
      }
    } else {
      utils.window = found.win;
      this.curBrowser = this.browsers[found.outerId];

      if (found.contentId) {
        this.curBrowser.switchToTab(found.tabIndex);
      }
    }
  } else {
    throw new NoSuchWindowError(`Unable to locate window: ${switchTo}`);
  }
};

GeckoDriver.prototype.getActiveFrame = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      
      resp.value = null;
      if (this.curFrame) {
        resp.value = this.curBrowser.elementManager
            .addToKnownElements(this.curFrame.frameElement);
      }
      break;

    case Context.CONTENT:
      resp.value = this.currentFrameElement;
      break;
  }
};










GeckoDriver.prototype.switchToFrame = function(cmd, resp) {
  let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  let curWindow = this.getCurrentWindow();

  let checkLoad = function() {
    let errorRegex = /about:.+(error)|(blocked)\?/;
    let curWindow = this.getCurrentWindow();
    if (curWindow.document.readyState == "complete") {
      return;
    } else if (curWindow.document.readyState == "interactive" &&
        errorRegex.exec(curWindow.document.baseURI)) {
      throw new UnknownError("Error loading page");
    }

    checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
  };

  if (this.context == Context.CHROME) {
    let foundFrame = null;

    
    
    if (cmd.parameters.id == null && !cmd.parameters.hasOwnProperty("element")) {
      this.curFrame = null;
      if (cmd.parameters.focus) {
        this.mainFrame.focus();
      }
      checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
      return;
    }

    if (typeof cmd.parameters.element != "undefined") {
      if (this.curBrowser.elementManager.seenItems[cmd.parameters.element]) {
        
        let wantedFrame = this.curBrowser.elementManager
            .getKnownElement(cmd.parameters.element, curWindow);
        
        if (wantedFrame.tagName == "xul:browser" || wantedFrame.tagName == "browser") {
          curWindow = wantedFrame.contentWindow;
          this.curFrame = curWindow;
          if (cmd.parameters.focus) {
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
            if (cmd.parameters.focus) {
              this.curFrame.focus();
            }
            checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
            return;
        }
      }
    }
  }
  switch(typeof(cmd.parameters.id)) {
    case "string" :
      let foundById = null;
      let frames = curWindow.document.getElementsByTagName("iframe");
      let numFrames = frames.length;
      for (let i = 0; i < numFrames; i++) {
        
        let frame = frames[i];
        if (frame.getAttribute("name") == cmd.parameters.id) {
          foundFrame = i;
          curWindow = frame.contentWindow;
          break;
        } else if ((foundById === null) && (frame.id == cmd.parameters.id)) {
          foundById = i;
        }
      }
      if ((foundFrame === null) && (foundById !== null)) {
        foundFrame = foundById;
        curWindow = frames[foundById].contentWindow;
      }
      break;
    case "number":
      if (typeof curWindow.frames[cmd.parameters.id] != "undefined") {
        foundFrame = cmd.parameters.id;
        curWindow = curWindow.frames[foundFrame].frameElement.contentWindow;
      }
      break;
    }
    if (foundFrame !== null) {
      this.curFrame = curWindow;
      if (cmd.parameters.focus) {
        this.curFrame.focus();
      }
      checkTimer.initWithCallback(checkLoad.bind(this), 100, Ci.nsITimer.TYPE_ONE_SHOT);
    } else {
      throw new NoSuchFrameError(
          `Unable to locate frame: ${cmd.parameters.id}`);
    }
  }
  else {
    if ((!cmd.parameters.id) && (!cmd.parameters.element) &&
        (this.curBrowser.frameManager.currentRemoteFrame !== null)) {
      
      
      
      
      this.switchToGlobalMessageManager();
    }
    cmd.command_id = cmd.id;

    let res = yield this.listener.switchToFrame(cmd.parameters);
    if (res) {
      let {win: winId, frame: frameId} = res;
      this.mm = this.curBrowser.frameManager.getFrameMM(winId, frameId);

      let registerBrowsers = this.registerPromise();
      let browserListening = this.listeningPromise();

      this.oopFrameId =
          this.curBrowser.frameManager.switchToFrame(winId, frameId);

      yield registerBrowsers;
      yield browserListening;
    }
  }
};







GeckoDriver.prototype.setSearchTimeout = function(cmd, resp) {
  let ms = parseInt(cmd.parameters.ms);
  if (isNaN(ms)) {
    throw new WebDriverError("Not a Number");
  }
  this.searchTimeout = ms;
};









GeckoDriver.prototype.timeouts = function(cmd, resp) {
  let typ = cmd.parameters.type;
  let ms = parseInt(cmd.parameters.ms);
  if (isNaN(ms)) {
    throw new WebDriverError("Not a Number");
  }

  switch (typ) {
    case "implicit":
      this.setSearchTimeout(cmd, resp);
      break;

    case "script":
      this.setScriptTimeout(cmd, resp);
      break;

    default:
      this.pageTimeout = ms;
      break;
  }
};


GeckoDriver.prototype.singleTap = function(cmd, resp) {
  let {id, x, y} = cmd.parameters;

  switch (this.context) {
    case Context.CHROME:
      throw new WebDriverError("Command 'singleTap' is not available in chrome context");

    case Context.CONTENT:
      this.addFrameCloseListener("tap");
      yield this.listener.singleTap({id: id, corx: x, cory: y});
      break;
  }
};











GeckoDriver.prototype.actionChain = function(cmd, resp) {
  let {chain, nextId} = cmd.parameters;

  switch (this.context) {
    case Context.CHROME:
      if (this.appName != "Firefox") {
        
        
        throw new WebDriverError(
            "Command 'actionChain' is not available in chrome context");
      }

      let cbs = {};
      cbs.onSuccess = val => resp.value = val;
      cbs.onError = err => { throw err; };

      let win = this.getCurrentWindow();
      let elm = this.curBrowser.elementManager;
      this.actions.dispatchActions(chain, nextId, win, elm, cbs);
      break;

    case Context.CONTENT:
      this.addFrameCloseListener("action chain");
      resp.value = yield this.listener.actionChain({chain: chain, nextId: nextId});
      break;
  }
};









GeckoDriver.prototype.multiAction = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      throw new WebDriverError("Command 'multiAction' is not available in chrome context");

    case Context.CONTENT:
      this.addFrameCloseListener("multi action chain");
      yield this.listener.multiAction(
          {value: cmd.parameters.value, maxlen: cmd.parameters.max_len});
      break;
  }
};









GeckoDriver.prototype.findElement = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      resp.value = yield new Promise((resolve, reject) => {
        let win = this.getCurrentWindow();
        this.curBrowser.elementManager.find(
            win,
            cmd.parameters,
            this.searchTimeout,
            false ,
            resolve,
            reject);
      }).then(null, e => { throw e; });
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.findElementContent({
        value: cmd.parameters.value,
        using: cmd.parameters.using,
        element: cmd.parameters.element,
        searchTimeout: this.searchTimeout});
      break;
  }
};












GeckoDriver.prototype.findChildElement = function(cmd, resp) {
  resp.value = yield this.listener.findElementContent({
    value: cmd.parameters.value,
    using: cmd.parameters.using,
    element: cmd.parameters.id,
    searchTimeout: this.searchTimeout});
};









GeckoDriver.prototype.findElements = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      resp.value = yield new Promise((resolve, reject) => {
        let win = this.getCurrentWindow();
        this.curBrowser.elementManager.find(
            win,
            cmd.parameters,
            this.searchTimeout,
            true ,
            resolve,
            reject);
      }).then(null, e => { throw new NoSuchElementError(e.message); });
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.findElementsContent({
        value: cmd.parameters.value,
        using: cmd.parameters.using,
        element: cmd.parameters.element,
        searchTimeout: this.searchTimeout});
      break;
  }
};












GeckoDriver.prototype.findChildElements = function(cmd, resp) {
  resp.value = yield this.listener.findElementsContent({
    value: cmd.parameters.value,
    using: cmd.parameters.using,
    element: cmd.parameters.id,
    searchTimeout: this.searchTimeout});
};


GeckoDriver.prototype.getActiveElement = function(cmd, resp) {
  resp.value = yield this.listener.getActiveElement();
};







GeckoDriver.prototype.clickElement = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      el.click();
      break;

    case Context.CONTENT:
      
      
      
      
      this.addFrameCloseListener("click");
      yield this.listener.clickElement(id);
      break;
  }
};









GeckoDriver.prototype.getElementAttribute = function(cmd, resp) {
  let {id, name} = cmd.parameters;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      resp.value = utils.getElementAttribute(el, name);
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementAttribute(id, name);
      break;
  }
};








GeckoDriver.prototype.getElementText = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      let lines = [];
      this.getVisibleText(el, lines);
      resp.value = lines.join("\n");
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementText(id);
      break;
  }
};







GeckoDriver.prototype.getElementTagName = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      resp.value = el.tagName.toLowerCase();
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementTagName(id);
      break;
  }
};







GeckoDriver.prototype.isElementDisplayed = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      resp.value = utils.isElementDisplayed(el);
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.isElementDisplayed({id: id});
      break;
  }
};









GeckoDriver.prototype.getElementValueOfCssProperty = function(cmd, resp) {
  let {id, propertyName: prop} = cmd.parameters;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      let sty = win.document.defaultView.getComputedStyle(el, null);
      resp.value = sty.getPropertyValue(prop);
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementValueOfCssProperty(
          {id: id, propertyName: prop});
      break;
  }
};








GeckoDriver.prototype.submitElement = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      throw new WebDriverError(
          "Command 'submitElement' is not available in chrome context");

    case Context.CONTENT:
      yield this.listener.submitElement({id: cmd.parameters.id});
      break;
  }
};







GeckoDriver.prototype.isElementEnabled = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      resp.value = !(!!el.disabled);
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.isElementEnabled(id);
      break;
  }
},







GeckoDriver.prototype.isElementSelected = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      if (typeof el.checked != "undefined") {
        resp.value = !!el.checked;
      } else if (typeof el.selected != "undefined") {
        resp.value = !!el.selected;
      } else {
        resp.value = true;
      }
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.isElementSelected({id: id});
      break;
  }
};

GeckoDriver.prototype.getElementSize = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      let rect = el.getBoundingClientRect();
      resp.value = {width: rect.width, height: rect.height};
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementSize(id);
      break;
  }
};

GeckoDriver.prototype.getElementRect = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      let rect = el.getBoundingClientRect();
      resp.value = {
        x: rect.x + win.pageXOffset,
        y: rect.y + win.pageYOffset,
        width: rect.width,
        height: rect.height
      };
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.getElementRect(id);
      break;
  }
};









GeckoDriver.prototype.sendKeysToElement = function(cmd, resp) {
  let {id, value} = cmd.parameters;

  if (!value) {
    throw new InvalidArgumentError(`Expected character sequence: ${value}`);
  }

  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      utils.sendKeysToElement(
          win,
          el,
          value,
          () => {},
          e => { throw e; },
          cmd.id,
          true );
      break;

    case Context.CONTENT:
      let err;
      let listener = function(msg) {
        this.mm.removeMessageListener("Marionette:setElementValue", listener);

        let val = msg.data.value;
        let el = msg.objects.element;
        let win = this.getCurrentWindow();

        if (el.type == "file") {
          Cu.importGlobalProperties(["File"]);
          let fs = Array.prototype.slice.call(el.files);
          let file;
          try {
            file = new File(val);
          } catch (e) {
            err = new InvalidArgumentError(`File not found: ${val}`);
          }
          fs.push(file);
          el.mozSetFileArray(fs);
        } else {
          el.value = val;
        }
      }.bind(this);

      this.mm.addMessageListener("Marionette:setElementValue", listener);
      yield this.listener.sendKeysToElement({id: id, value: value});
      this.mm.removeMessageListener("Marionette:setElementValue", listener);
      if (err) {
        throw err;
      }
      break;
  }
};


GeckoDriver.prototype.setTestName = function(cmd, resp) {
  let val = cmd.parameters.value;
  this.testName = val;
  yield this.listener.setTestName({value: val});
};







GeckoDriver.prototype.clearElement = function(cmd, resp) {
  let id = cmd.parameters.id;

  switch (this.context) {
    case Context.CHROME:
      
      let win = this.getCurrentWindow();
      let el = this.curBrowser.elementManager.getKnownElement(id, win);
      if (el.nodeName == "textbox") {
        el.value = "";
      } else if (el.nodeName == "checkbox") {
        el.checked = false;
      }
      break;

    case Context.CONTENT:
      yield this.listener.clearElement({id: id});
      break;
  }
};











GeckoDriver.prototype.getElementLocation = function(cmd, resp) {
  resp.value = yield this.listener.getElementLocation(
      {id: cmd.parameters.id});
};


GeckoDriver.prototype.addCookie = function(cmd, resp) {
  yield this.listener.addCookie({cookie: cmd.parameters.cookie});
};







GeckoDriver.prototype.getCookies = function(cmd, resp) {
  resp.value = yield this.listener.getCookies();
};


GeckoDriver.prototype.deleteAllCookies = function(cmd, resp) {
  yield this.listener.deleteAllCookies();
};


GeckoDriver.prototype.deleteCookie = function(cmd, resp) {
  yield this.listener.deleteCookie({name: cmd.parameters.name});
};







GeckoDriver.prototype.close = function(cmd, resp) {
  
  if (this.appName == "B2G") {
    return;
  }

  let nwins = 0;
  let winEn = this.getWinEnumerator();
  while (winEn.hasMoreElements()) {
    let win = winEn.getNext();

    
    if (win.gBrowser) {
      nwins += win.gBrowser.browsers.length;
    } else {
      nwins++;
    }
  }

  
  if (nwins == 1) {
    this.sessionTearDown();
    return;
  }

  try {
    if (this.mm != globalMessageManager) {
      this.mm.removeDelayedFrameScript(FRAME_SCRIPT);
    }

    if (this.curBrowser.tab) {
      this.curBrowser.closeTab();
    } else {
      this.getCurrentWindow().close();
    }
  } catch (e) {
    throw new UnknownError(`Could not close window: ${e.message}`);
  }
};







GeckoDriver.prototype.closeChromeWindow = function(cmd, resp) {
  
  if (this.appName == "B2G") {
    return;
  }

  
  let nwins = 0;
  let winEn = this.getWinEnumerator();
  while (winEn.hasMoreElements()) {
    nwins++;
    winEn.getNext();
  }

  
  if (nwins == 1) {
    this.sessionTearDown();
    return;
  }

  try {
    this.mm.removeDelayedFrameScript(FRAME_SCRIPT);
    this.getCurrentWindow().close();
  } catch (e) {
    throw new UnknownError(`Could not close window: ${e.message}`);
  }
};











GeckoDriver.prototype.sessionTearDown = function(cmd, resp) {
  if (this.curBrowser !== null) {
    if (this.appName == "B2G") {
      globalMessageManager.broadcastAsyncMessage(
          "Marionette:sleepSession" + this.curBrowser.mainContentId, {});
      this.curBrowser.knownFrames.splice(
          this.curBrowser.knownFrames.indexOf(this.curBrowser.mainContentId), 1);
    } else {
      
      Services.prefs.setBoolPref("marionette.contentListener", false);
    }

    
    for (let win in this.browsers) {
      let browser = this.browsers[win];
      for (let i in browser.knownFrames) {
        globalMessageManager.broadcastAsyncMessage(
            "Marionette:deleteSession" + browser.knownFrames[i], {});
      }
    }

    let winEn = this.getWinEnumerator();
    while (winEn.hasMoreElements()) {
      winEn.getNext().messageManager.removeDelayedFrameScript(FRAME_SCRIPT);
    }

    this.curBrowser.frameManager.removeSpecialPowers();
    this.curBrowser.frameManager.removeMessageManagerListeners(
        globalMessageManager);
  }

  this.switchToGlobalMessageManager();

  
  this.curFrame = null;
  if (this.mainFrame) {
    this.mainFrame.focus();
  }

  this.sessionId = null;
  this.deleteFile("marionetteChromeScripts");
  this.deleteFile("marionetteContentScripts");

  if (this.observing !== null) {
    for (let topic in this.observing) {
      Services.obs.removeObserver(this.observing[topic], topic);
    }
    this.observing = null;
  }
};





GeckoDriver.prototype.deleteSession = function(cmd, resp) {
  this.sessionTearDown();
};


GeckoDriver.prototype.getAppCacheStatus = function(cmd, resp) {
  resp.value = yield this.listener.getAppCacheStatus();
};

GeckoDriver.prototype.importScript = function(cmd, resp) {
  let script = cmd.parameters.script;

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let result = {};
  let data = converter.convertToByteArray(cmd.parameters.script, result);
  let ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  ch.init(ch.MD5);
  ch.update(data, data.length);
  let hash = ch.finish(true);
  
  if (this.importedScriptHashes[this.context].indexOf(hash) > -1) {
    return;
  }
  this.importedScriptHashes[this.context].push(hash);

  switch (this.context) {
    case Context.CHROME:
      let file;
      if (this.importedScripts.exists()) {
        file = FileUtils.openFileOutputStream(this.importedScripts,
            FileUtils.MODE_APPEND | FileUtils.MODE_WRONLY);
      } else {
        
        this.importedScripts.createUnique(
            Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0666", 8));
        file = FileUtils.openFileOutputStream(this.importedScripts,
            FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
        this.importedScripts.permissions = parseInt("0666", 8);
      }
      file.write(script, script.length);
      file.close();
      break;

    case Context.CONTENT:
      yield this.listener.importScript({script: script});
      break;
  }
};

GeckoDriver.prototype.clearImportedScripts = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      this.deleteFile("marionetteChromeScripts");
      break;

    case Context.CONTENT:
      this.deleteFile("marionetteContentScripts");
      break;
  }
};























GeckoDriver.prototype.takeScreenshot = function(cmd, resp) {
  switch (this.context) {
    case Context.CHROME:
      let win = this.getCurrentWindow();
      let canvas = win.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
      let doc;
      if (this.appName == "B2G") {
        doc = win.document.body;
      } else {
        doc = win.document.getElementsByTagName("window")[0];
      }
      let docRect = doc.getBoundingClientRect();
      let width = docRect.width;
      let height = docRect.height;

      
      
      let scale = win.devicePixelRatio;
      canvas.setAttribute("width", Math.round(width * scale));
      canvas.setAttribute("height", Math.round(height * scale));

      let context = canvas.getContext("2d");
      let flags;
      if (this.appName == "B2G") {
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
      let dataUrl = canvas.toDataURL("image/png", "");
      let data = dataUrl.substring(dataUrl.indexOf(",") + 1);
      resp.value = data;
      break;

    case Context.CONTENT:
      resp.value = yield this.listener.takeScreenshot({
        id: cmd.parameters.id,
        highlights: cmd.parameters.highlights,
        full: cmd.parameters.full});
      break;
  }
};








GeckoDriver.prototype.getScreenOrientation = function(cmd, resp) {
  resp.value = this.getCurrentWindow().screen.mozOrientation;
};












GeckoDriver.prototype.setScreenOrientation = function(cmd, resp) {
  const ors = [
    "portrait", "landscape",
    "portrait-primary", "landscape-primary",
    "portrait-secondary", "landscape-secondary"
  ];

  let or = String(cmd.parameters.orientation);
  let mozOr = or.toLowerCase();
  if (ors.indexOf(mozOr) < 0) {
    throw new WebDriverError(`Unknown screen orientation: ${or}`);
  }

  let win = this.getCurrentWindow();
  if (!win.screen.mozLockOrientation(mozOr)) {
    throw new WebDriverError(`Unable to set screen orientation: ${or}`);
  }
};








GeckoDriver.prototype.getWindowSize = function(cmd, resp) {
  let win = this.getCurrentWindow();
  resp.value = {width: win.outerWidth, height: win.outerHeight};
};











GeckoDriver.prototype.setWindowSize = function(cmd, resp) {
  if (this.appName !== "Firefox") {
    throw new UnsupportedOperationError("Not supported on mobile");
  }

  let width = parseInt(cmd.parameters.width);
  let height = parseInt(cmd.parameters.height);

  let win = this.getCurrentWindow();
  if (width >= win.screen.availWidth && height >= win.screen.availHeight) {
    throw new UnsupportedOperationError("Invalid requested size, cannot maximize");
  }

  win.resizeTo(width, height);
};







GeckoDriver.prototype.maximizeWindow = function(cmd, resp) {
  if (this.appName != "Firefox") {
    throw new UnsupportedOperationError("Not supported for mobile");
  }

  let win = this.getCurrentWindow();
  win.moveTo(0,0);
  win.resizeTo(win.screen.availWidth, win.screen.availHeight);
};





GeckoDriver.prototype.dismissDialog = function(cmd, resp) {
  if (!this.dialog) {
    throw new NoAlertOpenError(
        "No tab modal was open when attempting to dismiss the dialog");
  }

  let {button0, button1} = this.dialog.ui;
  (button1 ? button1 : button0).click();
  this.dialog = null;
};





GeckoDriver.prototype.acceptDialog = function(cmd, resp) {
  if (!this.dialog) {
    throw new NoAlertOpenError(
        "No tab modal was open when attempting to accept the dialog");
  }

  let {button0} = this.dialog.ui;
  button0.click();
  this.dialog = null;
};





GeckoDriver.prototype.getTextFromDialog = function(cmd, resp) {
  if (!this.dialog) {
    throw new NoAlertOpenError(
        "No tab modal was open when attempting to get the dialog text");
  }

  let {infoBody} = this.dialog.ui;
  resp.value = infoBody.textContent;
};







GeckoDriver.prototype.sendKeysToDialog = function(cmd, resp) {
  if (!this.dialog) {
    throw new NoAlertOpenError(
        "No tab modal was open when attempting to send keys to a dialog");
  }

  
  let {loginContainer, loginTextbox} = this.dialog.ui;
  if (loginContainer.hidden) {
    throw new ElementNotVisibleError("This prompt does not accept text input");
  }

  let win = this.dialog.window ? this.dialog.window : this.getCurrentWindow();
  utils.sendKeysToElement(
      win,
      loginTextbox,
      cmd.parameters.value,
      () => {},
      e => { throw e; },
      this.command_id,
      true );
};





GeckoDriver.prototype.generateFrameId = function(id) {
  let uid = id + (this.appName == "B2G" ? "-b2g" : "");
  return uid;
};


GeckoDriver.prototype.receiveMessage = function(message) {
  
  if (this.mozBrowserClose !== null) {
    let win = this.getCurrentWindow();
    win.removeEventListener("mozbrowserclose", this.mozBrowserClose, true);
    this.mozBrowserClose = null;
  }

  switch (message.name) {
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
      this.emulator.send(message.json);
      break;

    case "Marionette:switchToModalOrigin":
      this.curBrowser.frameManager.switchToModalOrigin(message);
      this.mm = this.curBrowser.frameManager
          .currentRemoteFrame.messageManager.get();
      break;

    case "Marionette:switchedToFrame":
      if (message.json.restorePrevious) {
        this.currentFrameElement = this.previousFrameElement;
      } else {
        
        
        
        if (message.json.storePrevious) {
          this.previousFrameElement = this.currentFrameElement;
        }
        this.currentFrameElement = message.json.frameValue;
      }
      break;

    case "Marionette:getVisibleCookies":
      let [currentPath, host] = message.json.value;
      let isForCurrentPath = path => currentPath.indexOf(path) != -1;
      let results = [];

      let en = cookieManager.enumerator;
      while (en.hasMoreElements()) {
        let cookie = en.getNext().QueryInterface(Ci.nsICookie);
        
        let hostname = host;
        do {
          if ((cookie.host == "." + hostname || cookie.host == hostname) &&
              isForCurrentPath(cookie.path)) {
            results.push({
              "name": cookie.name,
              "value": cookie.value,
              "path": cookie.path,
              "host": cookie.host,
              "secure": cookie.isSecure,
              "expiry": cookie.expires
            });
            break;
          }
          hostname = hostname.replace(/^.*?\./, "");
        } while (hostname.indexOf(".") != -1);
      }
      return results;

    case "Marionette:addCookie":
      let cookieToAdd = message.json.value;
      Services.cookies.add(
          cookieToAdd.domain,
          cookieToAdd.path,
          cookieToAdd.name,
          cookieToAdd.value,
          cookieToAdd.secure,
          false,
          false,
          cookieToAdd.expiry);
      return true;

    case "Marionette:deleteCookie":
      let cookieToDelete = message.json.value;
      cookieManager.remove(
          cookieToDelete.host,
          cookieToDelete.name,
          cookieToDelete.path,
          false);
      return true;

    case "Marionette:emitTouchEvent":
      globalMessageManager.broadcastAsyncMessage(
          "MarionetteMainListener:emitTouchEvent", message.json);
      break;

    case "Marionette:register":
      let wid = message.json.value;
      let be = message.target;
      let rv = this.registerBrowser(wid, be);
      return rv;

    case "Marionette:listenersAttached":
      if (message.json.listenerId === this.curBrowser.curFrameId) {
        
        
        
        let newSessionValues = {
          B2G: (this.appName == "B2G"),
          raisesAccessibilityExceptions:
              this.sessionCapabilities.raisesAccessibilityExceptions
        };
        this.sendAsync("newSession", newSessionValues);
        this.curBrowser.flushPendingCommands();
      }
      break;
  }
};

GeckoDriver.prototype.responseCompleted = function () {
  if (this.curBrowser !== null) {
    this.curBrowser.pendingCommands = [];
  }
};

GeckoDriver.prototype.commands = {
  "getMarionetteID": GeckoDriver.prototype.getMarionetteID,
  "sayHello": GeckoDriver.prototype.sayHello,
  "newSession": GeckoDriver.prototype.newSession,
  "getSessionCapabilities": GeckoDriver.prototype.getSessionCapabilities,
  "log": GeckoDriver.prototype.log,
  "getLogs": GeckoDriver.prototype.getLogs,
  "setContext": GeckoDriver.prototype.setContext,
  "getContext": GeckoDriver.prototype.getContext,
  "executeScript": GeckoDriver.prototype.execute,
  "setScriptTimeout": GeckoDriver.prototype.setScriptTimeout,
  "timeouts": GeckoDriver.prototype.timeouts,
  "singleTap": GeckoDriver.prototype.singleTap,
  "actionChain": GeckoDriver.prototype.actionChain,
  "multiAction": GeckoDriver.prototype.multiAction,
  "executeAsyncScript": GeckoDriver.prototype.executeWithCallback,
  "executeJSScript": GeckoDriver.prototype.executeJSScript,
  "setSearchTimeout": GeckoDriver.prototype.setSearchTimeout,
  "findElement": GeckoDriver.prototype.findElement,
  "findChildElement": GeckoDriver.prototype.findChildElements, 
  "findElements": GeckoDriver.prototype.findElements,
  "findChildElements":GeckoDriver.prototype.findChildElements, 
  "clickElement": GeckoDriver.prototype.clickElement,
  "getElementAttribute": GeckoDriver.prototype.getElementAttribute,
  "getElementText": GeckoDriver.prototype.getElementText,
  "getElementTagName": GeckoDriver.prototype.getElementTagName,
  "isElementDisplayed": GeckoDriver.prototype.isElementDisplayed,
  "getElementValueOfCssProperty": GeckoDriver.prototype.getElementValueOfCssProperty,
  "submitElement": GeckoDriver.prototype.submitElement,
  "getElementSize": GeckoDriver.prototype.getElementSize,  
  "getElementRect": GeckoDriver.prototype.getElementRect,
  "isElementEnabled": GeckoDriver.prototype.isElementEnabled,
  "isElementSelected": GeckoDriver.prototype.isElementSelected,
  "sendKeysToElement": GeckoDriver.prototype.sendKeysToElement,
  "getElementLocation": GeckoDriver.prototype.getElementLocation,  
  "getElementPosition": GeckoDriver.prototype.getElementLocation,  
  "clearElement": GeckoDriver.prototype.clearElement,
  "getTitle": GeckoDriver.prototype.getTitle,
  "getWindowType": GeckoDriver.prototype.getWindowType,
  "getPageSource": GeckoDriver.prototype.getPageSource,
  "get": GeckoDriver.prototype.get,
  "goUrl": GeckoDriver.prototype.get,  
  "getCurrentUrl": GeckoDriver.prototype.getCurrentUrl,
  "getUrl": GeckoDriver.prototype.getCurrentUrl,  
  "goBack": GeckoDriver.prototype.goBack,
  "goForward": GeckoDriver.prototype.goForward,
  "refresh":  GeckoDriver.prototype.refresh,
  "getWindowHandle": GeckoDriver.prototype.getWindowHandle,
  "getCurrentWindowHandle":  GeckoDriver.prototype.getWindowHandle,  
  "getChromeWindowHandle": GeckoDriver.prototype.getChromeWindowHandle,
  "getCurrentChromeWindowHandle": GeckoDriver.prototype.getChromeWindowHandle,
  "getWindow":  GeckoDriver.prototype.getWindowHandle,  
  "getWindowHandles": GeckoDriver.prototype.getWindowHandles,
  "getChromeWindowHandles": GeckoDriver.prototype.getChromeWindowHandles,
  "getCurrentWindowHandles": GeckoDriver.prototype.getWindowHandles,  
  "getWindows":  GeckoDriver.prototype.getWindowHandles,  
  "getWindowPosition": GeckoDriver.prototype.getWindowPosition,
  "setWindowPosition": GeckoDriver.prototype.setWindowPosition,
  "getActiveFrame": GeckoDriver.prototype.getActiveFrame,
  "switchToFrame": GeckoDriver.prototype.switchToFrame,
  "switchToWindow": GeckoDriver.prototype.switchToWindow,
  "deleteSession": GeckoDriver.prototype.deleteSession,
  "importScript": GeckoDriver.prototype.importScript,
  "clearImportedScripts": GeckoDriver.prototype.clearImportedScripts,
  "getAppCacheStatus": GeckoDriver.prototype.getAppCacheStatus,
  "close": GeckoDriver.prototype.close,
  "closeWindow": GeckoDriver.prototype.close,  
  "closeChromeWindow": GeckoDriver.prototype.closeChromeWindow,
  "setTestName": GeckoDriver.prototype.setTestName,
  "takeScreenshot": GeckoDriver.prototype.takeScreenshot,
  "screenShot": GeckoDriver.prototype.takeScreenshot,  
  "screenshot": GeckoDriver.prototype.takeScreenshot,  
  "addCookie": GeckoDriver.prototype.addCookie,
  "getCookies": GeckoDriver.prototype.getCookies,
  "getAllCookies": GeckoDriver.prototype.getCookies,  
  "deleteAllCookies": GeckoDriver.prototype.deleteAllCookies,
  "deleteCookie": GeckoDriver.prototype.deleteCookie,
  "getActiveElement": GeckoDriver.prototype.getActiveElement,
  "getScreenOrientation": GeckoDriver.prototype.getScreenOrientation,
  "setScreenOrientation": GeckoDriver.prototype.setScreenOrientation,
  "getWindowSize": GeckoDriver.prototype.getWindowSize,
  "setWindowSize": GeckoDriver.prototype.setWindowSize,
  "maximizeWindow": GeckoDriver.prototype.maximizeWindow,
  "dismissDialog": GeckoDriver.prototype.dismissDialog,
  "acceptDialog": GeckoDriver.prototype.acceptDialog,
  "getTextFromDialog": GeckoDriver.prototype.getTextFromDialog,
  "sendKeysToDialog": GeckoDriver.prototype.sendKeysToDialog
};










let BrowserObj = function(win, driver) {
  this.browser = undefined;
  this.window = win;
  this.driver = driver;
  this.knownFrames = [];
  this.startPage = "about:blank";
  
  this.mainContentId = null;
  
  this.newSession = true;
  this.elementManager = new ElementManager([NAME, LINK_TEXT, PARTIAL_LINK_TEXT]);
  this.setBrowser(win);

  
  this.tab = null;
  this.pendingCommands = [];

  
  this.frameManager = new FrameManager(driver);
  this.frameRegsPending = 0;

  
  this.frameManager.addMessageManagerListeners(driver.mm);
  this.getIdForBrowser = driver.getIdForBrowser.bind(driver);
  this.updateIdForBrowser = driver.updateIdForBrowser.bind(driver);
  this._curFrameId = null;
  this._browserWasRemote = null;
  this._hasRemotenessChange = false;
};

Object.defineProperty(BrowserObj.prototype, "browserForTab", {
  get() {
    return this.browser.getBrowserForTab(this.tab);
  }
});






Object.defineProperty(BrowserObj.prototype, "curFrameId", {
  get() {
    let rv = null;
    if (this.driver.appName != "Firefox") {
      rv = this._curFrameId;
    } else if (this.tab) {
      rv = this.getIdForBrowser(this.browserForTab);
    }
    return rv;
  },

  set(id) {
    if (this.driver.appName != "Firefox") {
      this._curFrameId = id;
    }
  }
});





BrowserObj.prototype.getTabModalUI = function() {
  let br = this.browserForTab;
  if (!br.hasAttribute("tabmodalPromptShowing")) {
    return null;
  }

  
  
  let modals = br.parentNode.getElementsByTagNameNS(
      XUL_NS, "tabmodalprompt");
  return modals[0].ui;
};







BrowserObj.prototype.setBrowser = function(win) {
  switch (this.driver.appName) {
    case "Firefox":
      this.browser = win.gBrowser;
      break;

    case "Fennec":
      this.browser = win.BrowserApp;
      break;

    case "B2G":
      
      
      
      this.driver.sessionCapabilities.b2g = true;
      break;
  }
};


BrowserObj.prototype.startSession = function(newSession, win, callback) {
  callback(win, newSession);
};


BrowserObj.prototype.closeTab = function() {
  if (this.browser &&
      this.browser.removeTab &&
      this.tab !== null && (this.driver.appName != "B2G")) {
    this.browser.removeTab(this.tab);
  }
};







BrowserObj.prototype.addTab = function(uri) {
  return this.browser.addTab(uri, true);
};




BrowserObj.prototype.switchToTab = function(ind) {
  if (this.browser) {
    this.browser.selectTabAtIndex(ind);
    this.tab = this.browser.selectedTab;
  }
  this._browserWasRemote = this.browserForTab.isRemoteBrowser;
  this._hasRemotenessChange = false;
};










BrowserObj.prototype.register = function(uid, target) {
  let remotenessChange = this.hasRemotenessChange();
  if (this.curFrameId === null || remotenessChange) {
    if (this.browser) {
      
      
      if (!this.tab) {
        this.switchToTab(this.browser.selectedIndex);
      }

      if (target == this.browserForTab) {
        this.updateIdForBrowser(this.browserForTab, uid);
        this.mainContentId = uid;
      }
    } else {
      this._curFrameId = uid;
      this.mainContentId = uid;
    }
  }

  
  this.knownFrames.push(uid);
  return remotenessChange;
};






BrowserObj.prototype.hasRemotenessChange = function() {
  
  
  if (this.driver.appName != "Firefox" || this.tab === null) {
    return false;
  }

  if (this._hasRemotenessChange) {
    return true;
  }

  let currentIsRemote = this.browserForTab.isRemoteBrowser;
  this._hasRemotenessChange = this._browserWasRemote !== currentIsRemote;
  this._browserWasRemote = currentIsRemote;
  return this._hasRemotenessChange;
};





BrowserObj.prototype.flushPendingCommands = function() {
  if (!this._hasRemotenessChange) {
    return;
  }

  this._hasRemotenessChange = false;
  this.pendingCommands.forEach(cb => cb());
  this.pendingCommands = [];
};











BrowserObj.prototype.executeWhenReady = function(cb) {
  if (this.hasRemotenessChange()) {
    this.pendingCommands.push(cb);
  } else {
    cb();
  }
};
