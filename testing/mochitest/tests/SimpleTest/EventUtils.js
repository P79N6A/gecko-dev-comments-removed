
















function sendMouseEvent(aEvent, aTarget) {
  if (['click', 'mousedown', 'mouseup', 'mouseover', 'mouseout'].indexOf(aEvent.type) == -1) {
    throw new Error("sendMouseEvent doesn't know about event type '"+aEvent.type+"'");
  }

  
  netscape.security.PrivilegeManager.enablePrivilege('UniversalBrowserWrite');

  var event = document.createEvent('MouseEvent');

  var typeArg          = aEvent.type;
  var canBubbleArg     = true;
  var cancelableArg    = true;
  var viewArg          = window;
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

  document.getElementById(aTarget).dispatchEvent(event);
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

  
  if (accepted) {
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
    accepted = $(aTarget).dispatchEvent(event);
  }

  
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












function synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  var utils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                     getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils) {
    var button = aEvent.button || 0;
    var clickCount = aEvent.clickCount || 1;
    var modifiers = _parseModifiers(aEvent);

    var left = aTarget.boxObject.x;
    var top = aTarget.boxObject.y;

    if (aEvent.type) {
      utils.sendMouseEvent(aEvent.type, left + aOffsetX, top + aOffsetY, button, clickCount, modifiers);
    }
    else {
      utils.sendMouseEvent("mousedown", left + aOffsetX, top + aOffsetY, button, clickCount, modifiers);
      utils.sendMouseEvent("mouseup", left + aOffsetX, top + aOffsetY, button, clickCount, modifiers);
    }
  }
}














function synthesizeKey(aKey, aEvent)
{
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  var utils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
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
      utils.sendKeyEvent("keydown", keyCode, charCode, modifiers);
      utils.sendKeyEvent("keypress", keyCode, charCode, modifiers);
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
                                    aExpectedTarget, aExpectedEvent, aTestName)
{
  var eventHandler = _expectEvent(aExpectedTarget, aExpectedEvent, aTestName);
  synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent);
  _checkExpectedEvent(aExpectedTarget, aExpectedEvent, eventHandler, aTestName);
}












function synthesizeKeyExpectEvent(key, aEvent, aExpectedTarget, aExpectedEvent, aTestName)
{
  var eventHandler = _expectEvent(aExpectedTarget, aExpectedEvent, aTestName);
  synthesizeKey(key, aEvent);
  _checkExpectedEvent(aExpectedTarget, aExpectedEvent, eventHandler, aTestName);
}
