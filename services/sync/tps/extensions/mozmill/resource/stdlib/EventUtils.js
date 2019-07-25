
var EXPORTED_SYMBOLS = ["sendMouseEvent", "sendChar", "sendString", "sendKey",
                        "synthesizeMouse", "synthesizeMouseScroll", "synthesizeKey",
                        "synthesizeMouseExpectEvent", "synthesizeKeyExpectEvent",
                        "synthesizeDragStart", "synthesizeDrop", "synthesizeText",
                        "disableNonTestMouseEvents", "synthesizeComposition", 
                        "synthesizeQuerySelectedText", "synthesizeQueryTextContent",
                        "synthesizeQueryCaretRect", "synthesizeQueryTextRect",
                        "synthesizeQueryEditorRect", "synthesizeCharAtPoint",
                        "synthesizeSelectionSet"];




function getKeyEvent(aWindow) {
  var win = aWindow.wrappedJSObject ? aWindow.wrappedJSObject : aWindow;
  return win.KeyEvent;
}



















function sendMouseEvent(aEvent, aTarget, aWindow) {
  if (['click', 'mousedown', 'mouseup', 'mouseover', 'mouseout'].indexOf(aEvent.type) == -1) {
    throw new Error("sendMouseEvent doesn't know about event type '"+aEvent.type+"'");
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

  aTarget.dispatchEvent(event);
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










function sendKey(aKey, aTarget, aWindow) {
  if (!aWindow)
    aWindow = window;

  keyName = "DOM_VK_" + aKey.toUpperCase();

  if (!getKeyEvent(aWindow)[keyName]) {
    throw "Unknown key: " + keyName;
  }

  return __doEventDispatch(aTarget, 0, getKeyEvent(aWindow)[keyName], false);
}









function __doEventDispatch(aTarget, aCharCode, aKeyCode, aHasShift) {
  if (aTarget === undefined) {
    aTarget = "target";
  }

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
  var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
                          .getService(Components.interfaces.nsIAppShellService)
                          .hiddenDOMWindow;

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
    mval |= (hwindow.navigator.platform.indexOf("Mac") >= 0) ? masks.META_MASK :
                                                               masks.CONTROL_MASK;

  return mval;
}














function synthesizeMouse(aTarget, aOffsetX, aOffsetY, aEvent, aWindow)
{
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
  if (!aWindow)
    aWindow = window;

  var utils = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                      getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils) {
    var keyCode = 0, charCode = 0;
    if (aKey.indexOf("VK_") == 0)
      keyCode = getKeyEvent(aWindow)["DOM_" + aKey];
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
    if (!epassed)
      throw new Error(aTestName + " " + type + " event target " +
                      (_gSeenEvent ? "twice" : ""));
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
    if (expectEvent)
      desc += " not";
    if (_gSeenEvent != expectEvent)
      throw new Error(aTestName + ": " + desc + " fired.");
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





















function synthesizeDragStart(element, expectedDragData, aWindow, x, y)
{
  if (!aWindow)
    aWindow = window;
  x = x || 2;
  y = y || 2;
  const step = 9;

  var result = "trapDrag was not called";
  var trapDrag = function(event) {
    try {
      var dataTransfer = event.dataTransfer;
      result = null;
      if (!dataTransfer)
        throw "no dataTransfer";
      if (expectedDragData == null ||
          dataTransfer.mozItemCount != expectedDragData.length)
        throw dataTransfer;
      for (var i = 0; i < dataTransfer.mozItemCount; i++) {
        var dtTypes = dataTransfer.mozTypesAt(i);
        if (dtTypes.length != expectedDragData[i].length)
          throw dataTransfer;
        for (var j = 0; j < dtTypes.length; j++) {
          if (dtTypes[j] != expectedDragData[i][j].type)
            throw dataTransfer;
          var dtData = dataTransfer.mozGetDataAt(dtTypes[j],i);
          if (expectedDragData[i][j].eqTest) {
            if (!expectedDragData[i][j].eqTest(dtData, expectedDragData[i][j].data))
              throw dataTransfer;
          }
          else if (expectedDragData[i][j].data != dtData)
            throw dataTransfer;
        }
      }
    } catch(ex) {
      result = ex;
    }
    event.preventDefault();
    event.stopPropagation();
  }
  aWindow.addEventListener("dragstart", trapDrag, false);
  synthesizeMouse(element, x, y, { type: "mousedown" }, aWindow);
  x += step; y += step;
  synthesizeMouse(element, x, y, { type: "mousemove" }, aWindow);
  x += step; y += step;
  synthesizeMouse(element, x, y, { type: "mousemove" }, aWindow);
  aWindow.removeEventListener("dragstart", trapDrag, false);
  synthesizeMouse(element, x, y, { type: "mouseup" }, aWindow);
  return result;
}















function synthesizeDrop(srcElement, destElement, dragData, dropEffect, aWindow)
{
  if (!aWindow)
    aWindow = window;

  var dataTransfer;
  var trapDrag = function(event) {
    dataTransfer = event.dataTransfer;
    for (var i = 0; i < dragData.length; i++) {
      var item = dragData[i];
      for (var j = 0; j < item.length; j++) {
        dataTransfer.mozSetDataAt(item[j].type, item[j].data, i);
      }
    }
    dataTransfer.dropEffect = dropEffect || "move";
    event.preventDefault();
    event.stopPropagation();
  }

  
  aWindow.addEventListener("dragstart", trapDrag, true);
  synthesizeMouse(srcElement, 2, 2, { type: "mousedown" }, aWindow);
  synthesizeMouse(srcElement, 11, 11, { type: "mousemove" }, aWindow);
  synthesizeMouse(srcElement, 20, 20, { type: "mousemove" }, aWindow);
  aWindow.removeEventListener("dragstart", trapDrag, true);

  event = aWindow.document.createEvent("DragEvents");
  event.initDragEvent("dragenter", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  destElement.dispatchEvent(event);

  var event = aWindow.document.createEvent("DragEvents");
  event.initDragEvent("dragover", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
  if (destElement.dispatchEvent(event)) {
    synthesizeMouse(destElement, 20, 20, { type: "mouseup" }, aWindow);
    return "none";
  }

  if (dataTransfer.dropEffect != "none") {
    event = aWindow.document.createEvent("DragEvents");
    event.initDragEvent("drop", true, true, aWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null, dataTransfer);
    destElement.dispatchEvent(event);
  }
  synthesizeMouse(destElement, 20, 20, { type: "mouseup" }, aWindow);

  return dataTransfer.dropEffect;
}

function disableNonTestMouseEvents(aDisable)
{
  var utils =
    window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
           getInterface(Components.interfaces.nsIDOMWindowUtils);
  if (utils)
    utils.disableNonTestMouseEvents(aDisable);
}

function _getDOMWindowUtils(aWindow)
{
  if (!aWindow) {
    aWindow = window;
  }
  return aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                 getInterface(Components.interfaces.nsIDOMWindowUtils);
}








function synthesizeComposition(aIsCompositionStart, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return;
  }

  utils.sendCompositionEvent(aIsCompositionStart ?
                               "compositionstart" : "compositionend");
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
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_SELECTED_TEXT, 0, 0, 0, 0);
}












function synthesizeQueryTextContent(aOffset, aLength, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_TEXT_CONTENT,
                                     aOffset, aLength, 0, 0);
}










function synthesizeQueryCaretRect(aOffset, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_CARET_RECT,
                                     aOffset, 0, 0, 0);
}












function synthesizeQueryTextRect(aOffset, aLength, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_TEXT_RECT,
                                     aOffset, aLength, 0, 0);
}








function synthesizeQueryEditorRect(aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_EDITOR_RECT, 0, 0, 0, 0);
}









function synthesizeCharAtPoint(aX, aY, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return nsnull;
  }
  return utils.sendQueryContentEvent(utils.QUERY_CHARACTER_AT_POINT,
                                     0, 0, aX, aY);
}













function synthesizeSelectionSet(aOffset, aLength, aReverse, aWindow)
{
  var utils = _getDOMWindowUtils(aWindow);
  if (!utils) {
    return false;
  }
  return utils.sendSelectionSetEvent(aOffset, aLength, aReverse);
}
