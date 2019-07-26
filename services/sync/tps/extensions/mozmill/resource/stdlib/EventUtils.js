
var EXPORTED_SYMBOLS = ["disableNonTestMouseEvents","sendMouseEvent", "sendChar",
                        "sendString", "sendKey", "synthesizeMouse", "synthesizeTouch",
                        "synthesizeMouseAtPoint", "synthesizeTouchAtPoint",
                        "synthesizeMouseAtCenter", "synthesizeTouchAtCenter",
                        "synthesizeWheel", "synthesizeKey",
                        "synthesizeMouseExpectEvent", "synthesizeKeyExpectEvent",
                        "synthesizeText",
                        "synthesizeComposition", "synthesizeQuerySelectedText"];

const Ci = Components.interfaces;
const Cc = Components.classes;

var window = Cc["@mozilla.org/appshell/appShellService;1"]
             .getService(Ci.nsIAppShellService).hiddenDOMWindow;

var _EU_Ci = Ci;
var navigator = window.navigator;
var KeyEvent = window.KeyEvent;
var parent = window.parent;

function is(aExpression1, aExpression2, aMessage) {
  if (aExpression1 !== aExpression2) {
    throw new Error(aMessage);
  }
}



























function getElement(id) {
  return ((typeof(id) == "string") ?
    document.getElementById(id) : id); 
};   

this.$ = this.getElement;

function sendMouseEvent(aEvent, aTarget, aWindow) {
  if (['click', 'dblclick', 'mousedown', 'mouseup', 'mouseover', 'mouseout'].indexOf(aEvent.type) == -1) {
    throw new Error("sendMouseEvent doesn't know about event type '" + aEvent.type + "'");
  }

  if (!aWindow) {
    aWindow = window;
  }

  if (!(aTarget instanceof aWindow.Element)) {
    aTarget = aWindow.document.getElementById(aTarget);
  }

  var event = aWindow.document.createEvent('MouseEvent');

  var typeArg          = aEvent.type;
  var canBubbleArg     = true;
  var cancelableArg    = true;
  var viewArg          = aWindow;
  var detailArg        = aEvent.detail        || (aEvent.type == 'click'     ||
                                                  aEvent.type == 'mousedown' ||
                                                  aEvent.type == 'mouseup' ? 1 :
                                                  aEvent.type == 'dblclick'? 2 : 0);
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

  SpecialPowers.dispatchEvent(aWindow, aTarget, event);
}









function sendChar(aChar, aWindow) {
  var hasShift;
  
  switch (aChar) {
    case "!":
    case "@":
    case "#":
    case "$":
    case "%":
    case "^":
    case "&":
    case "*":
    case "(":
    case ")":
    case "_":
    case "+":
    case "{":
    case "}":
    case ":":
    case "\"":
    case "|":
    case "<":
    case ">":
    case "?":
      hasShift = true;
      break;
    default:
      hasShift = (aChar == aChar.toUpperCase());
      break;
  }
  synthesizeKey(aChar, { shiftKey: hasShift }, aWindow);
}







function sendString(aStr, aWindow) {
  for (var i = 0; i < aStr.length; ++i) {
    sendChar(aStr.charAt(i), aWindow);
  }
}







function sendKey(aKey, aWindow) {
  var keyName = "VK_" + aKey.toUpperCase();
  synthesizeKey(keyName, { shiftKey: false }, aWindow);
}





function _parseModifiers(aEvent)
{
  const nsIDOMWindowUtils = _EU_Ci.nsIDOMWindowUtils;
  var mval = 0;
  if (aEvent.shiftKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_SHIFT;
  }
  if (aEvent.ctrlKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_CONTROL;
  }
  if (aEvent.altKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_ALT;
  }
  if (aEvent.metaKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_META;
  }
  if (aEvent.accelKey) {
    mval |= (navigator.platform.indexOf("Mac") >= 0) ?
      nsIDOMWindowUtils.MODIFIER_META : nsIDOMWindowUtils.MODIFIER_CONTROL;
  }
  if (aEvent.altGrKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_ALTGRAPH;
  }
  if (aEvent.capsLockKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_CAPSLOCK;
  }
  if (aEvent.fnKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_FN;
  }
  if (aEvent.numLockKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_NUMLOCK;
  }
  if (aEvent.scrollLockKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_SCROLLLOCK;
  }
  if (aEvent.symbolLockKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_SYMBOLLOCK;
  }
  if (aEvent.osKey) {
    mval |= nsIDOMWindowUtils.MODIFIER_OS;
  }

  return mval;
}
















function synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  var rect = aTarget.getBoundingClientRect();
  return synthesizeMouseAtPoint(rect.left + aOffsetX, rect.top + aOffsetY,
       aEvent, aWindow);
}
function synthesizeTouch(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  var rect = aTarget.getBoundingClientRect();
  synthesizeTouchAtPoint(rect.left + aOffsetX, rect.top + aOffsetY,
       aEvent, aWindow);
}












function synthesizeMouseAtPoint(left, top, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  var defaultPrevented = false;

  if (utils) {
    var button = aEvent.button || 0;
    var clickCount = aEvent.clickCount || 1;
    var modifiers = _parseModifiers(aEvent);
    var pressure = ("pressure" in aEvent) ? aEvent.pressure : 0;
    var inputSource = ("inputSource" in aEvent) ? aEvent.inputSource : 0;

    if (("type" in aEvent) && aEvent.type) {
      defaultPrevented = utils.sendMouseEvent(aEvent.type, left, top, button, clickCount, modifiers, false, pressure, inputSource);
    }
    else {
      utils.sendMouseEvent("mousedown", left, top, button, clickCount, modifiers, false, pressure, inputSource);
      utils.sendMouseEvent("mouseup", left, top, button, clickCount, modifiers, false, pressure, inputSource);
    }
  }

  return defaultPrevented;
}
function synthesizeTouchAtPoint(left, top, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);

  if (utils) {
    var id = aEvent.id || 0;
    var rx = aEvent.rx || 1;
    var ry = aEvent.rx || 1;
    var angle = aEvent.angle || 0;
    var force = aEvent.force || 1;
    var modifiers = _parseModifiers(aEvent);

    if (("type" in aEvent) && aEvent.type) {
      utils.sendTouchEvent(aEvent.type, [id], [left], [top], [rx], [ry], [angle], [force], 1, modifiers);
    }
    else {
      utils.sendTouchEvent("touchstart", [id], [left], [top], [rx], [ry], [angle], [force], 1, modifiers);
      utils.sendTouchEvent("touchend", [id], [left], [top], [rx], [ry], [angle], [force], 1, modifiers);
    }
  }
}

function synthesizeMouseAtCenter(aTarget, aEvent, aWindow)
{
  var rect = aTarget.getBoundingClientRect();
  synthesizeMouse(aTarget, rect.width / 2, rect.height / 2, aEvent,
                  aWindow);
}
function synthesizeTouchAtCenter(aTarget, aEvent, aWindow)
{
  var rect = aTarget.getBoundingClientRect();
  synthesizeTouch(aTarget, rect.width / 2, rect.height / 2, aEvent,
                  aWindow);
}


















function synthesizeWheel(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return;
  }

  var modifiers = _parseModifiers(aEvent);
  var options = 0;
  if (aEvent.isPixelOnlyDevice &&
      (aEvent.deltaMode == WheelEvent.DOM_DELTA_PIXEL)) {
    options |= utils.WHEEL_EVENT_CAUSED_BY_PIXEL_ONLY_DEVICE;
  }
  if (aEvent.isMomentum) {
    options |= utils.WHEEL_EVENT_CAUSED_BY_MOMENTUM;
  }
  if (aEvent.isCustomizedByPrefs) {
    options |= utils.WHEEL_EVENT_CUSTOMIZED_BY_USER_PREFS;
  }
  if (typeof aEvent.expectedOverflowDeltaX !== "undefined") {
    if (aEvent.expectedOverflowDeltaX === 0) {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_X_ZERO;
    } else if (aEvent.expectedOverflowDeltaX > 0) {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_X_POSITIVE;
    } else {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_X_NEGATIVE;
    }
  }
  if (typeof aEvent.expectedOverflowDeltaY !== "undefined") {
    if (aEvent.expectedOverflowDeltaY === 0) {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_Y_ZERO;
    } else if (aEvent.expectedOverflowDeltaY > 0) {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_Y_POSITIVE;
    } else {
      options |= utils.WHEEL_EVENT_EXPECTED_OVERFLOW_DELTA_Y_NEGATIVE;
    }
  }
  var isPixelOnlyDevice =
    aEvent.isPixelOnlyDevice && aEvent.deltaMode == WheelEvent.DOM_DELTA_PIXEL;

  
  if (!aEvent.deltaX) {
    aEvent.deltaX = 0;
  }
  if (!aEvent.deltaY) {
    aEvent.deltaY = 0;
  }
  if (!aEvent.deltaZ) {
    aEvent.deltaZ = 0;
  }

  var lineOrPageDeltaX =
    aEvent.lineOrPageDeltaX != null ? aEvent.lineOrPageDeltaX :
                  aEvent.deltaX > 0 ? Math.floor(aEvent.deltaX) :
                                      Math.ceil(aEvent.deltaX);
  var lineOrPageDeltaY =
    aEvent.lineOrPageDeltaY != null ? aEvent.lineOrPageDeltaY :
                  aEvent.deltaY > 0 ? Math.floor(aEvent.deltaY) :
                                      Math.ceil(aEvent.deltaY);

  var rect = aTarget.getBoundingClientRect();
  utils.sendWheelEvent(rect.left + aOffsetX, rect.top + aOffsetY,
                       aEvent.deltaX, aEvent.deltaY, aEvent.deltaZ,
                       aEvent.deltaMode, modifiers,
                       lineOrPageDeltaX, lineOrPageDeltaY, options);
}

function _computeKeyCodeFromChar(aChar)
{
  if (aChar.length != 1) {
    return 0;
  }
  const nsIDOMKeyEvent = _EU_Ci.nsIDOMKeyEvent;
  if (aChar >= 'a' && aChar <= 'z') {
    return nsIDOMKeyEvent.DOM_VK_A + aChar.charCodeAt(0) - 'a'.charCodeAt(0);
  }
  if (aChar >= 'A' && aChar <= 'Z') {
    return nsIDOMKeyEvent.DOM_VK_A + aChar.charCodeAt(0) - 'A'.charCodeAt(0);
  }
  if (aChar >= '0' && aChar <= '9') {
    return nsIDOMKeyEvent.DOM_VK_0 + aChar.charCodeAt(0) - '0'.charCodeAt(0);
  }
  
  switch (aChar) {
    case '~':
    case '`':
      return nsIDOMKeyEvent.DOM_VK_BACK_QUOTE;
    case '!':
      return nsIDOMKeyEvent.DOM_VK_1;
    case '@':
      return nsIDOMKeyEvent.DOM_VK_2;
    case '#':
      return nsIDOMKeyEvent.DOM_VK_3;
    case '$':
      return nsIDOMKeyEvent.DOM_VK_4;
    case '%':
      return nsIDOMKeyEvent.DOM_VK_5;
    case '^':
      return nsIDOMKeyEvent.DOM_VK_6;
    case '&':
      return nsIDOMKeyEvent.DOM_VK_7;
    case '*':
      return nsIDOMKeyEvent.DOM_VK_8;
    case '(':
      return nsIDOMKeyEvent.DOM_VK_9;
    case ')':
      return nsIDOMKeyEvent.DOM_VK_0;
    case '-':
    case '_':
      return nsIDOMKeyEvent.DOM_VK_SUBTRACT;
    case '+':
    case '=':
      return nsIDOMKeyEvent.DOM_VK_EQUALS;
    case '{':
    case '[':
      return nsIDOMKeyEvent.DOM_VK_OPEN_BRACKET;
    case '}':
    case ']':
      return nsIDOMKeyEvent.DOM_VK_CLOSE_BRACKET;
    case '|':
    case '\\':
      return nsIDOMKeyEvent.DOM_VK_BACK_SLASH;
    case ':':
    case ';':
      return nsIDOMKeyEvent.DOM_VK_SEMICOLON;
    case '\'':
    case '"':
      return nsIDOMKeyEvent.DOM_VK_QUOTE;
    case '<':
    case ',':
      return nsIDOMKeyEvent.DOM_VK_COMMA;
    case '>':
    case '.':
      return nsIDOMKeyEvent.DOM_VK_PERIOD;
    case '?':
    case '/':
      return nsIDOMKeyEvent.DOM_VK_SLASH;
    default:
      return 0;
  }
}








function isKeypressFiredKey(aDOMKeyCode)
{
  if (typeof(aDOMKeyCode) == "string") {
    if (aDOMKeyCode.indexOf("VK_") == 0) {
      aDOMKeyCode = KeyEvent["DOM_" + aDOMKeyCode];
      if (!aDOMKeyCode) {
        throw "Unknown key: " + aDOMKeyCode;
      }
    } else {
      
      return true;
    }
  }
  switch (aDOMKeyCode) {
    case KeyEvent.DOM_VK_SHIFT:
    case KeyEvent.DOM_VK_CONTROL:
    case KeyEvent.DOM_VK_ALT:
    case KeyEvent.DOM_VK_CAPS_LOCK:
    case KeyEvent.DOM_VK_NUM_LOCK:
    case KeyEvent.DOM_VK_SCROLL_LOCK:
    case KeyEvent.DOM_VK_META:
      return false;
    default:
      return true;
  }
}



















function synthesizeKey(aKey, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (utils) {
    var keyCode = 0, charCode = 0;
    if (aKey.indexOf("VK_") == 0) {
      keyCode = KeyEvent["DOM_" + aKey];
      if (!keyCode) {
        throw "Unknown key: " + aKey;
      }
    } else {
      charCode = aKey.charCodeAt(0);
      keyCode = _computeKeyCodeFromChar(aKey.charAt(0));
    }

    var modifiers = _parseModifiers(aEvent);
    var flags = 0;
    if (aEvent.location != undefined) {
      switch (aEvent.location) {
        case KeyboardEvent.DOM_KEY_LOCATION_STANDARD:
          flags |= utils.KEY_FLAG_LOCATION_STANDARD;
          break;
        case KeyboardEvent.DOM_KEY_LOCATION_LEFT:
          flags |= utils.KEY_FLAG_LOCATION_LEFT;
          break;
        case KeyboardEvent.DOM_KEY_LOCATION_RIGHT:
          flags |= utils.KEY_FLAG_LOCATION_RIGHT;
          break;
        case KeyboardEvent.DOM_KEY_LOCATION_NUMPAD:
          flags |= utils.KEY_FLAG_LOCATION_NUMPAD;
          break;
        case KeyboardEvent.DOM_KEY_LOCATION_MOBILE:
          flags |= utils.KEY_FLAG_LOCATION_MOBILE;
          break;
        case KeyboardEvent.DOM_KEY_LOCATION_JOYSTICK:
          flags |= utils.KEY_FLAG_LOCATION_JOYSTICK;
          break;
      }
    }

    if (!("type" in aEvent) || !aEvent.type) {
      
      var keyDownDefaultHappened =
        utils.sendKeyEvent("keydown", keyCode, 0, modifiers, flags);
      if (isKeypressFiredKey(keyCode)) {
        if (!keyDownDefaultHappened) {
          flags |= utils.KEY_FLAG_PREVENT_DEFAULT;
        }
        utils.sendKeyEvent("keypress", keyCode, charCode, modifiers, flags);
      }
      utils.sendKeyEvent("keyup", keyCode, 0, modifiers, flags);
    } else if (aEvent.type == "keypress") {
      
      utils.sendKeyEvent(aEvent.type, keyCode, charCode, modifiers, flags);
    } else {
      
      utils.sendKeyEvent(aEvent.type, keyCode, 0, modifiers, flags);
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

function disableNonTestMouseEvents(aDisable)
{
  var domutils = _getDOMWindowUtils();
  domutils.disableNonTestMouseEvents(aDisable);
}

function _getDOMWindowUtils(aWindow)
{
  if (!aWindow) {
    aWindow = window;
  }

  
  
  
  
  if ("SpecialPowers" in window && window.SpecialPowers != undefined) {
    return SpecialPowers.getDOMWindowUtils(aWindow);
  }
  if ("SpecialPowers" in parent && parent.SpecialPowers != undefined) {
    return parent.SpecialPowers.getDOMWindowUtils(aWindow);
  }

  
  return aWindow.QueryInterface(_EU_Ci.nsIInterfaceRequestor).
                               getInterface(_EU_Ci.nsIDOMWindowUtils);
}


const COMPOSITION_ATTR_RAWINPUT              = 0x02;
const COMPOSITION_ATTR_SELECTEDRAWTEXT       = 0x03;
const COMPOSITION_ATTR_CONVERTEDTEXT         = 0x04;
const COMPOSITION_ATTR_SELECTEDCONVERTEDTEXT = 0x05;















function synthesizeComposition(aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return;
  }

  utils.sendCompositionEvent(aEvent.type, aEvent.data ? aEvent.data : "",
                             aEvent.locale ? aEvent.locale : "");
}








































function synthesizeText(aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return;
  }

  if (!aEvent.composition || !aEvent.composition.clauses ||
      !aEvent.composition.clauses[0]) {
    return;
  }

  var firstClauseLength = aEvent.composition.clauses[0].length;
  var firstClauseAttr   = aEvent.composition.clauses[0].attr;
  var secondClauseLength = 0;
  var secondClauseAttr = 0;
  var thirdClauseLength = 0;
  var thirdClauseAttr = 0;
  if (aEvent.composition.clauses[1]) {
    secondClauseLength = aEvent.composition.clauses[1].length;
    secondClauseAttr   = aEvent.composition.clauses[1].attr;
    if (aEvent.composition.clauses[2]) {
      thirdClauseLength = aEvent.composition.clauses[2].length;
      thirdClauseAttr   = aEvent.composition.clauses[2].attr;
    }
  }

  var caretStart = -1;
  var caretLength = 0;
  if (aEvent.caret) {
    caretStart = aEvent.caret.start;
    caretLength = aEvent.caret.length;
  }

  utils.sendTextEvent(aEvent.composition.string,
                      firstClauseLength, firstClauseAttr,
                      secondClauseLength, secondClauseAttr,
                      thirdClauseLength, thirdClauseAttr,
                      caretStart, caretLength);
}








function synthesizeQuerySelectedText(aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return null;
  }

  return utils.sendQueryContentEvent(utils.QUERY_SELECTED_TEXT, 0, 0, 0, 0);
}
