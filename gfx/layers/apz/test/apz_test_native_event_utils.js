

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

function nativeMouseMoveEventMsg() {
  switch (getPlatform()) {
    case "windows": return 1; 
    case "mac": return 5; 
    case "linux": return 3; 
  }
  throw "Native wheel events not supported on platform " + getPlatform();
}







function synthesizeNativeWheel(aElement, aX, aY, aDeltaX, aDeltaY, aObserver) {
  var targetWindow = aElement.ownerDocument.defaultView;
  aX += targetWindow.mozInnerScreenX;
  aY += targetWindow.mozInnerScreenY;
  if (aDeltaX && aDeltaY) {
    throw "Simultaneous wheeling of horizontal and vertical is not supported on all platforms.";
  }
  var msg = aDeltaX ? nativeHorizontalWheelEventMsg() : nativeVerticalWheelEventMsg();
  _getDOMWindowUtils().sendNativeMouseScrollEvent(aX, aY, msg, aDeltaX, aDeltaY, 0, 0, 0, aElement, aObserver);
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
  var targetWindow = aElement.ownerDocument.defaultView;
  aX += targetWindow.mozInnerScreenX;
  aY += targetWindow.mozInnerScreenY;
  _getDOMWindowUtils().sendNativeMouseEvent(aX, aY, nativeMouseMoveEventMsg(), 0, aElement);
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
