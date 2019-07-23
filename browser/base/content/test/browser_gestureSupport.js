










































let test_utils;

function test()
{
  
  gGestureSupport.init(false);

  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  test_utils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIDOMWindowUtils);

  
  test_EnsureConstantsAreDisjoint();
  test_TestEventListeners();
  test_TestEventCreation();

  
  gGestureSupport.init(true);
}

let test_eventCount = 0;
let test_expectedType;
let test_expectedDirection;
let test_expectedDelta;
let test_expectedModifiers;

function test_gestureListener(evt)
{
  is(evt.type, test_expectedType,
     "evt.type (" + evt.type + ") does not match expected value");
  is(evt.direction, test_expectedDirection,
     "evt.direction (" + evt.direction + ") does not match expected value");
  is(evt.delta, test_expectedDelta,
     "evt.delta (" + evt.delta + ") does not match expected value");

  is(evt.shiftKey, (test_expectedModifiers & Components.interfaces.nsIDOMNSEvent.SHIFT_MASK) != 0,
     "evt.shiftKey did not match expected value");
  is(evt.ctrlKey, (test_expectedModifiers & Components.interfaces.nsIDOMNSEvent.CONTROL_MASK) != 0,
     "evt.ctrlKey did not match expected value");
  is(evt.altKey, (test_expectedModifiers & Components.interfaces.nsIDOMNSEvent.ALT_MASK) != 0,
     "evt.altKey did not match expected value");
  is(evt.metaKey, (test_expectedModifiers & Components.interfaces.nsIDOMNSEvent.META_MASK) != 0,
     "evt.metaKey did not match expected value");

  test_eventCount++;
}

function test_helper1(type, direction, delta, modifiers)
{
  
  test_expectedType = type;
  test_expectedDirection = direction;
  test_expectedDelta = delta;
  test_expectedModifiers = modifiers;

  let expectedEventCount = test_eventCount + 1;

  document.addEventListener(type, test_gestureListener, true);
  test_utils.sendSimpleGestureEvent(type, direction, delta, modifiers);
  document.removeEventListener(type, test_gestureListener, true);

  is(expectedEventCount, test_eventCount, "Event (" + type + ") was never received by event listener");
}

function test_TestEventListeners()
{
  let e = test_helper1;  

  
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_LEFT, 0.0, 0);
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_RIGHT, 0.0, 0);
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_UP, 0.0, 0);
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_DOWN, 0.0, 0);
  e("MozSwipeGesture",
    SimpleGestureEvent.DIRECTION_UP | SimpleGestureEvent.DIRECTION_LEFT, 0.0, 0);
  e("MozSwipeGesture",
    SimpleGestureEvent.DIRECTION_DOWN | SimpleGestureEvent.DIRECTION_RIGHT, 0.0, 0);
  e("MozSwipeGesture",
    SimpleGestureEvent.DIRECTION_UP | SimpleGestureEvent.DIRECTION_RIGHT, 0.0, 0);
  e("MozSwipeGesture",
    SimpleGestureEvent.DIRECTION_DOWN | SimpleGestureEvent.DIRECTION_LEFT, 0.0, 0);

  
  e("MozMagnifyGestureStart", 0, 50.0, 0);
  e("MozMagnifyGestureUpdate", 0, -25.0, 0);
  e("MozMagnifyGestureUpdate", 0, 5.0, 0);
  e("MozMagnifyGesture", 0, 30.0, 0);

  
  e("MozRotateGestureStart", SimpleGestureEvent.DIRECTION_RIGHT, 33.0, 0);
  e("MozRotateGestureUpdate", SimpleGestureEvent.DIRECTION_LEFT, -13.0, 0);
  e("MozRotateGestureUpdate", SimpleGestureEvent.DIRECTION_RIGHT, 13.0, 0);
  e("MozRotateGesture", SimpleGestureEvent.DIRECTION_RIGHT, 33.0, 0);

  
  let modifier = Components.interfaces.nsIDOMNSEvent.SHIFT_MASK;
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_RIGHT, 0, modifier);

  
  modifier = Components.interfaces.nsIDOMNSEvent.META_MASK;
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_RIGHT, 0, modifier);

  
  modifier = Components.interfaces.nsIDOMNSEvent.ALT_MASK;
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_RIGHT, 0, modifier);

  
  modifier = Components.interfaces.nsIDOMNSEvent.CONTROL_MASK;
  e("MozSwipeGesture", SimpleGestureEvent.DIRECTION_RIGHT, 0, modifier);
}

function test_eventDispatchListener(evt)
{
  test_eventCount++;
  evt.stopPropagation();
}

function test_helper2(type, direction, delta, altKey, ctrlKey, shiftKey, metaKey)
{
  let event = null;
  let successful;

  try {
    event = document.createEvent("SimpleGestureEvent");
    successful = true;
  }
  catch (ex) {
    successful = false;
  }
  ok(successful, "Unable to create SimpleGestureEvent");

  try {
    event.initSimpleGestureEvent(type, true, true, null, 0, direction,
                                 delta, altKey, ctrlKey, shiftKey, metaKey);
    successful = true;
  }
  catch (ex) {
    successful = false;
  }
  ok(successful, "event.initSimpleGestureEvent should not fail");

  
  is(event.type, type, "Mismatch on evt.type");
  is(event.direction, direction, "Mismatch on evt.direction");
  is(event.delta, delta, "Mismatch on evt.delta");
  is(event.altKey, altKey, "Mismatch on evt.altKey");
  is(event.ctrlKey, ctrlKey, "Mismatch on evt.ctrlKey");
  is(event.shiftKey, shiftKey, "Mismatch on evt.shiftKey");
  is(event.metaKey, metaKey, "Mismatch on evt.metaKey");

  
  let expectedEventCount = test_eventCount + 1;
  document.addEventListener(type, test_eventDispatchListener, true);
  document.dispatchEvent(event);
  document.removeEventListener(type, test_eventDispatchListener, true);
  is(expectedEventCount, test_eventCount, "Dispatched event was never received by listener");
}

function test_TestEventCreation()
{
  
  test_helper2("MozMagnifyGesture", SimpleGestureEvent.DIRECTION_RIGHT, 20.0,
               true, false, true, false);
  test_helper2("MozMagnifyGesture", SimpleGestureEvent.DIRECTION_LEFT, -20.0,
               false, true, false, true);
}

function test_EnsureConstantsAreDisjoint()
{
  let up = SimpleGestureEvent.DIRECTION_UP;
  let down = SimpleGestureEvent.DIRECTION_DOWN;
  let left = SimpleGestureEvent.DIRECTION_LEFT;
  let right = SimpleGestureEvent.DIRECTION_RIGHT;

  ok(up ^ down, "DIRECTION_UP and DIRECTION_DOWN are not bitwise disjoint");
  ok(up ^ left, "DIRECTION_UP and DIRECTION_LEFT are not bitwise disjoint");
  ok(up ^ right, "DIRECTION_UP and DIRECTION_RIGHT are not bitwise disjoint");
  ok(down ^ left, "DIRECTION_DOWN and DIRECTION_LEFT are not bitwise disjoint");
  ok(down ^ right, "DIRECTION_DOWN and DIRECTION_RIGHT are not bitwise disjoint");
  ok(left ^ right, "DIRECTION_LEFT and DIRECTION_RIGHT are not bitwise disjoint");
}
