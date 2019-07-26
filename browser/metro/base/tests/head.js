







XPCOMUtils.defineLazyModuleGetter(this, "Promise", "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");




const serverRoot = "http://example.com/browser/metro/";
const baseURI = "http://mochi.test:8888/browser/metro/";
const chromeRoot = getRootDirectory(gTestPath);
const kDefaultWait = 10000;
const kDefaultInterval = 50;





function checkContextUIMenuItemCount(aCount)
{
  let visibleCount = 0;
  for (let idx = 0; idx < ContextMenuUI._commands.childNodes.length; idx++) {
    if (!ContextMenuUI._commands.childNodes[idx].hidden)
      visibleCount++;
  }
  is(visibleCount, aCount, "command list count");
}





function hideContextUI()
{
  if (ContextUI.isVisible) {
    let promise = waitForEvent(Elements.tray, "transitionend");
    ContextUI.dismiss();
    return promise;
  }
}
















function addTab(aUrl) {
  return Task.spawn(function() {
    info("Opening "+aUrl+" in a new tab");
    let tab = Browser.addTab(aUrl, true);
    yield waitForEvent(tab.browser, "pageshow");

    is(tab.browser.currentURI.spec, aUrl, aUrl + " is loaded");
    registerCleanupFunction(function() Browser.closeTab(tab));
    throw new Task.Result(tab);
  });
}





















function waitForEvent(aSubject, aEventName, aTimeoutMs) {
  let eventDeferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let timerID = setTimeout(function wfe_canceller() {
    aSubject.removeEventListener(aEventName, onEvent);
    eventDeferred.reject( new Error(aEventName+" event timeout") );
  }, timeoutMs);

  function onEvent(aEvent) {
    
    clearTimeout(timerID);
    eventDeferred.resolve(aEvent);
  }

  function cleanup() {
    
    aSubject.removeEventListener(aEventName, onEvent);
  }
  eventDeferred.promise.then(cleanup, cleanup);

  aSubject.addEventListener(aEventName, onEvent, false);
  return eventDeferred.promise;
}











function waitForMs(aMs) {
  info("Wating for " + aMs + "ms");
  let deferred = Promise.defer();
  let startTime = Date.now();
  setTimeout(done, aMs);

  function done() {
    deferred.resolve(true);
    info("waitForMs finished waiting, waited for "
       + (Date.now() - startTime)
       + "ms");
  }

  return deferred.promise;
}
















function waitForCondition(aCondition, aTimeoutMs, aIntervalMs) {
  let deferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let intervalMs = aIntervalMs || kDefaultInterval;
  let startTime = Date.now();

  function testCondition() {
    let now = Date.now();
    if((now - startTime) > timeoutMs) {
      deferred.reject( new Error("Timed out waiting for condition to be true") );
      return;
    }

    let condition;
    try {
      condition = aCondition();
    } catch (e) {
      deferred.reject( new Error("Got exception while attempting to test conditino: " + e) );
      return;
    }

    if (condition) {
      deferred.resolve(true);
    } else {
      setTimeout(testCondition, intervalMs);
    }
  }

  setTimeout(testCondition, 0);
  return deferred.promise;
}








function waitForObserver(aObsEvent, aTimeoutMs) {
  try {
  
  let deferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let timerID = 0;

  var observeWatcher = {
    onEvent: function () {
      clearTimeout(timerID);
      Services.obs.removeObserver(this, aObsEvent);
      deferred.resolve();
    },

    onError: function () {
      clearTimeout(timerID);
      Services.obs.removeObserver(this, aObsEvent);
      deferred.reject(new Error(aObsEvent + " event timeout"));
    },

    observe: function (aSubject, aTopic, aData) {
      if (aTopic == aObsEvent) {
        this.onEvent();
      }
    },

    QueryInterface: function (aIID) {
      if (!aIID.equals(Ci.nsIObserver) &&
          !aIID.equals(Ci.nsISupportsWeakReference) &&
          !aIID.equals(Ci.nsISupports)) {
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
    },
  }

  timerID = setTimeout(function wfo_canceller() {
    observeWatcher.onError();
  }, timeoutMs);

  Services.obs.addObserver(observeWatcher, aObsEvent, true);
  return deferred.promise;
  
  } catch (ex) {
    info(ex.message);
  }
}





const usEnglish = 0x409;
const arSpanish = 0x2C0A;


const leftShift = 0x100;
const rightShift = 0x200;
const leftControl = 0x400;
const rightControl = 0x800;
const leftAlt = 0x1000;
const rightAlt = 0x2000;

function synthesizeNativeKey(aKbLayout, aVKey, aModifiers) {
  Browser.windowUtils.sendNativeKeyEvent(aKbLayout, aVKey, aModifiers, '', '');
}

function synthesizeNativeMouse(aElement, aOffsetX, aOffsetY, aMsg) {
  let x = aOffsetX;
  let y = aOffsetY;
  if (aElement) {
    if (aElement.getBoundingClientRect) {
      let rect = aElement.getBoundingClientRect();
      x += rect.left;
      y += rect.top;
    } else if(aElement.left && aElement.top) {
      x += aElement.left;
      y += aElement.top;
    }
  }
  Browser.windowUtils.sendNativeMouseEvent(x, y, aMsg, 0, null);
}

function synthesizeNativeMouseMove(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0001);  
}

function synthesizeNativeMouseLDown(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0002);  
}

function synthesizeNativeMouseLUp(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0004);  
}

function synthesizeNativeMouseRDown(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0008);  
}

function synthesizeNativeMouseRUp(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0010);  
}

function synthesizeNativeMouseMDown(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0020);  
}

function synthesizeNativeMouseMUp(aElement, aOffsetX, aOffsetY) {
  synthesizeNativeMouse(aElement,
                        aOffsetX,
                        aOffsetY,
                        0x0040);  
}




function sendContextMenuClick(aWindow, aX, aY) {
  let utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIDOMWindowUtils);

  utils.sendMouseEventToWindow("contextmenu", aX, aY, 2, 1, 0, true,
                                1, Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
}









function purgeEventQueue() {
  let thread = Services.tm.currentThread;
  while (thread.hasPendingEvents()) {
    if (!thread.processNextEvent(true))
      break;
  }
}




let gCurrentTest = null;
let gTests = [];

function runTests() {
  waitForExplicitFinish();
  Task.spawn(function() {
    while((gCurrentTest = gTests.shift())){
      info(gCurrentTest.desc);
      yield Task.spawn(gCurrentTest.run);
      info("END "+gCurrentTest.desc);
    }
    info("done with gTests while loop, calling finish");
    finish();
  });
}
