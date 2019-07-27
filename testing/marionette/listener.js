




let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator);

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);

loader.loadSubScript("chrome://marionette/content/simpletest.js");
loader.loadSubScript("chrome://marionette/content/common.js");
loader.loadSubScript("chrome://marionette/content/actions.js");
Cu.import("chrome://marionette/content/elements.js");
Cu.import("chrome://marionette/content/error.js");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let utils = {};
utils.window = content;

loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);
loader.loadSubScript("chrome://marionette/content/sendkeys.js", utils);

loader.loadSubScript("chrome://specialpowers/content/specialpowersAPI.js");
loader.loadSubScript("chrome://specialpowers/content/specialpowers.js");

let marionetteLogObj = new MarionetteLogObj();

let isB2G = false;

let marionetteTestName;
let winUtil = content.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils);
let listenerId = null; 
let curFrame = content;
let isRemoteBrowser = () => curFrame.contentWindow !== null;
let previousFrame = null;
let elementManager = new ElementManager([]);
let accessibility = new Accessibility();
let actions = new ActionChain(utils, checkForInterrupted);
let importedScripts = null;



let sandbox;


let onunload;


let asyncTestRunning = false;
let asyncTestCommandId;
let asyncTestTimeoutId;

let inactivityTimeoutId = null;
let heartbeatCallback = function () {}; 

let originalOnError;

let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

let readyStateTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

let navTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
let onDOMContentLoaded;

let EVENT_INTERVAL = 30; 

let multiLast = {};

Cu.import("resource://gre/modules/Log.jsm");
let logger = Log.repository.getLogger("Marionette");
logger.info("loaded listener.js");
let modalHandler = function() {
  
  sendSyncMessage("Marionette:switchedToFrame", { frameValue: null, storePrevious: true });
  let isLocal = sendSyncMessage("MarionetteFrame:handleModal", {})[0].value;
  if (isLocal) {
    previousFrame = curFrame;
  }
  curFrame = content;
  sandbox = null;
};






function registerSelf() {
  let msg = {value: winUtil.outerWindowID};
  
  let register = sendSyncMessage("Marionette:register", msg);

  if (register[0]) {
    let {id, remotenessChange} = register[0][0];
    listenerId = id;
    if (typeof id != "undefined") {
      
      if (register[0][1] == true) {
        addMessageListener("MarionetteMainListener:emitTouchEvent", emitTouchEventForIFrame);
      }
      importedScripts = FileUtils.getDir('TmpD', [], false);
      importedScripts.append('marionetteContentScripts');
      startListeners();
      let rv = {};
      if (remotenessChange) {
        rv.listenerId = id;
      }
      sendAsyncMessage("Marionette:listenersAttached", rv);
    }
  }
}

function emitTouchEventForIFrame(message) {
  message = message.json;
  let identifier = actions.nextTouchId;

  let domWindowUtils = curFrame.
    QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIDOMWindowUtils);
  var ratio = domWindowUtils.screenPixelsPerCSSPixel;

  var typeForUtils;
  switch (message.type) {
    case 'touchstart':
      typeForUtils = domWindowUtils.TOUCH_CONTACT;
      break;
    case 'touchend':
      typeForUtils = domWindowUtils.TOUCH_REMOVE;
      break;
    case 'touchcancel':
      typeForUtils = domWindowUtils.TOUCH_CANCEL;
      break;
    case 'touchmove':
      typeForUtils = domWindowUtils.TOUCH_CONTACT;
      break;
  }
  domWindowUtils.sendNativeTouchPoint(identifier, typeForUtils,
    Math.round(message.screenX * ratio), Math.round(message.screenY * ratio),
    message.force, 90);
}

function dispatch(fn) {
  return function(msg) {
    let id = msg.json.command_id;
    try {
      let rv;
      if (typeof msg.json == "undefined" || msg.json instanceof Array) {
        rv = fn.apply(null, msg.json);
      } else {
        rv = fn(msg.json);
      }

      if (typeof rv == "undefined") {
        sendOk(id);
      } else {
        sendResponse({value: rv}, id);
      }
    } catch (e) {
      sendError(e, id);
    }
  };
}




function addMessageListenerId(messageName, handler) {
  addMessageListener(messageName + listenerId, handler);
}




function removeMessageListenerId(messageName, handler) {
  removeMessageListener(messageName + listenerId, handler);
}

let getElementSizeFn = dispatch(getElementSize);
let getActiveElementFn = dispatch(getActiveElement);
let clickElementFn = dispatch(clickElement);
let getElementAttributeFn = dispatch(getElementAttribute);
let getElementTextFn = dispatch(getElementText);
let getElementTagNameFn = dispatch(getElementTagName);
let getElementRectFn = dispatch(getElementRect);
let isElementEnabledFn = dispatch(isElementEnabled);




function startListeners() {
  addMessageListenerId("Marionette:newSession", newSession);
  addMessageListenerId("Marionette:executeScript", executeScript);
  addMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  addMessageListenerId("Marionette:executeJSScript", executeJSScript);
  addMessageListenerId("Marionette:singleTap", singleTap);
  addMessageListenerId("Marionette:actionChain", actionChain);
  addMessageListenerId("Marionette:multiAction", multiAction);
  addMessageListenerId("Marionette:get", get);
  addMessageListenerId("Marionette:pollForReadyState", pollForReadyState);
  addMessageListenerId("Marionette:cancelRequest", cancelRequest);
  addMessageListenerId("Marionette:getCurrentUrl", getCurrentUrl);
  addMessageListenerId("Marionette:getTitle", getTitle);
  addMessageListenerId("Marionette:getPageSource", getPageSource);
  addMessageListenerId("Marionette:goBack", goBack);
  addMessageListenerId("Marionette:goForward", goForward);
  addMessageListenerId("Marionette:refresh", refresh);
  addMessageListenerId("Marionette:findElementContent", findElementContent);
  addMessageListenerId("Marionette:findElementsContent", findElementsContent);
  addMessageListenerId("Marionette:getActiveElement", getActiveElementFn);
  addMessageListenerId("Marionette:clickElement", clickElementFn);
  addMessageListenerId("Marionette:getElementAttribute", getElementAttributeFn);
  addMessageListenerId("Marionette:getElementText", getElementTextFn);
  addMessageListenerId("Marionette:getElementTagName", getElementTagNameFn);
  addMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  addMessageListenerId("Marionette:getElementValueOfCssProperty", getElementValueOfCssProperty);
  addMessageListenerId("Marionette:submitElement", submitElement);
  addMessageListenerId("Marionette:getElementSize", getElementSizeFn);  
  addMessageListenerId("Marionette:getElementRect", getElementRectFn);
  addMessageListenerId("Marionette:isElementEnabled", isElementEnabledFn);
  addMessageListenerId("Marionette:isElementSelected", isElementSelected);
  addMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  addMessageListenerId("Marionette:getElementLocation", getElementLocation); 
  addMessageListenerId("Marionette:clearElement", clearElement);
  addMessageListenerId("Marionette:switchToFrame", switchToFrame);
  addMessageListenerId("Marionette:deleteSession", deleteSession);
  addMessageListenerId("Marionette:sleepSession", sleepSession);
  addMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  addMessageListenerId("Marionette:importScript", importScript);
  addMessageListenerId("Marionette:getAppCacheStatus", getAppCacheStatus);
  addMessageListenerId("Marionette:setTestName", setTestName);
  addMessageListenerId("Marionette:takeScreenshot", takeScreenshot);
  addMessageListenerId("Marionette:addCookie", addCookie);
  addMessageListenerId("Marionette:getCookies", getCookies);
  addMessageListenerId("Marionette:deleteAllCookies", deleteAllCookies);
  addMessageListenerId("Marionette:deleteCookie", deleteCookie);
}




function waitForReady() {
  if (content.document.readyState == 'complete') {
    readyStateTimer.cancel();
    content.addEventListener("mozbrowsershowmodalprompt", modalHandler, false);
    content.addEventListener("unload", waitForReady, false);
  }
  else {
    readyStateTimer.initWithCallback(waitForReady, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
}





function newSession(msg) {
  isB2G = msg.json.B2G;
  accessibility.strict = msg.json.raisesAccessibilityExceptions;
  resetValues();
  if (isB2G) {
    readyStateTimer.initWithCallback(waitForReady, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    
    
    
    
    
    actions.inputSource = Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH;
  }
}






function sleepSession(msg) {
  deleteSession();
  addMessageListener("Marionette:restart", restart);
}




function restart(msg) {
  removeMessageListener("Marionette:restart", restart);
  if (isB2G) {
    readyStateTimer.initWithCallback(waitForReady, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  registerSelf();
}




function deleteSession(msg) {
  removeMessageListenerId("Marionette:newSession", newSession);
  removeMessageListenerId("Marionette:executeScript", executeScript);
  removeMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  removeMessageListenerId("Marionette:executeJSScript", executeJSScript);
  removeMessageListenerId("Marionette:singleTap", singleTap);
  removeMessageListenerId("Marionette:actionChain", actionChain);
  removeMessageListenerId("Marionette:multiAction", multiAction);
  removeMessageListenerId("Marionette:get", get);
  removeMessageListenerId("Marionette:pollForReadyState", pollForReadyState);
  removeMessageListenerId("Marionette:cancelRequest", cancelRequest);
  removeMessageListenerId("Marionette:getTitle", getTitle);
  removeMessageListenerId("Marionette:getPageSource", getPageSource);
  removeMessageListenerId("Marionette:getCurrentUrl", getCurrentUrl);
  removeMessageListenerId("Marionette:goBack", goBack);
  removeMessageListenerId("Marionette:goForward", goForward);
  removeMessageListenerId("Marionette:refresh", refresh);
  removeMessageListenerId("Marionette:findElementContent", findElementContent);
  removeMessageListenerId("Marionette:findElementsContent", findElementsContent);
  removeMessageListenerId("Marionette:getActiveElement", getActiveElementFn);
  removeMessageListenerId("Marionette:clickElement", clickElementFn);
  removeMessageListenerId("Marionette:getElementAttribute", getElementAttributeFn);
  removeMessageListenerId("Marionette:getElementText", getElementTextFn);
  removeMessageListenerId("Marionette:getElementTagName", getElementTagNameFn);
  removeMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  removeMessageListenerId("Marionette:getElementValueOfCssProperty", getElementValueOfCssProperty);
  removeMessageListenerId("Marionette:submitElement", submitElement);
  removeMessageListenerId("Marionette:getElementSize", getElementSizeFn); 
  removeMessageListenerId("Marionette:getElementRect", getElementRectFn);
  removeMessageListenerId("Marionette:isElementEnabled", isElementEnabledFn);
  removeMessageListenerId("Marionette:isElementSelected", isElementSelected);
  removeMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  removeMessageListenerId("Marionette:getElementLocation", getElementLocation);
  removeMessageListenerId("Marionette:clearElement", clearElement);
  removeMessageListenerId("Marionette:switchToFrame", switchToFrame);
  removeMessageListenerId("Marionette:deleteSession", deleteSession);
  removeMessageListenerId("Marionette:sleepSession", sleepSession);
  removeMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  removeMessageListenerId("Marionette:importScript", importScript);
  removeMessageListenerId("Marionette:getAppCacheStatus", getAppCacheStatus);
  removeMessageListenerId("Marionette:setTestName", setTestName);
  removeMessageListenerId("Marionette:takeScreenshot", takeScreenshot);
  removeMessageListenerId("Marionette:addCookie", addCookie);
  removeMessageListenerId("Marionette:getCookies", getCookies);
  removeMessageListenerId("Marionette:deleteAllCookies", deleteAllCookies);
  removeMessageListenerId("Marionette:deleteCookie", deleteCookie);
  if (isB2G) {
    content.removeEventListener("mozbrowsershowmodalprompt", modalHandler, false);
  }
  elementManager.reset();
  
  curFrame = content;
  curFrame.focus();
  actions.touchIds = {};
}








function sendToServer(name, data, objs, id) {
  if (!data) {
    data = {}
  }
  if (id) {
    data.command_id = id;
  }
  sendAsyncMessage(name, data, objs);
}




function sendResponse(value, command_id) {
  sendToServer("Marionette:done", value, null, command_id);
}




function sendOk(command_id) {
  sendToServer("Marionette:ok", null, null, command_id);
}




function sendLog(msg) {
  sendToServer("Marionette:log", {message: msg});
}




function sendError(err, cmdId) {
  sendToServer("Marionette:error", null, {error: err}, cmdId);
}




function resetValues() {
  sandbox = null;
  curFrame = content;
  actions.mouseEventsOnly = false;
}




function dumpLog(logline) {
  dump(Date.now() + " Marionette: " + logline);
}




function wasInterrupted() {
  if (previousFrame) {
    let element = content.document.elementFromPoint((content.innerWidth/2), (content.innerHeight/2));
    if (element.id.indexOf("modal-dialog") == -1) {
      return true;
    }
    else {
      return false;
    }
  }
  return sendSyncMessage("MarionetteFrame:getInterruptedState", {})[0].value;
}

function checkForInterrupted() {
    if (wasInterrupted()) {
      if (previousFrame) {
        
        cuFrame = actions.frame = previousFrame;
        previousFrame = null;
        sandbox = null;
      }
      else {
        
        sendSyncMessage("Marionette:switchToModalOrigin");
      }
      sendSyncMessage("Marionette:switchedToFrame", { restorePrevious: true });
    }
}








function createExecuteContentSandbox(aWindow, timeout) {
  let sandbox = new Cu.Sandbox(aWindow, {sandboxPrototype: aWindow});
  sandbox.global = sandbox;
  sandbox.window = aWindow;
  sandbox.document = sandbox.window.document;
  sandbox.navigator = sandbox.window.navigator;
  sandbox.testUtils = utils;
  sandbox.asyncTestCommandId = asyncTestCommandId;

  let marionette = new Marionette(this, aWindow, "content",
                                  marionetteLogObj, timeout,
                                  heartbeatCallback,
                                  marionetteTestName);
  marionette.runEmulatorCmd = (cmd, cb) => this.runEmulatorCmd(cmd, cb);
  marionette.runEmulatorShell = (args, cb) => this.runEmulatorShell(args, cb);
  sandbox.marionette = marionette;
  marionette.exports.forEach(function(fn) {
    try {
      sandbox[fn] = marionette[fn].bind(marionette);
    }
    catch(e) {
      sandbox[fn] = marionette[fn];
    }
  });

  if (aWindow.wrappedJSObject.SpecialPowers != undefined) {
    XPCOMUtils.defineLazyGetter(sandbox, 'SpecialPowers', function() {
      return aWindow.wrappedJSObject.SpecialPowers;
    });
  }
  else {
    XPCOMUtils.defineLazyGetter(sandbox, 'SpecialPowers', function() {
      return new SpecialPowers(aWindow);
    });
  }

  sandbox.asyncComplete = function(obj, id) {
    if (id == asyncTestCommandId) {
      curFrame.removeEventListener("unload", onunload, false);
      curFrame.clearTimeout(asyncTestTimeoutId);

      if (inactivityTimeoutId != null) {
        curFrame.clearTimeout(inactivityTimeoutId);
      }

      sendSyncMessage("Marionette:shareData",
          {log: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();

      if (error.isError(obj)) {
        sendError(obj, id);
      } else {
        if (Object.keys(_emu_cbs).length) {
          _emu_cbs = {};
          sendError({message: "Emulator callback still pending when finish() called"}, id);
        } else {
          sendResponse({value: elementManager.wrapValue(obj)}, id);
        }
      }

      asyncTestRunning = false;
      asyncTestTimeoutId = undefined;
      asyncTestCommandId = undefined;
      inactivityTimeoutId = null;
    }
  };
  sandbox.finish = function sandbox_finish() {
    if (asyncTestRunning) {
      sandbox.asyncComplete(marionette.generate_results(), sandbox.asyncTestCommandId);
    } else {
      return marionette.generate_results();
    }
  };
  sandbox.marionetteScriptFinished = val =>
      sandbox.asyncComplete(val, sandbox.asyncTestCommandId);

  return sandbox;
}





function executeScript(msg, directInject) {
  
  if (msg.json.inactivityTimeout) {
    let setTimer = function() {
      inactivityTimeoutId = curFrame.setTimeout(function() {
        sendError(new ScriptTimeoutError("timed out due to inactivity"), asyncTestCommandId);
      }, msg.json.inactivityTimeout);
   };

    setTimer();
    heartbeatCallback = function() {
      curFrame.clearTimeout(inactivityTimeoutId);
      setTimer();
    };
  }

  asyncTestCommandId = msg.json.command_id;
  let script = msg.json.script;

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curFrame,
                                          msg.json.timeout);
    if (!sandbox) {
      sendError(new WebDriverError("Could not create sandbox!"), asyncTestCommandId);
      return;
    }
  } else {
    sandbox.asyncTestCommandId = asyncTestCommandId;
  }

  try {
    if (directInject) {
      if (importedScripts.exists()) {
        let stream = Components.classes["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Components.interfaces.nsIFileInputStream);
        stream.init(importedScripts, -1, 0, 0);
        let data = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();
        script = data + script;
      }
      let res = Cu.evalInSandbox(script, sandbox, "1.8", "dummy file" ,0);
      sendSyncMessage("Marionette:shareData",
                      {log: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();

      if (res == undefined || res.passed == undefined) {
        sendError(new JavaScriptError("Marionette.finish() not called"), asyncTestCommandId);
      }
      else {
        sendResponse({value: elementManager.wrapValue(res)}, asyncTestCommandId);
      }
    }
    else {
      try {
        sandbox.__marionetteParams = Cu.cloneInto(elementManager.convertWrappedArguments(
          msg.json.args, curFrame), sandbox, { wrapReflectors: true });
      } catch (e) {
        sendError(e, asyncTestCommandId);
        return;
      }

      script = "let __marionetteFunc = function(){" + script + "};" +
                   "__marionetteFunc.apply(null, __marionetteParams);";
      if (importedScripts.exists()) {
        let stream = Components.classes["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Components.interfaces.nsIFileInputStream);
        stream.init(importedScripts, -1, 0, 0);
        let data = NetUtil.readInputStreamToString(stream, stream.available());
        stream.close();
        script = data + script;
      }
      let res = Cu.evalInSandbox(script, sandbox, "1.8", "dummy file", 0);
      sendSyncMessage("Marionette:shareData",
                      {log: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();
      sendResponse({value: elementManager.wrapValue(res)}, asyncTestCommandId);
    }
  } catch (e) {
    let err = new JavaScriptError(
        e,
        "execute_script",
        msg.json.filename,
        msg.json.line,
        script);
    sendError(err, asyncTestCommandId);
  }
}




function setTestName(msg) {
  marionetteTestName = msg.json.value;
  sendOk(msg.json.command_id);
}




function executeAsyncScript(msg) {
  executeWithCallback(msg);
}




function executeJSScript(msg) {
  if (msg.json.async) {
    executeWithCallback(msg, msg.json.async);
  }
  else {
    executeScript(msg, true);
  }
}









function executeWithCallback(msg, useFinish) {
  
  if (msg.json.inactivityTimeout) {
    let setTimer = function() {
      inactivityTimeoutId = curFrame.setTimeout(function() {
        sandbox.asyncComplete(new ScriptTimeout("timed out due to inactivity"), asyncTestCommandId);
      }, msg.json.inactivityTimeout);
    };

    setTimer();
    heartbeatCallback = function() {
      curFrame.clearTimeout(inactivityTimeoutId);
      setTimer();
    };
  }

  let script = msg.json.script;
  asyncTestCommandId = msg.json.command_id;

  onunload = function() {
    sendError(new JavaScriptError("unload was called"), asyncTestCommandId);
  };
  curFrame.addEventListener("unload", onunload, false);

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curFrame,
                                          msg.json.timeout);
    if (!sandbox) {
      sendError(new JavaScriptError("Could not create sandbox!"), asyncTestCommandId);
      return;
    }
  }
  else {
    sandbox.asyncTestCommandId = asyncTestCommandId;
  }
  sandbox.tag = script;

  
  
  
  
  
  asyncTestTimeoutId = curFrame.setTimeout(function() {
    sandbox.asyncComplete(new ScriptTimeoutError("timed out"), asyncTestCommandId);
  }, msg.json.timeout);

  originalOnError = curFrame.onerror;
  curFrame.onerror = function errHandler(msg, url, line) {
    sandbox.asyncComplete(new JavaScriptError(msg + "@" + url + ", line " + line), asyncTestCommandId);
    curFrame.onerror = originalOnError;
  };

  let scriptSrc;
  if (useFinish) {
    if (msg.json.timeout == null || msg.json.timeout == 0) {
      sendError(new TimeoutError("Please set a timeout"), asyncTestCommandId);
    }
    scriptSrc = script;
  }
  else {
    try {
      sandbox.__marionetteParams = Cu.cloneInto(elementManager.convertWrappedArguments(
        msg.json.args, curFrame), sandbox, { wrapReflectors: true });
    } catch (e) {
      sendError(e, asyncTestCommandId);
      return;
    }

    scriptSrc = "__marionetteParams.push(marionetteScriptFinished);" +
                "let __marionetteFunc = function() { " + script + "};" +
                "__marionetteFunc.apply(null, __marionetteParams); ";
  }

  try {
    asyncTestRunning = true;
    if (importedScripts.exists()) {
      let stream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
      stream.init(importedScripts, -1, 0, 0);
      let data = NetUtil.readInputStreamToString(stream, stream.available());
      stream.close();
      scriptSrc = data + scriptSrc;
    }
    Cu.evalInSandbox(scriptSrc, sandbox, "1.8", "dummy file", 0);
  } catch (e) {
    let err = new JavaScriptError(
        e,
        "execute_async_script",
        msg.json.filename,
        msg.json.line,
        scriptSrc);
    sandbox.asyncComplete(err, asyncTestCommandId);
  }
}




function emitTouchEvent(type, touch) {
  if (!wasInterrupted()) {
    let loggingInfo = "emitting Touch event of type " + type + " to element with id: " + touch.target.id + " and tag name: " + touch.target.tagName + " at coordinates (" + touch.clientX + ", " + touch.clientY + ") relative to the viewport";
    dumpLog(loggingInfo);
    var docShell = curFrame.document.defaultView.
                   QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                   getInterface(Components.interfaces.nsIWebNavigation).
                   QueryInterface(Components.interfaces.nsIDocShell);
    if (docShell.asyncPanZoomEnabled && actions.scrolling) {
      
      let index = sendSyncMessage("MarionetteFrame:getCurrentFrameId");
      
      if (index != null) {
        sendSyncMessage("Marionette:emitTouchEvent",
          { index: index, type: type, id: touch.identifier,
            clientX: touch.clientX, clientY: touch.clientY,
            screenX: touch.screenX, screenY: touch.screenY,
            radiusX: touch.radiusX, radiusY: touch.radiusY,
            rotation: touch.rotationAngle, force: touch.force });
        return;
      }
    }
    
    






    let domWindowUtils = curFrame.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
    domWindowUtils.sendTouchEvent(type, [touch.identifier], [touch.clientX], [touch.clientY], [touch.radiusX], [touch.radiusY], [touch.rotationAngle], [touch.force], 1, 0);
  }
}







function coordinates(target, x, y) {
  let box = target.getBoundingClientRect();
  if (x == null) {
    x = box.width / 2;
  }
  if (y == null) {
    y = box.height / 2;
  }
  let coords = {};
  coords.x = box.left + x;
  coords.y = box.top + y;
  return coords;
}







function elementInViewport(el, x, y) {
  let c = coordinates(el, x, y);
  let viewPort = {top: curFrame.pageYOffset,
                  left: curFrame.pageXOffset,
                  bottom: (curFrame.pageYOffset + curFrame.innerHeight),
                  right:(curFrame.pageXOffset + curFrame.innerWidth)};
  return (viewPort.left <= c.x + curFrame.pageXOffset &&
          c.x + curFrame.pageXOffset <= viewPort.right &&
          viewPort.top <= c.y + curFrame.pageYOffset &&
          c.y + curFrame.pageYOffset <= viewPort.bottom);
}







function checkVisible(el, x, y) {
  
  if (utils.getElementAttribute(el, "namespaceURI").indexOf("there.is.only.xul") == -1) {
    
    let visible = utils.isElementDisplayed(el);
    if (!visible) {
      return false;
    }
  }

  if (el.tagName.toLowerCase() === 'body') {
    return true;
  }
  if (!elementInViewport(el, x, y)) {
    
    if (el.scrollIntoView) {
      el.scrollIntoView(false);
      if (!elementInViewport(el)) {
        return false;
      }
    }
    else {
      return false;
    }
  }
  return true;
}





function singleTap(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    let acc = accessibility.getAccessibleObject(el, true);
    
    let visible = checkVisible(el, msg.json.corx, msg.json.cory);
    checkVisibleAccessibility(acc, visible);
    if (!visible) {
      sendError(new ElementNotVisibleError("Element is not currently visible and may not be manipulated"), command_id);
      return;
    }
    checkActionableAccessibility(acc);
    if (!curFrame.document.createTouch) {
      actions.mouseEventsOnly = true;
    }
    let c = coordinates(el, msg.json.corx, msg.json.cory);
    if (!actions.mouseEventsOnly) {
      let touchId = actions.nextTouchId++;
      let touch = createATouch(el, c.x, c.y, touchId);
      emitTouchEvent('touchstart', touch);
      emitTouchEvent('touchend', touch);
    }
    actions.mouseTap(el.ownerDocument, c.x, c.y);
    sendOk(command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}







function checkEnabledStateAccessibility(accesible, enabled) {
  if (!accesible) {
    return;
  }
  if (enabled && accessibility.matchState(accesible, 'STATE_UNAVAILABLE')) {
    accessibility.handleErrorMessage('Element is enabled but disabled via ' +
      'the accessibility API');
  }
}







function checkVisibleAccessibility(accesible, visible) {
  if (!accesible) {
    return;
  }
  let hiddenAccessibility = accessibility.isHidden(accesible);
  let message;
  if (visible && hiddenAccessibility) {
    message = 'Element is not currently visible via the accessibility API ' +
      'and may not be manipulated by it';
  } else if (!visible && !hiddenAccessibility) {
    message = 'Element is currently only visible via the accessibility API ' +
      'and can be manipulated by it';
  }
  accessibility.handleErrorMessage(message);
}





function checkActionableAccessibility(accesible) {
  if (!accesible) {
    return;
  }
  let message;
  if (!accessibility.hasActionCount(accesible)) {
    message = 'Element does not support any accessible actions';
  } else if (!accessibility.isActionableRole(accesible)) {
    message = 'Element does not have a correct accessibility role ' +
      'and may not be manipulated via the accessibility API';
  } else if (!accessibility.hasValidName(accesible)) {
    message = 'Element is missing an accesible name';
  }
  accessibility.handleErrorMessage(message);
}






function createATouch(el, corx, cory, touchId) {
  let doc = el.ownerDocument;
  let win = doc.defaultView;
  let [clientX, clientY, pageX, pageY, screenX, screenY] =
    actions.getCoordinateInfo(el, corx, cory);
  let atouch = doc.createTouch(win, el, touchId, pageX, pageY, screenX, screenY, clientX, clientY);
  return atouch;
}




function actionChain(msg) {
  let command_id = msg.json.command_id;
  let args = msg.json.chain;
  let touchId = msg.json.nextId;

  let callbacks = {};
  callbacks.onSuccess = value => sendResponse(value, command_id);
  callbacks.onError = err => sendError(err, command_id);

  let touchProvider = {};
  touchProvider.createATouch = createATouch;
  touchProvider.emitTouchEvent = emitTouchEvent;

  try {
    actions.dispatchActions(
        args,
        touchId,
        curFrame,
        elementManager,
        callbacks,
        touchProvider);
  } catch (e) {
    sendError(e, command_id);
  }
}





function emitMultiEvents(type, touch, touches) {
  let target = touch.target;
  let doc = target.ownerDocument;
  let win = doc.defaultView;
  
  let documentTouches = doc.createTouchList(touches.filter(function(t) {
    return ((t.target.ownerDocument === doc) && (type != 'touchcancel'));
  }));
  
  let targetTouches = doc.createTouchList(touches.filter(function(t) {
    return ((t.target === target) && ((type != 'touchcancel') || (type != 'touchend')));
  }));
  
  let changedTouches = doc.createTouchList(touch);
  
  let event = doc.createEvent('TouchEvent');
  event.initTouchEvent(type,
                       true,
                       true,
                       win,
                       0,
                       false, false, false, false,
                       documentTouches,
                       targetTouches,
                       changedTouches);
  target.dispatchEvent(event);
}





function setDispatch(batches, touches, command_id, batchIndex) {
  if (typeof batchIndex === "undefined") {
    batchIndex = 0;
  }
  
  if (batchIndex >= batches.length) {
    multiLast = {};
    sendOk(command_id);
    return;
  }
  
  let batch = batches[batchIndex];
  
  let pack;
  
  let touchId;
  
  let command;
  
  let el;
  let corx;
  let cory;
  let touch;
  let lastTouch;
  let touchIndex;
  let waitTime = 0;
  let maxTime = 0;
  let c;
  batchIndex++;
  
  for (let i = 0; i < batch.length; i++) {
    pack = batch[i];
    touchId = pack[0];
    command = pack[1];
    switch (command) {
      case 'press':
        el = elementManager.getKnownElement(pack[2], curFrame);
        c = coordinates(el, pack[3], pack[4]);
        touch = createATouch(el, c.x, c.y, touchId);
        multiLast[touchId] = touch;
        touches.push(touch);
        emitMultiEvents('touchstart', touch, touches);
        break;
      case 'release':
        touch = multiLast[touchId];
        
        touchIndex = touches.indexOf(touch);
        touches.splice(touchIndex, 1);
        emitMultiEvents('touchend', touch, touches);
        break;
      case 'move':
        el = elementManager.getKnownElement(pack[2], curFrame);
        c = coordinates(el);
        touch = createATouch(multiLast[touchId].target, c.x, c.y, touchId);
        touchIndex = touches.indexOf(lastTouch);
        touches[touchIndex] = touch;
        multiLast[touchId] = touch;
        emitMultiEvents('touchmove', touch, touches);
        break;
      case 'moveByOffset':
        el = multiLast[touchId].target;
        lastTouch = multiLast[touchId];
        touchIndex = touches.indexOf(lastTouch);
        let doc = el.ownerDocument;
        let win = doc.defaultView;
        
        let clientX = lastTouch.clientX + pack[2],
            clientY = lastTouch.clientY + pack[3];
        let pageX = clientX + win.pageXOffset,
            pageY = clientY + win.pageYOffset;
        let screenX = clientX + win.mozInnerScreenX,
            screenY = clientY + win.mozInnerScreenY;
        touch = doc.createTouch(win, el, touchId, pageX, pageY, screenX, screenY, clientX, clientY);
        touches[touchIndex] = touch;
        multiLast[touchId] = touch;
        emitMultiEvents('touchmove', touch, touches);
        break;
      case 'wait':
        if (pack[2] != undefined ) {
          waitTime = pack[2]*1000;
          if (waitTime > maxTime) {
            maxTime = waitTime;
          }
        }
        break;
    }
  }
  if (maxTime != 0) {
    checkTimer.initWithCallback(function(){setDispatch(batches, touches, command_id, batchIndex);}, maxTime, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  else {
    setDispatch(batches, touches, command_id, batchIndex);
  }
}




function multiAction(msg) {
  let command_id = msg.json.command_id;
  let args = msg.json.value;
  
  let maxlen = msg.json.maxlen;
  try {
    
    let commandArray = elementManager.convertWrappedArguments(args, curFrame);
    let concurrentEvent = [];
    let temp;
    for (let i = 0; i < maxlen; i++) {
      let row = [];
      for (let j = 0; j < commandArray.length; j++) {
        if (commandArray[j][i] != undefined) {
          
          temp = commandArray[j][i];
          temp.unshift(j);
          row.push(temp);
        }
      }
      concurrentEvent.push(row);
    }
    
    
    
    let pendingTouches = [];
    setDispatch(concurrentEvent, pendingTouches, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}






function pollForReadyState(msg, start, callback) {
  let {pageTimeout, url, command_id} = msg.json;
  start = start ? start : new Date().getTime();

  if (!callback) {
    callback = () => {};
  }

  let end = null;
  function checkLoad() {
    navTimer.cancel();
    end = new Date().getTime();
    let aboutErrorRegex = /about:.+(error)\?/;
    let elapse = end - start;
    if (pageTimeout == null || elapse <= pageTimeout) {
      if (curFrame.document.readyState == "complete") {
        callback();
        sendOk(command_id);
      } else if (curFrame.document.readyState == "interactive" &&
                 aboutErrorRegex.exec(curFrame.document.baseURI) &&
                 !curFrame.document.baseURI.startsWith(url)) {
        
        callback();
        sendError(new UnknownError("Error loading page"), command_id);
      } else if (curFrame.document.readyState == "interactive" &&
                 curFrame.document.baseURI.startsWith("about:")) {
        callback();
        sendOk(command_id);
      } else {
        navTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
      }
    } else {
      callback();
      sendError(new TimeoutError("Error loading page, timed out (checkLoad)"), command_id);
    }
  }
  checkLoad();
}







function get(msg) {
  let start = new Date().getTime();

  
  
  
  onDOMContentLoaded = function onDOMContentLoaded(event) {
    if (!event.originalTarget.defaultView.frameElement ||
        event.originalTarget.defaultView.frameElement == curFrame.frameElement) {
      pollForReadyState(msg, start, () => {
        removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
        onDOMContentLoaded = null;
      });
    }
  };

  function timerFunc() {
    removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
    sendError(new TimeoutError("Error loading page, timed out (onDOMContentLoaded)"), msg.json.command_id);
  }
  if (msg.json.pageTimeout != null) {
    navTimer.initWithCallback(timerFunc, msg.json.pageTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  addEventListener("DOMContentLoaded", onDOMContentLoaded, false);
  curFrame.location = msg.json.url;
}

 




function cancelRequest() {
  navTimer.cancel();
  if (onDOMContentLoaded) {
    removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
  }
}




function getCurrentUrl(msg) {
  let url;
  if (msg.json.isB2G) {
    url = curFrame.location.href;
  } else {
    url = content.location.href;
  }
  sendResponse({value: url}, msg.json.command_id);
}




function getTitle(msg) {
  sendResponse({value: curFrame.top.document.title}, msg.json.command_id);
}




function getPageSource(msg) {
  var XMLSerializer = curFrame.XMLSerializer;
  var pageSource = new XMLSerializer().serializeToString(curFrame.document);
  sendResponse({value: pageSource}, msg.json.command_id);
}




function goBack(msg) {
  curFrame.history.back();
  sendOk(msg.json.command_id);
}




function goForward(msg) {
  curFrame.history.forward();
  sendOk(msg.json.command_id);
}




function refresh(msg) {
  let command_id = msg.json.command_id;
  curFrame.location.reload(true);
  let listen = function() {
    removeEventListener("DOMContentLoaded", arguments.callee, false);
    sendOk(command_id);
  };
  addEventListener("DOMContentLoaded", listen, false);
}




function findElementContent(msg) {
  let command_id = msg.json.command_id;
  try {
    let onSuccess = (el, id) => sendResponse({value: el}, id);
    let onError = (err, id) => sendError(err, id);
    elementManager.find(curFrame, msg.json, msg.json.searchTimeout,
        false , onSuccess, onError, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}




function findElementsContent(msg) {
  let command_id = msg.json.command_id;
  try {
    let onSuccess = (els, id) => sendResponse({value: els}, id);
    let onError = (err, id) => sendError(err, id);
    elementManager.find(curFrame, msg.json, msg.json.searchTimeout,
        true , onSuccess, onError, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}







function getActiveElement() {
  let el = curFrame.document.activeElement;
  return elementManager.addToKnownElements(el);
}







function clickElement(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  let acc = accessibility.getAccessibleObject(el, true);
  let visible = checkVisible(el);
  checkVisibleAccessibility(acc, visible);
  if (!visible) {
    throw new ElementNotVisibleError("Element is not visible");
  }
  checkActionableAccessibility(acc);
  if (utils.isElementEnabled(el)) {
    utils.synthesizeMouseAtCenter(el, {}, el.ownerDocument.defaultView);
  } else {
    throw new InvalidElementStateError("Element is not Enabled");
  }
}












function getElementAttribute(id, name) {
  let el = elementManager.getKnownElement(id, curFrame);
  return utils.getElementAttribute(el, name);
}










function getElementText(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  return utils.getElementText(el);
}










function getElementTagName(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  return el.tagName.toLowerCase();
}




function isElementDisplayed(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    let displayed = utils.isElementDisplayed(el);
    checkVisibleAccessibility(accessibility.getAccessibleObject(el), displayed);
    sendResponse({value: displayed}, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}









function getElementValueOfCssProperty(msg){
  let command_id = msg.json.command_id;
  let propertyName = msg.json.propertyName;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: curFrame.document.defaultView.getComputedStyle(el, null).getPropertyValue(propertyName)},
                 command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}






function submitElement (msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    while (el.parentNode != null && el.tagName.toLowerCase() != 'form') {
      el = el.parentNode;
    }
    if (el.tagName && el.tagName.toLowerCase() == 'form') {
      el.submit();
      sendOk(command_id);
    } else {
      sendError(new NoSuchElementError("Element is not a form element or in a form"), command_id);
    }
  } catch (e) {
    sendError(e, command_id);
  }
}










function getElementSize(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  let clientRect = el.getBoundingClientRect();
  return {width: clientRect.width, height: clientRect.height};
}










function getElementRect(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  let clientRect = el.getBoundingClientRect();
  return {
    x: clientRect.x + curFrame.pageXOffset,
    y: clientRect.y  + curFrame.pageYOffset,
    width: clientRect.width,
    height: clientRect.height
  };
}










function isElementEnabled(id) {
  let el = elementManager.getKnownElement(id, curFrame);
  let enabled = utils.isElementEnabled(el);
  checkEnabledStateAccessibility(accessibility.getAccessibleObject(el), enabled);
  return enabled;
}




function isElementSelected(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.isElementSelected(el)}, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}




function sendKeysToElement(msg) {
  let command_id = msg.json.command_id;
  let val = msg.json.value;

  let el = elementManager.getKnownElement(msg.json.id, curFrame);
  if (el.type == "file") {
    let p = val.join("");

    
    
    
    
    
    if (isRemoteBrowser()) {
      let fs = Array.prototype.slice.call(el.files);
      let file;
      try {
        file = new File(p);
      } catch (e) {
        let err = new IllegalArgumentError(`File not found: ${val}`);
        sendError(err.message, err.code, err.stack, command_id);
        return;
      }
      fs.push(file);

      let wel = new SpecialPowers(utils.window).wrap(el);
      wel.mozSetFileArray(fs);
    } else {
      sendSyncMessage("Marionette:setElementValue", {value: p}, {element: el});
    }

    sendOk(command_id);
  } else {
    utils.sendKeysToElement(curFrame, el, val, sendOk, sendError, command_id);
  }
}




function getElementLocation(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    let rect = el.getBoundingClientRect();

    let location = {};
    location.x = rect.left;
    location.y = rect.top;

    sendResponse({value: location}, command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}




function clearElement(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    if (el.type == "file") {
      el.value = null;
    } else {
      utils.clearElement(el);
    }
    sendOk(command_id);
  } catch (e) {
    sendError(e, command_id);
  }
}





function switchToFrame(msg) {
  let command_id = msg.json.command_id;
  function checkLoad() {
    let errorRegex = /about:.+(error)|(blocked)\?/;
    if (curFrame.document.readyState == "complete") {
      sendOk(command_id);
      return;
    } else if (curFrame.document.readyState == "interactive" &&
        errorRegex.exec(curFrame.document.baseURI)) {
      sendError(new UnknownError("Error loading page"), command_id);
      return;
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  let foundFrame = null;
  let frames = [];
  let parWindow = null;
  
  try {
    frames = curFrame.frames;
    
    
    parWindow = curFrame.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  } catch (e) {
    
    
    
    msg.json.id = null;
    msg.json.element = null;
  }

  if ((msg.json.id === null || msg.json.id === undefined) && (msg.json.element == null)) {
    
    sendSyncMessage("Marionette:switchedToFrame", { frameValue: null });

    curFrame = content;
    if(msg.json.focus == true) {
      curFrame.focus();
    }
    sandbox = null;
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    return;
  }
  if (msg.json.element != undefined) {
    if (elementManager.seenItems[msg.json.element] != undefined) {
      let wantedFrame;
      try {
        wantedFrame = elementManager.getKnownElement(msg.json.element, curFrame); 
      } catch (e) {
        sendError(e, command_id);
      }

      if (frames.length > 0) {
        for (let i = 0; i < frames.length; i++) {
          
          if (XPCNativeWrapper(frames[i].frameElement) == XPCNativeWrapper(wantedFrame)) {
            curFrame = frames[i].frameElement;
            foundFrame = i;
          }
        }
      }
      if (foundFrame === null) {
        
        
        
        let iframes = curFrame.document.getElementsByTagName("iframe");
        for (var i = 0; i < iframes.length; i++) {
          if (XPCNativeWrapper(iframes[i]) == XPCNativeWrapper(wantedFrame)) {
            curFrame = iframes[i];
            foundFrame = i;
          }
        }
      }
    }
  }
  if (foundFrame === null) {
    if (typeof(msg.json.id) === 'number') {
      try {
        foundFrame = frames[msg.json.id].frameElement;
        if (foundFrame !== null) {
          curFrame = foundFrame;
          foundFrame = elementManager.addToKnownElements(curFrame);
        }
        else {
          
          
          sendSyncMessage("Marionette:switchedToFrame", { frameValue: null});
          curFrame = content;
          if(msg.json.focus == true) {
            curFrame.focus();
          }
          sandbox = null;
          checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
          return;
        }
      } catch (e) {
        
        
        
        let iframes = curFrame.document.getElementsByTagName("iframe");
        if (msg.json.id >= 0 && msg.json.id < iframes.length) {
          curFrame = iframes[msg.json.id];
          foundFrame = msg.json.id;
        }
      }
    }
  }

  if (foundFrame === null) {
    sendError(new NoSuchFrameError("Unable to locate frame: " + (msg.json.id || msg.json.element)), command_id);
    return true;
  }

  sandbox = null;

  
  
  let frameValue = elementManager.wrapValue(curFrame.wrappedJSObject)['ELEMENT'];
  sendSyncMessage("Marionette:switchedToFrame", { frameValue: frameValue });

  let rv = null;
  if (curFrame.contentWindow === null) {
    
    
    curFrame = content;
    rv = {win: parWindow, frame: foundFrame};
  } else {
    curFrame = curFrame.contentWindow;
    if (msg.json.focus)
      curFrame.focus();
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }

  sendResponse({value: rv}, command_id);
}
 


function addCookie(msg) {
  let cookie = msg.json.cookie;
  if (!cookie.expiry) {
    var date = new Date();
    var thePresent = new Date(Date.now());
    date.setYear(thePresent.getFullYear() + 20);
    cookie.expiry = date.getTime() / 1000;  
  }

  if (!cookie.domain) {
    var location = curFrame.document.location;
    cookie.domain = location.hostname;
  } else {
    var currLocation = curFrame.location;
    var currDomain = currLocation.host;
    if (currDomain.indexOf(cookie.domain) == -1) {
      sendError(new InvalidCookieDomainError("You may only set cookies for the current domain"), msg.json.command_id);
    }
  }

  
  
  
  if (cookie.domain.match(/:\d+$/)) {
    cookie.domain = cookie.domain.replace(/:\d+$/, '');
  }

  var document = curFrame.document;
  if (!document || !document.contentType.match(/html/i)) {
    sendError(new UnableToSetCookie("You may only set cookies on html documents"), msg.json.command_id);
  }

  let added = sendSyncMessage("Marionette:addCookie", {value: cookie});
  if (added[0] !== true) {
    sendError(new UnknownError("Error setting cookie"), msg.json.command_id);
    return;
  }
  sendOk(msg.json.command_id);
}




function getCookies(msg) {
  var toReturn = [];
  var cookies = getVisibleCookies(curFrame.location);
  for (let cookie of cookies) {
    var expires = cookie.expires;
    if (expires == 0) {  
      expires = null;
    } else if (expires == 1) { 
      expires = 0;
    }
    toReturn.push({
      'name': cookie.name,
      'value': cookie.value,
      'path': cookie.path,
      'domain': cookie.host,
      'secure': cookie.isSecure,
      'expiry': expires
    });
  }
  sendResponse({value: toReturn}, msg.json.command_id);
}




function deleteCookie(msg) {
  let toDelete = msg.json.name;
  let cookies = getVisibleCookies(curFrame.location);
  for (let cookie of cookies) {
    if (cookie.name == toDelete) {
      let deleted = sendSyncMessage("Marionette:deleteCookie", {value: cookie});
      if (deleted[0] !== true) {
        sendError(new UnknownError("Could not delete cookie: " + msg.json.name), msg.json.command_id);
        return;
      }
    }
  }

  sendOk(msg.json.command_id);
}




function deleteAllCookies(msg) {
  let cookies = getVisibleCookies(curFrame.location);
  for (let cookie of cookies) {
    let deleted = sendSyncMessage("Marionette:deleteCookie", {value: cookie});
    if (!deleted[0]) {
      sendError(new UnknownError("Could not delete cookie: " + JSON.stringify(cookie)), msg.json.command_id);
      return;
    }
  }
  sendOk(msg.json.command_id);
}




function getVisibleCookies(location) {
  let currentPath = location.pathname || '/';
  let result = sendSyncMessage("Marionette:getVisibleCookies",
                               {value: [currentPath, location.hostname]});
  return result[0];
}

function getAppCacheStatus(msg) {
  sendResponse({ value: curFrame.applicationCache.status },
               msg.json.command_id);
}


let _emu_cb_id = 0;
let _emu_cbs = {};

function runEmulatorCmd(cmd, callback) {
  if (callback) {
    _emu_cbs[_emu_cb_id] = callback;
  }
  sendAsyncMessage("Marionette:runEmulatorCmd", {emulator_cmd: cmd, id: _emu_cb_id});
  _emu_cb_id += 1;
}

function runEmulatorShell(args, callback) {
  if (callback) {
    _emu_cbs[_emu_cb_id] = callback;
  }
  sendAsyncMessage("Marionette:runEmulatorShell", {emulator_shell: args, id: _emu_cb_id});
  _emu_cb_id += 1;
}

function emulatorCmdResult(msg) {
  let message = msg.json;
  if (!sandbox) {
    return;
  }
  let cb = _emu_cbs[message.id];
  delete _emu_cbs[message.id];
  if (!cb) {
    return;
  }
  try {
    cb(message.result);
  } catch (e) {
    sendError(e, -1);
    return;
  }
}

function importScript(msg) {
  let command_id = msg.json.command_id;
  let file;
  if (importedScripts.exists()) {
    file = FileUtils.openFileOutputStream(importedScripts,
        FileUtils.MODE_APPEND | FileUtils.MODE_WRONLY);
  }
  else {
    
    importedScripts.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE,
                                 parseInt("0666", 8));
    file = FileUtils.openFileOutputStream(importedScripts,
                                          FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
    importedScripts.permissions = parseInt("0666", 8); 
  }
  file.write(msg.json.script, msg.json.script.length);
  file.close();
  sendOk(command_id);
}










function takeScreenshot(msg) {
  let node = null;
  if (msg.json.id) {
    try {
      node = elementManager.getKnownElement(msg.json.id, curFrame)
    }
    catch (e) {
      sendResponse(e.message, e.code, e.stack, msg.json.command_id);
      return;
    }
  }
  else {
    node = curFrame;
  }
  let highlights = msg.json.highlights;

  var document = curFrame.document;
  var rect, win, width, height, left, top;
  
  if (node == curFrame) {
    
    win = node;
    if (msg.json.full) {
      
      width = document.body.scrollWidth;
      height = document.body.scrollHeight;
      top = 0;
      left = 0;
    }
    else {
      
      width = document.documentElement.clientWidth;
      height = document.documentElement.clientHeight;
      left = curFrame.pageXOffset;
      top = curFrame.pageYOffset;
    }
  }
  else {
    
    win = node.ownerDocument.defaultView;
    rect = node.getBoundingClientRect();
    width = rect.width;
    height = rect.height;
    top = rect.top;
    left = rect.left;
  }

  var canvas = document.createElementNS("http://www.w3.org/1999/xhtml",
                                        "canvas");
  canvas.width = width;
  canvas.height = height;
  var ctx = canvas.getContext("2d");
  
  ctx.drawWindow(win, left, top, width, height, "rgb(255,255,255)");

  
  
  if (highlights) {
    ctx.lineWidth = "2";
    ctx.strokeStyle = "red";
    ctx.save();

    for (var i = 0; i < highlights.length; ++i) {
      var elem = elementManager.getKnownElement(highlights[i], curFrame);
      rect = elem.getBoundingClientRect();

      var offsetY = -top;
      var offsetX = -left;

      
      ctx.strokeRect(rect.left + offsetX,
                     rect.top + offsetY,
                     rect.width,
                     rect.height);
    }
  }

  
  
  var dataUrl = canvas.toDataURL("image/png", "");
  var data = dataUrl.substring(dataUrl.indexOf(",") + 1);
  sendResponse({value: data}, msg.json.command_id);
}


registerSelf();
