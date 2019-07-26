'use strict';




const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import('resource://gre/modules/accessibility/Utils.jsm');
Cu.import('resource://gre/modules/Geometry.jsm');

var win = getMainChromeWindow(window);

var winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(
  Ci.nsIDOMWindowUtils);













function calculateTouchListCoordinates(aTouchPoints) {
  var coords = [];
  var dpi = winUtils.displayDPI;
  for (var i = 0, target = aTouchPoints[i]; i < aTouchPoints.length; ++i) {
    var bounds = getBoundsForDOMElm(target.base);
    var parentBounds = getBoundsForDOMElm('root');
    var point = new Point(target.x || 0, target.y || 0);
    point.scale(dpi);
    point.add(bounds[0], bounds[1]);
    point.add(bounds[2] / 2, bounds[3] / 2);
    point.subtract(parentBounds[0], parentBounds[0]);
    coords.push({
      x: point.x,
      y: point.y
    });
  }
  return coords;
}







function sendTouchEvent(aTouchPoints, aName) {
  var touchList = sendTouchEvent.touchList;
  if (aName === 'touchend') {
    sendTouchEvent.touchList = null;
  } else {
    var coords = calculateTouchListCoordinates(aTouchPoints);
    var touches = [];
    for (var i = 0; i < coords.length; ++i) {
      var {x, y} = coords[i];
      var node = document.elementFromPoint(x, y);
      var touch = document.createTouch(window, node, aName === 'touchstart' ?
        1 : touchList.item(i).identifier, x, y, x, y);
      touches.push(touch);
    }
    touchList = document.createTouchList(touches);
    sendTouchEvent.touchList = touchList;
  }
  var evt = document.createEvent('TouchEvent');
  evt.initTouchEvent(aName, true, true, window, 0, false, false, false, false,
    touchList, touchList, touchList);
  document.dispatchEvent(evt);
}

sendTouchEvent.touchList = null;





var eventMap = {
  touchstart: sendTouchEvent,
  touchend: sendTouchEvent,
  touchmove: sendTouchEvent
};







function testMozAccessFuGesture(aExpectedGestures) {
  var types = typeof aExpectedGestures === "string" ?
    [aExpectedGestures] : aExpectedGestures;
  function handleGesture(aEvent) {
    if (aEvent.detail.type !== types[0]) {
      
      return;
    }
    ok(true, 'Received correct mozAccessFuGesture: ' + types.shift() + '.');
    if (types.length === 0) {
      win.removeEventListener('mozAccessFuGesture', handleGesture);
      if (AccessFuTest.sequenceCleanup) {
        AccessFuTest.sequenceCleanup();
      }
      AccessFuTest.nextTest();
    }
  }
  win.addEventListener('mozAccessFuGesture', handleGesture);
}







AccessFuTest.addSequence = function AccessFuTest_addSequence(aSequence) {
  AccessFuTest.addFunc(function testSequence() {
    testMozAccessFuGesture(aSequence.expectedGestures);
    var events = aSequence.events;
    function fireEvent(aEvent) {
      eventMap[aEvent.type](aEvent.target, aEvent.type);
      processEvents();
    }
    function processEvents() {
      if (events.length === 0) {
        return;
      }
      var event = events.shift();
      if (event.delay) {
        window.setTimeout(fireEvent, event.delay, event);
      } else {
        fireEvent(event);
      }
    }
    processEvents();
  });
};






function loadJSON(aPath, aCallback) {
  var request = new XMLHttpRequest();
  request.open('GET', aPath, true);
  request.responseType = 'json';
  request.onload = function onload() {
    aCallback(request.response);
  };
  request.send();
}
