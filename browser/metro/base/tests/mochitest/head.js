







XPCOMUtils.defineLazyModuleGetter(this, "Promise", "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");




const serverRoot = "http://example.com/browser/metro/";
const baseURI = "http://mochi.test:8888/browser/metro/";
const chromeRoot = getRootDirectory(gTestPath);
const kDefaultWait = 2000;
const kDefaultInterval = 50;





function isLandscapeMode()
{
  return (MetroUtils.snappedState == Ci.nsIWinMetroUtils.fullScreenLandscape);
}

function checkContextUIMenuItemCount(aCount)
{
  let visibleCount = 0;
  for (let idx = 0; idx < ContextMenuUI._commands.childNodes.length; idx++) {
    if (!ContextMenuUI._commands.childNodes[idx].hidden)
      visibleCount++;
  }
  is(visibleCount, aCount, "command list count");
}

function checkContextUIMenuItemVisibility(aVisibleList)
{
  let errors = 0;
  for (let idx = 0; idx < ContextMenuUI._commands.childNodes.length; idx++) {
    let item = ContextMenuUI._commands.childNodes[idx];
    if (aVisibleList.indexOf(item.id) != -1 && item.hidden) {
      
      errors++;
      info("should be visible:" + item.id);
    } else if (aVisibleList.indexOf(item.id) == -1 && !item.hidden) {
      
      errors++;
      info("should be hidden:" + item.id);
    }
  }
  is(errors, 0, "context menu item list visibility");
}

function checkMonoclePositionRange(aMonocle, aMinX, aMaxX, aMinY, aMaxY)
{
  let monocle = null;
  if (aMonocle == "start")
    monocle = SelectionHelperUI._startMark;
  else if (aMonocle == "end")
    monocle = SelectionHelperUI._endMark;
  else if (aMonocle == "caret")
    monocle = SelectionHelperUI._caretMark;
  else
    ok(false, "bad monocle id");

  ok(monocle.xPos > aMinX && monocle.xPos < aMaxX,
    "X position is " + monocle.xPos + ", expected between " + aMinX + " and " + aMaxX);
  ok(monocle.yPos > aMinY && monocle.yPos < aMaxY,
    "Y position is " + monocle.yPos + ", expected between " + aMinY + " and " + aMaxY);
}







function showNotification()
{
  return Task.spawn(function() {
    try {
      let strings = Strings.browser;
      var buttons = [
        {
          isDefault: false,
          label: strings.GetStringFromName("popupButtonAllowOnce2"),
          accessKey: "",
          callback: function() { }
        },
        {
          label: strings.GetStringFromName("popupButtonAlwaysAllow3"),
          accessKey: "",
          callback: function() { }
        },
        {
          label: strings.GetStringFromName("popupButtonNeverWarn3"),
          accessKey: "",
          callback: function() { }
        }
      ];
      let notificationBox = Browser.getNotificationBox();
      const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
      notificationBox.appendNotification("test notification", "popup-blocked",
                                          "chrome://browser/skin/images/infobar-popup.png",
                                          priority, buttons);
      yield waitForEvent(notificationBox, "transitionend");
      return;
    } catch (ex) {
      throw new Task.Result(ex);
    }
  });
}

function getSelection(aElement) {
  if (!aElement)
    return null;
  
  if (aElement instanceof Ci.nsIDOMNSEditableElement) {
    return aElement.QueryInterface(Ci.nsIDOMNSEditableElement)
                 .editor.selection;
  }
  
  if (aElement instanceof HTMLDocument || aElement instanceof Window) {
    return aElement.getSelection();
  }
  
  return aElement.contentWindow.getSelection();
};

function getTrimmedSelection(aElement) {
  let sel = getSelection(aElement);
  if (!sel)
    return "";
  return sel.toString().trim();
}






function clearSelection(aTarget) {
  SelectionHelperUI.closeEditSession(true);
  getSelection(aTarget).removeAllRanges();
  purgeEventQueue();
}





function hideContextUI()
{
  purgeEventQueue();
  if (ContextUI.isVisible) {
    info("is visible, waiting...");
    let promise = waitForEvent(Elements.tray, "transitionend", null, Elements.tray);
    if (ContextUI.dismiss())
    {
      return promise;
    }
    return true;
  }
}

function showNavBar()
{
  let promise = waitForEvent(Elements.tray, "transitionend");
  if (!ContextUI.isVisible) {
    ContextUI.displayNavbar();
    return promise;
  }
}

function fireAppBarDisplayEvent()
{
  let promise = waitForEvent(Elements.tray, "transitionend");
  let event = document.createEvent("Events");
  event.initEvent("MozEdgeUIGesture", true, false);
  gWindow.dispatchEvent(event);
  purgeEventQueue();
  return promise;
}
















function addTab(aUrl) {
  return Task.spawn(function() {
    info("Opening "+aUrl+" in a new tab");
    let tab = Browser.addTab(aUrl, true);
    yield tab.pageShowPromise;

    is(tab.browser.currentURI.spec, aUrl, aUrl + " is loaded");
    registerCleanupFunction(function() Browser.closeTab(tab));
    throw new Task.Result(tab);
  });
}





















function waitForEvent(aSubject, aEventName, aTimeoutMs, aTarget) {
  let eventDeferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let timerID = setTimeout(function wfe_canceller() {
    aSubject.removeEventListener(aEventName, onEvent);
    eventDeferred.reject( new Error(aEventName+" event timeout") );
  }, timeoutMs);

  function onEvent(aEvent) {
    if (aTarget && aTarget !== aEvent.target)
        return;

    
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
      deferred.reject( new Error("Got exception while attempting to test condition: " + e) );
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








function waitForImageLoad(aWindow, aImageId) {
  let elem = aWindow.document.getElementById(aImageId);
  return waitForCondition(function () {
    let request = elem.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
    if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
      return true;
    return false;
  }, 5000, 100);
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








function sendContextMenuClick(aX, aY) {
  let mediator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                            .getService(Components.interfaces.nsIWindowMediator);
  let mainwin = mediator.getMostRecentWindow("navigator:browser");
  let utils = mainwin.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIDOMWindowUtils);
  utils.sendMouseEvent("contextmenu", aX, aY, 2, 1, 0, true,
                        1, Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
}








function sendContextMenuClickToWindow(aWindow, aX, aY) {
  let utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIDOMWindowUtils);

  utils.sendMouseEventToWindow("contextmenu", aX, aY, 2, 1, 0, true,
                                1, Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
}

function sendContextMenuClickToElement(aWindow, aElement, aX, aY) {
  let utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIDOMWindowUtils);
  let rect = aElement.getBoundingClientRect();
  utils.sendMouseEventToWindow("contextmenu", rect.left + aX, rect.top + aY, 2, 1, 0, true,
                                1, Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
}




function sendDoubleTap(aWindow, aX, aY) {
  EventUtils.synthesizeMouseAtPoint(aX, aY, {
      clickCount: 1,
      inputSource: Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH
    }, aWindow);

  EventUtils.synthesizeMouseAtPoint(aX, aY, {
      clickCount: 2,
      inputSource: Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH
    }, aWindow);
}

function sendTap(aWindow, aX, aY) {
  EventUtils.synthesizeMouseAtPoint(aX, aY, {
      clickCount: 1,
      inputSource: Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH
    }, aWindow);
}





function sendTouchDrag(aWindow, aStartX, aStartY, aEndX, aEndY) {
  EventUtils.synthesizeTouchAtPoint(aStartX, aStartY, { type: "touchstart" }, aWindow);
  EventUtils.synthesizeTouchAtPoint(aEndX, aEndY, { type: "touchmove" }, aWindow);
  EventUtils.synthesizeTouchAtPoint(aEndX, aEndY, { type: "touchend" }, aWindow);
}




function TouchDragAndHold() {
}

TouchDragAndHold.prototype = {
  _timeoutStep: 2,
  _numSteps: 50,
  _debug: false,
  _win: null,

  callback: function callback() {
    if (this._win == null)
      return;
    if (++this._step.steps >= this._numSteps) {
      EventUtils.synthesizeTouchAtPoint(this._endPoint.xPos, this._endPoint.yPos,
                                        { type: "touchmove" }, this._win);
      this._defer.resolve();
      return;
    }
    this._currentPoint.xPos += this._step.x;
    this._currentPoint.yPos += this._step.y;
    if (this._debug) {
      info("[" + this._step.steps + "] touchmove " + this._currentPoint.xPos + " x " + this._currentPoint.yPos);
    }
    EventUtils.synthesizeTouchAtPoint(this._currentPoint.xPos, this._currentPoint.yPos,
                                      { type: "touchmove" }, this._win);
    let self = this;
    setTimeout(function () { self.callback(); }, this._timeoutStep);
  },

  start: function start(aWindow, aStartX, aStartY, aEndX, aEndY) {
    this._defer = Promise.defer();
    this._win = aWindow;
    this._endPoint = { xPos: aEndX, yPos: aEndY };
    this._currentPoint = { xPos: aStartX, yPos: aStartY };
    this._step = { steps: 0, x: (aEndX - aStartX) / this._numSteps, y: (aEndY - aStartY) / this._numSteps };
    if (this._debug) {
      info("[0] touchstart " + aStartX + " x " + aStartY);
    }
    EventUtils.synthesizeTouchAtPoint(aStartX, aStartY, { type: "touchstart" }, aWindow);
    let self = this;
    setTimeout(function () { self.callback(); }, this._timeoutStep);
    return this._defer.promise;
  },

  end: function start() {
    if (this._debug) {
      info("[" + this._step.steps + "] touchend " + this._endPoint.xPos + " x " + this._endPoint.yPos);
    }
    EventUtils.synthesizeTouchAtPoint(this._endPoint.xPos, this._endPoint.yPos,
                                      { type: "touchend" }, this._win);
    this._win = null;
  },
};





 


function emptyClipboard() {
  Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard)
                                       .emptyClipboard(Ci.nsIClipboard.kGlobalClipboard);
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
      try {
        if ('function' == typeof gCurrentTest.setUp) {
          info("SETUP " + gCurrentTest.desc);
          yield Task.spawn(gCurrentTest.setUp.bind(gCurrentTest));
        }
        try {
          info("RUN " + gCurrentTest.desc);
          yield Task.spawn(gCurrentTest.run.bind(gCurrentTest));
        } finally {
          if ('function' == typeof gCurrentTest.tearDown) {
            info("TEARDOWN " + gCurrentTest.desc);
            yield Task.spawn(gCurrentTest.tearDown.bind(gCurrentTest));
          }
        }
      } catch (ex) {
        ok(false, "runTests: Task failed - " + ex + ' at ' + ex.stack);
      } finally {
        info("END " + gCurrentTest.desc);
      }
    }
    finish();
  });
}

function stubMethod(aObj, aMethod) {
  let origFunc = aObj[aMethod];
  let func = function() {
    func.calledWith = Array.slice(arguments);
    func.callCount++;
  }
  func.callCount = 0;
  func.restore = function() {
    return (aObj[aMethod] = origFunc);
  };
  aObj[aMethod] = func;
  return func;
}