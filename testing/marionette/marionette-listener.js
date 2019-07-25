




let Cu = Components.utils;
let uuidGen = Components.classes["@mozilla.org/uuid-generator;1"]
             .getService(Components.interfaces.nsIUUIDGenerator);

let loader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
             .getService(Components.interfaces.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://marionette/content/marionette-simpletest.js");
loader.loadSubScript("chrome://marionette/content/marionette-log-obj.js");
Components.utils.import("chrome://marionette/content/marionette-elements.js");
let marionetteLogObj = new MarionetteLogObj();

let isB2G = false;

let marionetteTimeout = null;
let winUtil = content.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowUtils);
let listenerId = null; 
let activeFrame = null;
let win = content;
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

  marionette.exports.forEach(function(fn) {
    sandbox[fn] = marionette[fn].bind(marionette);
  });

  return sandbox;
}





function executeScript(msg, directInject) {
  let script = msg.json.value;
  let marionette = new Marionette(false, win, "content", marionetteLogObj);

  let sandbox = createExecuteContentSandbox(win, marionette, msg.json.args);
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
  win.addEventListener("unload", errUnload, false);
  let script = msg.json.value;
  let command_id = msg.json.id;

  
  
  
  
  
  let timeoutId = win.setTimeout(function() {
    contentAsyncReturnFunc('timed out', 28);
  }, marionetteTimeout);
  win.addEventListener('error', function win__onerror(evt) {
    win.removeEventListener('error', win__onerror, true);
    contentAsyncReturnFunc(evt, 17);
    return true;
  }, true);

  function contentAsyncReturnFunc(value, status) {
    win.removeEventListener("unload", errUnload, false);

    
    for(let i=0; i<=timeoutId; i++) {
      win.clearTimeout(i);
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

  let marionette = new Marionette(true, win, "content", marionetteLogObj);

  let sandbox = createExecuteContentSandbox(win, marionette, msg.json.args);
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
  if (activeFrame != null) {
    win.document.location = msg.json.value;
    
    let checkTimer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    let checkLoad = function () { 
                      if (win.document.readyState == "complete") { 
                        sendOk();
                      } 
                      else { 
                        checkTimer.initWithCallback(checkLoad, 100, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
                      }
                    };
    checkLoad();
  }
  else {
    sendAsyncMessage("Marionette:goUrl", {value: msg.json.value});
  }
}




function getUrl(msg) {
  sendResponse({value: win.location.href});
}




function goBack(msg) {
  win.history.back();
  sendOk();
}




function goForward(msg) {
  win.history.forward();
  sendOk();
}




function refresh(msg) {
  win.location.reload(true);
  let listen = function() { removeEventListener("DOMContentLoaded", arguments.callee, false); sendOk() } ;
  addEventListener("DOMContentLoaded", listen, false);
}




function findElementContent(msg) {
  
  let id;
  try {
    let notify = function(id) { sendResponse({value:id});};
    let curWin = activeFrame ? win.frames[activeFrame] : win;
    id = elementManager.find(curWin, msg.json, notify, false);
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
    return;
  }
}




function findElementsContent(msg) {
  
  let id;
  try {
    let notify = function(id) { sendResponse({value:id});};
    let curWin = activeFrame ? win.frames[activeFrame] : win;
    id = elementManager.find(curWin, msg.json, notify, true);
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
    return;
  }
}




function clickElement(msg) {
  let el;
  try {
    el = elementManager.getKnownElement(msg.json.element, win);
  }
  catch (e) {
    sendError(e.message, e.num, e.stack);
    return;
  }
  el.click();
  sendOk();
}





function switchToFrame(msg) {
  let foundFrame = null;
  if ((msg.json.value == null) && (msg.json.element == null)) {
    win = content;
    activeFrame = null;
    content.focus();
    sendOk();
    return;
  }
  if (msg.json.element != undefined) {
    if (elementManager.seenItems[msg.json.element] != undefined) {
      let wantedFrame = elementManager.getKnownElement(msg.json.element, win);
      let numFrames = win.frames.length;
      for (let i = 0; i < numFrames; i++) {
        if (win.frames[i].frameElement == wantedFrame) {
          win = win.frames[i]; 
          activeFrame = i;
          win.focus();
          sendOk();
          return;
        }
      }
    }
  }
  switch(typeof(msg.json.value)) {
    case "string" :
      let foundById = null;
      let numFrames = win.frames.length;
      for (let i = 0; i < numFrames; i++) {
        
        let frame = win.frames[i];
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
      if (win.frames[msg.json.value] != undefined) {
        foundFrame = msg.json.value;
      }
      break;
  }
  
  if (foundFrame != null) {
    let frameWindow = win.frames[foundFrame];
    activeFrame = foundFrame;
    win = frameWindow;
    win.focus();
    sendOk();
  } else {
    sendError("Unable to locate frame: " + msg.json.value, 8, null);
  }
}


registerSelf();
