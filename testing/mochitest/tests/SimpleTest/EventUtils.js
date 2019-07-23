
















function sendMouseEvent(aEvent, aTarget, aWindow) {
  if (['click', 'mousedown', 'mouseup', 'mouseover', 'mouseout'].indexOf(aEvent.type) == -1) {
    throw new Error("sendMouseEvent doesn't know about event type '"+aEvent.type+"'");
  }

  if (!aWindow) {
    aWindow = window;
  }

  
  netscape.security.PrivilegeManager.enablePrivilege('UniversalBrowserWrite');

  var event = aWindow.document.createEvent('MouseEvent');

  var typeArg          = aEvent.type;
  var canBubbleArg     = true;
  var cancelableArg    = true;
  var viewArg          = aWindow;
  var detailArg        = aEvent.detail        || (aEvent.type == 'click'     ||
                                                  aEvent.type == 'mousedown' ||
                                                  aEvent.type == 'mouseup' ? 1 : 0);
  var screenXArg       = aEvent.screenX       || 0;
  var screenYArg       = aEvent.screenY       || 0;
  var clientXArg       = aEvent.clientX       || 0;
  var clientYArg       = aEvent.clientY       || 0;
  var ctrlKeyArg       = aEvent.ctrlKey       || false;
  var altKeyArg        = aEvent.altKey        || false;
  var shiftKeyArg      = aEvent.shiftKey      || false;
  var metaKeyArg       = aEvent.metaKey       || false;
  var buttonArg        = aEvent.button        || 0;
  var relatedTargetArg = aEvent.relatedTarget || null;

  event.initMouseEvent(typeArg, canBubbleArg, cancelableArg, viewArg, detailArg,
                       screenXArg, screenYArg, clientXArg, clientYArg,
                       ctrlKeyArg, altKeyArg, shiftKeyArg, metaKeyArg,
                       buttonArg, relatedTargetArg);

  aWindow.document.getElementById(aTarget).dispatchEvent(event);
}













function sendChar(aChar, aTarget) {
  
  var hasShift = (aChar == aChar.toUpperCase());
  var charCode = aChar.charCodeAt(0);
  var keyCode = charCode;
  if (!hasShift) {
    
    keyCode -= 0x20;
  }

  return __doEventDispatch(aTarget, charCode, keyCode, hasShift);
}








function sendString(aStr, aTarget) {
  for (var i = 0; i < aStr.length; ++i) {
    sendChar(aStr.charAt(i), aTarget);
  }
}










function sendKey(aKey, aTarget) {
  keyName = "DOM_VK_" + aKey.toUpperCase();

  if (!KeyEvent[keyName]) {
    throw "Unknown key: " + keyName;
  }

  return __doEventDispatch(aTarget, 0, KeyEvent[keyName], false);
}









function __doEventDispatch(aTarget, aCharCode, aKeyCode, aHasShift) {
  if (aTarget === undefined) {
    aTarget = "target";
  }

  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  var event = document.createEvent("KeyEvents");
  event.initKeyEvent("keydown", true, true, document.defaultView,
                     false, false, aHasShift, false,
                     aKeyCode, 0);
  var accepted = $(aTarget).dispatchEvent(event);

  
  
  event = document.createEvent("KeyEvents");
  if (aCharCode) {
    event.initKeyEvent("keypress", true, true, document.defaultView,
                       false, false, aHasShift, false,
                       0, aCharCode);
  } else {
    event.initKeyEvent("keypress", true, true, document.defaultView,
                       false, false, aHasShift, false,
                       aKeyCode, 0);
  }
  if (!accepted) {
    event.preventDefault();
  }
  accepted = $(aTarget).dispatchEvent(event);

  
  var event = document.createEvent("KeyEvents");
  event.initKeyEvent("keyup", true, true, document.defaultView,
                     false, false, aHasShift, false,
                     aKeyCode, 0);
  $(aTarget).dispatchEvent(event);
  return accepted;
}





function _parseModifiers(aEvent)
{
  const masks = Components.interfaces.nsIDOMNSEvent;
  var mval = 0;
  if (aEvent.shiftKey)
    mval |= masks.SHIFT_MASK;
  if (aEvent.ctrlKey)
    mval |= masks.CONTROL_MASK;
  if (aEvent.altKey)
    mval |= masks.ALT_MASK;
  if (aEvent.metaKey)
    mval |= masks.META_MASK;
  if (aEvent.accelKey)
    mval |= (navigator.platform.indexOf("Mac") >= 0) ? masks.META_MASK :
                                                       masks.CONTROL_MASK;

  return mval;
}














function synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  if (!aWindow)
    aWindow = window;

  var utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                      getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils) {
    var button = aEvent.button || 0;
    var clickCount = aEvent.clickCount || 1;
    var modifiers = _parseModifiers(aEvent);

    var rect = aTarget.getBoundingClientRect();

    var left = rect.left + aOffsetX;
    var top = rect.top + aOffsetY;

    if (aEvent.type) {
      utils.sendMouseEvent(aEvent.type, left, top, button, clickCount, modifiers);
    }
    else {
      utils.sendMouseEvent("mousedown", left, top, button, clickCount, modifiers);
      utils.sendMouseEvent("mouseup", left, top, button, clickCount, modifiers);
    }
  }
}






















function synthesizeMouseScroll(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  if (!aWindow)
    aWindow = window;

  var utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                      getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils) {
    
    const kIsVertical = 0x02;
    const kIsHorizontal = 0x04;
    const kHasPixels = 0x08;

    var button = aEvent.button || 0;
    var modifiers = _parseModifiers(aEvent);

    var rect = aTarget.getBoundingClientRect();

    var left = rect.left;
    var top = rect.top;

    var type = aEvent.type || "DOMMouseScroll";
    var axis = aEvent.axis || "vertical";
    var scrollFlags = (axis == "horizontal") ? kIsHorizontal : kIsVertical;
    if (aEvent.hasPixels) {
      scrollFlags |= kHasPixels;
    }
    utils.sendMouseScrollEvent(type, left + aOffsetX, top + aOffsetY, button,
                               scrollFlags, aEvent.delta, modifiers);
  }
}
















function synthesizeKey(aKey, aEvent, aWindow)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  if (!aWindow)
    aWindow = window;

  var utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                      getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils) {
    var keyCode = 0, charCode = 0;
    if (aKey.indexOf("VK_") == 0)
      keyCode = KeyEvent["DOM_" + aKey];
    else
      charCode = aKey.charCodeAt(0);

    var modifiers = _parseModifiers(aEvent);

    if (aEvent.type) {
      utils.sendKeyEvent(aEvent.type, keyCode, charCode, modifiers);
    }
    else {
      var keyDownDefaultHappened =
          utils.sendKeyEvent("keydown", keyCode, charCode, modifiers);
      utils.sendKeyEvent("keypress", keyCode, charCode, modifiers,
                         !keyDownDefaultHappened);
      utils.sendKeyEvent("keyup", keyCode, charCode, modifiers);
    }
  }
}

var _gSeenEvent = false;






function _expectEvent(aExpectedTarget, aExpectedEvent, aTestName)
{
  if (!aExpectedTarget || !aExpectedEvent)
    return null;

  _gSeenEvent = false;

  var type = (aExpectedEvent.charAt(0) == "!") ?
             aExpectedEvent.substring(1) : aExpectedEvent;
  var eventHandler = function(event) {
    var epassed = (!_gSeenEvent && event.originalTarget == aExpectedTarget &&
                   event.type == type);
    is(epassed, true, aTestName + " " + type + " event target " + (_gSeenEvent ? "twice" : ""));
    _gSeenEvent = true;
  };

  aExpectedTarget.addEventListener(type, eventHandler, false);
  return eventHandler;
}





function _checkExpectedEvent(aExpectedTarget, aExpectedEvent, aEventHandler, aTestName)
{
  if (aEventHandler) {
    var expectEvent = (aExpectedEvent.charAt(0) != "!");
    var type = expectEvent ? aExpectedEvent : aExpectedEvent.substring(1);
    aExpectedTarget.removeEventListener(type, aEventHandler, false);
    var desc = type + " event";
    if (!expectEvent)
      desc += " not";
    is(_gSeenEvent, expectEvent, aTestName + " " + desc + " fired");
  }

  _gSeenEvent = false;
}















function synthesizeMouseExpectEvent(aTarget, aOffsetX, aOffsetY, aEvent,
                                    aExpectedTarget, aExpectedEvent, aTestName,
                                    aWindow)
{
  var eventHandler = _expectEvent(aExpectedTarget, aExpectedEvent, aTestName);
  synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent, aWindow);
  _checkExpectedEvent(aExpectedTarget, aExpectedEvent, eventHandler, aTestName);
}














function synthesizeKeyExpectEvent(key, aEvent, aExpectedTarget, aExpectedEvent,
                                  aTestName, aWindow)
{
  var eventHandler = _expectEvent(aExpectedTarget, aExpectedEvent, aTestName);
  synthesizeKey(key, aEvent, aWindow);
  _checkExpectedEvent(aExpectedTarget, aExpectedEvent, eventHandler, aTestName);
}










function synthesizeDragStart(element, expectedDragData)
{
  var failed = null;

  var trapDrag = function(event) {
    try {
      var dataTransfer = event.dataTransfer;
      if (dataTransfer.mozItemCount != expectedDragData.length)
        throw "Failed";

      for (var t = 0; t < dataTransfer.mozItemCount; t++) {
        var types = dataTransfer.mozTypesAt(t);
        var expecteditem = expectedDragData[t];
        if (types.length != expecteditem.length)
          throw "Failed";

        for (var f = 0; f < types.length; f++) {
          if (types[f] != expecteditem[f].substring(0, types[f].length) ||
              dataTransfer.mozGetDataAt(types[f], t) != expecteditem[f].substring(types[f].length + 2))
          throw "Failed";
        }
      }
    } catch(ex) {
      failed = dataTransfer;
    }

    event.preventDefault();
    event.stopPropagation();
  }

  window.addEventListener("dragstart", trapDrag, false);
  synthesizeMouse(element, 2, 2, { type: "mousedown" });
  synthesizeMouse(element, 9, 9, { type: "mousemove" });
  synthesizeMouse(element, 10, 10, { type: "mousemove" });
  window.removeEventListener("dragstart", trapDrag, false);
  synthesizeMouse(element, 10, 10, { type: "mouseup" });

  return failed;
}











function synthesizeDrop(element, dragData, effectAllowed)
{
  var dataTransfer;
  var trapDrag = function(event) {
    dataTransfer = event.dataTransfer;
    for (var t = 0; t < dragData.length; t++) {
      var item = dragData[t];
      for (var v = 0; v < item.length; v++) {
        var idx = item[v].indexOf(":");
        dataTransfer.mozSetDataAt(item[v].substring(0, idx), item[v].substring(idx + 2), t);
      }
    }

    dataTransfer.dropEffect = "move";
    event.preventDefault();
    event.stopPropagation();
  }

  
  window.addEventListener("dragstart", trapDrag, true);
  synthesizeMouse(element, 2, 2, { type: "mousedown" });
  synthesizeMouse(element, 9, 9, { type: "mousemove" });
  synthesizeMouse(element, 10, 10, { type: "mousemove" });
  window.removeEventListener("dragstart", trapDrag, true);
  synthesizeMouse(element, 10, 10, { type: "mouseup" });

  var event = document.createEvent("DragEvents");
  event.initDragEvent("dragover", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  if (element.dispatchEvent(event))
    return "none";

  event = document.createEvent("DragEvents");
  event.initDragEvent("dragexit", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  element.dispatchEvent(event);

  if (dataTransfer.dropEffect != "none") {
    event = document.createEvent("DragEvents");
    event.initDragEvent("drop", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
    element.dispatchEvent(event);
  }

  return dataTransfer.dropEffect;
}

function disableNonTestMouseEvents(aDisable)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  var utils =
    window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
           getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils)
    utils.disableNonTestMouseEvents(aDisable);
}
