




'use strict';

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function setup() {
  let appInfo = SpecialPowers.Cc['@mozilla.org/xre/app-info;1']
                .getService(SpecialPowers.Ci.nsIXULAppInfo);
  if (appInfo.name != 'B2G') {
    SpecialPowers.Cu.import("resource://gre/modules/Keyboard.jsm", window);
  }

  SpecialPowers.setBoolPref("dom.mozInputMethod.enabled", true);
  SpecialPowers.setBoolPref("dom.mozInputMethod.testing", true);
  SpecialPowers.addPermission('input-manage', true, document);
}

function tearDown() {
  SpecialPowers.setBoolPref("dom.mozInputMethod.enabled", false);
  SpecialPowers.setBoolPref("dom.mozInputMethod.testing", false);
  SpecialPowers.removePermission('input-manage', document);
  SimpleTest.finish();
}

function runTest() {
  let path = location.pathname;
  let imeUrl = location.protocol + '//' + location.host +
               path.substring(0, path.lastIndexOf('/')) +
               '/file_inputmethod.html';
  SpecialPowers.pushPermissions([{
    type: 'input',
    allow: true,
    context: {
      url: imeUrl,
      appId: SpecialPowers.Ci.nsIScriptSecurityManager.NO_APP_ID,
      isInBrowserElement: true
    }
  }], createFrames);
}

var gFrames = [];
var gInputFrame;

function createFrames() {
  
  let loadendCount = 0;
  let countLoadend = function() {
    ok(this.setInputMethodActive, 'Can access setInputMethodActive.');

    if (this === gInputFrame) {
      
      let appFrameScript = function appFrameScript() {
        let input = content.document.body.firstElementChild;
        input.oninput = function() {
          sendAsyncMessage('test:InputMethod:oninput', this.value);
        };

        



        content.setInterval(function() {
          input.focus();
        }, 500);
      }

      
      let mm = SpecialPowers.getBrowserFrameMessageManager(gInputFrame);
      mm.loadFrameScript('data:,(' + appFrameScript.toString() + ')();', false);
      mm.addMessageListener("test:InputMethod:oninput", next);
    }

    loadendCount++;
    if (loadendCount === 3) {
      startTest();
    }
  };

  
  gInputFrame = document.createElement('iframe');
  SpecialPowers.wrap(gInputFrame).mozbrowser = true;
  gInputFrame.src =
    'data:text/html,<input autofocus value="hello" />' +
    '<p>This is targetted mozbrowser frame.</p>';
  document.body.appendChild(gInputFrame);
  gInputFrame.addEventListener('mozbrowserloadend', countLoadend);

  for (let i = 0; i < 2; i++) {
    let frame = gFrames[i] = document.createElement('iframe');
    SpecialPowers.wrap(gFrames[i]).mozbrowser = true;
    
    
    
    frame.src = 'file_inputmethod.html#' + i;
    document.body.appendChild(frame);
    frame.addEventListener('mozbrowserloadend', countLoadend);
  }
}

function startTest() {
  
  SpecialPowers.DOMWindowUtils.focus(gInputFrame);

  let req0 = gFrames[0].setInputMethodActive(true);
  req0.onsuccess = function() {
    ok(true, 'setInputMethodActive succeeded (0).');
  };

  req0.onerror = function() {
    ok(false, 'setInputMethodActive failed (0): ' + this.error.name);
  };
}

var gTimerId = null;
var gCount = 0;

function next(msg) {
  gCount++;
  let wrappedMsg = SpecialPowers.wrap(msg);
  let value = wrappedMsg.data;
  
  
  switch (gCount) {
    case 1:
      is(value, '#0hello',
         'Failed to get correct input from the first iframe.');
      let req1 = gFrames[1].setInputMethodActive(true);
      req1.onsuccess = function() {
       ok(true, 'setInputMethodActive succeeded (1).');
      };
      req1.onerror = function() {
       ok(false, 'setInputMethodActive failed (1): ' + this.error.name);
      };
      break;

    case 2:
      is(value, '#0#1hello',
         'Failed to get correct input from the second iframe.');
      
      break;

    case 3:
      is(value, '#0#1#1hello',
         'Failed to get correct input from the second iframe.');
      
      
      let req3 = gFrames[1].setInputMethodActive(false);
      req3.onsuccess = function() {
        ok(true, 'setInputMethodActive(false) succeeded (3).');
      };
      req3.onerror = function() {
        ok(false, 'setInputMethodActive(false) failed (3): ' + this.error.name);
      };

      
      
      gTimerId = setTimeout(function() {
        ok(true, 'Successfully deactivate the second iframe.');
        tearDown();
      }, 1000);
      break;

    case 4:
      ok(false, 'Failed to deactivate the second iframe in time.');
      clearTimeout(gTimerId);
      tearDown();
      break;
  }
}

setup();
addEventListener('testready', runTest);
