




'use strict';

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function setup() {
  SpecialPowers.setBoolPref("dom.mozInputMethod.enabled", true);
  SpecialPowers.setBoolPref("dom.mozInputMethod.testing", true);
  SpecialPowers.addPermission('inputmethod-manage', true, document);
}

function tearDown() {
  SpecialPowers.setBoolPref("dom.mozInputMethod.enabled", false);
  SpecialPowers.setBoolPref("dom.mozInputMethod.testing", false);
  SpecialPowers.removePermission("inputmethod-manage", document);
  SimpleTest.finish();
}

function runTest() {
  
  let input = document.createElement('input');
  input.type = 'text';
  document.body.appendChild(input);

  
  let frames = [];
  for (let i = 0; i < 2; i++) {
    frames[i] = document.createElement('iframe');
    SpecialPowers.wrap(frames[i]).mozbrowser = true;
    
    
    
    frames[i].src = 'file_inputmethod.html#' + i;
    frames[i].setAttribute('mozapp', location.href.replace(/[^/]+$/, 'file_inputmethod.webapp'));
    document.body.appendChild(frames[i]);
  }

  let count = 0;

  
  SpecialPowers.DOMWindowUtils.focus(input);
  var timerId = null;
  input.oninput = function() {
    
    
    switch (count) {
      case 1:
        is(input.value, '#0', 'Failed to get correct input from the first iframe.');
        testNextInputMethod();
        break;
      case 2:
        is(input.value, '#0#1', 'Failed to get correct input from the second iframe.');
        
        count++;
        break;
      case 3:
        is(input.value, '#0#1#1', 'Failed to get correct input from the second iframe.');
        
        count++;
        
        frames[1].setInputMethodActive(false);
        
        
        timerId = setTimeout(function() {
          ok(true, 'Successfully deactivate the second iframe.');
          tearDown();
        }, 1000);
        break;
      default:
        ok(false, 'Failed to deactivate the second iframe.');
        clearTimeout(timerId);
        tearDown();
        break;
    }
  }

  ok(frames[0].setInputMethodActive, 'Cannot access setInputMethodActive.');

  function testNextInputMethod() {
    frames[count++].setInputMethodActive(true);
  }

  
  setTimeout(function() {
    testNextInputMethod();
  }, 500);
}

setup();
addEventListener('testready', runTest);
