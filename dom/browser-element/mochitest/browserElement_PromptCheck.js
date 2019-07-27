









"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest()
{
  var iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');
  document.body.appendChild(iframe);

  var numPrompts = 0;
  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    is(e.detail.message, String(numPrompts), "prompt message");
    if (numPrompts / 10 < 1) {
      is(e.detail.promptType, 'alert');
    }
    else if (numPrompts / 10 < 2) {
      is(e.detail.promptType, 'confirm');
    }
    else {
      is(e.detail.promptType, 'prompt');
    }

    numPrompts++;
    if (numPrompts == 30) {
      SimpleTest.finish();
    }
  });

  iframe.src =
    'data:text/html,<html><body><script>\
      addEventListener("load", function() { \
       setTimeout(function() { \
        var i = 0; \
        for (; i < 10; i++) { alert(i); } \
        for (; i < 20; i++) { confirm(i); } \
        for (; i < 30; i++) { prompt(i); } \
       }); \
     }); \
     </scr' + 'ipt></body></html>';
}



addEventListener('testready', function() {
  SpecialPowers.pushPrefEnv({'set': [['dom.successive_dialog_time_limit', 10]]}, runTest);
});
