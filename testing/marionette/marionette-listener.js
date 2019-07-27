




let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator);

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);

loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-common.js");
Cu.import("chrome://marionette/content/marionette-elements.js");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let utils = {};
utils.window = content;

loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);
loader.loadSubScript("chrome://marionette/content/marionette-sendkeys.js", utils);

loader.loadSubScript("chrome://specialpowers/content/specialpowersAPI.js");
loader.loadSubScript("chrome://specialpowers/content/specialpowers.js");

let marionetteLogObj = new MarionetteLogObj();

let isB2G = false;

let marionetteTestName;
let winUtil = content.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);
let listenerId = null; 
let curFrame = content;
let previousFrame = null;
let elementManager = new ElementManager([]);
let importedScripts = null;
let inputSource = null;



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

let EVENT_INTERVAL = 30; 

let nextTouchId = 1000;

let touchIds = {};

let multiLast = {};
let lastCoordinates = null;
let isTap = false;
let scrolling = false;

let mouseEventsOnly = false;

Cu.import("resource://gre/modules/Log.jsm");
let logger = Log.repository.getLogger("Marionette");
logger.info("loaded marionette-listener.js");
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
  let msg = {value: winUtil.outerWindowID, href: content.location.href};
  
  let register = sendSyncMessage("Marionette:register", msg);

  if (register[0]) {
    listenerId = register[0][0].id;
    if (typeof listenerId != "undefined") {
      
      if (register[0][1] == true) {
        addMessageListener("MarionetteMainListener:emitTouchEvent", emitTouchEventForIFrame);
      }
      importedScripts = FileUtils.getDir('TmpD', [], false);
      importedScripts.append('marionetteContentScripts');
      startListeners();
    }
  }
}

function emitTouchEventForIFrame(message) {
  message = message.json;
  let frames = curFrame.document.getElementsByTagName("iframe");
  let iframe = frames[message.index];
  let identifier = nextTouchId;
  let tabParent = iframe.QueryInterface(Components.interfaces.nsIFrameLoaderOwner).frameLoader.tabParent;
  tabParent.injectTouchEvent(message.type, [identifier],
                             [message.clientX], [message.clientY],
                             [message.radiusX], [message.radiusY],
                             [message.rotationAngle], [message.force],
                             1, 0);
}




function addMessageListenerId(messageName, handler) {
  addMessageListener(messageName + listenerId, handler);
}




function removeMessageListenerId(messageName, handler) {
  removeMessageListener(messageName + listenerId, handler);
}




function startListeners() {
  addMessageListenerId("Marionette:newSession", newSession);
  addMessageListenerId("Marionette:executeScript", executeScript);
  addMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  addMessageListenerId("Marionette:executeJSScript", executeJSScript);
  addMessageListenerId("Marionette:singleTap", singleTap);
  addMessageListenerId("Marionette:actionChain", actionChain);
  addMessageListenerId("Marionette:multiAction", multiAction);
  addMessageListenerId("Marionette:get", get);
  addMessageListenerId("Marionette:getCurrentUrl", getCurrentUrl);
  addMessageListenerId("Marionette:getTitle", getTitle);
  addMessageListenerId("Marionette:getPageSource", getPageSource);
  addMessageListenerId("Marionette:goBack", goBack);
  addMessageListenerId("Marionette:goForward", goForward);
  addMessageListenerId("Marionette:refresh", refresh);
  addMessageListenerId("Marionette:findElementContent", findElementContent);
  addMessageListenerId("Marionette:findElementsContent", findElementsContent);
  addMessageListenerId("Marionette:getActiveElement", getActiveElement);
  addMessageListenerId("Marionette:clickElement", clickElement);
  addMessageListenerId("Marionette:getElementAttribute", getElementAttribute);
  addMessageListenerId("Marionette:getElementText", getElementText);
  addMessageListenerId("Marionette:getElementTagName", getElementTagName);
  addMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  addMessageListenerId("Marionette:getElementValueOfCssProperty", getElementValueOfCssProperty);
  addMessageListenerId("Marionette:submitElement", submitElement);
  addMessageListenerId("Marionette:getElementSize", getElementSize);
  addMessageListenerId("Marionette:getElementRect", getElementRect);
  addMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
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
  addMessageListenerId("Marionette:ping", ping);
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
  resetValues();
  if (isB2G) {
    readyStateTimer.initWithCallback(waitForReady, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    
    
    
    
    
    inputSource = Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH;
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
  removeMessageListenerId("Marionette:getTitle", getTitle);
  removeMessageListenerId("Marionette:getPageSource", getPageSource);
  removeMessageListenerId("Marionette:getCurrentUrl", getCurrentUrl);
  removeMessageListenerId("Marionette:goBack", goBack);
  removeMessageListenerId("Marionette:goForward", goForward);
  removeMessageListenerId("Marionette:refresh", refresh);
  removeMessageListenerId("Marionette:findElementContent", findElementContent);
  removeMessageListenerId("Marionette:findElementsContent", findElementsContent);
  removeMessageListenerId("Marionette:getActiveElement", getActiveElement);
  removeMessageListenerId("Marionette:clickElement", clickElement);
  removeMessageListenerId("Marionette:getElementAttribute", getElementAttribute);
  removeMessageListenerId("Marionette:getElementText", getElementText);
  removeMessageListenerId("Marionette:getElementTagName", getElementTagName);
  removeMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  removeMessageListenerId("Marionette:getElementValueOfCssProperty", getElementValueOfCssProperty);
  removeMessageListenerId("Marionette:submitElement", submitElement);
  removeMessageListenerId("Marionette:getElementSize", getElementSize);  
  removeMessageListenerId("Marionette:getElementRect", getElementRect);
  removeMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
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
  removeMessageListenerId("Marionette:ping", ping);
  if (isB2G) {
    content.removeEventListener("mozbrowsershowmodalprompt", modalHandler, false);
  }
  elementManager.reset();
  
  curFrame = content;
  curFrame.focus();
  touchIds = {};
}








function sendToServer(msg, value, command_id) {
  if (command_id) {
    value.command_id = command_id;
  }
  sendAsyncMessage(msg, value);
}




function sendResponse(value, command_id) {
  sendToServer("Marionette:done", value, command_id);
}




function sendOk(command_id) {
  sendToServer("Marionette:ok", {}, command_id);
}




function sendLog(msg) {
  sendToServer("Marionette:log", { message: msg });
}




function sendError(message, status, trace, command_id) {
  let error_msg = { message: message, status: status, stacktrace: trace };
  sendToServer("Marionette:error", error_msg, command_id);
}




function resetValues() {
  sandbox = null;
  curFrame = content;
  mouseEventsOnly = false;
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

  sandbox.asyncComplete = function sandbox_asyncComplete(value, status, stack, commandId) {
    if (commandId == asyncTestCommandId) {
      curFrame.removeEventListener("unload", onunload, false);
      curFrame.clearTimeout(asyncTestTimeoutId);

      if (inactivityTimeoutId != null) {
        curFrame.clearTimeout(inactivityTimeoutId);
      }


      sendSyncMessage("Marionette:shareData",
                      {log: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();

      if (status == 0){
        if (Object.keys(_emu_cbs).length) {
          _emu_cbs = {};
          sendError("Emulator callback still pending when finish() called",
                    500, null, commandId);
        }
        else {
          sendResponse({value: elementManager.wrapValue(value), status: status},
                       commandId);
        }
      }
      else {
        sendError(value, status, stack, commandId);
      }

      asyncTestRunning = false;
      asyncTestTimeoutId = undefined;
      asyncTestCommandId = undefined;
      inactivityTimeoutId = null;
    }
  };
  sandbox.finish = function sandbox_finish() {
    if (asyncTestRunning) {
      sandbox.asyncComplete(marionette.generate_results(), 0, null, sandbox.asyncTestCommandId);
    } else {
      return marionette.generate_results();
    }
  };
  sandbox.marionetteScriptFinished = function sandbox_marionetteScriptFinished(value) {
    return sandbox.asyncComplete(value, 0, null, sandbox.asyncTestCommandId);
  };

  return sandbox;
}





function executeScript(msg, directInject) {
  
  if (msg.json.inactivityTimeout) {
    let setTimer = function() {
        inactivityTimeoutId = curFrame.setTimeout(function() {
        sendError('timed out due to inactivity', 28, null, asyncTestCommandId);
      }, msg.json.inactivityTimeout);
   };

    setTimer();
    heartbeatCallback = function resetInactivityTimeout() {
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
      sendError("Could not create sandbox!", 500, null, asyncTestCommandId);
      return;
    }
  }
  else {
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
        sendError("Marionette.finish() not called", 17, null, asyncTestCommandId);
      }
      else {
        sendResponse({value: elementManager.wrapValue(res)}, asyncTestCommandId);
      }
    }
    else {
      try {
        sandbox.__marionetteParams = Cu.cloneInto(elementManager.convertWrappedArguments(
          msg.json.args, curFrame), sandbox, { wrapReflectors: true });
      }
      catch(e) {
        sendError(e.message, e.code, e.stack, asyncTestCommandId);
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
  }
  catch (e) {
    
    let error = createStackMessage(e,
                                   "execute_script",
                                   msg.json.filename,
                                   msg.json.line,
                                   script);
    sendError(error[0], 17, error[1], asyncTestCommandId);
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
        sandbox.asyncComplete('timed out due to inactivity', 28, null, asyncTestCommandId);
      }, msg.json.inactivityTimeout);
    };

    setTimer();
    heartbeatCallback = function resetInactivityTimeout() {
      curFrame.clearTimeout(inactivityTimeoutId);
      setTimer();
    };
  }

  let script = msg.json.script;
  asyncTestCommandId = msg.json.command_id;

  onunload = function() {
    sendError("unload was called", 17, null, asyncTestCommandId);
  };
  curFrame.addEventListener("unload", onunload, false);

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curFrame,
                                          msg.json.timeout);
    if (!sandbox) {
      sendError("Could not create sandbox!", 17, null, asyncTestCommandId);
      return;
    }
  }
  else {
    sandbox.asyncTestCommandId = asyncTestCommandId;
  }
  sandbox.tag = script;

  
  
  
  
  
  asyncTestTimeoutId = curFrame.setTimeout(function() {
    sandbox.asyncComplete('timed out', 28, null, asyncTestCommandId);
  }, msg.json.timeout);

  originalOnError = curFrame.onerror;
  curFrame.onerror = function errHandler(errMsg, url, line) {
    sandbox.asyncComplete(errMsg, 17, "@" + url + ", line " + line, asyncTestCommandId);
    curFrame.onerror = originalOnError;
  };

  let scriptSrc;
  if (useFinish) {
    if (msg.json.timeout == null || msg.json.timeout == 0) {
      sendError("Please set a timeout", 21, null, asyncTestCommandId);
    }
    scriptSrc = script;
  }
  else {
    try {
      sandbox.__marionetteParams = Cu.cloneInto(elementManager.convertWrappedArguments(
        msg.json.args, curFrame), sandbox, { wrapReflectors: true });
    }
    catch(e) {
      sendError(e.message, e.code, e.stack, asyncTestCommandId);
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
    
    let error = createStackMessage(e,
                                   "execute_async_script",
                                   msg.json.filename,
                                   msg.json.line,
                                   scriptSrc);
    sandbox.asyncComplete(error[0], 17, error[1], asyncTestCommandId);
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
    if (docShell.asyncPanZoomEnabled && scrolling) {
      
      let index = sendSyncMessage("MarionetteFrame:getCurrentFrameId");
      
      if (index != null) {
        sendSyncMessage("Marionette:emitTouchEvent", {index: index, type: type, id: touch.identifier,
                                                      clientX: touch.clientX, clientY: touch.clientY,
                                                      radiusX: touch.radiusX, radiusY: touch.radiusY,
                                                      rotation: touch.rotationAngle, force: touch.force});
        return;
      }
    }
    
    






    let domWindowUtils = curFrame.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
    domWindowUtils.sendTouchEvent(type, [touch.identifier], [touch.clientX], [touch.clientY], [touch.radiusX], [touch.radiusY], [touch.rotationAngle], [touch.force], 1, 0);
  }
}








function emitMouseEvent(doc, type, elClientX, elClientY, clickCount, button) {
  if (!wasInterrupted()) {
    let loggingInfo = "emitting Mouse event of type " + type + " at coordinates (" + elClientX + ", " + elClientY + ") relative to the viewport";
    dumpLog(loggingInfo);
    






    let win = doc.defaultView;
    let domUtils = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
    domUtils.sendMouseEvent(type, elClientX, elClientY, button || 0, clickCount || 1, 0, false, 0, inputSource);
  }
}




function mousetap(doc, x, y) {
  emitMouseEvent(doc, 'mousemove', x, y);
  emitMouseEvent(doc, 'mousedown', x, y);
  emitMouseEvent(doc, 'mouseup', x, y);
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


function generateEvents(type, x, y, touchId, target) {
  lastCoordinates = [x, y];
  let doc = curFrame.document;
  switch (type) {
    case 'tap':
      if (mouseEventsOnly) {
        mousetap(target.ownerDocument, x, y);
      }
      else {
        let touchId = nextTouchId++;
        let touch = createATouch(target, x, y, touchId);
        emitTouchEvent('touchstart', touch);
        emitTouchEvent('touchend', touch);
        mousetap(target.ownerDocument, x, y);
      }
      lastCoordinates = null;
      break;
    case 'press':
      isTap = true;
      if (mouseEventsOnly) {
        emitMouseEvent(doc, 'mousemove', x, y);
        emitMouseEvent(doc, 'mousedown', x, y);
      }
      else {
        let touchId = nextTouchId++;
        let touch = createATouch(target, x, y, touchId);
        emitTouchEvent('touchstart', touch);
        touchIds[touchId] = touch;
        return touchId;
      }
      break;
    case 'release':
      if (mouseEventsOnly) {
        emitMouseEvent(doc, 'mouseup', lastCoordinates[0], lastCoordinates[1]);
      }
      else {
        let touch = touchIds[touchId];
        touch = createATouch(touch.target, lastCoordinates[0], lastCoordinates[1], touchId);
        emitTouchEvent('touchend', touch);
        if (isTap) {
          mousetap(touch.target.ownerDocument, touch.clientX, touch.clientY);
        }
        delete touchIds[touchId];
      }
      isTap = false;
      lastCoordinates = null;
      break;
    case 'cancel':
      isTap = false;
      if (mouseEventsOnly) {
        emitMouseEvent(doc, 'mouseup', lastCoordinates[0], lastCoordinates[1]);
      }
      else {
        emitTouchEvent('touchcancel', touchIds[touchId]);
        delete touchIds[touchId];
      }
      lastCoordinates = null;
      break;
    case 'move':
      isTap = false;
      if (mouseEventsOnly) {
        emitMouseEvent(doc, 'mousemove', x, y);
      }
      else {
        touch = createATouch(touchIds[touchId].target, x, y, touchId);
        touchIds[touchId] = touch;
        emitTouchEvent('touchmove', touch);
      }
      break;
    case 'contextmenu':
      isTap = false;
      let event = curFrame.document.createEvent('MouseEvents');
      if (mouseEventsOnly) {
        target = doc.elementFromPoint(lastCoordinates[0], lastCoordinates[1]);
      }
      else {
        target = touchIds[touchId].target;
      }
      let [ clientX, clientY,
            pageX, pageY,
            screenX, screenY ] = getCoordinateInfo(target, x, y);
      event.initMouseEvent('contextmenu', true, true,
                           target.ownerDocument.defaultView, 1,
                           screenX, screenY, clientX, clientY,
                           false, false, false, false, 0, null);
      target.dispatchEvent(event);
      break;
    default:
      throw {message:"Unknown event type: " + type, code: 500, stack:null};
  }
  if (wasInterrupted()) {
    if (previousFrame) {
      
      curFrame = previousFrame;
      previousFrame = null;
      sandbox = null;
    }
    else {
      
      sendSyncMessage("Marionette:switchToModalOrigin");
    }
    sendSyncMessage("Marionette:switchedToFrame", { restorePrevious: true });
  }
}




function singleTap(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    
    if (!checkVisible(el, msg.json.corx, msg.json.cory)) {
       sendError("Element is not currently visible and may not be manipulated", 11, null, command_id);
       return;
    }
    if (!curFrame.document.createTouch) {
      mouseEventsOnly = true;
    }
    c = coordinates(el, msg.json.corx, msg.json.cory);
    generateEvents('tap', c.x, c.y, null, el);
    sendOk(msg.json.command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}





function getCoordinateInfo(el, corx, cory) {
  let win = el.ownerDocument.defaultView;
  return [ corx, 
           cory, 
           corx + win.pageXOffset, 
           cory + win.pageYOffset, 
           corx + win.mozInnerScreenX, 
           cory + win.mozInnerScreenY 
         ];
}





function createATouch(el, corx, cory, touchId) {
  let doc = el.ownerDocument;
  let win = doc.defaultView;
  let [clientX, clientY, pageX, pageY, screenX, screenY] =
    getCoordinateInfo(el, corx, cory);
  let atouch = doc.createTouch(win, el, touchId, pageX, pageY, screenX, screenY, clientX, clientY);
  return atouch;
}





function actions(chain, touchId, command_id, i) {
  if (typeof i === "undefined") {
    i = 0;
  }
  if (i == chain.length) {
    sendResponse({value: touchId}, command_id);
    return;
  }
  let pack = chain[i];
  let command = pack[0];
  let el;
  let c;
  i++;
  if (command != 'press' && command != 'wait') {
    
    if (!(touchId in touchIds) && !mouseEventsOnly) {
      sendError("Element has not been pressed", 500, null, command_id);
      return;
    }
  }
  switch(command) {
    case 'press':
      if (lastCoordinates) {
        generateEvents('cancel', lastCoordinates[0], lastCoordinates[1], touchId);
        sendError("Invalid Command: press cannot follow an active touch event", 500, null, command_id);
        return;
      }
      
      if ((i != chain.length) && (chain[i][0].indexOf('move') !== -1)) {
        scrolling = true;
      }
      el = elementManager.getKnownElement(pack[1], curFrame);
      c = coordinates(el, pack[2], pack[3]);
      touchId = generateEvents('press', c.x, c.y, null, el);
      actions(chain, touchId, command_id, i);
      break;
    case 'release':
      generateEvents('release', lastCoordinates[0], lastCoordinates[1], touchId);
      actions(chain, null, command_id, i);
      scrolling =  false;
      break;
    case 'move':
      el = elementManager.getKnownElement(pack[1], curFrame);
      c = coordinates(el);
      generateEvents('move', c.x, c.y, touchId);
      actions(chain, touchId, command_id, i);
      break;
    case 'moveByOffset':
      generateEvents('move', lastCoordinates[0] + pack[1], lastCoordinates[1] + pack[2], touchId);
      actions(chain, touchId, command_id, i);
      break;
    case 'wait':
      if (pack[1] != null ) {
        let time = pack[1]*1000;
        
        let standard = 750;
        try {
          standard = Services.prefs.getIntPref("ui.click_hold_context_menus.delay");
        }
        catch (e){}
        if (time >= standard && isTap) {
            chain.splice(i, 0, ['longPress'], ['wait', (time-standard)/1000]);
            time = standard;
        }
        checkTimer.initWithCallback(function(){actions(chain, touchId, command_id, i);}, time, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        actions(chain, touchId, command_id, i);
      }
      break;
    case 'cancel':
      generateEvents('cancel', lastCoordinates[0], lastCoordinates[1], touchId);
      actions(chain, touchId, command_id, i);
      scrolling = false;
      break;
    case 'longPress':
      generateEvents('contextmenu', lastCoordinates[0], lastCoordinates[1], touchId);
      actions(chain, touchId, command_id, i);
      break;
  }
}




function actionChain(msg) {
  let command_id = msg.json.command_id;
  let args = msg.json.chain;
  let touchId = msg.json.nextId;
  try {
    let commandArray = elementManager.convertWrappedArguments(args, curFrame);
    
    if (touchId == null) {
      touchId = nextTouchId++;
    }
    if (!curFrame.document.createTouch) {
      mouseEventsOnly = true;
    }
    actions(commandArray, touchId, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
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
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}







function get(msg) {
  let command_id = msg.json.command_id;

  let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  let start = new Date().getTime();
  let end = null;
  function checkLoad() {
    checkTimer.cancel();
    end = new Date().getTime();
    let errorRegex = /about:.+(error)|(blocked)\?/;
    let elapse = end - start;
    if (msg.json.pageTimeout == null || elapse <= msg.json.pageTimeout) {
      if (curFrame.document.readyState == "complete") {
        removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
        sendOk(command_id);
        
        sendToServer("Marionette:startHeartbeat");
      }
      else if (curFrame.document.readyState == "interactive" &&
               errorRegex.exec(curFrame.document.baseURI)) {
        removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
        sendError("Error loading page", 13, null, command_id);
      }
      else {
        checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
      }
    }
    else {
      removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
      sendError("Error loading page, timed out (checkLoad)", 21, null,
                command_id);
    }
  }
  
  
  
  let onDOMContentLoaded = function onDOMContentLoaded(event) {
    if (!event.originalTarget.defaultView.frameElement ||
        event.originalTarget.defaultView.frameElement == curFrame.frameElement) {
      checkLoad();
    }
  };

  function timerFunc() {
    removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
    sendError("Error loading page, timed out (onDOMContentLoaded)", 21,
              null, command_id);
  }
  if (msg.json.pageTimeout != null) {
    checkTimer.initWithCallback(timerFunc, msg.json.pageTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  addEventListener("DOMContentLoaded", onDOMContentLoaded, false);
  curFrame.location = msg.json.url;
}




function getCurrentUrl(msg) {
  sendResponse({value: curFrame.location.href}, msg.json.command_id);
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
    let on_success = function(id, cmd_id) { sendResponse({value:id}, cmd_id); };
    let on_error = sendError;
    elementManager.find(curFrame, msg.json, msg.json.searchTimeout,
                        on_success, on_error, false, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function findElementsContent(msg) {
  let command_id = msg.json.command_id;
  try {
    let on_success = function(id, cmd_id) { sendResponse({value:id}, cmd_id); };
    let on_error = sendError;
    elementManager.find(curFrame, msg.json, msg.json.searchTimeout,
                        on_success, on_error, true, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getActiveElement(msg) {
  let command_id = msg.json.command_id;
  var element = curFrame.document.activeElement;
  var id = elementManager.addToKnownElements(element);
  sendResponse({value: id}, command_id);
}




function clickElement(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.id, curFrame);
    if (checkVisible(el)) {
      if (utils.isElementEnabled(el)) {
        utils.synthesizeMouseAtCenter(el, {}, el.ownerDocument.defaultView)
      }
      else {
        sendError("Element is not Enabled", 12, null, command_id)
      }
    }
    else {
      sendError("Element is not visible", 11, null, command_id)
    }
    sendOk(command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementAttribute(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.getElementAttribute(el, msg.json.name)},
                 command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementText(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.getElementText(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementTagName(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: el.tagName.toLowerCase()}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementDisplayed(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.isElementDisplayed(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}









function getElementValueOfCssProperty(msg){
  let command_id = msg.json.command_id;
  let propertyName = msg.json.propertyName;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: curFrame.document.defaultView.getComputedStyle(el, null).getPropertyValue(propertyName)},
                 command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
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
    }
    else {
      sendError("Element is not a form element or in a form", 7, null, command_id);
    }

  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementSize(msg){
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    let clientRect = el.getBoundingClientRect();
    sendResponse({value: {width: clientRect.width, height: clientRect.height}},
                 command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementRect(msg){
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    let clientRect = el.getBoundingClientRect();
    sendResponse({value: {x: clientRect.x + curFrame.pageXOffset,
                          y: clientRect.y  + curFrame.pageYOffset,
                          width: clientRect.width,
                          height: clientRect.height}},
                 command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementEnabled(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.isElementEnabled(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementSelected(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    sendResponse({value: utils.isElementSelected(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function sendKeysToElement(msg) {
  let command_id = msg.json.command_id;

  let el = elementManager.getKnownElement(msg.json.id, curFrame);
  let keysToSend = msg.json.value;

  utils.sendKeysToElement(curFrame, el, keysToSend, sendOk, sendError, command_id);
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
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function clearElement(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.id, curFrame);
    utils.clearElement(el);
    sendOk(command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}





function switchToFrame(msg) {
  let command_id = msg.json.command_id;
  function checkLoad() {
    let errorRegex = /about:.+(error)|(blocked)\?/;
    if (curFrame.document.readyState == "complete") {
      sendOk(command_id);
      return;
    }
    else if (curFrame.document.readyState == "interactive" && errorRegex.exec(curFrame.document.baseURI)) {
      sendError("Error loading page", 13, null, command_id);
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
      }
      catch(e) {
        sendError(e.message, e.code, e.stack, command_id);
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
    sendError("Unable to locate frame: " + (msg.json.id || msg.json.element), 8, null, command_id);
    return true;
  }

  sandbox = null;

  
  
  let frameValue = elementManager.wrapValue(curFrame.wrappedJSObject)['ELEMENT'];
  sendSyncMessage("Marionette:switchedToFrame", { frameValue: frameValue });

  if (curFrame.contentWindow == null) {
    
    
    curFrame = content;
    sendToServer('Marionette:switchToFrame', {win: parWindow,
                                              frame: foundFrame,
                                              command_id: command_id});
  }
  else {
    curFrame = curFrame.contentWindow;
    if(msg.json.focus == true) {
      curFrame.focus();
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
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
  }
  else {
    var currLocation = curFrame.location;
    var currDomain = currLocation.host;
    if (currDomain.indexOf(cookie.domain) == -1) {
      sendError("You may only set cookies for the current domain", 24, null, msg.json.command_id);
    }
  }

  
  
  
  if (cookie.domain.match(/:\d+$/)) {
    cookie.domain = cookie.domain.replace(/:\d+$/, '');
  }

  var document = curFrame.document;
  if (!document || !document.contentType.match(/html/i)) {
    sendError('You may only set cookies on html documents', 25, null, msg.json.command_id);
  }

  let added = sendSyncMessage("Marionette:addCookie", {value: cookie});
  if (added[0] !== true) {
    sendError("Error setting cookie", 13, null, msg.json.command_id);
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
        sendError("Could not delete cookie: " + msg.json.name, 13, null, msg.json.command_id);
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
      sendError("Could not delete cookie: " + JSON.stringify(cookie), 13, null, msg.json.command_id);
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




function ping(msg) {
  sendToServer("Marionette:pong", {}, msg.json.command_id);
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
  }
  catch(e) {
    sendError(e.message, e.code, e.stack, -1);
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
    width = document.body.scrollWidth;
    height = document.body.scrollHeight;
    top = 0;
    left = 0;
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
