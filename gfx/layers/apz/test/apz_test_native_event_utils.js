

function getPlatform() {
  if (navigator.platform.indexOf("Win") == 0) {
    return "windows";
  }
  if (navigator.platform.indexOf("Mac") == 0) {
    return "mac";
  }
  if (navigator.platform.indexOf("Linux") == 0) {
    return "linux";
  }
  return "unknown";
}

function nativeVerticalWheelEventMsg() {
  switch (getPlatform()) {
    case "windows": return 0x020A; 
    case "mac": return 0; 
    case "linux": return 4; 
  }
  throw "Native wheel events not supported on platform " + getPlatform();
}

function nativeHorizontalWheelEventMsg() {
  switch (getPlatform()) {
    case "windows": return 0x020E; 
    case "mac": return 0; 
    case "linux": return 4; 
  }
  throw "Native wheel events not supported on platform " + getPlatform();
}


function nativeScrollUnits(aElement, aDimen) {
  switch (getPlatform()) {
    case "linux": {
      
      var targetWindow = aElement.ownerDocument.defaultView;
      var lineHeight = targetWindow.getComputedStyle(aElement)["font-size"];
      return aDimen / (parseInt(lineHeight) * 3);
    }
  }
  return aDimen;
}

function nativeMouseMoveEventMsg() {
  switch (getPlatform()) {
    case "windows": return 1; 
    case "mac": return 5; 
    case "linux": return 3; 
  }
  throw "Native wheel events not supported on platform " + getPlatform();
}



function coordinatesRelativeToWindow(aX, aY, aElement) {
  var targetWindow = aElement.ownerDocument.defaultView;
  var scale = targetWindow.devicePixelRatio;
  var rect = aElement.getBoundingClientRect();
  return {
    x: targetWindow.mozInnerScreenX + ((rect.left + aX) * scale),
    y: targetWindow.mozInnerScreenY + ((rect.top + aY) * scale)
  };
}







function synthesizeNativeWheel(aElement, aX, aY, aDeltaX, aDeltaY, aObserver) {
  var pt = coordinatesRelativeToWindow(aX, aY, aElement);
  if (aDeltaX && aDeltaY) {
    throw "Simultaneous wheeling of horizontal and vertical is not supported on all platforms.";
  }
  aDeltaX = nativeScrollUnits(aElement, aDeltaX);
  aDeltaY = nativeScrollUnits(aElement, aDeltaY);
  var msg = aDeltaX ? nativeHorizontalWheelEventMsg() : nativeVerticalWheelEventMsg();
  var utils = SpecialPowers.getDOMWindowUtils(aElement.ownerDocument.defaultView);
  utils.sendNativeMouseScrollEvent(pt.x, pt.y, msg, aDeltaX, aDeltaY, 0, 0, 0, aElement, aObserver);
  return true;
}





function synthesizeNativeWheelAndWaitForObserver(aElement, aX, aY, aDeltaX, aDeltaY, aCallback) {
  var observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aCallback && aTopic == "mousescrollevent") {
        setTimeout(aCallback, 0);
      }
    }
  };
  return synthesizeNativeWheel(aElement, aX, aY, aDeltaX, aDeltaY, observer);
}





function synthesizeNativeWheelAndWaitForWheelEvent(aElement, aX, aY, aDeltaX, aDeltaY, aCallback) {
  var targetWindow = aElement.ownerDocument.defaultView;
  targetWindow.addEventListener("wheel", function wheelWaiter(e) {
    targetWindow.removeEventListener("wheel", wheelWaiter);
    setTimeout(aCallback, 0);
  });
  return synthesizeNativeWheel(aElement, aX, aY, aDeltaX, aDeltaY);
}






function synthesizeNativeWheelAndWaitForScrollEvent(aElement, aX, aY, aDeltaX, aDeltaY, aCallback) {
  var targetWindow = aElement.ownerDocument.defaultView;
  var useCapture = true;  
  targetWindow.addEventListener("scroll", function scrollWaiter(e) {
    targetWindow.removeEventListener("scroll", scrollWaiter, useCapture);
    setTimeout(aCallback, 0);
  }, useCapture);
  return synthesizeNativeWheel(aElement, aX, aY, aDeltaX, aDeltaY);
}



function synthesizeNativeMouseMove(aElement, aX, aY) {
  var pt = coordinatesRelativeToWindow(aX, aY, aElement);
  var utils = SpecialPowers.getDOMWindowUtils(aElement.ownerDocument.defaultView);
  utils.sendNativeMouseEvent(pt.x, pt.y, nativeMouseMoveEventMsg(), 0, aElement);
  return true;
}






function synthesizeNativeMouseMoveAndWaitForMoveEvent(aElement, aX, aY, aCallback) {
  var targetWindow = aElement.ownerDocument.defaultView;
  targetWindow.addEventListener("mousemove", function mousemoveWaiter(e) {
    targetWindow.removeEventListener("mousemove", mousemoveWaiter);
    setTimeout(aCallback, 0);
  });
  return synthesizeNativeMouseMove(aElement, aX, aY);
}



function synthesizeNativeTouch(aElement, aX, aY, aType, aObserver = null, aTouchId = 0) {
  var pt = coordinatesRelativeToWindow(aX, aY, aElement);
  var utils = SpecialPowers.getDOMWindowUtils(aElement.ownerDocument.defaultView);
  utils.sendNativeTouchPoint(aTouchId, aType, pt.x, pt.y, 1, 90, aObserver);
  return true;
}

function synthesizeNativeDrag(aElement, aX, aY, aDeltaX, aDeltaY, aObserver = null, aTouchId = 0) {
  synthesizeNativeTouch(aElement, aX, aY, SpecialPowers.DOMWindowUtils.TOUCH_CONTACT, null, aTouchId);
  var steps = Math.max(Math.abs(aDeltaX), Math.abs(aDeltaY));
  for (var i = 1; i < steps; i++) {
    var dx = i * (aDeltaX / steps);
    var dy = i * (aDeltaY / steps);
    synthesizeNativeTouch(aElement, aX + dx, aY + dy, SpecialPowers.DOMWindowUtils.TOUCH_CONTACT, null, aTouchId);
  }
  synthesizeNativeTouch(aElement, aX + aDeltaX, aY + aDeltaY, SpecialPowers.DOMWindowUtils.TOUCH_CONTACT, null, aTouchId);
  return synthesizeNativeTouch(aElement, aX + aDeltaX, aY + aDeltaY, SpecialPowers.DOMWindowUtils.TOUCH_REMOVE, aObserver, aTouchId);
}
