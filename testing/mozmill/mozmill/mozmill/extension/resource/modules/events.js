






































var EXPORTED_SYMBOLS = ["createEventObject", "triggerEvent", "getKeyCodeFromKeySequence",
                        "triggerKeyEvent", "triggerMouseEvent", "fakeOpenPopup"];
                        
var EventUtils = {}; Components.utils.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);

var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);





var createEventObject = function(element, controlKeyDown, altKeyDown, shiftKeyDown, metaKeyDown) {
  var evt = element.ownerDocument.createEventObject();
  evt.shiftKey = shiftKeyDown;
  evt.metaKey = metaKeyDown;
  evt.altKey = altKeyDown;
  evt.ctrlKey = controlKeyDown;
  return evt;
};









function fakeOpenPopup(aWindow, aPopup) {
  var popupEvent = aWindow.document.createEvent("MouseEvent");
  popupEvent.initMouseEvent("popupshowing", true, true, aWindow, 0,
                            0, 0, 0, 0, false, false, false, false,
                            0, null);
  aPopup.dispatchEvent(popupEvent);  
}

    
var triggerEvent = function(element, eventType, canBubble, controlKeyDown, altKeyDown, shiftKeyDown, metaKeyDown) {
  canBubble = (typeof(canBubble) == undefined) ? true: canBubble;
  var evt = element.ownerDocument.createEvent('HTMLEvents');

  evt.shiftKey = shiftKeyDown;
  evt.metaKey = metaKeyDown;
  evt.altKey = altKeyDown;
  evt.ctrlKey = controlKeyDown;

  evt.initEvent(eventType, canBubble, true);
  element.dispatchEvent(evt);

};

var getKeyCodeFromKeySequence = function(keySequence) {
  
  var match = /^\\(\d{1,3})$/.exec(keySequence);
  if (match != null) {
      return match[1];

  }
  match = /^.$/.exec(keySequence);
  if (match != null) {
      return match[0].charCodeAt(0);

  }
  
  
  match = /^\d{2,3}$/.exec(keySequence);
  if (match != null) {
      return match[0];

  }
  if (keySequence != null){
    
  }
  
}

var triggerKeyEvent = function(element, eventType, aKey, modifiers, expectedEvent) {
  
  var win = element.ownerDocument ? element.ownerDocument.defaultView : element;
  win.focus();
  utils.sleep(5);

  
  if (element.ownerDocument) {
    var focusedElement = utils.getChromeWindow(win).document.commandDispatcher.focusedElement;
    for (var node = focusedElement; node && node != element; )
      node = node.parentNode;

    
    if (!node)
      element.focus();
  }

  if (expectedEvent) {
    
    if (!expectedEvent.type)
      throw new Error(arguments.callee.name + ": Expected event type not specified");

    
    var target = expectedEvent.target ? expectedEvent.target.getNode() : element;
    if (!target) {
      throw new Error(arguments.callee.name + ": could not find element " +
                      expectedEvent.target.getInfo());
    }

    EventUtils.synthesizeKeyExpectEvent(aKey, modifiers, target,
                                        expectedEvent.type,
                                        "events.triggerKeyEvent()", win);
  } else {
    EventUtils.synthesizeKey(aKey, modifiers, win);
  }
}

    
var triggerMouseEvent = function(element, eventType, canBubble, clientX, clientY, controlKeyDown, altKeyDown, shiftKeyDown, metaKeyDown) {
  
  clientX = clientX ? clientX: 0;
  clientY = clientY ? clientY: 0;

  
  
  var screenX = element.boxObject.screenX ? element.boxObject.screenX : 0;
  var screenY = element.boxObject.screenY ? element.boxObject.screenY : 0;;

  canBubble = (typeof(canBubble) == undefined) ? true: canBubble;

  var evt = element.ownerDocument.defaultView.document.createEvent('MouseEvents');
  if (evt.initMouseEvent) {
      
      
      evt.initMouseEvent(eventType, canBubble, true, element.ownerDocument.defaultView, 1, screenX, screenY, clientX, clientY, controlKeyDown, altKeyDown, shiftKeyDown, metaKeyDown, 0, null)

  }
  else {
      
      evt.initEvent(eventType, canBubble, true);
      evt.shiftKey = shiftKeyDown;
      evt.metaKey = metaKeyDown;
      evt.altKey = altKeyDown;
      evt.ctrlKey = controlKeyDown;

  }
  
  element.dispatchEvent(evt);
}
