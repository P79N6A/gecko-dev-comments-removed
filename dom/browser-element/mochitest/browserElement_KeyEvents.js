




"use strict";

let Ci = SpecialPowers.Ci;

let whitelistedEvents = [
  Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE,   
  Ci.nsIDOMKeyEvent.DOM_VK_SLEEP,    
  Ci.nsIDOMKeyEvent.DOM_VK_CONTEXT_MENU,
  Ci.nsIDOMKeyEvent.DOM_VK_F5,       
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP,  
  Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN 
];

SimpleTest.waitForExplicitFinish();

browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();


var nbEvents = whitelistedEvents.length * 3;

var iframe;
var finished = false;
function runTest() {
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  iframe.src = browserElementTestHelpers.focusPage;
  document.body.appendChild(iframe);

  SimpleTest.waitForFocus(function() {
    iframe.focus();
    SimpleTest.executeSoon(test2);
  });
}

function eventHandler(e) {
  ok(((e.type == 'keydown' || e.type == 'keypress' || e.type == 'keyup') &&
      !e.defaultPrevented &&
      (whitelistedEvents.indexOf(e.keyCode) != -1)),
      "[ " + e.type + "] Handled event should be a non prevented key event in the white list.");

  nbEvents--;

  if (nbEvents == 0) {
    SimpleTest.finish();
    return;
  }

  if (nbEvents < 0 && !finished) {
    ok(false, "got an unexpected event! " + e.type + " " + e.keyCode);
    SimpleTest.finish();
    return;
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
