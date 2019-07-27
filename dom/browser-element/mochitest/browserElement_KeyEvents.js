




"use strict";

let Ci = SpecialPowers.Ci;

let whitelistedKeyCodes = [
  Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE,   
  Ci.nsIDOMKeyEvent.DOM_VK_SLEEP,    
  Ci.nsIDOMKeyEvent.DOM_VK_CONTEXT_MENU,
  Ci.nsIDOMKeyEvent.DOM_VK_F5,       
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP,  
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN 
];

let blacklistedKeyCodes = [
  Ci.nsIDOMKeyEvent.DOM_VK_A,
  Ci.nsIDOMKeyEvent.DOM_VK_B
];

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();


var nbEvents = whitelistedKeyCodes.length * 3;

var iframe;
var finished = false;
function runTest() {
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  iframe.src = browserElementTestHelpers.focusPage;

  var gotFocus = false;
  var gotLoadend = false;

  function maybeTest2() {
    if (gotFocus && gotLoadend) {
      SimpleTest.executeSoon(test2);
    }
  }

  iframe.addEventListener('mozbrowserloadend', function() {
    gotLoadend = true;
    maybeTest2();
  });

  document.body.appendChild(iframe);

  SimpleTest.waitForFocus(function() {
    iframe.focus();
    gotFocus = true;
    maybeTest2();
  });
}

function eventHandler(e) {
  if (whitelistedKeyCodes.indexOf(e.keyCode) == -1 &&
      blacklistedKeyCodes.indexOf(e.keyCode) == -1) {
    
    
    ok(true, "Ignoring unexpected " + e.type +
       " with keyCode " + e.keyCode + ".");
    return;
  }

  ok(e.type == 'keydown' || e.type == 'keypress' || e.type == 'keyup',
     "e.type was " + e.type + ", expected keydown, keypress, or keyup");
  ok(!e.defaultPrevented, "expected !e.defaultPrevented");
  ok(whitelistedKeyCodes.indexOf(e.keyCode) != -1,
     "Expected a whitelited keycode, but got " + e.keyCode + " instead.");

  nbEvents--;

  
  if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_F5) {
    e.preventDefault();
  }

  if (nbEvents == 0) {
    SimpleTest.finish();
    return;
  }

  if (nbEvents < 0 && !finished) {
    ok(false, "got an unexpected event! " + e.type + " " + e.keyCode);
  }
}

function test2() {
  is(document.activeElement, iframe, "iframe should be focused");

  addEventListener('keydown', eventHandler);
  addEventListener('keypress', eventHandler);
  addEventListener('keyup', eventHandler);

  
  synthesizeKey("VK_A", {});
  synthesizeKey("VK_B", {});

  
  synthesizeKey("VK_ESCAPE", {});

  
  synthesizeKey("VK_F5", {});
  synthesizeKey("VK_ESCAPE", {});
  synthesizeKey("VK_PAGE_UP", {});
  synthesizeKey("VK_PAGE_DOWN", {});
  synthesizeKey("VK_CONTEXT_MENU", {});
  synthesizeKey("VK_SLEEP", {});
  finished = true;
}

addEventListener('testready', runTest);
