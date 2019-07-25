




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

let marionetteTimeout = null;
let winUtil = content.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);
let listenerId = null; 
let activeFrame = null;
let curWindow = content;
let elementManager = new ElementManager([]);
let importedScripts = FileUtils.getFile('TmpD', ['marionettescript']);



let sandbox;


let asyncTestRunning = false;
let asyncTestCommandId;
let asyncTestTimeoutId;






function registerSelf() {
  let register = sendSyncMessage("Marionette:register", {value: winUtil.outerWindowID, href: content.location.href});
  
  if (register[0]) {
    listenerId = register[0];
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
  addMessageListenerId("Marionette:setScriptTimeout", setScriptTimeout);
  addMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  addMessageListenerId("Marionette:executeJSScript", executeJSScript);
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
  addMessageListenerId("Marionette:clickElement", clickElement);
  addMessageListenerId("Marionette:getElementAttribute", getElementAttribute);
  addMessageListenerId("Marionette:getElementText", getElementText);
  addMessageListenerId("Marionette:getElementTagName", getElementTagName);
  addMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  addMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
  addMessageListenerId("Marionette:isElementSelected", isElementSelected);
  addMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  addMessageListenerId("Marionette:clearElement", clearElement);
  addMessageListenerId("Marionette:switchToFrame", switchToFrame);
  addMessageListenerId("Marionette:deleteSession", deleteSession);
  addMessageListenerId("Marionette:sleepSession", sleepSession);
  addMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  addMessageListenerId("Marionette:importScript", importScript);
}





function newSession(msg) {
  isB2G = msg.json.B2G;
  resetValues();
}
 





function sleepSession(msg) {
  deleteSession();
  addMessageListener("Marionette:restart", restart);
}




function restart() {
  removeMessageListener("Marionette:restart", restart);
  registerSelf();
}




function deleteSession(msg) {
  removeMessageListenerId("Marionette:newSession", newSession);
  removeMessageListenerId("Marionette:executeScript", executeScript);
  removeMessageListenerId("Marionette:setScriptTimeout", setScriptTimeout);
  removeMessageListenerId("Marionette:executeAsyncScript", executeAsyncScript);
  removeMessageListenerId("Marionette:executeJSScript", executeJSScript);
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
  removeMessageListenerId("Marionette:clickElement", clickElement);
  removeMessageListenerId("Marionette:getElementAttribute", getElementAttribute);
  removeMessageListenerId("Marionette:getElementTagName", getElementTagName);
  removeMessageListenerId("Marionette:isElementDisplayed", isElementDisplayed);
  removeMessageListenerId("Marionette:isElementEnabled", isElementEnabled);
  removeMessageListenerId("Marionette:isElementSelected", isElementSelected);
  removeMessageListenerId("Marionette:sendKeysToElement", sendKeysToElement);
  removeMessageListenerId("Marionette:clearElement", clearElement);
  removeMessageListenerId("Marionette:switchToFrame", switchToFrame);
  removeMessageListenerId("Marionette:deleteSession", deleteSession);
  removeMessageListenerId("Marionette:sleepSession", sleepSession);
  removeMessageListenerId("Marionette:emulatorCmdResult", emulatorCmdResult);
  removeMessageListenerId("Marionette:importScript", importScript);
  this.elementManager.reset();
  try {
    importedScripts.remove(false);
  }
  catch (e) {
  }
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
  marionetteTimeout = null;
  curWin = content;
}




function errUnload() {
  sendError("unload was called", 17, null);
}








function createExecuteContentSandbox(aWindow) {
  let sandbox = new Cu.Sandbox(aWindow);
  sandbox.global = sandbox;
  sandbox.window = aWindow;
  sandbox.document = sandbox.window.document;
  sandbox.navigator = sandbox.window.navigator;
  sandbox.__proto__ = sandbox.window;
  sandbox.testUtils = utils;

  let marionette = new Marionette(this, aWindow, "content", marionetteLogObj, marionettePerf);
  sandbox.marionette = marionette;
  marionette.exports.forEach(function(fn) {
    sandbox[fn] = marionette[fn].bind(marionette);
  });

  sandbox.SpecialPowers = new SpecialPowers(aWindow);

  sandbox.asyncComplete = function sandbox_asyncComplete(value, status) {
    if (Object.keys(_emu_cbs).length) {
      _emu_cbs = {};
      value = "Emulator callback still pending when finish() called";
      status = 500;
    }

    curWindow.removeEventListener("unload", errUnload, false);

    
    for (let i = 0; i <= asyncTestTimeoutId; i++) {
      curWindow.clearTimeout(i);
    }

    sendSyncMessage("Marionette:shareData", {log: elementManager.wrapValue(marionetteLogObj.getLogs()),
                                             perf: elementManager.wrapValue(marionettePerf.getPerfData())});
    marionetteLogObj.clearLogs();
    marionettePerf.clearPerfData();
    if (status == 0){
      sendResponse({value: elementManager.wrapValue(value), status: status}, asyncTestCommandId);
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
  let script = msg.json.value;

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curWindow);
    if (!sandbox) {
      sendError("Could not create sandbox!");
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
        sendError("Marionette.finish() not called", 17, null);
      }
      else {
        sendResponse({value: elementManager.wrapValue(res)});
      }
    }
    else {
      try {
        sandbox.__marionetteParams = elementManager.convertWrappedArguments(
          msg.json.args, curWindow);
      }
      catch(e) {
        sendError(e.message, e.code, e.stack);
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
      sendResponse({value: elementManager.wrapValue(res)});
    }
  }
  catch (e) {
    
    sendError(e.name + ': ' + e.message, 17, e.stack);
  }
}




function setScriptTimeout(msg) {
  marionetteTimeout = msg.json.value;
  sendOk();
}




function executeAsyncScript(msg) {
  executeWithCallback(msg);
}




function executeJSScript(msg) {
  if (msg.json.timeout) {
    executeWithCallback(msg, msg.json.timeout);
  }
  else {
    executeScript(msg, true);
  }
}









function executeWithCallback(msg, timeout) {
  curWindow.addEventListener("unload", errUnload, false);
  let script = msg.json.value;
  asyncTestCommandId = msg.json.id;

  if (msg.json.newSandbox || !sandbox) {
    sandbox = createExecuteContentSandbox(curWindow);
    if (!sandbox) {
      sendError("Could not create sandbox!");
      return;
    }
  }

  
  
  
  
  
  asyncTestTimeoutId = curWindow.setTimeout(function() {
    sandbox.asyncComplete('timed out', 28);
  }, marionetteTimeout);
  sandbox.marionette.timeout = marionetteTimeout;

  curWindow.addEventListener('error', function win__onerror(evt) {
    curWindow.removeEventListener('error', win__onerror, true);
    sandbox.asyncComplete(evt, 17);
    return true;
  }, true);

  let scriptSrc;
  if (timeout) {
    if (marionetteTimeout == null || marionetteTimeout == 0) {
      sendError("Please set a timeout", 21, null);
    }
    scriptSrc = script;
  }
  else {
    try {
      sandbox.__marionetteParams = elementManager.convertWrappedArguments(
        msg.json.args, curWindow);
    }
    catch(e) {
      sendError(e.message, e.code, e.stack);
      return;
    }

    scriptSrc = "__marionetteParams.push(marionetteScriptFinished);" +
                "let __marionetteFunc = function() { " + script + "};" +
                "__marionetteFunc.apply(null, __marionetteParams); ";
  }

  try {
    asyncTestRunning = true;
    if (importedScripts.exists()) {
      let stream = Components.classes["@mozilla.org/network/file-input-stream;1"].  
                      createInstance(Components.interfaces.nsIFileInputStream);
      stream.init(importedScripts, -1, 0, 0);
      let data = NetUtil.readInputStreamToString(stream, stream.available());
      scriptSrc = data + scriptSrc;
    }
    Cu.evalInSandbox(scriptSrc, sandbox, "1.8");
  } catch (e) {
    
    sendError(e.name + ': ' + e.message, 17, e.stack);
  }
}




function setSearchTimeout(msg) {
  try {
    elementManager.setSearchTimeout(msg.json.value);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
    return;
  }
  sendOk();
}





function goUrl(msg) {
  curWindow.location = msg.json.value;
  
  let checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  function checkLoad() { 
    if (curWindow.document.readyState == "complete") { 
      sendOk();
      return;
    } 
    checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
  }
  checkTimer.initWithCallback(checkLoad, 100, Ci.nsITimer.TYPE_ONE_SHOT);
}




function getUrl(msg) {
  sendResponse({value: curWindow.location.href});
}




function getTitle(msg) {
  sendResponse({value: curWindow.top.document.title});
}




function getPageSource(msg) {
  var XMLSerializer = curWindow.XMLSerializer;
  var pageSource = new XMLSerializer().serializeToString(curWindow.document);
  sendResponse({value: pageSource });
}




function goBack(msg) {
  curWindow.history.back();
  sendOk();
}




function goForward(msg) {
  curWindow.history.forward();
  sendOk();
}




function refresh(msg) {
  curWindow.location.reload(true);
  let listen = function() { removeEventListener("DOMContentLoaded", arguments.callee, false); sendOk() } ;
  addEventListener("DOMContentLoaded", listen, false);
}




function findElementContent(msg) {
  try {
    let on_success = function(id) { sendResponse({value:id}); };
    let on_error = sendError;
    elementManager.find(curWindow, msg.json, on_success, on_error, false);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function findElementsContent(msg) {
  try {
    let on_success = function(id) { sendResponse({value:id}); };
    let on_error = sendError;
    elementManager.find(curWindow, msg.json, on_success, on_error, true);
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function clickElement(msg) {
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.click(el);
    sendOk();
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function getElementAttribute(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.getElementAttribute(el, msg.json.name)});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function getElementText(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.getElementText(el)});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function getElementTagName(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: el.tagName.toLowerCase()});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function isElementDisplayed(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementDisplayed(el)});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function isElementEnabled(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementEnabled(el)});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function isElementSelected(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementSelected(el)});
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function sendKeysToElement(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.type(curWindow.document, el, msg.json.value.join(""), true);
    sendOk();
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}




function clearElement(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.clearElement(el);
    sendOk();
  }
  catch (e) {
    sendError(e.message, e.code, e.stack);
  }
}





function switchToFrame(msg) {
  let foundFrame = null;
  if ((msg.json.value == null) && (msg.json.element == null)) {
    curWindow = content;
    curWindow.focus();
    sendOk();
    return;
  }
  if (msg.json.element != undefined) {
    if (elementManager.seenItems[msg.json.element] != undefined) {
      let wantedFrame = elementManager.getKnownElement(msg.json.element, curWindow); 
      let numFrames = curWindow.frames.length;
      for (let i = 0; i < numFrames; i++) {
        if (curWindow.frames[i].frameElement == wantedFrame) {
          curWindow = curWindow.frames[i]; 
          curWindow.focus();
          sendOk();
          return;
        }
      }
    }
  }
  switch(typeof(msg.json.value)) {
    case "string" :
      let foundById = null;
      let numFrames = curWindow.frames.length;
      for (let i = 0; i < numFrames; i++) {
        
        let frame = curWindow.frames[i];
        let frameElement = frame.frameElement;
        if (frameElement.name == msg.json.value) {
          foundFrame = i;
          break;
        } else if ((foundById == null) && (frameElement.id == msg.json.value)) {
          foundById = i;
        }
      }
      if ((foundFrame == null) && (foundById != null)) {
        foundFrame = foundById;
      }
      break;
    case "number":
      if (curWindow.frames[msg.json.value] != undefined) {
        foundFrame = msg.json.value;
      }
      break;
  }
  if (foundFrame == null) {
    sendError("Unable to locate frame: " + msg.json.value, 8, null);
    return;
  }
  curWindow = curWindow.frames[foundFrame];
  curWindow.focus();
  sendOk();

  sandbox = null;
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
    sendError(e.message, e.code, e.stack);
    return;
  }
}

function importScript(msg) {
  let file;
  if (importedScripts.exists()) {
    file = FileUtils.openFileOutputStream(importedScripts, FileUtils.MODE_APPEND | FileUtils.MODE_WRONLY);
  }
  else {
    file = FileUtils.openFileOutputStream(importedScripts, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE);
  }
  file.write(msg.json.script, msg.json.script.length);
  file.close();
  sendOk();
}


registerSelf();

