




let Cu = Components.utils;
let uuidGen = Components.classes["@mozilla.org/uuid-generator;1"]
             .getService(Components.interfaces.nsIUUIDGenerator);

let loader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
             .getService(Components.interfaces.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-log-obj.js");
Components.utils.import("chrome://marionette/content/marionette-elements.js");
let utils = {};
utils.window = content;

loader.loadSubScript("chrome://marionette/content/EventUtils.js", utils)
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", utils);
loader.loadSubScript("chrome://marionette/content/atoms.js", utils);
let marionetteLogObj = new MarionetteLogObj();

let isB2G = false;

let marionetteTimeout = null;
let winUtil = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
let listenerId = null; 
let activeFrame = null;
let curWindow = content;
let elementManager = new ElementManager([]);






function registerSelf() {
  let register = sendSyncMessage("Marionette:register", {value: winUtil.outerWindowID, href: content.location.href});
  
  if (register[0]) {
    listenerId = register[0];
    startListeners();
  }
}




function startListeners() {
  addMessageListener("Marionette:newSession" + listenerId, newSession);
  addMessageListener("Marionette:executeScript" + listenerId, executeScript);
  addMessageListener("Marionette:setScriptTimeout" + listenerId, setScriptTimeout);
  addMessageListener("Marionette:executeAsyncScript" + listenerId, executeAsyncScript);
  addMessageListener("Marionette:executeJSScript" + listenerId, executeJSScript);
  addMessageListener("Marionette:setSearchTimeout" + listenerId, setSearchTimeout);
  addMessageListener("Marionette:goUrl" + listenerId, goUrl);
  addMessageListener("Marionette:getUrl" + listenerId, getUrl);
  addMessageListener("Marionette:goBack" + listenerId, goBack);
  addMessageListener("Marionette:goForward" + listenerId, goForward);
  addMessageListener("Marionette:refresh" + listenerId, refresh);
  addMessageListener("Marionette:findElementContent" + listenerId, findElementContent);
  addMessageListener("Marionette:findElementsContent" + listenerId, findElementsContent);
  addMessageListener("Marionette:clickElement" + listenerId, clickElement);
  addMessageListener("Marionette:getElementAttribute" + listenerId, getElementAttribute);
  addMessageListener("Marionette:getElementText" + listenerId, getElementText);
  addMessageListener("Marionette:isElementDisplayed" + listenerId, isElementDisplayed);
  addMessageListener("Marionette:isElementEnabled" + listenerId, isElementEnabled);
  addMessageListener("Marionette:isElementSelected" + listenerId, isElementSelected);
  addMessageListener("Marionette:sendKeysToElement" + listenerId, sendKeysToElement);
  addMessageListener("Marionette:clearElement" + listenerId, clearElement);
  addMessageListener("Marionette:switchToFrame" + listenerId, switchToFrame);
  addMessageListener("Marionette:deleteSession" + listenerId, deleteSession);
  addMessageListener("Marionette:sleepSession" + listenerId, sleepSession);
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
  removeMessageListener("Marionette:newSession" + listenerId, newSession);
  removeMessageListener("Marionette:executeScript" + listenerId, executeScript);
  removeMessageListener("Marionette:setScriptTimeout" + listenerId, setScriptTimeout);
  removeMessageListener("Marionette:executeAsyncScript" + listenerId, executeAsyncScript);
  removeMessageListener("Marionette:executeJSScript" + listenerId, executeJSScript);
  removeMessageListener("Marionette:setSearchTimeout" + listenerId, setSearchTimeout);
  removeMessageListener("Marionette:goUrl" + listenerId, goUrl);
  removeMessageListener("Marionette:getUrl" + listenerId, getUrl);
  removeMessageListener("Marionette:goBack" + listenerId, goBack);
  removeMessageListener("Marionette:goForward" + listenerId, goForward);
  removeMessageListener("Marionette:refresh" + listenerId, refresh);
  removeMessageListener("Marionette:findElementContent" + listenerId, findElementContent);
  removeMessageListener("Marionette:findElementsContent" + listenerId, findElementsContent);
  removeMessageListener("Marionette:clickElement" + listenerId, clickElement);
  removeMessageListener("Marionette:getElementAttribute" + listenerId, getElementAttribute);
  removeMessageListener("Marionette:getElementText" + listenerId, getElementText);
  removeMessageListener("Marionette:isElementDisplayed" + listenerId, isElementDisplayed);
  removeMessageListener("Marionette:isElementEnabled" + listenerId, isElementEnabled);
  removeMessageListener("Marionette:isElementSelected" + listenerId, isElementSelected);
  removeMessageListener("Marionette:sendKeysToElement" + listenerId, sendKeysToElement);
  removeMessageListener("Marionette:clearElement" + listenerId, clearElement);
  removeMessageListener("Marionette:switchToFrame" + listenerId, switchToFrame);
  removeMessageListener("Marionette:deleteSession" + listenerId, deleteSession);
  removeMessageListener("Marionette:sleepSession" + listenerId, sleepSession);
  this.elementManager.reset();
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
  marionetteTimeout = null;
  curWin = content;
}




function errUnload() {
  sendError("unload was called", 17, null);
}









function createExecuteContentSandbox(aWindow, marionette, args) {
  try {
    args = elementManager.convertWrappedArguments(args, aWindow);
  }
  catch(e) {
    sendError(e.message, e.num, e.stack);
    return;
  }

  let sandbox = new Cu.Sandbox(aWindow);
  sandbox.window = aWindow;
  sandbox.document = sandbox.window.document;
  sandbox.navigator = sandbox.window.navigator;
  sandbox.__namedArgs = elementManager.applyNamedArgs(args);
  sandbox.__marionetteParams = args;
  sandbox.__proto__ = sandbox.window;
  sandbox.testUtils = utils;

  marionette.exports.forEach(function(fn) {
    sandbox[fn] = marionette[fn].bind(marionette);
  });

  return sandbox;
}





function executeScript(msg, directInject) {
  let script = msg.json.value;
  let marionette = new Marionette(false, curWindow, "content", marionetteLogObj);

  let sandbox = createExecuteContentSandbox(curWindow, marionette, msg.json.args);
  if (!sandbox)
    return;

  sandbox.finish = function sandbox_finish() {
    return marionette.generate_results();
  };

  try {
    if (directInject) {
      let res = Cu.evalInSandbox(script, sandbox, "1.8");
      sendSyncMessage("Marionette:testLog", {value: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();
      if (res == undefined || res.passed == undefined) {
        sendError("Marionette.finish() not called", 17, null);
      }
      else {
        sendResponse({value: elementManager.wrapValue(res)});
      }
    }
    else {
      let scriptSrc = "let __marionetteFunc = function(){" + script + "};" +
                      "__marionetteFunc.apply(null, __marionetteParams);";
      let res = Cu.evalInSandbox(scriptSrc, sandbox, "1.8");
      sendSyncMessage("Marionette:testLog", {value: elementManager.wrapValue(marionetteLogObj.getLogs())});
      marionetteLogObj.clearLogs();
      sendResponse({value: elementManager.wrapValue(res)});
    }
  }
  catch (e) {
    
    sendError(e.name + ': ' + e.message, 17, e.stack);
  }
}




function setScriptTimeout(msg) {
  marionetteTimeout = msg.json.value;
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
  let command_id = msg.json.id;

  
  
  
  
  
  let timeoutId = curWindow.setTimeout(function() {
    contentAsyncReturnFunc('timed out', 28);
  }, marionetteTimeout);
  curWindow.addEventListener('error', function win__onerror(evt) {
    curWindow.removeEventListener('error', win__onerror, true);
    contentAsyncReturnFunc(evt, 17);
    return true;
  }, true);

  function contentAsyncReturnFunc(value, status) {
    curWindow.removeEventListener("unload", errUnload, false);

    
    for(let i=0; i<=timeoutId; i++) {
      curWindow.clearTimeout(i);
    }

    sendSyncMessage("Marionette:testLog", {value: elementManager.wrapValue(marionetteLogObj.getLogs())});
    marionetteLogObj.clearLogs();
    if (status == 0){
      sendResponse({value: elementManager.wrapValue(value), status: status}, command_id);
    }
    else {
      sendError(value, status, null, command_id);
    }
  };

  let scriptSrc;
  if (timeout) {
    if (marionetteTimeout == null || marionetteTimeout == 0) {
      sendError("Please set a timeout", 21, null);
    }
    scriptSrc = script;
  }
  else {
    scriptSrc = "let marionetteScriptFinished = function(value) { return asyncComplete(value,0);};" +
                "__marionetteParams.push(marionetteScriptFinished);" +
                "let __marionetteFunc = function() { " + script + "};" +
                "__marionetteFunc.apply(null, __marionetteParams); ";
  }

  let marionette = new Marionette(true, curWindow, "content", marionetteLogObj);

  let sandbox = createExecuteContentSandbox(curWindow, marionette, msg.json.args);
  if (!sandbox)
    return;

  sandbox.asyncComplete = contentAsyncReturnFunc;
  sandbox.finish = function sandbox_finish() {
    contentAsyncReturnFunc(marionette.generate_results(), 0);
  };

  try {
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
    sendError(e.message, e.num, e.stack);
    return;
  }
  sendOk();
}





function goUrl(msg) {
  curWindow.location = msg.json.value;
  
  let checkTimer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
  let checkLoad = function () { 
                    if (curWindow.document.readyState == "complete") { 
                      sendOk();
                    } 
                    else { 
                      checkTimer.initWithCallback(checkLoad, 100, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
                    }
                  };
  checkTimer.initWithCallback(checkLoad, 100, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
}




function getUrl(msg) {
  sendResponse({value: curWindow.location.href});
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
  let id;
  try {
    let notify = function(id) { sendResponse({value:id});};
    id = elementManager.find(curWindow, msg.json, notify, false);
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function findElementsContent(msg) {
  let id;
  try {
    let notify = function(id) { sendResponse({value:id});};
    id = elementManager.find(curWindow, msg.json, notify, true);
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
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
    sendError(e.message, e.num, e.stack);
  }
}




function getElementAttribute(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.getElementAttribute(el, msg.json.name)});
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function getElementText(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.getElementText(el)});
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function isElementDisplayed(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementDisplayed(el)});
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function isElementEnabled(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementEnabled(el)});
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function isElementSelected(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    sendResponse({value: utils.isElementSelected(el)});
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function sendKeysToElement(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.sendKeysToElement(el, msg.json.value);
    sendOk();
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
  }
}




function clearElement(msg) {
  try {
    let el = elementManager.getKnownElement(msg.json.element, curWindow);
    utils.clearElement(el);
    sendOk();
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
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
  if (foundFrame != null) {
    curWindow = curWindow.frames[foundFrame];
    curWindow.focus();
    sendOk();
  } else {
    sendError("Unable to locate frame: " + msg.json.value, 8, null);
  }
}


registerSelf();
