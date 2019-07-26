




let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator);

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);

loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-log-obj.js");
loader.loadSubScript("chrome://marionette/content/marionette-perf.js");
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
let marionettePerf = new MarionettePerfData();

let isB2G = false;

let marionetteTestName;
let winUtil = content.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);
let listenerId = null; 
let activeFrame = null;
let curWindow = content;
let elementManager = new ElementManager([]);
let importedScripts = null;



let sandbox;


let onunload;


let asyncTestRunning = false;
let asyncTestCommandId;
let asyncTestTimeoutId;
let originalOnError;

let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

let EVENT_INTERVAL = 30; 

let touches = [];

let nextTouchId = 1000;
let touchIds = [];





function registerSelf() {
  let msg = {value: winUtil.outerWindowID, href: content.location.href};
  let register = sendSyncMessage("Marionette:register", msg);

  if (register[0]) {
    listenerId = register[0].id;
    importedScripts = FileUtils.File(register[0].importedScripts);
    startListeners();
  }
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
  addMessageListenerId("Marionette:doubleTap", doubleTap);
  addMessageListenerId("Marionette:press", press);
  addMessageListenerId("Marionette:release", release);
  addMessageListenerId("Marionette:setSearchTimeout", setSearchTimeout);
  addMessageListenerId("Marionette:goUrl", goUrl);
  addMessageListenerId("Marionette:getUrl", getUrl);
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
  addMessageListenerId("Marionette:getElementSize", getElementSize);
  addMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
  addMessageListenerId("Marionette:isElementSelected", isElementSelected);
  addMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  addMessageListenerId("Marionette:getElementPosition", getElementPosition);
  addMessageListenerId("Marionette:clearElement", clearElement);
  addMessageListenerId("Marionette:switchToFrame", switchToFrame);
  addMessageListenerId("Marionette:deleteSession", deleteSession);
  addMessageListenerId("Marionette:sleepSession", sleepSession);
  addMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  addMessageListenerId("Marionette:importScript", importScript);
  addMessageListenerId("Marionette:getAppCacheStatus", getAppCacheStatus);
  addMessageListenerId("Marionette:setTestName", setTestName);
  addMessageListenerId("Marionette:setState", setState);
  addMessageListenerId("Marionette:screenShot", screenShot);
  addMessageListenerId("Marionette:addCookie", addCookie);
  addMessageListenerId("Marionette:getAllCookies", getAllCookies);
  addMessageListenerId("Marionette:deleteAllCookies", deleteAllCookies);
  addMessageListenerId("Marionette:deleteCookie", deleteCookie);
}





function newSession(msg) {
  isB2G = msg.json.B2G;
  resetValues();
}
 





function sleepSession(msg) {
  deleteSession();
  addMessageListener("Marionette:restart", restart);
}




function setState(msg) {
  marionetteTimeout = msg.json.scriptTimeout;
  try {
    elementManager.setSearchTimeout(msg.json.searchTimeout);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
    return;
  }
  sendOk(msg.json.command_id);
}




function restart(msg) {
  removeMessageListener("Marionette:restart", restart);
  registerSelf();
}




function deleteSession(msg) {
  removeMessageListenerId("Marionette:newSession", newSession);
  removeMessageListenerId("Marionette:executeScript", executeScript);
  removeMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  removeMessageListenerId("Marionette:executeJSScript", executeJSScript);
  removeMessageListenerId("Marionette:singleTap", singleTap);
  removeMessageListenerId("Marionette:doubleTap", doubleTap);
  removeMessageListenerId("Marionette:press", press);
  removeMessageListenerId("Marionette:release", release);
  removeMessageListenerId("Marionette:setSearchTimeout", setSearchTimeout);
  removeMessageListenerId("Marionette:goUrl", goUrl);
  removeMessageListenerId("Marionette:getTitle", getTitle);
  removeMessageListenerId("Marionette:getPageSource", getPageSource);
  removeMessageListenerId("Marionette:getUrl", getUrl);
  removeMessageListenerId("Marionette:goBack", goBack);
  removeMessageListenerId("Marionette:goForward", goForward);
  removeMessageListenerId("Marionette:refresh", refresh);
  removeMessageListenerId("Marionette:findElementContent", findElementContent);
  removeMessageListenerId("Marionette:findElementsContent", findElementsContent);
  removeMessageListenerId("Marionette:getActiveElement", getActiveElement);
  removeMessageListenerId("Marionette:clickElement", clickElement);
  removeMessageListenerId("Marionette:getElementAttribute", getElementAttribute);
  removeMessageListenerId("Marionette:getElementTagName", getElementTagName);
  removeMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  removeMessageListenerId("Marionette:getElementSize", getElementSize);
  removeMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
  removeMessageListenerId("Marionette:isElementSelected", isElementSelected);
  removeMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  removeMessageListenerId("Marionette:getElementPosition", getElementPosition);
  removeMessageListenerId("Marionette:clearElement", clearElement);
  removeMessageListenerId("Marionette:switchToFrame", switchToFrame);
  removeMessageListenerId("Marionette:deleteSession", deleteSession);
  removeMessageListenerId("Marionette:sleepSession", sleepSession);
  removeMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  removeMessageListenerId("Marionette:importScript", importScript);
  removeMessageListenerId("Marionette:getAppCacheStatus", getAppCacheStatus);
  removeMessageListenerId("Marionette:setTestName", setTestName);
  removeMessageListenerId("Marionette:setState", setState);
  removeMessageListenerId("Marionette:screenShot", screenShot);
  removeMessageListenerId("Marionette:addCookie", addCookie);
  removeMessageListenerId("Marionette:getAllCookies", getAllCookies);
  removeMessageListenerId("Marionette:deleteAllCookies", deleteAllCookies);
  removeMessageListenerId("Marionette:deleteCookie", deleteCookie);
  this.elementManager.reset();
  
  curWindow = content;
  curWindow.focus();
  touches = [];
  touchIds = [];
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
  curWindow = content;
}








function createExecuteContentSandbox(aWindow, timeout) {
  let sandbox = new Cu.Sandbox(aWindow, {sandboxPrototype: aWindow});
  sandbox.global = sandbox;
  sandbox.window = aWindow;
  sandbox.document = sandbox.window.document;
  sandbox.navigator = sandbox.window.navigator;
  sandbox.testUtils = utils;

  let marionette = new Marionette(this, aWindow, "content",
                                  marionetteLogObj, marionettePerf,
                                  timeout, marionetteTestName);
  sandbox.marionette = marionette;
  marionette.exports.forEach(function(fn) {
    try {
      sandbox[fn] = marionette[fn].bind(marionette);
    }
    catch(e) {
      sandbox[fn] = marionette[fn];
    }
  });

  XPCOMUtils.defineLazyGetter(sandbox, 'SpecialPowers', function() {
    return new SpecialPowers(aWindow);
  });

  sandbox.asyncComplete = function sandbox_asyncComplete(value, status) {
    curWindow.removeEventListener("unload", onunload, false);

    curWindow.clearTimeout(asyncTestTimeoutId);

    sendSyncMessage("Marionette:shareData", {log: elementManager.wrapValue(marionetteLogObj.getLogs()),
                                             perf: elementManager.wrapValue(marionettePerf.getPerfData())});
    marionetteLogObj.clearLogs();
    marionettePerf.clearPerfData();

    if (status == 0){
      if (Object.keys(_emu_cbs).length) {
        _emu_cbs = {};
        sendError("Emulator callback still pending when finish() called",
                  500, null, asyncTestCommandId);
      }
      else {
        sendResponse({value: elementManager.wrapValue(value), status: status},
                     asyncTestCommandId);
      }
    }
    else {
      sendError(value, status, null, asyncTestCommandId);
    }

    asyncTestRunning = false;
    asyncTestTimeoutId = undefined;
    asyncTestCommandId = undefined;
  };
  sandbox.finish = function sandbox_finish() {
    if (asyncTestRunning) {
      sandbox.asyncComplete(marionette.generate_results(), 0);
    } else {
      return marionette.generate_results();
    }
  };
  sandbox.marionetteScriptFinished = function sandbox_marionetteScriptFinished(value) {
    return sandbox.asyncComplete(value, 0);
  };

  return sandbox;
}





function executeScript(msg, directInject) {
  asyncTestCommandId = msg.json.command_id;
  let script = msg.json.value;

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curWindow,
                                          msg.json.timeout);
    if (!sandbox) {
      sendError("Could not create sandbox!", 500, null, asyncTestCommandId);
      return;
    }
  }

  try {
    if (directInject) {
      if (importedScripts.exists()) {
        let stream = Components.classes["@mozilla.org/network/file-input-stream;1"].  
                      createInstance(Components.interfaces.nsIFileInputStream);
        stream.init(importedScripts, -1, 0, 0);
        let data = NetUtil.readInputStreamToString(stream, stream.available());
        script = data + script;
      }
      let res = Cu.evalInSandbox(script, sandbox, "1.8");
      sendSyncMessage("Marionette:shareData", {log: elementManager.wrapValue(marionetteLogObj.getLogs()),
                                               perf: elementManager.wrapValue(marionettePerf.getPerfData())});
      marionetteLogObj.clearLogs();
      marionettePerf.clearPerfData();
      if (res == undefined || res.passed == undefined) {
        sendError("Marionette.finish() not called", 17, null, asyncTestCommandId);
      }
      else {
        sendResponse({value: elementManager.wrapValue(res)}, asyncTestCommandId);
      }
    }
    else {
      try {
        sandbox.__marionetteParams = elementManager.convertWrappedArguments(
          msg.json.args, curWindow);
      }
      catch(e) {
        sendError(e.message, e.code, e.stack, asyncTestCommandId);
        return;
      }

      let scriptSrc = "let __marionetteFunc = function(){" + script + "};" +
                      "__marionetteFunc.apply(null, __marionetteParams);";
      if (importedScripts.exists()) {
        let stream = Components.classes["@mozilla.org/network/file-input-stream;1"].  
                      createInstance(Components.interfaces.nsIFileInputStream);
        stream.init(importedScripts, -1, 0, 0);
        let data = NetUtil.readInputStreamToString(stream, stream.available());
        scriptSrc = data + scriptSrc;
      }
      let res = Cu.evalInSandbox(scriptSrc, sandbox, "1.8");
      sendSyncMessage("Marionette:shareData", {log: elementManager.wrapValue(marionetteLogObj.getLogs()),
                                               perf: elementManager.wrapValue(marionettePerf.getPerfData())});
      marionetteLogObj.clearLogs();
      marionettePerf.clearPerfData();
      sendResponse({value: elementManager.wrapValue(res)}, asyncTestCommandId);
    }
  }
  catch (e) {
    
    sendError(e.name + ': ' + e.message, 17, e.stack, asyncTestCommandId);
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
  let script = msg.json.value;
  asyncTestCommandId = msg.json.command_id;

  onunload = function() {
    sendError("unload was called", 17, null, asyncTestCommandId);
  };
  curWindow.addEventListener("unload", onunload, false);

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curWindow,
                                          msg.json.timeout);
    if (!sandbox) {
      sendError("Could not create sandbox!", 17, null, asyncTestCommandId);
      return;
    }
  }
  sandbox.tag = script;

  
  
  
  
  
  asyncTestTimeoutId = curWindow.setTimeout(function() {
    sandbox.asyncComplete('timed out', 28, null, asyncTestCommandId);
  }, msg.json.timeout);

  originalOnError = curWindow.onerror;
  curWindow.onerror = function errHandler(errMsg, url, line) {
    sandbox.asyncComplete(errMsg, 17, null, asyncTestCommandId);
    curWindow.onerror = originalOnError;
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
      sandbox.__marionetteParams = elementManager.convertWrappedArguments(
        msg.json.args, curWindow);
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
      scriptSrc = data + scriptSrc;
    }
    Cu.evalInSandbox(scriptSrc, sandbox, "1.8");
  } catch (e) {
    
    sandbox.asyncComplete(e.name + ': ' + e.message, 17,
                          e.stack, asyncTestCommandId);
  }
}




function emitTouchEvent(type, touch) {
  var target = touch.target;
  var doc = target.ownerDocument;
  var win = doc.defaultView;
  
  var domWindowUtils = curWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
  domWindowUtils.sendTouchEvent(type, [touch.identifier], [touch.screenX], [touch.screenY], [touch.radiusX], [touch.radiusY], [touch.rotationAngle], [touch.force], 1, 0);
}





function touch(target, duration, xt, yt, then) {
  var doc = target.ownerDocument;
  var win = doc.defaultView;
  var touchId = nextTouchId++;
  var x = xt;
  if (typeof xt !== 'function') {
    x = function(t) { return xt[0] + t / duration * (xt[1] - xt[0]); };
  }
  var y = yt;
  if (typeof yt !== 'function') {
    y = function(t) { return yt[0] + t / duration * (yt[1] - yt[0]); };
  }
  
  var clientX = Math.round(x(0)), clientY = Math.round(y(0));
  
  var pageX = clientX + win.pageXOffset,
      pageY = clientY + win.pageYOffset;
  
  var screenX = clientX + win.mozInnerScreenX,
      screenY = clientY + win.mozInnerScreenY;
  
  var lastX = clientX, lastY = clientY;
  
  var touch = doc.createTouch(win, target, touchId,
                              pageX, pageY,
                              screenX, screenY,
                              clientX, clientY);
  
  touches.push(touch);
  
  emitTouchEvent('touchstart', touch);
  var startTime = Date.now();
  checkTimer.initWithCallback(nextEvent, EVENT_INTERVAL, Ci.nsITimer.TYPE_ONE_SHOT);
  function nextEvent() {
  
    var time = Date.now();
    var dt = time - startTime;
    var last = dt + EVENT_INTERVAL / 2 > duration;
    
    
    var touchIndex = touches.indexOf(touch);
    
    if (last)
       dt = duration;
    
    clientX = Math.round(x(dt));
    clientY = Math.round(y(dt));
    
    if (clientX !== lastX || clientY !== lastY) { 
      lastX = clientX;
      lastY = clientY;
      pageX = clientX + win.pageXOffset;
      pageY = clientY + win.pageYOffset;
      screenX = clientX + win.mozInnerScreenX;
      screenY = clientY + win.mozInnerScreenY;
      
      
      touch = doc.createTouch(win, target, touchId,
                              pageX, pageY,
                              screenX, screenY,
                              clientX, clientY);
      
      touches[touchIndex] = touch;
      
      emitTouchEvent('touchmove', touch);
    }
    
    
    if (last) {
      touches.splice(touchIndex, 1);
      emitTouchEvent('touchend', touch);
      if (then)
        checkTimer.initWithCallback(then, 0, Ci.nsITimer.TYPE_ONE_SHOT);
    }
    
    else {
      checkTimer.initWithCallback(nextEvent, EVENT_INTERVAL, Ci.nsITimer.TYPE_ONE_SHOT);
    }
  }
}






function coordinates(target, x0, y0, x1, y1) {
  var coords = {};
  var box = target.getBoundingClientRect();
  var tx0 = typeof x0;
  var ty0 = typeof y0;
  var tx1 = typeof x1;
  var ty1 = typeof y1; 
  function percent(s, x) {
    s = s.trim();
    var f = parseFloat(s);
    if (s[s.length - 1] === '%')
      f = f * x / 100;
      return f;
  }
  function relative(s, x) {
    var factor;
    if (s[0] === '+')
      factor = 1;
    else
      factor = -1;
      return factor * percent(s.substring(1), x);
  }
  if (tx0 === 'number')
    coords.x0 = box.left + x0;
  else if (tx0 === 'string')
    coords.x0 = box.left + percent(x0, box.width);
  
  if (tx1 === 'number')
    coords.x1 = box.left + x1;
  else if (tx1 === 'string') {
    x1 = x1.trim();
    if (x1[0] === '+' || x1[0] === '-')
      coords.x1 = coords.x0 + relative(x1, box.width);
    else
      coords.x1 = box.left + percent(x1, box.width);
  }
  
  if (ty0 === 'number')
    coords.y0 = box.top + y0;
  else if (ty0 === 'string')
    coords.y0 = box.top + percent(y0, box.height);
  
  if (ty1 === 'number')
    coords.y1 = box.top + y1;
  else if (ty1 === 'string') {
    y1 = y1.trim();
    if (y1[0] === '+' || y1[0] === '-')
      coords.y1 = coords.y0 + relative(y1, box.height);
    else
      coords.y1 = box.top + percent(y1, box.height);
  }
  return coords;
}




function elementInViewport(el) {
  var top = el.offsetTop;
  var left = el.offsetLeft;
  var width = el.offsetWidth;
  var height = el.offsetHeight;
  while(el.offsetParent) {
    el = el.offsetParent;
    top += el.offsetTop;
    left += el.offsetLeft;
  }
  return (top >= curWindow.pageYOffset &&
          left >= curWindow.pageXOffset &&
          (top + height) <= (curWindow.pageYOffset + curWindow.innerHeight) &&
          (left + width) <= (curWindow.pageXOffset + curWindow.innerWidth)
         );
}




function checkVisible(el, command_id) {
    
    let visible = utils.isElementDisplayed(el);
    if (!visible) {
      return false;
    }
    
    if (el.scrollIntoView) {
      el.scrollIntoView(true);
    }
    var scroll = elementInViewport(el);
    if (!scroll){
      return false;
    }
    return true;
}




function singleTap(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.value, curWindow);
    let x = msg.json.corx;
    let y = msg.json.cory;
    if (!checkVisible(el, command_id)) {
      sendError("Element is not currently visible and may not be manipulated", 11, null, command_id);
      return;
    }
    if (x == null) {
      x = '50%';
    }
    if (y == null) {
      y = '50%';
    }
    let c = coordinates(el, x, y);
    touch(el, 3000, [c.x0, c.x0], [c.y0, c.y0], null);
    sendOk(msg.json.command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}




function doubleTap(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.value, curWindow);
    let x = msg.json.corx;
    let y = msg.json.cory;
    if (!checkVisible(el, command_id)) {
      sendError("Element is not currently visible and may not be manipulated", 11, null, command_id);
      return;
    }
    if (x == null){
      x = '50%';
    }
    if (y == null){
      y = '50%';
    }
    let c = coordinates(el, x, y);
    touch(el, 25, [c.x0, c.x0], [c.y0, c.y0], function() {
      
      checkTimer.initWithCallback(function() {
          
          touch(el, 25, [c.x0, c.x0], [c.y0, c.y0], null);
      }, 50, Ci.nsITimer.TYPE_ONE_SHOT);
    });
    sendOk(msg.json.command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}




function createATouch(el, corx, cory, id) {
  var doc = el.ownerDocument;
  var win = doc.defaultView;
  if (corx == null) {
    corx = '50%';
  }
  if (cory == null){
    cory = '50%';
  }
  var c = coordinates(el, corx, cory);
  var clientX = Math.round(c.x0),
      clientY = Math.round(c.y0);
  var pageX = clientX + win.pageXOffset,
      pageY = clientY + win.pageYOffset;
  var screenX = clientX + win.mozInnerScreenX,
      screenY = clientY + win.mozInnerScreenY;
  var atouch = doc.createTouch(win, el, id, pageX, pageY, screenX, screenY, clientX, clientY);
  return atouch;
}




function press(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.value, curWindow);
    let corx = msg.json.corx;
    let cory = msg.json.cory;
    if (!checkVisible(el, command_id)) {
      sendError("Element is not currently visible and may not be manipulated", 11, null, command_id);
      return;
    }
    var touchId = nextTouchId++;
    var touch = createATouch(el, corx, cory, touchId);
    emitTouchEvent('touchstart', touch);
    touchIds.push(touchId);
    sendResponse({value: touch.identifier}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}




function release(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    let id = msg.json.touchId;
    let currentIndex = touchIds.indexOf(id);
    if (currentIndex != -1) {
      el = elementManager.getKnownElement(msg.json.value, curWindow);
      let corx = msg.json.corx;
      let cory = msg.json.cory;
      if (!checkVisible(el, command_id)) {
        sendError("Element is not currently visible and may not be manipulated", 11, null, command_id);
        return;
      }
      var touch = createATouch(el, corx, cory, id);
      emitTouchEvent('touchend', touch);
      touchIds.splice(currentIndex, 1);
      sendOk(msg.json.command_id);
    }
    else {
      sendError("Element has not be pressed: InvalidElementCoordinates", 29, null, command_id);
      return;
    }
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
  }
}




function setSearchTimeout(msg) {
  try {
    elementManager.setSearchTimeout(msg.json.value);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, msg.json.command_id);
    return;
  }
  sendOk(msg.json.command_id);
}





function goUrl(msg) {
  let command_id = msg.json.command_id;
  
  
  
  let onDOMContentLoaded = function onDOMContentLoaded(event){
    if (msg.json.pageTimeout != null){
      checkTimer.cancel();
    }
    if (!event.originalTarget.defaultView.frameElement ||
      event.originalTarget.defaultView.frameElement == curWindow.frameElement) {
      removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
      let errorRegex = /about:.+(error)|(blocked)\?/;
      if (curWindow.document.readyState == "interactive" && errorRegex.exec(curWindow.document.baseURI)) {
        sendError("Error loading page", 13, null, command_id);
        return;
      }
      sendOk(command_id);
    }
  };
  function timerFunc(){
    sendError("Error loading page", 13, null, command_id);
    removeEventListener("DOMContentLoaded", onDOMContentLoaded, false);
  }
  if (msg.json.pageTimeout != null){
    checkTimer.initWithCallback(timerFunc, msg.json.pageTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  addEventListener("DOMContentLoaded", onDOMContentLoaded, false);
  curWindow.location = msg.json.value;
}




function getUrl(msg) {
  sendResponse({value: curWindow.location.href}, msg.json.command_id);
}




function getTitle(msg) {
  sendResponse({value: curWindow.top.document.title}, msg.json.command_id);
}




function getPageSource(msg) {
  var XMLSerializer = curWindow.XMLSerializer;
  var pageSource = new XMLSerializer().serializeToString(curWindow.document);
  sendResponse({value: pageSource}, msg.json.command_id);
}




function goBack(msg) {
  curWindow.history.back();
  sendOk(msg.json.command_id);
}




function goForward(msg) {
  curWindow.history.forward();
  sendOk(msg.json.command_id);
}




function refresh(msg) {
  let command_id = msg.json.command_id;
  curWindow.location.reload(true);
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
    elementManager.find(curWindow, msg.json, on_success, on_error, false, command_id);
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
    elementManager.find(curWindow, msg.json, on_success, on_error, true, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getActiveElement(msg) {
  let command_id = msg.json.command_id;
  var element = curWindow.document.activeElement;
  var id = elementManager.addToKnownElements(element);
  sendResponse({value: id}, command_id);
}




function clickElement(msg) {
  let command_id = msg.json.command_id;
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.click(el);
    sendOk(command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementAttribute(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
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
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.getElementText(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementTagName(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: el.tagName.toLowerCase()}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementDisplayed(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementDisplayed(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementSize(msg){
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    let clientRect = el.getBoundingClientRect();  
    sendResponse({value: {width: clientRect.width, height: clientRect.height}},
                 command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementEnabled(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementEnabled(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function isElementSelected(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementSelected(el)}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function sendKeysToElement(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.type(curWindow.document, el, msg.json.value.join(""), true);
    sendOk(command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function getElementPosition(msg) {
  let command_id = msg.json.command_id;
  try{
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    var x = el.offsetLeft;
    var y = el.offsetTop;
    var elementParent = el.offsetParent;
    while (elementParent != null) {
      if (elementParent.tagName == "TABLE") {
        var parentBorder = parseInt(elementParent.border);
        if (isNaN(parentBorder)) {
          var parentFrame = elementParent.getAttribute('frame');
          if (parentFrame != null) {
            x += 1;
            y += 1;
          }
        } else if (parentBorder > 0) {
          x += parentBorder;
          y += parentBorder;
        }
      }
      x += elementParent.offsetLeft;
      y += elementParent.offsetTop;
      elementParent = elementParent.offsetParent;
    }

    let location = {};
    location.x = x;
    location.y = y;

    sendResponse({value: location}, command_id);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack, command_id);
  }
}




function clearElement(msg) {
  let command_id = msg.json.command_id;
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
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
    if (curWindow.document.readyState == "complete") {
      sendOk(command_id);
      return;
    } 
    else if (curWindow.document.readyState == "interactive" && errorRegex.exec(curWindow.document.baseURI)) {
      sendError("Error loading page", 13, null, command_id);
      return;
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  let foundFrame = null;
  let frames = []; 
  let parWindow = null; 
  
  try {
    frames = curWindow.document.getElementsByTagName("iframe");
    
    
    parWindow = curWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  } catch (e) {
    
    
    
    msg.json.value = null;
    msg.json.element = null;
  }
  if ((msg.json.value == null) && (msg.json.element == null)) {
    curWindow = content;
    if(msg.json.focus == true) {
      curWindow.focus();
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    return;
  }
  if (msg.json.element != undefined) {
    if (elementManager.seenItems[msg.json.element] != undefined) {
      let wantedFrame;
      try {
        wantedFrame = elementManager.getKnownElement(msg.json.element, curWindow); 
      }
      catch(e) {
        sendError(e.message, e.code, e.stack, command_id);
      }
      for (let i = 0; i < frames.length; i++) {
        
        if (XPCNativeWrapper(frames[i]) == XPCNativeWrapper(wantedFrame)) {
          curWindow = frames[i]; 
          foundFrame = i;
        }
      }
    }
  }
  if (foundFrame == null) {
    switch(typeof(msg.json.value)) {
      case "string" :
        let foundById = null;
        for (let i = 0; i < frames.length; i++) {
          
          let frame = frames[i];
          let name = utils.getElementAttribute(frame, 'name');
          let id = utils.getElementAttribute(frame, 'id');
          if (name == msg.json.value) {
            foundFrame = i;
            break;
          } else if ((foundById == null) && (id == msg.json.value)) {
            foundById = i;
          }
        }
        if ((foundFrame == null) && (foundById != null)) {
          foundFrame = foundById;
          curWindow = frames[foundFrame];
        }
        break;
      case "number":
        if (frames[msg.json.value] != undefined) {
          foundFrame = msg.json.value;
          curWindow = frames[foundFrame];
        }
        break;
    }
  }
  if (foundFrame == null) {
    sendError("Unable to locate frame: " + msg.json.value, 8, null, command_id);
    return;
  }

  sandbox = null;

  if (curWindow.contentWindow == null) {
    
    
    curWindow = content;
    sendToServer('Marionette:switchToFrame', {frame: foundFrame,
                                              win: parWindow,
                                              command_id: command_id});
  }
  else {
    curWindow = curWindow.contentWindow;
    if(msg.json.focus == true) {
      curWindow.focus();
    }
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
}
 


function addCookie(msg) {
  cookie = msg.json.cookie;

  if (!cookie.expiry) {
    var date = new Date();
    var thePresent = new Date(Date.now());
    date.setYear(thePresent.getFullYear() + 20);
    cookie.expiry = date.getTime() / 1000;  
  }

  if (!cookie.domain) {
    var location = curWindow.document.location;
    cookie.domain = location.hostname;
  }
  else {
    var currLocation = curWindow.location;
    var currDomain = currLocation.host;
    if (currDomain.indexOf(cookie.domain) == -1) {
      sendError("You may only set cookies for the current domain", 24, null, msg.json.command_id);
    }
  }

  
  
  
  if (cookie.domain.match(/:\d+$/)) {
    cookie.domain = cookie.domain.replace(/:\d+$/, '');
  }

  var document = curWindow.document;
  if (!document || !document.contentType.match(/html/i)) {
    sendError('You may only set cookies on html documents', 25, null, msg.json.command_id);
  }
  var cookieManager = Cc['@mozilla.org/cookiemanager;1'].
                        getService(Ci.nsICookieManager2);
  cookieManager.add(cookie.domain, cookie.path, cookie.name, cookie.value,
                   cookie.secure, false, false, cookie.expiry);
  sendOk(msg.json.command_id);
}




function getAllCookies(msg) {
  var toReturn = [];
  var cookies = getVisibleCookies(curWindow.location);
  for (var i = 0; i < cookies.length; i++) {
    var cookie = cookies[i];
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
  var toDelete = msg.json.name;
  var cookieManager = Cc['@mozilla.org/cookiemanager;1'].
                        getService(Ci.nsICookieManager);

  var cookies = getVisibleCookies(curWindow.location);
  for (var i = 0; i < cookies.length; i++) {
    var cookie = cookies[i];
    if (cookie.name == toDelete) {
      cookieManager.remove(cookie.host, cookie.name, cookie.path, false);
    }
  }

  sendOk(msg.json.command_id);
}




function deleteAllCookies(msg) {
  let cookieManager = Cc['@mozilla.org/cookiemanager;1'].
                        getService(Ci.nsICookieManager);
  let cookies = getVisibleCookies(curWindow.location);
  for (let i = 0; i < cookies.length; i++) {
    let cookie = cookies[i];
    cookieManager.remove(cookie.host, cookie.name, cookie.path, false);
  }
  sendOk(msg.json.command_id);
}




function getVisibleCookies(location) {
  let results = [];
  let currentPath = location.pathname;
  if (!currentPath) currentPath = '/';
  let isForCurrentPath = function(aPath) {
    return currentPath.indexOf(aPath) != -1;
  }

  let cookieManager = Cc['@mozilla.org/cookiemanager;1'].
                        getService(Ci.nsICookieManager);
  let enumerator = cookieManager.enumerator;
  while (enumerator.hasMoreElements()) {
    let cookie = enumerator.getNext().QueryInterface(Ci['nsICookie']);

    
    let hostname = location.hostname;
    do {
      if ((cookie.host == '.' + hostname || cookie.host == hostname)
          && isForCurrentPath(cookie.path)) {
          results.push(cookie);
          break;
      }
      hostname = hostname.replace(/^.*?\./, '');
    } while (hostname.indexOf('.') != -1);
  }

  return results;
}

function getAppCacheStatus(msg) {
  sendResponse({ value: curWindow.applicationCache.status },
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




function screenShot(msg) {
  let node = null;
  if (msg.json.element) {
    try {
      node = elementManager.getKnownElement(msg.json.element, curWindow)
    }
    catch (e) {
      sendResponse(e.message, e.code, e.stack, msg.json.command_id);
      return;
    }
  }
  else {
      node = curWindow;
  }
  let highlights = msg.json.highlights;

  var document = curWindow.document;
  var rect, win, width, height, left, top, needsOffset;
  
  if (node == curWindow) {
    
    win = node;
    width = win.innerWidth;
    height = win.innerHeight;
    top = 0;
    left = 0;
    
    needsOffset = true;
  }
  else {
    
    win = node.ownerDocument.defaultView;
    rect = node.getBoundingClientRect();
    width = rect.width;
    height = rect.height;
    top = rect.top;
    left = rect.left;
    
    needsOffset = false;
  }

  var canvas = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  canvas.width = width;
  canvas.height = height;
  var ctx = canvas.getContext("2d");
  
  ctx.drawWindow(win, left, top, width, height, 'rgb(255,255,255)');

  
  if (highlights) {
    ctx.lineWidth = "2";
    ctx.strokeStyle = "red";
    ctx.save();

    for (var i = 0; i < highlights.length; ++i) {
      var elem = highlights[i];
      rect = elem.getBoundingClientRect();

      var offsetY = 0, offsetX = 0;
      if (needsOffset) {
        var offset = getChromeOffset(elem);
        offsetX = offset.x;
        offsetY = offset.y;
      } else {
        
        offsetY = -top;
        offsetX = -left;
      }

      
      ctx.strokeRect(rect.left + offsetX, rect.top + offsetY, rect.width, rect.height);
    }
  }

  
  
  sendResponse({value:canvas.toDataURL("image/png","")}, msg.json.command_id);
}


registerSelf();

