





























function getElement(id) {
  return ((typeof(id) == "string") ?
    document.getElementById(id) : id); 
};   

this.$ = this.getElement;
const KeyEvent = Components.interfaces.nsIDOMKeyEvent;

function sendMouseEvent(aEvent, aTarget, aWindow) {
  if (['click', 'dblclick', 'mousedown', 'mouseup', 'mouseover', 'mouseout'].indexOf(aEvent.type) == -1) {
    throw new Error("sendMouseEvent doesn't know about event type '" + aEvent.type + "'");
  }

  if (!aWindow) {
    aWindow = window;
  }

  if (!(aTarget instanceof Element)) {
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

  
}









function sendChar(aChar, aWindow) {
  
  var hasShift = (aChar == aChar.toUpperCase());
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
  var rect = aTarget.getBoundingClientRect();
  synthesizeMouseAtPoint(rect.left + aOffsetX, rect.top + aOffsetY,
			 aEvent, aWindow);
}












function synthesizeMouseAtPoint(left, top, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);

  if (utils) {
    var button = aEvent.button || 0;
    var clickCount = aEvent.clickCount || 1;
    var modifiers = _parseModifiers(aEvent);

    if (("type" in aEvent) && aEvent.type) {
      utils.sendMouseEvent(aEvent.type, left, top, button, clickCount, modifiers);
    }
    else {
      utils.sendMouseEvent("mousedown", left, top, button, clickCount, modifiers);
      utils.sendMouseEvent("mouseup", left, top, button, clickCount, modifiers);
    }
  }
}


function synthesizeMouseAtCenter(aTarget, aEvent, aWindow)
{
  var rect = aTarget.getBoundingClientRect();
  synthesizeMouse(aTarget, rect.width / 2, rect.height / 2, aEvent,
                  aWindow);
}
























function synthesizeMouseScroll(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);

  if (utils) {
    
    const kIsVertical = 0x02;
    const kIsHorizontal = 0x04;
    const kHasPixels = 0x08;
    const kIsMomentum = 0x40;

    var button = aEvent.button || 0;
    var modifiers = _parseModifiers(aEvent);

    var rect = aTarget.getBoundingClientRect();

    var left = rect.left;
    var top = rect.top;

    var type = (("type" in aEvent) && aEvent.type) || "DOMMouseScroll";
    var axis = aEvent.axis || "vertical";
    var scrollFlags = (axis == "horizontal") ? kIsHorizontal : kIsVertical;
    if (aEvent.hasPixels) {
      scrollFlags |= kHasPixels;
    }
    if (aEvent.isMomentum) {
      scrollFlags |= kIsMomentum;
    }
    utils.sendMouseScrollEvent(type, left + aOffsetX, top + aOffsetY, button,
                               scrollFlags, aEvent.delta, modifiers);
  }
}

function _computeKeyCodeFromChar(aChar)
{
  if (aChar.length != 1) {
    return 0;
  }
  const nsIDOMKeyEvent = Components.interfaces.nsIDOMKeyEvent;
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
    case '\n':
      return nsIDOMKeyEvent.DOM_VK_RETURN;
    default:
      return 0;
  }
}








function isKeypressFiredKey(aDOMKeyCode)
{
  const KeyEvent = Components.interfaces.nsIDOMKeyEvent;
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

    if (!("type" in aEvent) || !aEvent.type) {
      
      var keyDownDefaultHappened =
          utils.sendKeyEvent("keydown", keyCode, 0, modifiers);
      if (isKeypressFiredKey(keyCode)) {
        utils.sendKeyEvent("keypress", charCode ? 0 : keyCode, charCode,
                           modifiers, !keyDownDefaultHappened);
      }
      utils.sendKeyEvent("keyup", keyCode, 0, modifiers);
    } else if (aEvent.type == "keypress") {
      
      utils.sendKeyEvent(aEvent.type, charCode ? 0 : keyCode,
                         charCode, modifiers);
    } else {
      
      utils.sendKeyEvent(aEvent.type, keyCode, 0, modifiers);
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

  
  return aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                               getInterface(Components.interfaces.nsIDOMWindowUtils);
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













function synthesizeSelectionSet(aOffset, aLength, aReverse, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return false;
  }
  return utils.sendSelectionSetEvent(aOffset, aLength, aReverse);
}
